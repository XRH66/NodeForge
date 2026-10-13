#include "NodeForgeEditorControlService.h"
#include "NodeForgeEditorControlProtocol.h"
#include "NodeForgeControlOperations.h"
#include "NodeForgePrototype.h"
#include "NodeForgeResourceSource.h"
#include "NodeForgeGraphCodec.h"
#include "NodeForgeAssetDefinitions.h"
#include "NodeForgeNodeDefinitions.h"
#include "NodeForgeInstallation.h"
#include "NodeForgeResultView.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Serialization/Archive.h"
#include "Windows/WindowsHWrapper.h"
#include "Misc/ScopeExit.h"
#include "Modules/ModuleManager.h"
#include "HAL/PlatformProcess.h"

namespace
{
bool AdmitInputPath(const FString& Requested, FString& Path, FString& Error)
{
    const FString Root = NodeForgePrototype::WorkspaceDirectory();
    const FString StateRoot = NodeForgeInstallation::StateDirectory();
    if (Root.IsEmpty() || Requested.IsEmpty() || FPaths::IsRelative(Requested))
    { Error = TEXT("inputPathRejected"); return false; }
    Path = FPaths::ConvertRelativePathToFull(Requested);
    FPaths::NormalizeFilename(Path); FPaths::CollapseRelativeDirectories(Path);
    if (StateRoot.IsEmpty() || FPaths::IsUnderDirectory(Path, StateRoot / TEXT("Reports/PluginControlSessions")))
    { Error = TEXT("inputPathRejected"); return false; }
    if (!FPaths::IsUnderDirectory(Path, Root / TEXT("Examples"))
        && !FPaths::IsUnderDirectory(Path, Root / TEXT("Tests"))
        && !FPaths::IsUnderDirectory(Path, StateRoot / TEXT("Reports")))
    { Error = TEXT("inputPathRejected"); return false; }
    // Deny junctions/symlinks in the entire admitted path, including its parents.
    FString Parent = Path;
    while (!Parent.IsEmpty())
    {
        const DWORD Attributes = GetFileAttributesW(*Parent);
        if (Attributes != INVALID_FILE_ATTRIBUTES && (Attributes & FILE_ATTRIBUTE_REPARSE_POINT))
        { Error = TEXT("inputReparseRejected"); return false; }
        const FString Next = FPaths::GetPath(Parent);
        if (Next == Parent) break;
        Parent = Next;
    }
    return true;
}
bool ReadAdmittedInput(const FString& Path, FString& Text, FString& Hash, FString& Error)
{ return NodeForgeResourceSource::ReadText(Path, NodeForgeEditorControl::MaxGraphBytes, Text, Hash, Error); }
FString ReadMode(const FString& Method, const TSharedRef<FJsonObject>& Args)
{
    if(Method==TEXT("rules.query"))return TEXT("GraphRules");
    if(Method==TEXT("catalog.get"))return Args->Values.IsEmpty()?TEXT("GraphCatalog"):FString();
    if(Method==TEXT("resources.query"))return TEXT("ResourceQuery");
    FString Kind,Source,Id,Target;
    if(Method==TEXT("resources.inspect") && Args->Values.Num()==2
        && Args->TryGetStringField(TEXT("source"),Source) && !Source.IsEmpty() && Args->TryGetStringField(TEXT("kind"),Kind)
        && (Kind==TEXT("MaterialParameters") || Kind==TEXT("MaterialFunction") || Kind==TEXT("Texture2D")))return Kind==TEXT("Texture2D")?TEXT("TextureInspect"):Kind;
    int32 FormatVersion=2;FString VersionError;auto ExportConfig=MakeShared<FJsonObject>();ExportConfig->Values=Args->Values;ExportConfig->RemoveField(TEXT("assetKind"));
    if(Method==TEXT("graphs.export") && NodeForgeAssetDefinitions::ExportVersion(ExportConfig,FormatVersion,VersionError) && Args->TryGetStringField(TEXT("source"),Source)
        && Args->TryGetStringField(TEXT("assetKind"),Kind) && Args->TryGetStringField(TEXT("requestId"),Id)
        && Args->TryGetStringField(TEXT("assetPath"),Target))
        return Kind==TEXT("Material")?TEXT("MaterialGraphExport"):Kind==TEXT("Blueprint")?TEXT("BlueprintGraphExport"):FString();
    return FString();
}
bool IsCopyGraph(const FString& Text)
{
    NodeForgeGraphDocument::FGraphDocument Document;FString Error;
    return NodeForgeGraphCodec::DecodeText(Text,Document,Error)
        && (Document.AssetType()==TEXT("Material") || Document.AssetType()==TEXT("Blueprint"));
}

}

bool FNodeForgeEditorControlService::EditFromUI(const FString& Text)
{
    check(IsInGameThread());
    FNodeForgeEditorBusinessScope Lease(*this); if (!Lease) return false;
    return EditDraftText(Text);
}
bool FNodeForgeEditorControlService::EditDraftText(const FString& Text)
{
    const bool Changed = Current.Input.Edit(Text);
    if (Changed && Current.bResultInputBound)
        Current.ResultNotice = TEXT("输入已更新；上次检查、预览和生成结果不代表当前输入，请重新执行。");
    return Changed;
}
bool FNodeForgeEditorControlService::RetainOriginEditCore(const FString& Text, FString& Error)
{
    check(IsInGameThread()); check(Current.bBusy);
    if (!FNodeForgePanelState::ValidateExternalText(Text, Error)) return false;
    const auto Ref = Current.Input.Draft().ExportOrigin;
    FString Path, Hash;
    if (!Ref || !Ref->TryGetStringField(TEXT("reportPath"), Path)
        || !Ref->TryGetStringField(TEXT("reportSha256"), Hash))
    { Error = TEXT("exportOriginRequired"); return false; }
    TSharedPtr<FJsonObject> Verified;
    if (!NodeForgeResourceSource::ReadExportOrigin(Path, Hash, Text, false, Verified, Error)) return false;
    NodeForgeResourceSource::FCapture Capture;
    const int32 ReadRevision=NodeForgeNodeDefinitions::Capabilities(Verified->GetStringField(TEXT("catalogDigest"))).ResourceReadRevision;
    if (!Capture.Begin(Verified->GetStringField(TEXT("source")), Verified->GetStringField(TEXT("assetKind")), Error, ReadRevision)) return false;
    if (Capture.Sha256() != Verified->GetStringField(TEXT("sourceAssetSha256")))
    { Error = TEXT("exportSourceHashChanged"); return false; }
    if (!Capture.Verify(Error) || NodeForgePrototype::FileSha256(Path) != Hash)
    { if (Error.IsEmpty()) Error = TEXT("exportReportChangedDuringExecution"); return false; }
    EditDraftText(Text); // Same text succeeds without a revision or a new restore point.
    return true;
}
bool FNodeForgeEditorControlService::ReplaceDraft(const FString& Text, const FString& Origin,
    const FString& SourcePath, const FString& SourceDigest, FString& Error)
{
    check(IsInGameThread());
    FNodeForgeEditorBusinessScope Lease(*this); if (!Lease) { Error = TEXT("busy"); return false; }
    return ReplaceDraftCore(Text, Origin, SourcePath, SourceDigest, Error);
}
bool FNodeForgeEditorControlService::ReplaceDraftCore(const FString& Text, const FString& Origin,
    const FString& SourcePath, const FString& SourceDigest, FString& Error,
    const FString& ExportReportPath, const FString& ExportReportSha256)
{
    TSharedPtr<FJsonObject> ExportOrigin;
    if(ExportReportPath.IsEmpty()!=ExportReportSha256.IsEmpty()) {Error=TEXT("invalidExportReference");return false;}
    if(!ExportReportPath.IsEmpty() && !NodeForgeResourceSource::ReadExportOrigin(ExportReportPath,ExportReportSha256,Text,true,ExportOrigin,Error))return false;
    if(!Current.Input.Replace(Text,Origin,SourcePath,SourceDigest,Error,ExportOrigin))return false;
    Current.LastExportReportPath=ExportOrigin?ExportOrigin->GetStringField(TEXT("reportPath")):FString();
    return true;
}
bool FNodeForgeEditorControlService::ReplaceFromUIWhileBusy(const FString& Text, const FString& Origin,
    const FString& SourcePath, const FString& SourceDigest, FString& Error)
{
    check(IsInGameThread());
    if(!Current.bBusy || !Gate->IsUIRunning()) {Error=TEXT("busy");return false;}
    return ReplaceDraftCore(Text,Origin,SourcePath,SourceDigest,Error);
}
bool FNodeForgeEditorControlService::CommitExportFromUI(const FString& Text, const FString& ReportPath,
    const FString& ReportSha256, uint64 ExpectedRevision, FString& Error)
{
    check(IsInGameThread());
    if(!Current.bBusy || !Gate->IsUIRunning()) {Error=TEXT("busy");return false;}
    if(Current.Input.Revision()!=ExpectedRevision) {Error=TEXT("staleRevision");return false;}
    return ReplaceDraftCore(Text,TEXT("export"),ReportPath,ReportSha256,Error,ReportPath,ReportSha256);
}

