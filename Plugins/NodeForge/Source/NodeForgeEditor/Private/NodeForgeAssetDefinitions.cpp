#include "NodeForgeAssetDefinitions.h"
#include "NodeForgeNodeDefinitions.h"
#include "NodeForgeResourceSource.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialExpression.h"
#include "Misc/DefaultValueHelper.h"
#include "UObject/NoExportTypes.h"
#include "UObject/UnrealType.h"

namespace NodeForgeAssetDefinitions
{
using FObject = TSharedPtr<FJsonObject>;
using FValues = TArray<FValue>;
static bool ExactText(const FString& A, const FString& B) { return A.Equals(B, ESearchCase::CaseSensitive); }
static bool Fail(FString& Error, const FString& Text) { Error = TEXT("AssetDefinitions: ") + Text; return false; }
const TArray<FFieldDefinition>& Fields(EFields Group)
{
    switch (Group)
    {
    case EFields::RootV2:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("format"), EFieldType::String, true},
            {TEXT("version"), EFieldType::Integer, true},
            {TEXT("requestId"), EFieldType::String, true},
            {TEXT("catalogDigest"), EFieldType::String, true},
            {TEXT("assetType"), EFieldType::String, true},
            {TEXT("assetPath"), EFieldType::String, true},
            {TEXT("settings"), EFieldType::Object, true},
            {TEXT("nodes"), EFieldType::Array, true},
            {TEXT("edges"), EFieldType::Array, true}
        };
        return Definition;
    }
    case EFields::RootV3:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("format"), EFieldType::String, true},
            {TEXT("version"), EFieldType::Integer, true},
            {TEXT("requestId"), EFieldType::String, true},
            {TEXT("catalogDigest"), EFieldType::String, true},
            {TEXT("target"), EFieldType::Object, true},
            {TEXT("asset"), EFieldType::Object, true},
            {TEXT("graphs"), EFieldType::Array, true}
        };
        return Definition;
    }
    case EFields::Target:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("assetPath"), EFieldType::String, true}
        };
        return Definition;
    }
    case EFields::MaterialAsset:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("type"), EFieldType::String, true},
            {TEXT("settings"), EFieldType::Object, true}
        };
        return Definition;
    }
    case EFields::BlueprintAsset:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("type"), EFieldType::String, true},
            {TEXT("parent"), EFieldType::Object, true},
            {TEXT("members"), EFieldType::Object, true}
        };
        return Definition;
    }
    case EFields::InstanceAsset:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("type"), EFieldType::String, true},
            {TEXT("parent"), EFieldType::Object, true},
            {TEXT("parameters"), EFieldType::Object, true}
        };
        return Definition;
    }
    case EFields::MaterialSettings:
    {
        static const TArray<FFieldDefinition> Definition = [] {
            TArray<FFieldDefinition> Result = {
                {TEXT("domain"), EFieldType::String, true, {TEXT("Surface")}},
                {TEXT("blend"), EFieldType::String, true, {TEXT("Opaque"),TEXT("Translucent"),TEXT("Masked")}},
                {TEXT("shading"), EFieldType::String, true, {TEXT("Unlit"),TEXT("DefaultLit")}},
                {TEXT("lighting"), EFieldType::String, false, {TEXT("SurfaceForwardShading")}}
            };
            for (const auto& Option : MaterialOptions()) Result.Add({Option.Name,Option.Type,Option.Required});
            return Result;
        }();
        return Definition;
    }
    case EFields::MemberAttributes:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("instanceEditable"), EFieldType::Boolean, true},
            {TEXT("category"), EFieldType::String, true},
            {TEXT("tooltip"), EFieldType::String, true}
        };
        return Definition;
    }
    case EFields::BlueprintSettingsV2:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("parentClass"), EFieldType::String, true},
            {TEXT("variables"), EFieldType::Array, true}
        };
        return Definition;
    }
    case EFields::BlueprintParent:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("class"), EFieldType::String, true}
        };
        return Definition;
    }
    case EFields::Members:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("variables"), EFieldType::Array, true}
        };
        return Definition;
    }
    case EFields::VariableV2:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("name"), EFieldType::String, true},
            {TEXT("type"), EFieldType::String, true},
            {TEXT("default"), EFieldType::Value, true},
            {TEXT("attributes"), EFieldType::Object, false}
        };
        return Definition;
    }
    case EFields::VariableV3:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("name"), EFieldType::String, true},
            {TEXT("type"), EFieldType::Object, true},
            {TEXT("default"), EFieldType::Value, true},
            {TEXT("attributes"), EFieldType::Object, false}
        };
        return Definition;
    }
    case EFields::ValueType:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("kind"), EFieldType::String, true},
            {TEXT("name"), EFieldType::String, true}
        };
        return Definition;
    }
    case EFields::InstanceSettingsV2:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("parent"), EFieldType::String, true},
            {TEXT("parentAssetSha256"), EFieldType::String, true},
            {TEXT("scalarParameters"), EFieldType::Object, true},
            {TEXT("vectorParameters"), EFieldType::Object, true},
            {TEXT("textureParameters"), EFieldType::Object, true}
        };
        return Definition;
    }
    case EFields::InstanceParent:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("path"), EFieldType::String, true},
            {TEXT("assetSha256"), EFieldType::String, true}
        };
        return Definition;
    }
    case EFields::Parameters:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("scalar"), EFieldType::Object, true},
            {TEXT("vector"), EFieldType::Object, true},
            {TEXT("texture"), EFieldType::Object, true}
        };
        return Definition;
    }
    case EFields::Graph:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("id"), EFieldType::String, true},
            {TEXT("kind"), EFieldType::String, true},
            {TEXT("nodes"), EFieldType::Array, true},
            {TEXT("edges"), EFieldType::Array, true}
        };
        return Definition;
    }
    case EFields::NodeV2:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("id"), EFieldType::String, true},
            {TEXT("class"), EFieldType::String, true},
            {TEXT("properties"), EFieldType::Object, true},
            {TEXT("defaults"), EFieldType::Object, false},
            {TEXT("position"), EFieldType::Array, false}
        };
        return Definition;
    }
    case EFields::NodeV3:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("id"), EFieldType::String, true},
            {TEXT("class"), EFieldType::String, true},
            {TEXT("properties"), EFieldType::Object, true},
            {TEXT("binding"), EFieldType::Object, false},
            {TEXT("defaults"), EFieldType::Object, false},
            {TEXT("position"), EFieldType::Array, false}
        };
        return Definition;
    }
    case EFields::Edge:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("from"), EFieldType::String, true},
            {TEXT("output"), EFieldType::String, true},
            {TEXT("to"), EFieldType::String, true},
            {TEXT("input"), EFieldType::String, true}
        };
        return Definition;
    }
    case EFields::FunctionBinding:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("kind"), EFieldType::String, true},
            {TEXT("owner"), EFieldType::String, true},
            {TEXT("member"), EFieldType::String, true}
        };
        return Definition;
    }
    case EFields::MemberBinding:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("kind"), EFieldType::String, true},
            {TEXT("name"), EFieldType::String, true}
        };
        return Definition;
    }
    case EFields::MaterialFunctionBinding:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("kind"), EFieldType::String, true},
            {TEXT("path"), EFieldType::String, true},
            {TEXT("assetSha256"), EFieldType::String, true}
        };
        return Definition;
    }
    case EFields::Rotator:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("pitch"), EFieldType::Number, true},
            {TEXT("yaw"), EFieldType::Number, true},
            {TEXT("roll"), EFieldType::Number, true}
        };
        return Definition;
    }
    case EFields::Sequence:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("outputCount"), EFieldType::Integer, true}
        };
        return Definition;
    }
    case EFields::SwitchInteger:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("startIndex"), EFieldType::Integer, true},
            {TEXT("caseCount"), EFieldType::Integer, true},
            {TEXT("hasDefault"), EFieldType::Boolean, true}
        };
        return Definition;
    }
    case EFields::SwitchName:
    {
        static const TArray<FFieldDefinition> Definition = {
            {TEXT("cases"), EFieldType::Array, true},
            {TEXT("hasDefault"), EFieldType::Boolean, true}
        };
        return Definition;
    }
    case EFields::FunctionPropertiesV2:
    { static const TArray<FFieldDefinition> D = {{TEXT("owner"),EFieldType::String,true},{TEXT("member"),EFieldType::String,true}}; return D; }
    case EFields::MemberPropertiesV2:
    { static const TArray<FFieldDefinition> D = {{TEXT("variable"),EFieldType::String,true}}; return D; }
    case EFields::MaterialFunctionPropertiesV2:
    { static const TArray<FFieldDefinition> D = {{TEXT("MaterialFunction"),EFieldType::String,true},{TEXT("FunctionAssetSha256"),EFieldType::String,true}}; return D; }
    case EFields::ExportConfig:
    { static const TArray<FFieldDefinition> D = {{TEXT("source"),EFieldType::String,true},{TEXT("requestId"),EFieldType::String,true},{TEXT("assetPath"),EFieldType::String,true},{TEXT("formatVersion"),EFieldType::Integer,false}}; return D; }
    }
    checkNoEntry();
    static const TArray<FFieldDefinition> Empty;
    return Empty;
}
bool FieldsValid(const FValueMap& Input, EFields Group, FString& Error)
{
    const auto& Definitions = Fields(Group);
    for (const auto& Definition : Definitions)
    {
        const FValue* Item = Input.Find(Definition.Name);
        if (!Item) { if (Definition.Required) return Fail(Error, TEXT("missing field: ") + Definition.Name); continue; }
        bool Valid = Item->IsValid();
        if (Valid) switch (Definition.Type)
        {
        case EFieldType::String: Valid = (*Item)->Type == EJson::String; break;
        case EFieldType::Integer: { double Numeric = 0; Valid = Number(*Item, Numeric, 1.e12) && Numeric == FMath::TruncToDouble(Numeric); break; }
        case EFieldType::Number: Valid = (*Item)->Type == EJson::Number; break;
        case EFieldType::Boolean: Valid = (*Item)->Type == EJson::Boolean; break;
        case EFieldType::Object: Valid = (*Item)->Type == EJson::Object; break;
        case EFieldType::Array: Valid = (*Item)->Type == EJson::Array; break;
        case EFieldType::Value: break;
        }
        if (Valid && !Definition.EnumValues.IsEmpty())
            Valid = (*Item)->Type==EJson::String && Definition.EnumValues.ContainsByPredicate([&](const FString& Candidate){return ExactText(Candidate,(*Item)->AsString());});
        if (!Valid) return Fail(Error, TEXT("wrong type or enum value: ") + Definition.Name);
    }
    for (const auto& Item : Input)
        if (!Definitions.ContainsByPredicate([&](const auto& D) { return ExactText(Item.Key, D.Name); })) return Fail(Error, TEXT("unknown field: ") + Item.Key);
    return true;
}
bool NodePropertiesValid(const FString& Class, const FValueMap& Properties, FString& Error)
{
    if (ExactText(Class, TEXT("/Script/BlueprintGraph.K2Node_ExecutionSequence")))
        return FieldsValid(Properties, EFields::Sequence, Error) && Properties[TEXT("outputCount")]->AsNumber() >= MinimumSequenceOutputs && Properties[TEXT("outputCount")]->AsNumber() <= MaximumSequenceOutputs;
    if (ExactText(Class, TEXT("/Script/BlueprintGraph.K2Node_SwitchInteger")))
    {
        if (!FieldsValid(Properties, EFields::SwitchInteger, Error)) return false;
        const auto Start = Properties[TEXT("startIndex")]; const double Count = Properties[TEXT("caseCount")]->AsNumber();
        return Value(EValueKind::Int32, Start) && Count >= 1 && Count <= MaximumSwitchCases && Start->AsNumber() + Count <= MAX_int32;
    }
    if (ExactText(Class, TEXT("/Script/BlueprintGraph.K2Node_SwitchName")))
    {
        if (!FieldsValid(Properties, EFields::SwitchName, Error)) return false;
        const auto& Cases = Properties[TEXT("cases")]->AsArray();
        if (Cases.Num() < 1 || Cases.Num() > MaximumSwitchCases) return false;
        TSet<FName> Names;
        for (const auto& Case : Cases)
        {
            if (!Case || Case->Type != EJson::String || !Identifier(Case->AsString(), true)) return false;
            const FString& Text = Case->AsString(); const FName Name(*Text);
            if (Name.IsNone() || Names.Contains(Name) || Name == TEXT("Default") || Name == TEXT("execute") || Name == TEXT("Selection")
                || Name == TEXT("NotEqual_NameName") || Name == TEXT("NotEqual_IntInt") || !ExactText(Text, Name.ToString())) return false;
            Names.Add(Name);
        }
        return true;
    }
    // Blueprint binding-only and Branch nodes have no ordinary configuration.
    return Properties.IsEmpty();
}
TSharedRef<FJsonObject> FieldsSchema(EFields Group)
{
    auto Result=MakeShared<FJsonObject>(),Properties=MakeShared<FJsonObject>();TArray<FValue> Required;
    const TCHAR* Names[]={TEXT("string"),TEXT("integer"),TEXT("number"),TEXT("boolean"),TEXT("object"),TEXT("array")};
    for(const auto& Field:Fields(Group))
    {
        auto Type=MakeShared<FJsonObject>();
        if(Field.Type!=EFieldType::Value)Type->SetStringField(TEXT("type"),Names[static_cast<int32>(Field.Type)]);
        if(!Field.EnumValues.IsEmpty())
        {TArray<FValue> Values;for(const auto& Name:Field.EnumValues)Values.Add(MakeShared<FJsonValueString>(Name));Type->SetArrayField(TEXT("enum"),Values);}
        if (Group == EFields::MemberAttributes && Field.Name == TEXT("category"))
        {
            Type->SetNumberField(TEXT("minLength"),1); Type->SetNumberField(TEXT("maxLength"),128);
            Type->SetStringField(TEXT("pattern"),TEXT("^(?!\\s)(?!.*\\s$)(?!.*[\\u0000-\\u001f\\u007f-\\u009f])[^|]+(?:\\|[^|]+)*$"));
        }
        if (Group == EFields::MemberAttributes && Field.Name == TEXT("tooltip"))
        {
            Type->SetNumberField(TEXT("maxLength"),1024); Type->SetStringField(TEXT("pattern"),TEXT("^[^\\u0000]*$"));
        }
        if (Group == EFields::MaterialSettings)
            for (const auto& Option : MaterialOptions()) if (Option.Name == Field.Name)
            {
                if (Option.Type == EFieldType::Number)
                { Type->SetNumberField(TEXT("minimum"),0); Type->SetNumberField(TEXT("maximum"),1); Type->SetNumberField(TEXT("default"),Option.Default); }
                else if (!Option.Required) Type->SetBoolField(TEXT("default"),Option.Default != 0);
            }
        Properties->SetObjectField(Field.Name,Type);if(Field.Required)Required.Add(MakeShared<FJsonValueString>(Field.Name));
    }
    Result->SetStringField(TEXT("type"),TEXT("object"));Result->SetObjectField(TEXT("properties"),Properties);
    Result->SetArrayField(TEXT("required"),Required);Result->SetBoolField(TEXT("additionalProperties"),false);return Result;
}
TSharedRef<FJsonObject> NodePropertiesSchema(const FString& Class)
{
    auto Result=MakeShared<FJsonObject>();Result->SetStringField(TEXT("type"),TEXT("object"));Result->SetBoolField(TEXT("additionalProperties"),false);
    if(ExactText(Class,TEXT("/Script/BlueprintGraph.K2Node_ExecutionSequence")))
    {
        Result=FieldsSchema(EFields::Sequence);auto Count=Result->GetObjectField(TEXT("properties"))->GetObjectField(TEXT("outputCount"));
        Count->SetNumberField(TEXT("minimum"),MinimumSequenceOutputs);Count->SetNumberField(TEXT("maximum"),MaximumSequenceOutputs);
    }
    else if(ExactText(Class,TEXT("/Script/BlueprintGraph.K2Node_SwitchInteger")))
    {
        Result=FieldsSchema(EFields::SwitchInteger);auto Properties=Result->GetObjectField(TEXT("properties"));
        Properties->SetObjectField(TEXT("startIndex"),ValueSchema(EValueKind::Int32));auto Count=Properties->GetObjectField(TEXT("caseCount"));Count->SetNumberField(TEXT("minimum"),1);Count->SetNumberField(TEXT("maximum"),MaximumSwitchCases);
        Result->SetStringField(TEXT("x-nativeConstraint"),TEXT("startIndex+caseCount<=MAX_int32"));
    }
    else if(ExactText(Class,TEXT("/Script/BlueprintGraph.K2Node_SwitchName")))
    {
        Result=FieldsSchema(EFields::SwitchName);auto Cases=Result->GetObjectField(TEXT("properties"))->GetObjectField(TEXT("cases"));
        Cases->SetNumberField(TEXT("minItems"),1);Cases->SetNumberField(TEXT("maxItems"),MaximumSwitchCases);Cases->SetBoolField(TEXT("uniqueItems"),true);
        auto Name=MakeShared<FJsonObject>();Name->SetStringField(TEXT("type"),TEXT("string"));Name->SetStringField(TEXT("pattern"),TEXT("^[A-Za-z][A-Za-z0-9]{0,63}$"));Cases->SetObjectField(TEXT("items"),Name);
        Result->SetStringField(TEXT("x-nativeConstraint"),TEXT("case-insensitive canonical FName uniqueness and reserved native pin names"));
    }
    return Result;
}
TSharedRef<FJsonObject> ValueSchema(EValueKind Kind, bool MemberContext)
{
    auto Result=MakeShared<FJsonObject>();
    auto NumberSchema=[&](double Limit)
    {auto N=MakeShared<FJsonObject>();N->SetStringField(TEXT("type"),TEXT("number"));N->SetNumberField(TEXT("minimum"),-Limit);N->SetNumberField(TEXT("maximum"),Limit);return N;};
    switch(Kind)
    {
    case EValueKind::Boolean:Result->SetStringField(TEXT("type"),TEXT("boolean"));break;
    case EValueKind::Int32:Result->SetStringField(TEXT("type"),TEXT("integer"));Result->SetNumberField(TEXT("minimum"),MIN_int32);Result->SetNumberField(TEXT("maximum"),MAX_int32);break;
    case EValueKind::Real:return NumberSchema(MemberNumberLimit);
    case EValueKind::String:Result->SetStringField(TEXT("type"),TEXT("string"));Result->SetNumberField(TEXT("maxLength"),MaxStringLength);break;
    case EValueKind::Name:
    {
        Result->SetStringField(TEXT("type"),TEXT("string"));Result->SetNumberField(TEXT("maxLength"),NAME_SIZE-1);
        if(MemberContext)
        {
            Result->SetNumberField(TEXT("minLength"),1);
            auto If=MakeShared<FJsonObject>(),Then=MakeShared<FJsonObject>();If->SetStringField(TEXT("pattern"),TEXT("^[Nn][Oo][Nn][Ee]$"));Then->SetStringField(TEXT("const"),TEXT("None"));
            Result->SetObjectField(TEXT("if"),If);Result->SetObjectField(TEXT("then"),Then);
            Result->SetStringField(TEXT("x-nativeConstraint"),TEXT("canonical FName display spelling checked against actual native storage"));
        }
        break;
    }
    case EValueKind::Vector:
    case EValueKind::LinearColor:
        Result->SetStringField(TEXT("type"),TEXT("array"));Result->SetNumberField(TEXT("minItems"),Kind==EValueKind::Vector?3:4);Result->SetNumberField(TEXT("maxItems"),Kind==EValueKind::Vector?3:4);
        Result->SetObjectField(TEXT("items"),NumberSchema(Kind==EValueKind::Vector?MemberNumberLimit:MaterialNumberLimit));break;
    case EValueKind::Rotator:
        Result=FieldsSchema(EFields::Rotator);
        for(const auto& Field:Fields(EFields::Rotator))Result->GetObjectField(TEXT("properties"))->SetObjectField(Field.Name,NumberSchema(MemberNumberLimit));break;
    }
    return Result;
}
const TArray<FMemberTypeDefinition>& MemberTypes()
{
    static const TArray<FMemberTypeDefinition> Types = {
        {TEXT("bool"), EValueKind::Boolean, 0}, {TEXT("float"), EValueKind::Real, 0},
        {TEXT("string"), EValueKind::String, 0}, {TEXT("vector"), EValueKind::Vector, 1},
        {TEXT("int"), EValueKind::Int32, 4}, {TEXT("name"), EValueKind::Name, 4},
        {TEXT("rotator"), EValueKind::Rotator, 6}
    };
    return Types;
}
const TArray<FAssetTypeDefinition>& AssetTypes()
{
    static const TArray<FAssetTypeDefinition> Types = {
        {TEXT("Material"),TEXT("MaterialGraph"),TEXT("Material"),1,1,256,1,1024,EFields::MaterialSettings,EFields::MaterialAsset},
        {TEXT("Blueprint"),TEXT("EventGraph"),TEXT("EventGraph"),1,1,128,0,256,EFields::BlueprintSettingsV2,EFields::BlueprintAsset},
        {TEXT("MaterialInstance"),TEXT(""),TEXT(""),0,0,0,0,0,EFields::InstanceSettingsV2,EFields::InstanceAsset}
    };
    return Types;
}
const FAssetTypeDefinition* AssetType(const FString& Type)
{
    return AssetTypes().FindByPredicate([&](const auto& D) { return ExactText(D.Type, Type); });
}
EBindingKind BindingKind(const FString& Class)
{
    if (const auto* D = NodeForgeNodeDefinitions::Special(Class)) return D->Binding;
    if (const auto* D = NodeForgeNodeDefinitions::Material(Class); D && D->RequiresFunctionBinding) return EBindingKind::MaterialFunction;
    return EBindingKind::None;
}
FString BindingName(EBindingKind Kind)
{
    switch (Kind)
    {
    case EBindingKind::Event: return TEXT("Event");
    case EBindingKind::Function: return TEXT("Function");
    case EBindingKind::Member: return TEXT("Member");
    case EBindingKind::MaterialFunction: return TEXT("MaterialFunction");
    default: return TEXT("None");
    }
}
FString TypeName(EValueKind Kind)
{
    for (const auto& Type : MemberTypes()) if (Type.Kind == Kind) return Type.Name;
    return TEXT("unsupported");
}
bool TypeFromName(const FString& Name, EValueKind& Kind)
{
    for (const auto& Type : MemberTypes()) if (ExactText(Name, Type.Name)) { Kind = Type.Kind; return true; }
    return false;
}
bool Identifier(const FString& Name, bool FirstLetter, int32 MaxLength)
{
    if (Name.IsEmpty() || Name.Len() > MaxLength) return false;
    if (FirstLetter && !((Name[0] >= 'a' && Name[0] <= 'z') || (Name[0] >= 'A' && Name[0] <= 'Z'))) return false;
    for (TCHAR Ch : Name) if (!((Ch >= 'a' && Ch <= 'z') || (Ch >= 'A' && Ch <= 'Z') || (Ch >= '0' && Ch <= '9'))) return false;
    return true;
}
bool Sha256(const FString& Text)
{
    if (Text.Len() != 64) return false;
    for (TCHAR Ch : Text) if (!((Ch >= '0' && Ch <= '9') || (Ch >= 'a' && Ch <= 'f'))) return false;
    return true;
}
bool Number(const FValue& Value, double& Out, double Magnitude)
{
    return Value && Value->Type == EJson::Number && Value->TryGetNumber(Out) && FMath::IsFinite(Out) && FMath::Abs(Out) <= Magnitude;
}
bool VectorValue(const TSharedPtr<FJsonValue>& Value)
{
    if (!Value || Value->Type != EJson::Array || Value->AsArray().Num() != 3) return false;
    for (const auto& Part : Value->AsArray())
        if (!Part || Part->Type != EJson::Number || !FMath::IsFinite(Part->AsNumber()) || FMath::Abs(Part->AsNumber()) > MemberNumberLimit) return false;
    return true;
}
FVector JsonVector(const TSharedPtr<FJsonValue>& Value)
{
    const auto& Parts = Value->AsArray();
    return FVector(Parts[0]->AsNumber(), Parts[1]->AsNumber(), Parts[2]->AsNumber());
}
TSharedPtr<FJsonValue> VectorJson(const FVector& Value)
{
    return MakeShared<FJsonValueArray>(FValues{MakeShared<FJsonValueNumber>(Value.X), MakeShared<FJsonValueNumber>(Value.Y), MakeShared<FJsonValueNumber>(Value.Z)});
}
bool RotatorValue(const TSharedPtr<FJsonValue>& Value)
{
    const FObject Rotation = Value && Value->Type == EJson::Object ? Value->AsObject() : nullptr;
    if (!Rotation || Rotation->Values.Num() != 3) return false;
    for (const TCHAR* Axis : {TEXT("pitch"), TEXT("yaw"), TEXT("roll")})
    {
        bool ExactAxis = false;
        for (const auto& Pair : Rotation->Values) if (ExactText(Pair.Key, Axis)) { ExactAxis = true; break; }
        double Number = 0;
        if (!ExactAxis || !Rotation->HasTypedField<EJson::Number>(Axis) || !Rotation->TryGetNumberField(Axis, Number)
            || !FMath::IsFinite(Number) || FMath::Abs(Number) > MemberNumberLimit) return false;
    }
    return true;
}
FRotator JsonRotator(const TSharedPtr<FJsonValue>& Value)
{
    const FObject Rotation = Value->AsObject();
    return FRotator(Rotation->GetNumberField(TEXT("pitch")), Rotation->GetNumberField(TEXT("yaw")), Rotation->GetNumberField(TEXT("roll")));
}
TSharedPtr<FJsonValue> RotatorJson(const FRotator& Value)
{
    FObject Rotation = MakeShared<FJsonObject>();
    Rotation->SetNumberField(TEXT("pitch"), Value.Pitch);
    Rotation->SetNumberField(TEXT("yaw"), Value.Yaw);
    Rotation->SetNumberField(TEXT("roll"), Value.Roll);
    return MakeShared<FJsonValueObject>(Rotation);
}
bool ScalarValue(const TSharedPtr<FJsonValue>& Value)
{
    if (!Value.IsValid()) return false;
    return Value->Type == EJson::Boolean || (Value->Type == EJson::String && Value->AsString().Len() <= MaxStringLength)
        || (Value->Type == EJson::Number && FMath::IsFinite(Value->AsNumber()) && FMath::Abs(Value->AsNumber()) <= MemberNumberLimit) || VectorValue(Value) || RotatorValue(Value);
}
FString NativeText(const TSharedPtr<FJsonValue>& Value, bool PinContext)
{
    if (RotatorValue(Value))
    {
        const FRotator Rotation = JsonRotator(Value);
        // Native K2 CSV is Pitch,Yaw,Roll; member struct text uses named fields.
        if (PinContext) return FString::Printf(TEXT("%.17g,%.17g,%.17g"), Rotation.Pitch, Rotation.Yaw, Rotation.Roll);
        FString Text;
        TBaseStructure<FRotator>::Get()->ExportText(Text, &Rotation, nullptr, nullptr, PPF_None, nullptr);
        return Text;
    }
    if (VectorValue(Value))
    {
        const FVector Vector = JsonVector(Value);
        // K2 vector pin defaults use native CSV syntax; member defaults use native struct text.
        if (PinContext) return FString::Printf(TEXT("%.17g,%.17g,%.17g"), Vector.X, Vector.Y, Vector.Z);
        FString Text;
        TBaseStructure<FVector>::Get()->ExportText(Text, &Vector, nullptr, nullptr, PPF_None, nullptr);
        return Text;
    }
    if (Value->Type == EJson::Boolean) return Value->AsBool() ? TEXT("true") : TEXT("false");
    if (Value->Type == EJson::Number) return FString::Printf(TEXT("%.17g"), Value->AsNumber());
    return Value->AsString();
}
bool StoredNumberMatches(double Value, double Wanted)
{
    // Native double decimal roundoff is allowed, but never a magnitude-sized gap.
    // K2 preserves numeric default text; float execution quantization is separate.
    const double Tolerance = FMath::Min(1.e-3, FMath::Max(1.e-6, FMath::Abs(Wanted) * 1.e-15));
    return FMath::IsFinite(Value) && FMath::Abs(Value - Wanted) <= Tolerance;
}
bool MatchesNativeText(const TSharedPtr<FJsonValue>& Expected, const FString& Actual, bool PinContext)
{
    if (RotatorValue(Expected))
    {
        FRotator Parsed = FRotator::ZeroRotator;
        if (!FDefaultValueHelper::ParseRotator(Actual, Parsed) || Parsed.ContainsNaN()) return false;
        const FRotator Target = JsonRotator(Expected);
        // Compare stored components, not quaternion equivalence: 360 must not silently become 0.
        return StoredNumberMatches(Parsed.Pitch, Target.Pitch) && StoredNumberMatches(Parsed.Yaw, Target.Yaw)
            && StoredNumberMatches(Parsed.Roll, Target.Roll);
    }
    if (VectorValue(Expected))
    {
        FVector Parsed = FVector::ZeroVector;
        const bool Valid = PinContext ? FDefaultValueHelper::ParseVector(Actual, Parsed) : Parsed.InitFromString(Actual);
        const FVector Target = JsonVector(Expected);
        if (!Valid || Parsed.ContainsNaN()) return false;
        for (int32 Axis = 0; Axis < 3; ++Axis)
            if (!StoredNumberMatches(Parsed[Axis], Target[Axis])) return false;
        return true;
    }
    if (Expected->Type == EJson::Number)
    {
        double Parsed = 0;
        return LexTryParseString(Parsed, *Actual) && StoredNumberMatches(Parsed, Expected->AsNumber());
    }
    if (Expected->Type == EJson::Boolean)
        return Actual.Equals(Expected->AsBool() ? TEXT("true") : TEXT("false"), ESearchCase::IgnoreCase)
            || Actual == (Expected->AsBool() ? TEXT("1") : TEXT("0"));
    return Expected->Type == EJson::String && ExactText(Actual, Expected->AsString());
}
FValue FromNativeText(const TSharedPtr<FJsonValue>& Expected, const FString& Actual, bool PinContext)
{
    if (RotatorValue(Expected))
    {
        FRotator Parsed = FRotator::ZeroRotator;
        if (!FDefaultValueHelper::ParseRotator(Actual, Parsed)) return MakeShared<FJsonValueNull>();
        return RotatorJson(Parsed);
    }
    if (VectorValue(Expected))
    {
        FVector Parsed = FVector::ZeroVector;
        if (PinContext) FDefaultValueHelper::ParseVector(Actual, Parsed); else Parsed.InitFromString(Actual);
        return VectorJson(Parsed);
    }
    if (Expected->Type == EJson::Number) return MakeShared<FJsonValueNumber>(FCString::Atod(*Actual));
    if (Expected->Type == EJson::Boolean) return MakeShared<FJsonValueBoolean>(Actual.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Actual == TEXT("1"));
    return MakeShared<FJsonValueString>(Actual);
}

