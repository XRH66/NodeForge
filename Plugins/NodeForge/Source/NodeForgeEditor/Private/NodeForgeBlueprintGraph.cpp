// NodeForge adaptation of selected ZiggyMar/unreal-mcp routines.
// Copyright (c) 2026 Ziggy. MIT: ../../../Licenses/ZiggyMarMIT.txt.
// Fixed upstream d1a85396d9c62328521256005fe7c420725651cc:
// MCPCommandHandler.cpp AddNodeCore (self-variable binding and initialization),
// MCPNodeCatalog.cpp ShouldIncludeFunction (reflection eligibility filter).
// Native spawners replace manual function/event setup. No networking or existing-asset edits.
#include "NodeForgeBlueprintGraph.h"
#include "NodeForgeAssetDefinitions.h"
#include "NodeForgeGraphCodec.h"
#include "NodeForgeGraphDiff.h"
#include "NodeForgeNodeDefinitions.h"
#include "NodeForgeBlueprintSwitches.h"
#include "NodeForgeGraphLayout.h"
#include "NodeForgeMaterialGraph.h"
#include "NodeForgePrototype.h"

#include "BlueprintActionDatabase.h"
#include "BlueprintEventNodeSpawner.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "K2Node_AddPinInterface.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "UObject/NoExportTypes.h"
#include "Misc/DefaultValueHelper.h"
#include "Misc/PackageName.h"
#include "Misc/ScopeExit.h"

namespace NodeForgeBlueprintGraph
{
using namespace NodeForgeGraphDocument;
using NodeForgeAssetDefinitions::NativeText;
using NodeForgeAssetDefinitions::MatchesNativeText;
using NodeForgeAssetDefinitions::FromNativeText;
using NodeForgeAssetDefinitions::VectorJson;
using NodeForgeAssetDefinitions::RotatorJson;
static bool ReadbackChecked(UBlueprint* Blueprint, const FGraphDocument& Request, FObject& Evidence, FString& Error,
    const TMap<UEdGraphNode*, FString>* ReadOnlyIds, NodeForgeNodeDefinitions::EContext Context);
namespace
{
using FValues = TArray<TSharedPtr<FJsonValue>>;
const FString IdPrefix(TEXT("NodeForge:"));
constexpr int32 MinSequenceOutputs = NodeForgeAssetDefinitions::MinimumSequenceOutputs;
constexpr int32 MaxSequenceOutputs = NodeForgeAssetDefinitions::MaximumSequenceOutputs;

bool ExactText(const FString& A, const FString& B)
{
    return A.Equals(B, ESearchCase::CaseSensitive);
}
bool ExactContains(const TSet<FString>& Values, const FString& Value)
{
    for (const FString& Candidate : Values) if (ExactText(Candidate, Value)) return true;
    return false;
}
bool ExactProperty(const FProperty* Property, const void* A, const void* B)
{
    // Native FStrProperty::Identical uses case-insensitive FString operator==.
    // Direct strings use their native getter; other property semantics stay native.
    if (const auto* Text = CastField<FStrProperty>(Property))
    {
        for (int32 I = 0; I < Property->ArrayDim; ++I)
            if (!ExactText(Text->GetPropertyValue_InContainer(A, I), Text->GetPropertyValue_InContainer(B, I))) return false;
        return true;
    }
    return Property->Identical_InContainer(A, B);
}

bool Fail(FString& Error, const FString& Message)
{
    Error = Message;
    UE_LOG(LogTemp, Display, TEXT("NodeForge blueprint graph rejected: %s"), *Message);
    return false;
}
FMemberAttributes ActualAttributes(const FBPVariableDescription& Variable)
{
    FMemberAttributes A;A.InstanceEditable=(Variable.PropertyFlags & CPF_DisableEditOnInstance)==0;
    A.Category=Variable.Category.ToString();
    for(const auto& Entry:Variable.MetaDataArray) if(Entry.DataKey==TEXT("Tooltip")) A.Tooltip=Entry.DataValue;
    return A;
}
void ApplyAttributes(UBlueprint* Blueprint,const FMemberDescription& Definition)
{
    if(!Definition.Attributes.IsSet())return;
    const auto& A=Definition.Attributes.GetValue();const FName Name(*Definition.Name);
    FBlueprintEditorUtils::SetBlueprintOnlyEditableFlag(Blueprint,Name,!A.InstanceEditable);
    FBlueprintEditorUtils::SetBlueprintVariableCategory(Blueprint,Name,nullptr,FText::FromString(A.Category),true);
    if(A.Tooltip.IsEmpty()) FBlueprintEditorUtils::RemoveBlueprintVariableMetaData(Blueprint,Name,nullptr,TEXT("Tooltip"));
    else FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint,Name,nullptr,TEXT("Tooltip"),A.Tooltip);
}
bool AttributesMatch(UBlueprint* Blueprint,const FBPVariableDescription& Variable,const FMemberDescription& Definition,FString& Error)
{
    const auto A=ActualAttributes(Variable);
    if(Definition.Attributes.IsSet())
    {
        const auto& Wanted=Definition.Attributes.GetValue();
        if(A.InstanceEditable!=Wanted.InstanceEditable || !ExactText(A.Category,Wanted.Category) || !ExactText(A.Tooltip,Wanted.Tooltip)) return Fail(Error,TEXT("native member attributes differ: ")+Definition.Name);
    }
    if(Blueprint->Status==BS_UpToDate || Blueprint->Status==BS_UpToDateWithWarnings)
    {
        const auto* Property=Blueprint->GeneratedClass->FindPropertyByName(Variable.VarName);
        if(!Property || Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance)==A.InstanceEditable
            || !ExactText(Property->GetMetaData(TEXT("Category")),A.Category) || !ExactText(Property->GetMetaData(TEXT("Tooltip")),A.Tooltip)) return Fail(Error,TEXT("compiled member flags/category/Tooltip differ: ")+Definition.Name);
    }
    return true;
}
FObject Object(const TSharedPtr<FJsonValue>& Value)
{
    return Value.IsValid() && Value->Type == EJson::Object ? Value->AsObject() : nullptr;
}
FObject FieldObject(const FObject& Value, const TCHAR* Key)
{
    const TSharedPtr<FJsonObject>* Child = nullptr;
    return Value && Value->TryGetObjectField(Key, Child) ? *Child : nullptr;
}
bool Keys(const FObject& Value, std::initializer_list<const TCHAR*> Required, std::initializer_list<const TCHAR*> Optional, FString& Error)
{
    if (!Value) return Fail(Error, TEXT("Expected JSON object"));
    TSet<FString> Allowed;
    for (const TCHAR* Key : Required)
    {
        Allowed.Add(Key);
        bool Present = false;
        for (const auto& Pair : Value->Values) if (ExactText(Pair.Key, Key)) { Present = true; break; }
        if (!Present) return Fail(Error, FString(TEXT("Missing exact field: ")) + Key);
    }
    for (const TCHAR* Key : Optional) Allowed.Add(Key);
    for (const auto& Pair : Value->Values)
        if (!ExactContains(Allowed, Pair.Key)) return Fail(Error, TEXT("Unsupported field: ") + Pair.Key);
    return true;
}
bool String(const FObject& Value, const TCHAR* Key, FString& Out)
{
    return Value && Value->HasTypedField<EJson::String>(Key) && Value->TryGetStringField(Key, Out);
}
bool Identifier(const FString& Name) { return NodeForgeAssetDefinitions::Identifier(Name); }

using FFunctionSpec = NodeForgeNodeDefinitions::FBlueprintFunctionDefinition;
using ENodeContext = NodeForgeNodeDefinitions::EContext;
const TArray<FFunctionSpec>& Functions() { return NodeForgeNodeDefinitions::Functions(); }
UFunction* ResolveFunction(const FString& Owner, const FString& Member, ENodeContext Context = ENodeContext::External)
{
    const auto* Definition = NodeForgeNodeDefinitions::Function(Owner, Member);
    if (!Definition || Definition->Admission == NodeForgeNodeDefinitions::EAdmission::Rejected
        || (Definition->Admission != NodeForgeNodeDefinitions::EAdmission::Admitted && Context != ENodeContext::NativeCatalogProbe)) return nullptr;
    UFunction* Function = Definition->Owner->FindFunctionByName(FName(*Member));
    return NodeForgeNodeDefinitions::EligibleExistingFunction(Function) ? Function : nullptr;
}
UBlueprintFunctionNodeSpawner* FindFunctionActionInDatabase(UFunction* Function)
{
    const auto& Actions = FBlueprintActionDatabase::Get().GetAllActions();
    for (const auto& Entry : Actions)
        for (UBlueprintNodeSpawner* Spawner : Entry.Value)
            if (auto* FunctionAction = Cast<UBlueprintFunctionNodeSpawner>(Spawner))
                if (FunctionAction->GetFunction() == Function && FunctionAction->NodeClass == UK2Node_CallFunction::StaticClass())
                    return FunctionAction;
    return nullptr;
}
UBlueprintNodeSpawner* FunctionSpawner(UFunction* Function, UObject* Outer, bool& FoundInDatabase)
{
    UBlueprintFunctionNodeSpawner* DatabaseAction = FindFunctionActionInDatabase(Function);
    FoundInDatabase = DatabaseAction != nullptr;
    if (DatabaseAction) return DatabaseAction;
    // UE5.3.2 FunctionNodeSpawner::Create reads/registers the global raw-pointer type-promotion
    // cache even with an explicit CallFunction class. That cache does not keep a private
    // transient spawner alive across GC. Reuse database-owned actions above; the
    // native generic spawner plus exact SetFromFunction binding below never enters that cache.
    return UBlueprintNodeSpawner::Create(UK2Node_CallFunction::StaticClass(), Outer,
        UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateLambda([Function](UEdGraphNode* Node, bool)
        { if (UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node)) Call->SetFromFunction(Function); }));
}
FString VariableTypeName(const FEdGraphPinType& Type)
{
    EValueKind Kind;
    return NodeForgeAssetDefinitions::KindFromPinType(Type, Kind) ? NodeForgeAssetDefinitions::TypeName(Kind) : TEXT("unsupported");
}
bool SequenceOutputCount(const FValueMap& Props, int32& Count, FString& Error)
{
    if (!NodeForgeAssetDefinitions::NodePropertiesValid(UK2Node_ExecutionSequence::StaticClass()->GetPathName(), Props, Error)) return Fail(Error, TEXT("invalid Sequence outputCount"));
    Count = static_cast<int32>(Props[TEXT("outputCount")]->AsNumber()); return true;
}
bool VariableDefault(UBlueprint* Blueprint, const FBPVariableDescription& Variable, FString& Text, FString& Source, FString& Error)
{
    Text=Variable.DefaultValue;Source=TEXT("uncompiledVariableDeclaration");
    if(Blueprint->Status!=BS_UpToDate && Blueprint->Status!=BS_UpToDateWithWarnings) return true;
    FProperty* P=Blueprint->GeneratedClass?FindFProperty<FProperty>(Blueprint->GeneratedClass,Variable.VarName):nullptr;
    if(!P) return Fail(Error,TEXT("Compiled variable property missing: ")+Variable.VarName.ToString());
    const UObject* CDO=Blueprint->GeneratedClass->GetDefaultObject();
    const FString Type=VariableTypeName(Variable.VarType);
    if(Type==TEXT("bool"))
    {auto* V=CastField<FBoolProperty>(P);if(!V)return Fail(Error,TEXT("Compiled variable bool type mismatch"));Text=V->GetPropertyValue_InContainer(CDO)?TEXT("true"):TEXT("false");}
    else if(Type==TEXT("float"))
    {auto* V=CastField<FDoubleProperty>(P);if(!V)return Fail(Error,TEXT("Compiled variable double type mismatch"));Text=FString::Printf(TEXT("%.17g"),V->GetPropertyValue_InContainer(CDO));}
    else if(Type==TEXT("vector"))
    {auto* V=CastField<FStructProperty>(P);if(!V || V->Struct!=TBaseStructure<FVector>::Get())return Fail(Error,TEXT("Compiled variable vector type mismatch"));Text.Reset();V->Struct->ExportText(Text,V->ContainerPtrToValuePtr<FVector>(CDO),nullptr,nullptr,PPF_None,nullptr);}
    else if(Type==TEXT("rotator"))
    {auto* V=CastField<FStructProperty>(P);if(!V || V->Struct!=TBaseStructure<FRotator>::Get())return Fail(Error,TEXT("Compiled variable rotator type mismatch"));Text.Reset();V->Struct->ExportText(Text,V->ContainerPtrToValuePtr<FRotator>(CDO),nullptr,nullptr,PPF_None,nullptr);}
    else if(Type==TEXT("string"))
    {auto* V=CastField<FStrProperty>(P);if(!V)return Fail(Error,TEXT("Compiled variable string type mismatch"));Text=V->GetPropertyValue_InContainer(CDO);}
    else if(Type==TEXT("int"))
    {auto* V=CastField<FIntProperty>(P);if(!V)return Fail(Error,TEXT("Compiled variable int32 type mismatch"));Text=FString::FromInt(V->GetPropertyValue_InContainer(CDO));}
    else if(Type==TEXT("name"))
    {auto* V=CastField<FNameProperty>(P);if(!V)return Fail(Error,TEXT("Compiled variable FName type mismatch"));Text=V->GetPropertyValue_InContainer(CDO).ToString();}
    else return Fail(Error,TEXT("Unsupported compiled variable type"));
    Source=TEXT("compiledClassDefaultObject");return true;
}
bool DeclaredCanEverTick(const FGraphDocument& Request)
{
    for (const auto& Graph : Request.Graphs) for (const auto& Node : Graph.Nodes)
        if (Node.Binding.Kind == EBindingKind::Event)
            if (const auto* Event = NodeForgeNodeDefinitions::Event(Node.Binding.Owner,Node.Binding.Member))
                if (Event->DerivesCanEverTick) return true;
    return false;
}
bool Shape(const FGraphDocument& Request, FString& Error, ENodeContext Context = ENodeContext::External)
{
    const auto* Asset = Request.Asset.TryGet<FBlueprintDefinition>();
    const auto Capabilities = NodeForgeNodeDefinitions::Capabilities(Request.CatalogDigest, Context);
    if (!Asset || !Capabilities.IsKnown() || Request.Graphs.Num() != 1 || !ExactText(Asset->ParentClass, AActor::StaticClass()->GetPathName())) return Fail(Error, TEXT("invalid Blueprint document"));
    const auto& Graph = Request.Graphs[0];
    for (const auto& Variable : Asset->Variables)
        if (AActor::StaticClass()->FindPropertyByName(FName(*Variable.Name)) || AActor::StaticClass()->FindFunctionByName(FName(*Variable.Name))) return Fail(Error, TEXT("member name conflicts with inherited Actor member"));
    TSet<FString> DeclaredEvents;
    TSet<FString> Ids;
    for (const auto& Node : Graph.Nodes)
    {
        Ids.Add(Node.Id);
        if (Node.Binding.Kind == EBindingKind::Event)
        {
            const auto* Event = NodeForgeNodeDefinitions::Event(Node.Binding.Owner, Node.Binding.Member);
            const FString Identity = Node.Binding.Owner + TEXT("|") + Node.Binding.Member;
            if (!Event || Capabilities.BlueprintRevision < Event->MinimumRevision || DeclaredEvents.Contains(Identity)) return Fail(Error, TEXT("event binding unavailable or duplicate in catalog"));
            DeclaredEvents.Add(Identity);
        }
        else if (Node.Binding.Kind == EBindingKind::Function)
        {
            const auto* Definition = NodeForgeNodeDefinitions::Function(Node.Binding.Owner, Node.Binding.Member);
            if (!Definition || !ResolveFunction(Node.Binding.Owner, Node.Binding.Member, Context) || Capabilities.BlueprintRevision < Definition->MinimumRevision) return Fail(Error, TEXT("exact function binding unavailable in catalog"));
        }
        else if (Node.Binding.Kind == EBindingKind::Member)
        {
            if (!Asset->Variables.ContainsByPredicate([&](const auto& V) { return ExactText(V.Name, Node.Binding.Name); })) return Fail(Error, TEXT("variable node requires exact declared member"));
        }
        if (!NodeForgeAssetDefinitions::NodePropertiesValid(Node.Class, Node.Properties, Error)) return Fail(Error, TEXT("invalid Blueprint configuration"));
    }
    TSet<FString> Targets; TMap<FString, int32> InDegree; TMap<FString, TArray<FString>> Children;
    for (const FString& Id : Ids) InDegree.Add(Id, 0);
    for (const auto& Edge : Graph.Edges)
    {
        if (!ExactContains(Ids, Edge.From) || !ExactContains(Ids, Edge.To) || ExactText(Edge.From, Edge.To)) return Fail(Error, TEXT("invalid edge endpoint"));
        const FString Target = Edge.To + TEXT("|") + Edge.Input;
        if (Targets.Contains(Target)) return Fail(Error, TEXT("duplicate target input"));
        Targets.Add(Target); Children.FindOrAdd(Edge.From).Add(Edge.To); ++InDegree[Edge.To];
    }
    TArray<FString> Queue;
    for (const auto& Pair : InDegree) if (!Pair.Value) Queue.Add(Pair.Key);
    int32 Visited = 0;
    while (Visited < Queue.Num()) for (const FString& Child : Children.FindOrAdd(Queue[Visited++])) if (--InDegree[Child] == 0) Queue.Add(Child);
    return Visited == Ids.Num() || Fail(Error, TEXT("cycles require an unimplemented flow adapter"));
}