bool FNodeForgeEditorControlService::LoadFile(const FString& Requested, FString& Error)
{
    check(IsInGameThread());
    FNodeForgeEditorBusinessScope Lease(*this); if (!Lease) { Error = TEXT("busy"); return false; }
    return LoadFileCore(Requested, Error);
}
bool FNodeForgeEditorControlService::LoadFileCore(const FString& Requested, FString& Error, const FString& ExportReportPath, const FString& ExportReportSha256)
{
    FString Path, Text, Hash;
    if (!AdmitInputPath(Requested, Path, Error)
        || !ReadAdmittedInput(Path, Text, Hash, Error)) return false;
    return ReplaceDraftCore(Text, TEXT("file"), Path, Hash, Error, ExportReportPath, ExportReportSha256);
}
bool FNodeForgeEditorControlService::RestoreDraft(FString& Error)
{
    check(IsInGameThread()); Error.Empty();
    FNodeForgeEditorBusinessScope Lease(*this); if (!Lease) { Error = TEXT("busy"); return false; }
    return RestoreDraftCore(Error);
}
bool FNodeForgeEditorControlService::RestoreDraftCore(FString& Error)
{
    if (!Current.Input.Restore()) { Error = TEXT("noRestorePoint"); return false; }
    Current.LastExportReportPath=Current.Input.Draft().ExportOrigin?Current.Input.Draft().ExportOrigin->GetStringField(TEXT("reportPath")):FString();
    Current.ResultNotice = TEXT("已恢复上次输入；请重新检查或预览。");
    return true;
}
bool FNodeForgeEditorControlService::TryBeginBusiness()
{
    check(IsInGameThread());
    if (!Gate->TryUI()) return false;
    Current.bBusy = true; return true;
}
void FNodeForgeEditorControlService::EndBusiness()
{
    check(IsInGameThread()); check(Current.bBusy); Current.bBusy = false; Gate->EndUI();
}
bool FNodeForgeEditorControlService::ResultIsStale() const
{
    check(IsInGameThread());
    return Current.bResultInputBound && Current.ResultRevision != Current.Input.Revision();
}
TSharedRef<FJsonObject> FNodeForgeEditorControlService::GetDraft() const
{
    check(IsInGameThread());
    auto Result = MakeShared<FJsonObject>();
    const FNodeForgeDraft& Draft = Current.Input.Draft();
    FString Error; const bool Readable = Draft.Text.TrimStartAndEnd().IsEmpty()
        ? FTCHARToUTF8(*Draft.Text).Length() <= 65536
        : FNodeForgePanelState::ValidateExternalText(Draft.Text, Error);
    Result->SetStringField(TEXT("draftRevision"), LexToString(Current.Input.Revision()));
    Result->SetStringField(TEXT("digest"), NodeForgePrototype::TextSha256(Draft.Text));
    Result->SetNumberField(TEXT("bytes"), FTCHARToUTF8(*Draft.Text).Length());
    Result->SetBoolField(TEXT("draftReadable"), Readable);
    if (Readable) Result->SetStringField(TEXT("text"), Draft.Text);
    Result->SetStringField(TEXT("origin"), Draft.Origin);
    Result->SetStringField(TEXT("sourcePath"), Draft.SourcePath);
    Result->SetStringField(TEXT("sourceOriginalDigest"), Draft.SourceDigest);
    Result->SetStringField(TEXT("copyPreparationReport"), Draft.PreparationReport);
    if(Draft.ExportOrigin)Result->SetObjectField(TEXT("exportOrigin"),Current.Input.ExportProvenance());
    Result->SetBoolField(TEXT("sourceEdited"), Draft.bEdited);
    Result->SetBoolField(TEXT("canRestore"), Current.Input.CanRestore());
    return Result;
}

