#pragma once
#include "CoreMinimal.h"
class SWidget;
class FNodeForgeEditorControlService;
class FNodeForgeEditorControlChannel;
struct FAssetData;

namespace NodeForgePanel
{
    TSharedPtr<FNodeForgeEditorControlService> GetProductService();
    TSharedPtr<FNodeForgeEditorControlChannel> GetControlChannel();
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
    // Read-only test locator shared by checks of the actual registered production tab.
    TSharedPtr<SWidget> FindTaggedForChecks(const TSharedRef<SWidget>& Root, FName Tag);
#endif
    // Shared by the real spawner and an isolated construction test. No import is executed here.
    TSharedRef<SWidget> Create();
    // Render-only review factory; the report is read through the existing Reports boundary.
    // It performs no import, clipboard action or asset edit. Production Create uses no report.
    TSharedRef<SWidget> CreateForReview(const FString& ReportPath);
    // Real content-browser selection dispatcher; reports only, never edits assets.
    // Negative return means no backend execution; nonnegative is Execute's exit code.
    int32 InspectSelection(const TArray<FAssetData>& Selection, const FString& ReportPath, FString& Error);
    int32 ExportSelection(const TArray<FAssetData>& Selection, const FString& ReportPath, FString& Error);
}
