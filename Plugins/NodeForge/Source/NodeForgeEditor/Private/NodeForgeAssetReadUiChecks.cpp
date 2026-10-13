// Two affected production buttons, inside the one explicitly prepared B3 Editor.
#include "NodeForgePanel.h"
#include "NodeForgePrototype.h"
#include "NodeForgeEditorControlService.h"
#include "NodeForgeEditorControlProtocol.h"
#include "NodeForgeResourceSource.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformProcess.h"
#include "Modules/ModuleManager.h"
#include "Materials/Material.h"
#include "UObject/Package.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SWindow.h"
#include "IAutomationDriverModule.h"
#include "IAutomationDriver.h"
#include "IDriverSequence.h"
#include "DriverConfiguration.h"
#include "LocateBy.h"
#include "Async/IAsyncTask.h"
#include "Serialization/JsonSerializer.h"

namespace
{
bool ReadUiJson(const FString& Path,TSharedPtr<FJsonObject>& Json)
{
    FString Normal=FPaths::ConvertRelativePathToFull(Path);FPaths::NormalizeFilename(Normal);
    FString Text,Hash,Error;
    return FPaths::IsUnderDirectory(Normal,NodeForgePrototype::WorkspaceDirectory()/TEXT("Reports"))
        && NodeForgeResourceSource::ReadText(Normal,NodeForgeEditorControl::MaxReportBytes,Text,Hash,Error)
        && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Json) && Json.IsValid();
}
class FAssetReadUiCommand : public IAutomationLatentCommand
{
public:
    FAssetReadUiCommand(FAutomationTestBase& In,const FString& Flag,const FString& Report)
        : Test(In),Ready(Flag),Output(Report),Started(FPlatformTime::Seconds()),At(Started) {}
    ~FAssetReadUiCommand() override {ReleaseDriver();}
    bool Update() override
    {
        if(FPlatformTime::Seconds()-Started>(Phase==0?600:120))
        {Row(TEXT("FiniteUiDeadline"),false);return Finish();}
        switch(Phase)
        {
        case 0:
        {
            if(!IFileManager::Get().FileExists(*Ready))return false;
            TSharedPtr<FJsonObject> Flag,Materials;
            if(!Row(TEXT("NativeGatePassedBeforeUi"),ReadUiJson(Ready,Flag)
                && ReadUiJson(Flag->GetStringField(TEXT("nativeReport")),Materials)
                && Materials->GetStringField(TEXT("status"))==TEXT("passed")))return Finish();
            if(!Row(TEXT("FrozenSourcePlanReadable"),ReadUiJson(NodeForgePrototype::WorkspaceDirectory()/TEXT("Reports/PluginControlStageTwo/AssetRead1/B0/Materials.json"),Materials)))return Finish();
            Source=Materials->GetArrayField(TEXT("sources"))[0]->AsObject()->GetStringField(TEXT("source"));
            UMaterial* Material=FindObject<UMaterial>(nullptr,*Source);
            if(!Row(TEXT("PreviouslyCapturedCleanSourceOnly"),Material && !Material->GetOutermost()->IsDirty()))return Finish();
            TArray<UObject*> Assets;Assets.Add(Material);
            FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser")).Get().SyncBrowserToAssets(Assets,false,false);
            Service=NodeForgePanel::GetProductService();Tab=FGlobalTabmanager::Get()->TryInvokeTab(FName(TEXT("NodeForge")));
            if(!Row(TEXT("ActualProductionTabAndService"),Service && Tab))return Finish();
            Tab->DrawAttention();Input=StaticCastSharedPtr<SMultiLineEditableTextBox>(NodeForgePanel::FindTaggedForChecks(Tab->GetContent(),TEXT("NodeForge.Input")));
            Rule=StaticCastSharedPtr<SExpandableArea>(NodeForgePanel::FindTaggedForChecks(Tab->GetContent(),TEXT("NodeForge.RuleSection")));
            Copy=StaticCastSharedPtr<SExpandableArea>(NodeForgePanel::FindTaggedForChecks(Tab->GetContent(),TEXT("NodeForge.CopySection")));
            Scroll=StaticCastSharedPtr<SScrollBox>(NodeForgePanel::FindTaggedForChecks(Tab->GetContent(),TEXT("NodeForge.AdvancedScroll")));
            if(!Row(TEXT("RealProductionControlsFound"),Input && Rule && Copy && Scroll))return Finish();
            BeforeText=Service->State().Input.Draft().Text;BeforeRevision=Service->State().Input.Revision();
            BeforeBaseline=Service->State().LastExportReportPath;BeforeSuccess=Service->State().LastSuccessReportPath;
            BeforeImport=Service->State().LastImportInputPath;BeforeOrigin=Service->State().Input.Provenance(FString(),FString());
            Rule->SetExpanded(true);Copy->SetExpanded(false);Position(TEXT("NodeForge.InspectSelection"));Next(1);return false;
        }
        case 1:
            if(!Settled())return false;
            if(!EnableDriver() || !Click(TEXT("NodeForge.InspectSelection")))return Finish();Next(2);return false;
        case 2:
            if(!Pending.GetFuture().IsReady())return false;
            if(!Row(TEXT("ActualInspectButtonClicked"),Pending.GetFuture().Get()))return Finish();Next(3);return false;
        case 3:
        {
            if(!Settled())return false;
            TSharedPtr<FJsonObject> Report;InspectReport=Service->State().LastReportPath;
            if(!Row(TEXT("InspectButtonReadsRealSource"),ReadUiJson(InspectReport,Report)
                && Report->GetStringField(TEXT("mode"))==TEXT("MaterialParameters") && Report->GetStringField(TEXT("operationStatus"))==TEXT("passed")
                && Report->GetObjectField(TEXT("resourceRead"))->GetStringField(TEXT("source"))==Source))return Finish();
            if(!Row(TEXT("InspectPreservesDraftOriginRevisionBaselineAndSuccess"),Service->State().Input.Draft().Text==BeforeText
                && Service->State().Input.Revision()==BeforeRevision && Service->State().LastExportReportPath==BeforeBaseline
                && Service->State().LastSuccessReportPath==BeforeSuccess && Service->State().LastImportInputPath==BeforeImport
                && NodeForgeEditorControl::SemanticDigest(Service->State().Input.Provenance(FString(),FString()))==NodeForgeEditorControl::SemanticDigest(BeforeOrigin.ToSharedRef())))return Finish();
            Rule->SetExpanded(false);Copy->SetExpanded(true);Position(TEXT("NodeForge.ExportGraph"));Next(4);return false;
        }
        case 4:
            if(!Settled())return false;
            if(!Click(TEXT("NodeForge.ExportGraph")))return Finish();Next(5);return false;
        case 5:
            if(!Pending.GetFuture().IsReady())return false;
            if(!Row(TEXT("ActualExportButtonClicked"),Pending.GetFuture().Get()))return Finish();Next(6);return false;
        case 6:
        {
            if(!Settled())return false;
            TSharedPtr<FJsonObject> Report;ExportReport=Service->State().LastExportReportPath;
            if(!Row(TEXT("ExportButtonOriginalPayloadAndVerifiedOrigin"),ReadUiJson(ExportReport,Report)
                && Report->GetStringField(TEXT("operationStatus"))==TEXT("passed")
                && Service->State().Input.Draft().Text==Report->GetStringField(TEXT("exportedRequestText"))
                && Service->State().Input.Draft().ExportOrigin && Service->State().Input.Revision()==BeforeRevision+1
                && Input->GetText().ToString()==Service->State().Input.Draft().Text
                && Service->State().Input.Draft().ExportOrigin->GetStringField(TEXT("reportSha256"))==NodeForgePrototype::FileSha256(ExportReport)))return Finish();
            const auto Payload=Report->GetObjectField(TEXT("exportedRequest"));FString Error;
            Row(TEXT("ExportUsesShortNewIdentityWithoutReservationOrGeneration"),Payload->GetStringField(TEXT("requestId")).Len()<=32
                && FPaths::GetCleanFilename(Payload->GetStringField(TEXT("assetPath"))).Len()<=32
                && NodeForgePrototype::ValidateNewTargetIdentity(Source,Payload->GetStringField(TEXT("requestId")),Payload->GetStringField(TEXT("assetPath")),Error)
                && Service->State().LastSuccessReportPath==BeforeSuccess && Service->State().LastImportInputPath==BeforeImport);
            return Finish();
        }
        default:Row(TEXT("UnexpectedUiPhase"),false);return Finish();
        }
    }
private:
    bool Row(const TCHAR* Name,bool Passed){Test.TestTrue(Name,Passed);All &= Passed;auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("case"),Name);R->SetBoolField(TEXT("passed"),Passed);Rows.Add(MakeShared<FJsonValueObject>(R));return Passed;}
    void Next(int32 Value){Phase=Value;At=FPlatformTime::Seconds();if(Value==1)Started=At;}
    bool Settled()const{return FPlatformTime::Seconds()-At>=.5;}
    void Position(const TCHAR* Tag){Scroll->ScrollDescendantIntoView(NodeForgePanel::FindTaggedForChecks(Tab->GetContent(),FName(Tag)),false);}
    bool EnableDriver()
    {
        auto& Module=IAutomationDriverModule::Get();if(!Row(TEXT("DriverNotOwnedElsewhere"),!Module.IsEnabled()))return false;
        Module.Enable();DriverOwned=true;auto Config=MakeShared<FDriverConfiguration,ESPMode::ThreadSafe>();Config->ImplicitWait=FTimespan::FromSeconds(3);
        Driver=Module.CreateAsyncDriver(Config);return true;
    }
    bool Click(const TCHAR* Tag)
    {
        const auto Window=FSlateApplication::Get().FindWidgetWindow(Tab.ToSharedRef());
        const auto Button=NodeForgePanel::FindTaggedForChecks(Tab->GetContent(),FName(Tag));
        if(!Row(TEXT("RealVisibleEnabledButton"),Window && Window->IsVisible() && Button && Button->IsEnabled() && !Service->IsBusy()))return false;
        Window->BringToFront(true);Sequence=Driver->CreateSequence();Sequence->Actions().Click(By::Path(Tag));Pending=Sequence->Perform();return true;
    }
    void ReleaseDriver()
    {
        if(!DriverOwned)return;
        if(Pending.GetFuture().IsValid() && !Pending.GetFuture().IsReady())if(const auto Task=Pending.GetTask())Task->Cancel();
        Sequence.Reset();Driver.Reset();IAutomationDriverModule::Get().Disable();DriverOwned=false;
    }
    bool Finish()
    {
        ReleaseDriver();Row(TEXT("DriverReleased"),!IAutomationDriverModule::Get().IsEnabled());auto Report=MakeShared<FJsonObject>();
        Report->SetStringField(TEXT("status"),All?TEXT("passed"):TEXT("failed"));Report->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());
        Report->SetStringField(TEXT("moduleSha256"),NodeForgePrototype::FileSha256(FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"))));
        Report->SetStringField(TEXT("inspectReport"),InspectReport);Report->SetStringField(TEXT("exportReport"),ExportReport);Report->SetArrayField(TEXT("cases"),Rows);
        Report->SetStringField(TEXT("scope"),TEXT("Two actual production buttons via AutomationDriver; selection/expansion positioned by native UI APIs. No generation, source save or channel shutdown. MCP next explicitly loads original prepared export."));
        Test.TestTrue(TEXT("FreshUiReportSaved"),NodeForgePrototype::WriteJson(Report,Output));return true;
    }
    FAutomationTestBase& Test;FString Ready,Output,Source,BeforeText,BeforeBaseline,BeforeSuccess,BeforeImport,InspectReport,ExportReport;
    TSharedPtr<FNodeForgeEditorControlService> Service;TSharedPtr<SDockTab> Tab;TSharedPtr<SMultiLineEditableTextBox> Input;
    TSharedPtr<SExpandableArea> Rule,Copy;TSharedPtr<SScrollBox> Scroll;TSharedPtr<FJsonObject> BeforeOrigin;
    TSharedPtr<IAsyncAutomationDriver,ESPMode::ThreadSafe> Driver;TSharedPtr<IAsyncDriverSequence,ESPMode::ThreadSafe> Sequence;TAsyncResult<bool> Pending;
    TArray<TSharedPtr<FJsonValue>> Rows;double Started,At;uint64 BeforeRevision=0;int32 Phase=0;bool All=true,DriverOwned=false;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeAssetReadUiTest,"NodeForge.AssetReadUi",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNodeForgeAssetReadUiTest::RunTest(const FString& Parameters)
{
    FString Ready,Output;const FString Root=NodeForgePrototype::WorkspaceDirectory();
    if(IsRunningCommandlet() || !FSlateApplication::IsInitialized() || FPaths::GetBaseFilename(FPaths::GetProjectFilePath())!=TEXT("NFCUiApiLab")
        || !FParse::Value(FCommandLine::Get(),TEXT("NFAssetReadUiReady="),Ready) || !FParse::Value(FCommandLine::Get(),TEXT("NFAssetReadUiReport="),Output)
        || !FPaths::IsUnderDirectory(Ready,Root/TEXT("Reports/PluginControlStageTwo/AssetRead1"))
        || !FPaths::IsUnderDirectory(Output,Root/TEXT("Reports/PluginControlStageTwo/AssetRead1")) || IFileManager::Get().FileExists(*Output))
    {AddError(TEXT("Prepared finite B3 UI inputs, fresh report and real reused Editor required"));return false;}
    AddCommand(new FAssetReadUiCommand(*this,Ready,Output));return true;
}
#endif
