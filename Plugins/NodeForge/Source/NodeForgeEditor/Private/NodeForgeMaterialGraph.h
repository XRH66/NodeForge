#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "NodeForgeGraphDocument.h"

class UMaterial;
class UMaterialExpression;
class UPackage;

namespace NodeForgeMaterialGraph
{
using FObject = TSharedPtr<FJsonObject>;
// Exact archived v2 catalog: only its original node family remains accepted with this digest.
inline const FString LegacyCatalogDigest = TEXT("0d786af7d63e1ea209691493e9b336f50e61eaeb45ef34eeec12ff01b22a884f");
inline const FString MaterialFamiliesCatalogDigest = TEXT("9703b92dac063c1a473c335d505d78d917fc97608a78501dc0c1d1f2491264ca");
inline const FString MaterialInstancesCatalogDigest = TEXT("1b12e94c89af0c2e18d3dbbf362222754473f341d3a353eff24ca6abc8809dc1");
inline const FString TextureSamplersCatalogDigest = TEXT("d1bcfbf04d3e221dcf2ee732ed1d34fa7db581d4e71d9449719755ceacf0edc9");
inline const FString MaterialFunctionsCatalogDigest = TEXT("7d11f08760383d7b4704eab618fb098447f2558d9b456955139fbd818b7851e2");
inline const FString BlueprintMotionCatalogDigest = TEXT("a6488c77da856b78bd72ac32d5c480008f34cdb897524c2296289f088893b786");
inline const FString BlueprintFunctionFamiliesCatalogDigest = TEXT("6993743a0513489069423790f678c2cccd6bccc29435cd0dd98e4156248aa069");
inline const FString TextureFunctionsCatalogDigest = TEXT("340e22d25dbc9c24271c5e05c89c5043d366f765a5e8b9fc4bc40bd2080389d1");
inline const FString BlueprintComponentsCatalogDigest = TEXT("7dea96da738f0f1b526e7782ec08230a4e0d6d14e815db2538638693f2309326");
inline const FString MemberScalarsCatalogDigest = TEXT("cd5de7be8e3662c0ca769a729f64b844b359a25bc46d39a26c2d528bef0bb9e1");
inline const FString BlueprintSequenceCatalogDigest = TEXT("57b27415fee08ce08df3b734d9ea8fbd7a03678564f218007a0f770c3e478b7d");
inline const FString SpatialInputsCatalogDigest = TEXT("588f572606053d53eebe80b3461b0e01baac2326f30e778c2cdbc3f03e4e2801");
inline const FString RotatorMembersCatalogDigest = TEXT("272ec558cb541bb1d69b4d998c4aced90cbcbe32ec42f3926a8005618eb52950");
inline const FString ColorSelectionCatalogDigest = TEXT("96c9459d583fd0c5156842882d5ba68a259db91e5b3db8d495f77be79cfee8c7");
inline const FString MathInterpolationCatalogDigest = TEXT("aec5b1e30e736a0a16b069b7567817d23a8e9ca9a583aa951ee538e6e73c24c7");
inline const FString BlueprintSwitchesCatalogDigest = TEXT("1ae2791da31e336b3d147d1ed20e173ddfe51e721d255df040c843c743171317");
inline const FString StringIdentityCatalogDigest = TEXT("af8a95377783d90668b062fef423717e1eec33b7a23139aed5de4d3206bc15aa");
TSharedRef<FJsonObject> Catalog();
bool Validate(const NodeForgeGraphDocument::FGraphDocument& Request, FString& Error);
UMaterial* Build(UPackage* Package, const FString& Name, const NodeForgeGraphDocument::FGraphDocument& Request, FString& Error);
bool Readback(UMaterial* Material, const NodeForgeGraphDocument::FGraphDocument& Request, FObject& Evidence, FString& Error);
// Read the saved actual graph, without requiring NodeForge tags or its original input.
// A complete supported request is returned only after native reconstruction comparison.
bool Export(const FObject& Config, const FString& CatalogDigest, FObject& Request, FObject& Evidence, FString& Error);
bool ExportExpressions(UMaterial* Material, TArray<UMaterialExpression*>& Expressions, FString& Error);
}