bool Value(EValueKind Kind, const FValue& Item, bool MemberContext)
{
    double Numeric = 0;
    switch (Kind)
    {
    case EValueKind::Boolean: return Item && Item->Type == EJson::Boolean;
    case EValueKind::Int32: return Number(Item, Numeric, 2147483648.0) && Numeric >= MIN_int32 && Numeric <= MAX_int32 && Numeric == FMath::TruncToDouble(Numeric);
    case EValueKind::Real: return Number(Item, Numeric, MemberNumberLimit);
    case EValueKind::String: return Item && Item->Type == EJson::String && Item->AsString().Len() <= MaxStringLength;
    case EValueKind::Name:
    {
        if (!Item || Item->Type != EJson::String || Item->AsString().Len() >= NAME_SIZE) return false;
        if (!MemberContext) return true;
        const FString& Text = Item->AsString();
        if (Text.IsEmpty()) return false;
        const FName Native(*Text);
        return Native.IsNone() ? ExactText(Text, TEXT("None")) : ExactText(Text, Native.ToString());
    }
    case EValueKind::Vector: return VectorValue(Item);
    case EValueKind::Rotator: return RotatorValue(Item);
    case EValueKind::LinearColor:
        if (!Item || Item->Type != EJson::Array || Item->AsArray().Num() != 4) return false;
        for (const auto& Channel : Item->AsArray()) if (!Number(Channel, Numeric, MaterialNumberLimit)) return false;
        return true;
    }
    return false;
}
bool Position(const FValue& Item, const FString& AssetType, FIntPoint& Out)
{
    if (!Item || Item->Type != EJson::Array || Item->AsArray().Num() != 2) return false;
    const double Limit = ExactText(AssetType, TEXT("Material")) ? MaterialPositionLimit : BlueprintPositionLimit;
    double Coordinates[2];
    for (int32 I = 0; I < 2; ++I)
        if (!Number(Item->AsArray()[I], Coordinates[I], Limit) || Coordinates[I] != FMath::TruncToDouble(Coordinates[I])) return false;
    Out = FIntPoint(static_cast<int32>(Coordinates[0]), static_cast<int32>(Coordinates[1])); return true;
}
bool PinType(EValueKind Kind, FEdGraphPinType& Out)
{
    Out = FEdGraphPinType();
    switch (Kind)
    {
    case EValueKind::Boolean: Out.PinCategory = UEdGraphSchema_K2::PC_Boolean; break;
    case EValueKind::Int32: Out.PinCategory = UEdGraphSchema_K2::PC_Int; break;
    case EValueKind::Real: Out.PinCategory = UEdGraphSchema_K2::PC_Real; Out.PinSubCategory = UEdGraphSchema_K2::PC_Double; break;
    case EValueKind::String: Out.PinCategory = UEdGraphSchema_K2::PC_String; break;
    case EValueKind::Name: Out.PinCategory = UEdGraphSchema_K2::PC_Name; break;
    case EValueKind::Vector: Out.PinCategory = UEdGraphSchema_K2::PC_Struct; Out.PinSubCategoryObject = TBaseStructure<FVector>::Get(); break;
    case EValueKind::Rotator: Out.PinCategory = UEdGraphSchema_K2::PC_Struct; Out.PinSubCategoryObject = TBaseStructure<FRotator>::Get(); break;
    default: return false;
    }
    return true;
}
const TArray<FMaterialOptionDefinition>& MaterialOptions()
{
    static const TArray<FMaterialOptionDefinition> Definitions = {
        {EMaterialOption::TwoSided,TEXT("twoSided"),TEXT("TwoSided"),EFieldType::Boolean,true,0},
        {EMaterialOption::Clip,TEXT("opacityMaskClipValue"),TEXT("OpacityMaskClipValue"),EFieldType::Number,false,0.3333},
        {EMaterialOption::MaskDither,TEXT("ditherOpacityMask"),TEXT("DitherOpacityMask"),EFieldType::Boolean,false,0},
        {EMaterialOption::LodDither,TEXT("ditheredLodTransition"),TEXT("DitheredLODTransition"),EFieldType::Boolean,false,0},
        {EMaterialOption::NegativeEmissive,TEXT("allowNegativeEmissiveColor"),TEXT("bAllowNegativeEmissiveColor"),EFieldType::Boolean,false,0}
    };
    return Definitions;
}
FValue MaterialOption(const FMaterialDefinition& Material, EMaterialOption Id, bool Effective)
{
    FValue Result;
    switch (Id)
    {
    case EMaterialOption::TwoSided: Result=MakeShared<FJsonValueBoolean>(Material.TwoSided); break;
    case EMaterialOption::Clip: if(Material.OpacityMaskClipValue.IsSet()) Result=MakeShared<FJsonValueNumber>(Material.OpacityMaskClipValue.GetValue()); break;
    case EMaterialOption::MaskDither: if(Material.DitherOpacityMask.IsSet()) Result=MakeShared<FJsonValueBoolean>(Material.DitherOpacityMask.GetValue()); break;
    case EMaterialOption::LodDither: if(Material.DitheredLodTransition.IsSet()) Result=MakeShared<FJsonValueBoolean>(Material.DitheredLodTransition.GetValue()); break;
    case EMaterialOption::NegativeEmissive: if(Material.AllowNegativeEmissiveColor.IsSet()) Result=MakeShared<FJsonValueBoolean>(Material.AllowNegativeEmissiveColor.GetValue()); break;
    }
    if (!Result && Effective)
        for(const auto& Option:MaterialOptions()) if(Option.Id==Id)
            Result=Option.Type==EFieldType::Number ? FValue(MakeShared<FJsonValueNumber>(Option.Default)) : FValue(MakeShared<FJsonValueBoolean>(Option.Default!=0));
    return Result;
}
void SetMaterialOption(FMaterialDefinition& Material, EMaterialOption Id, const FValue& Value)
{
    switch(Id)
    {
    case EMaterialOption::TwoSided: Material.TwoSided=Value->AsBool(); break;
    case EMaterialOption::Clip: Material.OpacityMaskClipValue=Value->AsNumber(); break;
    case EMaterialOption::MaskDither: Material.DitherOpacityMask=Value->AsBool(); break;
    case EMaterialOption::LodDither: Material.DitheredLodTransition=Value->AsBool(); break;
    case EMaterialOption::NegativeEmissive: Material.AllowNegativeEmissiveColor=Value->AsBool(); break;
    }
}
TSharedRef<FJsonObject> EncodeMaterialSettings(const FMaterialDefinition& Material)
{
    auto Result=MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("domain"),Material.Domain); Result->SetStringField(TEXT("blend"),Material.Blend);
    Result->SetStringField(TEXT("shading"),Material.Shading);
    if(Material.Lighting.IsSet()) Result->SetStringField(TEXT("lighting"),Material.Lighting.GetValue());
    for(const auto& Option:MaterialOptions()) if(auto V=MaterialOption(Material,Option.Id)) Result->SetField(Option.Name,V);
    return Result;
}
static int32 Codepoints(const FString& Text)
{
    int32 Count=0;
    for(int32 I=0;I<Text.Len();++I)
    {
        const uint32 C=Text[I];
        if(C>=0xD800 && C<=0xDBFF && I+1<Text.Len() && uint32(Text[I+1])>=0xDC00 && uint32(Text[I+1])<=0xDFFF) ++I;
        ++Count;
    }
    return Count;
}
bool MemberAttributesValid(const FMemberAttributes& Attributes)
{
    const FString& C=Attributes.Category;
    if(C.IsEmpty() || Codepoints(C)>128 || FChar::IsWhitespace(C[0]) || FChar::IsWhitespace(C[C.Len()-1]) || Codepoints(Attributes.Tooltip)>1024) return false;
    bool Segment=false;
    for(TCHAR Ch:C)
    {
        if(uint32(Ch)<32 || (uint32(Ch)>=127 && uint32(Ch)<=159)) return false;
        if(Ch==TEXT('|')) {if(!Segment)return false;Segment=false;} else Segment=true;
    }
    if(!Segment) return false;
    for(TCHAR Ch:Attributes.Tooltip) if(Ch==0)return false;
    return true;
}
TSharedRef<FJsonObject> EncodeMemberAttributes(const FMemberAttributes& Attributes)
{
    auto Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("instanceEditable"),Attributes.InstanceEditable);
    Result->SetStringField(TEXT("category"),Attributes.Category);Result->SetStringField(TEXT("tooltip"),Attributes.Tooltip);return Result;
}
bool MaterialSettingsValid(const FMaterialDefinition& Material)
{
    auto Registered=[&](const TCHAR* Name,const FString& Text)
    {
        const auto* Field=Fields(EFields::MaterialSettings).FindByPredicate([&](const auto& Definition){return ExactText(Definition.Name,Name);});
        return Field && Field->EnumValues.ContainsByPredicate([&](const FString& Value){return ExactText(Value,Text);});
    };
    return Registered(TEXT("domain"),Material.Domain) && Registered(TEXT("blend"),Material.Blend) && Registered(TEXT("shading"),Material.Shading)
        && (!Material.Lighting.IsSet() || Registered(TEXT("lighting"),Material.Lighting.GetValue()))
        && (!ExactText(Material.Blend, TEXT("Translucent")) || Material.Lighting.IsSet())
        && (!ExactText(Material.Blend,TEXT("Masked")) || Material.OpacityMaskClipValue.IsSet())
        && (!Material.OpacityMaskClipValue.IsSet() || (FMath::IsFinite(Material.OpacityMaskClipValue.GetValue()) && Material.OpacityMaskClipValue.GetValue()>=0 && Material.OpacityMaskClipValue.GetValue()<=1))
        && (!MaterialOption(Material,EMaterialOption::NegativeEmissive,true)->AsBool() || ExactText(Material.Shading,TEXT("Unlit")));
}
bool KindFromPinType(const FEdGraphPinType& Type, EValueKind& Out)
{
    if (Type.IsContainer() || Type.bIsReference) return false;
    for (const auto& Kind : MemberTypes())
    {
        FEdGraphPinType Candidate;
        PinType(Kind.Kind, Candidate);
        if (Candidate.PinCategory == Type.PinCategory && Candidate.PinSubCategory == Type.PinSubCategory
            && Candidate.PinSubCategoryObject == Type.PinSubCategoryObject) { Out = Kind.Kind; return true; }
    }
    return false;
}
bool MemberTextMatches(EValueKind Kind, const FValue& Expected, const FString& Actual)
{
    if (Kind == EValueKind::Int32)
    {
        int32 Parsed = 0;
        return Value(Kind, Expected) && LexTryParseString(Parsed, *Actual) && Parsed == static_cast<int32>(Expected->AsNumber());
    }
    return MatchesNativeText(Expected, Actual);
}
UEnum* NativeEnum(FProperty* Property)
{
    if (FEnumProperty* Enum = CastField<FEnumProperty>(Property)) return Enum->GetEnum();
    if (FByteProperty* Byte = CastField<FByteProperty>(Property)) return Byte->Enum;
    return nullptr;
}
namespace
{
struct FNumberBounds
{
    double Minimum = -MaterialNumberLimit, Maximum = MaterialNumberLimit;
    bool Integral = false, Positive = false;
};
FNumberBounds NumberBounds(FNumericProperty* Property)
{
    FNumberBounds Result;Result.Integral=Property->IsInteger();
    const FString Name=Property->GetName();
    if(Name==TEXT("CoordinateIndex") || Name==TEXT("ConstCoordinate")) {Result.Minimum=0;Result.Maximum=7;Result.Integral=true;}
    if(Name==TEXT("Period")) Result.Positive=true;
    return Result;
}
bool AllowedEnumName(const FString& Name) {return !Name.EndsWith(TEXT("MAX")) && !Name.Contains(TEXT("Virtual"));}
}
TSharedRef<FJsonObject> MaterialPropertySchema(FProperty* Property)
{
    auto Result=MakeShared<FJsonObject>();
    if(FBoolProperty* Boolean=CastField<FBoolProperty>(Property)) Result->SetStringField(TEXT("type"),TEXT("boolean"));
    else if(UEnum* Enum=NativeEnum(Property))
    {
        Result->SetStringField(TEXT("type"),TEXT("string"));TArray<FValue> Names;
        for(int32 I=0;I<Enum->NumEnums();++I)
        {
            const FString Name=Enum->GetNameStringByIndex(I);
            if(AllowedEnumName(Name)) Names.Add(MakeShared<FJsonValueString>(Name));
        }
        Result->SetArrayField(TEXT("enum"),Names);
    }
    else if(FNumericProperty* Numeric=CastField<FNumericProperty>(Property))
    {
        const auto Bounds=NumberBounds(Numeric);
        Result->SetStringField(TEXT("type"),Bounds.Integral?TEXT("integer"):TEXT("number"));
        Result->SetNumberField(Bounds.Positive?TEXT("exclusiveMinimum"):TEXT("minimum"),Bounds.Positive?0:Bounds.Minimum);
        Result->SetNumberField(TEXT("maximum"),Bounds.Maximum);
    }
    else if(CastField<FNameProperty>(Property))
    {Result->SetStringField(TEXT("type"),TEXT("string"));Result->SetStringField(TEXT("pattern"),TEXT("^[A-Za-z0-9]{1,64}$"));}
    else if(FStructProperty* Struct=CastField<FStructProperty>(Property);Struct && Struct->Struct==TBaseStructure<FLinearColor>::Get())
    {
        auto Channel=MakeShared<FJsonObject>();Channel->SetStringField(TEXT("type"),TEXT("number"));Channel->SetNumberField(TEXT("minimum"),-MaterialNumberLimit);Channel->SetNumberField(TEXT("maximum"),MaterialNumberLimit);
        Result->SetStringField(TEXT("type"),TEXT("array"));Result->SetNumberField(TEXT("minItems"),4);Result->SetNumberField(TEXT("maxItems"),4);Result->SetObjectField(TEXT("items"),Channel);
    }
    else if(CastField<FObjectPropertyBase>(Property) && Property->GetName()==TEXT("Texture"))
    {
        Result->SetStringField(TEXT("type"),TEXT("string"));Result->SetStringField(TEXT("pattern"),TEXT("^(?:/Engine/|/Game/)(?!.*\\.\\.).*\\..+$"));
        Result->SetStringField(TEXT("x-nativeConstraint"),TEXT("actual canonical existing nonvirtual Texture2D; sampler checked natively"));
    }
    else Result->SetObjectField(TEXT("not"),MakeShared<FJsonObject>());
    return Result;
}