TSharedRef<FJsonObject> FNodeForgeEditorControlService::ExecuteGraphCall(const FString& Method, uint64 ExpectedRevision,
    const FString& Session, const FString& CallId, const FString& ReportDirectory)
{
    check(IsInGameThread()); auto Result = MakeShared<FJsonObject>(); int32 Code = 2; FString Reason;
    const auto* Definition = NodeForgeControlOperations::Find(Method, NodeForgeControlOperations::EHost::ActiveEditor);
    const FString Mode = Definition && Definition->Mode ? Definition->Mode : TEXT("");
    const bool Preparing = Method == TEXT("draft.prepareCopy");
    const bool Previewing = Method == TEXT("graphs.preview");
    if ((Method != TEXT("graphs.preflight") && Method != TEXT("graphs.generate") && !Preparing && !Previewing) || Mode.IsEmpty()) Reason = TEXT("operationUnavailable");
    else if (Current.Input.Revision() != ExpectedRevision) Reason = TEXT("staleRevision");
    else if (Preparing && !IsCopyGraph(Current.Input.Draft().Text)) Reason = TEXT("copyGraphRequired");
    else if (Previewing && !Current.Input.Draft().ExportOrigin) Reason = TEXT("exportOriginRequired");
    else if (!Gate->BeginControl()) Reason = TEXT("busy");
    else
    {
        TGuardValue<bool> Running(Current.bBusy, true); ResetResult(true);
        FString Snapshot = ReportDirectory / TEXT("Calls") / (CallId + TEXT("Input.json"));
        const FString NativePath = ReportDirectory / TEXT("Calls") / (CallId + TEXT("Native.json"));
        const FString SidecarPath = ReportDirectory / TEXT("Calls") / (CallId + TEXT("Control.json"));
        FString Admitted, Hash, Error;
        bool SessionValid = Session.Len() == 32;
        for (TCHAR C : Session) SessionValid &= (C >= '0' && C <= '9') || (C >= 'a' && C <= 'f');
        if (!SessionValid || !NodeForgeEditorControl::IsCallId(CallId)
            || !FPaths::IsUnderDirectory(ReportDirectory, NodeForgeInstallation::StateDirectory() / TEXT("Reports/PluginControlStageTwo"))
            || IFileManager::Get().FileExists(*NativePath) || IFileManager::Get().FileExists(*SidecarPath)
            || !AdmitInputPath(Snapshot, Admitted, Error) || !SaveDraftSnapshotAt(Snapshot, Hash, Error))
        { Reason = TEXT("snapshotRejected"); Current.ResultText = TEXT("本次程序调用未执行：草稿或安全快照路径无法核实。"); }
        else
        {
            // Execute's existing context whitelist is unchanged. Program provenance is
            // an owned sidecar; never synthesize uiInput or call a Slate presentation callback.
            auto Context = MakeShared<FJsonObject>(); Context->SetStringField(TEXT("inputMethod"), TEXT("editorProgram"));
            TSharedPtr<FJsonObject> Preparation;
            Code = Preparing ? PrepareSnapshot(Snapshot, NativePath, Context, ExpectedRevision, Preparation)
                : Previewing ? PreviewSnapshot(Snapshot, NativePath, Context)
                : Method == TEXT("graphs.preflight") ? PreflightSnapshot(Snapshot, NativePath, Context)
                : ImportSnapshot(Snapshot, TEXT("editorProgram"), Mode, Hash, TFunction<void()>(), NativePath, Context);
            if (Preparing)
            {
                Result->SetBoolField(TEXT("draftApplied"), Code == 0);
                Result->SetStringField(TEXT("previousDraftRevision"), LexToString(ExpectedRevision));
                Result->SetStringField(TEXT("newDraftRevision"), LexToString(Current.Input.Revision()));
                if (Preparation) Result->SetObjectField(TEXT("copyPreparation"), Preparation);
            }
            Result->SetNumberField(TEXT("workflowExitCode"), Code);
            if (Code != 0 && Code != 2 && Code != 3 && Code != 4 && Code != 5) Code = 4;
            TSharedPtr<FJsonObject> Native; FString Raw;
            if (!ReadCurrentReport(Native, &Raw) || FTCHARToUTF8(*Raw).Length() > NodeForgeEditorControl::MaxReportBytes)
            { Code = 5; Reason = TEXT("nativeReportUnknown"); }
            else
            {
                FString NativeSession, RequestId, InputDigest, Overall, NativeProject, NativeMode, NativeInputMethod;
                double NativePid = 0;
                const bool HasDigest = Native->TryGetStringField(TEXT("inputDigest"), InputDigest);
                const bool HasOverall = Native->TryGetStringField(TEXT("overallStatus"), Overall);
                // Legacy Execute uses 1 for explicit validation rejection. Preserve the
                // workflow code but project its closed, bound domain receipt onto Control's codes.
                if (Code == 4 && HasOverall && Overall == TEXT("rejected")) Code = 2;
                const FString NativeHash = NodeForgePrototype::FileSha256(NativePath);
                const bool SuccessStatusMatches = Code != 0 || Overall == TEXT("succeeded")
                    || (Method == TEXT("graphs.generate") && Overall == TEXT("waitingPersistence"));
                const bool Bound = Native->TryGetStringField(TEXT("sessionId"), NativeSession) && !NativeSession.IsEmpty()
                    && HasOverall
                    && SuccessStatusMatches && NativeHash.Len() == 64
                    && Native->TryGetStringField(TEXT("mode"), NativeMode) && NativeMode == Mode
                    && (HasDigest ? InputDigest == Hash : Code == 2 && Overall == TEXT("rejected"))
                    && Native->TryGetNumberField(TEXT("pid"), NativePid) && NativePid == FPlatformProcess::GetCurrentProcessId()
                    && Native->TryGetStringField(TEXT("project"), NativeProject)
                    && FPaths::ConvertRelativePathToFull(NativeProject).Equals(FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()), ESearchCase::IgnoreCase)
                    && Native->TryGetStringField(TEXT("inputMethod"), NativeInputMethod) && NativeInputMethod == TEXT("editorProgram")
                    && !Native->HasField(TEXT("uiInput"));
                Native->TryGetStringField(TEXT("requestId"), RequestId);
                if (!Bound) { Code = 5; Reason = TEXT("nativeIdentityUnknown"); }
                else
                {
                    auto Reference = MakeShared<FJsonObject>(); Reference->SetStringField(TEXT("path"), NativePath);
                    Reference->SetStringField(TEXT("sha256"), NativeHash);
                    Reference->SetNumberField(TEXT("bytes"), FTCHARToUTF8(*Raw).Length());
                    Reference->SetStringField(TEXT("nativeSessionId"), NativeSession); Reference->SetStringField(TEXT("requestId"), RequestId);
                    Reference->SetStringField(TEXT("overallStatus"), Overall);
                    auto Sidecar = MakeShared<FJsonObject>(); Sidecar->SetStringField(TEXT("host"), TEXT("activeEditor"));
                    Sidecar->SetStringField(TEXT("inputMethod"), TEXT("editorProgram")); Sidecar->SetStringField(TEXT("controlSessionId"), Session);
                    Sidecar->SetStringField(TEXT("callId"), CallId); Sidecar->SetStringField(TEXT("operation"), Method);
                    Sidecar->SetStringField(TEXT("draftRevision"), LexToString(ExpectedRevision));
                    Sidecar->SetStringField(TEXT("newDraftRevision"), LexToString(Current.Input.Revision()));
                    if (Preparing) Sidecar->SetBoolField(TEXT("draftApplied"), Result->GetBoolField(TEXT("draftApplied")));
                    Sidecar->SetStringField(TEXT("snapshotPath"), Snapshot); Sidecar->SetStringField(TEXT("snapshotDigest"), Hash);
                    Sidecar->SetStringField(TEXT("binarySha256"), NodeForgePrototype::FileSha256(FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"))));
                    Sidecar->SetObjectField(TEXT("nativeReport"), Reference);
                    auto Draft = GetDraft(); Draft->RemoveField(TEXT("text")); Sidecar->SetObjectField(TEXT("draft"), Draft);
                    const bool SidecarWritten = NodeForgePrototype::WriteJson(Sidecar, SidecarPath);
                    const FString SidecarHash = SidecarWritten ? NodeForgePrototype::FileSha256(SidecarPath) : FString();
                    if (SidecarHash.Len() != 64) { Code = 5; Reason = TEXT("controlEvidenceUnknown"); }
                    else
                    {
                        Result->SetObjectField(TEXT("nativeReport"), Reference); Result->SetStringField(TEXT("controlEvidencePath"), SidecarPath);
                        Result->SetStringField(TEXT("controlEvidenceSha256"), SidecarHash);
                        Result->SetStringField(TEXT("inputSnapshot"), Snapshot); Result->SetStringField(TEXT("inputDigest"), Hash);
                        Reason = Code == 0 ? TEXT("succeeded") : Code == 2 ? TEXT("nativeRejected") : TEXT("nativeFailed");
                    }
                }
            }
        }
    }
    Result->SetStringField(TEXT("state"), Code == 0 ? TEXT("completed") : Code == 2 ? TEXT("rejected") : Code == 5 ? TEXT("unknown") : TEXT("failed"));
    Result->SetStringField(TEXT("code"), Reason); Result->SetNumberField(TEXT("cliExitCode"), Code);
    auto Metadata = GetDraft(); Metadata->RemoveField(TEXT("text")); Result->SetObjectField(TEXT("draft"), Metadata);
    return Result;
}
bool FNodeForgeEditorControlService::ValidateQueryReport(const FString& Mode, int32 WorkflowCode,
    const TSharedPtr<FJsonObject>& Native, int32& Code, FString& Error)
{
    Code = 5; Error = TEXT("queryReportUnknown");
    FString NativeMode, Project, Session, Status, Version, Digest, Overall;
    double Pid = 0;
    if (!Native || !Native->TryGetStringField(TEXT("mode"), NativeMode) || NativeMode != Mode
        || !Native->TryGetStringField(TEXT("project"), Project)
        || !FPaths::ConvertRelativePathToFull(Project).Equals(
            FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()), ESearchCase::IgnoreCase)
        || !Native->TryGetNumberField(TEXT("pid"), Pid) || Pid != FPlatformProcess::GetCurrentProcessId()
        || !Native->TryGetStringField(TEXT("sessionId"), Session) || Session.IsEmpty()) return false;
    if (WorkflowCode != 0)
    {
        FString Diagnosis;
        if ((WorkflowCode != 1 && WorkflowCode != 2 && WorkflowCode != 4)
            || !Native->TryGetStringField(TEXT("overallStatus"), Overall)
            || !Native->TryGetStringField(TEXT("nextAction"), Diagnosis) || Diagnosis.IsEmpty()
            || (Overall != TEXT("rejected") && Overall != TEXT("failed"))) return false;
        Code = Overall == TEXT("rejected") ? 2 : 4;
        Error = Code == 2 ? TEXT("nativeRejected") : TEXT("nativeFailed"); return true;
    }
    if (!Native->TryGetStringField(TEXT("operationStatus"), Status) || Status != TEXT("passed")
        || !Native->TryGetStringField(TEXT("reportVersion"), Version)) return false;
    if(Mode!=TEXT("TextureInspect") && Mode!=TEXT("MaterialParameters") && Mode!=TEXT("MaterialFunction"))
    {
        if(!Native->TryGetStringField(TEXT("catalogDigest"),Digest) || Digest.Len()!=64)return false;
        for(TCHAR C:Digest)if(!((C>='0' && C<='9') || (C>='a' && C<='f')))return false;
    }
    const TSharedPtr<FJsonObject>* Payload = nullptr;
    if (Mode == TEXT("GraphRules"))
    {
        FString Format, InputFormat, BundleDigest; double BundleVersion = 0, InputVersion = 0, Count = 0;
        if (Version != TEXT("NodeForgeTaskRules1")
            || !Native->TryGetStringField(TEXT("overallStatus"), Overall) || Overall != TEXT("succeeded")
            || !Native->TryGetObjectField(TEXT("taskRules"), Payload) || !Payload || !Payload->IsValid()
            || !(*Payload)->TryGetStringField(TEXT("format"), Format) || Format != TEXT("NodeForgeTaskRules")
            || !(*Payload)->TryGetNumberField(TEXT("version"), BundleVersion) || BundleVersion != 1
            || !(*Payload)->TryGetStringField(TEXT("inputFormat"), InputFormat) || InputFormat != TEXT("NodeForgeGraph")
            || !(*Payload)->TryGetNumberField(TEXT("inputVersion"), InputVersion) || InputVersion != 3
            || !(*Payload)->TryGetStringField(TEXT("catalogDigest"), BundleDigest) || BundleDigest != Digest
            || !(*Payload)->TryGetNumberField(TEXT("matchedNodeCount"), Count) || !FMath::IsFinite(Count)
            || Count < 1 || FMath::FloorToDouble(Count) != Count
            || !(*Payload)->HasTypedField<EJson::Object>(TEXT("capabilities"))) return false;
        const TArray<TSharedPtr<FJsonValue>>* Nodes = nullptr;
        if (!(*Payload)->GetObjectField(TEXT("capabilities"))->TryGetArrayField(TEXT("nodes"), Nodes)
            || !Nodes || Nodes->Num() != Count) return false;
    }
    else if (Mode == TEXT("GraphCatalog"))
    {
        if (Version != TEXT("NodeForgeActualCatalog2") || Native->HasField(TEXT("overallStatus"))
            || !Native->TryGetObjectField(TEXT("catalog"), Payload) || !Payload || !Payload->IsValid()
            || !(*Payload)->HasTypedField<EJson::Object>(TEXT("material"))
            || !(*Payload)->HasTypedField<EJson::Object>(TEXT("blueprint"))) return false;
    }
    else if(Mode==TEXT("ResourceQuery"))
    {
        NodeForgeInstallation::FBinding Binding;FString BindingError,ActualCatalog;
        bool Complete=false,Scanning=false,Truncated=false;double Entries=0,Directories=0,Returned=0;FString Next,Hash,CatalogHash,BindingHash;
        const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
        if(Version!=TEXT("ResultContractDraft1") || !Native->TryGetStringField(TEXT("overallStatus"),Overall) || Overall!=TEXT("succeeded")
            || !Native->TryGetObjectField(TEXT("resourceQuery"),Payload)
            || !NodeForgeInstallation::Read(Binding,BindingError) || !NodeForgeInstallation::CatalogIdentity(ActualCatalog,BindingError) || ActualCatalog!=Digest
            || !(*Payload)->TryGetStringField(TEXT("catalogDigest"),CatalogHash) || CatalogHash!=Digest
            || !(*Payload)->TryGetStringField(TEXT("binarySha256"),Hash) || Hash!=Binding.BinarySha256
            || !(*Payload)->TryGetStringField(TEXT("installationBindingDigest"),BindingHash) || BindingHash!=Binding.Digest
            || !(*Payload)->TryGetBoolField(TEXT("complete"),Complete) || !(*Payload)->TryGetBoolField(TEXT("registryScanning"),Scanning)
            || !(*Payload)->TryGetBoolField(TEXT("truncated"),Truncated) || !(*Payload)->TryGetStringField(TEXT("nextAfter"),Next)
            || (!Complete && (!Truncated || !Next.IsEmpty())) || (Scanning && Complete)
            || !(*Payload)->TryGetNumberField(TEXT("entriesVisited"),Entries) || !FMath::IsFinite(Entries)
            || FMath::FloorToDouble(Entries)!=Entries || Entries<0 || Entries>NodeForgeResourceSource::QueryMaximumEntries
            || !(*Payload)->TryGetNumberField(TEXT("directoriesVisited"),Directories) || !FMath::IsFinite(Directories)
            || FMath::FloorToDouble(Directories)!=Directories || Directories<0 || Directories>NodeForgeResourceSource::QueryMaximumDirectories
            || !(*Payload)->TryGetNumberField(TEXT("returned"),Returned)
            || !(*Payload)->TryGetArrayField(TEXT("rows"),Rows) || Returned!=Rows->Num()
            || !(*Payload)->HasTypedField<EJson::Object>(TEXT("filter")))return false;
        TSharedPtr<FJsonObject> Filter;FString FilterError;
        if(!NodeForgeResourceSource::EffectiveQueryArguments((*Payload)->GetObjectField(TEXT("filter")),Filter,FilterError)
            || NodeForgeEditorControl::SemanticDigest(Filter.ToSharedRef())!=NodeForgeEditorControl::SemanticDigest((*Payload)->GetObjectField(TEXT("filter")).ToSharedRef())
            || Rows->Num()>Filter->GetNumberField(TEXT("limit")) || (Complete && Truncated && Rows->IsEmpty())
            || (Complete && !Truncated && !Next.IsEmpty()))return false;
        FString Previous=Filter->GetStringField(TEXT("after"));
        for(const auto& Value:*Rows)
        {
            const TSharedPtr<FJsonObject>* Row=nullptr;bool Inspected=true,Required=false,Discovered=false,Allowed=false;FString Export;
            if(!Value->TryGetObject(Row) || !(*Row)->TryGetBoolField(TEXT("inspected"),Inspected) || Inspected
                || !(*Row)->TryGetBoolField(TEXT("requiresInspect"),Required) || !Required
                || !(*Row)->TryGetBoolField(TEXT("discovered"),Discovered) || !Discovered
                || !(*Row)->TryGetBoolField(TEXT("policyAllowed"),Allowed) || !Allowed
                || !(*Row)->TryGetStringField(TEXT("graphExportable"),Export) || Export!=TEXT("unknown"))return false;
            for(const TCHAR* Key:{TEXT("objectPath"),TEXT("packagePath"),TEXT("class"),TEXT("name")})if(!(*Row)->HasTypedField<EJson::String>(Key))return false;
            const FString Path=(*Row)->GetStringField(TEXT("objectPath"));
            if(Path.Compare(Previous,ESearchCase::CaseSensitive)<=0)return false;
            Previous=Path;
        }
        if(Complete && Truncated && Next!=Previous)return false;
    }
    else if(Mode==TEXT("TextureInspect") || Mode==TEXT("MaterialParameters") || Mode==TEXT("MaterialFunction") || Mode==TEXT("MaterialGraphExport") || Mode==TEXT("BlueprintGraphExport"))
    {
        const TSharedPtr<FJsonObject>* Read=nullptr;const TSharedPtr<FJsonObject>* Fingerprint=nullptr;
        FString Source,Hash,BinaryHash,Proof,Kind,ConfigurationDigest;bool Held=false;
        if(Version!=TEXT("ResultContractDraft1") || !Native->TryGetStringField(TEXT("overallStatus"),Overall) || Overall!=TEXT("succeeded")
            || !Native->TryGetObjectField(TEXT("resourceRead"),Read) || !(*Read)->TryGetStringField(TEXT("source"),Source)
            || !(*Read)->TryGetStringField(TEXT("savedSourceSha256"),Hash) || Hash.Len()!=64
            || !(*Read)->TryGetStringField(TEXT("configurationDigest"),ConfigurationDigest) || ConfigurationDigest.Len()!=64
            || !(*Read)->TryGetStringField(TEXT("kind"),Kind) || Kind!=(Mode==TEXT("TextureInspect")?TEXT("Texture2D"):Mode==TEXT("MaterialFunction")?TEXT("MaterialFunction"):Mode==TEXT("BlueprintGraphExport")?TEXT("Blueprint"):TEXT("Material"))
            || !(*Read)->TryGetBoolField(TEXT("sourceFileHeldReadOnly"),Held) || !Held
            || !(*Read)->TryGetStringField(TEXT("originProof"),Proof) || Proof!=TEXT("controlledSavedLoad")
            || !Native->TryGetObjectField(TEXT("readOnlyExecutionFingerprint"),Fingerprint)
            || !(*Fingerprint)->TryGetStringField(TEXT("binarySha256"),BinaryHash)
            || BinaryHash!=NodeForgePrototype::FileSha256(FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"))))return false;
        if(Mode==TEXT("TextureInspect"))
        {
            FString TextureSource,TextureHash,TextureClass,Sampler,PropertySource,PropertySampler;bool Virtual=true;
            const TSharedPtr<FJsonObject>* Properties=nullptr;
            if(!Native->TryGetObjectField(TEXT("texture"),Payload) || !(*Payload)->TryGetStringField(TEXT("source"),TextureSource) || TextureSource!=Source
                || !(*Payload)->TryGetStringField(TEXT("savedSourceSha256"),TextureHash) || TextureHash!=Hash
                || !(*Payload)->TryGetStringField(TEXT("class"),TextureClass) || TextureClass!=TEXT("/Script/Engine.Texture2D")
                || !(*Payload)->TryGetBoolField(TEXT("virtualTextureStreaming"),Virtual) || Virtual
                || !(*Payload)->TryGetStringField(TEXT("nativeSamplerType"),Sampler)
                || !(*Payload)->TryGetObjectField(TEXT("properties"),Properties)
                || !(*Properties)->TryGetStringField(TEXT("Texture"),PropertySource) || PropertySource!=Source
                || !(*Properties)->TryGetStringField(TEXT("SamplerType"),PropertySampler) || PropertySampler!=Sampler)return false;
            for(const TCHAR* Key:{TEXT("width"),TEXT("height"),TEXT("compressionSettings")})if(!(*Payload)->HasTypedField<EJson::Number>(Key))return false;
            if(!(*Payload)->HasTypedField<EJson::Boolean>(TEXT("sRGB")) || !(*Payload)->HasTypedField<EJson::String>(TEXT("sampleQualification")))return false;
        }
        else if(Mode==TEXT("MaterialParameters"))
        {
            if(!Native->TryGetObjectField(TEXT("parameters"),Payload) || !(*Payload)->HasTypedField<EJson::String>(TEXT("parent"))
                || (*Payload)->GetStringField(TEXT("parent"))!=Source || !(*Payload)->HasTypedField<EJson::String>(TEXT("parentAssetSha256"))
                || (*Payload)->GetStringField(TEXT("parentAssetSha256"))!=Hash)return false;
            for(const TCHAR* Key:{TEXT("scalarParameters"),TEXT("vectorParameters"),TEXT("textureParameters")})
                if(!(*Payload)->HasTypedField<EJson::Object>(Key))return false;
            if(!(*Payload)->HasTypedField<EJson::Array>(TEXT("textureSamplerSignatures")))return false;
        }
        else if(Mode==TEXT("MaterialFunction"))
        {
            if(!Native->TryGetObjectField(TEXT("functionSignature"),Payload) || !(*Payload)->HasTypedField<EJson::Object>(TEXT("properties"))
                || !(*Payload)->HasTypedField<EJson::Array>(TEXT("inputs")) || !(*Payload)->HasTypedField<EJson::Array>(TEXT("outputs"))
                || !Native->HasTypedField<EJson::Object>(TEXT("nodeTemplate")))return false;
            const auto Properties=(*Payload)->GetObjectField(TEXT("properties"));
            if(!Properties->HasTypedField<EJson::String>(TEXT("MaterialFunction")) || Properties->GetStringField(TEXT("MaterialFunction"))!=Source
                || !Properties->HasTypedField<EJson::String>(TEXT("FunctionAssetSha256")) || Properties->GetStringField(TEXT("FunctionAssetSha256"))!=Hash)return false;
            const auto Template=Native->GetObjectField(TEXT("nodeTemplate"));FString Class,Id;
            const TSharedPtr<FJsonObject>* TemplateProperties=nullptr;
            if(!Template->TryGetStringField(TEXT("id"),Id) || Id.IsEmpty()
                || !Template->TryGetStringField(TEXT("class"),Class) || Class!=TEXT("/Script/Engine.MaterialExpressionMaterialFunctionCall")
                || !Template->TryGetObjectField(TEXT("properties"),TemplateProperties)
                || NodeForgeEditorControl::SemanticDigest((*TemplateProperties).ToSharedRef())!=NodeForgeEditorControl::SemanticDigest(Properties.ToSharedRef())
                || !(*Payload)->HasTypedField<EJson::String>(TEXT("defaultPolicy")) || !(*Payload)->HasTypedField<EJson::String>(TEXT("hashScope")))return false;
            const auto& Inputs=(*Payload)->GetArrayField(TEXT("inputs"));const auto& Outputs=(*Payload)->GetArrayField(TEXT("outputs"));
            if(Inputs.Num()>64 || Outputs.Num()<1 || Outputs.Num()>64)return false;
            for(const auto& Value:Inputs)
            {
                if(!Value || Value->Type!=EJson::Object)return false;const auto Row=Value->AsObject();
                if(!Row->HasTypedField<EJson::String>(TEXT("name")) || !Row->HasTypedField<EJson::String>(TEXT("guid"))
                    || !Row->HasTypedField<EJson::Number>(TEXT("nativeType")) || !Row->HasTypedField<EJson::Boolean>(TEXT("required"))
                    || !Row->HasTypedField<EJson::Boolean>(TEXT("usesNativePreviewDefault")) || !Row->HasTypedField<EJson::Array>(TEXT("nativePreviewValue")))return false;
                const auto& Preview=Row->GetArrayField(TEXT("nativePreviewValue"));if(Preview.Num()!=4)return false;
                for(const auto& Number:Preview)if(!Number || Number->Type!=EJson::Number || !FMath::IsFinite(Number->AsNumber()))return false;
            }
            for(const auto& Value:Outputs)
            {
                if(!Value || Value->Type!=EJson::Object)return false;const auto Row=Value->AsObject();
                for(const TCHAR* Key:{TEXT("index"),TEXT("name"),TEXT("guid")})if(!Row->HasTypedField<EJson::String>(Key))return false;
            }
        }
        else
        {
            FString Text,TextHash;TSharedPtr<FJsonObject> Origin;
            if(!Native->TryGetStringField(TEXT("exportedRequestText"),Text) || !Native->TryGetStringField(TEXT("exportedRequestSha256"),TextHash)
                || FTCHARToUTF8(*Text).Length()>NodeForgeEditorControl::MaxGraphBytes || NodeForgePrototype::TextSha256(Text)!=TextHash
                || !Native->HasTypedField<EJson::Object>(TEXT("exportedRequest")) || !Native->HasTypedField<EJson::Object>(TEXT("exportEvidence")))return false;
        }
    }
    else return false;
    Code = 0; Error = TEXT("succeeded"); return true;
}

int32 FNodeForgeEditorControlService::ExecuteReadAt(const FString& Method,
    const TSharedRef<FJsonObject>& Arguments, const FString& Snapshot, const FString& ReportPath,
    FString& QueryDigest, FString& Error)
{
    check(IsInGameThread());
    const FString Mode=ReadMode(Method,Arguments);FString Admitted,NativeReport;
    if(Mode.IsEmpty() || !NodeForgeControlOperations::Find(Method,NodeForgeControlOperations::EHost::ActiveEditor)
        || !AdmitInputPath(Snapshot,Admitted,Error) || !AdmitInputPath(ReportPath,NativeReport,Error)
        || !FPaths::IsUnderDirectory(NativeReport,NodeForgeInstallation::StateDirectory()/TEXT("Reports"))
        || IFileManager::Get().FileExists(*NativeReport) || IFileManager::Get().FileExists(*Admitted))
    {Error=TEXT("queryArgumentsRejected");return 2;}
    FString Saved,ReadError;
    if(!NodeForgePrototype::WriteJson(Arguments,Admitted) || !NodeForgePrototype::ReadInputFile(Admitted,Saved,QueryDigest,ReadError))
    {Error=TEXT("querySnapshotRejected");return 2;}
    FString Input=Method==TEXT("catalog.get")?FString():Admitted,ConfigDigest;
    if(Method==TEXT("resources.inspect") || Method==TEXT("graphs.export"))
    {
        auto Config=MakeShared<FJsonObject>();Config->SetStringField(Mode==TEXT("TextureInspect")?TEXT("texture"):Mode==TEXT("MaterialFunction")?TEXT("function"):Mode==TEXT("MaterialParameters")?TEXT("parent"):TEXT("source"),Arguments->GetStringField(TEXT("source")));
        if(Method==TEXT("graphs.export"))
        {Config->SetStringField(TEXT("requestId"),Arguments->GetStringField(TEXT("requestId")));Config->SetStringField(TEXT("assetPath"),Arguments->GetStringField(TEXT("assetPath")));if(Arguments->HasField(TEXT("formatVersion")))Config->SetField(TEXT("formatVersion"),Arguments->TryGetField(TEXT("formatVersion")));}
        Input=Admitted+TEXT(".Native.json");
        if(!NodeForgePrototype::WriteJson(Config,Input) || !NodeForgePrototype::ReadInputFile(Input,Saved,ConfigDigest,ReadError)
            || FTCHARToUTF8(*Saved).Length()>16384){Error=TEXT("resourceConfigRejected");return 2;}
    }
    const int32 WorkflowCode=NodeForgePrototype::Execute(Mode,Input,NativeReport);
    TSharedPtr<FJsonObject> Native;FString Raw,ReportDigest;int32 Code=5;
    if(!NodeForgeResourceSource::ReadText(NativeReport,NodeForgeEditorControl::MaxReportBytes,Raw,ReportDigest,ReadError)
        || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Raw),Native)
        || !ValidateQueryReport(Mode,WorkflowCode,Native,Code,Error)){Error=TEXT("queryReportUnknown");return 5;}
    if(Code==0 && Method==TEXT("rules.query"))
    {
        const auto Bundle=Native->GetObjectField(TEXT("taskRules"));FString Kind,Query;
        if(!Arguments->TryGetStringField(TEXT("assetKind"),Kind) || !Arguments->TryGetStringField(TEXT("query"),Query)
            || !Bundle->HasTypedField<EJson::String>(TEXT("assetKind")) || !Bundle->HasTypedField<EJson::String>(TEXT("query"))
            || Bundle->GetStringField(TEXT("assetKind"))!=Kind || Bundle->GetStringField(TEXT("query"))!=Query)
        {Error=TEXT("queryParametersUnknown");return 5;}
    }
    if(Code==0 && Method==TEXT("resources.query"))
    {
        TSharedPtr<FJsonObject> Effective; FString ParameterError;
        const auto Query=Native->GetObjectField(TEXT("resourceQuery"));
        FString ActualDigest;
        if(!NodeForgeResourceSource::EffectiveQueryArguments(Arguments,Effective,ParameterError)
            || !Query->TryGetStringField(TEXT("configurationDigest"),ActualDigest) || ActualDigest!=QueryDigest
            || NodeForgeEditorControl::SemanticDigest(Effective.ToSharedRef())
                !=NodeForgeEditorControl::SemanticDigest(Query->GetObjectField(TEXT("filter")).ToSharedRef()))
        {Error=TEXT("resourceQueryParametersUnknown");return 5;}
    }
    if(Code==0 && !ConfigDigest.IsEmpty())
    {
        const auto Read=Native->GetObjectField(TEXT("resourceRead"));
        if(Read->GetStringField(TEXT("configurationDigest"))!=ConfigDigest || Read->GetStringField(TEXT("source"))!=Arguments->GetStringField(TEXT("source")))
        {Error=TEXT("resourceParametersUnknown");return 5;}
        if(Method==TEXT("graphs.export"))
        {
            NodeForgeGraphDocument::FGraphDocument Payload;TSharedPtr<FJsonObject> Origin;
            const int32 Selected=Arguments->HasField(TEXT("formatVersion"))?static_cast<int32>(Arguments->GetNumberField(TEXT("formatVersion"))):2;
            if(!NodeForgeGraphCodec::DecodeText(Native->GetStringField(TEXT("exportedRequestText")),Payload,Error)
                || Payload.RequestId!=Arguments->GetStringField(TEXT("requestId"))
                || Payload.Target!=Arguments->GetStringField(TEXT("assetPath"))
                || Payload.AssetType()!=Arguments->GetStringField(TEXT("assetKind")) || Payload.WireVersion!=Selected
                || !NodeForgeResourceSource::ReadExportOrigin(NativeReport,ReportDigest,Native->GetStringField(TEXT("exportedRequestText")),true,Origin,Error))
            {Error=TEXT("exportParametersUnknown");return 5;}
        }
    }
    return Code;
}
int32 FNodeForgeEditorControlService::QuerySnapshot(const FString& Method,
    const TSharedRef<FJsonObject>& Arguments, const FString& Snapshot, const FString& ReportPath,
    FString& QueryDigest, FString& Error)
{
    check(IsInGameThread());check(Current.bBusy);ResetResult();
    const int32 Code=ExecuteReadAt(Method,Arguments,Snapshot,ReportPath,QueryDigest,Error);
    if(IFileManager::Get().FileExists(*ReportPath))Current.LastReportPath=ReportPath;
    TSharedPtr<FJsonObject> Native;FString Raw;
    if(HasReport() && ReadCurrentReport(Native,&Raw) && Code==0)Current.LastReadOnlyData=MoveTemp(Raw);
    Current.ResultNotice=Code==0?TEXT("只读查询完成；完整结果见报告，草稿和资产未修改。")
        :Code==5?TEXT("本次读取结果无法核实，请检查原报告；停止后续依赖操作。") :TEXT("本次只读查询未成功；请按报告修订参数。");
    if(Code==0 && Method==TEXT("resources.query") && Native.IsValid()
        && !Native->GetObjectField(TEXT("resourceQuery"))->GetBoolField(TEXT("complete")))
        Current.ResultNotice=TEXT("本次目录查询未完成；请查看原报告原因并缩小目录，结果不提供继续游标。草稿和资产未修改。");
    FString ResultMode;
    if(Code==0 && Native.IsValid() && Native->TryGetStringField(TEXT("mode"),ResultMode) && ResultMode==TEXT("MaterialGraphExport"))
    {
        Current.ResultNotice+=TEXT("\n若日志仅提示 /Engine/Transient 没有磁盘包，这是临时内存对象的正常扫描提示，不代表本次导出失败。其他警告仍需查看完整报告。");
    }
    return Code;
}

