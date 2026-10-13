// Operates only the real registered production tab in an explicitly owned test Editor.
#include "NodeForgePanel.h"
#include "NodeForgePrototype.h"
#include "NodeForgeInstallation.h"
#include "NodeForgeEditorControlService.h"
#include "NodeForgeEditorControlChannel.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformMisc.h"
#include "Modules/ModuleManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/SWindow.h"
#include "IAutomationDriverModule.h"
#include "IAutomationDriver.h"
#include "IDriverSequence.h"
#include "DriverConfiguration.h"
#include "LocateBy.h"
#include "InputCoreTypes.h"
#include "Async/IAsyncTask.h"
#include "ImageUtils.h"

namespace
{
TSharedPtr<SWidget> FindTagged(const TSharedRef<SWidget>& Root, const FName Tag, int32 Depth = 0)
{
    if (Root->GetTag() == Tag) return Root;
    if (Depth > 64) return nullptr;
    FChildren* Children = Root->GetChildren();
    for (int32 Index = 0; Children && Index < Children->Num(); ++Index)
        if (const auto Found = FindTagged(Children->GetChildAt(Index), Tag, Depth + 1)) return Found;
    return nullptr;
}

class FNodeForgeProductionUiCommand : public IAutomationLatentCommand
{
public:
    FNodeForgeProductionUiCommand(FAutomationTestBase* InTest, FString InOutput)
        : Test(InTest), Output(MoveTemp(InOutput)), Started(FPlatformTime::Seconds()), PhaseTime(Started),
          bExternal(FParse::Param(FCommandLine::Get(), TEXT("NFEditorControlUiChecks"))) {}
    virtual ~FNodeForgeProductionUiCommand() override { DisableDriver(); }
    virtual bool Update() override
    {
        if (FPlatformTime::Seconds() - Started > 60)
        { Row(TEXT("UiDeadline"), false); return Finish(); }
        FString Error;
        switch (Phase)
        {
        case 0:
            Service = NodeForgePanel::GetProductService();
            Tab = FGlobalTabmanager::Get()->TryInvokeTab(FName(TEXT("NodeForge")));
            if (!Row(TEXT("RealRegisteredProductionTabAndOwner"), Service.IsValid() && Tab.IsValid())) return Finish();
            Tab->DrawAttention();
            Input = StaticCastSharedPtr<SMultiLineEditableTextBox>(FindTagged(Tab->GetContent(), TEXT("NodeForge.Input")));
            if (!Row(TEXT("RealProductionInputFound"), Input.IsValid())) return Finish();
            OriginalPath = NodeForgePrototype::WorkspaceDirectory() / (FParse::Param(FCommandLine::Get(), TEXT("NFUiIntegrationChecks"))
                ? TEXT("Examples/FirstMaterial.json") : TEXT("Examples/GraphV2Samples/BlueprintConcat.json"));
            if (bExternal)
            {
                Channel = NodeForgePanel::GetControlChannel();
                if (!Row(TEXT("ExternalUiUsesProductionChannel"), Channel.IsValid())) return Finish();
                FString Hash;
                if (!Row(TEXT("FixedExpectedSourceReadOnly"), NodeForgePrototype::ReadInputFile(OriginalPath, Original, Hash, Error))) return Finish();
                auto Ready = MakeShared<FJsonObject>(); Ready->SetObjectField(TEXT("identity"), Channel->PublicIdentity());
                Ready->SetStringField(TEXT("inputPath"), OriginalPath); Ready->SetStringField(TEXT("digest"), Hash);
                if (!Row(TEXT("ExternalUiReadyPublished"), NodeForgePrototype::WriteJson(Ready, FPaths::GetPath(Output) / TEXT("UiReady.json")))) return Finish();
                Next(20); return false;
            }
            if (!Row(TEXT("ServiceLoadIntoProduction"), Service->LoadFile(OriginalPath, Error))) return Finish();
            Original = Service->State().Input.Draft().Text; Revision = Service->State().Input.Revision();
            Next(1); return false;
        case 20:
            if (!IFileManager::Get().FileExists(*(FPaths::GetPath(Output) / TEXT("UiLoaded.json")))) return false;
            if (!Row(TEXT("ExternalLoadChangedSingleSharedDraft"), Service->State().Input.Draft().Text == Original && Service->State().Input.Revision() == 1)) return Finish();
            Revision = Service->State().Input.Revision(); Next(1); return false;
        case 1:
            if (!Settled()) return false;
            {
                const auto Window = FSlateApplication::Get().FindWidgetWindow(Tab.ToSharedRef());
                if (!Row(TEXT("ActualVisibleNativeWindow"), Window.IsValid() && Window->IsVisible() && Window->GetNativeWindow().IsValid())) return Finish();
                Window->BringToFront(true);
                if (!Row(TEXT("ServiceToUIExactAndOneRevision"), Input->GetText().ToString() == Original
                    && Service->State().Input.Revision() == Revision && Service->GetState()->GetBoolField(TEXT("panelAttached")))) return Finish();
                if (!Screenshot(TEXT("ServiceToProduction.png"))) return Finish();
            }
            if (!EnableDriver()) return Finish();
            EditOneCharacter(); Next(2); return false;
        case 2:
            if (!Pending.GetFuture().IsReady()) return false;
            if (!Row(TEXT("ActualDriverTypingCompleted"), Pending.GetFuture().Get())) return Finish();
            Next(22); return false;
        case 22:
            if (!Settled()) return false;
            if (!Row(TEXT("UIToServiceExactOneRevision"), Input->GetText().ToString() == EditedExpectation()
                && Service->GetDraft()->GetStringField(TEXT("text")) == EditedExpectation()
                && Service->State().Input.Revision() == Revision + 1)) return Finish();
            if (!Screenshot(TEXT("ProductionToService.png"))) return Finish();
            if (bExternal)
            {
                auto Typed = MakeShared<FJsonObject>(); Typed->SetStringField(TEXT("expectedText"), EditedExpectation());
                Typed->SetStringField(TEXT("expectedRevision"), LexToString(Revision + 1));
                if (!Row(TEXT("ActualDriverTypedForExternalRead"), NodeForgePrototype::WriteJson(Typed, FPaths::GetPath(Output) / TEXT("UiTyped.json")))) return Finish();
                Next(23); return false;
            }
            if (!Row(TEXT("RestoreOriginalThroughService"), Service->LoadFile(OriginalPath, Error))) return Finish();
            Revision = Service->State().Input.Revision(); Next(3); return false;
        case 23:
            if (!IFileManager::Get().FileExists(*(FPaths::GetPath(Output) / TEXT("UiRead.json")))) return false;
            if (!Row(TEXT("ReloadAfterExternalReadProof"), Service->LoadFile(OriginalPath, Error))) return Finish();
            Revision = Service->State().Input.Revision(); Next(3); return false;
        case 3:
            if (!Settled()) return false;
            if (!Row(TEXT("ProgramReloadDoesNotDuplicateRevision"), Input->GetText().ToString() == Original
                && Service->State().Input.Revision() == Revision)) return Finish();
            {
                Sequence = Driver->CreateSequence();
                Sequence->Actions().Click(By::Path(TEXT("NodeForge.Preflight")));
                Pending = Sequence->Perform();
            }
            Next(4); return false;
        case 4:
            if (!Pending.GetFuture().IsReady()) return false;
            if (!Row(TEXT("ActualPreflightButtonClicked"), Pending.GetFuture().Get())) return Finish();
            Next(41); return false;
        case 41:
            if (!Settled()) return false;
            {
                const bool Ready = Service->State().bResultInputBound
                    && Service->State().ResultRevision == Revision && !Service->ResultIsStale()
                    && !Service->State().LastReportPath.IsEmpty();
                if (!Ready && FPlatformTime::Seconds() - PhaseTime < 3) return false;
                const auto Button = FindTagged(Tab->GetContent(), TEXT("NodeForge.Preflight"));
                auto Observation = MakeShared<FJsonObject>();
                Observation->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
                Observation->SetBoolField(TEXT("busy"), Service->IsBusy());
                Observation->SetBoolField(TEXT("inputBound"), Service->State().bResultInputBound);
                Observation->SetBoolField(TEXT("stale"), Service->ResultIsStale());
                Observation->SetStringField(TEXT("draftRevision"), LexToString(Service->State().Input.Revision()));
                Observation->SetStringField(TEXT("resultRevision"), LexToString(Service->State().ResultRevision));
                Observation->SetStringField(TEXT("expectedRevision"), LexToString(Revision));
                Observation->SetStringField(TEXT("reportPath"), Service->State().LastReportPath);
                Observation->SetStringField(TEXT("resultText"), Service->State().ResultText);
                Observation->SetStringField(TEXT("notice"), Service->State().ResultNotice);
                Observation->SetBoolField(TEXT("buttonEnabled"), Button.IsValid() && Button->IsEnabled());
                Observation->SetBoolField(TEXT("buttonHovered"), Button.IsValid() && Button->IsHovered());
                Observation->SetBoolField(TEXT("ready"), Ready);
                if (!Row(TEXT("PreflightButtonObservationSaved"), NodeForgePrototype::WriteJson(Observation,
                    FPaths::GetPath(Output) / TEXT("PreflightButtonObservation.json")))) return Finish();
                if (!Screenshot(TEXT("PreflightButtonProduction.png"))) return Finish();
            }
            if (!Row(TEXT("CurrentResultBoundToSharedDraft"), Service->State().bResultInputBound
                && Service->State().ResultRevision == Revision && !Service->ResultIsStale()
                && !Service->State().LastReportPath.IsEmpty())) return Finish();
            PreflightReport = Service->State().LastReportPath;
            EditOneCharacter(); Next(5); return false;
        case 5:
            if (!Pending.GetFuture().IsReady()) return false;
            Next(55); return false;
        case 55:
            if (!Settled()) return false;
            if (!Row(TEXT("SecondTypingAndStaleResult"), Pending.GetFuture().Get() && Service->ResultIsStale()
                && Service->State().Input.Revision() == Revision + 1
                && Service->State().LastSuccessAssetPath.IsEmpty())) return Finish();
            Sequence = Driver->CreateSequence();
            Sequence->Actions().Click(By::Path(TEXT("NodeForge.Preflight")));
            Pending = Sequence->Perform(); Next(56); return false;
        case 56:
            if (!Pending.GetFuture().IsReady()) return false;
            if (!Row(TEXT("ActualInvalidPreflightButtonClicked"), Pending.GetFuture().Get())) return Finish();
            FailedPreflightReport = Service->State().LastReportPath;
            if (!Row(TEXT("FailureReplacesCurrentKeepsSuccessSeparate"), FailedPreflightReport != PreflightReport
                && Service->State().LastAssetPath.IsEmpty() && Service->State().LastSuccessAssetPath.IsEmpty()
                && !Service->ResultIsStale() && Service->State().ResultRevision == Revision + 1)) return Finish();
            if (!Row(TEXT("ReloadBeforeTabReopen"), Service->LoadFile(OriginalPath, Error))) return Finish();
            Revision = Service->State().Input.Revision();
            {
                const FString Before = Service->State().Input.Draft().Text;
                const auto Review = NodeForgePanel::CreateForReview(FString());
                if (!Row(TEXT("ActualReviewWidgetDoesNotPolluteProduct"), Service->State().Input.Revision() == Revision
                    && Service->State().Input.Draft().Text == Before)) return Finish();
            }
            DisableDriver();
            if (!Row(TEXT("ProductionTabNormalCloseRequested"), Tab->RequestCloseTab())) return Finish();
            Input.Reset(); Tab.Reset(); Next(6); return false;
        case 6:
            if (!Settled()) return false;
            Tab = FGlobalTabmanager::Get()->TryInvokeTab(FName(TEXT("NodeForge")));
            if (!Row(TEXT("ProductionTabReopened"), Tab.IsValid())) return Finish();
            Tab->DrawAttention();
            Input = StaticCastSharedPtr<SMultiLineEditableTextBox>(FindTagged(Tab->GetContent(), TEXT("NodeForge.Input")));
            Next(7); return false;
        case 7:
            if (!Settled()) return false;
            if (!Row(TEXT("ReopenRetainsOwnerTextAndRevision"), Input.IsValid() && Input->GetText().ToString() == Original
                && NodeForgePanel::GetProductService() == Service && Service->State().Input.Revision() == Revision)) return Finish();
            BusyOwned = Service->TryBeginBusiness();
            if (!Row(TEXT("BusySlotAcquired"), BusyOwned)) return Finish();
            Next(8); return false;
        case 8:
            if (!Settled()) return false; // Slate enabled attributes update during native frames.
            {
                const auto Generate = FindTagged(Tab->GetContent(), TEXT("NodeForge.Generate"));
                const auto Restore = FindTagged(Tab->GetContent(), TEXT("NodeForge.RestoreInput"));
                BusyObservation = MakeShared<FJsonObject>();
                BusyObservation->SetBoolField(TEXT("inputEnabled"), Input->IsEnabled());
                BusyObservation->SetBoolField(TEXT("generateEnabled"), Generate.IsValid() && Generate->IsEnabled());
                BusyObservation->SetBoolField(TEXT("restoreEnabled"), Restore.IsValid() && Restore->IsEnabled());
                Row(TEXT("BusyDisablesRealUIAndRejectsMutation"), !Input->IsEnabled()
                    && Generate.IsValid() && !Generate->IsEnabled() && Restore.IsValid() && !Restore->IsEnabled()
                    && !Service->LoadFile(OriginalPath, Error) && Service->State().Input.Revision() == Revision);
                if (bExternal)
                {
                    auto Busy = MakeShared<FJsonObject>(); Busy->SetStringField(TEXT("draftRevision"), LexToString(Revision));
                    if (!Row(TEXT("GuiBusyPublishedForRealRpcRejection"), NodeForgePrototype::WriteJson(Busy, FPaths::GetPath(Output) / TEXT("UiBusy.json")))) return Finish();
                    Next(81); return false;
                }
                Service->EndBusiness(); BusyOwned = false;
            }
            if (!All) return Finish();
            Next(9); return false;
        case 81:
            if (!IFileManager::Get().FileExists(*(FPaths::GetPath(Output) / TEXT("UiBusyRead.json")))) return false;
            Service->EndBusiness(); BusyOwned = false;
            Next(9); return false;
        case 9:
            if (!Settled()) return false;
            if (!EnableDriver()) return Finish();
            Sequence = Driver->CreateSequence();
            Sequence->Actions().Click(By::Path(TEXT("NodeForge.RestoreInput")));
            Pending = Sequence->Perform(); Next(10); return false;
        case 10:
            if (!Pending.GetFuture().IsReady()) return false;
            if (!Row(TEXT("ActualRestoreButtonClicked"), Pending.GetFuture().Get())) return Finish();
            Next(11); return false;
        case 11:
            if (!Settled()) return false;
            if (!Row(TEXT("ActualUIRestoreExactAndOneRevision"), Input->GetText().ToString() == EditedExpectation()
                && Service->State().Input.Draft().Text == EditedExpectation()
                && Service->State().Input.Revision() == Revision + 1)) return Finish();
            if (!Row(TEXT("ReloadFixedOriginalAfterRestore"), Service->LoadFile(OriginalPath, Error))) return Finish();
            Revision = Service->State().Input.Revision(); Next(12); return false;
        case 12:
            if (!Settled()) return false;
            Row(TEXT("IdleUIEnabledAndOriginalRestored"), Input->IsEnabled() && Input->GetText().ToString() == Original
                && Service->State().Input.Revision() == Revision);
            Screenshot(TEXT("ReopenedProduction.png"));
            if (bExternal && FParse::Param(FCommandLine::Get(), TEXT("NFEditorControlDisableUiChecks")))
            {
                if (!Row(TEXT("PublishIdleUiForFormalDisable"), NodeForgePrototype::WriteJson(MakeShared<FJsonObject>(), FPaths::GetPath(Output) / TEXT("UiIdle.json")))) return Finish();
                Next(121); return false;
            }
            Next(122); return false;
        case 121:
            if (!IFileManager::Get().FileExists(*(FPaths::GetPath(Output) / TEXT("UiDisabled.json")))) return false;
            if (!Row(TEXT("FormalDisableKeepsProductionUiAndServiceUsable"), Channel->IsStopped()
                && !NodeForgePanel::GetControlChannel() && NodeForgePanel::GetProductService() == Service
                && Input->IsEnabled() && !Service->IsBusy() && Input->GetText().ToString() == Original)) return Finish();
            Next(122); return false;
        case 122:
            Sequence = Driver->CreateSequence();
            Sequence->Actions().Click(By::Path(TEXT("NodeForge.PrepareCopy")));
            Pending = Sequence->Perform(); Next(13); return false;
        case 13:
            if (!Pending.GetFuture().IsReady()) return false;
            if (!Row(TEXT("ActualPrepareCopyButtonClickedNoAsset"), Pending.GetFuture().Get())) return Finish();
            PreparationReport = Service->State().LastReportPath;
            if (!Row(TEXT("PreparedInputBoundToSingleOwner"), !Service->State().Input.Draft().PreparationReport.IsEmpty()
                && Service->State().Input.Revision() == Revision + 1
                && Service->State().ResultRevision == Revision + 1 && Service->State().LastAssetPath.IsEmpty())) return Finish();
            if (!Row(TEXT("FixedOriginalReloadAfterPreparation"), Service->LoadFile(OriginalPath, Error))) return Finish();
            Revision = Service->State().Input.Revision(); Next(14); return false;
        case 14:
            if (!Settled()) return false;
            Row(TEXT("PreparationLeavesProductWithFixedOriginal"), Input->GetText().ToString() == Original
                && Service->State().Input.Revision() == Revision);
            return Finish();
        default: Row(TEXT("UnexpectedUiPhase"), false); return Finish();
        }
    }
private:
    bool Row(const TCHAR* Label, bool Passed)
    {
        Test->TestTrue(Label, Passed); All &= Passed;
        auto Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("case"), Label);
        Result->SetBoolField(TEXT("passed"), Passed); Rows.Add(MakeShared<FJsonValueObject>(Result)); return Passed;
    }
    void Next(int32 Value) { Phase = Value; PhaseTime = FPlatformTime::Seconds(); }
    bool Settled() const { return FPlatformTime::Seconds() - PhaseTime >= 0.4; }
    bool EnableDriver()
    {
        auto& Module = IAutomationDriverModule::Get();
        if (!Row(TEXT("DriverNotOwnedByAnotherTest"), !Module.IsEnabled())) return false;
        Module.Enable(); DriverOwned = true;
        auto Config = MakeShared<FDriverConfiguration, ESPMode::ThreadSafe>();
        Config->ImplicitWait = FTimespan::FromSeconds(3);
        Driver = Module.CreateAsyncDriver(Config); return true;
    }
    void DisableDriver()
    {
        if (!DriverOwned) return;
        if (Pending.GetFuture().IsValid() && !Pending.GetFuture().IsReady())
            if (const auto Task = Pending.GetTask()) Task->Cancel();
        Sequence.Reset(); Driver.Reset(); IAutomationDriverModule::Get().Disable(); DriverOwned = false;
    }
    void EditOneCharacter()
    {
        // Position only; text still changes exclusively through actual driver character input.
        // Native Windows Slate reconstructs edited lines using LINE_TERMINATOR (CRLF).
        FSlateApplication::Get().SetKeyboardFocus(Input, EFocusCause::SetDirectly);
        Input->GoTo(ETextLocation::EndOfDocument);
        Sequence = Driver->CreateSequence();
        Sequence->Actions().Type(TEXT("x"));
        Pending = Sequence->Perform();
    }
    FString EditedExpectation() const
    {
        return Original.Replace(TEXT("\r\n"), TEXT("\n"))
            .Replace(TEXT("\n"), LINE_TERMINATOR) + TEXT("x");
    }
    bool Screenshot(const TCHAR* Filename)
    {
        const FString Path = FPaths::GetPath(Output) / Filename;
        TArray<FColor> Pixels; FIntVector Size = FIntVector::ZeroValue;
        if (!Row(TEXT("FreshRealProductionScreenshot"), !IFileManager::Get().FileExists(*Path)
            && FSlateApplication::Get().TakeScreenshot(Tab->GetContent(), Pixels, Size))) return false;
        TArray64<uint8> Bytes; FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Bytes);
        return Row(TEXT("ProductionScreenshotSaved"), Bytes.Num() > 0 && FFileHelper::SaveArrayToFile(Bytes, *Path));
    }
    bool Finish()
    {
        if (BusyOwned && Service) { Service->EndBusiness(); BusyOwned = false; }
        DisableDriver(); Row(TEXT("DriverActuallyDisabled"), !IAutomationDriverModule::Get().IsEnabled());
        if (bExternal && Channel) Row(TEXT("ExternalUiChannelStoppedBeforeNormalExit"), Channel->Stop() && Channel->IsStopped());
        auto Report = MakeShared<FJsonObject>();
        Report->SetStringField(TEXT("status"), All ? TEXT("passed") : TEXT("failed"));
        Report->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
        const FString Module = FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"));
        Report->SetStringField(TEXT("module"), Module); Report->SetStringField(TEXT("moduleSha256"), NodeForgePrototype::FileSha256(Module));
        Report->SetStringField(TEXT("preflightReport"), PreflightReport);
        Report->SetStringField(TEXT("failedPreflightReport"), FailedPreflightReport);
        Report->SetStringField(TEXT("preparationReport"), PreparationReport);
        // Fixed public graph only, for diagnosing exact UI/state discrepancies.
        Report->SetStringField(TEXT("observedUiText"), Input.IsValid() ? Input->GetText().ToString() : FString());
        Report->SetStringField(TEXT("observedDraftText"), Service.IsValid() ? Service->State().Input.Draft().Text : FString());
        Report->SetStringField(TEXT("observedRevision"), Service.IsValid() ? LexToString(Service->State().Input.Revision()) : FString());
        Report->SetStringField(TEXT("referenceRevision"), LexToString(Revision));
        if (BusyObservation) Report->SetObjectField(TEXT("busyObservation"), BusyObservation.ToSharedRef());
        Report->SetStringField(TEXT("scope"), bExternal ? TEXT("Formal external CLI and actual production tab/Driver, shared busy gate; no assets")
            : TEXT("Real registered production tab, internal service and AutomationDriver; no external IPC or assets"));
        Report->SetArrayField(TEXT("checks"), Rows);
        Test->TestTrue(TEXT("PublishFreshProductionUiReport"), NodeForgePrototype::WriteJson(Report, Output));
        if (bExternal) FPlatformMisc::RequestExitWithStatus(false, All ? 0 : 1);
        return true;
    }
    FAutomationTestBase* Test;
    FString Output, OriginalPath, Original, PreflightReport, FailedPreflightReport, PreparationReport;
    TSharedPtr<FNodeForgeEditorControlService> Service;
    TSharedPtr<FNodeForgeEditorControlChannel> Channel;
    TSharedPtr<SDockTab> Tab;
    TSharedPtr<SMultiLineEditableTextBox> Input;
    TSharedPtr<IAsyncAutomationDriver, ESPMode::ThreadSafe> Driver;
    // UE StepExecutor schedules through a weak owner; retain the sequence until completion.
    TSharedPtr<IAsyncDriverSequence, ESPMode::ThreadSafe> Sequence;
    TAsyncResult<bool> Pending;
    TArray<TSharedPtr<FJsonValue>> Rows;
    TSharedPtr<FJsonObject> BusyObservation;
    double Started, PhaseTime;
    uint64 Revision = 0;
    int32 Phase = 0;
    bool All = true, DriverOwned = false, BusyOwned = false;
    bool bExternal = false;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeProductionUiTest, "NodeForge.EditorProductionUi",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