bool ApplyMaterialProperty(UMaterialExpression* Expression, FProperty* Property, const TSharedPtr<FJsonValue>& Value, FString& Error, int32 ReadRevision)
{
    if (!Property || !Value) return Fail(Error, TEXT("missing native property/value"));
    void* Address = Property->ContainerPtrToValuePtr<void>(Expression);
    const FString Name = Property->GetName();
    if (FBoolProperty* Bool = CastField<FBoolProperty>(Property))
    {
        bool B = false;
        if (Value->Type != EJson::Boolean || !Value->TryGetBool(B)) return Fail(Error, Name + TEXT(" requires boolean"));
        Bool->SetPropertyValue(Address, B);
        return true;
    }
    if (UEnum* Enum = NativeEnum(Property))
    {
        FString EnumName;
        if (Value->Type != EJson::String || !Value->TryGetString(EnumName)) return Fail(Error, Name + TEXT(" requires exact enum name"));
        const int64 EnumValue = Enum->GetValueByNameString(EnumName);
        if (EnumValue == INDEX_NONE || !ExactText(Enum->GetNameStringByValue(EnumValue), EnumName)
            || !AllowedEnumName(EnumName))
            return Fail(Error, Name + TEXT(" unsupported enum value ") + EnumName);
        if (FEnumProperty* Scoped = CastField<FEnumProperty>(Property))
            Scoped->GetUnderlyingProperty()->SetIntPropertyValue(Address,static_cast<uint64>(EnumValue));
        else CastFieldChecked<FByteProperty>(Property)->SetPropertyValue(Address, static_cast<uint8>(EnumValue));
        return true;
    }
    if (FNumericProperty* NativeNumber = CastField<FNumericProperty>(Property))
    {
        double V = 0;
        if (!Number(Value, V, MaterialNumberLimit)) return Fail(Error, Name + TEXT(" requires finite number in [-1000000,1000000]"));
        const auto Bounds=NumberBounds(NativeNumber);
        if(V<Bounds.Minimum || V>Bounds.Maximum || (Bounds.Positive && V<=0) || (Bounds.Integral && V!=FMath::TruncToDouble(V)))
            return Fail(Error,Name+TEXT(" outside its registered native numeric bounds"));
        if (NativeNumber->IsInteger())
        {
            if (V != FMath::TruncToDouble(V)) return Fail(Error, Name + TEXT(" requires integer"));
            NativeNumber->SetIntPropertyValue(Address, static_cast<int64>(V));
        }
        else NativeNumber->SetFloatingPointPropertyValue(Address, V);
        return true;
    }
    if (FNameProperty* NameProperty = CastField<FNameProperty>(Property))
    {
        FString Text;
        if (Value->Type != EJson::String || !Value->TryGetString(Text) || !Identifier(Text))
            return Fail(Error, Name + TEXT(" requires ASCII alphanumeric name 1..64"));
        NameProperty->SetPropertyValue(Address, FName(*Text));
        return true;
    }
    if (FStructProperty* Struct = CastField<FStructProperty>(Property); Struct && Struct->Struct == TBaseStructure<FLinearColor>::Get())
    {
        const TArray<TSharedPtr<FJsonValue>>* Array = nullptr;
        double V[4] = {};
        if (!Value->TryGetArray(Array) || Array->Num() != 4) return Fail(Error, Name + TEXT(" requires four linear RGBA numbers"));
        for (int32 I = 0; I < 4; ++I) if (!Number((*Array)[I], V[I], MaterialNumberLimit)) return Fail(Error, Name + TEXT(" contains invalid color number"));
        *static_cast<FLinearColor*>(Address) = FLinearColor(V[0], V[1], V[2], V[3]);
        return true;
    }
    if (FObjectPropertyBase* Object = CastField<FObjectPropertyBase>(Property); Object && Name == TEXT("Texture"))
    {
        FString Path;
        if (Value->Type != EJson::String || !Value->TryGetString(Path)) return Fail(Error, TEXT("Texture requires an exact object path"));
        NodeForgeResourceSource::FCapture Capture;
        if (!Capture.Begin(Path, TEXT("Texture2D"), Error, ReadRevision, false)) return false;
        UTexture2D* Texture = CastChecked<UTexture2D>(Capture.Object());
        Object->SetObjectPropertyValue(Address, Texture);
        return Capture.Verify(Error);
    }
    return Fail(Error, TEXT("unsupported reflected property type: ") + Name);
}

