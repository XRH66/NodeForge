// Finite integration check of the registered first-use production tab, not a separate UI.
#include "NodeForgePanel.h"
#include "NodeForgePrototype.h"
#include "NodeForgeInstallation.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "Modules/ModuleManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/SWindow.h"
#include "IAutomationDriverModule.h"
#include "IAutomationDriver.h"
#include "IDriverSequence.h"
#include "DriverConfiguration.h"
#include "LocateBy.h"
#include "Async/IAsyncTask.h"
#include "ImageUtils.h"

namespace
{
class FInstallationUiCommand : public IAutomationLatentCommand
{
public:
    FInstallationUiCommand(FAutomationTestBase* InTest, FString InOutput)
        : Test(InTest), Output(MoveTemp(InOutput)), Started(FPlatformTime::Seconds()) {}
    virtual ~FInstallationUiCommand() override { ReleaseDriver(); }
    virtual bool Update() override
    {
        if (FPlatformTime::Seconds() - Started > 40) { Row(TEXT("Deadline"), false); return Finish(); }
        FString Error;
        if (Phase == 0)
        {
            Tab = FGlobalTabmanager::Get()->TryInvokeTab(FName(TEXT("NodeForge")));
            if (!Row(TEXT("RealRegisteredTab"), Tab.IsValid())) return Finish();
            Tab->DrawAttention();
            const auto Confirm = NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), TEXT("NodeForge.InstallationConfirm"));
            if (!Row(TEXT("FirstUseBeforeFunctionalPanel"), Confirm.IsValid() && Confirm->IsEnabled()
                && !NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), TEXT("NodeForge.Input")).IsValid())) return Finish();
            NodeForgeInstallation::FBinding Previous; Previous.Project = TEXT("stale");
            if (!Row(TEXT("MissingBindingClearsPriorIdentity"), !NodeForgeInstallation::Read(Previous, Error) && Previous.Project.IsEmpty())) return Finish();
            NodeForgeInstallation::FBinding Preview;
            if (!Row(TEXT("CurrentIdentityDetectedWithoutConfig"), NodeForgeInstallation::InspectCurrent(Preview, Error)
                && FPaths::IsSamePath(Preview.PluginDirectory, FPaths::ProjectDir() / TEXT("Plugins/NodeForge"))
                && !FPaths::FileExists(FPaths::ProjectDir() / TEXT("Config/NodeForgeInstallation.json")))) return Finish();
            Phase = 1; Changed = FPlatformTime::Seconds(); return false;
        }
        if (Phase == 1)
        {
            if (FPlatformTime::Seconds() - Changed < 1) return false;
            const auto Window = FSlateApplication::Get().FindWidgetWindow(Tab.ToSharedRef());
            if (!Row(TEXT("OwnedNativeWindowVisible"), Window.IsValid() && Window->IsVisible() && Window->GetNativeWindow().IsValid())) return Finish();
            Window->BringToFront(true);
            if (!Picture(TEXT("FirstUse.png"))) return Finish();
            auto& Module = IAutomationDriverModule::Get();
            if (!Row(TEXT("DriverNotOwnedElsewhere"), !Module.IsEnabled())) return Finish();
            Module.Enable(); DriverOwned = true;
            auto Config = MakeShared<FDriverConfiguration, ESPMode::ThreadSafe>(); Config->ImplicitWait = FTimespan::FromSeconds(3);
            Driver = Module.CreateAsyncDriver(Config); Sequence = Driver->CreateSequence();
            Sequence->Actions().Click(By::Path(TEXT("NodeForge.InstallationConfirm"))); Pending = Sequence->Perform();
            Phase = 2; return false;
        }
        if (Phase == 2)
        {
            if (!Pending.GetFuture().IsReady()) return false;
            if (!Row(TEXT("ActualConfirmButtonClicked"), Pending.GetFuture().Get())) return Finish();
            ReleaseDriver(); Phase = 3; Changed = FPlatformTime::Seconds(); return false;
        }
        if (Phase == 3)
        {
            if (FPlatformTime::Seconds() - Changed < 1) return false;
            NodeForgeInstallation::FBinding Binding; FString Catalog;
            if (!Row(TEXT("OriginalNativeReadAndCatalogPassed"), NodeForgeInstallation::Read(Binding, Error)
                && NodeForgeInstallation::CatalogIdentity(Catalog, Error))) return Finish();
            BindingSha = NodeForgePrototype::FileSha256(FPaths::ProjectDir() / TEXT("Config/NodeForgeInstallation.json"));
            if (!Row(TEXT("FunctionalPanelEntered"), NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), TEXT("NodeForge.Input")).IsValid()
                && !NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), TEXT("NodeForge.InstallationConfirm")).IsValid())) return Finish();
            if (!Picture(TEXT("FunctionalPanel.png"))) return Finish();
            FString Receipt;
            if (!Row(TEXT("RepeatedConfirmationDoesNotOverwrite"), !NodeForgeInstallation::ConfirmCurrent(Receipt, Error)
                && Error == TEXT("installationExistingBindingConflict")
                && NodeForgePrototype::FileSha256(FPaths::ProjectDir() / TEXT("Config/NodeForgeInstallation.json")) == BindingSha)) return Finish();
            if (!Row(TEXT("NormalTabClose"), Tab->RequestCloseTab())) return Finish();
            Tab.Reset(); Phase = 4; Changed = FPlatformTime::Seconds(); return false;
        }
        if (Phase == 4)
        {
            if (FPlatformTime::Seconds() - Changed < 1) return false;
            Tab = FGlobalTabmanager::Get()->TryInvokeTab(FName(TEXT("NodeForge")));
            Row(TEXT("ReopenSkipsSetupAndKeepsBinding"), Tab.IsValid()
                && NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), TEXT("NodeForge.Input")).IsValid()
                && !NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), TEXT("NodeForge.InstallationConfirm")).IsValid()
                && NodeForgePrototype::FileSha256(FPaths::ProjectDir() / TEXT("Config/NodeForgeInstallation.json")) == BindingSha);
            return Finish();
        }
        return false;
    }