UEdGraphPin* ExactPin(UEdGraphNode* Node, const FString& Name, EEdGraphPinDirection Direction)
{
    UEdGraphPin* Result = nullptr;
    for (UEdGraphPin* Pin : Node->Pins)
        if (Pin && Pin->PinName.ToString().Equals(Name, ESearchCase::CaseSensitive) && Pin->Direction == Direction && !Pin->bOrphanedPin)
        { if (Result) return nullptr; Result = Pin; }
    return Result;
}
bool SequencePins(UK2Node_ExecutionSequence* Sequence, int32 Count, FString& Error)
{
    if (!Sequence || Sequence->GetClass() != UK2Node_ExecutionSequence::StaticClass()
        || Count < MinSequenceOutputs || Count > MaxSequenceOutputs || Sequence->Pins.Num() != Count + 1)
        return Fail(Error, TEXT("Sequence native class or exact output count differs"));
    FEdGraphPinType ExecType;
    ExecType.PinCategory = UEdGraphSchema_K2::PC_Exec;
    auto NativeExec = [&](UEdGraphPin* Pin, EEdGraphPinDirection Direction)
    {
        return Pin && Pin->GetOwningNode() == Sequence && Pin->Direction == Direction && Pin->PinType == ExecType
            && !Pin->bHidden && !Pin->bNotConnectable && !Pin->bOrphanedPin && !Pin->bAdvancedView
            && !Pin->bDefaultValueIsReadOnly && !Pin->bDefaultValueIsIgnored
            && !Pin->ParentPin && Pin->SubPins.IsEmpty() && !Pin->DefaultObject
            && Pin->DefaultValue.IsEmpty() && Pin->AutogeneratedDefaultValue.IsEmpty() && Pin->DefaultTextValue.IsEmpty();
    };
    UEdGraphPin* Execute = ExactPin(Sequence, UEdGraphSchema_K2::PN_Execute.ToString(), EGPD_Input);
    if (!NativeExec(Execute, EGPD_Input) || Sequence->Pins[0] != Execute)
        return Fail(Error, TEXT("Sequence requires exactly the native leading execute input"));
    for (int32 Index = 0; Index < Count; ++Index)
    {
        UEdGraphPin* Then = Sequence->GetThenPinGivenIndex(Index);
        const FString ExpectedName = UEdGraphSchema_K2::PN_Then.ToString() + TEXT("_") + FString::FromInt(Index);
        // UE compiles sequence outputs in Pins-array order, not sorted name order.
        // Check every output; never drop missing, renamed or reordered outputs.
        if (!NativeExec(Then, EGPD_Output) || Sequence->Pins[Index + 1] != Then
            || !Then->PinName.ToString().Equals(ExpectedName, ESearchCase::CaseSensitive) || ExactPin(Sequence, ExpectedName, EGPD_Output) != Then)
            return Fail(Error, TEXT("Sequence native output name/type/order differs at index ") + FString::FromInt(Index));
    }
    return true;
}
bool NativeObjectPin(UEdGraphPin* Pin, UClass* Class)
{
    FEdGraphPinType Expected;
    Expected.PinCategory = UEdGraphSchema_K2::PC_Object;
    Expected.PinSubCategoryObject = Class;
    return Pin && Pin->PinType == Expected && !Pin->bOrphanedPin && !Pin->ParentPin && Pin->SubPins.IsEmpty()
        && !Pin->DefaultObject && Pin->DefaultValue.IsEmpty() && Pin->AutogeneratedDefaultValue.IsEmpty() && Pin->DefaultTextValue.IsEmpty();
}
bool RootComponentSource(UK2Node_CallFunction* Call, UEdGraph* Graph, UEdGraphPin*& ReturnValue, FString& Error)
{
    ReturnValue = nullptr;
    UFunction* Function = Call ? Call->GetTargetFunction() : nullptr;
    if (!Graph || !Call || Call->GetClass() != UK2Node_CallFunction::StaticClass() || Call->GetGraph() != Graph
        || !Graph->Nodes.Contains(Call) || !Function
        || Function != ResolveFunction(AActor::StaticClass()->GetPathName(), TEXT("K2_GetRootComponent"))
        || !Call->FunctionReference.IsSelfContext())
        return Fail(Error, TEXT("Component source must be same-graph native Actor.K2_GetRootComponent on implicit self"));
    UEdGraphPin* Self = ExactPin(Call, UEdGraphSchema_K2::PN_Self.ToString(), EGPD_Input);
    ReturnValue = ExactPin(Call, UEdGraphSchema_K2::PN_ReturnValue.ToString(), EGPD_Output);
    if (!NativeObjectPin(Self, AActor::StaticClass()) || !Self->LinkedTo.IsEmpty()
        || !NativeObjectPin(ReturnValue, USceneComponent::StaticClass()) || ReturnValue->bHidden)
        return Fail(Error, TEXT("RootComponent getter requires exact native Actor self and SceneComponent ReturnValue with no object/string/text defaults"));
    return true;
}
bool SceneComponentContext(UK2Node_CallFunction* Call, UEdGraph* Graph, FString& Error)
{
    UFunction* Function = Call ? Call->GetTargetFunction() : nullptr;
    if (!Graph || !Call || Call->GetClass() != UK2Node_CallFunction::StaticClass() || Call->GetGraph() != Graph
        || !Graph->Nodes.Contains(Call) || !Function || Function->GetOuterUClass() != USceneComponent::StaticClass()
        || Function != ResolveFunction(USceneComponent::StaticClass()->GetPathName(), Function->GetName())
        || Call->FunctionReference.IsSelfContext()
        || Call->FunctionReference.GetMemberParentClass() != USceneComponent::StaticClass())
        return Fail(Error, TEXT("Component call requires an allowlisted native SceneComponent external-member binding"));
    UEdGraphPin* Self = ExactPin(Call, UEdGraphSchema_K2::PN_Self.ToString(), EGPD_Input);
    if (!NativeObjectPin(Self, USceneComponent::StaticClass()) || Self->bHidden || Self->LinkedTo.Num() != 1)
        return Fail(Error, TEXT("SceneComponent self requires exactly one direct RootComponent source and no object/string/text defaults"));
    UEdGraphPin* Source = Self->LinkedTo[0];
    UK2Node_CallFunction* Root = Source ? Cast<UK2Node_CallFunction>(Source->GetOwningNode()) : nullptr;
    UEdGraphPin* ReturnValue = nullptr;
    if (!RootComponentSource(Root, Graph, ReturnValue, Error)) return false;
    if (Source != ReturnValue || !ReturnValue->LinkedTo.Contains(Self))
        return Fail(Error, TEXT("SceneComponent self must connect directly and reciprocally to Actor.K2_GetRootComponent.ReturnValue"));
    return true;
}
bool DefaultType(UEdGraphPin* Pin, const TSharedPtr<FJsonValue>& Value)
{
    if (!Pin || Pin->bHidden || Pin->bDefaultValueIsReadOnly || Pin->bDefaultValueIsIgnored || Pin->PinType.IsContainer()
        || (Pin->PinType.bIsReference && !Pin->PinType.bIsConst)) return false;
    EValueKind Kind;
    if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Float || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Double) Kind = EValueKind::Real;
    else
    {
        FEdGraphPinType Type = Pin->PinType; Type.bIsReference = false;
        if (!NodeForgeAssetDefinitions::KindFromPinType(Type, Kind)) return false;
    }
    return NodeForgeAssetDefinitions::Value(Kind, Value, false);
}
FObject PinEvidence(UEdGraphPin* Pin)
{
    FObject Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("name"), Pin->PinName.ToString());
    Result->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
    Result->SetStringField(TEXT("category"), Pin->PinType.PinCategory.ToString());
    Result->SetStringField(TEXT("subcategory"), Pin->PinType.PinSubCategory.ToString());
    Result->SetStringField(TEXT("subtype"), Pin->PinType.PinSubCategoryObject.IsValid() ? Pin->PinType.PinSubCategoryObject->GetPathName() : TEXT(""));
    Result->SetBoolField(TEXT("hidden"), Pin->bHidden);
    Result->SetBoolField(TEXT("defaultWritable"), Pin->Direction == EGPD_Input && !Pin->bHidden && !Pin->bDefaultValueIsReadOnly && !Pin->bDefaultValueIsIgnored
        && !Pin->PinType.IsContainer() && (!Pin->PinType.bIsReference || Pin->PinType.bIsConst)
        && (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_String || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Name
            || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real
            || (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct && (Pin->PinType.PinSubCategoryObject == TBaseStructure<FVector>::Get()
                || Pin->PinType.PinSubCategoryObject == TBaseStructure<FRotator>::Get()))));
    Result->SetStringField(TEXT("defaultText"), Pin->DefaultValue);
    Result->SetStringField(TEXT("defaultObject"), Pin->DefaultObject ? Pin->DefaultObject->GetPathName() : TEXT(""));
    if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
    {
        Result->SetNumberField(TEXT("defaultMinimum"), MIN_int32);
        Result->SetNumberField(TEXT("defaultMaximum"), MAX_int32);
        Result->SetBoolField(TEXT("defaultIntegral"), true);
    }
    if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Name)
        Result->SetNumberField(TEXT("defaultMaximumLength"), NAME_SIZE - 1);
    return Result;
}
FObject NodeEvidence(UEdGraphNode* Node)
{
    FObject Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("id"), Node->NodeComment.Mid(IdPrefix.Len()));
    Result->SetStringField(TEXT("class"), Node->GetClass()->GetPathName());
    FObject Props = MakeShared<FJsonObject>();
    if (UK2Node_Event* Event = Cast<UK2Node_Event>(Node))
    {
        Props->SetStringField(TEXT("owner"), GetPathNameSafe(Event->EventReference.GetMemberParentClass()));
        Props->SetStringField(TEXT("member"), Event->EventReference.GetMemberName().ToString());
    }
    else if (UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node))
    {
        UFunction* Function = Call->GetTargetFunction();
        Props->SetStringField(TEXT("owner"), Function ? Function->GetOuterUClass()->GetPathName() : TEXT(""));
        Props->SetStringField(TEXT("member"), Function ? Function->GetName() : TEXT(""));
    }
    else if (UK2Node_Variable* Variable = Cast<UK2Node_Variable>(Node))
        Props->SetStringField(TEXT("variable"), Variable->VariableReference.GetMemberName().ToString());
    else if (UK2Node_ExecutionSequence* Sequence = Cast<UK2Node_ExecutionSequence>(Node))
        Props->SetNumberField(TEXT("outputCount"), Sequence->Pins.Num() - 1);
    else if (NodeForgeBlueprintSwitches::ClassFor(Node->GetClass()->GetPathName()))
        Props = NodeForgeBlueprintSwitches::Properties(Node);
    Result->SetObjectField(TEXT("properties"), Props);
    FValues Pins;
    for (UEdGraphPin* Pin : Node->Pins) if (Pin) Pins.Add(MakeShared<FJsonValueObject>(PinEvidence(Pin)));
    Result->SetArrayField(TEXT("pins"), Pins);
    Result->SetArrayField(TEXT("position"), {MakeShared<FJsonValueNumber>(Node->NodePosX), MakeShared<FJsonValueNumber>(Node->NodePosY)});
    return Result;
}

