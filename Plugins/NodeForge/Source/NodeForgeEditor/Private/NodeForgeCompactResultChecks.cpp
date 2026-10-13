// Project-owned regression for bounded presentation of actual reports. No assets are changed.
#include "NodeForgeResultView.h"
#include "NodeForgePrototype.h"
#include "NodeForgeGraphDiff.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeCompactResultsTest,"NodeForge.CompactResults",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNodeForgeCompactResultsTest::RunTest(const FString& Parameters)
{
    FString Output;
    const FString Root=NodeForgePrototype::WorkspaceDirectory();
    if(!FParse::Value(FCommandLine::Get(),TEXT("NFReport="),Output)
        || IFileManager::Get().FileExists(*Output)
        || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Output),Root/TEXT("Reports")))
    {AddError(TEXT("Use a new NFReport inside this project's Reports."));return false;}
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Output),true);
    TArray<TSharedPtr<FJsonValue>> Rows;
    auto Row=[&](const FString& Name,bool Passed,const FString& Summary)
    {
        TestTrue(Name,Passed);
        const auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("case"),Name);
        Item->SetBoolField(TEXT("passed"),Passed);Item->SetStringField(TEXT("compact"),Summary);
        Item->SetNumberField(TEXT("characterCount"),Summary.Len());Rows.Add(MakeShared<FJsonValueObject>(Item));
    };
    auto Has=[&](const FString& Text,const TCHAR* A,const TCHAR* B)
    {return Text.Contains(A,ESearchCase::CaseSensitive)||Text.Contains(B,ESearchCase::CaseSensitive);};
    struct FCase{const TCHAR* Name;const TCHAR* Path;};
    const FCase Cases[]={
        {TEXT("SavedWaiting"),TEXT("Reports/20261009UsableEditor/PreviewSmoke1/MaterialCopyImport.json")},
        {TEXT("IndependentReopen"),TEXT("Reports/20261009UsableEditor/PreviewSmoke1/MaterialCopyReopen.json")},
        {TEXT("UnknownProperty"),TEXT("Reports/20261008GraphV2/GraphRun2/UnknownPropertyImport.json")},
        {TEXT("CompileFailureResidue"),TEXT("Reports/20261009UsableEditor/GraphPreview1/MaterialCopyImport.json")},
        {TEXT("WrongDll"),TEXT("Reports/20261009UsableEditor/PreviewSmoke1/BlueprintWrongDll.json")},
        {TEXT("UserPreview"),TEXT("Reports/UiPreviews/4BBACC094CCB74754E9592BF8B40E68F.json")}
    };
    for(const FCase& C:Cases)
    {
        const FString Path=Root/C.Path,Before=NodeForgePrototype::FileSha256(Path);
        FString Original,Copy,Detail;TSharedPtr<FJsonObject> Report;
        const bool Loaded=FFileHelper::LoadFileToString(Original,*Path)
            && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Original),Report) && Report;
        const FString Compact=Loaded?NodeForgeResultView::Compact(Report.ToSharedRef()):TEXT("");
        const bool ReadOk=NodeForgeResultView::Read(Path,Copy,Detail);
        bool Semantics=true;
        if(FString(C.Name)==TEXT("SavedWaiting"))
            Semantics=Has(Compact,TEXT("waitingPersistence"),TEXT("待")) && Has(Compact,TEXT("重开"),TEXT("persistence"));
        if(FString(C.Name)==TEXT("CompileFailureResidue"))
            Semantics=Has(Compact,TEXT("compile"),TEXT("编译")) && Has(Compact,TEXT("cleanupRequired"),TEXT("残留"));
        if(FString(C.Name)==TEXT("UnknownProperty")||FString(C.Name)==TEXT("WrongDll"))
            Semantics=Has(Compact,TEXT("rejected"),TEXT("拒绝"))||Compact.Contains(TEXT("未通过"));
        if(FString(C.Name)==TEXT("UserPreview"))
            Semantics=Compact.Contains(TEXT("PulseSpeed")) && Compact.Contains(TEXT("0.9"))
                && (Compact.Contains(TEXT("未生成"))||Compact.Contains(TEXT("不生成")));
        Row(C.Name,Loaded && ReadOk && Copy.Equals(Original,ESearchCase::CaseSensitive)
            && !Before.IsEmpty() && Before==NodeForgePrototype::FileSha256(Path)
            && !Compact.IsEmpty() && Compact.Len()<=1200 && Semantics
            && !Compact.Contains(TEXT("\"reportVersion\"")) && !Compact.Contains(TEXT("\"evidenceRef\"")),Compact);
    }
    const auto Huge=MakeShared<FJsonObject>();Huge->SetStringField(TEXT("mode"),TEXT("Import"));
    Huge->SetStringField(TEXT("overallStatus"),TEXT("cleanupRequired"));
    Huge->SetStringField(TEXT("failedStage"),TEXT("compile"));Huge->SetStringField(TEXT("errorCode"),TEXT("NFLongDiagnostic"));
    Huge->SetStringField(TEXT("nextAction"),TEXT("Native compile failure: ")+FString::ChrN(50000,TEXT('x')));
    const auto Checks=MakeShared<FJsonObject>(),Compile=MakeShared<FJsonObject>(),Persistence=MakeShared<FJsonObject>();
    Compile->SetStringField(TEXT("status"),TEXT("failed"));Compile->SetStringField(TEXT("reason"),TEXT("Material shader rejected ")+FString::ChrN(50000,TEXT('y')));
    Persistence->SetStringField(TEXT("status"),TEXT("notRun"));Checks->SetObjectField(TEXT("compile"),Compile);Checks->SetObjectField(TEXT("persistence"),Persistence);Huge->SetObjectField(TEXT("checks"),Checks);
    const auto Residue=MakeShared<FJsonObject>();Residue->SetBoolField(TEXT("packageRemaining"),true);Huge->SetObjectField(TEXT("residue"),Residue);
    const FString Bounded=NodeForgeResultView::Compact(Huge);
    Row(TEXT("LongFailureStaysBoundedAndKeepsCriticalState"),Bounded.Len()<=1200
        && Has(Bounded,TEXT("compile"),TEXT("编译")) && Bounded.Contains(TEXT("NFLongDiagnostic"))
        && Has(Bounded,TEXT("cleanupRequired"),TEXT("残留"))
        && (Bounded.Contains(TEXT("完整"))||Bounded.Contains(TEXT("省略"))||Bounded.Contains(TEXT("…"))),Bounded);
    const FString Missing=NodeForgeResultView::Compact(MakeShared<FJsonObject>());
    Row(TEXT("MissingStateCannotImplySuccess"),Missing.Len()<=1200 && Missing.Contains(TEXT("不能确认成功")),Missing);
    Row(TEXT("FailureReasonPrecedesTechnicalStage"),Bounded.Find(TEXT("Native compile failure"),ESearchCase::CaseSensitive)>=0
        &&Bounded.Find(TEXT("Native compile failure"),ESearchCase::CaseSensitive)<Bounded.Find(TEXT("失败阶段"),ESearchCase::CaseSensitive),Bounded);
    {
        auto Contradictory=MakeShared<FJsonObject>(),FailedChecks=MakeShared<FJsonObject>(),FailedSave=MakeShared<FJsonObject>();
        Contradictory->SetStringField(TEXT("mode"),TEXT("Import"));Contradictory->SetStringField(TEXT("overallStatus"),TEXT("succeeded"));
        FailedSave->SetStringField(TEXT("status"),TEXT("failed"));FailedSave->SetStringField(TEXT("reason"),TEXT("Owned save did not finish"));
        FailedChecks->SetObjectField(TEXT("save"),FailedSave);Contradictory->SetObjectField(TEXT("checks"),FailedChecks);
        const FString Summary=NodeForgeResultView::Compact(Contradictory);
        Row(TEXT("ContradictoryOverallCannotClaimSuccess"),Summary.Contains(TEXT("状态字段矛盾"))&&Summary.Contains(TEXT("不能确认成功"))
            &&Summary.Contains(TEXT("Owned save did not finish"))&&!Summary.Contains(TEXT("本次操作已完成")),Summary);
    }
    {
        NodeForgeGraphDocument::FGraphDocument Before,After;
        NodeForgeGraphDocument::FBlueprintDefinition Blueprint;Blueprint.ParentClass=TEXT("/Script/Engine.Actor");Before.Asset.Set<NodeForgeGraphDocument::FBlueprintDefinition>(Blueprint);
        NodeForgeGraphDocument::FGraphDefinition Graph;Graph.Id=TEXT("EventGraph");Graph.Kind=TEXT("EventGraph");
        NodeForgeGraphDocument::FNodeDescription Node;Node.Id=TEXT("Print");Node.Class=TEXT("/Script/BlueprintGraph.K2Node_CallFunction");Node.DefaultsPresent=true;
        Node.Defaults.Add(TEXT("InString"),MakeShared<FJsonValueString>(TEXT("AbC")));Graph.Nodes.Add(Node);Before.Graphs.Add(Graph);After=Before;
        After.Graphs[0].Nodes[0].Defaults[TEXT("InString")]=MakeShared<FJsonValueString>(TEXT("abc"));
        auto& OldDefault=Before.Graphs[0].Nodes[0].Defaults;auto& NewDefault=After.Graphs[0].Nodes[0].Defaults;
        const auto Difference=NodeForgeGraphDiff::Compare(Before,After),PreviewReport=MakeShared<FJsonObject>();
        PreviewReport->SetStringField(TEXT("mode"),TEXT("GraphPreview"));PreviewReport->SetStringField(TEXT("overallStatus"),TEXT("succeeded"));PreviewReport->SetObjectField(TEXT("graphDifference"),Difference);
        const FString CaseSummary=NodeForgeResultView::Compact(PreviewReport);
        Row(TEXT("CaseOnlyDefaultIsActualDeltaAndVisibleInSummary"),Difference->GetBoolField(TEXT("requestStructureOrValuesChanged"))
            && Difference->GetArrayField(TEXT("nodesChanged")).Num()==1 && CaseSummary.Contains(TEXT("AbC"),ESearchCase::CaseSensitive)
            && CaseSummary.Contains(TEXT("abc"),ESearchCase::CaseSensitive),CaseSummary);
        OldDefault.Add(TEXT("Magnitude"),MakeShared<FJsonValueNumber>(100000000));NewDefault.Add(TEXT("Magnitude"),MakeShared<FJsonValueNumber>(100000001));
        const auto NumberDifference=NodeForgeGraphDiff::Compare(Before,After);
        PreviewReport->SetObjectField(TEXT("graphDifference"),NumberDifference);
        const FString NumberSummary=NodeForgeResultView::Compact(PreviewReport);
        Row(TEXT("DistinctLargeNumericDeltaIsVisible"),NumberDifference->GetArrayField(TEXT("nodesChanged")).Num()==1
            &&NumberSummary.Contains(TEXT("100000000"),ESearchCase::CaseSensitive)
            &&NumberSummary.Contains(TEXT("100000001"),ESearchCase::CaseSensitive),NumberSummary);
        After.Graphs[0].Nodes[0].Id=TEXT("print");
        const auto Rename=NodeForgeGraphDiff::Compare(Before,After);
        auto OldObject=MakeShared<FJsonObject>(),NewObject=MakeShared<FJsonObject>();OldObject->Values=OldDefault;NewObject->Values=NewDefault;
        Row(TEXT("CaseOnlyLogicalIdIsRemoveAdd"),Rename->GetArrayField(TEXT("nodesAdded")).Num()==1
            && Rename->GetArrayField(TEXT("nodesRemoved")).Num()==1 && Rename->GetArrayField(TEXT("nodesChanged")).IsEmpty(),TEXT("Print -> print: remove/add, no guessed match"));
        Row(TEXT("JsonOrderIgnoredButExactKeysAndScalarTypesRetained"),NodeForgeGraphDiff::EqualValue(MakeShared<FJsonValueNumber>(1),MakeShared<FJsonValueNumber>(1))
            && !NodeForgeGraphDiff::EqualValue(MakeShared<FJsonValueString>(TEXT("AbC")),MakeShared<FJsonValueString>(TEXT("abc")))
            && !NodeForgeGraphDiff::EqualValue(MakeShared<FJsonValueString>(TEXT("1")),MakeShared<FJsonValueNumber>(1))
            && !NodeForgeGraphDiff::EqualValue(MakeShared<FJsonValueObject>(OldObject),MakeShared<FJsonValueObject>(NewObject)),TEXT("native primitive equality, exact string values/object keys"));
    }
    const auto Result=MakeShared<FJsonObject>();Result->SetStringField(TEXT("test"),TEXT("NodeForge.CompactResults"));
    Result->SetStringField(TEXT("status"),HasAnyErrors()?TEXT("failed"):TEXT("passed"));Result->SetArrayField(TEXT("cases"),Rows);
    Result->SetStringField(TEXT("moduleFilename"),FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor")));
    Result->SetStringField(TEXT("moduleFileSha256"),NodeForgePrototype::FileSha256(FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"))));
    Result->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());Result->SetStringField(TEXT("timeUtc"),FDateTime::UtcNow().ToIso8601());
    Result->SetStringField(TEXT("scope"),TEXT("Actual frozen report compact display and exact original read; bounded huge diagnostics. No clipboard/UI clicks or asset generation implied."));
    return NodeForgePrototype::WriteJson(Result,Output) && !HasAnyErrors();
}
#endif
