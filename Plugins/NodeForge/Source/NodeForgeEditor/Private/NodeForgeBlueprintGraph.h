#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "NodeForgeGraphDocument.h"

class UBlueprint;
class UPackage;
class UEdGraphNode;

namespace NodeForgeBlueprintGraph
{
using FObject = TSharedPtr<FJsonObject>;
TSharedRef<FJsonObject> Catalog();
bool Validate(const NodeForgeGraphDocument::FGraphDocument& Request, FString& Error);
UBlueprint* Build(UPackage* Package, const FString& Name, const NodeForgeGraphDocument::FGraphDocument& Request, FString& Error);
bool Readback(UBlueprint* Blueprint, const NodeForgeGraphDocument::FGraphDocument& Request, FObject& Evidence, FString& Error,
    const TMap<UEdGraphNode*, FString>* ReadOnlyIds = nullptr);
bool Export(const FObject& Config, const FString& CatalogDigest, FObject& Request, FObject& Evidence, FString& Error);
}
