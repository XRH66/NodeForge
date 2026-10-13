// Native UE material construction plus a bounded JSON adapter.
// ConnectExact adapts the output-index validation and Wire core in ChiR24/Unreal_mcp
// commit 591d3b0ea3c46a2a1926dbcafc225afecb6a75b3, MaterialAuthoringHandlersConnectNodes.cpp.
// Copyright (c) 2025 Unreal Engine MCP Server Contributors. MIT:
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "NodeForgeMaterialGraph.h"
#include "NodeForgeGraphCodec.h"
#include "NodeForgeAssetDefinitions.h"
#include "NodeForgeNodeDefinitions.h"
#include "NodeForgeGraphLayout.h"
#include "NodeForgeMaterialFunctions.h"
#include "NodeForgePrototype.h"
#include "Engine/Texture2D.h"
#include "MaterialDomain.h"
#include "MaterialEditingLibrary.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionTextureBase.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Misc/PackageName.h"
#include "Misc/ScopeExit.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace NodeForgeMaterialGraph
{
using namespace NodeForgeGraphDocument;
using NodeForgeAssetDefinitions::NativeEnum;
using NodeForgeAssetDefinitions::ApplyMaterialProperty;
using NodeForgeAssetDefinitions::ReadMaterialProperty;
namespace
{
using FSpec = NodeForgeNodeDefinitions::FMaterialDefinition;
bool ExactText(const FString& A, const FString& B) { return A.Equals(B, ESearchCase::CaseSensitive); }
bool ExactContains(const TArray<FString>& Values, const FString& Text)
{
    return Values.ContainsByPredicate([&](const FString& Value) { return ExactText(Value, Text); });
}

const TArray<FSpec>& Specs() { return NodeForgeNodeDefinitions::Materials(); }
const FSpec* SpecFor(const FString& Class)
{
    const auto* Definition = NodeForgeNodeDefinitions::Material(Class);
    return Definition && Definition->Admission == NodeForgeNodeDefinitions::EAdmission::Admitted ? Definition : nullptr;
}

bool Fail(FString& Error, const FString& Text)
{
    Error = TEXT("MaterialGraph: ") + Text;
    return false;
}

bool SafeId(const FString& Id) { return NodeForgeAssetDefinitions::Identifier(Id); }

bool Fields(const FObject& Object, std::initializer_list<const TCHAR*> Required, std::initializer_list<const TCHAR*> Optional = {})
{
    if (!Object) return false;
    for (const TCHAR* Key : Required)
    {
        bool Found = false;
        for (const auto& Field : Object->Values) if (ExactText(Field.Key, Key)) { Found = true; break; }
        if (!Found) return false;
    }
    for (const auto& Field : Object->Values)
    {
        bool Known = false;
        for (const TCHAR* Key : Required) Known |= ExactText(Field.Key, Key);
        for (const TCHAR* Key : Optional) Known |= ExactText(Field.Key, Key);
        if (!Known) return false;
    }
    return true;
}

bool String(const FObject& Object, const TCHAR* Key, FString& Value)
{
    return Object && Object->HasTypedField<EJson::String>(Key) && Object->TryGetStringField(Key, Value);
}

FObject ObjectField(const FObject& Object, const TCHAR* Key)
{
    const TSharedPtr<FJsonObject>* Value = nullptr;
    return Object && Object->TryGetObjectField(Key, Value) ? *Value : nullptr;
}

UClass* Resolve(const FSpec& Spec)
{
    UClass* Class = LoadObject<UClass>(nullptr, *Spec.Class);
    return Class && Class->IsChildOf(UMaterialExpression::StaticClass()) && !Class->HasAnyClassFlags(CLASS_Abstract)
        && Class->GetPathName() == Spec.Class ? Class : nullptr;
}

TArray<FExpressionInput*> NativeInputs(UMaterialExpression* Expression)
{
    // GetInputsView may refer to a mutable cache (native MaterialFunctionCall).
    // Own the pointer array across nested queries; input objects themselves stay native.
    const auto View=Expression->GetInputsView();
    TArray<FExpressionInput*> Result;Result.Append(View.GetData(),View.Num());return Result;
}
FStructProperty* InputProperty(UMaterialExpression* Expression, FExpressionInput* Input)
{
    for (TFieldIterator<FStructProperty> It(Expression->GetClass()); It; ++It)
        if (It->Struct->GetFName() == NAME_ExpressionInput && It->ArrayDim == 1
            && It->ContainerPtrToValuePtr<FExpressionInput>(Expression) == Input) return *It;
    return nullptr;
}

FExpressionInput* FindInput(UMaterialExpression* Expression, const FString& Name)
{
    const auto Inputs = NativeInputs(Expression);
    for (int32 I = 0; I < Inputs.Num(); ++I)
        if (ExactText(Expression->GetInputName(I).ToString(), Name)) return Inputs[I];
    return nullptr;
}

FProperty* DefaultProperty(UMaterialExpression* Expression, const FSpec& Spec, const FString& InputName)
{
    FExpressionInput* Input = FindInput(Expression, InputName);
    FStructProperty* InputProp = Input ? InputProperty(Expression, Input) : nullptr;
    if (!InputProp) return nullptr;
    FProperty* Found = nullptr;
    for (const FString& Name : Spec.Properties)
    {
        FProperty* Property = Expression->GetClass()->FindPropertyByName(FName(*Name));
        if (Property && Property->HasMetaData(TEXT("OverridingInputProperty"))
            && Property->GetMetaData(TEXT("OverridingInputProperty")) == InputProp->GetName())
        {
            if (Found || !CastField<FNumericProperty>(Property)) return nullptr;
            Found = Property;
        }
    }
    return Found;
}

const TArray<TPair<FString, EMaterialProperty>>& MaterialInputs()
{
    static const TArray<TPair<FString, EMaterialProperty>> Values = {
        {TEXT("BaseColor"), MP_BaseColor}, {TEXT("EmissiveColor"), MP_EmissiveColor},
        {TEXT("Metallic"), MP_Metallic}, {TEXT("Normal"), MP_Normal}, {TEXT("Opacity"), MP_Opacity}, {TEXT("OpacityMask"), MP_OpacityMask},
        {TEXT("Roughness"), MP_Roughness}, {TEXT("Specular"), MP_Specular}, {TEXT("WorldPositionOffset"), MP_WorldPositionOffset}
    };
    return Values;
}

FExpressionInput* MaterialInput(UMaterial* Material, const FString& Name)
{
    for (const auto& Pair : MaterialInputs())
        if (ExactText(Pair.Key, Name) && Material->IsPropertyActive(Pair.Value)) return Material->GetExpressionInputForProperty(Pair.Value);
    return nullptr;
}

bool Settings(UMaterial* Material, const FMaterialDefinition& Settings, FString& Error)
{
    if (!NodeForgeAssetDefinitions::MaterialSettingsValid(Settings)) return Fail(Error, TEXT("unsupported material settings"));
    Material->MaterialDomain = MD_Surface;
    Material->BlendMode = Settings.Blend == TEXT("Opaque") ? BLEND_Opaque : Settings.Blend == TEXT("Masked") ? BLEND_Masked : BLEND_Translucent;
    Material->SetShadingModel(Settings.Shading == TEXT("Unlit") ? MSM_Unlit : MSM_DefaultLit);
    for(const auto& Option:NodeForgeAssetDefinitions::MaterialOptions())
    {
        const auto Value=NodeForgeAssetDefinitions::MaterialOption(Settings,Option.Id,true);
        FProperty* Property=Material->GetClass()->FindPropertyByName(FName(*Option.NativeName));
        if(auto* Bool=CastField<FBoolProperty>(Property)) Bool->SetPropertyValue_InContainer(Material,Value->AsBool());
        else if(auto* Float=CastField<FFloatProperty>(Property)) Float->SetPropertyValue_InContainer(Material,float(Value->AsNumber()));
        else return Fail(Error,TEXT("native material option type changed: ")+Option.NativeName);
    }
    Material->TranslucencyLightingMode = TLM_SurfacePerPixelLighting;
    return true;
}
FMaterialDefinition ActualSettings(UMaterial* Material)
{
    FMaterialDefinition Out;
    Out.Domain = Material->MaterialDomain == MD_Surface ? TEXT("Surface") : TEXT("unsupported");
    Out.Blend = Material->BlendMode == BLEND_Opaque ? TEXT("Opaque") : Material->BlendMode == BLEND_Masked ? TEXT("Masked") : Material->BlendMode == BLEND_Translucent ? TEXT("Translucent") : TEXT("unsupported");
    Out.Shading = Material->GetShadingModels().IsUnlit() ? TEXT("Unlit") : Material->GetShadingModels().HasOnlyShadingModel(MSM_DefaultLit) ? TEXT("DefaultLit") : TEXT("unsupported");
    for(const auto& Option:NodeForgeAssetDefinitions::MaterialOptions())
    {
        FProperty* Property=Material->GetClass()->FindPropertyByName(FName(*Option.NativeName));
        if(auto* Bool=CastField<FBoolProperty>(Property)) NodeForgeAssetDefinitions::SetMaterialOption(Out,Option.Id,MakeShared<FJsonValueBoolean>(Bool->GetPropertyValue_InContainer(Material)));
        else if(auto* Float=CastField<FFloatProperty>(Property)) NodeForgeAssetDefinitions::SetMaterialOption(Out,Option.Id,MakeShared<FJsonValueNumber>(Float->GetPropertyValue_InContainer(Material)));
    }
    Out.Lighting = Material->TranslucencyLightingMode == TLM_SurfacePerPixelLighting ? TEXT("SurfaceForwardShading") : TEXT("unsupported");
    return Out;
}

FObject ReadSettings(UMaterial* Material)
{
    return NodeForgeAssetDefinitions::EncodeMaterialSettings(ActualSettings(Material));
}

bool OutputIndex(UMaterialExpression* Source, const FString& Text, int32& Index)
{
    if (Text.IsEmpty() || Text.Len() > 3) return false;
    for (TCHAR C : Text) if (C < '0' || C > '9') return false;
    Index = FCString::Atoi(*Text);
    return FString::FromInt(Index) == Text && Source->GetOutputs().IsValidIndex(Index);
}

bool ConnectExact(UMaterialExpression* Source, int32 Index, FExpressionInput* Target, FString& Error)
{
    // Adapted from ChiR24 HandleConnectNodes output check and Wire (lines 140-159).
    // Native ConnectExpression carries the actual output channel mask. No aliases,
    // empty-pin fallback, unchecked struct cast, or implicit channel reinterpretation.
    if (!Source || !Target || !Source->GetOutputs().IsValidIndex(Index) || Target->Expression)
        return Fail(Error, TEXT("invalid output, input, or duplicate destination"));
    Source->ConnectExpression(Target, Index);
    const FExpressionOutput& Output = Source->GetOutputs()[Index];
    return (Target->Expression == Source && Target->OutputIndex == Index && Target->Mask == Output.Mask
        && Target->MaskR == Output.MaskR && Target->MaskG == Output.MaskG
        && Target->MaskB == Output.MaskB && Target->MaskA == Output.MaskA)
        || Fail(Error, TEXT("native connection readback mismatch"));
}

struct FGraph
{
    TMap<FString, UMaterialExpression*> Nodes;
    TMap<FString, const FNodeDescription*> Definitions;
    FObject Layout;
};

bool Populate(UMaterial* Material, const FGraphDocument& Request, FGraph& Graph, FString& Error)
{
    const auto* Asset = Request.Asset.TryGet<FMaterialDefinition>();
    const auto Capabilities = NodeForgeNodeDefinitions::Capabilities(Request.CatalogDigest);
    if (!Capabilities.IsKnown() || !Asset || Request.Graphs.Num() != 1 || !Settings(Material, *Asset, Error))
        return Error.IsEmpty() ? Fail(Error, TEXT("invalid material document")) : false;
    const auto& Definition = Request.Graphs[0];
    TSet<FName> ParameterNames;
    for (const FNodeDescription& Node : Definition.Nodes)
    {
        const FString& Id = Node.Id; const FString& ClassPath = Node.Class;
        if (Id == TEXT("Material") || Graph.Nodes.Contains(Id)) return Fail(Error, TEXT("invalid/duplicate material node"));
        const FSpec* Spec = SpecFor(ClassPath);
        if (Spec && Capabilities.MaterialRevision < Spec->MinimumRevision) return Fail(Error, Id + TEXT(" class unavailable in archived catalog"));
        UClass* Class = Spec ? Resolve(*Spec) : nullptr;
        if (!Class) return Fail(Error, TEXT("unsupported node class: ") + ClassPath);
        const FValueMap& Properties = Node.Properties; const FValueMap& Defaults = Node.Defaults;
        const FIntPoint Position = Node.Position.Get(FIntPoint::ZeroValue);
        UMaterialExpression* Expression = UMaterialEditingLibrary::CreateMaterialExpression(Material, Class, Position.X, Position.Y);
        if (!Expression) return Fail(Error, Id + TEXT(" native expression creation failed"));
        Expression->Desc = TEXT("NodeForge:") + Id;
        Graph.Nodes.Add(Id, Expression); Graph.Definitions.Add(Id, &Node);
        if (Spec->RequiresFunctionBinding)
        {
            if (!Defaults.IsEmpty()) return Fail(Error, Id + TEXT(" function defaults require explicit connected nodes"));
            if (!NodeForgeMaterialFunctions::Bind(Cast<UMaterialExpressionMaterialFunctionCall>(Expression), Node.Binding, Error, Capabilities.ResourceReadRevision)) return false;
            continue;
        }
        TArray<FString> PropertyNames; Properties.GetKeys(PropertyNames); PropertyNames.Sort();
        for (const FString& Key : PropertyNames)
        {
            if (!ExactContains(Spec->Properties, Key)) return Fail(Error, Id + TEXT(" unsupported property: ") + Key);
            if (!ApplyMaterialProperty(Expression, Class->FindPropertyByName(FName(*Key)), Properties[Key], Error, Capabilities.ResourceReadRevision)) return false;
        }
        for (const auto& Default : Defaults)
        {
            FProperty* Property = DefaultProperty(Expression, *Spec, Default.Key);
            if (!Property) return Fail(Error, Id + TEXT(" input has no supported single native default: ") + Default.Key);
            if (Properties.Contains(Property->GetName())) return Fail(Error, Id + TEXT(" duplicate default/property assignment: ") + Default.Key);
            if (!ApplyMaterialProperty(Expression, Property, Default.Value, Error, Capabilities.ResourceReadRevision)) return false;
        }
        if (FNameProperty* Parameter = CastField<FNameProperty>(Class->FindPropertyByName(TEXT("ParameterName"))))
        {
            const FName Name = Parameter->GetPropertyValue_InContainer(Expression);
            if (!Properties.Contains(TEXT("ParameterName")) || Name.IsNone() || ParameterNames.Contains(Name)) return Fail(Error, Id + TEXT(" parameter requires unique explicit ParameterName"));
            ParameterNames.Add(Name);
        }
        if (ClassPath.EndsWith(TEXT("ComponentMask")))
        {
            bool Any = false;
            for (const TCHAR* Channel : {TEXT("R"),TEXT("G"),TEXT("B"),TEXT("A")})
                if (FBoolProperty* P = CastField<FBoolProperty>(Class->FindPropertyByName(Channel))) Any |= P->GetPropertyValue_InContainer(Expression);
            if (!Any) return Fail(Error, Id + TEXT(" component mask selects no channels"));
        }
        if (auto* Texture = Cast<UMaterialExpressionTextureBase>(Expression))
        {
            if (!Properties.Contains(TEXT("Texture")) || !Texture->Texture) return Fail(Error, Id + TEXT(" requires explicit existing Texture"));
            const auto Expected = UMaterialExpressionTextureBase::GetSamplerTypeForTexture(Texture->Texture);
            if (Properties.Contains(TEXT("SamplerType")) && Texture->SamplerType != Expected) return Fail(Error, Id + TEXT(" SamplerType does not match texture"));
            if (!Properties.Contains(TEXT("SamplerType"))) Texture->AutoSetSampleType();
        }
    }
    TMap<FString,TArray<FString>> Children, Parents;
    TMap<FString,int32> Incoming;
    TArray<FString> OutputSources;
    for (const auto& Node : Graph.Nodes) Incoming.Add(Node.Key,0);
    TSet<FString> Destinations;
    int32 RootEdges = 0;
    for (const FEdgeDescription& Edge : Definition.Edges)
    {
        const FString& From = Edge.From; const FString& Output = Edge.Output;
        const FString& To = Edge.To; const FString& Input = Edge.Input;
        UMaterialExpression* Source = Graph.Nodes.FindRef(From);
        int32 Index = 0;
        if (!Source || !ExactText(Graph.Definitions.FindChecked(From)->Id,From)
            || !OutputIndex(Source,Output,Index)) return Fail(Error,TEXT("missing source or exact output index: ") + From + TEXT(".") + Output);
        const FString Destination = To + TEXT(".") + Input;
        if (Destinations.Contains(Destination)) return Fail(Error,TEXT("duplicate input destination: ") + Destination);
        Destinations.Add(Destination);
        FExpressionInput* Target = nullptr;
        if (ExactText(To,TEXT("Material")))
        {
            Target = MaterialInput(Material,Input); ++RootEdges;
            OutputSources.AddUnique(From);
        }
        else
        {
            UMaterialExpression* TargetExpression = Graph.Nodes.FindRef(To);
            if (TargetExpression && !ExactText(Graph.Definitions.FindChecked(To)->Id,To))
                return Fail(Error,TEXT("noncanonical target node ID: ") + To);
            const FSpec* TargetSpec = TargetExpression ? SpecFor(TargetExpression->GetClass()->GetPathName()) : nullptr;
            if (TargetSpec && !TargetSpec->ConsumesInputs)
                return Fail(Error,TEXT("Texture object parameters do not consume inherited UV/mip inputs; connect the object to a sampler or function instead: ") + Destination);
            if (TargetExpression) Target = FindInput(TargetExpression,Input);
            const FNodeDescription* TargetDefinition = Graph.Definitions.FindRef(To);
            if (TargetDefinition && TargetDefinition->Defaults.Contains(Input)) return Fail(Error,TEXT("input cannot have both edge and explicit default: ") + Destination);
            if (TargetExpression)
            {
                Children.FindOrAdd(From).Add(To); Parents.FindOrAdd(To).Add(From);
                Incoming.FindChecked(To)++;
            }
        }
        if (!Target) return Fail(Error,TEXT("unknown or inactive target input: ") + Destination);
        if (!ConnectExact(Source,Index,Target,Error)) return false;
    }
    if (!RootEdges) return Fail(Error,TEXT("material needs at least one active output connection"));
    TArray<FString> Queue;
    for (const auto& Entry : Incoming) if (!Entry.Value) Queue.Add(Entry.Key);
    int32 Visited = 0;
    for (int32 I = 0; I < Queue.Num(); ++I)
    {
        ++Visited;
        for (const FString& Child : Children.FindOrAdd(Queue[I])) if (--Incoming.FindChecked(Child) == 0) Queue.Add(Child);
    }
    if (Visited != Graph.Nodes.Num()) return Fail(Error,TEXT("cycle in material expression graph"));
    // Native shader compilation only visits expressions feeding active outputs. Require every
    // declared expression to participate, so a disconnected invalid subgraph cannot escape it.
    // This is a graph rule for all materials; no nodes or edges are deleted or inserted.
    TSet<FString> ReachesOutput;
    for (const FString& Id : OutputSources) ReachesOutput.Add(Id);
    for (int32 I = 0; I < OutputSources.Num(); ++I)
    {
        for (const FString& Parent : Parents.FindOrAdd(OutputSources[I]))
            if (!ReachesOutput.Contains(Parent))
            { ReachesOutput.Add(Parent); OutputSources.Add(Parent); }
    }
    if (ReachesOutput.Num() != Graph.Nodes.Num())
    {
        TArray<FString> Unreachable;
        for (const auto& Entry : Graph.Nodes) if (!ReachesOutput.Contains(Entry.Key)) Unreachable.Add(Entry.Key);
        Unreachable.Sort();
        return Fail(Error,TEXT("declared expressions do not reach an active material output: ") + FString::Join(Unreachable,TEXT(", ")));
    }
    for (const auto& Entry : Graph.Nodes)
    {
        UMaterialExpression* Expression = Entry.Value;
        const auto Inputs = NativeInputs(Expression);
        for (int32 I = 0; I < Inputs.Num(); ++I)
        {
            FStructProperty* Property = InputProperty(Expression,Inputs[I]);
            const bool Required = Property ? (!Property->HasMetaData(TEXT("RequiredInput")) || Property->GetBoolMetaData(TEXT("RequiredInput"))) : Expression->IsInputConnectionRequired(I);
            if (Required && !Inputs[I]->Expression) return Fail(Error,Entry.Key + TEXT(" missing required input: ") + Expression->GetInputName(I).ToString());
        }
    }
    return NodeForgeGraphLayout::Material(Material,Request,Graph.Nodes,Graph.Layout,Error);
}

UMaterial* NewMaterial(UPackage* Package, const FString& Name)
{
    const EObjectFlags Flags = Package == GetTransientPackage() ? RF_Transient : RF_Public | RF_Standalone | RF_Transactional;
    return NewObject<UMaterial>(Package, FName(*Name), Flags);
}

FString TemporaryName()
{
    return MakeUniqueObjectName(GetTransientPackage(),UMaterial::StaticClass(),TEXT("NFGraphInspection")).ToString();
}

FObject EffectiveProperties(UMaterialExpression* Expression, const FSpec& Spec)
{
    if(Spec.RequiresFunctionBinding) return NodeForgeMaterialFunctions::Properties(Cast<UMaterialExpressionMaterialFunctionCall>(Expression));
    FObject Properties = MakeShared<FJsonObject>();
    for (const FString& Name : Spec.Properties)
        if (FProperty* Property = Expression->GetClass()->FindPropertyByName(FName(*Name))) Properties->SetField(Name,ReadMaterialProperty(Expression,Property));
    return Properties;
}

bool SameInput(const FExpressionInput* Actual, const FExpressionInput* Expected,
    const TMap<UMaterialExpression*,FString>& ActualIds, const TMap<UMaterialExpression*,FString>& ExpectedIds)
{
    if (!Actual || !Expected) return Actual == Expected;
    if (!Actual->Expression || !Expected->Expression) return Actual->Expression == Expected->Expression;
    const FString* ActualId = ActualIds.Find(Actual->Expression);
    const FString* ExpectedId = ExpectedIds.Find(Expected->Expression);
    return ActualId && ExpectedId && ExactText(*ActualId,*ExpectedId) && Actual->OutputIndex == Expected->OutputIndex
        && Actual->Mask == Expected->Mask && Actual->MaskR == Expected->MaskR && Actual->MaskG == Expected->MaskG
        && Actual->MaskB == Expected->MaskB && Actual->MaskA == Expected->MaskA;
}

void AppendEdge(TArray<TSharedPtr<FJsonValue>>& Edges, FExpressionInput* Input, const FString& To,
    const FString& Pin, const TMap<UMaterialExpression*,FString>& Ids)
{
    if (!Input || !Input->Expression) return;
    FObject Edge = MakeShared<FJsonObject>();
    const FString* SourceId = Ids.Find(Input->Expression);
    Edge->SetStringField(TEXT("from"),SourceId ? *SourceId : TEXT("UNDECLARED"));
    Edge->SetStringField(TEXT("output"),FString::FromInt(Input->OutputIndex));
    Edge->SetStringField(TEXT("to"),To); Edge->SetStringField(TEXT("input"),Pin);
    Edge->SetNumberField(TEXT("mask"),Input->Mask);
    Edge->SetNumberField(TEXT("maskR"),Input->MaskR); Edge->SetNumberField(TEXT("maskG"),Input->MaskG);
    Edge->SetNumberField(TEXT("maskB"),Input->MaskB); Edge->SetNumberField(TEXT("maskA"),Input->MaskA);
    Edges.Add(MakeShared<FJsonValueObject>(Edge));
}
}

TSharedRef<FJsonObject> Catalog()
{
    check(IsInGameThread());
    bool ExistingSignaturesValid = true;
    TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("adapter"),TEXT("NodeForgeMaterialGraph2"));
    Result->SetStringField(TEXT("inputIdentity"),TEXT("exact native GetInputName; output is canonical numeric string index"));
    Result->SetStringField(TEXT("verification"),TEXT("catalog construction is not graph compilation/runtime verification; consult per-asset reports"));
    Result->SetStringField(TEXT("defaultPolicy"),TEXT("omitted allowed properties use recorded native defaults; defaults map only unique numeric OverridingInputProperty; no inserted nodes"));
    Result->SetStringField(TEXT("reachabilityPolicy"),TEXT("every declared expression must reach an active material output; disconnected expressions are rejected, never deleted"));
    Result->SetStringField(TEXT("texturePolicy"),TEXT("saved clean exact nonvirtual Texture2D; current resource-read revision uses explicit installation read roots; archived catalogs retain original Engine/test-root admission; native sampler inferred if omitted and read back"));
    Result->SetNumberField(TEXT("resourceReadRevision"),1);
    Result->SetStringField(TEXT("compatibleCatalogDigest"),LegacyCatalogDigest);
    Result->SetStringField(TEXT("compatibilityPolicy"),TEXT("archived digests retain their original material revision: 0 original, 1 extended families, 2 function binding, 3 texture composition, 4 spatial inputs, 5 color selection, 6 bulk ordinary Surface Math; each expression declares minimumMaterialRevision; no input rewriting"));
    Result->SetStringField(TEXT("spatialPolicy"),TEXT("native spatial inputs, Surface materials only; world/object position origins use exact native enum names; normals/directions are world space, not automatically transformed to tangent space; native shader compilation determines stage/property validity, PixelNormalWS is not valid for Normal or vertex WorldPositionOffset"));
    Result->SetNumberField(TEXT("maximumNodes"),256); Result->SetNumberField(TEXT("maximumEdges"),1024);
    UMaterial* Material = NewMaterial(GetTransientPackage(),TemporaryName());
    TArray<TSharedPtr<FJsonValue>> Entries;
    for (const FSpec& Spec : Specs())
    {
        FObject Entry = MakeShared<FJsonObject>();
        Entry->SetStringField(TEXT("class"),Spec.Class);
        UClass* Class = Resolve(Spec);
        UMaterialExpression* Expression = Class ? UMaterialEditingLibrary::CreateMaterialExpression(Material,Class) : nullptr;
        FString QualificationError;
        if (!Expression) QualificationError = TEXT("native expression class/factory unavailable");
        else if (Spec.IsBulk)
        {
            TSet<FString> Names;
            const auto Inputs = NativeInputs(Expression);
            for (int32 Index = 0; Index < Inputs.Num(); ++Index)
            {
                const FString Name = Expression->GetInputName(Index).ToString();
                if (Name.IsEmpty() || Names.Contains(Name) || !InputProperty(Expression, Inputs[Index]))
                { QualificationError = TEXT("ambiguous or non-reflected fixed input: ") + Name; break; }
                Names.Add(Name);
            }
            if (QualificationError.IsEmpty() && Expression->GetOutputs().IsEmpty())
                QualificationError = TEXT("ordinary expression requires native numeric output");
            for (int32 Index = 0; QualificationError.IsEmpty() && Index < Expression->GetOutputs().Num(); ++Index)
                if (Expression->IsResultMaterialAttributes(Index) || !(Expression->GetOutputType(Index) & MCT_Float))
                    QualificationError = TEXT("requires nonordinary native output semantics");
            for (const FString& Name : Spec.Properties)
            {
                if (!QualificationError.IsEmpty()) break;
                FProperty* Property = Class->FindPropertyByName(FName(*Name));
                if (!Property) { QualificationError = TEXT("declared scalar property absent: ") + Name; break; }
                const auto Value = ReadMaterialProperty(Expression, Property);
                if (!Value || Value->Type == EJson::Null)
                { QualificationError = TEXT("unrepresentable scalar default: ") + Name; break; }
                if (!ApplyMaterialProperty(Expression, Property, Value, QualificationError)) break;
                const auto Actual = ReadMaterialProperty(Expression, Property);
                const bool Equal = Actual && Actual->Type == Value->Type
                    && ((Value->Type == EJson::Boolean && Actual->AsBool() == Value->AsBool())
                        || (Value->Type == EJson::Number && Actual->AsNumber() == Value->AsNumber()));
                if (!Equal) QualificationError = TEXT("native scalar property default roundtrip differs: ") + Name;
            }
        }
        NodeForgeNodeDefinitions::QualifyMaterial(Spec.Class, QualificationError);
        if (!QualificationError.IsEmpty())
        {
            if (Spec.IsBulk) continue; // Qualification failure remains in the complete exclusions.
            ExistingSignaturesValid = false; Entry->SetStringField(TEXT("error"),QualificationError);
        }
        Entry->SetBoolField(TEXT("discovered"),Class != nullptr); Entry->SetBoolField(TEXT("constructible"),Expression != nullptr);
        Entry->SetStringField(TEXT("family"),Spec.Family.IsEmpty()?TEXT("ExistingMaterialFamilies"):Spec.Family);
        Entry->SetNumberField(TEXT("minimumMaterialRevision"),Spec.MinimumRevision);
        Entry->SetStringField(TEXT("supportState"),Expression?TEXT("experimental"):TEXT("discovered"));
        Entry->SetStringField(TEXT("verified"),TEXT("notEstablishedByCatalog"));
        if (Expression)
        {
            if(Spec.RequiresFunctionBinding)
            {
                Entry->SetStringField(TEXT("signaturePolicy"),TEXT("dynamic: MaterialFunction inspection binds saved actual function and exports native names/indices/GUIDs; unbound inputs/outputs below are not usable signatures"));
                Entry->SetStringField(TEXT("configuration"),TEXT("exactly MaterialFunction canonical path and FunctionAssetSha256; no function instances/layers; no per-call defaults"));
            }
            TArray<TSharedPtr<FJsonValue>> Inputs, Outputs;
            const auto ActualInputs = NativeInputs(Expression);
            for (int32 I = 0; I < ActualInputs.Num(); ++I)
            {
                FObject Input = MakeShared<FJsonObject>();
                const FString Name = Expression->GetInputName(I).ToString();
                Input->SetStringField(TEXT("name"),Name);
                if (!Spec.ConsumesInputs)
                {
                    Input->SetBoolField(TEXT("connectible"),false);
                    Input->SetStringField(TEXT("adapterLimitation"),TEXT("inherited native sample input is not consumed by TextureObjectParameter::Compile; connections rejected"));
                }
                Input->SetNumberField(TEXT("nativeType"),Expression->GetInputType(I));
                FStructProperty* P = InputProperty(Expression,ActualInputs[I]);
                Input->SetBoolField(TEXT("required"),P ? (!P->HasMetaData(TEXT("RequiredInput")) || P->GetBoolMetaData(TEXT("RequiredInput"))) : Expression->IsInputConnectionRequired(I));
                if (FProperty* Default = DefaultProperty(Expression,Spec,Name))
                {
                    Input->SetStringField(TEXT("defaultProperty"),Default->GetName());
                    Input->SetField(TEXT("defaultValue"),ReadMaterialProperty(Expression,Default));
                }
                Inputs.Add(MakeShared<FJsonValueObject>(Input));
            }
            for (int32 I = 0; I < Expression->GetOutputs().Num(); ++I)
            {
                const FExpressionOutput& Native = Expression->GetOutputs()[I];
                FObject Output = MakeShared<FJsonObject>();
                Output->SetStringField(TEXT("index"),FString::FromInt(I)); Output->SetStringField(TEXT("name"),Native.OutputName.ToString());
                Output->SetNumberField(TEXT("nativeType"),Expression->GetOutputType(I));
                Output->SetNumberField(TEXT("mask"),Native.Mask); Output->SetNumberField(TEXT("maskR"),Native.MaskR);
                Output->SetNumberField(TEXT("maskG"),Native.MaskG); Output->SetNumberField(TEXT("maskB"),Native.MaskB); Output->SetNumberField(TEXT("maskA"),Native.MaskA);
                Outputs.Add(MakeShared<FJsonValueObject>(Output));
            }
            Entry->SetArrayField(TEXT("inputs"),Inputs); Entry->SetArrayField(TEXT("outputs"),Outputs);
            FObject Properties = MakeShared<FJsonObject>();
            for (const FString& Name : Spec.Properties)
            {
                FObject Description = MakeShared<FJsonObject>();
                FProperty* Property = Class->FindPropertyByName(FName(*Name));
                if(Spec.RequiresFunctionBinding)
                {
                    Description->SetStringField(TEXT("nativeType"),Name==TEXT("MaterialFunction")?TEXT("saved UMaterialFunction reference bound by SetMaterialFunction"):TEXT("adapter-owned direct asset byte SHA256 guard"));
                    Description->SetBoolField(TEXT("required"),true);
                }
                else if (!Property) { Description->SetStringField(TEXT("error"),TEXT("native property absent")); }
                else
                {
                    Description->SetStringField(TEXT("nativeType"),Property->GetCPPType());
                    Description->SetField(TEXT("default"),ReadMaterialProperty(Expression,Property));
                    Description->SetObjectField(TEXT("valueSchema"),NodeForgeAssetDefinitions::MaterialPropertySchema(Property));
                    if (UEnum* Enum = NativeEnum(Property))
                    {
                        TArray<TSharedPtr<FJsonValue>> EnumNames;
                        for (int32 I = 0; I < Enum->NumEnums(); ++I)
                        {
                            const FString EnumName = Enum->GetNameStringByIndex(I);
                            if (!EnumName.EndsWith(TEXT("MAX")) && !EnumName.Contains(TEXT("Virtual"))) EnumNames.Add(MakeShared<FJsonValueString>(EnumName));
                        }
                        Description->SetArrayField(TEXT("enumNames"),EnumNames);
                    }
                }
                Properties->SetObjectField(Name,Description);
            }
            Entry->SetObjectField(TEXT("properties"),Properties);
        }
        Entries.Add(MakeShared<FJsonValueObject>(Entry));
    }
    Result->SetArrayField(TEXT("nodes"),Entries);
    Result->SetBoolField(TEXT("nativeQualificationPassed"),ExistingSignaturesValid);
    Result->SetArrayField(TEXT("excludedCandidates"),NodeForgeNodeDefinitions::Exclusions(TEXT("Material")));
    Result->SetStringField(TEXT("definitionPolicy"),TEXT("Shared existing/special definitions plus ordinary native Surface Math expressions; fixed native ports and scalar/bool properties qualified with the production factory/property adapter. Export preserves unknown/nondefault configuration protection."));
    FObject SettingRules = MakeShared<FJsonObject>();
    for(const auto& Field:NodeForgeAssetDefinitions::Fields(NodeForgeAssetDefinitions::EFields::MaterialSettings))
        if(!Field.EnumValues.IsEmpty()) SettingRules->SetStringField(Field.Name,FString::Join(Field.EnumValues,TEXT("|")));
    SettingRules->SetObjectField(TEXT("schema"),NodeForgeAssetDefinitions::MaterialSettingsSchema());
    SettingRules->SetStringField(TEXT("activePolicy"),TEXT("IsPropertyActive controls root connections; inactive clip/dither values remain stored. Masked requires clip0..1; negative emissive true only Unlit. No visual/TAA/LOD acceptance from setting values."));
    Result->SetObjectField(TEXT("settings"),SettingRules);
    TArray<TSharedPtr<FJsonValue>> Outputs;
    for (const auto& Pair : MaterialInputs()) Outputs.Add(MakeShared<FJsonValueString>(Pair.Key));
    Result->SetArrayField(TEXT("materialInputs"),Outputs);
    Material->MarkAsGarbage();
    return Result;
}

