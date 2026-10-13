#pragma once
#include "CoreMinimal.h"
#include "NodeForgePanelState.h"
#include "NodeForgeEditorBusinessGate.h"

// One GT-owned product state. Review instances are separate; I/O receives JSON copies only.
struct FNodeForgeEditorState
{
    FNodeForgePanelState Input;
    bool bBusy = false;
    FString ResultText;
    FString ResultNotice;
    FString LastReportPath;
    FString LastAssetPath;
    FString LastImportInputPath;
    FString LastImportInputDigest;
    FString LastSuccessReportPath;
    FString LastSuccessAssetPath;
    FString LastReadOnlyData;
    FString LastExportReportPath;
    bool bResultInputBound = false;
    bool bResultRelayout = false;
    uint64 ResultRevision = 0;
};

class FNodeForgeEditorControlService : public TSharedFromThis<FNodeForgeEditorControlService>
{
public:
    FNodeForgeEditorState& MutableState() { check(IsInGameThread()); return Current; }
    const FNodeForgeEditorState& State() const { check(IsInGameThread()); return Current; }
    bool EditFromUI(const FString& Text);
    bool ReplaceDraft(const FString& Text, const FString& Origin, const FString& SourcePath,
        const FString& SourceDigest, FString& Error);
    bool ReplaceFromUIWhileBusy(const FString& Text, const FString& Origin, const FString& SourcePath,
        const FString& SourceDigest, FString& Error);
    bool CommitExportFromUI(const FString& Text, const FString& ReportPath, const FString& ReportSha256,
        uint64 ExpectedRevision, FString& Error);
    bool LoadFile(const FString& Path, FString& Error);
    bool RestoreDraft(FString& Error);
    bool TryBeginBusiness();
    void EndBusiness();
    bool IsBusy() const { return Gate->IsBusy(); }
    TSharedRef<FNodeForgeEditorBusinessGate> BusinessGate() const { return Gate; }
    // Called only by the module's drain after the session-owned registry reserved this gate.
    TSharedRef<FJsonObject> ExecuteDraftCall(const FString& Method, const TSharedRef<FJsonObject>& Arguments, uint64 ExpectedRevision);
    TSharedRef<FJsonObject> ExecuteGraphCall(const FString& Method, uint64 ExpectedRevision,
        const FString& Session, const FString& CallId, const FString& ReportDirectory);
    TSharedRef<FJsonObject> ExecuteQueryCall(const FString& Method, const TSharedRef<FJsonObject>& Arguments,
        uint64 ExpectedRevision, const FString& Session, const FString& CallId, const FString& ReportDirectory);
    int32 QueryFromUI(const FString& Method, const TSharedRef<FJsonObject>& Arguments);
    // Mode-specific projection shared by UI/API; keeps generation validation separate.
    static bool ValidateQueryReport(const FString& Mode, int32 WorkflowCode, const TSharedPtr<FJsonObject>& Native,
        int32& Code, FString& Error);
    // One read/report coordinator used by API and the UI selection adapters. Caller owns GT/gate.
    static int32 ExecuteReadAt(const FString& Method, const TSharedRef<FJsonObject>& Arguments,
        const FString& Snapshot, const FString& ReportPath, FString& QueryDigest, FString& Error);
#if WITH_DEV_AUTOMATION_TESTS
    void SetControlProbeForChecks(TFunction<void()> Probe) { check(IsInGameThread()); ControlProbe = MoveTemp(Probe); }
#endif
    bool ResultIsStale() const;
    TSharedRef<FJsonObject> GetState() const;
    TSharedRef<FJsonObject> GetDraft() const;
    bool SaveDraftSnapshot(FString& Path, FString& Hash, FString& Error) const;
    bool HasReport() const;
    void ResetResult(bool InputBound = false, bool Relayout = false);
    bool ReadCurrentReport(TSharedPtr<FJsonObject>& Report, FString* Original = nullptr);
    int32 PreflightFromUI();
    int32 PrepareCopyFromUI();
    int32 PreviewFromUI();
    // Optional synchronous presentation callback; never retained or serialized by the service.
    int32 GenerateFromUI(const TFunction<void()>& BeforeExecute = TFunction<void()>());
    int32 RelayoutFromUI(const TFunction<void()>& BeforeExecute = TFunction<void()>());
    void AttachPanel() { check(IsInGameThread()); ++PanelCount; }
    void DetachPanel() { check(IsInGameThread()); check(PanelCount > 0); --PanelCount; }
private:
    bool EditDraftText(const FString& Text);
    bool RetainOriginEditCore(const FString& Text, FString& Error);
    bool ReplaceDraftCore(const FString& Text, const FString& Origin, const FString& SourcePath, const FString& SourceDigest, FString& Error,
        const FString& ExportReportPath = FString(), const FString& ExportReportSha256 = FString());
    bool LoadFileCore(const FString& Path, FString& Error, const FString& ExportReportPath = FString(), const FString& ExportReportSha256 = FString());
    bool RestoreDraftCore(FString& Error);
    bool SaveDraftSnapshotAt(const FString& Path, FString& Hash, FString& Error) const;
    int32 PreflightSnapshot(const FString& Input, const FString& ReportPath, const TSharedPtr<FJsonObject>& Context);
    int32 PrepareSnapshot(const FString& Input, const FString& ReportPath, const TSharedPtr<FJsonObject>& Context,
        uint64 ExpectedRevision, TSharedPtr<FJsonObject>& Preparation);
    int32 PreviewSnapshot(const FString& Input, const FString& ReportPath, const TSharedPtr<FJsonObject>& Context);
    int32 QuerySnapshot(const FString& Method, const TSharedRef<FJsonObject>& Arguments,
        const FString& Snapshot, const FString& ReportPath, FString& QueryDigest, FString& Error);
    int32 ImportSnapshot(const FString& InputPath, const FString& Method, const FString& Mode,
        const FString& SavedDigest, const TFunction<void()>& BeforeExecute,
        const FString& OutputReport = FString(), const TSharedPtr<FJsonObject>& ContextOverride = nullptr);
    FNodeForgeEditorState Current;
    int32 PanelCount = 0;
    TSharedRef<FNodeForgeEditorBusinessGate> Gate = MakeShared<FNodeForgeEditorBusinessGate>();
#if WITH_DEV_AUTOMATION_TESTS
    TFunction<void()> ControlProbe; // GT-only synchronous native fault probe; never from RPC or graph input.
#endif
};

class FNodeForgeEditorBusinessScope
{
public:
    explicit FNodeForgeEditorBusinessScope(FNodeForgeEditorControlService& In) : Service(In), bOwns(In.TryBeginBusiness()) {}
    ~FNodeForgeEditorBusinessScope() { if (bOwns) Service.EndBusiness(); }
    explicit operator bool() const { return bOwns; }
    FNodeForgeEditorBusinessScope(const FNodeForgeEditorBusinessScope&) = delete;
private:
    FNodeForgeEditorControlService& Service; bool bOwns;
};