UBlueprint* BuildChecked(UPackage* Package, const FString& Name, const FGraphDocument& Request, FString& Error, ENodeContext Context = ENodeContext::External)
{
    UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(AActor::StaticClass(), Package, FName(*Name), BPTYPE_Normal,
        UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    if (!Blueprint) { Fail(Error, TEXT("Native blueprint factory failed")); return nullptr; }
    auto Abort = [&](const FString& Message) -> UBlueprint*
    {
        Fail(Error,Message);Blueprint->ClearFlags(RF_Public|RF_Standalone);Blueprint->SetFlags(RF_Transient);
        if(Package==GetTransientPackage())Blueprint->MarkAsGarbage();
        return nullptr; // Saved/attempted product candidates retain the original residue policy.
    };
    if (!Blueprint->SimpleConstructionScript)
        Blueprint->SimpleConstructionScript = NewObject<USimpleConstructionScript>(Blueprint->GeneratedClass, NAME_None, RF_Transactional);
    Blueprint->SimpleConstructionScript->ValidateSceneRootNodes();
    if (!FBlueprintEditorUtils::FindUserConstructionScript(Blueprint)) FKismetEditorUtilities::CreateUserConstructionScript(Blueprint);
    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    if (!Graph)
    {
        Graph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, TEXT("EventGraph"), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
        FBlueprintEditorUtils::AddUbergraphPage(Blueprint, Graph);
    }
    // Only a newly-created candidate is touched: native starter events are removed, then exactly
    // the JSON-declared EventGraph is created. ConstructionScript/default root stay as scaffolding.
    const auto FactoryNodes = Graph->Nodes;
    for (UEdGraphNode* Node : FactoryNodes) FBlueprintEditorUtils::RemoveNode(Blueprint, Node, true);
    const auto& Asset = Request.Asset.Get<FBlueprintDefinition>();
    for (const auto& Definition : Asset.Variables)
    {
        FEdGraphPinType Type; NodeForgeAssetDefinitions::PinType(Definition.Type.Kind, Type);
        if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*Definition.Name), Type, NativeText(Definition.Default))) return Abort(TEXT("native AddMemberVariable failed"));
        ApplyAttributes(Blueprint,Definition);
    }
    TMap<FString, UEdGraphNode*> Instances;
    TArray<FNodeDescription> Ordered = Request.Graphs[0].Nodes;
    Ordered.Sort([](const FNodeDescription& A, const FNodeDescription& B) { return A.Id < B.Id; });
    int32 NodeIndex = 0;
    for (const FNodeDescription& Definition : Ordered)
    {
        const FString& Id = Definition.Id; const FString& Class = Definition.Class;
        const FValueMap& Props = Definition.Properties;
        FVector2D Position(NodeIndex * 280, 0); ++NodeIndex;
        if (Definition.Position.IsSet()) Position = FVector2D(Definition.Position->X, Definition.Position->Y);
        UEdGraphNode* NewNode = nullptr;
        if (ExactText(Class, UK2Node_Event::StaticClass()->GetPathName()))
        {
            const auto* Event = NodeForgeNodeDefinitions::Event(Definition.Binding.Owner, Definition.Binding.Member);
            UFunction* Function = Event ? Event->Owner->FindFunctionByName(FName(*Event->Member)) : nullptr;
            UBlueprintEventNodeSpawner* Spawner = UBlueprintEventNodeSpawner::Create(Function, Blueprint);
            NewNode = Spawner ? Spawner->Invoke(Graph, IBlueprintNodeBinder::FBindingSet(), Position) : nullptr;
        }
        else if (ExactText(Class, UK2Node_CallFunction::StaticClass()->GetPathName()))
        {
            UFunction* Function = ResolveFunction(Definition.Binding.Owner, Definition.Binding.Member, Context);
            bool InDatabase = false;
            UBlueprintNodeSpawner* Spawner = FunctionSpawner(Function, Blueprint, InDatabase);
            NewNode = Spawner ? Spawner->Invoke(Graph, IBlueprintNodeBinder::FBindingSet(), Position) : nullptr;
        }
        else if (ExactText(Class, UK2Node_IfThenElse::StaticClass()->GetPathName()))
        {
            UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(UK2Node_IfThenElse::StaticClass(), Blueprint);
            NewNode = Spawner ? Spawner->Invoke(Graph, IBlueprintNodeBinder::FBindingSet(), Position) : nullptr;
        }
        else if (ExactText(Class, UK2Node_ExecutionSequence::StaticClass()->GetPathName()))
        {
            UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(UK2Node_ExecutionSequence::StaticClass(), Blueprint);
            NewNode = Spawner ? Spawner->Invoke(Graph, IBlueprintNodeBinder::FBindingSet(), Position) : nullptr;
        }
        else if (NodeForgeBlueprintSwitches::ClassFor(Class))
            NewNode = NodeForgeBlueprintSwitches::Spawn(Graph, Blueprint, Class, Props, Position, Error);
        else
        {
            // Adapted selected self-variable branch and initialization from AddNodeCore.
            // These new Blueprint members do not yet have a stable compiled FProperty for
            // BlueprintVariableNodeSpawner; bind the native K2 variable reference by exact name.
            UK2Node_Variable* Variable = ExactText(Class, UK2Node_VariableGet::StaticClass()->GetPathName())
                ? static_cast<UK2Node_Variable*>(NewObject<UK2Node_VariableGet>(Graph, NAME_None, RF_Transactional))
                : static_cast<UK2Node_Variable*>(NewObject<UK2Node_VariableSet>(Graph, NAME_None, RF_Transactional));
            Variable->VariableReference.SetSelfMember(FName(*Definition.Binding.Name));
            Graph->AddNode(Variable, false, false); Variable->CreateNewGuid(); Variable->PostPlacedNewNode(); Variable->AllocateDefaultPins();
            NewNode = Variable;
        }
        if (!NewNode || NewNode->GetClass()->GetPathName() != Class || !Graph->Nodes.Contains(NewNode)) return Abort(TEXT("Native spawner returned no node or unexpected class/graph"));
        if (!NewNode->NodeComment.IsEmpty() && NewNode->NodeComment.StartsWith(IdPrefix, ESearchCase::CaseSensitive)) return Abort(TEXT("Spawner reused another declared node"));
        if (UK2Node_ExecutionSequence* Sequence = Cast<UK2Node_ExecutionSequence>(NewNode))
        {
            int32 Count = 0;
            if (!SequenceOutputCount(Props, Count, Error) || !SequencePins(Sequence, MinSequenceOutputs, Error)) return Abort(Error);
            // MinimalAPI: dispatch the non-exported override through the public
            // virtual interface. Do not call qualified native implementation/helpers.
            IK2Node_AddPinInterface* AddPin = Sequence;
            for (int32 Existing = MinSequenceOutputs; Existing < Count; ++Existing)
            {
                AddPin->AddInputPin();
                if (!SequencePins(Sequence, Existing + 1, Error)) return Abort(Error);
            }
        }
        NewNode->NodePosX = static_cast<int32>(Position.X); NewNode->NodePosY = static_cast<int32>(Position.Y);
        NewNode->NodeComment = IdPrefix + Id; NewNode->bCommentBubbleVisible = false;
        Instances.Add(Id, NewNode);
    }
    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
    for (const FNodeDescription& Definition : Ordered)
        for (const auto& Pair : Definition.Defaults)
        {
            UEdGraphPin* Pin = ExactPin(Instances[Definition.Id], Pair.Key, EGPD_Input);
            if (Pin && NodeForgeNodeDefinitions::Capabilities(Request.CatalogDigest, Context).BlueprintRevision < 2 && (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Name)) return Abort(TEXT("integer/name defaults unavailable in archived catalog"));
            if (!DefaultType(Pin, Pair.Value)) return Abort(TEXT("unsupported default input: ") + Definition.Id + TEXT(".") + Pair.Key);
            const FString Value = NativeText(Pair.Value, true);
            const FString Invalid = Schema->IsPinDefaultValid(Pin, Value, nullptr, FText::GetEmpty());
            if (!Invalid.IsEmpty()) return Abort(TEXT("invalid pin default: ") + Invalid);
            Schema->TrySetDefaultValue(*Pin, Value);
            if (!MatchesNativeText(Pair.Value, Pin->DefaultValue, true) || Pin->DefaultObject) return Abort(TEXT("pin default readback mismatch"));
        }
    for (const FEdgeDescription& Edge : Request.Graphs[0].Edges)
    {
        UEdGraphPin* From = ExactPin(Instances[Edge.From], Edge.Output, EGPD_Output);
        UEdGraphPin* To = ExactPin(Instances[Edge.To], Edge.Input, EGPD_Input);
        if (!From || !To || From->bHidden || To->bHidden) return Abort(TEXT("missing, hidden, ambiguous or wrong-direction pin"));
        const auto* Target = Request.Graphs[0].FindNode(Edge.To);
        if (Target && Target->Defaults.Contains(Edge.Input)) return Abort(TEXT("connected input also declares a default"));
        const FPinConnectionResponse Response = Schema->CanCreateConnection(From, To);
        if (Response.Response != CONNECT_RESPONSE_MAKE) return Abort(TEXT("connection requires conversion/replacement or is invalid: ") + Response.Message.ToString());
        if (!Schema->TryCreateConnection(From, To)) return Abort(TEXT("native K2 connection failed"));
    }
    FObject Layout;
    if (!NodeForgeGraphLayout::Blueprint(Request,Instances,Layout,Error)) return Abort(Error);
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    FObject Evidence;
    if (!ReadbackChecked(Blueprint, Request, Evidence, Error, nullptr, Context)) return Abort(Error);
    return Blueprint;
}
}

UBlueprint* Build(UPackage* Package, const FString& Name, const FGraphDocument& Request, FString& Error)
{
    if (!Package || !Shape(Request, Error)) return nullptr;
    return BuildChecked(Package, Name, Request, Error);
}
bool Validate(const FGraphDocument& Request, FString& Error)
{
    if (!Shape(Request, Error)) return false;
    const FName Name = MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(), TEXT("NodeForgeGraphValidation"));
    UBlueprint* Candidate = BuildChecked(GetTransientPackage(), Name.ToString(), Request, Error);
    if (!Candidate) return false;
    Candidate->ClearFlags(RF_Public | RF_Standalone); Candidate->SetFlags(RF_Transient);
    Candidate->MarkAsGarbage();
    return true;
}