TSharedPtr<SWidget> NodeForgePanel::FindTaggedForChecks(const TSharedRef<SWidget>& Root, FName Tag)
{
    return FindTagged(Root, Tag);
}
bool FNodeForgeProductionUiTest::RunTest(const FString& Parameters)
{
    FString Output; const FString Root = NodeForgePrototype::WorkspaceDirectory();
    const FString Project = FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
    const FString ProjectName = FPaths::GetBaseFilename(Project);
    const bool UiIntegration = FParse::Param(FCommandLine::Get(), TEXT("NFUiIntegrationChecks")) && ProjectName == TEXT("NFCUiPreview");
    if (IsRunningCommandlet() || !FSlateApplication::IsInitialized()
        || !FParse::Param(FCommandLine::Get(), TEXT("NFEditorUiChecks"))
        || (!UiIntegration && !FPaths::IsUnderDirectory(Project, Root / TEXT("ValidationProject/RegressionRuns")))
        || ProjectName.Len() > 20
        || !(ProjectName.StartsWith(TEXT("NFCUi")) || ProjectName.StartsWith(TEXT("ControlEditorUiState")))
        || !FParse::Value(FCommandLine::Get(), TEXT("NFEditorUiReport="), Output)
        || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Output), UiIntegration
            ? NodeForgeInstallation::StateDirectory() / TEXT("UiIntegrationChecks") : Root / TEXT("Reports/PluginControlStageTwo"))
        || IFileManager::Get().FileExists(*Output))
    { AddError(TEXT("Explicit owned UI checks project, real Editor and fresh second-stage report required")); return false; }
    ADD_LATENT_AUTOMATION_COMMAND(FNodeForgeProductionUiCommand(this, Output)); return true;
}
#endif