int32 FNodeForgeEditorControlService::QueryFromUI(const FString& Method, const TSharedRef<FJsonObject>& Arguments)
{
    check(IsInGameThread()); FNodeForgeEditorBusinessScope Lease(*this); if (!Lease) return 2;
    const FString Unique = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    FString Hash, Error;
    return QuerySnapshot(Method, Arguments, NodeForgeInstallation::StateDirectory() / TEXT("Reports/UiInputs") / (Unique + TEXT(".json")),
        NodeForgeInstallation::StateDirectory() / TEXT("Reports/UiRules") / (Unique + TEXT(".json")), Hash, Error);
}

TSharedRef<FJsonObject> FNodeForgeEditorControlService::ExecuteQueryCall(const FString& Method,
    const TSharedRef<FJsonObject>& Arguments, uint64 ExpectedRevision,
    const FString& Session, const FString& CallId, const FString& ReportDirectory)
{
    check(IsInGameThread()); auto Result = MakeShared<FJsonObject>(); int32 Code = 2; FString Reason;
    if (Current.Input.Revision() != ExpectedRevision) Reason = TEXT("staleRevision");
    else if (!Gate->BeginControl()) Reason = TEXT("busy");
    else
    {
        TGuardValue<bool> Running(Current.bBusy, true);
        const FString Snapshot = ReportDirectory / TEXT("Calls") / (CallId + TEXT("Query.json"));
        const FString NativePath = ReportDirectory / TEXT("Calls") / (CallId + TEXT("Native.json"));
        const FString SidecarPath = ReportDirectory / TEXT("Calls") / (CallId + TEXT("Control.json"));
        FString Admitted, Hash;
        bool SessionValid = Session.Len() == 32;
        for (TCHAR C : Session) SessionValid &= (C >= '0' && C <= '9') || (C >= 'a' && C <= 'f');
        if (!SessionValid || !NodeForgeEditorControl::IsCallId(CallId)
            || !FPaths::IsUnderDirectory(ReportDirectory, NodeForgeInstallation::StateDirectory() / TEXT("Reports/PluginControlStageTwo"))
            || IFileManager::Get().FileExists(*NativePath) || IFileManager::Get().FileExists(*SidecarPath)
            || !AdmitInputPath(Snapshot, Admitted, Reason)) Reason = TEXT("querySnapshotRejected");
        else
        {
            Code = QuerySnapshot(Method, Arguments, Snapshot, NativePath, Hash, Reason);
            // No native result exists for a snapshot rejection. Closed business failures
            // and successes both bind their actual reports; unknown never becomes success.
            if ((Code == 0 || Code == 2 || Code == 4) && HasReport())
            {
                FString Raw; TSharedPtr<FJsonObject> Native;
                const FString NativeHash = NodeForgePrototype::FileSha256(NativePath);
                FILETIME Created, Exited, Kernel, User;
                if (!ReadCurrentReport(Native, &Raw) || NativeHash.Len() != 64
                    || !GetProcessTimes(GetCurrentProcess(), &Created, &Exited, &Kernel, &User))
                { Code = 5; Reason = TEXT("queryEvidenceUnknown"); }
                else
                {
                    auto Reference = MakeShared<FJsonObject>(); Reference->SetStringField(TEXT("path"), NativePath);
                    Reference->SetStringField(TEXT("sha256"), NativeHash); Reference->SetNumberField(TEXT("bytes"), FTCHARToUTF8(*Raw).Length());
                    Reference->SetStringField(TEXT("nativeSessionId"), Native->GetStringField(TEXT("sessionId")));
                    auto Sidecar = MakeShared<FJsonObject>(); Sidecar->SetStringField(TEXT("host"), TEXT("activeEditor"));
                    Sidecar->SetStringField(TEXT("inputMethod"), TEXT("editorProgram")); Sidecar->SetStringField(TEXT("controlSessionId"), Session);
                    Sidecar->SetStringField(TEXT("callId"), CallId); Sidecar->SetStringField(TEXT("operation"), Method);
                    Sidecar->SetStringField(TEXT("mode"), Native->GetStringField(TEXT("mode")));
                    Sidecar->SetStringField(TEXT("draftRevision"), LexToString(ExpectedRevision));
                    Sidecar->SetStringField(TEXT("querySnapshot"), Snapshot); Sidecar->SetStringField(TEXT("queryDigest"), Hash);
                    Sidecar->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
                    Sidecar->SetStringField(TEXT("processStart"), LexToString((uint64(Created.dwHighDateTime) << 32) | Created.dwLowDateTime));
                    Sidecar->SetStringField(TEXT("project"), FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()));
                    const FString Module = FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"));
                    const FString ModuleHash = NodeForgePrototype::FileSha256(Module);
                    Sidecar->SetStringField(TEXT("module"), Module); Sidecar->SetStringField(TEXT("binarySha256"), ModuleHash);
                    Sidecar->SetObjectField(TEXT("nativeReport"), Reference);
                    auto Metadata = GetDraft(); Metadata->RemoveField(TEXT("text")); Sidecar->SetObjectField(TEXT("draft"), Metadata);
                    FString Digest; Native->TryGetStringField(TEXT("catalogDigest"), Digest);
                    Sidecar->SetStringField(TEXT("catalogDigest"), Digest); Sidecar->SetStringField(TEXT("resultCode"), Reason);
                    if (ModuleHash.Len() != 64 || !NodeForgePrototype::WriteJson(Sidecar, SidecarPath))
                    { Code = 5; Reason = TEXT("queryEvidenceUnknown"); }
                    else
                    {
                        const FString SidecarHash = NodeForgePrototype::FileSha256(SidecarPath);
                        if (SidecarHash.Len() != 64) { Code = 5; Reason = TEXT("queryEvidenceUnknown"); }
                        else
                        {
                            Result->SetObjectField(TEXT("nativeReport"), Reference);
                            Result->SetStringField(TEXT("controlEvidencePath"), SidecarPath);
                            Result->SetStringField(TEXT("controlEvidenceSha256"), SidecarHash);
                            Result->SetStringField(TEXT("querySnapshot"), Snapshot); Result->SetStringField(TEXT("queryDigest"), Hash);
                            Result->SetStringField(TEXT("catalogDigest"), Digest);
                        }
                    }
                }
            }
            if (Code == 5) { Current.LastReadOnlyData.Empty(); Current.ResultNotice = TEXT("本次读取或调用关联结果未知，停止后续依赖操作。"); }
        }
    }
    Result->SetStringField(TEXT("state"), Code == 0 ? TEXT("completed") : Code == 2 ? TEXT("rejected") : Code == 5 ? TEXT("unknown") : TEXT("failed"));
    Result->SetStringField(TEXT("code"), Reason); Result->SetNumberField(TEXT("cliExitCode"), Code);
    auto Metadata = GetDraft(); Metadata->RemoveField(TEXT("text")); Result->SetObjectField(TEXT("draft"), Metadata);
    return Result;
}

