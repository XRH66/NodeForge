#include "NodeForgeEditorControlChannel.h"
#include "NodeForgeEditorControlService.h"
#include "NodeForgePanel.h"
#include "NodeForgePrototype.h"
#include "NodeForgeTaskRules.h"
#include "NodeForgeResultView.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Windows/WindowsHWrapper.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWindow.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetData.h"
#include "UObject/Package.h"
#include "UObject/UObjectIterator.h"
#include "ImageUtils.h"
#include "IAutomationDriverModule.h"
#include "IAutomationDriver.h"
#include "IDriverSequence.h"
#include "DriverConfiguration.h"
#include "LocateBy.h"
#include "Async/IAsyncTask.h"

namespace
{
class FNodeForgeChannelIdentityCommand : public IAutomationLatentCommand
{
public:
    FNodeForgeChannelIdentityCommand(FAutomationTestBase* InTest, FString InOutput)
        : Test(InTest), Output(MoveTemp(InOutput)), Started(FPlatformTime::Seconds()) {}
    bool Update() override
    {
        if (bFinishing) return Finish();
        if (!bReady)
        {
            Channel = NodeForgePanel::GetControlChannel();
            if (!Row(TEXT("ExplicitProductChannelStarted"), Channel.IsValid())) return Finish();
            QueuedProbe = FParse::Param(FCommandLine::Get(), TEXT("NFEditorControlQueuedStopChecks"));
            RunningProbe = FParse::Param(FCommandLine::Get(), TEXT("NFEditorControlRunningStopChecks"));
            Service = NodeForgePanel::GetProductService();
            GuiGenerate = FParse::Param(FCommandLine::Get(), TEXT("NFEditorControlGuiGenerateChecks"));
            if (GuiGenerate && FParse::Value(FCommandLine::Get(), TEXT("NFEditorControlGuiCaseCount="), GraphCaseCount)
                && !Row(TEXT("ExplicitGuiCaseCountBounded"), GraphCaseCount == 1 || GraphCaseCount == 2)) return Finish();
            BusinessUi = FParse::Param(FCommandLine::Get(), TEXT("NFEditorControlBusinessUiChecks"));
            AuxUi = FParse::Param(FCommandLine::Get(), TEXT("NFEditorControlAuxUiChecks"));
            QueryUi = FParse::Param(FCommandLine::Get(), TEXT("NFEditorControlQueryUiChecks"));
            GraphUi = FParse::Param(FCommandLine::Get(), TEXT("NFEditorControlGraphUiChecks")) || GuiGenerate || BusinessUi || AuxUi || QueryUi;
            AssetBoundary = FParse::Param(FCommandLine::Get(), TEXT("NFEditorControlAssetBoundaryChecks"));
            if (GraphUi)
            {
                if (!Row(TEXT("GraphUiHasProductServiceAndSlate"), Service.IsValid() && FSlateApplication::IsInitialized())) return Finish();
                Tab = FGlobalTabmanager::Get()->TryInvokeTab(FName(TEXT("NodeForge")));
                if (!Row(TEXT("GraphUiUsesRealRegisteredProductionTab"), Tab.IsValid())) return Finish();
                Tab->DrawAttention();
                Input = StaticCastSharedPtr<SMultiLineEditableTextBox>(NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), TEXT("NodeForge.Input")));
                Summary = StaticCastSharedPtr<STextBlock>(NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), TEXT("NodeForge.InputSource")));
                if (!Row(TEXT("GraphUiActualInputAndResultWidgetsFound"), Input.IsValid() && Summary.IsValid())) return Finish();
                if (GuiGenerate || BusinessUi || AuxUi)
                {
                    auto& DriverModule = IAutomationDriverModule::Get();
                    if (!Row(TEXT("GuiDriverNotOwnedByAnotherTest"), !DriverModule.IsEnabled())) return Finish();
                    DriverModule.Enable(); DriverOwned = true;
                    auto Config = MakeShared<FDriverConfiguration, ESPMode::ThreadSafe>();
                    Config->ImplicitWait = FTimespan::FromSeconds(3);
                    Driver = DriverModule.CreateAsyncDriver(Config);
                }
            }
            if (QueuedProbe)
                Channel->SetBeforeDrainProbeForChecks([this]()
                {
                    ++ProbeCount; Channel->RequestStop(); Row(TEXT("QueuedStopReturnsWithoutJoiningGt"), !Channel->Stop());
                    Channel->Drain(*Service); Row(TEXT("QueuedNestedDrainCannotReenter"), ProbeCount == 1);
                });
            if (RunningProbe)
                Service->SetControlProbeForChecks([this]()
                {
                    ++ProbeCount; Channel->RequestStop(); Row(TEXT("RunningStopReturnsWithoutJoiningGt"), !Channel->Stop());
                    Channel->Drain(*Service); FString Error;
                    Row(TEXT("RunningNativeProbeRejectsNestedUiAndDrain"), ProbeCount == 1 && !Service->EditFromUI(TEXT("blocked"))
                        && !Service->RestoreDraft(Error) && Service->State().Input.Revision() == 0);
                });
            Identity = Channel->PublicIdentity();
            auto Restricted = MakeShared<FJsonObject>();
            if (!QueryUi)
            {
                const bool Denied = Channel->CheckRestrictedConnection(Restricted);
                Report->SetObjectField(TEXT("restrictedConnection"), Restricted);
                if (!Row(TEXT("ActualRestrictedTokenConnectDeniedAndReverted"), Denied)) return Finish();
            }
            else Report->SetStringField(TEXT("unchangedAclEvidence"), TEXT("Reports/PluginControlStageTwo/EditorControl1/S5/FinalReport.md"));
            Row(TEXT("NativeIdentityMatchesCurrentPidAndModule"), Identity->GetNumberField(TEXT("pid")) == FPlatformProcess::GetCurrentProcessId()
                && Identity->GetStringField(TEXT("binarySha256")) == NodeForgePrototype::FileSha256(FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"))));
            Report->SetObjectField(TEXT("identity"), Identity.ToSharedRef());
            auto Ready = MakeShared<FJsonObject>(); Ready->SetStringField(TEXT("operationStatus"), TEXT("ready"));
            Ready->SetObjectField(TEXT("identity"), Identity.ToSharedRef());
            Ready->SetObjectField(TEXT("restrictedConnection"), Restricted);
            if (!Row(TEXT("FreshPublicReadyWritten"), NodeForgePrototype::WriteJson(Ready, FPaths::GetPath(Output) / TEXT("ClientReady.json")))) return Finish();
            bReady = true; return false;
        }
        const FString DonePath = FPaths::GetPath(Output) / TEXT("ClientDone.json");
        if (QueryUi && !QueryBoundaryDone && IFileManager::Get().FileExists(*(FPaths::GetPath(Output) / TEXT("QueryBoundaryReady.json"))))
            return ObserveQueryBoundaries();
        if (QueryUi && QueryCase < 2 && !IFileManager::Get().FileExists(*DonePath)) return ObserveQueryUi();
        if (AuxUi && AuxCase < 10 && !IFileManager::Get().FileExists(*DonePath)) return ObserveAuxUi();
        if (BusinessUi && BusinessCase < 10 && !IFileManager::Get().FileExists(*DonePath)) return ObserveBusinessUi();
        if (GraphUi && !BusinessUi && !AuxUi && !QueryUi && GraphCase < GraphCaseCount && !(GuiGenerate ? ObserveGuiGenerate() : ObserveGraphUi())) return false;
        if (IFileManager::Get().FileExists(*DonePath))
        {
            FString Text; TSharedPtr<FJsonObject> Done;
            const bool Valid = FFileHelper::LoadFileToString(Text, *DonePath) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Done) && Done
                && Done->GetStringField(TEXT("controlSessionId")) == Identity->GetStringField(TEXT("controlSessionId"));
            Row(TEXT("ExternalClientBatchFinishedForThisSession"), Valid && Done->GetStringField(TEXT("operationStatus")) == TEXT("passed"));
            if (QueryUi) Row(TEXT("BothQueryUiProofsAndBoundariesCompleted"), QueryCase == 2 && QueryBoundaryDone);
            else if (AuxUi) Row(TEXT("AllFixedAuxUiProofsCompleted"), AuxCase == 10);
            else if (BusinessUi) Row(TEXT("AllFixedBusinessUiProofsCompleted"), BusinessCase == 10);
            else if (GraphUi) Row(TEXT("RequestedGraphProductionUiProofsCompleted"), GraphCase == GraphCaseCount);
            if (QueuedProbe || RunningProbe)
                Row(TEXT("RealDispatchStopProbeFinishedOnceWithExpectedDraft"), ProbeCount == 1
                    && Service->State().Input.Revision() == (RunningProbe ? 1 : 0)
                    && Service->State().Input.Draft().Text == (RunningProbe ? TEXT("{\"mechanism\":true}") : TEXT("")));
            return Finish();
        }
        if (FPlatformTime::Seconds() - Started >= ((GraphUi || AssetBoundary) ? 600 : 120)) { Row(TEXT("ExternalClientBatchBoundedDeadline"), false); return Finish(); }
        return false;
    }
