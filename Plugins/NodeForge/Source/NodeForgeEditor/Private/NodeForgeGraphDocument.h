#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonValue.h"
#include "Misc/TVariant.h"

// Shared modern production data. Only the codec knows v2/v3 wire layouts.
namespace NodeForgeGraphDocument
{
using FValue = TSharedPtr<FJsonValue>;
using FValueMap = TMap<FString, FValue>;
enum class EValueKind : uint8 { Boolean, Int32, Real, String, Name, Vector, Rotator, LinearColor };
struct FValueType { EValueKind Kind = EValueKind::Real; };
struct FMemberAttributes
{
    bool InstanceEditable = false;
    FString Category, Tooltip;
};
struct FMemberDescription
{
    FString Name;
    FValueType Type;
    FValue Default;
    TOptional<FMemberAttributes> Attributes;
};
struct FMaterialDefinition
{
    FString Domain, Blend, Shading;
    bool TwoSided = false;
    TOptional<FString> Lighting;
    TOptional<double> OpacityMaskClipValue;
    TOptional<bool> DitherOpacityMask, DitheredLodTransition, AllowNegativeEmissiveColor;
};
struct FBlueprintDefinition
{
    FString ParentClass;
    TArray<FMemberDescription> Variables;
};
struct FInstanceDefinition
{
    FString ParentPath, ParentAssetSha256;
    FValueMap Scalars, Vectors, Textures;
};
enum class EBindingKind : uint8 { None, Event, Function, Member, MaterialFunction };
struct FNodeBinding
{
    EBindingKind Kind = EBindingKind::None;
    FString Owner, Member, Name, Path, AssetSha256;
};
struct FNodeDescription
{
    FString Id, Class;
    FNodeBinding Binding;
    FValueMap Properties, Defaults;
    bool DefaultsPresent = false;
    TOptional<FIntPoint> Position;
};
struct FEdgeDescription { FString From, Output, To, Input; };
struct FGraphDefinition
{
    FString Id, Kind;
    TArray<FNodeDescription> Nodes;
    TArray<FEdgeDescription> Edges;
    const FNodeDescription* FindNode(const FString& Id) const;
};
struct FGraphDocument
{
    int32 WireVersion = 2;
    FString RequestId, CatalogDigest, Target;
    TVariant<FMaterialDefinition, FBlueprintDefinition, FInstanceDefinition> Asset;
    TArray<FGraphDefinition> Graphs;
    // Byte identities stay separate from value projections and differences.
    FString RawText, RawInputSha256;
    FString AssetType() const;
};
}
