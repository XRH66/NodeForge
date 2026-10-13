#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

// Presentation of existing evidence only; never validates graphs or repairs assets.
namespace NodeForgeResultView
{
    // Bounded panel summary (at most 1200 TCHARs); full evidence stays in Describe/Read.
    FString Compact(const TSharedRef<FJsonObject>& Report);
    FString Describe(const TSharedRef<FJsonObject>& Report);
    bool Read(const FString& Path, FString& OriginalJson, FString& Description);
}
