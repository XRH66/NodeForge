#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

namespace NodeForgePrototype
{
    int32 Execute(const FString& Mode, const FString& InputPath, const FString& ReportPath, const FString& PreviousReport = FString(), const TSharedPtr<FJsonObject>& ReportContext = nullptr);
    bool ReadInputFile(const FString& Path, FString& Text, FString& InputDigest, FString& Error);
    bool WriteJson(const TSharedRef<FJsonObject>& Object, const FString& Path);
    FString FileSha256(const FString& Path);
    FString TextSha256(const FString& Text);
    FString WorkspaceDirectory();
    // Shared finite output boundary for Import, derived copies and native checks.
    bool ValidateTargetPath(const FString& PackageName, FString& Error);
    bool ValidateNewTargetIdentity(const FString& SourceObjectPath, const FString& RequestId, const FString& Target, FString& Error);
    bool NewCopyTarget(const FString& Source, FString& Target, FString& Error, int32 MaxLeafLength = 64);
    bool ValidateTargetFilePath(const FString& Filename, FString& Error);
}