bool Readback(UBlueprint* Blueprint, const FGraphDocument& Request, FObject& Evidence, FString& Error,
    const TMap<UEdGraphNode*, FString>* ReadOnlyIds)
{
    return ReadbackChecked(Blueprint, Request, Evidence, Error, ReadOnlyIds, ENodeContext::External);
}

static bool ReadbackChecked(UBlueprint* Blueprint, const FGraphDocument& Request, FObject& Evidence, FString& Error,
    const TMap<UEdGraphNode*, FString>* ReadOnlyIds, NodeForgeNodeDefinitions::EContext Context)
{
    Evidence = MakeShared<FJsonObject>();
    if (!Blueprint || !Shape(Request, Error, Context)) return false;
    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    if (!Graph || Blueprint->ParentClass != AActor::StaticClass() || Blueprint->UbergraphPages.Num() != 1
        || !Blueprint->SimpleConstructionScript || !FBlueprintEditorUtils::FindUserConstructionScript(Blueprint))
        return Fail(Error, TEXT("Blueprint parent, graph or Actor scaffolding mismatch"));
    const auto& DeclaredNodes = Request.Graphs[0].Nodes;
    if (Graph->Nodes.Num() != DeclaredNodes.Num()) return Fail(Error, TEXT("Actual node count differs from JSON"));
    TMap<FString, UEdGraphNode*> Instances;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (!Node || (ReadOnlyIds ? !ReadOnlyIds->Contains(Node) : !Node->NodeComment.StartsWith(IdPrefix, ESearchCase::CaseSensitive))) return Fail(Error, TEXT("Undeclared EventGraph node"));
        const FString Id = ReadOnlyIds ? ReadOnlyIds->FindChecked(Node) : Node->NodeComment.Mid(IdPrefix.Len());
        if (!Identifier(Id) || Instances.Contains(Id)) return Fail(Error, TEXT("Missing or duplicate logical identity on asset"));
        Instances.Add(Id, Node);
    }
    FValues ActualNodes;
    FObject Layout;
    if (!NodeForgeGraphLayout::Blueprint(Request,Instances,Layout,Error,false)) return false;
    TMap<FString,FVector2D> ExpectedPositions;
    for (const auto& Value : Layout->GetArrayField(TEXT("nodes")))
    {
        auto N=Value->AsObject(); const auto& P=N->GetArrayField(TEXT("position"));
        ExpectedPositions.Add(N->GetStringField(TEXT("id")),FVector2D(P[0]->AsNumber(),P[1]->AsNumber()));
    }
    Evidence->SetObjectField(TEXT("layout"),Layout);
    for (const FNodeDescription& Definition : DeclaredNodes)
    {
        UEdGraphNode** Found = Instances.Find(Definition.Id);
        if (!Found || !ExactText((*Found)->GetClass()->GetPathName(), Definition.Class)
            || !ExactText(ReadOnlyIds ? ReadOnlyIds->FindChecked(*Found) : (*Found)->NodeComment.Mid(IdPrefix.Len()), Definition.Id)) return Fail(Error, TEXT("actual node identity/class mismatch"));
        UEdGraphNode* Node = *Found;
        if (UK2Node_ExecutionSequence* Sequence = Cast<UK2Node_ExecutionSequence>(Node))
        {
            int32 Count = 0;
            if (!SequenceOutputCount(Definition.Properties, Count, Error) || !SequencePins(Sequence, Count, Error)) return false;
        }
        FObject Actual = NodeEvidence(Node), ActualProps = FieldObject(Actual, TEXT("properties"));
        Actual->SetStringField(TEXT("id"), Definition.Id);
        const FVector2D ExpectedPosition = ExpectedPositions.FindChecked(Definition.Id);
        if (Node->NodePosX != ExpectedPosition.X || Node->NodePosY != ExpectedPosition.Y) return Fail(Error, TEXT("automatic or explicit layout changed: ") + Definition.Id);
        const auto& Binding = Definition.Binding;
        if (Binding.Kind == EBindingKind::Event || Binding.Kind == EBindingKind::Function)
        {
            if (!ExactText(ActualProps->GetStringField(TEXT("owner")), Binding.Owner) || !ExactText(ActualProps->GetStringField(TEXT("member")), Binding.Member)) return Fail(Error, TEXT("actual native event/function binding differs"));
        }
        else if (Binding.Kind == EBindingKind::Member)
        {
            if (!ExactText(ActualProps->GetStringField(TEXT("variable")), Binding.Name)) return Fail(Error, TEXT("actual native member binding differs"));
        }
        else if (NodeForgeBlueprintSwitches::ClassFor(Node->GetClass()->GetPathName()))
        { if (!NodeForgeBlueprintSwitches::Verify(Node, Definition.Properties, Error)) return Fail(Error, Error); }
        else
        {
            if (ActualProps->Values.Num() != Definition.Properties.Num()) return Fail(Error, TEXT("actual ordinary configuration count differs"));
            for (const auto& Pair : Definition.Properties)
                if (!NodeForgeGraphDiff::EqualValue(Pair.Value, ActualProps->TryGetField(Pair.Key))) return Fail(Error, TEXT("actual ordinary configuration differs"));
        }
        if (auto* Event = Cast<UK2Node_Event>(Node)) if (!Event->bOverrideFunction) return Fail(Error, TEXT("Expected native override event"));
        if (auto* Variable = Cast<UK2Node_Variable>(Node)) if (!Variable->VariableReference.IsSelfContext()) return Fail(Error, TEXT("Only own declared member variables are supported"));
        if (auto* Call = Cast<UK2Node_CallFunction>(Node))
            if (UFunction* Function = Call->GetTargetFunction())
                if (!Function->HasAnyFunctionFlags(FUNC_Static))
                {
                    if (Function->GetOuterUClass() == USceneComponent::StaticClass())
                    {
                        if (!SceneComponentContext(Call, Graph, Error)) return false;
                        Actual->SetStringField(TEXT("functionContext"), TEXT("native SceneComponent target from own Actor RootComponent"));
                    }
                    else
                    {
                        UEdGraphPin* Self = Call->FindPin(UEdGraphSchema_K2::PN_Self, EGPD_Input);
                        if (Function->GetOuterUClass() != AActor::StaticClass() || !Call->FunctionReference.IsSelfContext()
                            || !Self || Self->LinkedTo.Num() || Self->DefaultObject)
                            return Fail(Error, TEXT("Native Actor calls require implicit self context; external targets are unsupported"));
                        if (Function->GetName() == TEXT("K2_GetRootComponent"))
                        {
                            UEdGraphPin* ReturnValue = nullptr;
                            if (!RootComponentSource(Call, Graph, ReturnValue, Error)) return false;
                            for (UEdGraphPin* Target : ReturnValue->LinkedTo)
                            {
                                UK2Node_CallFunction* Component = Target ? Cast<UK2Node_CallFunction>(Target->GetOwningNode()) : nullptr;
                                if (!Component || Target != ExactPin(Component, UEdGraphSchema_K2::PN_Self.ToString(), EGPD_Input)
                                    || !Target->LinkedTo.Contains(ReturnValue))
                                    return Fail(Error, TEXT("RootComponent ReturnValue may connect only to supported SceneComponent self inputs"));
                                if (!SceneComponentContext(Component, Graph, Error)) return false;
                            }
                        }
                        Actual->SetStringField(TEXT("functionContext"), TEXT("native implicit self Actor"));
                    }
                }
        if (Definition.Position.IsSet() && (Node->NodePosX != Definition.Position->X || Node->NodePosY != Definition.Position->Y)) return Fail(Error, TEXT("node position mismatch"));
        auto ActualDefaults = MakeShared<FJsonObject>();
        for (const auto& Pair : Definition.Defaults)
        {
            UEdGraphPin* Pin = ExactPin(Node, Pair.Key, EGPD_Input);
            if (!DefaultType(Pin, Pair.Value) || Pin->LinkedTo.Num() || Pin->DefaultObject || !MatchesNativeText(Pair.Value, Pin->DefaultValue, true)) return Fail(Error, TEXT("Actual default differs from JSON: ") + Pair.Key);
            ActualDefaults->SetField(Pair.Key, FromNativeText(Pair.Value, Pin->DefaultValue, true));
        }
        Actual->SetObjectField(TEXT("defaults"), ActualDefaults); ActualNodes.Add(MakeShared<FJsonValueObject>(Actual));
    }
    TSet<FString> ExpectedEdges;
    auto EdgeKey = [](const FString& From, const FString& Output, const FString& To, const FString& Input) { return From + TEXT("|") + Output + TEXT("|") + To + TEXT("|") + Input; };
    for (const auto& E : Request.Graphs[0].Edges) ExpectedEdges.Add(EdgeKey(E.From, E.Output, E.To, E.Input));
    FValues ActualEdges; TSet<FString> FoundEdges;
    for (const auto& Pair : Instances)
        for (UEdGraphPin* Pin : Pair.Value->Pins)
            if (Pin && Pin->Direction == EGPD_Output)
                for (UEdGraphPin* Target : Pin->LinkedTo)
                {
                    UEdGraphNode* Owner = Target ? Target->GetOwningNode() : nullptr;
                    if (!Owner || Target->Direction != EGPD_Input || (ReadOnlyIds ? !ReadOnlyIds->Contains(Owner) : !Owner->NodeComment.StartsWith(IdPrefix, ESearchCase::CaseSensitive)) || !Target->LinkedTo.Contains(Pin))
                        return Fail(Error, TEXT("Invalid actual link"));
                    const FString To = ReadOnlyIds ? ReadOnlyIds->FindChecked(Owner) : Owner->NodeComment.Mid(IdPrefix.Len());
                    if (Instances.FindRef(To) != Owner) return Fail(Error, TEXT("Link leaves declared graph"));
                    const FString Key = EdgeKey(Pair.Key, Pin->PinName.ToString(), To, Target->PinName.ToString());
                    if (!ExactContains(ExpectedEdges, Key) || FoundEdges.Contains(Key)) return Fail(Error, TEXT("Undeclared/duplicate actual link: ") + Key);
                    FoundEdges.Add(Key);
                    FObject E = MakeShared<FJsonObject>(); E->SetStringField(TEXT("from"), Pair.Key); E->SetStringField(TEXT("output"), Pin->PinName.ToString());
                    E->SetStringField(TEXT("to"), To); E->SetStringField(TEXT("input"), Target->PinName.ToString()); ActualEdges.Add(MakeShared<FJsonValueObject>(E));
                }
    if (FoundEdges.Num() != ExpectedEdges.Num()) return Fail(Error, TEXT("Declared link missing from actual graph"));
    const auto& Definitions = Request.Asset.Get<FBlueprintDefinition>().Variables;
    if (Definitions.Num() != Blueprint->NewVariables.Num()) return Fail(Error, TEXT("declared variable count mismatch"));
    FValues ActualVariables;
    for (const FMemberDescription& Definition : Definitions)
    {
        const FString& Name = Definition.Name;
        const FBPVariableDescription* Actual = Blueprint->NewVariables.FindByPredicate([&](const FBPVariableDescription& V) { return ExactText(V.VarName.ToString(), Name); });
        if (!Actual || !ExactText(VariableTypeName(Actual->VarType), NodeForgeAssetDefinitions::TypeName(Definition.Type.Kind))) return Fail(Error, TEXT("actual variable definition/type differs: ") + Name);
        if(!AttributesMatch(Blueprint,*Actual,Definition,Error))return false;
        FString ActualDefault, DefaultSource;
        if (!VariableDefault(Blueprint, *Actual, ActualDefault, DefaultSource, Error)) return false;
        if (!NodeForgeAssetDefinitions::MemberTextMatches(Definition.Type.Kind, Definition.Default, ActualDefault)) return Fail(Error, TEXT("Actual variable default differs: ") + Name);
        auto V = MakeShared<FJsonObject>(); V->SetStringField(TEXT("name"), Actual->VarName.ToString()); V->SetStringField(TEXT("type"), VariableTypeName(Actual->VarType));
        V->SetField(TEXT("default"), FromNativeText(Definition.Default, ActualDefault)); V->SetStringField(TEXT("defaultSource"), DefaultSource); V->SetObjectField(TEXT("attributes"),NodeForgeAssetDefinitions::EncodeMemberAttributes(ActualAttributes(*Actual))); ActualVariables.Add(MakeShared<FJsonValueObject>(V));
    }
    FObject Settings = MakeShared<FJsonObject>(); Settings->SetStringField(TEXT("parentClass"), Blueprint->ParentClass->GetPathName()); Settings->SetArrayField(TEXT("variables"), ActualVariables);
    FObject Scaffolding = MakeShared<FJsonObject>(); Scaffolding->SetBoolField(TEXT("nativeConstructionScript"), true); Scaffolding->SetBoolField(TEXT("nativeSceneRoot"), Blueprint->SimpleConstructionScript->GetRootNodes().Num() > 0);
    Scaffolding->SetStringField(TEXT("logicalIdStorage"), ReadOnlyIds?TEXT("read-only external node-to-ID mapping; source comments unchanged"):TEXT("EventGraph.NodeComment=NodeForge:<id>"));
    Scaffolding->SetStringField(TEXT("factoryEventRule"), TEXT("Remove factory starter events in new candidate; create exactly JSON declared EventGraph nodes"));
    Scaffolding->SetNumberField(TEXT("constructionGraphCount"), Blueprint->FunctionGraphs.Num());
    Evidence->SetArrayField(TEXT("nodes"), ActualNodes); Evidence->SetArrayField(TEXT("edges"), ActualEdges); Evidence->SetObjectField(TEXT("settings"), Settings); Evidence->SetObjectField(TEXT("scaffolding"), Scaffolding);
    if (Blueprint->Status==BS_UpToDate || Blueprint->Status==BS_UpToDateWithWarnings)
    {
        const AActor* Actor=Blueprint->GeneratedClass->GetDefaultObject<AActor>();
        const AActor* Native=AActor::StaticClass()->GetDefaultObject<AActor>();
        bool TickMatches=Actor && Actor->PrimaryActorTick.bCanEverTick==(Native->PrimaryActorTick.bCanEverTick || DeclaredCanEverTick(Request));
        if (Actor) for (TFieldIterator<FProperty> It(FActorTickFunction::StaticStruct());It;++It)
            if (It->GetFName()!=TEXT("bCanEverTick") && !It->Identical_InContainer(&Actor->PrimaryActorTick,&Native->PrimaryActorTick)) TickMatches=false;
        if (!TickMatches
            || Actor->PrimaryActorTick.bHighPriority!=Native->PrimaryActorTick.bHighPriority
            || Actor->PrimaryActorTick.bRunOnAnyThread!=Native->PrimaryActorTick.bRunOnAnyThread) return Fail(Error,TEXT("compiled Actor Tick state differs from exact event-derived native expectation"));
        auto Tick=MakeShared<FJsonObject>();Tick->SetBoolField(TEXT("bCanEverTick"),Actor->PrimaryActorTick.bCanEverTick);
        Tick->SetBoolField(TEXT("bStartWithTickEnabled"),Actor->PrimaryActorTick.bStartWithTickEnabled);Tick->SetNumberField(TEXT("tickInterval"),Actor->PrimaryActorTick.TickInterval);
        Tick->SetStringField(TEXT("expectation"),TEXT("Native Actor Tick structure with only declared event-derived bCanEverTick"));Evidence->SetObjectField(TEXT("compiledTickState"),Tick);
    }
    Evidence->SetBoolField(TEXT("matchesInput"), true);
    return true;
}