FValue ReadMaterialProperty(UMaterialExpression* Expression, FProperty* Property)
{
    const void* Address = Property->ContainerPtrToValuePtr<void>(Expression);
    if (FBoolProperty* Bool = CastField<FBoolProperty>(Property)) return MakeShared<FJsonValueBoolean>(Bool->GetPropertyValue(Address));
    if (UEnum* Enum = NativeEnum(Property))
    {
        const FEnumProperty* Scoped = CastField<FEnumProperty>(Property);
        const int64 Value = Scoped ? Scoped->GetUnderlyingProperty()->GetSignedIntPropertyValue(Address)
            : CastFieldChecked<FByteProperty>(Property)->GetPropertyValue(Address);
        return MakeShared<FJsonValueString>(Enum->GetNameStringByValue(Value));
    }
    if (FNumericProperty* Number = CastField<FNumericProperty>(Property))
        return MakeShared<FJsonValueNumber>(Number->IsInteger() ? static_cast<double>(Number->GetSignedIntPropertyValue(Address)) : Number->GetFloatingPointPropertyValue(Address));
    if (FNameProperty* Name = CastField<FNameProperty>(Property)) return MakeShared<FJsonValueString>(Name->GetPropertyValue(Address).ToString());
    if (FStructProperty* Struct = CastField<FStructProperty>(Property); Struct && Struct->Struct == TBaseStructure<FLinearColor>::Get())
    {
        const FLinearColor& Color = *static_cast<const FLinearColor*>(Address);
        TArray<TSharedPtr<FJsonValue>> Array;
        for (float Channel : {Color.R, Color.G, Color.B, Color.A}) Array.Add(MakeShared<FJsonValueNumber>(Channel));
        return MakeShared<FJsonValueArray>(Array);
    }
    if (FObjectPropertyBase* Object = CastField<FObjectPropertyBase>(Property))
    {
        UObject* Value = Object->GetObjectPropertyValue(Address);
        if (Value) return MakeShared<FJsonValueString>(Value->GetPathName());
        return MakeShared<FJsonValueNull>();
    }
    return MakeShared<FJsonValueNull>();
}

