// Project-owned input ownership and bounded reader regression. No asset generation.
#include "NodeForgePanelState.h"
#include "NodeForgePrototype.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/Archive.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgePanelInputGuardsTest,"NodeForge.PanelInputGuards",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNodeForgePanelInputGuardsTest::RunTest(const FString& Parameters)
{
    FString Output;const FString Root=NodeForgePrototype::WorkspaceDirectory();
    if(!FParse::Value(FCommandLine::Get(),TEXT("NFPanelInputReport="),Output)
        || IFileManager::Get().FileExists(*Output)
        || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Output),Root/TEXT("Reports")))
    {AddError(TEXT("Use a fresh NFPanelInputReport inside project Reports."));return false;}
    const FString Work=FPaths::ProjectSavedDir()/TEXT("NodeForge/PanelInputGuards")/FGuid::NewGuid().ToString(EGuidFormats::Digits);
    IFileManager::Get().MakeDirectory(*Work,true);
    TArray<TSharedPtr<FJsonValue>> Rows;
    auto Row=[&](const TCHAR* Name,bool Passed)
    {TestTrue(Name,Passed);auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("case"),Name);R->SetBoolField(TEXT("passed"),Passed);Rows.Add(MakeShared<FJsonValueObject>(R));};
    FNodeForgePanelState State;FString Error;
    FNodeForgePanelState Editing;
    Row(TEXT("FirstManualEditCapturesEmptyRestorePoint"),Editing.Edit(TEXT("first"))&&Editing.CanRestore());
    Row(TEXT("ContinuousManualEditsKeepPreEditSnapshot"),Editing.Edit(TEXT("second"))&&Editing.Restore()&&Editing.Draft().Text.IsEmpty());
    Row(TEXT("ExplicitLoadStartsNewEditingSegment"),Editing.Replace(TEXT("loaded"),TEXT("file"),TEXT("Input.json"),TEXT("hash"),Error)
        &&Editing.Edit(TEXT("loaded edit"))&&Editing.Edit(TEXT("loaded edit 2"))&&Editing.Restore()&&Editing.Draft().Text==TEXT("loaded")
        &&Editing.Draft().SourcePath==TEXT("Input.json")&&Editing.Draft().SourceDigest==TEXT("hash"));
    const FString A=TEXT("{\"value\":\"AbC\"}"),B=TEXT("{\"value\":\"中文\"}");
    Row(TEXT("NativeEditUpdatesRevision"),State.Edit(A)&&State.Revision()==1);
    const uint64 RevisionA=State.Revision();
    Row(TEXT("IdenticalNativeEditHasNoFalseRevision"),!State.Edit(A)&&State.Revision()==RevisionA);
    Row(TEXT("CaseOnlyEditCountsAsChanged"),State.Edit(TEXT("{\"value\":\"abc\"}"))&&State.Revision()>RevisionA);
    Row(TEXT("ClipboardReplacesEntireDraft"),State.Replace(B,TEXT("clipboard"),FString(),FString(),Error)&&State.Draft().Text.Equals(B,ESearchCase::CaseSensitive));
    const uint64 RevisionB=State.Revision();
    Row(TEXT("RepeatClipboardHasOneCopyAndKeepsRevision"),State.Replace(B,TEXT("clipboard"),FString(),FString(),Error)&&State.Revision()==RevisionB&&State.Draft().Text==B);
    Row(TEXT("EmptyClipboardPreservesDraft"),!State.Replace(TEXT(""),TEXT("clipboard"),FString(),FString(),Error)&&State.Revision()==RevisionB&&State.Draft().Text==B);
    Row(TEXT("WhitespaceClipboardPreservesDraft"),!State.Replace(TEXT(" \r\n\t"),TEXT("clipboard"),FString(),FString(),Error)&&State.Revision()==RevisionB&&State.Draft().Text==B);
    Row(TEXT("RestoreUsesPreviousDraftAndInvalidatesOldRevision"),State.Restore()&&State.Draft().Text.Equals(TEXT("{\"value\":\"abc\"}"),ESearchCase::CaseSensitive)&&State.Revision()>RevisionB&&!State.CanRestore());
    Row(TEXT("RestoreIsSingleStep"),!State.Restore());
    Row(TEXT("FileSourceIsRetainedWhenEdited"),State.Replace(A,TEXT("file"),TEXT("Source.json"),TEXT("original"),Error)
        &&State.Edit(B)&&State.Draft().bEdited&&State.Draft().SourcePath==TEXT("Source.json")&&State.Draft().SourceDigest==TEXT("original"));
    Row(TEXT("NativeEditingBackToOriginalClearsEditedFlag"),State.Edit(A)&&!State.Draft().bEdited);
    const auto Provenance=State.Provenance(TEXT("Snapshot.json"),TEXT("consumed"));
    Row(TEXT("OriginalSourceDigestIsSeparateFromConsumedDigest"),Provenance->GetStringField(TEXT("sourceOriginalDigest"))==TEXT("original")
        &&Provenance->GetStringField(TEXT("snapshotDigest"))==TEXT("consumed")&&Provenance->GetStringField(TEXT("contentSource"))==TEXT("currentDraft"));
    const FString Chinese65535=FString::ChrN(21845,TCHAR(0x6c34));
    Row(TEXT("ChineseUtf8Bytes65535Accepted"),FNodeForgePanelState::ValidateExternalText(Chinese65535,Error));
    Row(TEXT("ChineseUtf8Bytes65536Accepted"),FNodeForgePanelState::ValidateExternalText(Chinese65535+TEXT("a"),Error));
    Row(TEXT("ChineseUtf8Bytes65537Rejected"),!FNodeForgePanelState::ValidateExternalText(Chinese65535+TEXT("ab"),Error));
    Row(TEXT("Ascii65536Accepted"),FNodeForgePanelState::ValidateExternalText(FString::ChrN(65536,TEXT('a')),Error));
    Row(TEXT("Ascii65537Rejected"),!FNodeForgePanelState::ValidateExternalText(FString::ChrN(65537,TEXT('a')),Error));
    Row(TEXT("NativeOversizeEditingIsRetainedForRepair"),State.Edit(FString::ChrN(65537,TEXT('a')))&&State.Draft().Text.Len()==65537);
    const uint64 OversizeRevision=State.Revision();
    Row(TEXT("OversizeExternalLoadDoesNotReplaceOrTruncate"),!State.Replace(FString::ChrN(65538,TEXT('b')),TEXT("file"),TEXT("Bad.json"),TEXT("bad"),Error)
        &&State.Revision()==OversizeRevision&&State.Draft().Text.Len()==65537);
    Row(TEXT("BomClipboardRejectedBeforeReplacingDraft"),!State.Replace(FString::Chr(TCHAR(0xfeff))+A,TEXT("clipboard"),FString(),FString(),Error)&&State.Revision()==OversizeRevision);
    const FString File=Work/TEXT("Reader.json");FString Text,Hash;
    Row(TEXT("CreateOwnedReaderFile"),FFileHelper::SaveStringToFile(B,*File,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
    Row(TEXT("ReaderPreservesUnicodeAndExactByteDigest"),NodeForgePrototype::ReadInputFile(File,Text,Hash,Error)
        &&Text.Equals(B,ESearchCase::CaseSensitive)&&Hash.Equals(NodeForgePrototype::FileSha256(File),ESearchCase::CaseSensitive));
    Row(TEXT("MissingReaderFileClearsOutputs"),!NodeForgePrototype::ReadInputFile(Work/TEXT("Missing.json"),Text,Hash,Error)&&Text.IsEmpty()&&Hash.IsEmpty());
    const FString OversizeFile=Work/TEXT("Oversize.json");
    FFileHelper::SaveStringToFile(FString::ChrN(65537,TEXT('a')),*OversizeFile,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    Row(TEXT("OversizeReaderDoesNotReturnPartialDraft"),!NodeForgePrototype::ReadInputFile(OversizeFile,Text,Hash,Error)&&Text.IsEmpty()&&Hash.IsEmpty());

    // Same real parser/preflight used by the panel, in this isolated test project.
    FString Fixture;TSharedPtr<FJsonObject> Request;
    const bool FixtureLoaded=FFileHelper::LoadFileToString(Fixture,*(Root/TEXT("Tests/GraphV2/MaterialDefaults.json")))
        &&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Fixture),Request)&&Request;
    Row(TEXT("ActualExistingGraphFixtureLoaded"),FixtureLoaded);
    if(FixtureLoaded)
    {
        // A historical fixture is topology evidence, not the current engine catalogue.
        const FString CatalogPath=Work/TEXT("ActualCatalog.json");
        FString CatalogText;TSharedPtr<FJsonObject> CatalogReport;
        const bool CatalogReady=NodeForgePrototype::Execute(TEXT("GraphCatalog"),FString(),CatalogPath)==0
            &&FFileHelper::LoadFileToString(CatalogText,*CatalogPath)
            &&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(CatalogText),CatalogReport)&&CatalogReport
            &&CatalogReport->HasTypedField<EJson::String>(TEXT("catalogDigest"));
        Row(TEXT("ActualNativeCatalogRetrieved"),CatalogReady);
        if(!CatalogReady) return false;
        Request->SetStringField(TEXT("catalogDigest"),CatalogReport->GetStringField(TEXT("catalogDigest")));
        const FString Id=TEXT("PanelGuard")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
        Request->SetStringField(TEXT("requestId"),Id);Request->SetStringField(TEXT("assetPath"),TEXT("/Game/NodeForgeTests/")+Id);
        const FString Input=Work/TEXT("PreflightInput.json");
        Row(TEXT("CreateNewPreflightSnapshot"),NodeForgePrototype::WriteJson(Request.ToSharedRef(),Input));
        auto Context=MakeShared<FJsonObject>(),Ui=MakeShared<FJsonObject>();
        Ui->SetStringField(TEXT("snapshotPath"),Input);Ui->SetStringField(TEXT("snapshotDigest"),NodeForgePrototype::FileSha256(Input));
        Context->SetStringField(TEXT("inputMethod"),TEXT("draft"));Context->SetObjectField(TEXT("uiInput"),Ui);
        Context->SetStringField(TEXT("overallStatus"),TEXT("forgedSuccess"));
        const FString Good=Work/TEXT("GoodPreflight.json");
        const int32 GoodCode=NodeForgePrototype::Execute(TEXT("GraphPreflight"),Input,Good,FString(),Context);
        FString Raw;TSharedPtr<FJsonObject> Report;
        const bool ReportRead=FFileHelper::LoadFileToString(Raw,*Good)&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Raw),Report)&&Report;
        Row(TEXT("UiProvenanceWrittenOnFirstNativeReport"),GoodCode==0&&ReportRead&&Report->GetStringField(TEXT("inputMethod"))==TEXT("draft")
            &&Report->HasTypedField<EJson::Object>(TEXT("uiInput"))&&Report->GetStringField(TEXT("inputDigest"))==NodeForgePrototype::FileSha256(Input));
        Row(TEXT("ContextCannotOverrideContractStatus"),ReportRead&&Report->GetStringField(TEXT("overallStatus"))==TEXT("succeeded"));
        const FString GoodDigest=NodeForgePrototype::FileSha256(Good);
        AddExpectedError(TEXT("NodeForge refuses to overwrite report"),EAutomationExpectedErrorFlags::Contains,1);
        Row(TEXT("ExistingProvenanceReportIsNeverRewritten"),NodeForgePrototype::Execute(TEXT("GraphPreflight"),Input,Good,FString(),Context)!=0&&NodeForgePrototype::FileSha256(Good)==GoodDigest);
        Ui->SetStringField(TEXT("snapshotDigest"),FString::ChrN(64,TEXT('0')));
        const FString Bad=Work/TEXT("WrongDigest.json");
        const FString Reservation=FPaths::ProjectSavedDir()/TEXT("NodeForge/Requests")/(Id+TEXT(".json"));
        Row(TEXT("ChangedSnapshotRejectedBeforeAnyConstruction"),NodeForgePrototype::Execute(TEXT("GraphPreflight"),Input,Bad,FString(),Context)!=0
            &&!IFileManager::Get().FileExists(*Reservation));
    }
    auto Result=MakeShared<FJsonObject>();Result->SetStringField(TEXT("test"),TEXT("NodeForge.PanelInputGuards"));
    Result->SetStringField(TEXT("status"),HasAnyErrors()?TEXT("failed"):TEXT("passed"));Result->SetArrayField(TEXT("cases"),Rows);
    Result->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());
    Result->SetStringField(TEXT("moduleFilename"),FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor")));
    Result->SetStringField(TEXT("moduleFileSha256"),NodeForgePrototype::FileSha256(FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"))));
    Result->SetStringField(TEXT("evidenceDirectory"),Work);
    Result->SetStringField(TEXT("scope"),TEXT("Native production draft-state, bounded reader and real metadata/preflight guards only. No real UI clicks, file dialog, clipboard, import/save/reopen or runtime proof."));
    return NodeForgePrototype::WriteJson(Result,Output)&&!HasAnyErrors();
}
#endif