bool Export(const FObject& Config,const FString& CatalogDigest,FObject& Request,FObject& Evidence,FString& Error)
{
    Request.Reset();Evidence.Reset();Error.Reset();FString Source,Id,Target;int32 FormatVersion=2;
    if(!NodeForgeAssetDefinitions::ExportVersion(Config,FormatVersion,Error)
        || !String(Config,TEXT("source"),Source) || !String(Config,TEXT("requestId"),Id) || !Identifier(Id)
        || !String(Config,TEXT("assetPath"),Target) || !Source.StartsWith(TEXT("/Game/NodeForgeTests/"))
        || Source.Contains(TEXT("..")) || !FPackageName::IsValidObjectPath(Source))
        return Fail(Error,TEXT("export requires exact saved source and new ASCII requestId/assetPath in NodeForgeTests"));
    const FString SourcePackage=FPackageName::ObjectPathToPackageName(Source);
    if(!NodeForgePrototype::ValidateNewTargetIdentity(Source,Id,Target,Error))
        return Fail(Error,TEXT("export target exists or equals source; no overwrite"));
    UBlueprint* BP=LoadObject<UBlueprint>(nullptr,*Source);
    if(!BP || BP->GetClass()!=UBlueprint::StaticClass() || !ExactText(BP->GetPathName(),Source) || BP->GetPackage()->IsDirty()
        || BP->BlueprintType!=BPTYPE_Normal || BP->ParentClass!=AActor::StaticClass() || !BP->GeneratedClass
        || (BP->Status!=BS_UpToDate && BP->Status!=BS_UpToDateWithWarnings))
        return Fail(Error,TEXT("source must be a clean saved compiled ordinary Actor Blueprint"));
    const FString File=FPackageName::LongPackageNameToFilename(SourcePackage,FPackageName::GetAssetPackageExtension());
    const FString Before=NodeForgePrototype::FileSha256(File);
    if(Before.Len()!=64) return Fail(Error,TEXT("cannot fingerprint source Blueprint"));
    UEdGraph* Graph=FBlueprintEditorUtils::FindEventGraph(BP);
    UEdGraph* Construction=FBlueprintEditorUtils::FindUserConstructionScript(BP);
    if(!Graph || Graph->GetClass()!=UEdGraph::StaticClass() || !Graph->GetSchema() || Graph->GetSchema()->GetClass()!=UEdGraphSchema_K2::StaticClass()
        || BP->UbergraphPages.Num()!=1 || BP->FunctionGraphs.Num()!=1 || BP->FunctionGraphs[0]!=Construction
        || !Construction || BP->MacroGraphs.Num()!=0 || BP->DelegateSignatureGraphs.Num()!=0 || BP->ImplementedInterfaces.Num()!=0
        || BP->Timelines.Num()!=0 || !BP->SimpleConstructionScript || !Graph->SubGraphs.IsEmpty() || !Construction->SubGraphs.IsEmpty())
        return Fail(Error,TEXT("only one EventGraph and native empty construction scaffolding; functions, macros, interfaces, delegates, timelines and subgraphs unsupported"));
    TArray<UEdGraphNode*> Actual;
    for(UEdGraphNode* N:Graph->Nodes){if(!N)return Fail(Error,TEXT("null source node"));Actual.Add(N);}
    Actual.Sort([](const UEdGraphNode& A,const UEdGraphNode& B){return A.GetName()<B.GetName();});
    TMap<UEdGraphNode*,FString> Ids;FValues Nodes,Edges,Mapping,Variables;
    FGraphDocument Document;Document.RequestId=Id;Document.Target=Target;Document.CatalogDigest=CatalogDigest;
    FBlueprintDefinition Asset;Asset.ParentClass=BP->ParentClass->GetPathName();
    FGraphDefinition Description;Description.Id=TEXT("EventGraph");Description.Kind=TEXT("EventGraph");
    for(int32 I=0;I<Actual.Num();++I)
    {
        if(!Actual[I])return Fail(Error,TEXT("null native node"));
        Ids.Add(Actual[I],FString::Printf(TEXT("Node%d"),I+1));
    }
    for(const FBPVariableDescription& V:BP->NewVariables)
    {
        const FString Type=VariableTypeName(V.VarType);FString Text,Origin;
        if(Type==TEXT("unsupported") || V.VarType.IsContainer() || V.VarType.bIsReference || !VariableDefault(BP,V,Text,Origin,Error))
            return Fail(Error,TEXT("unexpressible member variable: ")+V.VarName.ToString());
        TSharedPtr<FJsonValue> Kind=Type==TEXT("bool")?TSharedPtr<FJsonValue>(MakeShared<FJsonValueBoolean>(false)):
            (Type==TEXT("float") || Type==TEXT("int"))?TSharedPtr<FJsonValue>(MakeShared<FJsonValueNumber>(0)):
            Type==TEXT("vector")?VectorJson(FVector::ZeroVector):Type==TEXT("rotator")?RotatorJson(FRotator::ZeroRotator):TSharedPtr<FJsonValue>(MakeShared<FJsonValueString>(TEXT("")));
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("name"),V.VarName.ToString());Row->SetStringField(TEXT("type"),Type);
        const auto ActualDefault=FromNativeText(Kind,Text);
        Row->SetField(TEXT("default"),ActualDefault);Variables.Add(MakeShared<FJsonValueObject>(Row));
        FMemberDescription Member;Member.Name=V.VarName.ToString();NodeForgeAssetDefinitions::TypeFromName(Type,Member.Type.Kind);Member.Default=ActualDefault;
        Member.Attributes=ActualAttributes(V);
        if(!NodeForgeAssetDefinitions::MemberAttributesValid(Member.Attributes.GetValue()) || !AttributesMatch(BP,V,Member,Error)) return Fail(Error,TEXT("unexpressible native member attributes: ")+V.VarName.ToString());
        if(!NodeForgeAssetDefinitions::Value(Member.Type.Kind,Member.Default))return Fail(Error,TEXT("native member default cannot be represented"));
        Asset.Variables.Add(MoveTemp(Member));
    }
    for(UEdGraphNode* Node:Actual)
    {
        if (UK2Node_ExecutionSequence* Sequence = Cast<UK2Node_ExecutionSequence>(Node))
            if (!SequencePins(Sequence, Sequence->Pins.Num() - 1, Error)) return false;
        if (NodeForgeBlueprintSwitches::ClassFor(Node->GetClass()->GetPathName()))
            if (!NodeForgeBlueprintSwitches::Verify(Node, NodeForgeBlueprintSwitches::Properties(Node)->Values, Error)) return false;
        auto N=NodeEvidence(Node);N->RemoveField(TEXT("pins"));N->SetStringField(TEXT("id"),Ids[Node]);
        FNodeDescription NativeNode;NativeNode.Id=Ids[Node];NativeNode.Class=Node->GetClass()->GetPathName();NativeNode.Position=FIntPoint(Node->NodePosX,Node->NodePosY);
        if(auto* Event=Cast<UK2Node_Event>(Node))
        {NativeNode.Binding.Kind=EBindingKind::Event;NativeNode.Binding.Owner=GetPathNameSafe(Event->EventReference.GetMemberParentClass());NativeNode.Binding.Member=Event->EventReference.GetMemberName().ToString();}
        else if(auto* Call=Cast<UK2Node_CallFunction>(Node))
        {auto* Function=Call->GetTargetFunction();NativeNode.Binding.Kind=EBindingKind::Function;NativeNode.Binding.Owner=Function?Function->GetOuterUClass()->GetPathName():FString();NativeNode.Binding.Member=Function?Function->GetName():FString();}
        else if(auto* Variable=Cast<UK2Node_Variable>(Node))
        {NativeNode.Binding.Kind=EBindingKind::Member;NativeNode.Binding.Name=Variable->VariableReference.GetMemberName().ToString();}
        else NativeNode.Properties=FieldObject(N,TEXT("properties"))->Values;
        auto Defaults=MakeShared<FJsonObject>();
        for(UEdGraphPin* Pin:Node->Pins)
        {
            if(!Pin || Pin->bOrphanedPin || Pin->ParentPin || !Pin->SubPins.IsEmpty()) return Fail(Error,TEXT("orphaned/split/dynamic pin requires an export adapter"));
            if(Pin->Direction==EGPD_Input && Pin->LinkedTo.IsEmpty())
            {
                TSharedPtr<FJsonValue> Kind;
                if(Pin->PinType.PinCategory==UEdGraphSchema_K2::PC_String || Pin->PinType.PinCategory==UEdGraphSchema_K2::PC_Name) Kind=MakeShared<FJsonValueString>(TEXT(""));
                else if(Pin->PinType.PinCategory==UEdGraphSchema_K2::PC_Boolean) Kind=MakeShared<FJsonValueBoolean>(false);
                else if(Pin->PinType.PinCategory==UEdGraphSchema_K2::PC_Int || Pin->PinType.PinCategory==UEdGraphSchema_K2::PC_Real || Pin->PinType.PinCategory==UEdGraphSchema_K2::PC_Float || Pin->PinType.PinCategory==UEdGraphSchema_K2::PC_Double) Kind=MakeShared<FJsonValueNumber>(0);
                else if(Pin->PinType.PinCategory==UEdGraphSchema_K2::PC_Struct && Pin->PinType.PinSubCategoryObject==TBaseStructure<FVector>::Get()) Kind=VectorJson(FVector::ZeroVector);
                else if(Pin->PinType.PinCategory==UEdGraphSchema_K2::PC_Struct && Pin->PinType.PinSubCategoryObject==TBaseStructure<FRotator>::Get()) Kind=RotatorJson(FRotator::ZeroRotator);
                if(Kind && DefaultType(Pin,Kind))
                {
                    const auto Value=FromNativeText(Kind,Pin->DefaultValue,true);
                    if(Pin->DefaultObject || !Pin->DefaultTextValue.IsEmpty() || !NodeForgeAssetDefinitions::ScalarValue(Value) || !MatchesNativeText(Value,Pin->DefaultValue,true))
                        return Fail(Error,TEXT("native pin default cannot be represented: ")+Ids[Node]+TEXT(".")+Pin->PinName.ToString());
                    Defaults->SetField(Pin->PinName.ToString(),Value);
                }
            }
            if(Pin->Direction==EGPD_Output) for(UEdGraphPin* To:Pin->LinkedTo)
            {
                if(!To || To->Direction!=EGPD_Input || !Ids.Contains(To->GetOwningNode()) || !To->LinkedTo.Contains(Pin))
                    return Fail(Error,TEXT("native connection leaves graph or is not reciprocal"));
                auto E=MakeShared<FJsonObject>();E->SetStringField(TEXT("from"),Ids[Node]);E->SetStringField(TEXT("output"),Pin->PinName.ToString());
                E->SetStringField(TEXT("to"),Ids[To->GetOwningNode()]);E->SetStringField(TEXT("input"),To->PinName.ToString());Edges.Add(MakeShared<FJsonValueObject>(E));
                Description.Edges.Add({Ids[Node],Pin->PinName.ToString(),Ids[To->GetOwningNode()],To->PinName.ToString()});
            }
        }
        if(!Defaults->Values.IsEmpty())N->SetObjectField(TEXT("defaults"),Defaults);
        NativeNode.DefaultsPresent=!Defaults->Values.IsEmpty();NativeNode.Defaults=Defaults->Values;Description.Nodes.Add(MoveTemp(NativeNode));
        Nodes.Add(MakeShared<FJsonValueObject>(N));auto M=MakeShared<FJsonObject>();M->SetStringField(TEXT("id"),Ids[Node]);
        M->SetStringField(TEXT("object"),Node->GetPathName());M->SetStringField(TEXT("sourceComment"),Node->NodeComment);Mapping.Add(MakeShared<FJsonValueObject>(M));
    }
    Document.Asset.Set<FBlueprintDefinition>(MoveTemp(Asset));Document.Graphs.Add(MoveTemp(Description));
    FGraphDocument Validated;
    if(!NodeForgeGraphCodec::Decode(NodeForgeGraphCodec::Encode(Document,3),Validated,Error) || !Shape(Document,Error))return false;
    UBlueprint* Expected=BuildChecked(GetTransientPackage(),MakeUniqueObjectName(GetTransientPackage(),UBlueprint::StaticClass(),TEXT("NodeForgeExportProbe")).ToString(),Document,Error);
    if(!Expected)return false;
    Expected->ClearFlags(RF_Public|RF_Standalone);Expected->SetFlags(RF_Transient);
    ON_SCOPE_EXIT {Expected->MarkAsGarbage();};
    if (DeclaredCanEverTick(Document))
    {
        FKismetEditorUtilities::CompileBlueprint(Expected);
        FObject CompiledEvidence;
        if ((Expected->Status != BS_UpToDate && Expected->Status != BS_UpToDateWithWarnings) || !Readback(Expected,Document,CompiledEvidence,Error)) return Fail(Error,TEXT("compiled Tick export expectation failed: ")+Error);
    }
    auto Editable=[&](const UObject* A,const UObject* B,const TSet<FName>& Skip)->bool
    {
        if(!A || !B || A->GetClass()!=B->GetClass())return Fail(Error,TEXT("native scaffold class differs"));
        for(TFieldIterator<FProperty> It(A->GetClass());It;++It)
            if(It->HasAnyPropertyFlags(CPF_Edit) && !It->HasAnyPropertyFlags(CPF_Transient) && !Skip.Contains(It->GetFName()) && !ExactProperty(*It,A,B))
                return Fail(Error,TEXT("unexpressible editable property: ")+A->GetPathName()+TEXT(".")+It->GetName());
        return true;
    };
    if(!Editable(BP,Expected,{TEXT("NewVariables")}))return false;
    if(BP->SimpleConstructionScript->GetAllNodes().Num()!=Expected->SimpleConstructionScript->GetAllNodes().Num()
        || BP->SimpleConstructionScript->GetRootNodes().Num()!=Expected->SimpleConstructionScript->GetRootNodes().Num())
        return Fail(Error,TEXT("nondefault components are not expressible"));
    const USCS_Node* Root=BP->SimpleConstructionScript->GetDefaultSceneRootNode();
    const USCS_Node* WantedRoot=Expected->SimpleConstructionScript->GetDefaultSceneRootNode();
    if(!Root || !WantedRoot || Root->ComponentClass!=WantedRoot->ComponentClass || !Root->ChildNodes.IsEmpty()
        || Root->AttachToName!=WantedRoot->AttachToName || Root->ParentComponentOrVariableName!=WantedRoot->ParentComponentOrVariableName
        || Root->ParentComponentOwnerClassName!=WantedRoot->ParentComponentOwnerClassName || Root->bIsParentComponentNative!=WantedRoot->bIsParentComponentNative
        || Root->GetVariableName()!=WantedRoot->GetVariableName() || !Editable(Root,WantedRoot,{})
        || !Editable(Root->ComponentTemplate,WantedRoot->ComponentTemplate,{}))return Fail(Error,TEXT("nondefault native root component"));
    for(const USCS_Node* S:BP->SimpleConstructionScript->GetAllNodes())if(S!=Root)return Fail(Error,TEXT("custom SCS component unsupported"));
    const auto* WantedConstruction=FBlueprintEditorUtils::FindUserConstructionScript(Expected);
    if(Construction->Nodes.Num()!=WantedConstruction->Nodes.Num())return Fail(Error,TEXT("nonempty construction script unsupported"));
    for(int32 I=0;I<Construction->Nodes.Num();++I)
    {
        UEdGraphNode* A=Construction->Nodes[I];UEdGraphNode* B=WantedConstruction->Nodes[I];
        if(!A || A->GetClass()!=B->GetClass() || A->Pins.Num()!=B->Pins.Num() || A->GetDesiredEnabledState()!=B->GetDesiredEnabledState())return Fail(Error,TEXT("construction scaffold differs"));
        auto* Entry=Cast<UK2Node_FunctionEntry>(A);auto* WantedEntry=Cast<UK2Node_FunctionEntry>(B);
        if(!Entry || !WantedEntry || !Entry->LocalVariables.IsEmpty() || Entry->GetExtraFlags()!=WantedEntry->GetExtraFlags()
            || Entry->CustomGeneratedFunctionName!=WantedEntry->CustomGeneratedFunctionName || Entry->bEnforceConstCorrectness!=WantedEntry->bEnforceConstCorrectness
            || Entry->FunctionReference.GetMemberName()!=WantedEntry->FunctionReference.GetMemberName()
            || Entry->FunctionReference.GetMemberParentClass()!=WantedEntry->FunctionReference.GetMemberParentClass()
            || Entry->FunctionReference.IsSelfContext()!=WantedEntry->FunctionReference.IsSelfContext())return Fail(Error,TEXT("nondefault construction entry binding/flags/locals"));
        if(FProperty* Metadata=Entry->GetClass()->FindPropertyByName(TEXT("MetaData")))if(!Metadata->Identical_InContainer(Entry,WantedEntry))return Fail(Error,TEXT("custom construction metadata unsupported"));
        for(auto* P:A->Pins)
        {
            auto* Q=P?ExactPin(B,P->PinName.ToString(),P->Direction):nullptr;
            if(!P || !Q || !P->LinkedTo.IsEmpty() || P->PinType!=Q->PinType || !ExactText(P->DefaultValue,Q->DefaultValue) || P->DefaultObject!=Q->DefaultObject
                || !P->DefaultTextValue.EqualTo(Q->DefaultTextValue) || P->bOrphanedPin || P->ParentPin || !P->SubPins.IsEmpty())return Fail(Error,TEXT("construction graph contains behavior or custom pins"));
        }
        if(!Editable(A,B,{}))return false;
    }
    for(const FBPVariableDescription& V:BP->NewVariables)
    {
        const auto* W=Expected->NewVariables.FindByPredicate([&](const FBPVariableDescription& T){return T.VarName==V.VarName;});
        if(!W || V.VarType!=W->VarType || V.PropertyFlags!=W->PropertyFlags || V.RepNotifyFunc!=W->RepNotifyFunc
            || V.ReplicationCondition!=W->ReplicationCondition || !ExactText(V.FriendlyName,W->FriendlyName) || !V.Category.EqualTo(W->Category)
            || V.MetaDataArray.Num()!=W->MetaDataArray.Num())return Fail(Error,TEXT("variable flags/metadata/type require an adapter: ")+V.VarName.ToString());
        for(int32 I=0;I<V.MetaDataArray.Num();++I)if(V.MetaDataArray[I].DataKey!=W->MetaDataArray[I].DataKey || !ExactText(V.MetaDataArray[I].DataValue,W->MetaDataArray[I].DataValue))
            return Fail(Error,TEXT("custom variable metadata unsupported"));
    }
    // Inherited Actor editable defaults cannot silently disappear into native factory values.
    const UObject* CDO=BP->GeneratedClass->GetDefaultObject();const UObject* NativeCDO=AActor::StaticClass()->GetDefaultObject();
    const UObject* CompiledExpectedCDO=Expected->GeneratedClass->GetDefaultObject();
    for(TFieldIterator<FProperty> It(AActor::StaticClass());It;++It)
        if(It->HasAnyPropertyFlags(CPF_Edit) && !It->HasAnyPropertyFlags(CPF_Transient) && !ExactProperty(*It,CDO,DeclaredCanEverTick(Document) && It->GetFName()==TEXT("PrimaryActorTick") ? CompiledExpectedCDO : NativeCDO))
            return Fail(Error,TEXT("nondefault inherited Actor setting: ")+It->GetName());
    auto* WantedGraph=FBlueprintEditorUtils::FindEventGraph(Expected);TMap<FString,UEdGraphNode*> WantedNodes;
    for(UEdGraphNode* N:WantedGraph->Nodes)WantedNodes.Add(N->NodeComment.Mid(IdPrefix.Len()),N);
    int32 OwnSelfRebindings=0;
    for(auto* A:Actual)
    {
        auto* B=WantedNodes.FindRef(Ids[A]);
        if(!B || !Cast<UK2Node>(A) || !Cast<UK2Node>(B) || A->GetClass()!=B->GetClass() || A->Pins.Num()!=B->Pins.Num() || Cast<UK2Node>(A)->IsNodePure()!=Cast<UK2Node>(B)->IsNodePure()
            || A->GetDesiredEnabledState()!=B->GetDesiredEnabledState() || !Editable(A,B,{}))return Fail(Error,TEXT("native node configuration differs: ")+Ids[A]);
        for(auto* P:A->Pins)
        {
            auto* Q=ExactPin(B,P->PinName.ToString(),P->Direction);
            FEdGraphPinType Compared=P->PinType;
            // Native self-variable pins carry their owning Blueprint generated/skeleton
            // class, which necessarily differs in a new asset. Normalize only that exact
            // implicit-self binding, never an external target or arbitrary object type.
            const auto* AV=Cast<UK2Node_Variable>(A);const auto* BV=Cast<UK2Node_Variable>(B);
            if(Q && P->PinName==UEdGraphSchema_K2::PN_Self && P->Direction==EGPD_Input && P->bHidden && Q->bHidden
                && AV && BV && AV->VariableReference.IsSelfContext() && BV->VariableReference.IsSelfContext()
                && P->LinkedTo.IsEmpty() && Q->LinkedTo.IsEmpty() && !P->DefaultObject && !Q->DefaultObject
                && P->PinType.PinCategory==UEdGraphSchema_K2::PC_Object && Q->PinType.PinCategory==UEdGraphSchema_K2::PC_Object
                && (P->PinType.PinSubCategoryObject.Get()==BP->GeneratedClass.Get() || P->PinType.PinSubCategoryObject.Get()==BP->SkeletonGeneratedClass.Get())
                && (Q->PinType.PinSubCategoryObject.Get()==Expected->GeneratedClass.Get() || Q->PinType.PinSubCategoryObject.Get()==Expected->SkeletonGeneratedClass.Get()))
            {Compared.PinSubCategoryObject=Q->PinType.PinSubCategoryObject;++OwnSelfRebindings;}
            if(!Q || Compared!=Q->PinType || P->bHidden!=Q->bHidden || P->bDefaultValueIsIgnored!=Q->bDefaultValueIsIgnored
                || P->bDefaultValueIsReadOnly!=Q->bDefaultValueIsReadOnly || P->DefaultObject!=Q->DefaultObject
                || !P->DefaultTextValue.EqualTo(Q->DefaultTextValue) || P->LinkedTo.Num()!=Q->LinkedTo.Num())return Fail(Error,TEXT("native pin signature/object/text/link count differs: ")+Ids[A]+TEXT(".")+P->PinName.ToString());
            auto Defaults=FieldObject(Object(Nodes[Actual.IndexOfByKey(A)]),TEXT("defaults"));
            if(Defaults && Defaults->HasField(P->PinName.ToString()))
            {if(!MatchesNativeText(Defaults->Values[P->PinName.ToString()],P->DefaultValue,true))return Fail(Error,TEXT("exported typed default differs"));}
            else if(!ExactText(P->DefaultValue,Q->DefaultValue))return Fail(Error,TEXT("unsupported or dormant native default differs: ")+Ids[A]+TEXT(".")+P->PinName.ToString());
        }
    }
    FObject ActualReadback;
    if(!Readback(BP,Document,ActualReadback,Error,&Ids))return false;
    if(BP->GetPackage()->IsDirty() || NodeForgePrototype::FileSha256(File)!=Before)return Fail(Error,TEXT("source dirty state or bytes changed"));
    Evidence=MakeShared<FJsonObject>();Evidence->SetStringField(TEXT("source"),Source);Evidence->SetStringField(TEXT("sourceAssetSha256"),Before);
    Evidence->SetBoolField(TEXT("sourceBytesUnchanged"),true);Evidence->SetBoolField(TEXT("sourceRemainsClean"),true);Evidence->SetArrayField(TEXT("logicalIdMapping"),Mapping);
    Evidence->SetObjectField(TEXT("sourceReadback"),ActualReadback);Evidence->SetNumberField(TEXT("nodeCount"),Nodes.Num());Evidence->SetNumberField(TEXT("edgeCount"),Edges.Num());
    Evidence->SetNumberField(TEXT("implicitOwnVariableSelfRebindings"),OwnSelfRebindings);
    Evidence->SetStringField(TEXT("metadataPolicy"),TEXT("source node comments recorded only; new nodes get logical-ID comments and fresh identities; unknown graph structures, pin configurations, variable flags, components and inherited defaults rejected"));
    Evidence->SetStringField(TEXT("verification"),TEXT("native reconstruction, exact source pins/links/settings and typed compiled CDO defaults; source not compiled or saved; new copy compile/persistence/runtime separate"));
    Request=NodeForgeGraphCodec::Encode(Document,FormatVersion);return true;
}

