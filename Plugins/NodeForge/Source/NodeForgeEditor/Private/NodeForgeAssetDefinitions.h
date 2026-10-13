#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "NodeForgeGraphDocument.h"

struct FEdGraphPinType;
class UMaterialExpression;
class FProperty;
class UEnum;
namespace NodeForgeAssetDefinitions
{
using namespace NodeForgeGraphDocument;
enum class EFields : uint8
{
    RootV2, RootV3, Target, MaterialAsset, BlueprintAsset, InstanceAsset,
    MaterialSettings, BlueprintSettingsV2, BlueprintParent, Members,
    VariableV2, VariableV3, ValueType, InstanceSettingsV2, InstanceParent,
    Parameters, Graph, NodeV2, NodeV3, Edge, FunctionBinding, MemberBinding,
    MaterialFunctionBinding, MemberAttributes, Rotator, Sequence, SwitchInteger, SwitchName,
    FunctionPropertiesV2, MemberPropertiesV2, MaterialFunctionPropertiesV2, ExportConfig
};
enum class EFieldType : uint8 { String, Integer, Number, Boolean, Object, Array, Value };
struct FFieldDefinition
{
    FString Name;
    EFieldType Type;
    bool Required;
    TArray<FString> EnumValues;
};
enum class EMaterialOption : uint8 { TwoSided, Clip, MaskDither, LodDither, NegativeEmissive };
struct FMaterialOptionDefinition
{
    EMaterialOption Id;
    FString Name, NativeName;
    EFieldType Type;
    bool Required;
    double Default;
};
const TArray<FMaterialOptionDefinition>& MaterialOptions();
FValue MaterialOption(const FMaterialDefinition& Material, EMaterialOption Id, bool Effective = false);
void SetMaterialOption(FMaterialDefinition& Material, EMaterialOption Id, const FValue& Value);
TSharedRef<FJsonObject> EncodeMaterialSettings(const FMaterialDefinition& Material);
bool MemberAttributesValid(const FMemberAttributes& Attributes);
TSharedRef<FJsonObject> EncodeMemberAttributes(const FMemberAttributes& Attributes);
struct FMemberTypeDefinition
{
    FString Name;
    EValueKind Kind;
    int32 MinimumRevision;
};
struct FAssetTypeDefinition
{
    FString Type, GraphId, GraphKind;
    int32 GraphCount, MinimumNodes, MaximumNodes, MinimumEdges, MaximumEdges;
    EFields SettingsV2, AssetV3;
};
inline constexpr int32 MaxInputUtf8Bytes = 65536;
inline constexpr int32 MaxJsonDepth = 32;
inline constexpr int32 MaxVariables = 64;
inline constexpr int32 MaxInstanceOverrides = 128;
inline constexpr int32 MaxStringLength = 4096;
inline constexpr int32 MinimumSequenceOutputs = 2;
inline constexpr int32 MaximumSequenceOutputs = 8;
inline constexpr int32 MaximumSwitchCases = 8;
inline constexpr double MemberNumberLimit = 1.e12;
inline constexpr double MaterialNumberLimit = 1000000.0;
inline constexpr double MaterialPositionLimit = 1000000.0;
inline constexpr double BlueprintPositionLimit = 100000.0;
const TArray<FFieldDefinition>& Fields(EFields Group);
bool FieldsValid(const FValueMap& Input, EFields Group, FString& Error);
bool ExportVersion(const TSharedPtr<FJsonObject>& Config, int32& Version, FString& Error);
TSharedRef<FJsonObject> FieldsSchema(EFields Group);
TSharedRef<FJsonObject> ValueSchema(EValueKind Kind, bool MemberContext = true);
bool NodePropertiesValid(const FString& Class, const FValueMap& Properties, FString& Error);
TSharedRef<FJsonObject> NodePropertiesSchema(const FString& Class);
const TArray<FMemberTypeDefinition>& MemberTypes();
const TArray<FAssetTypeDefinition>& AssetTypes();
const FAssetTypeDefinition* AssetType(const FString& Type);
EBindingKind BindingKind(const FString& Class);
FString BindingName(EBindingKind Kind);
FString TypeName(EValueKind Kind);
bool TypeFromName(const FString& Name, EValueKind& Kind);
bool Identifier(const FString& Name, bool FirstLetter = false, int32 MaxLength = 64);
bool Sha256(const FString& Text);
bool Number(const FValue& Value, double& Out, double Magnitude);
bool Value(EValueKind Kind, const FValue& Value, bool MemberContext = true);
bool ScalarValue(const FValue& Value);
FValue VectorJson(const FVector& Value);
FValue RotatorJson(const FRotator& Value);
bool Position(const FValue& Value, const FString& AssetType, FIntPoint& Out);
bool MaterialSettingsValid(const FMaterialDefinition& Material);
TSharedRef<FJsonObject> MaterialSettingsSchema();
bool PinType(EValueKind Kind, FEdGraphPinType& Out);
bool KindFromPinType(const FEdGraphPinType& Type, EValueKind& Out);
FString NativeText(const FValue& Value, bool PinContext = false);
FValue FromNativeText(const FValue& ExpectedShape, const FString& Actual, bool PinContext = false);
bool MatchesNativeText(const FValue& Expected, const FString& Actual, bool PinContext = false);
bool MemberTextMatches(EValueKind Kind, const FValue& Expected, const FString& Actual);
UEnum* NativeEnum(FProperty* Property);
TSharedRef<FJsonObject> MaterialPropertySchema(FProperty* Property);
bool ApplyMaterialProperty(UMaterialExpression* Expression, FProperty* Property, const FValue& Value, FString& Error, int32 ReadRevision = 0);
FValue ReadMaterialProperty(UMaterialExpression* Expression, FProperty* Property);
// Serializable facts/schema generation consume these same field/type definitions.
TSharedRef<FJsonObject> Metadata();
}
