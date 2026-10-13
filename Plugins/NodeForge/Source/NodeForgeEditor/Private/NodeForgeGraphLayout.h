#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "NodeForgeGraphDocument.h"
class UMaterial;
class UEdGraphNode;
class UMaterialExpression;
namespace NodeForgeGraphLayout
{
    using FObject = TSharedPtr<FJsonObject>;
    bool Material(UMaterial* Asset, const NodeForgeGraphDocument::FGraphDocument& Request,
        const TMap<FString, UMaterialExpression*>& Instances, FObject& Evidence, FString& Error);
    bool Blueprint(const NodeForgeGraphDocument::FGraphDocument& Request, const TMap<FString, UEdGraphNode*>& Instances,
        FObject& Evidence, FString& Error, bool ApplyPositions = true);
}
