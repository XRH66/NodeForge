#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
namespace NodeForgeTaskRules
{
// Filters the actual catalog only; never discovers or constructs another node system.
bool Select(const TSharedPtr<FJsonObject>& Config, const TSharedRef<FJsonObject>& Catalog,
    const FString& FullDigest, TSharedPtr<FJsonObject>& Bundle, FString& Error);
}
