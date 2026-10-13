#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "NodeForgeGraphDocument.h"
class UMaterialInstanceConstant;
class UPackage;
namespace NodeForgeMaterialInstances
{
using FObject = TSharedPtr<FJsonObject>;
TSharedRef<FJsonObject> Catalog();
bool Inspect(const FObject& Config, FObject& Evidence, FString& Error);
bool Validate(const NodeForgeGraphDocument::FGraphDocument& Request, FString& Error);
UMaterialInstanceConstant* Build(UPackage* Package, const FString& Name, const NodeForgeGraphDocument::FGraphDocument& Request, FString& Error);
bool Readback(UMaterialInstanceConstant* Instance, const NodeForgeGraphDocument::FGraphDocument& Request, FObject& Evidence, FString& Error);
}
