// Project-owned test-only guards, shared by the two saved export fixture tests.
#pragma once
#include "CoreMinimal.h"
#include "NodeForgePrototype.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"

namespace NodeForgeExportFixtureChecks
{
inline bool ValidateSetup(const FString& ReportPath,const TArray<const TCHAR*>& Names,
    const FString& Prefix,FString& Error)
{
    const FString Root=NodeForgePrototype::WorkspaceDirectory();
    FString Project=FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
    FString Report=FPaths::ConvertRelativePathToFull(ReportPath);
    if(!FPaths::CollapseRelativeDirectories(Project)
        || !FPaths::IsUnderDirectory(Project,Root/TEXT("ValidationProject/RegressionRuns")))
    {Error=TEXT("Export fixtures require an isolated RegressionRuns project");return false;}
    if(!FPaths::CollapseRelativeDirectories(Report)
        || !FPaths::IsUnderDirectory(Report,Root/TEXT("Reports"))
        || !FPaths::GetExtension(Report).Equals(TEXT("json"),ESearchCase::CaseSensitive)
        || IFileManager::Get().FileExists(*Report) || IFileManager::Get().DirectoryExists(*Report))
    {Error=TEXT("Export fixtures require a fresh JSON report inside Reports");return false;}
    // Check every target before creating the first package; a late collision must
    // not leave the earlier fixtures saved without a terminal report.
    for(const TCHAR* Name:Names)
    {
        const FString Target=TEXT("/Game/NodeForgeTests/")+Prefix+Name;
        const FString File=FPackageName::LongPackageNameToFilename(Target,FPackageName::GetAssetPackageExtension());
        if(FPackageName::DoesPackageExist(Target) || FindPackage(nullptr,*Target)
            || IFileManager::Get().FileExists(*File))
        {Error=TEXT("Export fixture target collision: ")+Target;return false;}
    }
    return true;
}
}
