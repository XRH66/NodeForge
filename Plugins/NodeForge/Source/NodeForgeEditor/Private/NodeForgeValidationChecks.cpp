// Project-owned native API validation batch; not a replacement for human UI tests.
#include "NodeForgePrototype.h"
#include "NodeForgeResultView.h"
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

namespace
{
TSharedPtr<FJsonObject> LoadValidationJson(const FString& Path,bool IsReport=false)
{
    FString Text,Digest,Error;TSharedPtr<FJsonObject> Json;
    const bool Read=IsReport?NodeForgeResultView::Read(Path,Text,Error):NodeForgePrototype::ReadInputFile(Path,Text,Digest,Error);
    if(Read)FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Json);
    return Json;
}
TSharedRef<FJsonObject> ValidationFileSnapshot()
{
    const auto Snapshot=MakeShared<FJsonObject>();
    for(const FString& Root:{FPaths::ProjectContentDir(),FPaths::ProjectSavedDir()/TEXT("NodeForge/Requests")})
    {
        TArray<FString> Files;IFileManager::Get().FindFilesRecursive(Files,*Root,TEXT("*"),true,false);Files.Sort();
        for(const FString& File:Files)Snapshot->SetStringField(File,NodeForgePrototype::FileSha256(File));
    }
    return Snapshot;
}
bool SameValidationSnapshot(const TSharedRef<FJsonObject>& A,const TSharedRef<FJsonObject>& B)
{
    return FJsonValue::CompareEqual(*MakeShared<FJsonValueObject>(A),*MakeShared<FJsonValueObject>(B));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeValidationBatchTest,"NodeForge.ValidationBatch",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNodeForgeValidationBatchTest::RunTest(const FString& Parameters)
{
    FString CasesPath,Output;
    if(!FParse::Value(FCommandLine::Get(),TEXT("NFValidationCases="),CasesPath)
        || !FParse::Value(FCommandLine::Get(),TEXT("NFReport="),Output)
        || IFileManager::Get().FileExists(*Output))
    {AddError(TEXT("Fresh report and explicit validation cases required."));return false;}
    const FString Root=NodeForgePrototype::WorkspaceDirectory()/TEXT("Reports");
    if(!FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Output),Root)
        || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(CasesPath),Root))
    {AddError(TEXT("Validation evidence must stay under local Reports."));return false;}
    const auto Config=LoadValidationJson(CasesPath);
    const TArray<TSharedPtr<FJsonValue>>* Cases=nullptr;
    if(!Config || !Config->TryGetArrayField(TEXT("cases"),Cases) || Cases->IsEmpty() || Cases->Num()>40)
    {AddError(TEXT("Bounded explicit validation cases required."));return false;}
    const auto Before=ValidationFileSnapshot();
    TArray<TSharedPtr<FJsonValue>> Evidence;
    for(const auto& Value:*Cases)
    {
        const auto C=Value->AsObject();FString Name,Input,Mode,Expected,Previous;
        if(!C || !C->TryGetStringField(TEXT("name"),Name) || !C->TryGetStringField(TEXT("input"),Input)
            || !C->TryGetStringField(TEXT("mode"),Mode) || !C->TryGetStringField(TEXT("expected"),Expected)
            || Name.IsEmpty() || Name.Len()>64 || Name.Contains(TEXT("/")) || Name.Contains(TEXT("\\")) || Name.Contains(TEXT(".."))
            || (Mode!=TEXT("GraphPreflight") && Mode!=TEXT("Import") && Mode!=TEXT("Verify"))
            || (Expected!=TEXT("succeeded") && Expected!=TEXT("rejected") && Expected!=TEXT("failed"))
            || (Expected==TEXT("succeeded") && Mode!=TEXT("GraphPreflight")))
        {AddError(TEXT("Batch only accepts successful preflight or explicit negative Import/Verify cases."));return false;}
        C->TryGetStringField(TEXT("previous"),Previous);
        const FString Report=FPaths::GetPath(Output)/(Name+TEXT(".json"));
        if(IFileManager::Get().FileExists(*Report)){AddError(TEXT("Case report exists; no overwrite."));return false;}
        const FString InputBefore=NodeForgePrototype::FileSha256(Input);
        const int32 Code=NodeForgePrototype::Execute(Mode,Input,Report,Previous);
        const auto Result=LoadValidationJson(Report,true);
        FString Status,Disk,Description,Original;
        bool Passed=Result && Result->TryGetStringField(TEXT("overallStatus"),Status) && Status==Expected
            && (Code==0)==(Expected==TEXT("succeeded")) && !InputBefore.IsEmpty()
            && InputBefore==NodeForgePrototype::FileSha256(Input);
        if(Result)
        {
            const auto Checks=Result->GetObjectField(TEXT("checks"));
            for(const TCHAR* Stage:{TEXT("build"),TEXT("initialReadback"),TEXT("compile"),TEXT("finalReadback"),TEXT("publish"),TEXT("save"),TEXT("runtime")})
                Passed &= Checks->GetObjectField(Stage)->GetStringField(TEXT("status"))==TEXT("notRun");
            Passed &= Result->GetObjectField(TEXT("residue"))->TryGetStringField(TEXT("diskState"),Disk) && Disk==TEXT("notWritten");
            const bool ReadOk=NodeForgeResultView::Read(Report,Original,Description);
            Passed &= ReadOk && Description.Contains(Expected);
            if(Expected==TEXT("succeeded"))Passed &= Result->HasTypedField<EJson::Object>(TEXT("preflightSummary")) && Description.Contains(TEXT("尚未生成"));
            else Passed &= !Result->HasField(TEXT("preflightSummary"));
        }
        Passed &= SameValidationSnapshot(Before,ValidationFileSnapshot());
        TestTrue(Name,Passed);
        const auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("name"),Name);Item->SetStringField(TEXT("mode"),Mode);
        Item->SetStringField(TEXT("input"),Input);Item->SetStringField(TEXT("report"),Report);Item->SetNumberField(TEXT("code"),Code);
        Item->SetStringField(TEXT("expected"),Expected);Item->SetStringField(TEXT("actual"),Status);Item->SetBoolField(TEXT("passed"),Passed);
        Item->SetStringField(TEXT("inputSha256"),InputBefore);Evidence.Add(MakeShared<FJsonValueObject>(Item));
    }
    const auto Result=MakeShared<FJsonObject>();Result->SetStringField(TEXT("status"),HasAnyErrors()?TEXT("failed"):TEXT("passed"));
    Result->SetStringField(TEXT("test"),TEXT("NodeForge.ValidationBatch"));Result->SetArrayField(TEXT("cases"),Evidence);
    Result->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());
    Result->SetStringField(TEXT("timeUtc"),FDateTime::UtcNow().ToIso8601());
    const FString Binary=FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"));
    Result->SetStringField(TEXT("actualLoadedBinary"),Binary);Result->SetStringField(TEXT("actualLoadedBinarySha256"),NodeForgePrototype::FileSha256(Binary));
    Result->SetBoolField(TEXT("projectAssetsAndReservationsUnchanged"),SameValidationSnapshot(Before,ValidationFileSnapshot()));
    Result->SetStringField(TEXT("scope"),TEXT("Real API preflight/negative requests and result presentation; no target graph creation or save, UI buttons/clipboard not tested"));
    return NodeForgePrototype::WriteJson(Result,Output) && !HasAnyErrors();
}
#endif