TSharedRef<FJsonObject> Catalog()
{
    check(IsInGameThread());
    auto Result = MakeShared<FJsonObject>();
    bool ExistingSignaturesValid = true;
    Result->SetStringField(TEXT("assetType"), TEXT("Blueprint"));
    Result->SetStringField(TEXT("parentClass"), AActor::StaticClass()->GetPathName());
    Result->SetStringField(TEXT("discovery"), TEXT("Native BlueprintActionDatabase plus exact UFunction reflection; not a complete node library"));
    Result->SetStringField(TEXT("verification"), TEXT("Instance signatures collected; compile/runtime evidence is maintained in test reports, not inferred from discovery"));
    FValues MemberTypes;
    for (const auto& Definition : NodeForgeNodeDefinitions::Members()) MemberTypes.Add(MakeShared<FJsonValueString>(Definition.Type));
    Result->SetArrayField(TEXT("variableTypes"), MemberTypes);
    Result->SetObjectField(TEXT("memberAttributesSchema"),NodeForgeAssetDefinitions::FieldsSchema(NodeForgeAssetDefinitions::EFields::MemberAttributes));
    Result->SetStringField(TEXT("rotatorDefaultRules"), TEXT("rotator member or unconnected native Rotator pin: exact object {pitch:number,yaw:number,roll:number}, degrees, finite components each abs<=1e12; no arrays/strings/additional keys. Native member struct and K2 Pitch,Yaw,Roll CSV are generated by the adapter; stored components are read back without angle normalization. Earlier catalogs reject new members/defaults."));
    Result->SetStringField(TEXT("memberDefaultRules"), TEXT("int: integral JSON number in [-2147483648,2147483647], exact compiled int32 readback. name: nonempty string shorter than NAME_SIZE in native FName display spelling; use None for the empty identity; no silent default normalization. No int/name members under earlier catalogs."));
    FValues Entries;
    auto Add = [&](const FString& Class, FObject Props, const FString& VariableTypeName, bool DatabaseMatch, const FString& Family = FString(), bool RequiresMathCatalog = false, bool RequiresStringTransformsCatalog = false)
    {
        FGraphDocument Request;
        FBlueprintDefinition Asset;Asset.ParentClass=AActor::StaticClass()->GetPathName();
        if(!VariableTypeName.IsEmpty())
        {
            FMemberDescription V;V.Name=TEXT("Value");NodeForgeAssetDefinitions::TypeFromName(VariableTypeName,V.Type.Kind);
            if(V.Type.Kind==EValueKind::Boolean)V.Default=MakeShared<FJsonValueBoolean>(false);
            else if(V.Type.Kind==EValueKind::Int32 || V.Type.Kind==EValueKind::Real)V.Default=MakeShared<FJsonValueNumber>(0);
            else if(V.Type.Kind==EValueKind::Vector)V.Default=VectorJson(FVector::ZeroVector);
            else if(V.Type.Kind==EValueKind::Rotator)V.Default=RotatorJson(FRotator::ZeroRotator);
            else V.Default=MakeShared<FJsonValueString>(V.Type.Kind==EValueKind::Name?TEXT("None"):TEXT(""));
            Asset.Variables.Add(MoveTemp(V));
        }
        Request.Asset.Set<FBlueprintDefinition>(MoveTemp(Asset));
        FGraphDefinition Graph;Graph.Id=TEXT("EventGraph");Graph.Kind=TEXT("EventGraph");
        FNodeDescription Node;Node.Id=TEXT("Sample");Node.Class=Class;
        if(ExactText(Class,UK2Node_Event::StaticClass()->GetPathName()) || ExactText(Class,UK2Node_CallFunction::StaticClass()->GetPathName()))
        {Node.Binding.Kind=ExactText(Class,UK2Node_Event::StaticClass()->GetPathName())?EBindingKind::Event:EBindingKind::Function;Node.Binding.Owner=Props->GetStringField(TEXT("owner"));Node.Binding.Member=Props->GetStringField(TEXT("member"));}
        else if(ExactText(Class,UK2Node_VariableGet::StaticClass()->GetPathName()) || ExactText(Class,UK2Node_VariableSet::StaticClass()->GetPathName()))
        {Node.Binding.Kind=EBindingKind::Member;Node.Binding.Name=Props->GetStringField(TEXT("variable"));}
        else Node.Properties=Props->Values;
        const bool NeedsRoot=Node.Binding.Kind==EBindingKind::Function && ExactText(Node.Binding.Owner,USceneComponent::StaticClass()->GetPathName());
        Graph.Nodes.Add(MoveTemp(Node));
        if(NeedsRoot)
        {
            FNodeDescription Root;Root.Id=TEXT("Root");Root.Class=UK2Node_CallFunction::StaticClass()->GetPathName();Root.Binding.Kind=EBindingKind::Function;
            Root.Binding.Owner=AActor::StaticClass()->GetPathName();Root.Binding.Member=TEXT("K2_GetRootComponent");Graph.Nodes.Add(MoveTemp(Root));
            Graph.Edges.Add({TEXT("Root"),UEdGraphSchema_K2::PN_ReturnValue.ToString(),TEXT("Sample"),UEdGraphSchema_K2::PN_Self.ToString()});
        }
        Request.Graphs.Add(MoveTemp(Graph));
        FString Error;
        UBlueprint* Candidate=Shape(Request,Error,ENodeContext::NativeCatalogProbe)
            ?BuildChecked(GetTransientPackage(),MakeUniqueObjectName(GetTransientPackage(),UBlueprint::StaticClass(),TEXT("NodeForgeCatalogProbe")).ToString(),Request,Error,ENodeContext::NativeCatalogProbe):nullptr;
        UEdGraphNode* Sample = nullptr;
        if (Candidate)
        {
            UEdGraph* NativeGraph = FBlueprintEditorUtils::FindEventGraph(Candidate);
            if (NativeGraph && NativeGraph->Nodes.Num() == Request.Graphs[0].Nodes.Num())
                for (UEdGraphNode* NativeNode : NativeGraph->Nodes)
                    if (NativeNode && NativeNode->NodeComment == IdPrefix + TEXT("Sample")) Sample = NativeNode;
            if (!Sample) Error = TEXT("Native catalog probe did not preserve the exact Sample node");
        }
        const FFunctionSpec* FunctionDefinition = ExactText(Class, UK2Node_CallFunction::StaticClass()->GetPathName())
            ? NodeForgeNodeDefinitions::Function(Props->GetStringField(TEXT("owner")), Props->GetStringField(TEXT("member"))) : nullptr;
        if (Sample && FunctionDefinition && FunctionDefinition->IsBulk)
        {
            auto* Call = Cast<UK2Node_CallFunction>(Sample);
            UFunction* Function = Call ? Call->GetTargetFunction() : nullptr;
            const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
            TSet<FName> Parameters;
            if (!Function || Function != FunctionDefinition->Owner->FindFunctionByName(FName(*FunctionDefinition->Member)))
                Error = TEXT("native exact function binding differs");
            else for (TFieldIterator<FProperty> It(Function, EFieldIteratorFlags::ExcludeSuper); It; ++It)
            {
                FProperty* Property = *It; if (!Property->HasAnyPropertyFlags(CPF_Parm)) continue;
                const bool Output = Property->HasAnyPropertyFlags(CPF_ReturnParm)
                    || (Property->HasAnyPropertyFlags(CPF_OutParm) && !Property->HasAnyPropertyFlags(CPF_ConstParm));
                const FName Name = Property->HasAnyPropertyFlags(CPF_ReturnParm) ? UEdGraphSchema_K2::PN_ReturnValue : Property->GetFName();
                FEdGraphPinType Type;
                UEdGraphPin* Pin = ExactPin(Sample, Name.ToString(), Output ? EGPD_Output : EGPD_Input);
                if (!Schema->ConvertPropertyToPinType(Property, Type) || !Pin || Pin->PinType != Type || Pin->bHidden
                    || Pin->bOrphanedPin || Pin->ParentPin || !Pin->SubPins.IsEmpty() || Pin->DefaultObject
                    || !Pin->DefaultTextValue.IsEmpty())
                { Error = TEXT("native parameter/pin type, direction or configuration differs: ") + Name.ToString(); break; }
                if (!Output && !Pin->bDefaultValueIsIgnored && !Pin->bDefaultValueIsReadOnly
                    && !Schema->IsPinDefaultValid(Pin, Pin->DefaultValue, nullptr, FText::GetEmpty()).IsEmpty())
                { Error = TEXT("invalid native input default: ") + Name.ToString(); break; }
                Parameters.Add(Name);
            }
            if (Error.IsEmpty()) for (UEdGraphPin* Pin : Sample->Pins)
                if (Pin && !Parameters.Contains(Pin->PinName)
                    && !(Pin->PinName == UEdGraphSchema_K2::PN_Self && Pin->bHidden && Pin->Direction == EGPD_Input))
                { Error = TEXT("unrepresented native extra pin: ") + Pin->PinName.ToString(); break; }
            if (!Error.IsEmpty()) Sample = nullptr;
        }
        if (FunctionDefinition)
            NodeForgeNodeDefinitions::QualifyFunction(FunctionDefinition->Owner->GetPathName(), FunctionDefinition->Member,
                Sample ? FString() : Error.IsEmpty() ? TEXT("native function signature unavailable") : Error);
        if (!Sample && (!FunctionDefinition || !FunctionDefinition->IsBulk)) ExistingSignaturesValid = false;
        FObject Entry = MakeShared<FJsonObject>(); Entry->SetStringField(TEXT("class"), Class); Entry->SetObjectField(TEXT("properties"), Props);
        Entry->SetBoolField(TEXT("discovered"), true); Entry->SetBoolField(TEXT("actionDatabaseMatch"), DatabaseMatch); Entry->SetBoolField(TEXT("constructible"), Sample != nullptr);
        Entry->SetStringField(TEXT("family"), Family.IsEmpty() ? TEXT("ExistingCore") : Family);
        if (Class==TEXT("/Script/BlueprintGraph.K2Node_Event"))
            if (const auto* Event=NodeForgeNodeDefinitions::Event(Props->GetStringField(TEXT("owner")),Props->GetStringField(TEXT("member"))))
                Entry->SetNumberField(TEXT("minimumBlueprintRevision"),Event->MinimumRevision);
        if (FunctionDefinition)
        {
            Entry->SetNumberField(TEXT("minimumBlueprintRevision"),FunctionDefinition->MinimumRevision);
            if (FunctionDefinition->IsBulk)
            {
                const UFunction* Native = FunctionDefinition->Owner->FindFunctionByName(FName(*FunctionDefinition->Member));
                Entry->SetStringField(TEXT("nativeDescription"),Native->GetMetaData(TEXT("ToolTip")));
                Entry->SetStringField(TEXT("nativeCategory"),Native->GetMetaData(TEXT("Category")));
            }
        }
        if (RequiresMathCatalog) Entry->SetBoolField(TEXT("requiresMathCatalog"), true);
        if (RequiresStringTransformsCatalog) Entry->SetBoolField(TEXT("requiresStringTransformsCatalog"), true);
        if (NodeForgeBlueprintSwitches::ClassFor(Class))
        {
            FObject Adapter = MakeShared<FJsonObject>();
            Adapter->SetStringField(TEXT("kind"), Props->HasField(TEXT("startIndex")) ? TEXT("switchInteger") : TEXT("switchName"));
            Adapter->SetNumberField(TEXT("minimumCaseCount"), 1); Adapter->SetNumberField(TEXT("maximumCaseCount"), 8);
            Adapter->SetStringField(TEXT("casePinNames"), Props->HasField(TEXT("startIndex"))
                ? TEXT("native decimal startIndex+i in case order; startIndex+caseCount<=2147483647; no int32 increment overflow")
                : TEXT("exact cases[i] display spelling; ASCII letter then letters/digits, length<=64, unique by native case-insensitive FName; None/Default/execute/Selection/comparison function names reserved"));
            Adapter->SetBoolField(TEXT("hasDefaultRequired"), true);
            Adapter->SetStringField(TEXT("signature"), TEXT("Native sample input/default signatures; case output names/count specialize from properties and are fully checked against each actual constructed instance. Hidden comparison context remains native and cannot be connected/configured."));
            Entry->SetObjectField(TEXT("caseAdapter"), Adapter);
        }
        Entry->SetStringField(TEXT("supportState"), Sample ? TEXT("experimental") : TEXT("discovered"));
        Entry->SetStringField(TEXT("verified"), TEXT("See regression evidence; discovery and instance construction do not imply compiled/runtime support"));
        Entry->SetStringField(TEXT("variableType"), VariableTypeName);
        if (Family == TEXT("SelfRootComponent"))
            Entry->SetStringField(TEXT("functionContext"), NeedsRoot
                ? TEXT("External-member SceneComponent call: self requires exactly one direct same-graph Actor.K2_GetRootComponent.ReturnValue; exact native SceneComponent object type; no object/string/text defaults or other sources")
                : TEXT("Implicit self Actor only; exact native SceneComponent ReturnValue may connect only to SelfRootComponent family SceneComponent self pins"));
        if (Sample)
        {
            FValues Pins = NodeEvidence(Sample)->GetArrayField(TEXT("pins"));
            for (const auto& PinValue : Pins)
            {
                FObject Pin = PinValue->AsObject();
                for (const TCHAR* Field : {TEXT("subtype"), TEXT("defaultObject")})
                    if (Pin->GetStringField(Field).Contains(TEXT("NodeForgeCatalogProbe")))
                        Pin->SetStringField(Field, TEXT("{selfBlueprint}"));
            }
            Entry->SetArrayField(TEXT("pins"), Pins);
        }
        else Entry->SetStringField(TEXT("error"), Error);
        if (Candidate) { Candidate->ClearFlags(RF_Public | RF_Standalone); Candidate->SetFlags(RF_Transient); Candidate->MarkAsGarbage(); }
        if (FunctionDefinition && FunctionDefinition->IsBulk && !Sample) return;
        Entries.Add(MakeShared<FJsonValueObject>(Entry));
    };
    bool BranchInDatabase = false, SequenceInDatabase = false;
    for (const auto& Pair : FBlueprintActionDatabase::Get().GetAllActions())
        for (UBlueprintNodeSpawner* Spawner : Pair.Value)
        {
            if (Spawner && Spawner->NodeClass == UK2Node_IfThenElse::StaticClass()) BranchInDatabase = true;
            if (Spawner && Spawner->NodeClass == UK2Node_ExecutionSequence::StaticClass()) SequenceInDatabase = true;
        }
    for (const auto& Definition : NodeForgeNodeDefinitions::Events())
    {
        FObject Properties = MakeShared<FJsonObject>();
        Properties->SetStringField(TEXT("owner"), Definition.Owner->GetPathName()); Properties->SetStringField(TEXT("member"), Definition.Member);
        bool InDatabase = false;
        const UFunction* Function = Definition.Owner->FindFunctionByName(FName(*Definition.Member));
        for (const auto& Pair : FBlueprintActionDatabase::Get().GetAllActions())
            for (const UBlueprintNodeSpawner* Spawner : Pair.Value)
                if (const auto* Event = Cast<UBlueprintEventNodeSpawner>(Spawner)) InDatabase |= Event->GetEventFunction() == Function;
        Add(UK2Node_Event::StaticClass()->GetPathName(), Properties, TEXT(""), InDatabase);
    }
    for (const FFunctionSpec& Spec : Functions())
    {
        UFunction* Function = ResolveFunction(Spec.Owner->GetPathName(), Spec.Member, ENodeContext::NativeCatalogProbe);
        if (!Function)
        {
            NodeForgeNodeDefinitions::QualifyFunction(Spec.Owner->GetPathName(), Spec.Member, TEXT("configured native function absent/ineligible"));
            if (!Spec.IsBulk) ExistingSignaturesValid = false;
            continue;
        }
        FObject Props = MakeShared<FJsonObject>(); Props->SetStringField(TEXT("owner"), Spec.Owner->GetPathName()); Props->SetStringField(TEXT("member"), Spec.Member);
        // Discovery must not create and discard a private function spawner just to query a match.
        const bool InDatabase = FindFunctionActionInDatabase(Function) != nullptr;
        Add(UK2Node_CallFunction::StaticClass()->GetPathName(), Props, TEXT(""), InDatabase, Spec.Family, Spec.MinimumRevision == 7, Spec.MinimumRevision == 9);
    }
    Add(UK2Node_IfThenElse::StaticClass()->GetPathName(), MakeShared<FJsonObject>(), TEXT(""), BranchInDatabase);
    for (int32 Count = MinSequenceOutputs; Count <= MaxSequenceOutputs; ++Count)
    {
        FObject Props = MakeShared<FJsonObject>(); Props->SetNumberField(TEXT("outputCount"), Count);
        Add(UK2Node_ExecutionSequence::StaticClass()->GetPathName(), Props, TEXT(""), SequenceInDatabase, TEXT("ExecutionSequence"));
    }
    for (const TCHAR* Class : {TEXT("/Script/BlueprintGraph.K2Node_SwitchInteger"), TEXT("/Script/BlueprintGraph.K2Node_SwitchName")})
        for (bool HasDefault : {false, true})
        {
            UClass* NativeClass = NodeForgeBlueprintSwitches::ClassFor(Class);
            bool InDatabase = false;
            for (const auto& Pair : FBlueprintActionDatabase::Get().GetAllActions())
                for (const UBlueprintNodeSpawner* Spawner : Pair.Value)
                    if (Spawner && Spawner->NodeClass == NativeClass) InDatabase = true;
            FObject Props = MakeShared<FJsonObject>(); Props->SetBoolField(TEXT("hasDefault"), HasDefault);
            if (NativeClass->GetName() == TEXT("K2Node_SwitchInteger"))
            { Props->SetNumberField(TEXT("startIndex"), 0); Props->SetNumberField(TEXT("caseCount"), 2); }
            else Props->SetArrayField(TEXT("cases"), {MakeShared<FJsonValueString>(TEXT("CaseA")), MakeShared<FJsonValueString>(TEXT("CaseB"))});
            Add(Class, Props, TEXT(""), InDatabase, TEXT("MultiwaySelection"));
        }
    for (const auto& MemberDefinition : NodeForgeNodeDefinitions::Members())
        for (UClass* Class : {UK2Node_VariableGet::StaticClass(), UK2Node_VariableSet::StaticClass()})
        { const FString& Type = MemberDefinition.Type; FObject Props = MakeShared<FJsonObject>(); Props->SetStringField(TEXT("variable"), TEXT("Value")); Add(Class->GetPathName(), Props, Type, false,
            FString(Type) == TEXT("rotator") ? TEXT("RotatorMembers") :
            FString(Type) == TEXT("int") || FString(Type) == TEXT("name") ? TEXT("MemberScalars") : TEXT("")); }
    Entries.Sort([](const auto& A, const auto& B)
    {
        const FObject X = A->AsObject(), Y = B->AsObject();
        const FObject XP = FieldObject(X, TEXT("properties")), YP = FieldObject(Y, TEXT("properties"));
        FString XM, YM, XO, YO; XP->TryGetStringField(TEXT("member"), XM); YP->TryGetStringField(TEXT("member"), YM);
        XP->TryGetStringField(TEXT("owner"), XO); YP->TryGetStringField(TEXT("owner"), YO);
        const FString XKey = X->GetStringField(TEXT("class")) + TEXT("|") + XO + TEXT("|") + XM + TEXT("|") + X->GetStringField(TEXT("variableType"));
        const FString YKey = Y->GetStringField(TEXT("class")) + TEXT("|") + YO + TEXT("|") + YM + TEXT("|") + Y->GetStringField(TEXT("variableType"));
        if (XKey != YKey) return XKey < YKey;
        double XCount = 0, YCount = 0;
        XP->TryGetNumberField(TEXT("outputCount"), XCount); YP->TryGetNumberField(TEXT("outputCount"), YCount);
        if (XCount != YCount) return XCount < YCount;
        bool XDefault = false, YDefault = false;
        XP->TryGetBoolField(TEXT("hasDefault"), XDefault); YP->TryGetBoolField(TEXT("hasDefault"), YDefault);
        return XDefault < YDefault;
    });
    Result->SetArrayField(TEXT("nodes"), Entries);
    Result->SetArrayField(TEXT("excludedCandidates"), NodeForgeNodeDefinitions::Exclusions(TEXT("Blueprint")));
    Result->SetBoolField(TEXT("nativeQualificationPassed"), ExistingSignaturesValid);
    Result->SetStringField(TEXT("definitionPolicy"), TEXT("Single shared definitions and exact archived capability profiles; ordinary pure functions in two native libraries require full instance/pin/default qualification before admission. All new entries remain experimental, runtime is separate."));
    Result->SetStringField(TEXT("variablePinRule"), TEXT("The sample Value pin is replaced by the exact declared member name; variableType controls actual pin type"));
    Result->SetStringField(TEXT("connections"), TEXT("Exact pin names and directions; MAKE only, no implicit conversions/replacements, cycles unsupported"));
    Result->SetStringField(TEXT("defaults"), TEXT("Unconnected visible input only; exact native bool/real/string/vector plus int32 integral range and name string shorter than NAME_SIZE. Rotator uses the exact bounded pitch/yaw/roll object from rotatorDefaultRules. No arbitrary object/container defaults."));
    Result->SetStringField(TEXT("supportStates"), TEXT("discovered: native identity found; experimental: native instance/signature available, generation allowed in supported context; verified: consult exact per-function graph/compile/reopen/runtime evidence, never inferred from catalog construction"));
    Result->SetStringField(TEXT("previousCatalogDigest"), NodeForgeMaterialGraph::StringIdentityCatalogDigest);
    Result->SetStringField(TEXT("stringTransformsRule"), TEXT("StringTransforms entries use native KismetStringLibrary Left/Right/ToUpper/ToLower via the same configured function spawner and exact pins. Native string values and int32 Count; native substring/case behavior, no custom text algorithm or graph recipe. Earlier archived digests reject these functions. Len/Contains/Mid already exist; their combination does not imply all string functions, locale-sensitive normalization, containers or wildcard support. Actual per-case compile/reopen/runtime proof is separate from discovery."));
    Result->SetStringField(TEXT("exactStringRule"), TEXT("Blueprint class/parent/owner/member/type, declared variable references, logical node IDs and ports require exact display spelling. String defaults and native name display text compare case-sensitively, including canonical None. Direct reflected FStrProperty values, FriendlyName/metadata values and unexpressed pin defaults cannot silently change case on export. Native FName runtime equality/collision, bool parsing and numeric tolerances stay unchanged; nested arbitrary containers are not admitted by this rule."));
    Result->SetStringField(TEXT("mathCatalogRule"), TEXT("Entries with requiresMathCatalog use the current math-capable catalog; all earlier archived digests keep their original subset. Abs/FMin/FMax/Lerp/DegSin/DegCos use native double pins; degree trig is explicitly degrees. RLerp uses native Rotator A/B, float Alpha and bool bShortestPath. Values and pin types are read from actual instances; no hidden cast, clamping or graph recipe."));
    Result->SetStringField(TEXT("switchRule"), TEXT("Current switch-capable catalog only: native SwitchInteger properties {startIndex:int32,caseCount:integer1..8,hasDefault:bool} with startIndex+caseCount<=2147483647, or SwitchName {cases:uniqueCaseIdentifiers1..8,hasDefault:bool}. Selection input uses native int/name. Exact native case outputs and optional Default; absent outputs never fall back. Unmatched selection without Default ends execution. Native name equality ignores case, graph port identity retains exact display spelling."));
    Result->SetStringField(TEXT("sequenceRule"), TEXT("ExecutionSequence properties.outputCount is a required integral JSON number in 2..8. Each count has its own native instance signature. Native execute and then_0..then_N pins remain in execution order; unconnected outputs are preserved, declared edges are never skipped. No manual pins, cycles or latent-node support implied; earlier catalogs reject Sequence."));
    Result->SetStringField(TEXT("instanceFunctionContext"), TEXT("Allowlisted native Actor functions target implicit self. SelfRootComponent family SceneComponent calls retain native external-member context and require exactly one direct same-graph Actor.K2_GetRootComponent.ReturnValue to self. No object defaults, variables, literals, casts, reroutes, arbitrary external objects or custom components."));
    return Result;
}
}