TSharedRef<FJsonObject> FNodeForgeEditorControlService::GetState() const
{
    check(IsInGameThread());
    auto Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("state"), IsBusy() ? TEXT("busy") : TEXT("idle"));
    Result->SetStringField(TEXT("draftRevision"), LexToString(Current.Input.Revision()));
    Result->SetBoolField(TEXT("panelAttached"), PanelCount > 0);
    Result->SetBoolField(TEXT("resultStale"), ResultIsStale());
    Result->SetStringField(TEXT("resultSummary"), Current.ResultText);
    Result->SetStringField(TEXT("resultNotice"), Current.ResultNotice);
    Result->SetStringField(TEXT("reportPath"), Current.LastReportPath);
    Result->SetStringField(TEXT("assetPath"), ResultIsStale() ? FString() : Current.LastAssetPath);
    auto Success = MakeShared<FJsonObject>();
    Success->SetStringField(TEXT("reportPath"), Current.LastSuccessReportPath);
    Success->SetStringField(TEXT("assetPath"), Current.LastSuccessAssetPath);
    Success->SetStringField(TEXT("inputPath"), Current.LastImportInputPath);
    Success->SetStringField(TEXT("inputDigest"), Current.LastImportInputDigest);
    Result->SetObjectField(TEXT("lastSuccess"), Success);
    return Result;
}
bool FNodeForgeEditorControlService::SaveDraftSnapshot(FString& Path, FString& Hash, FString& Error) const
{
    check(IsInGameThread());
    Path = NodeForgeInstallation::StateDirectory() / TEXT("Reports/UiInputs")
        / (FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".json"));
    return SaveDraftSnapshotAt(Path, Hash, Error);
}
bool FNodeForgeEditorControlService::SaveDraftSnapshotAt(const FString& Path, FString& Hash, FString& Error) const
{
    check(IsInGameThread()); const FString Text = Current.Input.Draft().Text;
    if (!FNodeForgePanelState::ValidateExternalText(Text, Error)) return false;
    if (!IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true))
    { Error = TEXT("无法创建输入快照目录。"); return false; }
    TUniquePtr<FArchive> Writer(IFileManager::Get().CreateFileWriter(*Path, FILEWRITE_NoReplaceExisting));
    if (!Writer) { Error = TEXT("无法保存新输入快照；已有文件不会覆盖。"); return false; }
    FTCHARToUTF8 Utf8(*Text); Writer->Serialize(const_cast<ANSICHAR*>(Utf8.Get()), Utf8.Length());
    const bool Written = !Writer->IsError(); const bool Closed = Writer->Close();
    FString SavedText;
    if (!Written || !Closed || !NodeForgePrototype::ReadInputFile(Path, SavedText, Hash, Error)
        || !SavedText.Equals(Text, ESearchCase::CaseSensitive)
        || Hash != NodeForgePrototype::TextSha256(Text))
    { Error = TEXT("输入快照未完整写入；保留文件供检查，未开始生成。"); return false; }
    return true;
}

