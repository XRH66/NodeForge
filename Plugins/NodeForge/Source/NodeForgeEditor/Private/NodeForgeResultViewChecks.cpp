// Real frozen reports exercise presentation without building or modifying assets.
#include "NodeForgeResultView.h"
#include "NodeForgePrototype.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeResultViewTest,"NodeForge.ResultView",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNodeForgeResultViewTest::RunTest(const FString& Parameters)
{
    FString Output;
    if(!FParse::Value(FCommandLine::Get(),TEXT("NFReport="),Output) || IFileManager::Get().FileExists(*Output))
    {AddError(TEXT("Use an unused NFReport evidence path."));return false;}
    const FString Root=NodeForgePrototype::WorkspaceDirectory();
    if(!FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Output),Root/TEXT("Reports")))
    {AddError(TEXT("Test evidence must stay under Reports."));return false;}
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Output),true);
    struct FCase{const TCHAR* Name;const TCHAR* Relative;const TCHAR* State;const TCHAR* Detail;};
    const FCase Cases[]={
        {TEXT("SavedWaiting"),TEXT("Reports/20261009UsableEditor/PreviewSmoke1/MaterialCopyImport.json"),TEXT("waitingPersistence"),TEXT("persistence：notRun")},
        {TEXT("IndependentReopen"),TEXT("Reports/20261009UsableEditor/PreviewSmoke1/MaterialCopyReopen.json"),TEXT("succeeded"),TEXT("persistence：passed")},
        {TEXT("UnknownProperty"),TEXT("Reports/20261008GraphV2/GraphRun2/UnknownPropertyImport.json"),TEXT("rejected"),TEXT("validation：failed")},
        {TEXT("CompileFailureResidue"),TEXT("Reports/20261009UsableEditor/GraphPreview1/MaterialCopyImport.json"),TEXT("cleanupRequired"),TEXT("失败阶段：compile")},
        {TEXT("WrongDll"),TEXT("Reports/20261009UsableEditor/PreviewSmoke1/BlueprintWrongDll.json"),TEXT("rejected"),TEXT("matching successful source export")},
        {TEXT("RealUiReadOnlyExport"),TEXT("Reports/UiRules/0A4506D74AB21F220507139D51DD1DA7.json"),TEXT("succeeded"),TEXT("save：notRun")}
    };
    TArray<TSharedPtr<FJsonValue>> Evidence;
    for(const FCase& C:Cases)
    {
        const FString Path=Root/C.Relative;const FString Before=NodeForgePrototype::FileSha256(Path);
        FString Copy,Summary,Original;
        const bool ReadOk=NodeForgeResultView::Read(Path,Copy,Summary);
        FFileHelper::LoadFileToString(Original,*Path);
        TSharedPtr<FJsonObject> Report;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Original),Report);
        FString SourceStatus,Reason;
        if(Report){Report->TryGetStringField(TEXT("overallStatus"),SourceStatus);Report->TryGetStringField(TEXT("nextAction"),Reason);}
        const bool Passed=ReadOk && Copy==Original && Report && SourceStatus==C.State && Summary.Contains(C.State)
            && Summary.Contains(C.Detail) && (Reason.IsEmpty() || Summary.Contains(Reason)) &&
            !Before.IsEmpty() && Before==NodeForgePrototype::FileSha256(Path);
        TestTrue(C.Name,Passed);
        const auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("case"),C.Name);
        Item->SetStringField(TEXT("source"),C.Relative);Item->SetStringField(TEXT("sourceSha256"),Before);
        Item->SetBoolField(TEXT("passed"),Passed);Item->SetStringField(TEXT("description"),Summary);
        Item->SetBoolField(TEXT("copyExactOriginal"),Copy==Original);Evidence.Add(MakeShared<FJsonValueObject>(Item));
    }
    const FString Malformed=FPaths::GetPath(Output)/TEXT("Malformed.json");
    const FString Oversized=FPaths::GetPath(Output)/TEXT("Oversized.json");
    if(IFileManager::Get().FileExists(*Malformed) || IFileManager::Get().FileExists(*Oversized))
    {AddError(TEXT("Read-rejection fixtures already exist; nothing overwritten."));return false;}
    FFileHelper::SaveStringToFile(TEXT("{bad json"),*Malformed,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    FFileHelper::SaveStringToFile(FString::ChrN(1024*1024+1,TEXT('x')),*Oversized,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    for(const FString& Path:{Root/TEXT("Reports/NoSuchResultViewReport.json"),Root/TEXT("Catalog/UE532Graph.json"),Malformed,Oversized})
    {
        FString Copy(TEXT("previous result")),Summary(TEXT("previous result"));
        const bool Passed=!NodeForgeResultView::Read(Path,Copy,Summary) && Copy.IsEmpty() && !Summary.IsEmpty() && Summary!=TEXT("previous result");
        TestTrue(TEXT("Invalid read never returns cached result"),Passed);
        const auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("source"),Path);
        Item->SetStringField(TEXT("case"),TEXT("RejectedReadClearsCopy"));Item->SetBoolField(TEXT("passed"),Passed);Evidence.Add(MakeShared<FJsonValueObject>(Item));
    }
    TestTrue(TEXT("Missing status cannot imply success"),NodeForgeResultView::Describe(MakeShared<FJsonObject>()).Contains(TEXT("不能确认成功")));
    const auto Result=MakeShared<FJsonObject>();Result->SetStringField(TEXT("test"),TEXT("NodeForge.ResultView"));
    Result->SetStringField(TEXT("status"),HasAnyErrors()?TEXT("failed"):TEXT("passed"));Result->SetArrayField(TEXT("cases"),Evidence);
    Result->SetStringField(TEXT("scope"),TEXT("Actual report presentation, exact copy payload and rejected reads; no clipboard click/UI acceptance, asset import or new compile/runtime evidence"));
    Result->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());
    Result->SetStringField(TEXT("timeUtc"),FDateTime::UtcNow().ToIso8601());
    return NodeForgePrototype::WriteJson(Result,Output) && !HasAnyErrors();
}
#endif
