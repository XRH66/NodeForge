// Project-owned adversarial checks of native saved Sequence graphs in isolated copies.
#include "CoreMinimal.h"
#include "NodeForgeBlueprintGraph.h"
#include "NodeForgePrototype.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_Knot.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeSequenceExportGuards,"NodeForge.SequenceExportGuards",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNodeForgeSequenceExportGuards::RunTest(const FString& Parameters)
{
    FString Source,ReportPath,Digest;
    FString Project=FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());FPaths::CollapseRelativeDirectories(Project);
    const FString Allowed=FPaths::ConvertRelativePathToFull(NodeForgePrototype::WorkspaceDirectory()/TEXT("ValidationProject/RegressionRuns"))+TEXT("/");
    if(!Project.StartsWith(Allowed,ESearchCase::IgnoreCase)
        || !FParse::Value(FCommandLine::Get(),TEXT("NFExportSource="),Source)
        || !FParse::Value(FCommandLine::Get(),TEXT("NFCatalogDigest="),Digest) || Digest.Len()!=64
        || !FParse::Value(FCommandLine::Get(),TEXT("NFReport="),ReportPath) || !Source.StartsWith(TEXT("/Game/NodeForgeTests/")))
    {AddError(TEXT("Explicit isolated source, current digest and fresh report required"));return false;}
    UBlueprint* BP=LoadObject<UBlueprint>(nullptr,*Source);
    if(!BP || BP->GetPackage()->IsDirty()){AddError(TEXT("Clean saved generated source required"));return false;}
    UEdGraph* Graph=FBlueprintEditorUtils::FindEventGraph(BP);UK2Node_ExecutionSequence* Sequence=nullptr;
    if(Graph)for(UEdGraphNode* N:Graph->Nodes)if(auto* S=Cast<UK2Node_ExecutionSequence>(N)){Sequence=S;break;}
    if(!Sequence || Sequence->Pins.Num()!=4){AddError(TEXT("A real saved three-output Sequence source is required"));return false;}
    const auto SavedPins=Sequence->Pins;
    const FName SavedName=SavedPins[1]->PinName;const FString SavedDefault=SavedPins[0]->DefaultValue;
    const FString File=FPackageName::LongPackageNameToFilename(BP->GetPackage()->GetName(),FPackageName::GetAssetPackageExtension());
    const FString Before=NodeForgePrototype::FileSha256(File);
    if(Before.Len()!=64){AddError(TEXT("Source fingerprint unavailable"));return false;}
    auto Config=MakeShared<FJsonObject>();Config->SetStringField(TEXT("source"),Source);
    Config->SetStringField(TEXT("requestId"),TEXT("SequenceGuard"));Config->SetStringField(TEXT("assetPath"),TEXT("/Game/NodeForgeTests/NFSequenceGuardCopy"));
    TArray<TSharedPtr<FJsonValue>> Rows;
    auto Check=[&](const FString& Name,bool Expected,const FString& Reason)
    {
        TSharedPtr<FJsonObject> Request,Evidence;FString Error;
        const bool Actual=NodeForgeBlueprintGraph::Export(Config,Digest,Request,Evidence,Error);
        const bool Unchanged=NodeForgePrototype::FileSha256(File)==Before && !BP->GetPackage()->IsDirty();
        const bool Good=Actual==Expected && Unchanged && (Expected?(Request.IsValid() && Evidence.IsValid()):(!Request && !Evidence && Error.Contains(Reason)));
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("case"),Name);Row->SetBoolField(TEXT("expectedExport"),Expected);
        Row->SetStringField(TEXT("result"),Good?TEXT("passed"):TEXT("failed"));Row->SetStringField(TEXT("error"),Error);Row->SetBoolField(TEXT("sourceBytesAndCleanStateUnchanged"),Unchanged);
        Rows.Add(MakeShared<FJsonValueObject>(Row));if(!Good)AddError(TEXT("Sequence source guard failed: ")+Name+TEXT(" / ")+Error);
    };
    Check(TEXT("Original"),true,FString());
    Sequence->Pins.Swap(1,2);Check(TEXT("ReorderedOutputs"),false,TEXT("Sequence"));Sequence->Pins=SavedPins;
    Sequence->Pins.RemoveAt(1);Check(TEXT("MissingMiddleOutput"),false,TEXT("Sequence"));Sequence->Pins=SavedPins;
    SavedPins[1]->PinName=TEXT("UnexpectedOutput");Check(TEXT("RenamedOutput"),false,TEXT("Sequence"));SavedPins[1]->PinName=SavedName;
    SavedPins[0]->DefaultValue=TEXT("invalid");Check(TEXT("ExecutionDefault"),false,TEXT("Sequence"));SavedPins[0]->DefaultValue=SavedDefault;
    auto* Unknown=NewObject<UK2Node_Knot>(Graph);Unknown->CreateNewGuid();Unknown->AllocateDefaultPins();Graph->Nodes.Add(Unknown);
    Check(TEXT("UnsupportedReroute"),false,TEXT("Unsupported blueprint node class"));Graph->Nodes.Remove(Unknown);
    Check(TEXT("RestoredOriginal"),true,FString());
    const bool Restored=Sequence->Pins==SavedPins && SavedPins[1]->PinName==SavedName && SavedPins[0]->DefaultValue==SavedDefault
        && !Graph->Nodes.Contains(Unknown) && NodeForgePrototype::FileSha256(File)==Before && !BP->GetPackage()->IsDirty();
    if(!Restored)AddError(TEXT("In-memory graph was not completely restored"));
    auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("status"),HasAnyErrors()?TEXT("failed"):TEXT("passed"));
    Report->SetStringField(TEXT("test"),TEXT("NodeForge.SequenceExportGuards"));Report->SetArrayField(TEXT("cases"),Rows);
    Report->SetStringField(TEXT("source"),Source);Report->SetStringField(TEXT("sourceSha256"),Before);Report->SetBoolField(TEXT("inMemoryGraphRestored"),Restored);
    Report->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());
    const FString ModuleFile=FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"));
    Report->SetStringField(TEXT("moduleFilename"),ModuleFile);Report->SetStringField(TEXT("moduleFileSha256"),NodeForgePrototype::FileSha256(ModuleFile));
    Report->SetStringField(TEXT("scope"),TEXT("Actual saved graph with temporary in-memory pin/order/unsupported-node corruption; source never saved or compiled during tampering; original restored; no UI proof"));
    return NodeForgePrototype::WriteJson(Report,ReportPath) && !HasAnyErrors();
}
#endif