private:
    bool ObserveQueryBoundaries()
    {
        const FString Directory = FPaths::GetPath(Output);
        FString Text; TSharedPtr<FJsonObject> Ready;
        if (!FFileHelper::LoadFileToString(Text, *(Directory / TEXT("QueryBoundaryReady.json")))
            || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Ready) || !Ready) return false;
        if (!Row(TEXT("QueryBoundaryMarkerBelongsToSession"), Ready->GetStringField(TEXT("controlSessionId"))
            == Identity->GetStringField(TEXT("controlSessionId")))) return Finish();
        Row(TEXT("RejectedQueryClearsPreviousReadOnlyData"), Service->State().LastReadOnlyData.IsEmpty());
        const FString Root = TEXT("/Game/NodeForgeTests/"); FString Error;
        const TArray<FString> Legal = {Root + TEXT("Sample01"), Root + TEXT("Material/Sample01"),
            Root + TEXT("Material/B000001/Sample01"), Root + FString::ChrN(64, 'A')};
        for (int32 I = 0; I < Legal.Num(); ++I)
            Row(*(TEXT("LegalTarget") + LexToString(I)), NodeForgePrototype::ValidateTargetPath(Legal[I], Error));
        const TArray<FString> Illegal = {TEXT("/Game/NodeForgeTestsOther/A"), TEXT("/Engine/A"), Root,
            Root + TEXT("/A"), Root + TEXT("Material//A"), Root + TEXT("Material/A/"), Root + TEXT("./A"),
            Root + TEXT("../A"), Root + TEXT("A\\B"), Root + TEXT("A B"), Root + TEXT("A_B"),
            Root + FString::ChrN(17, 'A') + TEXT("/B"), Root + TEXT("A/B/C/D"), Root + FString::ChrN(65, 'A')};
        for (int32 I = 0; I < Illegal.Num(); ++I)
            Row(*(TEXT("IllegalTarget") + LexToString(I)), !NodeForgePrototype::ValidateTargetPath(Illegal[I], Error));
        Row(TEXT("TotalFilenameLengthRejected"), !NodeForgePrototype::ValidateTargetFilePath(
            FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()) / FString::ChrN(260, 'A') + TEXT(".uasset"), Error));
        for (const TCHAR* Mode : {TEXT("GraphRules"), TEXT("GraphCatalog")})
        {
            const FString Path = Ready->GetStringField(Mode);
            TSharedPtr<FJsonObject> Native;
            bool Valid = FPaths::IsUnderDirectory(Path, Directory) && FFileHelper::LoadFileToString(Text, *Path)
                && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Native) && Native;
            int32 Code = 5;
            if (!Row(*(FString(Mode) + TEXT("ActualReportAccepted")), Valid
                && FNodeForgeEditorControlService::ValidateQueryReport(Mode, 0, Native, Code, Error) && Code == 0)) return Finish();
            for (const TCHAR* Missing : {TEXT("mode"), TEXT("pid"), TEXT("project"), TEXT("catalogDigest"), TEXT("reportVersion")})
            {
                auto Bad = MakeShared<FJsonObject>(); Bad->Values = Native->Values; Bad->RemoveField(Missing);
                Row(*(FString(Mode) + TEXT("Missing") + Missing),
                    !FNodeForgeEditorControlService::ValidateQueryReport(Mode, 0, Bad, Code, Error) && Code == 5);
            }
            auto WrongPid = MakeShared<FJsonObject>(); WrongPid->Values = Native->Values;
            WrongPid->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId() + 1);
            Row(*(FString(Mode) + TEXT("WrongPidUnknown")), !FNodeForgeEditorControlService::ValidateQueryReport(Mode, 0, WrongPid, Code, Error));
            auto WrongMode = MakeShared<FJsonObject>(); WrongMode->Values = Native->Values;
            WrongMode->SetStringField(TEXT("mode"), TEXT("Import"));
            Row(*(FString(Mode) + TEXT("WrongModeUnknown")), !FNodeForgeEditorControlService::ValidateQueryReport(Mode, 0, WrongMode, Code, Error));
            auto WrongProject = MakeShared<FJsonObject>(); WrongProject->Values = Native->Values;
            WrongProject->SetStringField(TEXT("project"), TEXT("C:/NodeForgeExample/Unknown.uproject"));
            Row(*(FString(Mode) + TEXT("WrongProjectUnknown")), !FNodeForgeEditorControlService::ValidateQueryReport(Mode, 0, WrongProject, Code, Error));
            auto WrongDigest = MakeShared<FJsonObject>(); WrongDigest->Values = Native->Values;
            WrongDigest->SetStringField(TEXT("catalogDigest"), FString::ChrN(64, 'z'));
            Row(*(FString(Mode) + TEXT("WrongDigestUnknown")), !FNodeForgeEditorControlService::ValidateQueryReport(Mode, 0, WrongDigest, Code, Error));
            if (FString(Mode) == TEXT("GraphCatalog"))
            {
                for (const FString& Query : {FString(TEXT("VectorParameter")) + FString::ChrN(241, ' '),
                    FString(TEXT("VectorParameter VectorParameter VectorParameter VectorParameter VectorParameter VectorParameter VectorParameter VectorParameter"))})
                {
                    auto Config = MakeShared<FJsonObject>(); Config->SetStringField(TEXT("assetKind"), TEXT("Material"));
                    Config->SetStringField(TEXT("query"), Query); TSharedPtr<FJsonObject> Bundle;
                    Row(Query.Len() == 256 ? TEXT("Exactly256CharactersAccepted") : TEXT("ExactlyEightTermsAccepted"),
                        NodeForgeTaskRules::Select(Config, Native->GetObjectField(TEXT("catalog")).ToSharedRef(),
                            Native->GetStringField(TEXT("catalogDigest")), Bundle, Error) && Bundle
                            && Bundle->GetNumberField(TEXT("matchedNodeCount")) > 0);
                }
            }
        }
        for (const TCHAR* Case : {TEXT("Flat"), TEXT("Subdirectory")})
        {
            const FString SourcePath = Ready->GetStringField(FString(TEXT("Copy")) + Case);
            const FString CopyReport = Directory / (FString(Case) + TEXT("CopyBoundary.json"));
            TSharedPtr<FJsonObject> Source, Copy;
            const bool SourceRead = FPaths::IsUnderDirectory(SourcePath, Directory)
                && FFileHelper::LoadFileToString(Text, *SourcePath)
                && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Source) && Source;
            const FString SourceHash = NodeForgePrototype::FileSha256(SourcePath);
            const int32 CopyCode = SourceRead ? NodeForgePrototype::Execute(TEXT("PrepareNewCopy"), SourcePath, CopyReport) : 5;
            bool Good = CopyCode == 0 && FFileHelper::LoadFileToString(Text, *CopyReport)
                && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Copy) && Copy;
            TSharedPtr<FJsonObject> Prepared;
            if (Good)
            {
                const auto Evidence = Copy->GetObjectField(TEXT("copyPreparation"));
                Good = FPackageName::GetLongPackagePath(Evidence->GetStringField(TEXT("newAssetPath")))
                    == FPackageName::GetLongPackagePath(Source->GetStringField(TEXT("assetPath")))
                    && Evidence->GetStringField(TEXT("newRequestId")) != Source->GetStringField(TEXT("requestId"))
                    && !Evidence->GetBoolField(TEXT("assetGenerated")) && !Evidence->GetBoolField(TEXT("targetReserved"))
                    && NodeForgePrototype::FileSha256(SourcePath) == SourceHash
                    && FFileHelper::LoadFileToString(Text, *Evidence->GetStringField(TEXT("preparedInputPath")))
                    && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Prepared) && Prepared;
                if (Good)
                {
                    Prepared->SetStringField(TEXT("assetPath"), Source->GetStringField(TEXT("assetPath")));
                    Prepared->SetStringField(TEXT("requestId"), Source->GetStringField(TEXT("requestId")));
                    FString A, B; FJsonSerializer::Serialize(Source.ToSharedRef(), TJsonWriterFactory<>::Create(&A));
                    FJsonSerializer::Serialize(Prepared.ToSharedRef(), TJsonWriterFactory<>::Create(&B));
                    Good = A == B;
                }
            }
            Row(*(FString(Case) + TEXT("CopyKeepsParentAndOnlyChangesIdentity")), Good);
        }
        auto Fixture = MakeShared<FNodeForgeEditorControlService>();
        auto& State = Fixture->MutableState(); State.LastReadOnlyData = TEXT("old query");
        State.LastSuccessReportPath = TEXT("fixtureSuccess"); State.LastImportInputPath = TEXT("fixtureInput");
        State.LastImportInputDigest = TEXT("fixtureDigest"); State.LastExportReportPath = TEXT("fixtureExport");
        State.LastReportPath = Directory / TEXT("AbsentQuery.json");
        TSharedPtr<FJsonObject> Missing;
        Row(TEXT("MissingReportClearsOnlyReadResult"), !Fixture->ReadCurrentReport(Missing)
            && State.LastReadOnlyData.IsEmpty() && State.LastSuccessReportPath == TEXT("fixtureSuccess")
            && State.LastImportInputPath == TEXT("fixtureInput") && State.LastImportInputDigest == TEXT("fixtureDigest")
            && State.LastExportReportPath == TEXT("fixtureExport"));
        const FString Corrupt = Directory / TEXT("CorruptQuery.json");
        Row(TEXT("CorruptFixtureCreated"), !IFileManager::Get().FileExists(*Corrupt)
            && FFileHelper::SaveStringToFile(TEXT("{broken"), *Corrupt, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
        State.LastReportPath = Corrupt; State.LastReadOnlyData = TEXT("old query");
        Row(TEXT("CorruptReportCannotBecomeSuccess"), !Fixture->ReadCurrentReport(Missing) && State.LastReadOnlyData.IsEmpty());
        Fixture->ResetResult();
        Row(TEXT("QueryDisplayResetPreservesSuccessAndExport"), State.LastSuccessReportPath == TEXT("fixtureSuccess")
            && State.LastImportInputPath == TEXT("fixtureInput") && State.LastImportInputDigest == TEXT("fixtureDigest")
            && State.LastExportReportPath == TEXT("fixtureExport"));
        auto Proof = MakeShared<FJsonObject>(); Proof->SetStringField(TEXT("operationStatus"), bAll ? TEXT("passed") : TEXT("failed"));
        Proof->SetStringField(TEXT("controlSessionId"), Identity->GetStringField(TEXT("controlSessionId")));
        Proof->SetArrayField(TEXT("checks"), Rows); QueryBoundaryDone = true;
        if (!Row(TEXT("QueryBoundariesPublished"), NodeForgePrototype::WriteJson(Proof, Directory / TEXT("QueryBoundaryDone.json")))) return Finish();
        if (!bAll) return Finish();
        return false;
    }
    bool ObserveQueryUi()
    {
        const FString Case = QueryCase == 0 ? TEXT("QueryRules") : TEXT("QueryCatalog");
        const FString Tag = QueryCase == 0 ? TEXT("NodeForge.RulesMaterial") : TEXT("NodeForge.Catalog");
        const FString Directory = FPaths::GetPath(Output); FString Text; TSharedPtr<FJsonObject> Marker;
        if (GuiPublished)
        {
            if (!FFileHelper::LoadFileToString(Text, *(Directory / (Case + TEXT("UiChecked.json"))))
                || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Marker) || !Marker) return false;
            if (!Row(*(Case + TEXT("IndependentReceiptConfirmed")), Marker->GetStringField(TEXT("controlSessionId"))
                == Identity->GetStringField(TEXT("controlSessionId")) && Marker->GetStringField(TEXT("operationStatus")) == TEXT("passed")
                && Marker->GetStringField(TEXT("nativeReportSha256")) == NodeForgePrototype::FileSha256(Service->State().LastReportPath))) return Finish();
            if (!Row(*(Case + TEXT("UiCheckAccepted")), NodeForgePrototype::WriteJson(Marker.ToSharedRef(), Directory / (Case + TEXT("UiCheckedAccepted.json"))))) return Finish();
            ++QueryCase; GuiPublished = false; GuiClickStarted = false; GuiClickCompleted = false;
            GuiObservedAt = 0; BusinessBefore.Reset(); Sequence.Reset();
            return false;
        }
        if (!IFileManager::Get().FileExists(*(Directory / (Case + TEXT("UiReady.json"))))) return false;
        if (!GuiClickStarted)
        {
            if (GuiObservedAt == 0)
            {
                auto Area = StaticCastSharedPtr<SExpandableArea>(NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), TEXT("NodeForge.RuleSection")));
                auto Scroll = StaticCastSharedPtr<SScrollBox>(NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), TEXT("NodeForge.AdvancedScroll")));
                if (!Row(*(Case + TEXT("ActualRuleSectionFound")), Area.IsValid() && Scroll.IsValid())) return Finish();
                Area->SetExpanded(true); Scroll->ScrollDescendantIntoView(NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), FName(*Tag)), false);
                GuiObservedAt = FPlatformTime::Seconds(); return false;
            }
            if (FPlatformTime::Seconds() - GuiObservedAt < 0.5) return false;
            if (!FFileHelper::LoadFileToString(Text, *(Directory / (Case + TEXT("UiReady.json"))))
                || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Marker) || !Marker) return false;
            const auto Window = FSlateApplication::Get().FindWidgetWindow(Tab.ToSharedRef());
            if (!Row(*(Case + TEXT("OwnedVisibleProductionDraft")), Window.IsValid() && Window->IsVisible() && !Service->IsBusy()
                && Marker->GetStringField(TEXT("controlSessionId")) == Identity->GetStringField(TEXT("controlSessionId"))
                && Marker->GetStringField(TEXT("draftRevision")) == LexToString(Service->State().Input.Revision())
                && Marker->GetStringField(TEXT("inputDigest")) == NodeForgePrototype::TextSha256(Input->GetText().ToString())
                && Input->GetText().ToString() == Service->State().Input.Draft().Text)) return Finish();
            if (!Row(TEXT("QueryDriverAvailable"), !IAutomationDriverModule::Get().IsEnabled())) return Finish();
            Window->BringToFront(true); BusinessBefore = BusinessSnapshot();
            IAutomationDriverModule::Get().Enable(); DriverOwned = true;
            auto Config = MakeShared<FDriverConfiguration, ESPMode::ThreadSafe>(); Config->ImplicitWait = FTimespan::FromSeconds(3);
            Driver = IAutomationDriverModule::Get().CreateAsyncDriver(Config);
            Sequence = Driver->CreateSequence(); Sequence->Actions().Click(By::Path(Tag));
            Pending = Sequence->Perform(); GuiClickStarted = true; return false;
        }
        if (!GuiClickCompleted)
        {
            if (!Pending.GetFuture().IsReady()) return false;
            if (!Row(*(Case + TEXT("ActualDriverClickCompleted")), Pending.GetFuture().Get())) return Finish();
            GuiClickCompleted = true; GuiCompletedAt = FPlatformTime::Seconds(); return false;
        }
        const auto After = BusinessSnapshot();
        const bool Reflected = !Service->IsBusy() && After->GetStringField(TEXT("nativeReport")) != BusinessBefore->GetStringField(TEXT("nativeReport"))
            && Summary->GetText().ToString() == Service->State().ResultText && !Service->State().LastReadOnlyData.IsEmpty();
        if (!Reflected && FPlatformTime::Seconds() - GuiCompletedAt < 3) return false;
        if (!Row(*(Case + TEXT("RealResultAndDraftPreserved")), Reflected
            && After->GetStringField(TEXT("draftRevision")) == BusinessBefore->GetStringField(TEXT("draftRevision"))
            && After->GetStringField(TEXT("inputDigest")) == BusinessBefore->GetStringField(TEXT("inputDigest")))) return Finish();
        auto Proof = MakeShared<FJsonObject>(); Proof->SetStringField(TEXT("operationStatus"), TEXT("passed"));
        Proof->SetStringField(TEXT("source"), TEXT("actualGui")); Proof->SetStringField(TEXT("buttonTag"), Tag);
        Proof->SetBoolField(TEXT("actualDriverClickCompleted"), true); Proof->SetBoolField(TEXT("actualVisibleProductionTab"), true);
        Proof->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
        Proof->SetStringField(TEXT("controlSessionId"), Identity->GetStringField(TEXT("controlSessionId")));
        Proof->SetObjectField(TEXT("before"), BusinessBefore.ToSharedRef()); Proof->SetObjectField(TEXT("after"), After);
        Sequence.Reset(); Driver.Reset(); IAutomationDriverModule::Get().Disable(); DriverOwned = false;
        if (!Row(*(Case + TEXT("DriverReleasedAfterClick")), !IAutomationDriverModule::Get().IsEnabled())) return Finish();
        if (!Row(*(Case + TEXT("UiProofPublished")), NodeForgePrototype::WriteJson(Proof, Directory / (Case + TEXT("UiDone.json"))))) return Finish();
        GuiPublished = true; return false;
    }
    TSharedRef<FJsonObject> BusinessSnapshot()
    {
        const auto& State = Service->State(); auto Json = MakeShared<FJsonObject>();
        Json->SetStringField(TEXT("draftRevision"), LexToString(State.Input.Revision()));
        Json->SetStringField(TEXT("inputText"), State.Input.Draft().Text);
        Json->SetStringField(TEXT("inputDigest"), NodeForgePrototype::TextSha256(State.Input.Draft().Text));
        Json->SetStringField(TEXT("resultText"), State.ResultText); Json->SetStringField(TEXT("resultNotice"), State.ResultNotice);
        Json->SetStringField(TEXT("nativeReport"), State.LastReportPath); Json->SetStringField(TEXT("assetPath"), State.LastAssetPath);
        Json->SetStringField(TEXT("lastSuccessReport"), State.LastSuccessReportPath); Json->SetStringField(TEXT("lastSuccessAsset"), State.LastSuccessAssetPath);
        Json->SetStringField(TEXT("lastImportInput"), State.LastImportInputPath); Json->SetStringField(TEXT("lastImportDigest"), State.LastImportInputDigest);
        Json->SetStringField(TEXT("lastExportReport"), State.LastExportReportPath);
        Json->SetStringField(TEXT("readOnlyDigest"), NodeForgePrototype::TextSha256(State.LastReadOnlyData));
        Json->SetNumberField(TEXT("readOnlyLength"), State.LastReadOnlyData.Len());
        Json->SetBoolField(TEXT("resultStale"), Service->ResultIsStale()); Json->SetBoolField(TEXT("resultRelayout"), State.bResultRelayout);
        return Json;
    }
    bool ObserveAuxUi()
    {
        static const TCHAR* Names[] = { TEXT("PasteValid"), TEXT("PasteReject"), TEXT("FileCancel"), TEXT("RulesMaterial"),
            TEXT("CopyReadOnly"), TEXT("CopyReport"), TEXT("BusyButtons"), TEXT("Locate"), TEXT("OpenAsset"), TEXT("StaleAssets") };
        static const TCHAR* Tags[] = { TEXT("NodeForge.Paste"), TEXT("NodeForge.Paste"), TEXT("NodeForge.LoadFile"),
            TEXT("NodeForge.RulesMaterial"), TEXT("NodeForge.CopyReadOnly"), TEXT("NodeForge.CopyReport"), TEXT("NodeForge.CopyReadOnly"),
            TEXT("NodeForge.LocateAsset"), TEXT("NodeForge.OpenAsset"), TEXT("NodeForge.LocateAsset") };
        const FString Case = Names[AuxCase], Tag = Tags[AuxCase], Directory = FPaths::GetPath(Output);
        if (FPlatformTime::Seconds() - Started >= 600) { Row(TEXT("AuxUiBoundedDeadline"), false); return Finish(); }
        if (GuiPublished)
        {
            const FString CheckedPath = Directory / (Case + TEXT("UiChecked.json"));
            if (!IFileManager::Get().FileExists(*CheckedPath)) return false;
            FString Text; TSharedPtr<FJsonObject> Checked;
            if (!FFileHelper::LoadFileToString(Text, *CheckedPath)
                || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Checked) || !Checked) return false;
            const FString Hash = Service->State().LastReportPath.IsEmpty() ? FString() : NodeForgePrototype::FileSha256(Service->State().LastReportPath);
            if (!Row(*(Case + TEXT("AuxIndependentCheckMatchesCurrentReceipt")), Checked->GetStringField(TEXT("operationStatus")) == TEXT("passed")
                && Checked->GetStringField(TEXT("controlSessionId")) == Identity->GetStringField(TEXT("controlSessionId"))
                && Checked->GetStringField(TEXT("nativeReportSha256")) == Hash)) return Finish();
            if (!Row(*(Case + TEXT("AuxCheckAccepted")), NodeForgePrototype::WriteJson(Checked.ToSharedRef(),
                Directory / (Case + TEXT("UiCheckedAccepted.json"))))) return Finish();
            ++AuxCase; GuiPublished = false; GuiClickStarted = false; GuiClickCompleted = false;
            GuiObservedAt = 0; GuiCompletedAt = 0; AuxObservedOnly = false;
            BusinessBefore.Reset(); AuxObservation.Reset(); Sequence.Reset(); return false;
        }
        const FString ReadyPath = Directory / (Case + TEXT("UiReady.json"));
        if (!IFileManager::Get().FileExists(*ReadyPath)) return false;
        if (!GuiClickStarted)
        {
            if (GuiObservedAt == 0)
            {
                Tab->DrawAttention();
                auto RuleArea = StaticCastSharedPtr<SExpandableArea>(NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), TEXT("NodeForge.RuleSection")));
                auto CopyArea = StaticCastSharedPtr<SExpandableArea>(NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), TEXT("NodeForge.CopySection")));
                auto Scroll = StaticCastSharedPtr<SScrollBox>(NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), TEXT("NodeForge.AdvancedScroll")));
                if (!Row(TEXT("AuxUsesActualVisibleSections"), RuleArea.IsValid() && CopyArea.IsValid() && Scroll.IsValid())) return Finish();
                RuleArea->SetExpanded(AuxCase >= 3 && AuxCase <= 6); CopyArea->SetExpanded(false); Scroll->SetScrollOffset(0);
                BusinessBefore = BusinessSnapshot(); AuxObservation = MakeShared<FJsonObject>();
                if (AuxCase == 6)
                {
                    AuxBusinessScope = MakeUnique<FNodeForgeEditorBusinessScope>(*Service);
                    if (!Row(TEXT("AuxOwnsRealProductBusinessSlotForUiObservation"), static_cast<bool>(*AuxBusinessScope))) return Finish();
                }
                GuiObservedAt = FPlatformTime::Seconds(); return false;
            }
            if (FPlatformTime::Seconds() - GuiObservedAt < 0.5) return false;
            FString Text; TSharedPtr<FJsonObject> Ready;
            const bool Read = FFileHelper::LoadFileToString(Text, *ReadyPath)
                && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Ready) && Ready;
            const auto Window = FSlateApplication::Get().FindWidgetWindow(Tab.ToSharedRef());
            const auto Button = NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), FName(*Tag));
            if (!Row(*(Case + TEXT("AuxReadyMatchesActualSharedDraft")), Read && Window.IsValid() && Window->IsVisible()
                && Button.IsValid() && Ready->GetStringField(TEXT("controlSessionId")) == Identity->GetStringField(TEXT("controlSessionId"))
                && Ready->GetStringField(TEXT("draftRevision")) == LexToString(Service->State().Input.Revision())
                && Ready->GetStringField(TEXT("inputDigest")) == NodeForgePrototype::TextSha256(Input->GetText().ToString())
                && Input->GetText().ToString() == Service->State().Input.Draft().Text)) return Finish();
            Window->BringToFront(true);
            if (AuxCase == 6 || AuxCase == 9)
            {
                static const TCHAR* BusyTags[] = { TEXT("NodeForge.Input"), TEXT("NodeForge.RestoreInput"), TEXT("NodeForge.Paste"),
                    TEXT("NodeForge.LoadFile"), TEXT("NodeForge.Preflight"), TEXT("NodeForge.Generate"), TEXT("NodeForge.PrepareCopy"),
                    TEXT("NodeForge.RulesMaterial"), TEXT("NodeForge.Catalog"), TEXT("NodeForge.InspectSelection"), TEXT("NodeForge.CopyReadOnly"),
                    TEXT("NodeForge.CopyReport"), TEXT("NodeForge.LocateAsset"), TEXT("NodeForge.OpenAsset") };
                bool Disabled = true;
                for (const TCHAR* DisabledTag : BusyTags)
                {
                    if (AuxCase == 9 && FString(DisabledTag) != TEXT("NodeForge.LocateAsset") && FString(DisabledTag) != TEXT("NodeForge.OpenAsset")) continue;
                    const auto Widget = NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), FName(DisabledTag));
                    const bool Enabled = !Widget.IsValid() || Widget->IsEnabled();
                    AuxObservation->SetBoolField(DisabledTag, Enabled); Disabled &= !Enabled;
                }
                if (!Row(*(Case + TEXT("AuxActualButtonsDisabledAfterVisiblePrepass")), Disabled
                    && (AuxCase == 6 ? Service->IsBusy() : Service->ResultIsStale()))) return Finish();
                AuxBusinessScope.Reset(); AuxObservedOnly = true; GuiClickStarted = true; GuiClickCompleted = true;
                GuiCompletedAt = FPlatformTime::Seconds(); return false;
            }
            if (!Row(*(Case + TEXT("AuxButtonEnabledBeforeActualClick")), Button->IsEnabled() && !Service->IsBusy())) return Finish();
            if (AuxCase < 2 || AuxCase == 4 || AuxCase == 5)
            {
                FString ClipboardSequence;
                if (!Row(*(Case + TEXT("AuxOriginalClipboardSequenceUnchangedBeforeWrite")), Ready->TryGetStringField(TEXT("clipboardSequence"), ClipboardSequence)
                    && FCString::Strtoui64(*ClipboardSequence, nullptr, 10) == ::GetClipboardSequenceNumber())) return Finish();
            }
            if (AuxCase < 2)
            {
                AuxClipboardTouched = true;
                FString ClipboardText, Hash, Error;
                if (AuxCase == 0 && !Row(TEXT("AuxPasteUsesOriginalFixedControlledFile"), NodeForgePrototype::ReadInputFile(
                    NodeForgePrototype::WorkspaceDirectory() / TEXT("Examples/GraphV2Samples/BlueprintConcat.json"), ClipboardText, Hash, Error))) return Finish();
                FPlatformApplicationMisc::ClipboardCopy(*ClipboardText); AuxClipboardOwned = ClipboardText;
            }
            if (AuxCase == 4) AuxExpectedClipboard = Service->State().LastReadOnlyData;
            if (AuxCase == 5)
            {
                FString Description;
                if (!Row(TEXT("AuxCopyReportUsesActualBoundedCurrentReport"), NodeForgeResultView::Read(Service->State().LastReportPath,
                    AuxExpectedClipboard, Description))) return Finish();
            }
            if (AuxCase == 8)
            {
                const FString Asset = Service->State().LastAssetPath;
                AuxOpenedAsset = Asset == TEXT("/Game/NodeForgeTests/NFGraphBlueprintConcat")
                    ? LoadObject<UObject>(nullptr, *(Asset + TEXT(".") + FPaths::GetCleanFilename(Asset))) : nullptr;
                if (!Row(TEXT("AuxOnlyOpensExactOwnedSavedCleanAsset"), AuxOpenedAsset.IsValid()
                    && !AuxOpenedAsset->GetOutermost()->IsDirty())) return Finish();
            }
            if (AuxCase == 2)
            {
                auto Dialog = MakeShared<FJsonObject>();
                const void* Parent = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(Tab->GetContent());
                Dialog->SetStringField(TEXT("parentHwnd"), LexToString(reinterpret_cast<UPTRINT>(Parent)));
                Dialog->SetStringField(TEXT("controlSessionId"), Identity->GetStringField(TEXT("controlSessionId")));
                Dialog->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
                Dialog->SetStringField(TEXT("title"), TEXT("载入 NodeForge JSON（不会生成资产）"));
                if (!Row(TEXT("AuxPublishesExactOwnedDialogParentBeforeRealClick"), Parent
                    && NodeForgePrototype::WriteJson(Dialog, Directory / TEXT("FileCancelDialogReady.json")))) return Finish();
            }
            Sequence = Driver->CreateSequence(); Sequence->Actions().Click(By::Path(Tag));
            Pending = Sequence->Perform(); GuiClickStarted = true; return false;
        }
        if (!GuiClickCompleted)
        {
            if (!Pending.GetFuture().IsReady()) return false;
            if (!Row(*(Case + TEXT("AuxActualDriverClickCompleted")), Pending.GetFuture().Get())) return Finish();
            GuiClickCompleted = true; GuiCompletedAt = FPlatformTime::Seconds(); return false;
        }
        if (FPlatformTime::Seconds() - GuiCompletedAt < 0.5) return false;
        bool Observed = true;
        if (AuxCase == 0) Observed = Service->State().Input.Revision() == FCString::Strtoui64(*BusinessBefore->GetStringField(TEXT("draftRevision")), nullptr, 10) + 1;
        if (AuxCase == 1) Observed = !Service->State().ResultNotice.IsEmpty();
        if (AuxCase == 2) Observed = Service->State().ResultNotice == TEXT("已取消载入；原输入、检查和结果保留。");
        if (AuxCase == 3) Observed = !Service->State().LastReadOnlyData.IsEmpty() && Service->State().LastReportPath != BusinessBefore->GetStringField(TEXT("nativeReport"));
        if (AuxCase == 6)
        {
            FString ClipboardText; FPlatformApplicationMisc::ClipboardPaste(ClipboardText);
            Observed = ClipboardText == AuxClipboardOwned;
            AuxObservation->SetBoolField(TEXT("clipboardChangedDuringBusy"), !Observed);
        }
        if (AuxCase == 4 || AuxCase == 5)
        {
            FString ClipboardText; FPlatformApplicationMisc::ClipboardPaste(ClipboardText);
            const bool Exact = !AuxExpectedClipboard.IsEmpty() && ClipboardText == AuxExpectedClipboard;
            AuxObservation->SetBoolField(TEXT("clipboardMatchesExpected"), Exact);
            if (Exact) { AuxClipboardOwned = ClipboardText; AuxObservation->SetStringField(TEXT("controlledClipboardDigest"), NodeForgePrototype::TextSha256(ClipboardText)); }
            Observed = Exact;
        }
        if (AuxCase == 7)
        {
            TArray<FAssetData> Selected; FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser")).Get().GetSelectedAssets(Selected);
            Observed = Selected.Num() == 1 && Selected[0].GetObjectPathString() == Service->State().LastAssetPath + TEXT(".")
                + FPaths::GetCleanFilename(Service->State().LastAssetPath);
            AuxObservation->SetBoolField(TEXT("actualSelectedAssetMatchesCurrentResult"), Observed);
        }
        if (AuxCase == 8)
        {
            const FString Asset = Service->State().LastAssetPath;
            UObject* Object = Asset == TEXT("/Game/NodeForgeTests/NFGraphBlueprintConcat")
                ? LoadObject<UObject>(nullptr, *(Asset + TEXT(".") + FPaths::GetCleanFilename(Asset))) : nullptr;
            Observed = Object && !Object->GetOutermost()->IsDirty() && GEditor
                && GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(Object, false);
            AuxObservation->SetBoolField(TEXT("actualAssetEditorOpenWithoutDirtyPackage"), Observed);
        }
        const bool Reflected = Observed && !Service->IsBusy() && Input->GetText().ToString() == Service->State().Input.Draft().Text
            && Summary->GetText().ToString() == Service->State().ResultText;
        if (!Reflected && FPlatformTime::Seconds() - GuiCompletedAt < 3) return false;
        if (!Row(*(Case + TEXT("AuxActualObservedResultAndSharedWidgetsAgree")), Reflected)) return Finish();
        Tab->DrawAttention(); const auto Window = FSlateApplication::Get().FindWidgetWindow(Tab.ToSharedRef());
        const bool Visible = Window.IsValid() && Window->IsVisible() && Window->GetNativeWindow().IsValid();
        const FString Screenshot = Directory / (Case + TEXT("AuxProduction.png"));
        TArray<FColor> Pixels; FIntVector Size = FIntVector::ZeroValue; TArray64<uint8> Bytes;
        const bool Captured = Visible && FSlateApplication::Get().TakeScreenshot(Tab->GetContent(), Pixels, Size);
        if (Captured) FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Bytes);
        if (!Row(*(Case + TEXT("AuxActualVisibleScreenshotSaved")), Captured && Bytes.Num() > 0
            && !IFileManager::Get().FileExists(*Screenshot) && FFileHelper::SaveArrayToFile(Bytes, *Screenshot))) return Finish();
        auto Proof = MakeShared<FJsonObject>(); Proof->SetStringField(TEXT("operationStatus"), TEXT("passed"));
        Proof->SetStringField(TEXT("source"), TEXT("actualGui")); Proof->SetStringField(TEXT("buttonTag"), Tag);
        Proof->SetStringField(TEXT("actionKind"), AuxObservedOnly ? TEXT("observeDisabled") : TEXT("actualDriverClick"));
        Proof->SetBoolField(TEXT("actualDriverClickCompleted"), GuiClickCompleted && !AuxObservedOnly);
        Proof->SetBoolField(TEXT("actualVisibleProductionTab"), Visible); Proof->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
        Proof->SetStringField(TEXT("controlSessionId"), Identity->GetStringField(TEXT("controlSessionId")));
        Proof->SetStringField(TEXT("screenshot"), Screenshot); Proof->SetObjectField(TEXT("before"), BusinessBefore.ToSharedRef());
        Proof->SetObjectField(TEXT("after"), BusinessSnapshot()); Proof->SetObjectField(TEXT("observations"), AuxObservation.ToSharedRef());
        if (!Row(*(Case + TEXT("AuxActualProofPublished")), NodeForgePrototype::WriteJson(Proof, Directory / (Case + TEXT("UiDone.json"))))) return Finish();
        GuiPublished = true; return false;
    }
    bool ObserveBusinessUi()
    {
        static const TCHAR* Names[] = { TEXT("EmptyExport"), TEXT("Catalog"), TEXT("InspectNegative"), TEXT("Prepare"), TEXT("CopyGenerate"),
            TEXT("Relayout"), TEXT("Export"), TEXT("Preview"), TEXT("InvalidPreview"), TEXT("InvalidPrepare") };
        static const TCHAR* Tags[] = { TEXT("NodeForge.ExportGraph"), TEXT("NodeForge.Catalog"), TEXT("NodeForge.InspectSelection"),
            TEXT("NodeForge.PrepareCopy"), TEXT("NodeForge.Generate"), TEXT("NodeForge.Relayout"), TEXT("NodeForge.ExportGraph"),
            TEXT("NodeForge.PreviewGraph"), TEXT("NodeForge.PreviewGraph"), TEXT("NodeForge.PrepareCopy") };
        const FString Case = Names[BusinessCase], Tag = Tags[BusinessCase], Directory = FPaths::GetPath(Output);
        if (FPlatformTime::Seconds() - Started >= 600) { Row(TEXT("BusinessUiBoundedDeadline"), false); return Finish(); }
        if (GuiPublished)
        {
            const FString CheckedPath = Directory / (Case + TEXT("UiChecked.json"));
            if (!IFileManager::Get().FileExists(*CheckedPath)) return false;
            FString Text; TSharedPtr<FJsonObject> Checked;
            if (!FFileHelper::LoadFileToString(Text, *CheckedPath)
                || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Checked) || !Checked) return false;
            const FString Hash = Service->State().LastReportPath.IsEmpty() ? FString() : NodeForgePrototype::FileSha256(Service->State().LastReportPath);
            if (!Row(*(Case + TEXT("IndependentBusinessCheckMatchesCurrentReceipt")), Checked->GetStringField(TEXT("operationStatus")) == TEXT("passed")
                && Checked->GetStringField(TEXT("controlSessionId")) == Identity->GetStringField(TEXT("controlSessionId"))
                && Checked->GetStringField(TEXT("nativeReportSha256")) == Hash)) return Finish();
            if (!Row(*(Case + TEXT("BusinessCheckAccepted")), NodeForgePrototype::WriteJson(Checked.ToSharedRef(),
                Directory / (Case + TEXT("UiCheckedAccepted.json"))))) return Finish();
            ++BusinessCase; GuiPublished = false; GuiClickStarted = false; GuiClickCompleted = false;
            GuiObservedAt = 0; GuiCompletedAt = 0; BusinessBefore.Reset(); Sequence.Reset(); return false;
        }
        const FString ReadyPath = Directory / (Case + TEXT("UiReady.json"));
        if (!IFileManager::Get().FileExists(*ReadyPath)) return false;
        if (!GuiClickStarted)
        {
            if (GuiObservedAt == 0)
            {
                const bool Rules = BusinessCase == 1 || BusinessCase == 2;
                const bool Copies = BusinessCase == 0 || (BusinessCase >= 5 && BusinessCase <= 8);
                auto RuleArea = StaticCastSharedPtr<SExpandableArea>(NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), TEXT("NodeForge.RuleSection")));
                auto CopyArea = StaticCastSharedPtr<SExpandableArea>(NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), TEXT("NodeForge.CopySection")));
                auto Scroll = StaticCastSharedPtr<SScrollBox>(NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), TEXT("NodeForge.AdvancedScroll")));
                if (!Row(TEXT("BusinessUsesActualExpandableSections"), RuleArea.IsValid() && CopyArea.IsValid() && Scroll.IsValid())) return Finish();
                RuleArea->SetExpanded(Rules); CopyArea->SetExpanded(Copies); Scroll->SetScrollOffset(0);
                const auto ActionWidget = NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), FName(*Tag));
                if (Rules || Copies) Scroll->ScrollDescendantIntoView(ActionWidget, false);
                auto& Browser = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser")).Get();
                if (BusinessCase == 0 || BusinessCase == 2)
                {
                    TArray<FAssetData> Selected; Browser.GetSelectedAssets(Selected);
                    if (!Row(*(Case + TEXT("ActualContentBrowserSelectionEmpty")), Selected.IsEmpty())) return Finish();
                }
                if (BusinessCase == 6)
                {
                    const FString Asset = Service->State().LastSuccessAssetPath;
                    UObject* Object = Asset.StartsWith(TEXT("/Game/NodeForgeTests/GraphLayout"))
                        ? LoadObject<UObject>(nullptr, *(Asset + TEXT(".") + FPaths::GetCleanFilename(Asset))) : nullptr;
                    if (!Row(TEXT("ExportSelectionIsOwnedSavedRelayoutAsset"), Object && !Object->GetOutermost()->IsDirty())) return Finish();
                    TArray<UObject*> Assets; Assets.Add(Object); Browser.SyncBrowserToAssets(Assets, false, false);
                }
                GuiObservedAt = FPlatformTime::Seconds(); return false;
            }
            if (FPlatformTime::Seconds() - GuiObservedAt < 0.5) return false;
            if (BusinessCase == 0)
            {
                const auto Preview = NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), TEXT("NodeForge.PreviewGraph"));
                const auto Relayout = NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), TEXT("NodeForge.Relayout"));
                auto Observation = MakeShared<FJsonObject>();
                Observation->SetBoolField(TEXT("previewFound"), Preview.IsValid());
                Observation->SetBoolField(TEXT("relayoutFound"), Relayout.IsValid());
                Observation->SetBoolField(TEXT("previewEnabled"), Preview.IsValid() && Preview->IsEnabled());
                Observation->SetBoolField(TEXT("relayoutEnabled"), Relayout.IsValid() && Relayout->IsEnabled());
                Observation->SetStringField(TEXT("lastExportReport"), Service->State().LastExportReportPath);
                Observation->SetStringField(TEXT("lastImportInput"), Service->State().LastImportInputPath);
                Observation->SetNumberField(TEXT("secondsAfterExpand"), FPlatformTime::Seconds() - GuiObservedAt);
                if (!Row(TEXT("InitialButtonsAfterPrepassObservationWritten"), NodeForgePrototype::WriteJson(Observation,
                    Directory / TEXT("InitialButtonsAfterPrepass.json")))) return Finish();
                if (!Row(TEXT("PreviewAndRelayoutInitiallyDisabled"), Preview.IsValid() && Relayout.IsValid()
                    && !Preview->IsEnabled() && !Relayout->IsEnabled())) return Finish();
            }
            FString Text; TSharedPtr<FJsonObject> Ready;
            const bool Read = FFileHelper::LoadFileToString(Text, *ReadyPath)
                && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Ready) && Ready;
            const auto Window = FSlateApplication::Get().FindWidgetWindow(Tab.ToSharedRef());
            const auto Button = NodeForgePanel::FindTaggedForChecks(Tab->GetContent(), FName(*Tag));
            if (!Row(*(Case + TEXT("BusinessReadyMatchesRealSharedDraft")), Read && Window.IsValid() && Window->IsVisible()
                && Button.IsValid() && Button->IsEnabled() && !Service->IsBusy()
                && Ready->GetStringField(TEXT("controlSessionId")) == Identity->GetStringField(TEXT("controlSessionId"))
                && Ready->GetStringField(TEXT("draftRevision")) == LexToString(Service->State().Input.Revision())
                && Ready->GetStringField(TEXT("inputDigest")) == NodeForgePrototype::TextSha256(Input->GetText().ToString())
                && Input->GetText().ToString() == Service->State().Input.Draft().Text)) return Finish();
            if (BusinessCase == 6)
            {
                TArray<FAssetData> Selected; FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser")).Get().GetSelectedAssets(Selected);
                if (!Row(TEXT("ExportActualSingleSelectionMatchesSavedSource"), Selected.Num() == 1
                    && Selected[0].GetObjectPathString() == Service->State().LastSuccessAssetPath + TEXT(".")
                        + FPaths::GetCleanFilename(Service->State().LastSuccessAssetPath))) return Finish();
            }
            Window->BringToFront(true); BusinessBefore = BusinessSnapshot();
            Sequence = Driver->CreateSequence(); Sequence->Actions().Click(By::Path(Tag));
            Pending = Sequence->Perform(); GuiClickStarted = true; return false;
        }
        if (!GuiClickCompleted)
        {
            if (!Pending.GetFuture().IsReady()) return false;
            if (!Row(*(Case + TEXT("ActualBusinessDriverClickCompleted")), Pending.GetFuture().Get())) return Finish();
            GuiClickCompleted = true; GuiCompletedAt = FPlatformTime::Seconds(); return false;
        }
        const auto After = BusinessSnapshot();
        const bool Changed = After->GetStringField(TEXT("nativeReport")) != BusinessBefore->GetStringField(TEXT("nativeReport"))
            || After->GetStringField(TEXT("resultText")) != BusinessBefore->GetStringField(TEXT("resultText"))
            || After->GetStringField(TEXT("draftRevision")) != BusinessBefore->GetStringField(TEXT("draftRevision"));
        const bool Reflected = !Service->IsBusy() && Changed && Input->GetText().ToString() == Service->State().Input.Draft().Text
            && Summary->GetText().ToString() == Service->State().ResultText && !Service->State().ResultText.IsEmpty();
        if (!Reflected && FPlatformTime::Seconds() - GuiCompletedAt < 3) return false;
        if (!Row(*(Case + TEXT("ActualBusinessResultReflectedAfterClick")), Reflected)) return Finish();
        const auto Window = FSlateApplication::Get().FindWidgetWindow(Tab.ToSharedRef());
        const bool Visible = Window.IsValid() && Window->IsVisible() && Window->GetNativeWindow().IsValid();
        if (!Row(*(Case + TEXT("BusinessVisibleProductionTab")), Visible)) return Finish();
        const FString Screenshot = Directory / (Case + TEXT("BusinessProduction.png"));
        TArray<FColor> Pixels; FIntVector Size = FIntVector::ZeroValue; TArray64<uint8> Bytes;
        bool Captured = FSlateApplication::Get().TakeScreenshot(Tab->GetContent(), Pixels, Size);
        if (Captured) FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Bytes);
        if (!Row(*(Case + TEXT("BusinessActualScreenshotSaved")), Captured && Bytes.Num() > 0
            && !IFileManager::Get().FileExists(*Screenshot) && FFileHelper::SaveArrayToFile(Bytes, *Screenshot))) return Finish();
        auto Proof = MakeShared<FJsonObject>(); Proof->SetStringField(TEXT("operationStatus"), TEXT("passed"));
        Proof->SetStringField(TEXT("source"), TEXT("actualGui")); Proof->SetStringField(TEXT("buttonTag"), Tag);
        Proof->SetBoolField(TEXT("actualDriverClickCompleted"), GuiClickCompleted); Proof->SetBoolField(TEXT("actualVisibleProductionTab"), Visible);
        Proof->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
        Proof->SetStringField(TEXT("controlSessionId"), Identity->GetStringField(TEXT("controlSessionId")));
        Proof->SetStringField(TEXT("screenshot"), Screenshot); Proof->SetObjectField(TEXT("before"), BusinessBefore.ToSharedRef());
        Proof->SetObjectField(TEXT("after"), After);
        if (!Row(*(Case + TEXT("BusinessActualProofPublished")), NodeForgePrototype::WriteJson(Proof, Directory / (Case + TEXT("UiDone.json"))))) return Finish();
        GuiPublished = true; return false;
    }
    bool ObserveGuiGenerate()
    {
        const FString Case = GraphCase == 0 ? TEXT("Blueprint") : TEXT("Material");
        const FString Directory = FPaths::GetPath(Output);
        if (FPlatformTime::Seconds() - Started >= 600)
        { Row(TEXT("GuiGenerationBoundedDeadline"), false); return Finish(); }
        if (GuiPublished)
        {
            FString CheckedText; TSharedPtr<FJsonObject> Checked;
            const FString CheckedPath = Directory / (Case + TEXT("UiChecked.json"));
            if (!IFileManager::Get().FileExists(*CheckedPath)) return true;
            if (!FFileHelper::LoadFileToString(CheckedText, *CheckedPath)
                || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(CheckedText), Checked) || !Checked) return false;
            if (!Row(*(Case + TEXT("IndependentGuiReceiptCheckedBeforeNextGraph")),
                Checked->GetStringField(TEXT("operationStatus")) == TEXT("passed")
                && Checked->GetStringField(TEXT("controlSessionId")) == Identity->GetStringField(TEXT("controlSessionId"))
                && Checked->GetStringField(TEXT("nativeReportSha256")) == NodeForgePrototype::FileSha256(Service->State().LastReportPath))) return Finish();
            if (!Row(*(Case + TEXT("IndependentGuiCheckAcceptedPublished")), NodeForgePrototype::WriteJson(Checked.ToSharedRef(),
                Directory / (Case + TEXT("UiCheckedAccepted.json"))))) return Finish();
            ++GraphCase; GuiPublished = false; GuiClickStarted = false; GuiClickCompleted = false;
            GuiObservedAt = 0; GuiCompletedAt = 0; Sequence.Reset(); return true;
        }
        const FString ReadyPath = Directory / (Case + TEXT("UiGenerateReady.json"));
        if (!IFileManager::Get().FileExists(*ReadyPath)) return true;
        if (!GuiClickStarted)
        {
            if (GuiObservedAt == 0) { GuiObservedAt = FPlatformTime::Seconds(); return false; }
            if (FPlatformTime::Seconds() - GuiObservedAt < 0.4) return false;
            FString Text; TSharedPtr<FJsonObject> Ready;
            const bool Read = FFileHelper::LoadFileToString(Text, *ReadyPath)
                && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Ready) && Ready;
            const auto Window = FSlateApplication::Get().FindWidgetWindow(Tab.ToSharedRef());
            if (!Row(*(Case + TEXT("RealGuiReadyMatchesSharedDraft")), Read && Window.IsValid() && Window->IsVisible()
                && Ready->GetStringField(TEXT("controlSessionId")) == Identity->GetStringField(TEXT("controlSessionId"))
                && Ready->GetStringField(TEXT("draftRevision")) == LexToString(Service->State().Input.Revision())
                && Ready->GetStringField(TEXT("inputDigest")) == NodeForgePrototype::TextSha256(Input->GetText().ToString())
                && Input->GetText().ToString() == Service->State().Input.Draft().Text && !Service->IsBusy())) return Finish();
            Window->BringToFront(true); GuiPreviousReport = Service->State().LastReportPath;
            Sequence = Driver->CreateSequence(); Sequence->Actions().Click(By::Path(TEXT("NodeForge.Generate")));
            Pending = Sequence->Perform(); GuiClickStarted = true; return false;
        }
        if (!GuiClickCompleted)
        {
            if (!Pending.GetFuture().IsReady()) return false;
            if (!Row(*(Case + TEXT("ActualGenerateDriverClickCompleted")), Pending.GetFuture().Get())) return Finish();
            GuiClickCompleted = true; GuiCompletedAt = FPlatformTime::Seconds(); return false;
        }
        const auto& State = Service->State(); FString NativeText; TSharedPtr<FJsonObject> Native;
        const bool Closed = State.LastReportPath != GuiPreviousReport && !Service->IsBusy()
            && FFileHelper::LoadFileToString(NativeText, *State.LastReportPath)
            && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(NativeText), Native) && Native;
        FString Status, Mode;
        const bool Bound = Closed && Native->TryGetStringField(TEXT("overallStatus"), Status) && Status == TEXT("waitingPersistence")
            && Native->TryGetStringField(TEXT("mode"), Mode) && Mode == TEXT("Import")
            && State.LastSuccessReportPath == State.LastReportPath && !State.LastAssetPath.IsEmpty()
            && State.ResultRevision == State.Input.Revision() && !Service->ResultIsStale();
        if (!Bound && FPlatformTime::Seconds() - GuiCompletedAt < 3) return false;
        bool Passed = Row(*(Case + TEXT("ActualGuiNativeReceiptBound")), Bound);
        const auto Window = FSlateApplication::Get().FindWidgetWindow(Tab.ToSharedRef());
        const bool Visible = Window.IsValid() && Window->IsVisible() && Window->GetNativeWindow().IsValid();
        Passed &= Row(*(Case + TEXT("ActualGuiVisibleResultAndDraft")), Visible
            && Input->GetText().ToString() == State.Input.Draft().Text
            && Summary->GetText().ToString() == (State.ResultNotice.IsEmpty() ? State.ResultText : State.ResultNotice)
            && !State.ResultText.IsEmpty());
        const FString Screenshot = Directory / (Case + TEXT("GuiProduction.png"));
        if (Passed)
        {
            TArray<FColor> Pixels; FIntVector Size = FIntVector::ZeroValue; TArray64<uint8> Bytes;
            Passed = FSlateApplication::Get().TakeScreenshot(Tab->GetContent(), Pixels, Size);
            if (Passed) FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Bytes);
            Passed = Row(*(Case + TEXT("ActualGuiScreenshotSaved")), Passed && Bytes.Num() > 0
                && !IFileManager::Get().FileExists(*Screenshot) && FFileHelper::SaveArrayToFile(Bytes, *Screenshot));
        }
        auto Proof = MakeShared<FJsonObject>();
        Proof->SetStringField(TEXT("operationStatus"), Passed ? TEXT("passed") : TEXT("failed"));
        Proof->SetStringField(TEXT("source"), TEXT("actualGui"));
        Proof->SetBoolField(TEXT("actualDriverClickCompleted"), GuiClickCompleted);
        Proof->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
        Proof->SetStringField(TEXT("controlSessionId"), Identity->GetStringField(TEXT("controlSessionId")));
        Proof->SetStringField(TEXT("draftRevision"), LexToString(State.Input.Revision()));
        Proof->SetStringField(TEXT("inputDigest"), NodeForgePrototype::TextSha256(Input->GetText().ToString()));
        Proof->SetStringField(TEXT("inputSnapshot"), State.LastImportInputPath);
        Proof->SetStringField(TEXT("nativeReport"), State.LastReportPath); Proof->SetStringField(TEXT("assetPath"), State.LastAssetPath);
        Proof->SetStringField(TEXT("resultSummary"), Summary->GetText().ToString());
        Proof->SetStringField(TEXT("screenshot"), Screenshot); Proof->SetBoolField(TEXT("actualVisibleProductionTab"), Visible);
        Row(*(Case + TEXT("ActualGuiProofWritten")), NodeForgePrototype::WriteJson(Proof, Directory / (Case + TEXT("UiGenerated.json"))));
        GuiPublished = true; return true;
    }
    bool ObserveGraphUi()
    {
        const FString Case = GraphCase == 0 ? TEXT("Blueprint") : TEXT("Material");
        const FString Directory = FPaths::GetPath(Output);
        const FString MarkerPath = Directory / (Case + TEXT("Complete.json"));
        if (!IFileManager::Get().FileExists(*MarkerPath)) return true;
        if (GraphMarkedAt == 0) { GraphMarkedAt = FPlatformTime::Seconds(); return false; }
        FString Text; TSharedPtr<FJsonObject> Marker;
        if (!FFileHelper::LoadFileToString(Text, *MarkerPath) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Marker) || !Marker)
        {
            if (FPlatformTime::Seconds() - GraphMarkedAt < 10) return false;
            Row(TEXT("GraphUiClosedMarkerReadable"), false); ++GraphCase; GraphMarkedAt = 0; return true;
        }
        if (FPlatformTime::Seconds() - GraphMarkedAt < 0.25) return false;
        const auto Window = FSlateApplication::Get().FindWidgetWindow(Tab.ToSharedRef());
        const auto& State = Service->State();
        const FString Source = NodeForgePrototype::WorkspaceDirectory() / TEXT("Examples/GraphV2Samples")
            / (GraphCase == 0 ? TEXT("BlueprintConcat.json") : TEXT("StaticColor.json"));
        FString Original, Hash, Error; TSharedPtr<FJsonObject> Request;
        const bool SourceValid = NodeForgePrototype::ReadInputFile(Source, Original, Hash, Error)
            && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Original), Request) && Request;
        const FString ExpectedReport = Directory / TEXT("Calls") / (Case + TEXT("-GenerateNative.json"));
        FString Session, CallId, Asset, MarkerReport, MarkerDigest;
        const bool MarkerValid = Marker->TryGetStringField(TEXT("controlSessionId"), Session) && Session == Identity->GetStringField(TEXT("controlSessionId"))
            && Marker->TryGetStringField(TEXT("callId"), CallId) && CallId == Case + TEXT("-Generate")
            && Marker->TryGetStringField(TEXT("assetPath"), Asset) && SourceValid && Asset == Request->GetStringField(TEXT("assetPath"))
            && Marker->TryGetStringField(TEXT("nativeReport"), MarkerReport) && FPaths::ConvertRelativePathToFull(MarkerReport) == FPaths::ConvertRelativePathToFull(ExpectedReport)
            && Marker->TryGetStringField(TEXT("inputDigest"), MarkerDigest) && MarkerDigest == Hash;
        const bool Visible = Window.IsValid() && Window->IsVisible() && Window->GetNativeWindow().IsValid();
        const bool Exact = MarkerValid && Visible && Service->GetState()->GetBoolField(TEXT("panelAttached"))
            && Input->GetText().ToString() == Original && State.Input.Draft().Text == Original
            && Summary->GetText().ToString() == State.ResultText && !State.ResultText.IsEmpty()
            && State.Input.Revision() == uint64(GraphCase + 1) && State.ResultRevision == State.Input.Revision()
            && !Service->ResultIsStale() && FPaths::ConvertRelativePathToFull(State.LastReportPath) == FPaths::ConvertRelativePathToFull(ExpectedReport)
            && State.LastSuccessReportPath == State.LastReportPath && State.LastAssetPath == Asset;
        bool Passed = Row(*(Case + TEXT("ActualProductionUiMatchesFormalGenerate")), Exact);
        const FString Screenshot = Directory / (Case + TEXT("Production.png"));
        if (Passed)
        {
            TArray<FColor> Pixels; FIntVector Size = FIntVector::ZeroValue; TArray64<uint8> Bytes;
            Passed = FSlateApplication::Get().TakeScreenshot(Tab->GetContent(), Pixels, Size);
            if (Passed) FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Bytes);
            Passed = Row(*(Case + TEXT("ActualProductionScreenshotSaved")), Passed && Bytes.Num() > 0
                && !IFileManager::Get().FileExists(*Screenshot) && FFileHelper::SaveArrayToFile(Bytes, *Screenshot));
        }
        auto Proof = MakeShared<FJsonObject>();
        Proof->SetStringField(TEXT("operationStatus"), Passed ? TEXT("passed") : TEXT("failed"));
        Proof->SetStringField(TEXT("controlSessionId"), Identity->GetStringField(TEXT("controlSessionId")));
        Proof->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
        Proof->SetStringField(TEXT("callId"), CallId); Proof->SetStringField(TEXT("draftRevision"), LexToString(State.Input.Revision()));
        Proof->SetStringField(TEXT("inputDigest"), NodeForgePrototype::TextSha256(Input->GetText().ToString()));
        Proof->SetStringField(TEXT("resultSummary"), Summary->GetText().ToString());
        Proof->SetStringField(TEXT("nativeReport"), State.LastReportPath); Proof->SetStringField(TEXT("assetPath"), State.LastAssetPath);
        Proof->SetStringField(TEXT("screenshot"), Screenshot); Proof->SetBoolField(TEXT("actualVisibleProductionTab"), Visible);
        Row(*(Case + TEXT("UiProofWritten")), NodeForgePrototype::WriteJson(Proof, Directory / (Case + TEXT("Ui.json"))));
        ++GraphCase; GraphMarkedAt = 0; return true;
    }
    bool Row(const TCHAR* Name, bool Passed)
    {
        Test->TestTrue(Name, Passed); bAll &= Passed; auto Json = MakeShared<FJsonObject>();
        Json->SetStringField(TEXT("case"), Name); Json->SetBoolField(TEXT("passed"), Passed); Rows.Add(MakeShared<FJsonValueObject>(Json)); return Passed;
    }
    bool Finish()
    {
        bFinishing = true;
        AuxBusinessScope.Reset();
        if (DriverOwned)
        {
            if (Pending.GetFuture().IsValid() && !Pending.GetFuture().IsReady())
                if (const auto Task = Pending.GetTask()) Task->Cancel();
            Sequence.Reset(); Driver.Reset(); IAutomationDriverModule::Get().Disable(); DriverOwned = false;
            Row(TEXT("GuiDriverActuallyDisabled"), !IAutomationDriverModule::Get().IsEnabled());
        }
        if (AuxClipboardTouched)
        {
            const FString CleanupPath = FPaths::GetPath(Output) / TEXT("ClipboardMemoryRestore.json");
            if (!IFileManager::Get().FileExists(*CleanupPath))
            {
                if (AuxCleanupWaitAt == 0) AuxCleanupWaitAt = FPlatformTime::Seconds();
                const FString FailurePath = FPaths::GetPath(Output) / TEXT("AuxUiFailure.json");
                if (!IFileManager::Get().FileExists(*FailurePath))
                {
                    auto Failure = MakeShared<FJsonObject>(); Failure->SetStringField(TEXT("operationStatus"), TEXT("failed"));
                    Failure->SetStringField(TEXT("reason"), TEXT("Native action failed; awaiting external opaque clipboard restoration before exit"));
                    NodeForgePrototype::WriteJson(Failure, FailurePath);
                }
                if (FPlatformTime::Seconds() - AuxCleanupWaitAt < 35) return false;
                AuxClipboardCleanupFailed = true;
                Row(TEXT("AuxClipboardCleanupMissingPreserveOwnedEditor"), false);
            }
            else
            {
                FString Text; TSharedPtr<FJsonObject> Cleanup;
                const bool Read = FFileHelper::LoadFileToString(Text, *CleanupPath)
                    && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Cleanup) && Cleanup;
                const bool Valid = Read && Cleanup->GetStringField(TEXT("operationStatus")) == TEXT("passed")
                    && Cleanup->GetNumberField(TEXT("pid")) == FPlatformProcess::GetCurrentProcessId()
                    && Cleanup->GetStringField(TEXT("controlSessionId")) == Identity->GetStringField(TEXT("controlSessionId"));
                AuxClipboardCleanupFailed = !Row(TEXT("AuxOpaqueClipboardCleanupVerifiedBeforeOwnedExit"), Valid);
                if (Valid)
                {
                    Report->SetBoolField(TEXT("clipboardRestored"), Cleanup->GetStringField(TEXT("mode")) == TEXT("originalFormatsRestored"));
                    Report->SetBoolField(TEXT("clipboardExternalChangePreserved"), Cleanup->GetBoolField(TEXT("externalChangePreserved")));
                }
            }
            AuxClipboardOwned.Empty(); AuxClipboardTouched = false;
        }
        if (Channel) Channel->SetBeforeDrainProbeForChecks(TFunction<void()>());
        if (Service) Service->SetControlProbeForChecks(TFunction<void()>());
        if (Channel)
        {
            Row(TEXT("BoundedChannelStopBeforeOwnedEditorExit"), Channel->Stop() && Channel->IsStopped());
            Report->SetStringField(TEXT("exitRequest"), TEXT("normalOwnedEditorAfterChannelStop"));
        }
        Report->SetStringField(TEXT("operationStatus"), bAll ? TEXT("passed") : TEXT("failed"));
        Report->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
        Report->SetStringField(TEXT("moduleSha256"), NodeForgePrototype::FileSha256(FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"))));
        Report->SetStringField(TEXT("scope"), QueryUi ? TEXT("Two actual query buttons, new query adapter/path boundaries and formal client batch; no old UI matrix")
            : AuxUi ? TEXT("Fixed actual auxiliary production UI; owned dialog cancellation, controlled clipboard, asset result actions; no business RPC additions")
            : BusinessUi ? TEXT("Fixed actual production UI business clicks; original backend reports; no business RPC additions")
            : GuiGenerate ? TEXT("Formal CLI load/preflight; actual production Driver Generate clicks and original UI Import; no RPC generate receipt")
            : GraphUi ? TEXT("Formal external CLI generates two assets; native test only reads actual production tab and captures results")
            : AssetBoundary ? TEXT("Production graph boundary/real accepted Import disconnect; native fixture only observes client completion, no graph execution")
            : TEXT("Production channel/restricted token/identity; external evidence independently checked by runner; no assets"));
        Report->SetArrayField(TEXT("checks"), Rows);
        bool OwnedAssetDirty = AuxUi && AuxOpenedAsset.IsValid() && AuxOpenedAsset->GetOutermost()->IsDirty();
        if (QueryUi)
            for (TObjectIterator<UPackage> It; It; ++It)
                if (It->GetName().StartsWith(TEXT("/Game/NodeForgeTests/")) && It->IsDirty()) OwnedAssetDirty = true;
        if (OwnedAssetDirty || AuxClipboardCleanupFailed) Report->SetStringField(TEXT("exitRequest"), TEXT("notRequestedDirtyOrClipboardCleanupFailure; preserve live editor"));
        if (!NodeForgePrototype::WriteJson(Report, Output)) Test->AddError(TEXT("Fresh identity report write failed"));
        if (!OwnedAssetDirty && !AuxClipboardCleanupFailed) FPlatformMisc::RequestExitWithStatus(false, bAll ? 0 : 1);
        return true;
    }
    FAutomationTestBase* Test;
    FString Output; double Started; bool bReady = false, bAll = true, bFinishing = false;
    TSharedPtr<FNodeForgeEditorControlChannel> Channel; TSharedPtr<FJsonObject> Identity;
    TSharedPtr<FNodeForgeEditorControlService> Service;
    int32 ProbeCount = 0; bool QueuedProbe = false, RunningProbe = false;
    bool GraphUi = false, AssetBoundary = false, GuiGenerate = false, BusinessUi = false, AuxUi = false;
    bool QueryUi = false, QueryBoundaryDone = false; int32 QueryCase = 0;
    int32 AuxCase = 0; bool AuxObservedOnly = false, AuxClipboardTouched = false;
    FString AuxClipboardOwned, AuxExpectedClipboard;
    double AuxCleanupWaitAt = 0; bool AuxClipboardCleanupFailed = false;
    TSharedPtr<FJsonObject> AuxObservation;
    TUniquePtr<FNodeForgeEditorBusinessScope> AuxBusinessScope;
    TWeakObjectPtr<UObject> AuxOpenedAsset;
    int32 GraphCase = 0, GraphCaseCount = 2, BusinessCase = 0; double GraphMarkedAt = 0; TSharedPtr<FJsonObject> BusinessBefore;
    bool DriverOwned = false, GuiClickStarted = false, GuiClickCompleted = false, GuiPublished = false;
    double GuiObservedAt = 0, GuiCompletedAt = 0; FString GuiPreviousReport;
    TSharedPtr<IAsyncAutomationDriver, ESPMode::ThreadSafe> Driver;
    TSharedPtr<IAsyncDriverSequence, ESPMode::ThreadSafe> Sequence;
    TAsyncResult<bool> Pending;
    TSharedPtr<SDockTab> Tab; TSharedPtr<SMultiLineEditableTextBox> Input;
    TSharedPtr<STextBlock> Summary;
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>(); TArray<TSharedPtr<FJsonValue>> Rows;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeEditorControlIdentityTest, "NodeForge.EditorControlIdentity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNodeForgeEditorControlIdentityTest::RunTest(const FString& Parameters)
{
    FString Output;
    if (IsRunningCommandlet() || !FParse::Value(FCommandLine::Get(), TEXT("NFEditorControlReport="), Output)
        || !(FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Output), NodeForgePrototype::WorkspaceDirectory() / TEXT("Reports/PluginControlStageTwo"))
            || FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Output), FPaths::ProjectSavedDir() / TEXT("NodeForge/Reports/PluginControlStageTwo")))
        || IFileManager::Get().FileExists(*Output)) { AddError(TEXT("Fresh owned editor control report required")); return false; }
    ADD_LATENT_AUTOMATION_COMMAND(FNodeForgeChannelIdentityCommand(this, Output)); return true;
}
#endif
