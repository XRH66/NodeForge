#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "NodeForgeGraphDocument.h"

class UEdGraph;
class UEdGraphNode;

namespace NodeForgeBlueprintSwitches
{
using FObject = TSharedPtr<FJsonObject>;
UClass* ClassFor(const FString& Class);
bool ValidateProperties(const FString& Class, const NodeForgeGraphDocument::FValueMap& Properties, FString& Error);
UEdGraphNode* Spawn(UEdGraph* Graph, UObject* Outer, const FString& Class, const NodeForgeGraphDocument::FValueMap& Properties, const FVector2D& Position, FString& Error);
FObject Properties(UEdGraphNode* Node);
bool Verify(UEdGraphNode* Node, const NodeForgeGraphDocument::FValueMap& Expected, FString& Error);
}