bool ExportVersion(const TSharedPtr<FJsonObject>& Config, int32& Version, FString& Error)
{
    Version=2;
    if(!Config || !FieldsValid(Config->Values,EFields::ExportConfig,Error))return false;
    if(Config->HasField(TEXT("formatVersion")))
    {
        const double Selected=Config->GetNumberField(TEXT("formatVersion"));
        if(Selected!=2 && Selected!=3){Error=TEXT("formatVersion must be 2 or 3");return false;}
        Version=static_cast<int32>(Selected);
    }
    return true;
}
TSharedRef<FJsonObject> MaterialSettingsSchema()
{
    auto Result=FieldsSchema(EFields::MaterialSettings);auto If=MakeShared<FJsonObject>(),Properties=MakeShared<FJsonObject>(),Blend=MakeShared<FJsonObject>(),Then=MakeShared<FJsonObject>();
    Blend->SetStringField(TEXT("const"),TEXT("Translucent"));Properties->SetObjectField(TEXT("blend"),Blend);If->SetObjectField(TEXT("properties"),Properties);
    Then->SetArrayField(TEXT("required"),{MakeShared<FJsonValueString>(TEXT("lighting"))});Result->SetObjectField(TEXT("if"),If);Result->SetObjectField(TEXT("then"),Then);
    TArray<FValue> All;
    auto Masked=MakeShared<FJsonObject>(),MaskIf=MakeShared<FJsonObject>(),MaskProps=MakeShared<FJsonObject>(),MaskBlend=MakeShared<FJsonObject>(),MaskThen=MakeShared<FJsonObject>();
    MaskBlend->SetStringField(TEXT("const"),TEXT("Masked"));MaskProps->SetObjectField(TEXT("blend"),MaskBlend);MaskIf->SetObjectField(TEXT("properties"),MaskProps);
    MaskThen->SetArrayField(TEXT("required"),{MakeShared<FJsonValueString>(TEXT("opacityMaskClipValue"))});Masked->SetObjectField(TEXT("if"),MaskIf);Masked->SetObjectField(TEXT("then"),MaskThen);All.Add(MakeShared<FJsonValueObject>(Masked));
    auto Negative=MakeShared<FJsonObject>(),NegIf=MakeShared<FJsonObject>(),NegProps=MakeShared<FJsonObject>(),NegBool=MakeShared<FJsonObject>(),NegThen=MakeShared<FJsonObject>(),ShadingProps=MakeShared<FJsonObject>(),Shading=MakeShared<FJsonObject>();
    NegBool->SetBoolField(TEXT("const"),true);NegProps->SetObjectField(TEXT("allowNegativeEmissiveColor"),NegBool);NegIf->SetObjectField(TEXT("properties"),NegProps);NegIf->SetArrayField(TEXT("required"),{MakeShared<FJsonValueString>(TEXT("allowNegativeEmissiveColor"))});
    Shading->SetStringField(TEXT("const"),TEXT("Unlit"));ShadingProps->SetObjectField(TEXT("shading"),Shading);NegThen->SetObjectField(TEXT("properties"),ShadingProps);Negative->SetObjectField(TEXT("if"),NegIf);Negative->SetObjectField(TEXT("then"),NegThen);All.Add(MakeShared<FJsonValueObject>(Negative));
    Result->SetArrayField(TEXT("allOf"),All);Result->SetStringField(TEXT("description"),TEXT("Surface only. Inactive clip/dither values are stored; no visual/TAA/LOD claim. Root connections must pass native IsPropertyActive."));return Result;
}
TSharedRef<FJsonObject> Metadata()
{
    TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    const TPair<EFields,const TCHAR*> Groups[] = {
        {EFields::RootV2,TEXT("RootV2")},{EFields::RootV3,TEXT("RootV3")},{EFields::Target,TEXT("Target")},
        {EFields::MaterialAsset,TEXT("MaterialAsset")},{EFields::BlueprintAsset,TEXT("BlueprintAsset")},{EFields::InstanceAsset,TEXT("InstanceAsset")},
        {EFields::MemberAttributes,TEXT("MemberAttributes")},{EFields::MaterialSettings,TEXT("MaterialSettings")},{EFields::BlueprintSettingsV2,TEXT("BlueprintSettingsV2")},{EFields::BlueprintParent,TEXT("BlueprintParent")},
        {EFields::Members,TEXT("Members")},{EFields::VariableV2,TEXT("VariableV2")},{EFields::VariableV3,TEXT("VariableV3")},{EFields::ValueType,TEXT("ValueType")},
        {EFields::InstanceSettingsV2,TEXT("InstanceSettingsV2")},{EFields::InstanceParent,TEXT("InstanceParent")},{EFields::Parameters,TEXT("Parameters")},
        {EFields::Graph,TEXT("Graph")},{EFields::NodeV2,TEXT("NodeV2")},{EFields::NodeV3,TEXT("NodeV3")},{EFields::Edge,TEXT("Edge")},
        {EFields::FunctionBinding,TEXT("FunctionBinding")},{EFields::MemberBinding,TEXT("MemberBinding")},{EFields::MaterialFunctionBinding,TEXT("MaterialFunctionBinding")},
        {EFields::Rotator,TEXT("Rotator")},{EFields::Sequence,TEXT("Sequence")},{EFields::SwitchInteger,TEXT("SwitchInteger")},{EFields::SwitchName,TEXT("SwitchName")},
        {EFields::FunctionPropertiesV2,TEXT("FunctionPropertiesV2")},{EFields::MemberPropertiesV2,TEXT("MemberPropertiesV2")},{EFields::MaterialFunctionPropertiesV2,TEXT("MaterialFunctionPropertiesV2")},{EFields::ExportConfig,TEXT("ExportConfig")}
    };
    auto FieldGroups = MakeShared<FJsonObject>();
    const TCHAR* FieldTypeNames[] = {TEXT("string"),TEXT("integer"),TEXT("number"),TEXT("boolean"),TEXT("object"),TEXT("array"),TEXT("value")};
    for (const auto& Group : Groups)
    {
        TArray<FValue> Registered;
        for (const auto& Field : Fields(Group.Key))
        {
            auto Item = MakeShared<FJsonObject>();Item->SetStringField(TEXT("name"),Field.Name);
            Item->SetStringField(TEXT("type"),FieldTypeNames[static_cast<int32>(Field.Type)]);Item->SetBoolField(TEXT("required"),Field.Required);
            if(!Field.EnumValues.IsEmpty()) {TArray<FValue> Values;for(const auto& Name:Field.EnumValues)Values.Add(MakeShared<FJsonValueString>(Name));Item->SetArrayField(TEXT("enum"),Values);}
            Registered.Add(MakeShared<FJsonValueObject>(Item));
        }
        FieldGroups->SetArrayField(Group.Value,Registered);
    }
    Result->SetObjectField(TEXT("fieldGroups"),FieldGroups);
    Result->SetObjectField(TEXT("materialSettingsSchema"),MaterialSettingsSchema());
    Result->SetObjectField(TEXT("memberAttributesSchema"),FieldsSchema(EFields::MemberAttributes));
    TArray<FValue> Options;
    for(const auto& Option:MaterialOptions())
    {
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("name"),Option.Name);Row->SetStringField(TEXT("nativeProperty"),Option.NativeName);
        Row->SetBoolField(TEXT("required"),Option.Required);Row->SetField(TEXT("default"),Option.Type==EFieldType::Number ? FValue(MakeShared<FJsonValueNumber>(Option.Default)) : FValue(MakeShared<FJsonValueBoolean>(Option.Default!=0)));
        Options.Add(MakeShared<FJsonValueObject>(Row));
    }
    Result->SetArrayField(TEXT("materialOptions"),Options);
    TArray<FValue> Assets;
    for (const auto& Type : AssetTypes())
    {
        auto Item = MakeShared<FJsonObject>();Item->SetStringField(TEXT("type"),Type.Type);
        Item->SetStringField(TEXT("graphId"),Type.GraphId);Item->SetStringField(TEXT("graphKind"),Type.GraphKind);Item->SetNumberField(TEXT("graphCount"),Type.GraphCount);
        Item->SetNumberField(TEXT("minimumNodes"),Type.MinimumNodes);Item->SetNumberField(TEXT("maximumNodes"),Type.MaximumNodes);
        Item->SetNumberField(TEXT("minimumEdges"),Type.MinimumEdges);Item->SetNumberField(TEXT("maximumEdges"),Type.MaximumEdges);
        Assets.Add(MakeShared<FJsonValueObject>(Item));
    }
    Result->SetArrayField(TEXT("assetTypes"),Assets);
    TArray<FValue> Bindings;auto NodeConfigurations=MakeShared<FJsonObject>();
    auto AddBinding = [&](const FString& Class,EBindingKind Kind)
    {
        auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("class"),Class);Item->SetStringField(TEXT("kind"),BindingName(Kind));
        Bindings.Add(MakeShared<FJsonValueObject>(Item));
    };
    for (const auto& Node : NodeForgeNodeDefinitions::Specials()) {AddBinding(Node.Class,Node.Binding);NodeConfigurations->SetObjectField(Node.Class,NodePropertiesSchema(Node.Class));}
    for (const auto& Node : NodeForgeNodeDefinitions::Materials()) if (Node.RequiresFunctionBinding) AddBinding(Node.Class,EBindingKind::MaterialFunction);
    Result->SetArrayField(TEXT("nodeBindings"),Bindings);
    Result->SetObjectField(TEXT("nodeConfigurationSchemas"),NodeConfigurations);
    TArray<FValue> Types;
    for (const auto& Type : MemberTypes())
    {
        auto Item = MakeShared<FJsonObject>();
        Item->SetStringField(TEXT("name"), Type.Name);
        Item->SetNumberField(TEXT("minimumBlueprintRevision"), Type.MinimumRevision);
        Item->SetObjectField(TEXT("valueSchema"),ValueSchema(Type.Kind));
        Types.Add(MakeShared<FJsonValueObject>(Item));
    }
    Result->SetArrayField(TEXT("memberTypes"), Types);
    TArray<FValue> DefaultShapes;
    for(EValueKind Kind:{EValueKind::Boolean,EValueKind::Real,EValueKind::String,EValueKind::Vector,EValueKind::Rotator})DefaultShapes.Add(MakeShared<FJsonValueObject>(ValueSchema(Kind,false)));
    auto PinDefaults=MakeShared<FJsonObject>();PinDefaults->SetArrayField(TEXT("anyOf"),DefaultShapes);Result->SetObjectField(TEXT("pinDefaultSchema"),PinDefaults);
    Result->SetObjectField(TEXT("instanceScalarSchema"),ValueSchema(EValueKind::Real));
    Result->GetObjectField(TEXT("instanceScalarSchema"))->SetNumberField(TEXT("minimum"),-MaterialNumberLimit);
    Result->GetObjectField(TEXT("instanceScalarSchema"))->SetNumberField(TEXT("maximum"),MaterialNumberLimit);
    Result->SetObjectField(TEXT("instanceVectorSchema"),ValueSchema(EValueKind::LinearColor));
    Result->SetArrayField(TEXT("acceptedGraphVersions"), {MakeShared<FJsonValueNumber>(2), MakeShared<FJsonValueNumber>(3)});
    Result->SetNumberField(TEXT("materialPositionLimit"), MaterialPositionLimit);
    Result->SetNumberField(TEXT("blueprintPositionLimit"), BlueprintPositionLimit);
    Result->SetNumberField(TEXT("materialNumberLimit"), MaterialNumberLimit);
    Result->SetNumberField(TEXT("memberNumberLimit"), MemberNumberLimit);
    Result->SetNumberField(TEXT("maxStringLength"), MaxStringLength);
    Result->SetNumberField(TEXT("nameMaximumLength"), NAME_SIZE - 1);
    Result->SetNumberField(TEXT("maxVariables"), MaxVariables);
    Result->SetNumberField(TEXT("maxInstanceOverrides"), MaxInstanceOverrides);
    Result->SetNumberField(TEXT("maxInputUtf8Bytes"), MaxInputUtf8Bytes);
    Result->SetNumberField(TEXT("maxDepth"), MaxJsonDepth);
    return Result;
}
}