bool FNodeForgeEditorControlService::HasReport() const
{
    check(IsInGameThread());
    return !Current.LastReportPath.IsEmpty() && IFileManager::Get().FileExists(*Current.LastReportPath);
}
void FNodeForgeEditorControlService::ResetResult(bool InputBound,bool Relayout)
    {
        check(IsInGameThread());
        Current.LastReportPath.Empty();Current.LastAssetPath.Empty();Current.LastReadOnlyData.Empty();Current.ResultNotice.Empty();
        Current.bResultInputBound=InputBound;Current.bResultRelayout=Relayout;Current.ResultRevision=Current.Input.Revision();
        Current.ResultText=TEXT("正在处理本次操作…");
    }
bool FNodeForgeEditorControlService::ReadCurrentReport(TSharedPtr<FJsonObject>& Report,FString* Original)
    {
        check(IsInGameThread());
        FString Json,Description;
        if(!NodeForgeResultView::Read(Current.LastReportPath,Json,Description)
            || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Report) || !Report)
        {
            Current.LastAssetPath.Empty();Current.LastReadOnlyData.Empty();
            Current.ResultText=TEXT("无法读取本次结果；不能确认成功。\n")+Description;
            if(!HasReport())Current.LastReportPath.Empty();
            return false;
        }
        if(Original)*Original=MoveTemp(Json);
        Current.ResultText=NodeForgeResultView::Compact(Report.ToSharedRef());
        return true;
    }
int32 FNodeForgeEditorControlService::PreflightFromUI()
    {
        check(IsInGameThread());
        FNodeForgeEditorBusinessScope Guard(*this); if (!Guard) return 2; ResetResult(true);
        FString Input,Hash,Error;
        if(!SaveDraftSnapshot(Input,Hash,Error)){Current.ResultText=TEXT("生成前检查未执行：")+Error;return 2;}
        const FString Unique=FGuid::NewGuid().ToString(EGuidFormats::Digits);
        Current.LastReportPath=NodeForgeInstallation::StateDirectory()/TEXT("Reports/UiPreflights")/(Unique+TEXT(".json"));
        auto Context=MakeShared<FJsonObject>();Context->SetStringField(TEXT("inputMethod"),TEXT("draft"));
        Context->SetObjectField(TEXT("uiInput"),Current.Input.Provenance(Input,Hash));
        return PreflightSnapshot(Input, Current.LastReportPath, Context);
    }
