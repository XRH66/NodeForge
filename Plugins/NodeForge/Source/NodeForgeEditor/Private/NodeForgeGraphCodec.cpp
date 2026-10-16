#include "NodeForgeGraphCodec.h"
#include "NodeForgeAssetDefinitions.h"
#include "NodeForgeGraphDiff.h"
#include "NodeForgePrototype.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace NodeForgeGraphCodec
{
namespace
{
using namespace NodeForgeAssetDefinitions;
using EContext = NodeForgeNodeDefinitions::EContext;
using FValues = TArray<FValue>;
bool Exact(const FString& A, const FString& B) { return A.Equals(B, ESearchCase::CaseSensitive); }
bool Fail(FString& Error, const FString& Text) { Error = TEXT("GraphCodec: ") + Text; return false; }
FObject Object(const FValue& Value) { return Value && Value->Type == EJson::Object ? Value->AsObject() : nullptr; }
FObject Child(const FObject& Input, const TCHAR* Name) { return Input ? Object(Input->TryGetField(Name)) : nullptr; }
bool Shape(const FObject& Input, EFields Group, FString& Error)
{
    if (!Input) return Fail(Error, TEXT("expected object"));
    return FieldsValid(Input->Values, Group, Error);
}
FObject MapObject(const FValueMap& Map) { auto Result = MakeShared<FJsonObject>(); Result->Values = Map; return Result; }
FObject MaterialSettings(const FMaterialDefinition& Material)
{
    return EncodeMaterialSettings(Material);
}
bool Binding(FNodeDescription& Node, const FObject& Source, int32 Version, FString& Error)
{
    const EBindingKind Required = BindingKind(Node.Class);
    if (Required == EBindingKind::None) return !Source->HasField(TEXT("binding")) || Fail(Error, TEXT("ordinary node cannot have binding"));
    Node.Binding.Kind = Required;
    if (Version == 3)
    {
        const auto B = Child(Source, TEXT("binding"));
        const EFields FieldsGroup = Required == EBindingKind::Member ? EFields::MemberBinding : Required == EBindingKind::MaterialFunction ? EFields::MaterialFunctionBinding : EFields::FunctionBinding;
        if (!Shape(B, FieldsGroup, Error) || !Node.Properties.IsEmpty()) return Fail(Error, TEXT("bound node requires its closed binding and empty properties"));
        const FString Kind = BindingName(Required);
        if (!Exact(B->GetStringField(TEXT("kind")), Kind)) return Fail(Error, TEXT("binding kind does not match node class"));
        if (Required == EBindingKind::Member) Node.Binding.Name = B->GetStringField(TEXT("name"));
        else if (Required == EBindingKind::MaterialFunction) { Node.Binding.Path = B->GetStringField(TEXT("path")); Node.Binding.AssetSha256 = B->GetStringField(TEXT("assetSha256")); }
        else { Node.Binding.Owner = B->GetStringField(TEXT("owner")); Node.Binding.Member = B->GetStringField(TEXT("member")); }
    }
    else
    {
        const auto P = Child(Source, TEXT("properties"));
        const TCHAR* First = Required == EBindingKind::Member ? TEXT("variable") : Required == EBindingKind::MaterialFunction ? TEXT("MaterialFunction") : TEXT("owner");
        const TCHAR* Second = Required == EBindingKind::MaterialFunction ? TEXT("FunctionAssetSha256") : TEXT("member");
        const EFields Group = Required == EBindingKind::Member ? EFields::MemberPropertiesV2 : Required == EBindingKind::MaterialFunction ? EFields::MaterialFunctionPropertiesV2 : EFields::FunctionPropertiesV2;
        if (!Shape(P, Group, Error)) return false;
        if (Required == EBindingKind::Member) Node.Binding.Name = P->GetStringField(First);
        else if (Required == EBindingKind::MaterialFunction) { Node.Binding.Path = P->GetStringField(First); Node.Binding.AssetSha256 = P->GetStringField(Second); }
        else { Node.Binding.Owner = P->GetStringField(First); Node.Binding.Member = P->GetStringField(Second); }
        Node.Properties.Empty();
    }
    if (Required == EBindingKind::Member && !Identifier(Node.Binding.Name, true)) return Fail(Error, TEXT("invalid member binding name"));
    if (Required == EBindingKind::MaterialFunction && !Sha256(Node.Binding.AssetSha256)) return Fail(Error, TEXT("function binding SHA must be lowercase SHA256"));
    return true;
}
bool Graph(const FObject& Input, const FString& AssetType, int32 Version, const NodeForgeNodeDefinitions::FCapabilities& Capabilities,
    FGraphDefinition& Out, FString& Error, EContext Context)
{
    if (Version == 3 && !Shape(Input, EFields::Graph, Error)) return false;
    const bool Material = Exact(AssetType, TEXT("Material"));
    const auto& Definition = *NodeForgeAssetDefinitions::AssetType(AssetType);
    Out.Id = Definition.GraphId; Out.Kind = Definition.GraphKind;
    if (Version == 3 && (!Exact(Input->GetStringField(TEXT("id")), Out.Id) || !Exact(Input->GetStringField(TEXT("kind")), Out.Kind))) return Fail(Error, TEXT("only the fixed single graph is supported"));
    const FValues& Nodes = Input->GetArrayField(TEXT("nodes")); const FValues& Edges = Input->GetArrayField(TEXT("edges"));
    if (Nodes.Num() < 1 || Nodes.Num() > Definition.MaximumNodes || Edges.Num() < Definition.MinimumEdges || Edges.Num() > Definition.MaximumEdges) return Fail(Error, TEXT("graph exceeds its existing node/edge bounds"));
    TSet<FString> Ids;
    for (const auto& Item : Nodes)
    {
        const auto Source = Object(Item);
        if (!Shape(Source, Version == 3 ? EFields::NodeV3 : EFields::NodeV2, Error)) return false;
        FNodeDescription Node; Node.Id = Source->GetStringField(TEXT("id")); Node.Class = Source->GetStringField(TEXT("class"));
        if (!Identifier(Node.Id) || Ids.Contains(Node.Id)) return Fail(Error, TEXT("invalid or duplicate node id"));
        Ids.Add(Node.Id); Node.Properties = Child(Source, TEXT("properties"))->Values;
        if (Material)
        {
            const auto* Spec = NodeForgeNodeDefinitions::Material(Node.Class);
            if (!Spec || Capabilities.MaterialRevision < Spec->MinimumRevision || Spec->Admission == NodeForgeNodeDefinitions::EAdmission::Rejected
                || (Spec->Admission != NodeForgeNodeDefinitions::EAdmission::Admitted && Context != EContext::NativeCatalogProbe)) return Fail(Error, TEXT("material class unavailable in catalog"));
        }
        else
        {
            const auto* Spec = NodeForgeNodeDefinitions::Special(Node.Class);
            if (!Spec || Capabilities.BlueprintRevision < Spec->MinimumRevision) return Fail(Error, TEXT("Unsupported blueprint node class or archived capability"));
        }
        if (!Binding(Node, Source, Version, Error)) return false;
        if (!Material && Node.Binding.Kind == EBindingKind::Function)
        {
            const auto* Function = NodeForgeNodeDefinitions::Function(Node.Binding.Owner,Node.Binding.Member);
            if(!Function || Capabilities.BlueprintRevision<Function->MinimumRevision || Function->Admission==NodeForgeNodeDefinitions::EAdmission::Rejected
                || (Function->Admission!=NodeForgeNodeDefinitions::EAdmission::Admitted && Context!=EContext::NativeCatalogProbe))
                return Fail(Error,TEXT("function binding unavailable in catalog"));
        }
        if (!Material && !NodePropertiesValid(Node.Class, Node.Properties, Error)) return Fail(Error, TEXT("invalid native blueprint configuration"));
        Node.DefaultsPresent = Source->HasField(TEXT("defaults"));
        if (Node.DefaultsPresent) Node.Defaults = Child(Source, TEXT("defaults"))->Values;
        for (const auto& Default : Node.Defaults)
        {
            if (Default.Key.IsEmpty() || Default.Key.Len() > 128 || !ScalarValue(Default.Value)) return Fail(Error, TEXT("invalid pin default"));
            if (!Material && ((Value(EValueKind::Vector, Default.Value) && Capabilities.BlueprintRevision < 1)
                || (Value(EValueKind::Rotator, Default.Value) && Capabilities.BlueprintRevision < 6))) return Fail(Error, TEXT("pin default unavailable in archived catalog"));
        }
        if (Source->HasField(TEXT("position")))
        {
            FIntPoint Coordinates;
            if (!Position(Source->TryGetField(TEXT("position")), AssetType, Coordinates)) return Fail(Error, TEXT("invalid native position range"));
            Node.Position = Coordinates;
        }
        Out.Nodes.Add(MoveTemp(Node));
    }
    for (const auto& Item : Edges)
    {
        const auto Source = Object(Item); if (!Shape(Source, EFields::Edge, Error)) return false;
        FEdgeDescription Edge{Source->GetStringField(TEXT("from")), Source->GetStringField(TEXT("output")), Source->GetStringField(TEXT("to")), Source->GetStringField(TEXT("input"))};
        if (!Ids.Contains(Edge.From) || (!Ids.Contains(Edge.To) && !(Material && Exact(Edge.To, TEXT("Material"))))
            || Edge.From == Edge.To || Edge.Output.IsEmpty() || Edge.Input.IsEmpty() || Edge.Output.Len() > 128 || Edge.Input.Len() > 128) return Fail(Error, TEXT("invalid edge endpoint"));
        Out.Edges.Add(MoveTemp(Edge));
    }
    return true;
}
FObject EncodeBinding(const FNodeBinding& Binding)
{
    auto Result = MakeShared<FJsonObject>();
    if (Binding.Kind == EBindingKind::Member) { Result->SetStringField(TEXT("kind"), TEXT("Member")); Result->SetStringField(TEXT("name"), Binding.Name); }
    else if (Binding.Kind == EBindingKind::MaterialFunction) { Result->SetStringField(TEXT("kind"), TEXT("MaterialFunction")); Result->SetStringField(TEXT("path"), Binding.Path); Result->SetStringField(TEXT("assetSha256"), Binding.AssetSha256); }
    else { Result->SetStringField(TEXT("kind"), BindingName(Binding.Kind)); Result->SetStringField(TEXT("owner"), Binding.Owner); Result->SetStringField(TEXT("member"), Binding.Member); }
    return Result;
}
FObject EncodeGraph(const FGraphDefinition& Graph, int32 Version)
{
    auto Result = MakeShared<FJsonObject>(); FValues Nodes, Edges;
    if (Version == 3) { Result->SetStringField(TEXT("id"), Graph.Id); Result->SetStringField(TEXT("kind"), Graph.Kind); }
    for (const auto& Node : Graph.Nodes)
    {
        auto N = MakeShared<FJsonObject>(); N->SetStringField(TEXT("id"), Node.Id); N->SetStringField(TEXT("class"), Node.Class);
        auto Properties = MapObject(Node.Properties);
        if (Node.Binding.Kind != EBindingKind::None)
        {
            if (Version == 3) N->SetObjectField(TEXT("binding"), EncodeBinding(Node.Binding));
            else if (Node.Binding.Kind == EBindingKind::Member) Properties->SetStringField(TEXT("variable"), Node.Binding.Name);
            else if (Node.Binding.Kind == EBindingKind::MaterialFunction) { Properties->SetStringField(TEXT("MaterialFunction"), Node.Binding.Path); Properties->SetStringField(TEXT("FunctionAssetSha256"), Node.Binding.AssetSha256); }
            else { Properties->SetStringField(TEXT("owner"), Node.Binding.Owner); Properties->SetStringField(TEXT("member"), Node.Binding.Member); }
        }
        N->SetObjectField(TEXT("properties"), Properties);
        if (Node.DefaultsPresent) N->SetObjectField(TEXT("defaults"), MapObject(Node.Defaults));
        if (Node.Position.IsSet()) N->SetArrayField(TEXT("position"), {MakeShared<FJsonValueNumber>(Node.Position->X), MakeShared<FJsonValueNumber>(Node.Position->Y)});
        Nodes.Add(MakeShared<FJsonValueObject>(N));
    }
    for (const auto& Edge : Graph.Edges)
    {
        auto E = MakeShared<FJsonObject>(); E->SetStringField(TEXT("from"), Edge.From); E->SetStringField(TEXT("output"), Edge.Output);
        E->SetStringField(TEXT("to"), Edge.To); E->SetStringField(TEXT("input"), Edge.Input); Edges.Add(MakeShared<FJsonValueObject>(E));
    }
    Result->SetArrayField(TEXT("nodes"), Nodes); Result->SetArrayField(TEXT("edges"), Edges); return Result;
}
}
bool Decode(const FObject& Input, FGraphDocument& Output, FString& Error, EContext Context)
{
    Output = FGraphDocument();
    Error.Reset();
    if (!Input || !Input->HasTypedField<EJson::Number>(TEXT("version"))) return Fail(Error, TEXT("missing graph version"));
    const double Version = Input->GetNumberField(TEXT("version"));
    if (Version != 2 && Version != 3) return Fail(Error, TEXT("modern graph version must be 2 or 3"));
    Output.WireVersion = static_cast<int32>(Version);
    if (!Shape(Input, Version == 3 ? EFields::RootV3 : EFields::RootV2, Error) || !Exact(Input->GetStringField(TEXT("format")), TEXT("NodeForgeGraph"))) return Fail(Error, TEXT("unsupported graph format or root fields"));
    Output.RequestId = Input->GetStringField(TEXT("requestId")); Output.CatalogDigest = Input->GetStringField(TEXT("catalogDigest"));
    if (!Identifier(Output.RequestId) || !Sha256(Output.CatalogDigest)) return Fail(Error, TEXT("invalid request identity"));
    const auto Capabilities = NodeForgeNodeDefinitions::Capabilities(Output.CatalogDigest, Context);
    if (!Capabilities.IsKnown() || !NodeForgeNodeDefinitions::AllowsGraphVersion(Output.CatalogDigest, Output.WireVersion, Context)) return Fail(Error, TEXT("catalog does not support this wire version"));
    FObject Asset, Settings; FString AssetType;
    if (Version == 3)
    {
        const auto Target = Child(Input, TEXT("target")); Asset = Child(Input, TEXT("asset"));
        if (!Shape(Target, EFields::Target, Error) || !Asset || !Asset->HasTypedField<EJson::String>(TEXT("type"))) return Fail(Error, TEXT("invalid target/asset"));
        Output.Target = Target->GetStringField(TEXT("assetPath")); AssetType = Asset->GetStringField(TEXT("type"));
    }
    else { Output.Target = Input->GetStringField(TEXT("assetPath")); AssetType = Input->GetStringField(TEXT("assetType")); Settings = Child(Input, TEXT("settings")); }
    if (!NodeForgePrototype::ValidateTargetPath(Output.Target, Error)) return false;
    if (Exact(AssetType, TEXT("Material")))
    {
        if (Version == 3) { if (!Shape(Asset, EFields::MaterialAsset, Error)) return false; Settings = Child(Asset, TEXT("settings")); }
        if (!Shape(Settings, EFields::MaterialSettings, Error)) return false;
        FMaterialDefinition Material; Material.Domain = Settings->GetStringField(TEXT("domain")); Material.Blend = Settings->GetStringField(TEXT("blend"));
        Material.Shading = Settings->GetStringField(TEXT("shading")); Material.TwoSided = Settings->GetBoolField(TEXT("twoSided"));
        if (Settings->HasField(TEXT("lighting"))) Material.Lighting = Settings->GetStringField(TEXT("lighting"));
        if (Settings->HasField(TEXT("refractionMethod"))) Material.RefractionMethod = Settings->GetStringField(TEXT("refractionMethod"));
        for(const auto& Option:MaterialOptions()) if(Settings->HasField(Option.Name))
        {
            if(!Option.Required && Capabilities.MaterialRevision<7) return Fail(Error,TEXT("material setting unavailable in archived catalog: ")+Option.Name);
            SetMaterialOption(Material,Option.Id,Settings->TryGetField(Option.Name));
        }
        if(Material.Blend==TEXT("Masked") && Capabilities.MaterialRevision<7) return Fail(Error,TEXT("Masked unavailable in archived catalog"));
        if (!MaterialSettingsValid(Material)) return Fail(Error, TEXT("unsupported material settings: Masked requires clip0..1; negative emissive requires Unlit; ClearCoat requires Opaque/Masked; IOR refraction requires Translucent DefaultLit with lighting"));
        Output.Asset.Set<FMaterialDefinition>(MoveTemp(Material));
    }
    else if (Exact(AssetType, TEXT("Blueprint")))
    {
        FBlueprintDefinition Blueprint; const FValues* Variables;
        if (Version == 3)
        {
            if (!Shape(Asset, EFields::BlueprintAsset, Error) || !Shape(Child(Asset, TEXT("parent")), EFields::BlueprintParent, Error) || !Shape(Child(Asset, TEXT("members")), EFields::Members, Error)) return false;
            Blueprint.ParentClass = Child(Asset, TEXT("parent"))->GetStringField(TEXT("class")); Variables = &Child(Asset, TEXT("members"))->GetArrayField(TEXT("variables"));
        }
        else { if (!Shape(Settings, EFields::BlueprintSettingsV2, Error)) return false; Blueprint.ParentClass = Settings->GetStringField(TEXT("parentClass")); Variables = &Settings->GetArrayField(TEXT("variables")); }
        if (!Exact(Blueprint.ParentClass, TEXT("/Script/Engine.Actor")) || Variables->Num() > MaxVariables) return Fail(Error, TEXT("only Actor parent and up to 64 variables are supported"));
        TSet<FName> Names;
        for (const auto& Item : *Variables)
        {
            const auto Variable = Object(Item); if (!Shape(Variable, Version == 3 ? EFields::VariableV3 : EFields::VariableV2, Error)) return false;
            FMemberDescription Member; Member.Name = Variable->GetStringField(TEXT("name")); FString Type;
            if (Version == 3) { const auto T = Child(Variable, TEXT("type")); if (!Shape(T, EFields::ValueType, Error) || !Exact(T->GetStringField(TEXT("kind")), TEXT("Value"))) return Fail(Error, TEXT("variable type must be Value/name")); Type = T->GetStringField(TEXT("name")); }
            else Type = Variable->GetStringField(TEXT("type"));
            Member.Default = Variable->TryGetField(TEXT("default"));
            if (!Identifier(Member.Name, true) || Names.Contains(FName(*Member.Name)) || !TypeFromName(Type, Member.Type.Kind)
                || Capabilities.BlueprintRevision < NodeForgeNodeDefinitions::MemberMinimumRevision(Type) || !Value(Member.Type.Kind, Member.Default)) return Fail(Error, TEXT("invalid variable name/type/default or archived capability"));
            if(Variable->HasField(TEXT("attributes")))
            {
                const auto Attributes=Child(Variable,TEXT("attributes"));
                if(Capabilities.BlueprintRevision<12) return Fail(Error,TEXT("member attributes unavailable in archived catalog"));
                if(!Shape(Attributes,EFields::MemberAttributes,Error)) return false;
                FMemberAttributes A;A.InstanceEditable=Attributes->GetBoolField(TEXT("instanceEditable"));A.Category=Attributes->GetStringField(TEXT("category"));A.Tooltip=Attributes->GetStringField(TEXT("tooltip"));
                if(!MemberAttributesValid(A)) return Fail(Error,TEXT("invalid member category/tooltip Unicode length, whitespace, segment or NUL"));
                Member.Attributes=MoveTemp(A);
            }
            Names.Add(FName(*Member.Name)); Blueprint.Variables.Add(MoveTemp(Member));
        }
        Output.Asset.Set<FBlueprintDefinition>(MoveTemp(Blueprint));
    }
    else if (Exact(AssetType, TEXT("MaterialInstance")))
    {
        if (!Capabilities.AllowsMaterialInstance) return Fail(Error, TEXT("instance unavailable in catalog"));
        FInstanceDefinition Instance;
        if (Version == 3)
        {
            if (!Shape(Asset, EFields::InstanceAsset, Error) || !Shape(Child(Asset, TEXT("parent")), EFields::InstanceParent, Error) || !Shape(Child(Asset, TEXT("parameters")), EFields::Parameters, Error)) return false;
            const auto Parent = Child(Asset, TEXT("parent")); const auto Parameters = Child(Asset, TEXT("parameters"));
            Instance.ParentPath = Parent->GetStringField(TEXT("path")); Instance.ParentAssetSha256 = Parent->GetStringField(TEXT("assetSha256"));
            Instance.Scalars = Child(Parameters, TEXT("scalar"))->Values; Instance.Vectors = Child(Parameters, TEXT("vector"))->Values; Instance.Textures = Child(Parameters, TEXT("texture"))->Values;
        }
        else
        {
            if (!Shape(Settings, EFields::InstanceSettingsV2, Error)) return false;
            Instance.ParentPath = Settings->GetStringField(TEXT("parent")); Instance.ParentAssetSha256 = Settings->GetStringField(TEXT("parentAssetSha256"));
            Instance.Scalars = Child(Settings, TEXT("scalarParameters"))->Values; Instance.Vectors = Child(Settings, TEXT("vectorParameters"))->Values; Instance.Textures = Child(Settings, TEXT("textureParameters"))->Values;
        }
        if (!Sha256(Instance.ParentAssetSha256) || Instance.Scalars.Num() + Instance.Vectors.Num() + Instance.Textures.Num() > MaxInstanceOverrides) return Fail(Error, TEXT("invalid parent SHA or instance override bound"));
        for (const auto& Pair : Instance.Scalars) { double N = 0; if (!Number(Pair.Value, N, 1000000.0)) return Fail(Error, TEXT("invalid scalar override")); }
        for (const auto& Pair : Instance.Vectors) if (!Value(EValueKind::LinearColor, Pair.Value)) return Fail(Error, TEXT("invalid vector override"));
        for (const auto& Pair : Instance.Textures) if (!Pair.Value || Pair.Value->Type != EJson::String) return Fail(Error, TEXT("invalid texture override"));
        Output.Asset.Set<FInstanceDefinition>(MoveTemp(Instance));
    }
    else return Fail(Error, TEXT("unsupported exact asset type"));
    if (Version == 3)
    {
        const FValues& Graphs = Input->GetArrayField(TEXT("graphs"));
        if (Graphs.Num() != NodeForgeAssetDefinitions::AssetType(AssetType)->GraphCount) return Fail(Error, TEXT("unsupported graph count"));
        if (Graphs.Num()) { FGraphDefinition G; if (!Graph(Object(Graphs[0]), AssetType, 3, Capabilities, G, Error, Context)) return false; Output.Graphs.Add(MoveTemp(G)); }
    }
    else if (AssetType == TEXT("MaterialInstance")) { if (!Input->GetArrayField(TEXT("nodes")).IsEmpty() || !Input->GetArrayField(TEXT("edges")).IsEmpty()) return Fail(Error, TEXT("instance must have no graph")); }
    else { FGraphDefinition G; if (!Graph(Input, AssetType, 2, Capabilities, G, Error, Context)) return false; Output.Graphs.Add(MoveTemp(G)); }
    return true;
}
bool DecodeText(const FString& Text, FGraphDocument& Output, FString& Error, EContext Context)
{
    Output=FGraphDocument();Error.Reset();
    const FTCHARToUTF8 Bytes(*Text);
    if (Bytes.Length() < 1 || Bytes.Length() > MaxInputUtf8Bytes || Text[0] == 0xfeff) return Fail(Error, TEXT("invalid UTF8 input size/BOM"));
    for (TCHAR Character : Text) if (Character == 0) return Fail(Error, TEXT("NUL is forbidden"));
    auto Reader = TJsonReaderFactory<>::Create(Text); EJsonNotation Notation; TArray<TSet<FString>> Keys; TArray<bool> Objects;
    while (Reader->ReadNext(Notation))
    {
        if (Notation == EJsonNotation::ObjectEnd || Notation == EJsonNotation::ArrayEnd) { if (Keys.Num()) { Keys.Pop(); Objects.Pop(); } continue; }
        if (Objects.Num() && Objects.Last()) { const FString Key = Reader->GetIdentifier(); if (Keys.Last().Contains(Key)) return Fail(Error, TEXT("duplicate JSON key")); Keys.Last().Add(Key); }
        if (Notation == EJsonNotation::ObjectStart || Notation == EJsonNotation::ArrayStart) { if (Keys.Num() >= MaxJsonDepth) return Fail(Error, TEXT("JSON nesting exceeds 32 levels")); Keys.AddDefaulted(); Objects.Add(Notation == EJsonNotation::ObjectStart); }
    }
    FObject Input;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Input) || !Decode(Input, Output, Error, Context)) return Error.IsEmpty() ? Fail(Error, TEXT("malformed JSON")) : false;
    Output.RawText = Text; Output.RawInputSha256 = NodeForgePrototype::TextSha256(Text); return true;
}
FObject Encode(const FGraphDocument& Document, int32 Version)
{
    check(Version == 2 || Version == 3);
    auto Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("format"), TEXT("NodeForgeGraph")); Result->SetNumberField(TEXT("version"), Version);
    Result->SetStringField(TEXT("requestId"), Document.RequestId); Result->SetStringField(TEXT("catalogDigest"), Document.CatalogDigest);
    auto Asset = MakeShared<FJsonObject>(); FObject Settings = MakeShared<FJsonObject>(); Asset->SetStringField(TEXT("type"), Document.AssetType());
    if (const auto* Material = Document.Asset.TryGet<FMaterialDefinition>()) { Settings = MaterialSettings(*Material); Asset->SetObjectField(TEXT("settings"), Settings); }
    else if (const auto* Blueprint = Document.Asset.TryGet<FBlueprintDefinition>())
    {
        auto Parent = MakeShared<FJsonObject>(); Parent->SetStringField(TEXT("class"), Blueprint->ParentClass); Asset->SetObjectField(TEXT("parent"), Parent);
        FValues Variables;
        for (const auto& Variable : Blueprint->Variables)
        {
            auto V = MakeShared<FJsonObject>(); V->SetStringField(TEXT("name"), Variable.Name); V->SetField(TEXT("default"), Variable.Default);
            if (Version == 3) { auto T = MakeShared<FJsonObject>(); T->SetStringField(TEXT("kind"), TEXT("Value")); T->SetStringField(TEXT("name"), TypeName(Variable.Type.Kind)); V->SetObjectField(TEXT("type"), T); }
            else V->SetStringField(TEXT("type"), TypeName(Variable.Type.Kind));
            if(Variable.Attributes.IsSet()) V->SetObjectField(TEXT("attributes"),EncodeMemberAttributes(Variable.Attributes.GetValue()));
            Variables.Add(MakeShared<FJsonValueObject>(V));
        }
        auto Members = MakeShared<FJsonObject>(); Members->SetArrayField(TEXT("variables"), Variables); Asset->SetObjectField(TEXT("members"), Members);
        Settings->SetStringField(TEXT("parentClass"), Blueprint->ParentClass); Settings->SetArrayField(TEXT("variables"), Variables);
    }
    else if (const auto* Instance = Document.Asset.TryGet<FInstanceDefinition>())
    {
        auto Parent = MakeShared<FJsonObject>(); Parent->SetStringField(TEXT("path"), Instance->ParentPath); Parent->SetStringField(TEXT("assetSha256"), Instance->ParentAssetSha256); Asset->SetObjectField(TEXT("parent"), Parent);
        auto Parameters = MakeShared<FJsonObject>(); Parameters->SetObjectField(TEXT("scalar"), MapObject(Instance->Scalars)); Parameters->SetObjectField(TEXT("vector"), MapObject(Instance->Vectors)); Parameters->SetObjectField(TEXT("texture"), MapObject(Instance->Textures)); Asset->SetObjectField(TEXT("parameters"), Parameters);
        Settings->SetStringField(TEXT("parent"), Instance->ParentPath); Settings->SetStringField(TEXT("parentAssetSha256"), Instance->ParentAssetSha256);
        Settings->SetObjectField(TEXT("scalarParameters"), MapObject(Instance->Scalars)); Settings->SetObjectField(TEXT("vectorParameters"), MapObject(Instance->Vectors)); Settings->SetObjectField(TEXT("textureParameters"), MapObject(Instance->Textures));
    }
    if (Version == 3)
    {
        auto Target = MakeShared<FJsonObject>(); Target->SetStringField(TEXT("assetPath"), Document.Target); Result->SetObjectField(TEXT("target"), Target); Result->SetObjectField(TEXT("asset"), Asset);
        FValues Graphs; for (const auto& Graph : Document.Graphs) Graphs.Add(MakeShared<FJsonValueObject>(EncodeGraph(Graph, 3))); Result->SetArrayField(TEXT("graphs"), Graphs);
    }
    else
    {
        Result->SetStringField(TEXT("assetType"), Document.AssetType()); Result->SetStringField(TEXT("assetPath"), Document.Target); Result->SetObjectField(TEXT("settings"), Settings);
        if (Document.Graphs.Num()) { const auto Graph = EncodeGraph(Document.Graphs[0], 2); Result->SetArrayField(TEXT("nodes"), Graph->GetArrayField(TEXT("nodes"))); Result->SetArrayField(TEXT("edges"), Graph->GetArrayField(TEXT("edges"))); }
        else { Result->SetArrayField(TEXT("nodes"), {}); Result->SetArrayField(TEXT("edges"), {}); }
    }
    return Result;
}
bool EncodeText(const FGraphDocument& Document, int32 Version, FString& Text, FString& Error)
{
    Text.Reset();Error.Reset();
    if (Version != 2 && Version != 3) return Fail(Error, TEXT("formatVersion must be 2 or 3"));
    Text.Reset(); const auto Writer = TJsonWriterFactory<>::Create(&Text);
    if (!FJsonSerializer::Serialize(Encode(Document, Version).ToSharedRef(), Writer)) return Fail(Error, TEXT("graph serialization failed"));
    const FTCHARToUTF8 Bytes(*Text); return Bytes.Length() <= MaxInputUtf8Bytes || Fail(Error, TEXT("complete export exceeds graph limit"));
}
bool EqualOriginal(const FGraphDocument& A, const FGraphDocument& B)
{
    // Projection is only a comparison representation. Neither production nor
    // provenance hashes consume it; array order/default presence stay explicit.
    return NodeForgeGraphDiff::EqualValue(MakeShared<FJsonValueObject>(Encode(A, 3)), MakeShared<FJsonValueObject>(Encode(B, 3)));
}
}