bool Validate(const FGraphDocument& Request, FString& Error)
{
    Error.Reset();
    UMaterial* Material = NewMaterial(GetTransientPackage(),TemporaryName());
    FGraph Graph;
    const bool Valid = Material && Populate(Material,Request,Graph,Error);
    if (Material) Material->MarkAsGarbage();
    return Valid;
}

UMaterial* Build(UPackage* Package, const FString& Name, const FGraphDocument& Request, FString& Error)
{
    Error.Reset();
    if (!Package || FindObject<UObject>(Package,*Name)) { Fail(Error,TEXT("candidate package/name unavailable")); return nullptr; }
    UMaterial* Material = NewMaterial(Package,Name);
    FGraph Graph;
    if (!Material || !Populate(Material,Request,Graph,Error)) return nullptr;
    return Material;
}

bool Readback(UMaterial* Material, const FGraphDocument& Request, FObject& Evidence, FString& Error)
{
    Error.Reset(); Evidence = MakeShared<FJsonObject>();
    if (!Material) return Fail(Error,TEXT("null material readback"));
    UMaterial* Expected = NewMaterial(GetTransientPackage(),TemporaryName());
    FGraph ExpectedGraph;
    if (!Populate(Expected,Request,ExpectedGraph,Error)) { Expected->MarkAsGarbage(); return false; }
    FGraph ActualGraph;
    TMap<UMaterialExpression*,FString> ActualIds, ExpectedIds;
    bool Match = true;
    for (UMaterialExpression* Expression : Material->GetExpressions())
    {
        const FString Id = Expression && Expression->Desc.StartsWith(TEXT("NodeForge:")) ? Expression->Desc.Mid(10) : FString();
        if (!SafeId(Id) || ActualGraph.Nodes.Contains(Id)) { Match = false; continue; }
        ActualGraph.Nodes.Add(Id,Expression); ActualIds.Add(Expression,Id);
    }
    for (const auto& Pair : ExpectedGraph.Nodes) ExpectedIds.Add(Pair.Value,Pair.Key);
    Match &= ActualGraph.Nodes.Num() == ExpectedGraph.Nodes.Num() && Material->GetExpressions().Num() == ExpectedGraph.Nodes.Num();
    Match &= Material->MaterialDomain == Expected->MaterialDomain && Material->BlendMode == Expected->BlendMode
        && Material->GetShadingModels() == Expected->GetShadingModels() && Material->TwoSided == Expected->TwoSided
        && Material->TranslucencyLightingMode == Expected->TranslucencyLightingMode;
    for(const auto& Option:NodeForgeAssetDefinitions::MaterialOptions())
    {
        FProperty* Property=Material->GetClass()->FindPropertyByName(FName(*Option.NativeName));
        Match &= Property && Property->Identical_InContainer(Material,Expected);
    }
    TArray<FString> SortedIds; ActualGraph.Nodes.GetKeys(SortedIds); SortedIds.Sort();
    TArray<TSharedPtr<FJsonValue>> Nodes, Edges;
    for (const FString& Id : SortedIds)
    {
        UMaterialExpression* Actual = ActualGraph.Nodes[Id];
        UMaterialExpression* Wanted = ExpectedGraph.Nodes.FindRef(Id);
        const FSpec* Spec = SpecFor(Actual->GetClass()->GetPathName());
        FObject Node = MakeShared<FJsonObject>();
        Node->SetStringField(TEXT("id"),Id); Node->SetStringField(TEXT("class"),Actual->GetClass()->GetPathName());
        if (!Wanted || !Spec || Actual->GetClass() != Wanted->GetClass()
            || !ExactText(Id,Wanted->Desc.Mid(FString(TEXT("NodeForge:")).Len()))) { Match = false; Nodes.Add(MakeShared<FJsonValueObject>(Node)); continue; }
        Node->SetObjectField(TEXT("properties"),EffectiveProperties(Actual,*Spec));
        if(Spec->RequiresFunctionBinding)
        {
            auto* A=Cast<UMaterialExpressionMaterialFunctionCall>(Actual);
            auto* B=Cast<UMaterialExpressionMaterialFunctionCall>(Wanted);
            Match &= NodeForgeMaterialFunctions::Matches(A,B);
            Node->SetObjectField(TEXT("functionSignature"),NodeForgeMaterialFunctions::Signature(A));
        }
        FObject Defaults = MakeShared<FJsonObject>();
        for (const FString& Name : Spec->Properties)
        {
            FProperty* Property = Actual->GetClass()->FindPropertyByName(FName(*Name));
            if(Spec->RequiresFunctionBinding && Name==TEXT("FunctionAssetSha256")) continue;
            if (!Property || !Property->Identical_InContainer(Actual,Wanted)) Match = false;
        }
        Match &= Actual->MaterialExpressionEditorX == Wanted->MaterialExpressionEditorX && Actual->MaterialExpressionEditorY == Wanted->MaterialExpressionEditorY;
        Node->SetArrayField(TEXT("position"),{MakeShared<FJsonValueNumber>(Actual->MaterialExpressionEditorX),MakeShared<FJsonValueNumber>(Actual->MaterialExpressionEditorY)});
        const auto Inputs = NativeInputs(Actual);
        Match &= Inputs.Num() == Wanted->GetInputsView().Num();
        for (int32 I = 0; I < Inputs.Num(); ++I)
        {
            const FString InputName = Actual->GetInputName(I).ToString();
            if (!SameInput(Inputs[I],FindInput(Wanted,InputName),ActualIds,ExpectedIds)) Match = false;
            if(Inputs[I]->Expression && !ActualIds.Contains(Inputs[I]->Expression)) Match = false;
            if (!Inputs[I]->Expression)
                if (FProperty* P = DefaultProperty(Actual,*Spec,InputName)) Defaults->SetField(InputName,ReadMaterialProperty(Actual,P));
            AppendEdge(Edges,Inputs[I],Id,InputName,ActualIds);
        }
        Node->SetObjectField(TEXT("defaults"),Defaults);
        Nodes.Add(MakeShared<FJsonValueObject>(Node));
    }
    for (const auto& Pair : MaterialInputs())
    {
        FExpressionInput* Actual = Material->GetExpressionInputForProperty(Pair.Value);
        FExpressionInput* Wanted = Expected->GetExpressionInputForProperty(Pair.Value);
        if (!SameInput(Actual,Wanted,ActualIds,ExpectedIds)) Match = false;
        AppendEdge(Edges,Actual,TEXT("Material"),Pair.Key,ActualIds);
    }
    Evidence->SetArrayField(TEXT("nodes"),Nodes); Evidence->SetArrayField(TEXT("edges"),Edges);
    Match &= Edges.Num()==Request.Graphs[0].Edges.Num();
    Evidence->SetObjectField(TEXT("settings"),ReadSettings(Material));
    Evidence->SetNumberField(TEXT("expressionCount"),Material->GetExpressions().Num());
    Evidence->SetNumberField(TEXT("edgeCountIncludingOutput"),Edges.Num());
    Evidence->SetBoolField(TEXT("matchesRequest"),Match);
    Evidence->SetObjectField(TEXT("layout"),ExpectedGraph.Layout);
    Evidence->SetStringField(TEXT("scaffolding"),TEXT("native material output only; no expression inserted"));
    Expected->MarkAsGarbage();
    return Match || Fail(Error,TEXT("actual expression, property, default, settings, position, or connection differs from request"));
}