int32 FNodeForgeEditorControlService::PreflightSnapshot(const FString& Input, const FString& ReportPath, const TSharedPtr<FJsonObject>& Context)
    {
        check(IsInGameThread()); check(Current.bBusy); Current.LastReportPath = ReportPath;
        if(Current.Input.Draft().ExportOrigin)Context->SetObjectField(TEXT("exportOrigin"),Current.Input.Draft().ExportOrigin);
        const int32 Code=NodeForgePrototype::Execute(TEXT("GraphPreflight"),Input,Current.LastReportPath,FString(),Context);
        TSharedPtr<FJsonObject> Report;
        if(ReadCurrentReport(Report))
        {
            FString Status;Report->TryGetStringField(TEXT("operationStatus"),Status);
            Current.ResultNotice=Code==0 && Status==TEXT("passed") && Report->HasTypedField<EJson::Object>(TEXT("preflightSummary"))
                ?TEXT("输入检查通过，尚未生成或预留目标。可继续生成新资产。"):TEXT("输入检查未通过或报告不完整；请按报告修订输入。");
        }
        else return 5;
        return Code;
    }
int32 FNodeForgeEditorControlService::PrepareSnapshot(const FString& Input, const FString& ReportPath,
    const TSharedPtr<FJsonObject>& Context, uint64 ExpectedRevision, TSharedPtr<FJsonObject>& Preparation)
{
    check(IsInGameThread()); check(Current.bBusy); Current.LastReportPath = ReportPath; Preparation.Reset();
    if (Current.Input.Draft().ExportOrigin) Context->SetObjectField(TEXT("exportOrigin"), Current.Input.Draft().ExportOrigin);
    const int32 Code = NodeForgePrototype::Execute(TEXT("PrepareNewCopy"), Input, ReportPath, FString(), Context);
    TSharedPtr<FJsonObject> Native;
    if (!ReadCurrentReport(Native)) return 5;
    if (Code != 0)
    { Current.ResultNotice = TEXT("未准备副本，原输入保持；请查看本次诊断。"); return Code; }
    FString Mode, Overall, Status, Project, NativeMethod, Digest;
    double Pid = 0;
    const TSharedPtr<FJsonObject>* Evidence = nullptr;
    const bool Bound = Native->TryGetStringField(TEXT("mode"), Mode) && Mode == TEXT("PrepareNewCopy")
        && Native->TryGetStringField(TEXT("overallStatus"), Overall) && Overall == TEXT("succeeded")
        && Native->TryGetStringField(TEXT("operationStatus"), Status) && Status == TEXT("passed")
        && Native->TryGetNumberField(TEXT("pid"), Pid) && Pid == FPlatformProcess::GetCurrentProcessId()
        && Native->TryGetStringField(TEXT("project"), Project)
        && FPaths::ConvertRelativePathToFull(Project).Equals(FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()), ESearchCase::IgnoreCase)
        && Native->TryGetStringField(TEXT("inputMethod"), NativeMethod) && NativeMethod == Context->GetStringField(TEXT("inputMethod"))
        && Native->TryGetStringField(TEXT("inputDigest"), Digest) && Digest == NodeForgePrototype::FileSha256(Input)
        && Native->TryGetObjectField(TEXT("copyPreparation"), Evidence);
    if (!Bound)
    { Current.ResultNotice = TEXT("副本报告身份无法核实，未提交草稿；保留原报告和记录，停止依赖操作。"); return 5; }
    Preparation = *Evidence;
    FString Path, ExpectedHash, Admitted, Text, ActualHash, Error, SourceDigest, NewId, NewTarget, OldId, OldTarget;
    bool Reserved = true, Generated = true, SourceUnchanged = false;
    TSharedPtr<FJsonObject> Original, Prepared;
    const FString OriginalText = Current.Input.Draft().Text;
    FTCHARToUTF8 OriginalUtf8(*OriginalText);
    bool Valid = Current.Input.Revision() == ExpectedRevision
        && NodeForgePrototype::TextSha256(OriginalText) == Digest
        && Preparation->TryGetStringField(TEXT("sourceInputDigest"), SourceDigest) && SourceDigest == Digest
        && Preparation->TryGetStringField(TEXT("preparedInputPath"), Path)
        && FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Path), NodeForgeInstallation::StateDirectory() / TEXT("Reports/PreparedCopies"))
        && AdmitInputPath(Path, Admitted, Error)
        && Preparation->TryGetStringField(TEXT("preparedInputDigest"), ExpectedHash)
        && ReadAdmittedInput(Admitted, Text, ActualHash, Error) && ActualHash == ExpectedHash
        && Preparation->TryGetBoolField(TEXT("targetReserved"), Reserved) && !Reserved
        && Preparation->TryGetBoolField(TEXT("assetGenerated"), Generated) && !Generated
        && Preparation->TryGetBoolField(TEXT("sourceInputUnchanged"), SourceUnchanged) && SourceUnchanged
        && Preparation->TryGetStringField(TEXT("newRequestId"), NewId)
        && Preparation->TryGetStringField(TEXT("newAssetPath"), NewTarget)
        && Preparation->TryGetStringField(TEXT("originalRequestId"), OldId)
        && Preparation->TryGetStringField(TEXT("originalAssetPath"), OldTarget)
        && NodeForgeEditorControl::ParseObject(MakeArrayView(reinterpret_cast<const uint8*>(OriginalUtf8.Get()), OriginalUtf8.Length()), Original);
    if (Valid)
    {
        NodeForgeGraphDocument::FGraphDocument OriginalDocument,PreparedDocument;
        Valid = NodeForgeGraphCodec::DecodeText(OriginalText,OriginalDocument,Error)
            && NodeForgeGraphCodec::DecodeText(Text,PreparedDocument,Error)
            && OriginalDocument.RequestId==OldId && OriginalDocument.Target==OldTarget
            && PreparedDocument.RequestId==NewId && PreparedDocument.Target==NewTarget
            && OriginalDocument.WireVersion==PreparedDocument.WireVersion
            && NewId != OldId && NewTarget != OldTarget
            && FPaths::GetPath(NewTarget) == FPaths::GetPath(OldTarget);
        if (Valid)
        {
            auto IdentityOnly=PreparedDocument;IdentityOnly.RequestId=OldId;IdentityOnly.Target=OldTarget;
            Valid=NodeForgeGraphCodec::EqualOriginal(IdentityOnly,OriginalDocument);
        }
    }
    if (!Valid || !Current.Input.PrepareCopy(Text, ReportPath, Error))
    { Current.ResultNotice = TEXT("副本记录已保留，但未提交草稿；原输入和修订保持，停止依赖操作。"); return 5; }
    Current.ResultRevision = Current.Input.Revision();
    Current.ResultNotice = TEXT("新名称已写入输入区，原文件和资产未修改。确认后点“生成新资产”；可恢复上次输入。");
    return 0;
}
int32 FNodeForgeEditorControlService::PreviewSnapshot(const FString& Input, const FString& ReportPath,
    const TSharedPtr<FJsonObject>& Context)
{
    check(IsInGameThread()); check(Current.bBusy); Current.LastReportPath = ReportPath;
    const auto Origin = Current.Input.Draft().ExportOrigin;
    if (!Origin) { Current.ResultNotice = TEXT("预览需要当前草稿的可靠导出来源。"); return 2; }
    Context->SetObjectField(TEXT("exportOrigin"), Origin);
    const int32 Code = NodeForgePrototype::Execute(TEXT("GraphPreview"), Input, ReportPath, Origin->GetStringField(TEXT("reportPath")), Context);
    TSharedPtr<FJsonObject> Report;
    if (!ReadCurrentReport(Report)) return 5;
    FString Status;
    const bool Complete = Report->TryGetStringField(TEXT("operationStatus"), Status) && Status == TEXT("passed")
        && Report->HasTypedField<EJson::Object>(TEXT("graphDifference"));
    Current.ResultNotice = Code == 0 && Complete
        ? TEXT("预览完成。审查差异后，使用“生成新资产”执行本次输入。") : TEXT("预览未通过或报告不完整；请查看完整报告。");
    return Code == 0 && !Complete ? 5 : Code;
}
int32 FNodeForgeEditorControlService::PrepareCopyFromUI()
{
    check(IsInGameThread()); FNodeForgeEditorBusinessScope Guard(*this); if (!Guard) return 2; ResetResult(true);
    FString Input, Hash, Error;
    if (!SaveDraftSnapshot(Input, Hash, Error)) { Current.ResultText = TEXT("未准备副本：") + Error; return 2; }
    const FString Report = NodeForgeInstallation::StateDirectory() / TEXT("Reports/UiCopyPreparations") / (FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".json"));
    auto Context = MakeShared<FJsonObject>(); Context->SetStringField(TEXT("inputMethod"), TEXT("draft"));
    Context->SetObjectField(TEXT("uiInput"), Current.Input.Provenance(Input, Hash)); TSharedPtr<FJsonObject> Preparation;
    return PrepareSnapshot(Input, Report, Context, Current.Input.Revision(), Preparation);
}
int32 FNodeForgeEditorControlService::PreviewFromUI()
{
    check(IsInGameThread()); FNodeForgeEditorBusinessScope Guard(*this); if (!Guard) return 2; ResetResult(true);
    if (!Current.Input.Draft().ExportOrigin) { Current.ResultText = TEXT("预览未执行：当前草稿没有可靠导出来源。"); return 2; }
    FString Input, Hash, Error;
    if (!SaveDraftSnapshot(Input, Hash, Error)) { Current.ResultText = TEXT("预览未执行：") + Error; return 2; }
    const FString Report = NodeForgeInstallation::StateDirectory() / TEXT("Reports/UiPreviews") / (FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".json"));
    auto Context = MakeShared<FJsonObject>(); Context->SetStringField(TEXT("inputMethod"), TEXT("draft"));
    Context->SetObjectField(TEXT("uiInput"), Current.Input.Provenance(Input, Hash));
    return PreviewSnapshot(Input, Report, Context);
}
int32 FNodeForgeEditorControlService::RelayoutFromUI(const TFunction<void()>& BeforeExecute)
    {
        check(IsInGameThread());
        if(Current.LastImportInputPath.IsEmpty())return 2;
        FNodeForgeEditorBusinessScope Guard(*this); if (!Guard) return 2;
        FString Text,Hash,Error;
        if(!NodeForgePrototype::ReadInputFile(Current.LastImportInputPath,Text,Hash,Error)
            || !Hash.Equals(Current.LastImportInputDigest,ESearchCase::CaseSensitive))
        {
            Current.ResultNotice=TEXT("未重排：上次成功输入快照缺失或已改变。不会回退到外部原文件；请重新生成并核对报告。");
            return 2;
        }
        return ImportSnapshot(Current.LastImportInputPath,TEXT("relayoutCopy"),TEXT("RelayoutCopy"),FString(),BeforeExecute);
    }

