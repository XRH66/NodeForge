// Shared-state contract checks; no graph construction, asset mutation or UI claim.
#include "NodeForgeEditorControlService.h"
#include "NodeForgeControlOperations.h"
#include "NodeForgePrototype.h"
#include "NodeForgePanel.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeEditorSharedStateTest, "NodeForge.EditorSharedState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNodeForgeEditorSharedStateTest::RunTest(const FString& Parameters)
{
    const FString Root = NodeForgePrototype::WorkspaceDirectory(); FString Output;
    if (!FParse::Value(FCommandLine::Get(), TEXT("NFEditorStateReport="), Output)
        || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Output), Root / TEXT("Reports"))
        || IFileManager::Get().FileExists(*Output))
    { AddError(TEXT("Fresh Reports NFEditorStateReport required")); return false; }
    TArray<TSharedPtr<FJsonValue>> Rows; bool All = true;
    auto Row = [&](const TCHAR* Name, bool Passed)
    {
        TestTrue(Name, Passed); All &= Passed;
        auto Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("case"), Name);
        Result->SetBoolField(TEXT("passed"), Passed); Rows.Add(MakeShared<FJsonValueObject>(Result));
    };
    auto Service = MakeShared<FNodeForgeEditorControlService>(); FString Error;
    Row(TEXT("DefaultProductChannelDisabled"), !NodeForgePanel::GetControlChannel().IsValid());
    Row(TEXT("NoRestoreOnFreshService"), !Service->RestoreDraft(Error) && Error == TEXT("noRestorePoint"));
    const FString Input = Root / TEXT("Examples/GraphV2Samples/BlueprintConcat.json");
    Row(TEXT("LoadOriginalFile"), Service->LoadFile(Input, Error));
    const auto First = Service->GetDraft(); const uint64 Revision = Service->State().Input.Revision();
    Row(TEXT("FixedInputDigestAndExactText"), First->GetStringField(TEXT("digest"))
        == TEXT("2c14c2820ce59a5c7df8ec67bfac4743974865861f2e6f693f900d621db655cf")
        && First->GetStringField(TEXT("text")) == Service->State().Input.Draft().Text
        && Revision == 1 && Service->State().Input.Draft().Origin == TEXT("file"));
    const FString Original = Service->State().Input.Draft().Text;
    auto Controlled = MakeShared<FNodeForgeEditorControlService>();
    Row(TEXT("ControlGatePreparation"), Controlled->LoadFile(Input, Error));
    const auto Gate = Controlled->BusinessGate();
    Row(TEXT("QueuedControlSlotBlocksGuiBeforeDispatch"), Gate->ReserveControl() && !Gate->ReserveControl()
        && Controlled->IsBusy() && !Controlled->TryBeginBusiness() && !Controlled->EditFromUI(TEXT("blocked"))
        && !Controlled->LoadFile(Input, Error) && Controlled->State().Input.Draft().Text == Original
        && Controlled->GetState()->GetStringField(TEXT("state")) == TEXT("busy"));
    auto Arguments = MakeShared<FJsonObject>(); Arguments->SetStringField(TEXT("text"), TEXT("{}"));
    auto ControlledResult = Controlled->ExecuteDraftCall(TEXT("draft.replace"), Arguments, 1);
    Row(TEXT("ReservedControlChangesOneSharedDraftOnce"), ControlledResult->GetStringField(TEXT("state")) == TEXT("completed")
        && Controlled->State().Input.Revision() == 2 && Controlled->State().Input.Draft().Text == TEXT("{}")
        && Controlled->State().Input.Draft().Origin == TEXT("program") && Gate->IsBusy() && !Controlled->TryBeginBusiness());
    const auto Nested = Controlled->ExecuteDraftCall(TEXT("draft.replace"), Arguments, 2);
    Row(TEXT("RunningControlRejectsReentrantDispatchAndPublicUiMutation"), Nested->GetStringField(TEXT("code")) == TEXT("busy")
        && !Controlled->LoadFile(Input, Error) && !Controlled->RestoreDraft(Error)
        && !Controlled->ReplaceDraft(TEXT("blocked"), TEXT("program"), FString(), FString(), Error)
        && Controlled->State().Input.Revision() == 2 && Controlled->State().Input.Draft().Text == TEXT("{}"));
    Gate->EndControl();
    Row(TEXT("FinishedControlSlotReturnsGuiToIdle"), !Controlled->IsBusy() && Controlled->TryBeginBusiness()); Controlled->EndBusiness();
    Row(TEXT("ReserveStaleControl"), Gate->ReserveControl());
    ControlledResult = Controlled->ExecuteDraftCall(TEXT("draft.replace"), Arguments, 1);
    Row(TEXT("DispatchRechecksRevisionBeforeDraftChange"), ControlledResult->GetStringField(TEXT("code")) == TEXT("staleRevision")
        && Controlled->State().Input.Revision() == 2 && Gate->IsControlReserved()); Gate->EndControl();
    Row(TEXT("ReserveControlRestore"), Gate->ReserveControl());
    ControlledResult = Controlled->ExecuteDraftCall(TEXT("draft.restore"), MakeShared<FJsonObject>(), 2);
    Row(TEXT("ControlRestoreReusesExactPreviousOnce"), ControlledResult->GetStringField(TEXT("state")) == TEXT("completed")
        && Controlled->State().Input.Revision() == 3 && Controlled->State().Input.Draft().Text == Original && !Controlled->State().Input.CanRestore()); Gate->EndControl();
    Row(TEXT("SameLoadNoRevisionOrNewRestore"), Service->LoadFile(Input, Error)
        && Service->State().Input.Revision() == Revision);
    Row(TEXT("MissingLoadKeepsDraftAndSource"), !Service->LoadFile(Root / TEXT("Tests/MissingEditorInput.json"), Error)
        && Service->State().Input.Revision() == Revision && Service->State().Input.Draft().Text == Original
        && Service->State().Input.Draft().SourcePath == Input);
    Row(TEXT("RelativeAndOutsidePathRejected"), !Service->LoadFile(TEXT("Tests/Missing.json"), Error)
        && !Service->LoadFile(TEXT("C:/NodeForgeExample/PrivateSentinelNotPresent.json"), Error)
        && Service->State().Input.Revision() == Revision);
    Row(TEXT("EmptyExternalReplaceRejected"), !Service->ReplaceDraft(TEXT(""), TEXT("program"), TEXT(""), TEXT(""), Error)
        && Service->State().Input.Revision() == Revision);
    Row(TEXT("ProgramReplaceAndSameReplace"), Service->ReplaceDraft(TEXT("{\"new\":true}"), TEXT("program"), TEXT(""), TEXT(""), Error)
        && Service->State().Input.Revision() == Revision + 1
        && Service->ReplaceDraft(TEXT("{\"new\":true}"), TEXT("program"), TEXT(""), TEXT(""), Error)
        && Service->State().Input.Revision() == Revision + 1);
    Row(TEXT("RestoreExactPreviousOnce"), Service->RestoreDraft(Error)
        && Service->State().Input.Draft().Text == Original
        && Service->State().Input.Revision() == Revision + 2 && !Service->RestoreDraft(Error));
    Service->MutableState().bResultInputBound = true;
    Service->MutableState().ResultRevision = Service->State().Input.Revision();
    Service->MutableState().LastSuccessAssetPath = TEXT("/Game/NodeForgeTests/ExistingSuccessSentinel");
    Row(TEXT("UIEmptyDraftAllowedAndStale"), Service->EditFromUI(TEXT(""))
        && Service->State().Input.Revision() == Revision + 3 && Service->ResultIsStale()
        && Service->GetDraft()->GetStringField(TEXT("text")).IsEmpty()
        && Service->GetState()->GetObjectField(TEXT("lastSuccess"))->GetStringField(TEXT("assetPath"))
            == TEXT("/Game/NodeForgeTests/ExistingSuccessSentinel"));
    Row(TEXT("BusyRejectsUIAndExternalMutation"), Service->TryBeginBusiness() && !Service->TryBeginBusiness()
        && !Service->EditFromUI(Original) && !Service->LoadFile(Input, Error)
        && !Service->ReplaceDraft(Original, TEXT("program"), TEXT(""), TEXT(""), Error)
        && !Service->RestoreDraft(Error) && Service->State().Input.Revision() == Revision + 3
        && Service->GetState()->GetStringField(TEXT("state")) == TEXT("busy"));
    Service->EndBusiness();
    auto Review = MakeShared<FNodeForgeEditorControlService>(); Review->EditFromUI(TEXT("review"));
    Row(TEXT("ReviewOwnerDoesNotPolluteProduct"), Service->State().Input.Draft().Text.IsEmpty()
        && Review->State().Input.Draft().Text == TEXT("review"));
    Service->AttachPanel(); Row(TEXT("AttachThenDetachRetainsState"), Service->GetState()->GetBoolField(TEXT("panelAttached")));
    Service->DetachPanel(); Row(TEXT("DetachedProductRetainsRevision"), !Service->GetState()->GetBoolField(TEXT("panelAttached"))
        && Service->State().Input.Revision() == Revision + 3);
    const FString Work = FPaths::GetPath(Output) / (TEXT("StateInputs") + FGuid::NewGuid().ToString(EGuidFormats::Digits));
    IFileManager::Get().MakeDirectory(*Work, true);
    const FString Oversize = Work / TEXT("Oversize.json");
    const FString OversizeText = FString::ChrN(65537, 'x'); // Byte-size fixture, never truncated.
    const FString Bom = Work / TEXT("Bom.json"); const FString Invalid = Work / TEXT("InvalidUtf8.json");
    TArray<uint8> InvalidBytes = {0xff, 0xfe, 0x78};
    const bool Fixtures = FFileHelper::SaveStringToFile(OversizeText, *Oversize, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)
        && FFileHelper::SaveStringToFile(TEXT("{}"), *Bom, FFileHelper::EEncodingOptions::ForceUTF8)
        && FFileHelper::SaveArrayToFile(InvalidBytes, *Invalid);
    Row(TEXT("BoundedInvalidLoadsKeepOriginalState"), Fixtures && !Service->LoadFile(Oversize, Error)
        && !Service->LoadFile(Bom, Error) && !Service->LoadFile(Invalid, Error)
        && Service->State().Input.Revision() == Revision + 3);
    Row(TEXT("SnapshotUsesServiceDraftAndExactHash"), Service->LoadFile(Input, Error));
    FString Snapshot, Digest;
    Row(TEXT("SnapshotClosedAndReadBackMatchesFixedSHA"), Service->SaveDraftSnapshot(Snapshot, Digest, Error)
        && Digest == TEXT("2c14c2820ce59a5c7df8ec67bfac4743974865861f2e6f693f900d621db655cf")
        && Service->State().Input.Draft().SourcePath == Input);
    const auto Definitions = NodeForgeControlOperations::Definitions();
    int32 CommandletCount = 0;
    for (const auto& Definition : Definitions)
        if (NodeForgeControlOperations::Supports(Definition, NodeForgeControlOperations::EHost::Commandlet)) ++CommandletCount;
    TArray<FString> CommandletNames;
    for (const auto& Definition : Definitions)
        if (NodeForgeControlOperations::Supports(Definition, NodeForgeControlOperations::EHost::Commandlet)) CommandletNames.Add(Definition.Name);
    const TArray<FString> ExpectedNames = {TEXT("operations.describe"), TEXT("graphs.generateFromFile"),
        TEXT("graphs.preflightFromFile"), TEXT("graphs.verifySaved"), TEXT("resources.query")};
    Row(TEXT("ExistingFiveCommandletDefinitionsRemainSingleSource"), CommandletCount == 5 && CommandletNames == ExpectedNames);
    Row(TEXT("SharedNativePreflightUsesExactDraft"), Service->PreflightFromUI() == 0
        && Service->State().bResultInputBound && !Service->ResultIsStale());
    const FString GoodPreflight = Service->State().LastReportPath;
    Row(TEXT("EditedInvalidGraphMakesPriorResultStale"), Service->EditFromUI(Original + TEXT("x")) && Service->ResultIsStale());
    Row(TEXT("FailedNativePreflightKeepsLastSuccess"), Service->PreflightFromUI() != 0
        && Service->State().LastAssetPath.IsEmpty() && !Service->ResultIsStale()
        && Service->State().LastSuccessAssetPath == TEXT("/Game/NodeForgeTests/ExistingSuccessSentinel")
        && Service->State().LastReportPath != GoodPreflight);
    Service->EditFromUI(TEXT(" \n "));
    Row(TEXT("TemporaryWhitespaceReadableButSnapshotRejected"), Service->GetDraft()->GetStringField(TEXT("text")) == TEXT(" \n ")
        && !Service->SaveDraftSnapshot(Snapshot, Digest, Error));
    Service->MutableState().LastReportPath = Work / TEXT("MissingResult.json");
    Service->MutableState().LastAssetPath = TEXT("/Game/NodeForgeTests/OldResultSentinel");
    TSharedPtr<FJsonObject> MissingResult;
    Row(TEXT("ResultReadFailureClearsCurrentNotLastSuccess"), !Service->ReadCurrentReport(MissingResult)
        && Service->State().LastAssetPath.IsEmpty() && Service->State().LastReportPath.IsEmpty()
        && Service->State().LastSuccessAssetPath == TEXT("/Game/NodeForgeTests/ExistingSuccessSentinel"));
    auto Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("operationStatus"), All ? TEXT("passed") : TEXT("failed"));
    Report->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
    const FString Module = FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"));
    Report->SetStringField(TEXT("module"), Module);
    Report->SetStringField(TEXT("moduleSha256"), NodeForgePrototype::FileSha256(Module));
    Report->SetStringField(TEXT("scope"), TEXT("Internal shared service; no external IPC/UI/asset claim"));
    Report->SetStringField(TEXT("snapshotPath"), Snapshot);
    Report->SetStringField(TEXT("snapshotDigest"), Digest);
    Report->SetArrayField(TEXT("checks"), Rows);
    return NodeForgePrototype::WriteJson(Report, Output) && All;
}
#endif