bool ExportExpressions(UMaterial* Material, TArray<UMaterialExpression*>& Expressions, FString& Error)
{
    Expressions.Reset();
    if(!Material)return Fail(Error,TEXT("null source material"));
    const auto& Collection=Material->GetEditorOnlyData()->ExpressionCollection;
    if(!Collection.EditorComments.IsEmpty() || Collection.ExpressionExecBegin || Collection.ExpressionExecEnd)
        return Fail(Error,TEXT("comment boxes and material execution graph are not expressible in this export revision"));
    for(UMaterialExpression* Expression:Material->GetExpressions())
    {
        if(!Expression || !SpecFor(Expression->GetClass()->GetPathName()))
            return Fail(Error,TEXT("unsupported source expression: ")+GetPathNameSafe(Expression));
        Expressions.Add(Expression);
    }
    return true;
}
bool Export(const FObject& Config, const FString& CatalogDigest, FObject& Request, FObject& Evidence, FString& Error)
{
    Request.Reset(); Evidence.Reset(); Error.Reset();
    FString Source, Id, Target; int32 FormatVersion=2;
    if (!NodeForgeAssetDefinitions::ExportVersion(Config,FormatVersion,Error)
        || !String(Config,TEXT("source"),Source) || !String(Config,TEXT("requestId"),Id) || !SafeId(Id)
        || !String(Config,TEXT("assetPath"),Target)
        || !(Source.StartsWith(TEXT("/Engine/")) || Source.StartsWith(TEXT("/Game/NodeForgeTests/")))
        || Source.Contains(TEXT("..")) || !FPackageName::IsValidObjectPath(Source))
        return Fail(Error,TEXT("export requires exact source object path and new ASCII requestId/assetPath in NodeForgeTests"));
    const FString SourcePackage=FPackageName::ObjectPathToPackageName(Source);
    if (!NodeForgePrototype::ValidateNewTargetIdentity(Source,Id,Target,Error))
        return Fail(Error,TEXT("export target already exists or equals source; no overwrite"));
    UMaterial* Material=LoadObject<UMaterial>(nullptr,*Source);
    if (!Material || Material->GetClass()!=UMaterial::StaticClass() || Material->GetPathName()!=Source || Material->GetPackage()->IsDirty())
        return Fail(Error,TEXT("export source must be a saved, clean, ordinary Material"));
    const FString SourceFile=FPackageName::LongPackageNameToFilename(SourcePackage,FPackageName::GetAssetPackageExtension());
    const FString Before=NodeForgePrototype::FileSha256(SourceFile);
    if (Before.Len()!=64) return Fail(Error,TEXT("cannot fingerprint saved export source"));
    TArray<UMaterialExpression*> Expressions;
    if(!ExportExpressions(Material,Expressions,Error))return false;
    // Stable actual object names, not Desc tags, input order, or a known recipe.
    Expressions.Sort([](const UMaterialExpression& A,const UMaterialExpression& B){return A.GetName()<B.GetName();});
    UMaterial* NativeDefaultsMaterial=NewMaterial(GetTransientPackage(),TemporaryName());
    ON_SCOPE_EXIT {NativeDefaultsMaterial->MarkAsGarbage();};
    if(!Settings(NativeDefaultsMaterial,ActualSettings(Material),Error)) return false;
    TMap<FString,UMaterialExpression*> NativeDefaults;
    TMap<UMaterialExpression*,FString> Ids;
    TArray<TSharedPtr<FJsonValue>> Nodes, Edges, Mapping;
    FGraphDocument Document; Document.RequestId=Id;Document.Target=Target;Document.CatalogDigest=CatalogDigest;
    Document.Asset.Set<FMaterialDefinition>(ActualSettings(Material));
    FGraphDefinition Description;Description.Id=TEXT("MaterialGraph");Description.Kind=TEXT("Material");
    for (int32 I=0;I<Expressions.Num();++I) Ids.Add(Expressions[I],FString::Printf(TEXT("Node%d"),I+1));
    for (UMaterialExpression* Expression : Expressions)
    {
        const FSpec& Spec=*SpecFor(Expression->GetClass()->GetPathName());
        auto Node=MakeShared<FJsonObject>(); Node->SetStringField(TEXT("id"),Ids[Expression]);
        Node->SetStringField(TEXT("class"),Spec.Class);
        auto Properties=EffectiveProperties(Expression,Spec);
        if(!Spec.RequiresFunctionBinding)
        {
            // Preserve actual editor-creation defaults by omission. Some inactive defaults
            // (Time.Period=0 with override disabled) are intentionally not writable
            // explicit values under the existing import policy. No policy relaxed.
            UMaterialExpression* Default=NativeDefaults.FindRef(Spec.Class);
            if(!Default)
            {
                Default=UMaterialEditingLibrary::CreateMaterialExpression(NativeDefaultsMaterial,Expression->GetClass());
                if(!Default) return Fail(Error,TEXT("native default expression creation failed: ")+Spec.Class);
                NativeDefaults.Add(Spec.Class,Default);
            }
            for(const FString& Name:Spec.Properties)
                if(FProperty* P=Expression->GetClass()->FindPropertyByName(FName(*Name)))
                    if(Name!=TEXT("ParameterName") && Name!=TEXT("Texture") && P->Identical_InContainer(Expression,Default)) Properties->RemoveField(Name);
        }
        FNodeDescription NativeNode;NativeNode.Id=Ids[Expression];NativeNode.Class=Spec.Class;
        if(Spec.RequiresFunctionBinding) NativeNode.Binding=NodeForgeMaterialFunctions::Binding(Cast<UMaterialExpressionMaterialFunctionCall>(Expression));
        else NativeNode.Properties=Properties->Values;
        NativeNode.Position=FIntPoint(Expression->MaterialExpressionEditorX,Expression->MaterialExpressionEditorY);
        Description.Nodes.Add(MoveTemp(NativeNode));
        Node->SetObjectField(TEXT("properties"),Properties);
        Node->SetArrayField(TEXT("position"),{MakeShared<FJsonValueNumber>(Expression->MaterialExpressionEditorX),MakeShared<FJsonValueNumber>(Expression->MaterialExpressionEditorY)});
        Nodes.Add(MakeShared<FJsonValueObject>(Node));
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("id"),Ids[Expression]);Row->SetStringField(TEXT("object"),Expression->GetPathName());
        Row->SetStringField(TEXT("sourceDescription"),Expression->Desc);Mapping.Add(MakeShared<FJsonValueObject>(Row));
    }
    auto Edge=[&](FExpressionInput* Input,const FString& To,const FString& Pin)->bool
    {
        if (!Input || !Input->Expression) return true;
        if (!Ids.Contains(Input->Expression) || !Input->Expression->GetOutputs().IsValidIndex(Input->OutputIndex))
            return Fail(Error,TEXT("source connection has external/missing expression or invalid output: ")+To+TEXT(".")+Pin);
        const FExpressionOutput& Output=Input->Expression->GetOutputs()[Input->OutputIndex];
        if (Input->Mask!=Output.Mask || Input->MaskR!=Output.MaskR || Input->MaskG!=Output.MaskG || Input->MaskB!=Output.MaskB || Input->MaskA!=Output.MaskA)
            return Fail(Error,TEXT("custom connection channel mask is not expressible: ")+To+TEXT(".")+Pin);
        auto E=MakeShared<FJsonObject>();E->SetStringField(TEXT("from"),Ids[Input->Expression]);E->SetStringField(TEXT("output"),FString::FromInt(Input->OutputIndex));
        E->SetStringField(TEXT("to"),To);E->SetStringField(TEXT("input"),Pin);Edges.Add(MakeShared<FJsonValueObject>(E));
        Description.Edges.Add({Ids[Input->Expression],FString::FromInt(Input->OutputIndex),To,Pin});return true;
    };
    for (UMaterialExpression* Expression : Expressions)
    {
        const auto Inputs=NativeInputs(Expression);
        for (int32 I=0;I<Inputs.Num();++I) if (!Edge(Inputs[I],Ids[Expression],Expression->GetInputName(I).ToString())) return false;
    }
    // Inspect every native root input, including outputs outside the supported set.
    for (int32 I=0;I<MP_MAX;++I)
    {
        FExpressionInput* Input=Material->GetExpressionInputForProperty(static_cast<EMaterialProperty>(I));
        if (!Input || !Input->Expression) continue;
        const auto* Named=MaterialInputs().FindByPredicate([I](const auto& Pair){return static_cast<int32>(Pair.Value)==I;});
        if (!Named || !Material->IsPropertyActive(Named->Value))
            return Fail(Error,FString::Printf(TEXT("unsupported or inactive connected material output %d"),I));
        if (!Edge(Input,TEXT("Material"),Named->Key)) return false;
    }
    Document.Graphs.Add(MoveTemp(Description));
    FGraphDocument Validated;
    if(!NodeForgeGraphCodec::Decode(NodeForgeGraphCodec::Encode(Document,3),Validated,Error))return false;
    UMaterial* Expected=NewMaterial(GetTransientPackage(),TemporaryName());FGraph Graph;
    bool Good=Populate(Expected,Document,Graph,Error);
    // Conservative native editable-property guard catches supported classes configured
    // beyond our protocol (mip modes, clamp modes, material flags, parameter groups...).
    auto Editable=[&](UObject* Actual,UObject* Wanted,const TSet<FName>& Skip)->bool
    {
        for (TFieldIterator<FProperty> It(Actual->GetClass());It;++It)
        {
            if (!It->HasAnyPropertyFlags(CPF_Edit) || It->HasAnyPropertyFlags(CPF_Transient) || Skip.Contains(It->GetFName())) continue;
            auto* Struct=CastField<FStructProperty>(*It);
            if (Struct && Struct->Struct->GetFName()==NAME_ExpressionInput) continue; // exact native connection comparison below
            if (!It->Identical_InContainer(Actual,Wanted)) return Fail(Error,TEXT("unexpressible nondefault editable property: ")+Actual->GetPathName()+TEXT(".")+It->GetName());
        }
        return true;
    };
    if (Good) Good=Editable(Material,Expected,{}) && Editable(Material->GetEditorOnlyData(),Expected->GetEditorOnlyData(),{});
    // Root UseConstant/constant values are native structs, not expressions. Reject
    // rather than silently turning them into an unconnected default output.
    if (Good) for (TFieldIterator<FStructProperty> It(Material->GetEditorOnlyData()->GetClass());It;++It)
    {
        if (!It->Struct->GetName().EndsWith(TEXT("MaterialInput"))) continue;
        for (int32 A=0;A<It->ArrayDim && Good;++A)
            for (TFieldIterator<FProperty> Member(It->Struct);Member && Good;++Member)
                if (Member->GetName().StartsWith(TEXT("Constant")) || Member->GetName()==TEXT("UseConstant"))
                    if (!Member->Identical_InContainer(It->ContainerPtrToValuePtr<void>(Material->GetEditorOnlyData(),A),It->ContainerPtrToValuePtr<void>(Expected->GetEditorOnlyData(),A)))
                        Good=Fail(Error,TEXT("nondefault root constant cannot be exported: ")+It->GetName()+TEXT(".")+Member->GetName());
    }
    TMap<UMaterialExpression*,FString> ExpectedIds;
    for (const auto& Pair:Graph.Nodes) ExpectedIds.Add(Pair.Value,Pair.Key);
    if (Good) for (UMaterialExpression* Actual : Expressions)
    {
        UMaterialExpression* Wanted=Graph.Nodes.FindRef(Ids[Actual]);
        if (!Wanted || !Editable(Actual,Wanted,{TEXT("Desc")})) {Good=false;break;}
        if (auto* Function=Cast<UMaterialExpressionMaterialFunctionCall>(Actual))
            if (!NodeForgeMaterialFunctions::Matches(Function,Cast<UMaterialExpressionMaterialFunctionCall>(Wanted)))
            {Good=Fail(Error,TEXT("function signature differs from saved reference"));break;}
        const auto Inputs=NativeInputs(Actual), WantedInputs=NativeInputs(Wanted);
        if (Inputs.Num()!=WantedInputs.Num()) {Good=Fail(Error,TEXT("native input count changed"));break;}
        const auto& Outputs=Actual->GetOutputs();const auto& WantedOutputs=Wanted->GetOutputs();
        if(Outputs.Num()!=WantedOutputs.Num()){Good=Fail(Error,TEXT("native output count changed"));break;}
        for(int32 I=0;I<Outputs.Num() && Good;++I)
        {
            const auto& A=Outputs[I];const auto& B=WantedOutputs[I];
            if(A.OutputName!=B.OutputName || Actual->GetOutputType(I)!=Wanted->GetOutputType(I)
                || A.Mask!=B.Mask || A.MaskR!=B.MaskR || A.MaskG!=B.MaskG || A.MaskB!=B.MaskB || A.MaskA!=B.MaskA)
                Good=Fail(Error,TEXT("native output signature changed: ")+Ids[Actual]);
        }
        for (int32 I=0;I<Inputs.Num() && Good;++I)
            if (!SameInput(Inputs[I],FindInput(Wanted,Actual->GetInputName(I).ToString()),Ids,ExpectedIds))
                Good=Fail(Error,TEXT("native reconstruction connection mismatch: ")+Ids[Actual]);
    }
    Expected->MarkAsGarbage();
    if (!Good) return false;
    if (Material->GetPackage()->IsDirty() || NodeForgePrototype::FileSha256(SourceFile)!=Before)
        return Fail(Error,TEXT("source dirty state or saved bytes changed during read-only export"));
    Evidence=MakeShared<FJsonObject>();Evidence->SetStringField(TEXT("source"),Source);Evidence->SetStringField(TEXT("sourceAssetSha256"),Before);
    Evidence->SetBoolField(TEXT("sourceBytesUnchanged"),true);Evidence->SetBoolField(TEXT("sourceRemainsClean"),true);
    Evidence->SetArrayField(TEXT("logicalIdMapping"),Mapping);Evidence->SetNumberField(TEXT("expressionCount"),Nodes.Num());Evidence->SetNumberField(TEXT("edgeCount"),Edges.Num());
    Evidence->SetStringField(TEXT("metadataPolicy"),TEXT("source descriptions recorded in mapping; new Desc uses logical ID; object/GUID identity and comment descriptions are not copied; comment boxes rejected"));
    Evidence->SetStringField(TEXT("verification"),TEXT("native reconstruction and editable property/connection comparison; not compilation, persistence, runtime or recursive dependency locking"));
    Request=NodeForgeGraphCodec::Encode(Document,FormatVersion);return true;
}
}