int32 FNodeForgeEditorControlService::ImportSnapshot(const FString& InputPath,const FString& Method,const FString& Mode,const FString& SavedDigest,const TFunction<void()>& BeforeExecute,
    const FString& OutputReport, const TSharedPtr<FJsonObject>& ContextOverride)
    {
        check(IsInGameThread()); check(Current.bBusy);
        const bool Relayout=Mode.Equals(TEXT("RelayoutCopy"),ESearchCase::CaseSensitive);
        ResetResult(!Relayout,Relayout);
        const FString ExpectedSourceDigest=Relayout?Current.LastImportInputDigest:SavedDigest;
        if(ExpectedSourceDigest.IsEmpty() || !ExpectedSourceDigest.Equals(NodeForgePrototype::FileSha256(InputPath),ESearchCase::CaseSensitive))
        {Current.ResultText=TEXT("未开始生成：输入快照缺失或已改变。");return 2;}
        const FString UniqueName = FString::Printf(TEXT("%s-%s.json"),
            *FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S")), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
        Current.LastReportPath = OutputReport.IsEmpty() ? FPaths::Combine(NodeForgeInstallation::StateDirectory(), TEXT("Reports/UiImports"), UniqueName) : OutputReport;

        if(BeforeExecute) BeforeExecute();
        TSharedPtr<FJsonObject> Context = ContextOverride;
        if (!Context) { Context = MakeShared<FJsonObject>(); Context->SetStringField(TEXT("inputMethod"),Method); }
        if(!ContextOverride && Mode==TEXT("RelayoutCopy"))
        {
            auto Source=MakeShared<FJsonObject>();Source->SetStringField(TEXT("contentSource"),TEXT("lastSuccessfulSnapshot"));
            Source->SetStringField(TEXT("snapshotPath"),InputPath);Source->SetStringField(TEXT("snapshotDigest"),ExpectedSourceDigest);
            Source->SetStringField(TEXT("successfulReport"),Current.LastSuccessReportPath);Source->SetStringField(TEXT("successfulAsset"),Current.LastSuccessAssetPath);
            Context->SetObjectField(TEXT("uiInput"),Source);
        }
        else if (!ContextOverride) Context->SetObjectField(TEXT("uiInput"),Current.Input.Provenance(InputPath,ExpectedSourceDigest));
        if(!Relayout && Current.Input.Draft().ExportOrigin)Context->SetObjectField(TEXT("exportOrigin"),Current.Input.Draft().ExportOrigin);
        const int32 ExitCode = NodeForgePrototype::Execute(Mode, InputPath, Current.LastReportPath,FString(),Context);

        TSharedPtr<FJsonObject> Report;
        if(!ReadCurrentReport(Report))
        {
            Current.ResultNotice=FString::Printf(TEXT("后端返回码：%d。未确认本次生成资产。"),ExitCode);
            return 5;
        }
        FString OverallStatus,NextAction;
        if(!Report->TryGetStringField(TEXT("overallStatus"),OverallStatus) || !Report->TryGetStringField(TEXT("nextAction"),NextAction))
        {
            Current.ResultNotice=TEXT("结果报告缺少必要字段，不能确认成功；资产定位已禁用。");
            return 5;
        }

        Report->TryGetStringField(TEXT("assetPath"), Current.LastAssetPath);
        if (ExitCode==0 && (OverallStatus==TEXT("waitingPersistence") || OverallStatus==TEXT("succeeded"))
            && Current.LastAssetPath.StartsWith(TEXT("/Game/NodeForgeTests/"),ESearchCase::CaseSensitive))
        {
            FString Snapshot=InputPath,RecordedDigest,SnapshotText,SnapshotDigest,SnapshotError;
            Report->TryGetStringField(TEXT("derivedInputPath"),Snapshot);
            Report->TryGetStringField(TEXT("inputDigest"),RecordedDigest);
            FString SourceDigest;
            const bool SourceMatches=Mode!=TEXT("RelayoutCopy")
                || (Report->TryGetStringField(TEXT("sourceInputDigest"),SourceDigest)
                    && SourceDigest.Equals(ExpectedSourceDigest,ESearchCase::CaseSensitive));
            if(!SourceMatches || !NodeForgePrototype::ReadInputFile(Snapshot,SnapshotText,SnapshotDigest,SnapshotError)
                || !RecordedDigest.Equals(SnapshotDigest,ESearchCase::CaseSensitive)
                || (Mode!=TEXT("RelayoutCopy") && !SnapshotDigest.Equals(ExpectedSourceDigest,ESearchCase::CaseSensitive)))
            {
                Current.LastAssetPath.Empty();Current.ResultText=TEXT("后端已结束，但成功输入快照无法核实；资产可能已写入，请查看完整报告。\n")+Current.ResultText;
                Current.ResultNotice=TEXT("本次未更新成功快照，资产定位已禁用；未重新生成或覆盖资产。");
                return 5;
            }
            Current.LastImportInputPath=Snapshot;Current.LastImportInputDigest=SnapshotDigest;
            Current.LastSuccessReportPath=Current.LastReportPath;Current.LastSuccessAssetPath=Current.LastAssetPath;
        }
        if (ExitCode!=0 || (OverallStatus!=TEXT("waitingPersistence") && OverallStatus!=TEXT("succeeded")) ||
            !Current.LastAssetPath.StartsWith(TEXT("/Game/NodeForgeTests/"),ESearchCase::CaseSensitive)) Current.LastAssetPath.Empty();
        Current.ResultText=NodeForgeResultView::Compact(Report.ToSharedRef());
        Current.ResultNotice=ExitCode==0?TEXT("完整诊断和分阶段结果已保存在报告中。"):FString::Printf(TEXT("生成未成功（返回码 %d）。请复制完整报告查看诊断。"),ExitCode);
        return ExitCode;
    }


int32 FNodeForgeEditorControlService::GenerateFromUI(const TFunction<void()>& BeforeExecute)
{
    check(IsInGameThread());
    FNodeForgeEditorBusinessScope Guard(*this); if (!Guard) return 2;
    ResetResult(true); FString Path,Hash,Error;
    if(!SaveDraftSnapshot(Path,Hash,Error))
    {Current.ResultText=TEXT("导入未执行：")+Error;return 2;}
    return ImportSnapshot(Path,TEXT("draft"),TEXT("Import"),Hash,BeforeExecute);
}

TSharedRef<FJsonObject> FNodeForgeEditorControlService::ExecuteDraftCall(const FString& Method,
    const TSharedRef<FJsonObject>& Arguments, uint64 ExpectedRevision)
{
    check(IsInGameThread()); auto Result = MakeShared<FJsonObject>(); FString Error;
    bool Succeeded = false;
    if (Current.Input.Revision() != ExpectedRevision) Error = TEXT("staleRevision");
    else if (!Gate->BeginControl()) Error = TEXT("busy");
    else
    {
        TGuardValue<bool> Running(Current.bBusy, true);
#if WITH_DEV_AUTOMATION_TESTS
        if (ControlProbe) ControlProbe();
#endif
        FString ExportPath,ExportHash;Arguments->TryGetStringField(TEXT("exportReportPath"),ExportPath);Arguments->TryGetStringField(TEXT("exportReportSha256"),ExportHash);
        if (Method == TEXT("draft.loadFile")) Succeeded = LoadFileCore(Arguments->GetStringField(TEXT("path")), Error,ExportPath,ExportHash);
        else if (Method == TEXT("draft.replace"))
        {
            bool Retain = false;
            if (Arguments->HasField(TEXT("retainExportOrigin")) && !Arguments->TryGetBoolField(TEXT("retainExportOrigin"), Retain)) Error = TEXT("invalidParams");
            else if (Retain && (Arguments->HasField(TEXT("exportReportPath")) || Arguments->HasField(TEXT("exportReportSha256")))) Error = TEXT("invalidExportReference");
            else Succeeded = Retain ? RetainOriginEditCore(Arguments->GetStringField(TEXT("text")), Error)
                : ReplaceDraftCore(Arguments->GetStringField(TEXT("text")), TEXT("program"), FString(), FString(), Error, ExportPath, ExportHash);
        }
        else if (Method == TEXT("draft.restore")) Succeeded = RestoreDraftCore(Error);
        else Error = TEXT("operationUnavailable");
    }
    Result->SetStringField(TEXT("state"), Succeeded ? TEXT("completed") : TEXT("rejected"));
    Result->SetStringField(TEXT("code"), Succeeded ? TEXT("succeeded")
        : Error == TEXT("staleRevision") || Error == TEXT("busy") || Error == TEXT("noRestorePoint") || Error == TEXT("operationUnavailable") ? *Error : TEXT("draftRejected"));
    Result->SetNumberField(TEXT("cliExitCode"), Succeeded ? 0 : 2);
    if (!Succeeded) Result->SetStringField(TEXT("reason"), Error);
    auto Metadata = GetDraft(); Metadata->RemoveField(TEXT("text")); Result->SetObjectField(TEXT("draft"), Metadata);
    return Result;
}