private:
    bool Row(const TCHAR* Name, bool Passed)
    {
        All &= Passed; Test->TestTrue(Name, Passed); auto R = MakeShared<FJsonObject>();
        R->SetStringField(TEXT("case"), Name); R->SetBoolField(TEXT("passed"), Passed); Rows.Add(MakeShared<FJsonValueObject>(R)); return Passed;
    }
    bool Picture(const TCHAR* Name)
    {
        const FString Path = FPaths::GetPath(Output) / Name;
        TArray<FColor> Pixels; FIntVector Size = FIntVector::ZeroValue;
        if (!Row(TEXT("CaptureActualTab"), !FPaths::FileExists(Path) && FSlateApplication::Get().TakeScreenshot(Tab->GetContent(), Pixels, Size))) return false;
        TArray64<uint8> Bytes; FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Bytes);
        return Row(TEXT("SaveFreshScreenshot"), Bytes.Num() > 0 && FFileHelper::SaveArrayToFile(Bytes, *Path));
    }
    void ReleaseDriver()
    {
        if (!DriverOwned) return;
        if (Pending.GetFuture().IsValid() && !Pending.GetFuture().IsReady()) if (const auto Task = Pending.GetTask()) Task->Cancel();
        Sequence.Reset(); Driver.Reset(); IAutomationDriverModule::Get().Disable(); DriverOwned = false;
    }
    bool Finish()
    {
        ReleaseDriver(); Row(TEXT("DriverReleased"), !IAutomationDriverModule::Get().IsEnabled());
        auto Report = MakeShared<FJsonObject>(); Report->SetStringField(TEXT("status"), All ? TEXT("passed") : TEXT("failed"));
        Report->SetStringField(TEXT("project"), FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()));
        Report->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
        const FString Module = FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"));
        Report->SetStringField(TEXT("module"), Module); Report->SetStringField(TEXT("moduleSha256"), NodeForgePrototype::FileSha256(Module));
        Report->SetStringField(TEXT("bindingSha256"), BindingSha); Report->SetArrayField(TEXT("checks"), Rows);
        Test->TestTrue(TEXT("PublishFreshReport"), NodeForgePrototype::WriteJson(Report, Output));
        FPlatformMisc::RequestExitWithStatus(false, All ? 0 : 1); return true;
    }
    FAutomationTestBase* Test; FString Output, BindingSha;
    TSharedPtr<SDockTab> Tab;
    TSharedPtr<IAsyncAutomationDriver, ESPMode::ThreadSafe> Driver;
    TSharedPtr<IAsyncDriverSequence, ESPMode::ThreadSafe> Sequence;
    TAsyncResult<bool> Pending; TArray<TSharedPtr<FJsonValue>> Rows;
    double Started, Changed = 0; int32 Phase = 0; bool All = true, DriverOwned = false;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInstallationUiTest, "NodeForge.InstallationFirstUse", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInstallationUiTest::RunTest(const FString&)
{
    FString Output;
    const FString Project = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
    if (IsRunningCommandlet() || !FSlateApplication::IsInitialized()
        || !FParse::Param(FCommandLine::Get(), TEXT("NFInstallationUiChecks"))
        || FPaths::GetBaseFilename(FPaths::GetProjectFilePath()) != TEXT("NFCInstall")
        || !FParse::Value(FCommandLine::Get(), TEXT("NFReport="), Output)
        || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Output), Project / TEXT("Saved/NodeForge/InstallationChecks"))
        || FPaths::FileExists(Output))
    { AddError(TEXT("Explicit fresh owned NFCInstall Editor and new bounded report required")); return false; }
    ADD_LATENT_AUTOMATION_COMMAND(FInstallationUiCommand(this, Output)); return true;
}
#endif
