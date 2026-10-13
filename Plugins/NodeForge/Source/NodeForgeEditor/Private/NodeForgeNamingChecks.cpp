// Project-owned regressions for explicit copy preparation; uses the production
// parser, native name service, report path and existing draft-state implementation.
#include "NodeForgePrototype.h"
#include "NodeForgePanelState.h"
#include "NodeForgeResultView.h"
#include "NodeForgeGraphDiff.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "Misc/ScopeExit.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeNamingCopiesTest,"NodeForge.NamingCopies",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNodeForgeNamingCopiesTest::RunTest(const FString& Parameters)
{
    FString Output;const FString Workspace=NodeForgePrototype::WorkspaceDirectory();
    if(!FParse::Value(FCommandLine::Get(),TEXT("NFNamingReport="),Output)
        || IFileManager::Get().FileExists(*Output)
        || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Output),Workspace/TEXT("Reports")))
    {AddError(TEXT("Use a fresh NFNamingReport in workspace Reports."));return false;}
    const FString Work=FPaths::GetPath(Output)/TEXT("NamingEvidence");
    TArray<TSharedPtr<FJsonValue>> Rows;TArray<UPackage*> OwnedPackages;
    ON_SCOPE_EXIT{for(UPackage* Package:OwnedPackages){Package->RemoveFromRoot();Package->MarkAsGarbage();}};
    auto Row=[&](const TCHAR* Name,bool Passed)
    {TestTrue(Name,Passed);auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("case"),Name);Item->SetBoolField(TEXT("passed"),Passed);Rows.Add(MakeShared<FJsonValueObject>(Item));};
    auto Read=[](const FString& Path)->TSharedPtr<FJsonObject>
    {FString Text;TSharedPtr<FJsonObject> Json;if(FFileHelper::LoadFileToString(Text,*Path))FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Json);return Json;};
    auto OwnPackage=[&](const FString& Path)
    {UPackage* Package=CreatePackage(*Path);Package->AddToRoot();OwnedPackages.Add(Package);return Package;};
    const FString CatalogFile=Work/TEXT("Catalog.json");
    const bool CatalogOk=NodeForgePrototype::Execute(TEXT("GraphCatalog"),FString(),CatalogFile)==0;
    const auto Catalog=Read(CatalogFile);
    auto Fixture=Read(Workspace/TEXT("Tests/GraphV2/MaterialDefaults.json"));
    Row(TEXT("ActualCatalogAndExistingFixture"),CatalogOk && Catalog && Fixture);
    if(!CatalogOk || !Catalog || !Fixture)return false;
    Fixture->SetStringField(TEXT("catalogDigest"),Catalog->GetStringField(TEXT("catalogDigest")));
    const FString Id=TEXT("Naming")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    const FString Original=TEXT("/Game/NodeForgeTests/")+Id;
    Fixture->SetStringField(TEXT("requestId"),Id);Fixture->SetStringField(TEXT("assetPath"),Original);
    const FString Input=Work/TEXT("Original.json");
    Row(TEXT("OriginalInputSaved"),NodeForgePrototype::WriteJson(Fixture.ToSharedRef(),Input));
    const FString OriginalHash=NodeForgePrototype::FileSha256(Input);
    OwnPackage(Original);
    auto Conflict=[&](const FString& Mode,const FString& Name)->TSharedPtr<FJsonObject>
    {
        const FString ReportPath=Work/(Name+TEXT(".json"));
        const int32 Code=NodeForgePrototype::Execute(Mode,Input,ReportPath);
        auto Report=Read(ReportPath);
        Row(*(Name+TEXT("Rejected")),Code!=0 && Report && Report->GetStringField(TEXT("overallStatus"))==TEXT("rejected")
            && Report->GetObjectField(TEXT("checks"))->GetObjectField(TEXT("build"))->GetStringField(TEXT("status"))==TEXT("notRun"));
        return Report;
    };
    auto Target=Conflict(TEXT("Import"),TEXT("TargetOnly"));
    Row(TEXT("TargetOnlyHasPreciseConflict"),Target && Target->GetObjectField(TEXT("conflict"))->GetStringField(TEXT("kind"))==TEXT("targetExists")
        && Target->GetStringField(TEXT("errorCode"))==TEXT("NFERR-TARGET-EXISTS"));
    const FString Reservation=FPaths::ProjectSavedDir()/TEXT("NodeForge/Requests")/(Id+TEXT(".json"));
    Row(TEXT("TestOwnRequestReceiptSaved"),NodeForgePrototype::WriteJson(MakeShared<FJsonObject>(),Reservation));
    auto Both=Conflict(TEXT("Import"),TEXT("Both"));
    Row(TEXT("BothConflictNotCollapsed"),Both && Both->GetObjectField(TEXT("conflict"))->GetStringField(TEXT("kind"))==TEXT("targetAndRequestExist"));
    auto Preflight=Conflict(TEXT("GraphPreflight"),TEXT("PreflightBoth"));
    Row(TEXT("PreflightUsesSameConflictContract"),Preflight && Preflight->GetStringField(TEXT("errorCode"))==TEXT("NFERR-TARGET-AND-REQUEST-EXIST"));
    auto Preview=Conflict(TEXT("GraphPreview"),TEXT("PreviewBoth"));
    Row(TEXT("PreviewUsesSameConflictContract"),Preview && Preview->GetObjectField(TEXT("conflict"))->GetStringField(TEXT("kind"))==TEXT("targetAndRequestExist"));
    auto RequestOnly=MakeShared<FJsonObject>();RequestOnly->Values=Fixture->Values;
    RequestOnly->SetStringField(TEXT("assetPath"),Original+TEXT("Fresh"));
    const FString RequestOnlyInput=Work/TEXT("RequestOnlyInput.json");
    Row(TEXT("RequestOnlyInputSaved"),NodeForgePrototype::WriteJson(RequestOnly,RequestOnlyInput));
    const FString RequestOnlyReport=Work/TEXT("RequestOnly.json");
    const int32 RequestCode=NodeForgePrototype::Execute(TEXT("Import"),RequestOnlyInput,RequestOnlyReport);
    auto RequestReport=Read(RequestOnlyReport);
    Row(TEXT("UsedRequestWithFreeTargetIsRejectedSeparately"),RequestCode!=0 && RequestReport
        && RequestReport->GetStringField(TEXT("errorCode"))==TEXT("NFERR-REQUEST-EXISTS")
        && RequestReport->GetObjectField(TEXT("conflict"))->GetStringField(TEXT("kind"))==TEXT("requestExists"));

    auto Prepare=[&](const FString& Source,const FString& Name)->TSharedPtr<FJsonObject>
    {
        const FString ReportPath=Work/(Name+TEXT(".json"));
        const int32 Code=NodeForgePrototype::Execute(TEXT("PrepareNewCopy"),Source,ReportPath);
        auto Report=Read(ReportPath);
        Row(*(Name+TEXT("Prepared")),Code==0 && Report && Report->GetStringField(TEXT("operationStatus"))==TEXT("passed")
            && Report->GetObjectField(TEXT("checks"))->GetObjectField(TEXT("save"))->GetStringField(TEXT("status"))==TEXT("notRun"));
        return Code==0 && Report && Report->HasTypedField<EJson::Object>(TEXT("copyPreparation"))?Report->GetObjectField(TEXT("copyPreparation")):nullptr;
    };
    auto First=Prepare(Input,TEXT("First"));
    Row(TEXT("FirstNameIsCopy1"),First && First->GetStringField(TEXT("newAssetPath"))==Original+TEXT("Copy1"));
    if(!First)return false;
    const FString PreparedInput=First->GetStringField(TEXT("preparedInputPath"));
    const auto Derived=Read(PreparedInput);
    bool OnlyIdentity=Derived && Derived->Values.Num()==Fixture->Values.Num();
    for(const auto& Pair:Fixture->Values)
        if(Pair.Key!=TEXT("assetPath") && Pair.Key!=TEXT("requestId"))
        {const auto* Value=Derived?Derived->Values.Find(Pair.Key):nullptr;OnlyIdentity&=Value && NodeForgeGraphDiff::EqualValue(Pair.Value,*Value);}
    Row(TEXT("EveryNonIdentityFieldPreserved"),OnlyIdentity);
    const FString CopyReceipt=FPaths::ProjectSavedDir()/TEXT("NodeForge/Requests")/(First->GetStringField(TEXT("newRequestId"))+TEXT(".json"));
    Row(TEXT("PreparationDoesNotReserveOrGenerate"),!IFileManager::Get().FileExists(*CopyReceipt)
        && !FindPackage(nullptr,*First->GetStringField(TEXT("newAssetPath")))
        && !FPackageName::DoesPackageExist(First->GetStringField(TEXT("newAssetPath"))));
    auto Again=Prepare(Input,TEXT("Again"));
    Row(TEXT("UnreservedProposalIsReusableButRequestIsFresh"),Again && Again->GetStringField(TEXT("newAssetPath"))==First->GetStringField(TEXT("newAssetPath"))
        && Again->GetStringField(TEXT("newRequestId"))!=First->GetStringField(TEXT("newRequestId")));
    OwnPackage(First->GetStringField(TEXT("newAssetPath")));
    auto Second=Prepare(Input,TEXT("Second"));
    Row(TEXT("InMemoryOccupiedCopy1ProducesCopy2"),Second && Second->GetStringField(TEXT("newAssetPath"))==Original+TEXT("Copy2"));
    auto Chained=Prepare(PreparedInput,TEXT("Chained"));
    Row(TEXT("CopySuffixDoesNotGrowIntoCopy1Copy1"),Chained && Chained->GetStringField(TEXT("newAssetPath"))==Original+TEXT("Copy2"));
    const FString RaceReport=Work/TEXT("OccupiedAfterPrepare.json");
    const int32 RaceCode=NodeForgePrototype::Execute(TEXT("Import"),PreparedInput,RaceReport);
    const auto Race=Read(RaceReport);
    Row(TEXT("TargetOccupiedAfterPrepareStillRejectsBeforeReservation"),RaceCode!=0 && Race
        && Race->GetStringField(TEXT("errorCode"))==TEXT("NFERR-TARGET-EXISTS") && !IFileManager::Get().FileExists(*CopyReceipt));
    for(const auto& Pair:TArray<TPair<FString,FString>>{{TEXT("Long"),FString::ChrN(64,TEXT('L'))},{TEXT("Numeric"),FString::ChrN(64,TEXT('7'))}})
    {
        auto EdgeCase=MakeShared<FJsonObject>();EdgeCase->Values=Fixture->Values;
        EdgeCase->SetStringField(TEXT("assetPath"),TEXT("/Game/NodeForgeTests/")+Pair.Value);
        const FString EdgeInput=Work/(Pair.Key+TEXT("Input.json"));
        Row(*(Pair.Key+TEXT("InputSaved")),NodeForgePrototype::WriteJson(EdgeCase,EdgeInput));
        auto Proposal=Prepare(EdgeInput,Pair.Key);
        const FString Name=Proposal?FPaths::GetCleanFilename(Proposal->GetStringField(TEXT("newAssetPath"))):FString();
        Row(*(Pair.Key+TEXT("UsesBoundedAlphanumericCopyName")),!Name.IsEmpty() && Name.Len()<=64 && !Name.Contains(TEXT("_")) && Name.EndsWith(TEXT("Copy1")));
    }
    auto Invalid=MakeShared<FJsonObject>();Invalid->Values=Fixture->Values;Invalid->SetStringField(TEXT("unexpected"),TEXT("no"));
    const FString InvalidInput=Work/TEXT("InvalidInput.json"),InvalidReport=Work/TEXT("Invalid.json");
    Row(TEXT("InvalidInputSaved"),NodeForgePrototype::WriteJson(Invalid,InvalidInput));
    const int32 InvalidCode=NodeForgePrototype::Execute(TEXT("PrepareNewCopy"),InvalidInput,InvalidReport);
    const auto Bad=Read(InvalidReport);
    Row(TEXT("InvalidGraphNotRepairedByRenaming"),InvalidCode!=0 && Bad && !Bad->HasField(TEXT("copyPreparation")));

    FString OriginalText,PreparedText,Hash,Error;
    FNodeForgePanelState State;
    const bool TextReady=NodeForgePrototype::ReadInputFile(Input,OriginalText,Hash,Error)
        && NodeForgePrototype::ReadInputFile(PreparedInput,PreparedText,Hash,Error)
        && State.Replace(OriginalText,TEXT("file"),Input,OriginalHash,Error);
    Row(TEXT("PreparedDraftPreservesOriginalFileProvenance"),TextReady && State.PrepareCopy(PreparedText,Work/TEXT("First.json"),Error)
        && State.Draft().SourcePath==Input && State.Draft().SourceDigest==OriginalHash && State.Draft().bEdited
        && State.Provenance(TEXT("snapshot"),TEXT("digest"))->GetStringField(TEXT("copyPreparationReport"))==Work/TEXT("First.json"));
    Row(TEXT("PreparedDraftCanRestoreOriginalExactly"),State.Restore() && State.Draft().Text.Equals(OriginalText,ESearchCase::CaseSensitive)
        && State.Draft().PreparationReport.IsEmpty());
    const auto PreparationReport=Read(Work/TEXT("First.json"));
    Row(TEXT("PreparedSummaryDoesNotClaimAssetSaved"),PreparationReport && NodeForgeResultView::Compact(PreparationReport.ToSharedRef()).Contains(TEXT("尚未生成资产"))
        && !NodeForgeResultView::Compact(PreparationReport.ToSharedRef()).Contains(TEXT("资产已保存")));
    Row(TEXT("ConflictSummaryDoesNotInventNewResidue"),Both && !NodeForgeResultView::Compact(Both.ToSharedRef()).Contains(TEXT("请先核对残留")));
    Row(TEXT("OriginalInputBytesNeverChanged"),NodeForgePrototype::FileSha256(Input)==OriginalHash);

    auto Result=MakeShared<FJsonObject>();Result->SetStringField(TEXT("test"),TEXT("NodeForge.NamingCopies"));
    Result->SetStringField(TEXT("status"),HasAnyErrors()?TEXT("failed"):TEXT("passed"));Result->SetArrayField(TEXT("cases"),Rows);
    Result->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());
    const FString Module=FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"));
    Result->SetStringField(TEXT("moduleFilename"),Module);Result->SetStringField(TEXT("moduleFileSha256"),NodeForgePrototype::FileSha256(Module));
    Result->SetStringField(TEXT("scope"),TEXT("Production parser/native naming, identity-only preparation, precise conflicts, transient target races, provenance and summaries. No real UI clicks, saved graph generation, reopen or visual proof."));
    return NodeForgePrototype::WriteJson(Result,Output) && !HasAnyErrors();
}
#endif
