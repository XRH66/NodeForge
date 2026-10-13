#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

// Draft ownership only. Parsing, asset construction and validation stay in Prototype.
struct FNodeForgeDraft
{
    FString Text;
    FString OriginalText;
    FString Origin = TEXT("manual");
    FString SourcePath;
    FString SourceDigest;
    FString PreparationReport;
    TSharedPtr<FJsonObject> ExportOrigin; // Verified original report; retained only with this draft lineage.
    bool bEdited = false;
};

class FNodeForgePanelState
{
public:
    const FNodeForgeDraft& Draft() const { return Current; }
    uint64 Revision() const { return CurrentRevision; }
    bool CanRestore() const { return bHasPrevious; }
    bool Edit(const FString& Text);
    bool Replace(const FString& Text, const FString& Origin, const FString& SourcePath,
                 const FString& SourceDigest, FString& Error, const TSharedPtr<FJsonObject>& ExportOrigin = nullptr);
    bool Restore();
    bool PrepareCopy(const FString& Text, const FString& PreparationReport, FString& Error);
    FString DescribeSource() const;
    TSharedRef<FJsonObject> Provenance(const FString& SnapshotPath, const FString& SnapshotDigest) const;
    TSharedPtr<FJsonObject> ExportProvenance() const;
    static bool ValidateExternalText(const FString& Text, FString& Error);

private:
    FNodeForgeDraft Current;
    FNodeForgeDraft Previous;
    uint64 CurrentRevision = 0;
    bool bHasPrevious = false;
    bool bManualEditing = false; // One pre-edit snapshot per continuous editing segment.
};
