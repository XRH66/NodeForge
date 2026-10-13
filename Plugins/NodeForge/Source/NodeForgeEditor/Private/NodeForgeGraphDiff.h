#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "NodeForgeGraphDocument.h"

namespace NodeForgeGraphDiff
{
// UE5.3 JSON equality folds string case. Keep native primitive equality, but
// compare string values and object keys exactly for declared request differences.
bool EqualValue(const TSharedPtr<FJsonValue>& A, const TSharedPtr<FJsonValue>& B);
// Requests reach this comparison as validated shared modern documents.
// No UObject, asset factory, compilation or persistence operations occur here.
TSharedRef<FJsonObject> Compare(const NodeForgeGraphDocument::FGraphDocument& Before, const NodeForgeGraphDocument::FGraphDocument& After);
}
