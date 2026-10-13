// Project-owned native fixture preparation; only fresh isolated validation copies.
#include "CoreMinimal.h"
#include "NodeForgeBlueprintGraph.h"
#include "NodeForgePrototype.h"
#include "NodeForgeExportFixtureChecks.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Components/SceneComponent.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "GameFramework/Actor.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Knot.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/NoExportTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeBlueprintExportFixtures,"NodeForge.BlueprintExportFixtures",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNodeForgeBlueprintExportFixtures::RunTest(const FString& Parameters)
{
    FString Source,ReportPath,SetupError;
    const TArray<const TCHAR*> Names={TEXT("Untagged"),TEXT("Changed"),TEXT("Unknown"),TEXT("Subgraph"),TEXT("RootSetting"),TEXT("ActorSetting"),TEXT("VariableFlags"),TEXT("ColorDefault"),TEXT("Disabled"),TEXT("PinName"),TEXT("CompileMode"),TEXT("ConstructionLocal"),TEXT("VariableTarget")};
    FString Project=FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());FPaths::CollapseRelativeDirectories(Project);
    const FString Allowed=FPaths::ConvertRelativePathToFull(NodeForgePrototype::WorkspaceDirectory()/TEXT("ValidationProject/RegressionRuns"))+TEXT("/");
    if(!Project.StartsWith(Allowed,ESearchCase::IgnoreCase) || !FParse::Value(FCommandLine::Get(),TEXT("NFExportSource="),Source)
        || !FParse::Value(FCommandLine::Get(),TEXT("NFExportFixtureReport="),ReportPath) || !Source.StartsWith(TEXT("/Game/NodeForgeTests/"),ESearchCase::CaseSensitive))
    {AddError(TEXT("Only explicit own isolated copied source and fresh report allowed"));return false;}
    if(!NodeForgeExportFixtureChecks::ValidateSetup(ReportPath,Names,TEXT("NFBPExportNative"),SetupError))
    {AddError(SetupError);return false;}
    UBlueprint* Original=LoadObject<UBlueprint>(nullptr,*Source);
    if(!Original || !Original->GetPathName().Equals(Source,ESearchCase::CaseSensitive)
        || Original->GetPackage()->IsDirty()){AddError(TEXT("Clean canonical saved copied Actor blueprint required"));return false;}
    const FString File=FPackageName::LongPackageNameToFilename(Original->GetPackage()->GetName(),FPackageName::GetAssetPackageExtension());
    const FString Before=NodeForgePrototype::FileSha256(File);TArray<TSharedPtr<FJsonValue>> Cases;
    auto Fail=[&](const FString& Reason)
    {
        auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("status"),TEXT("failed"));
        Report->SetStringField(TEXT("reason"),Reason);Report->SetArrayField(TEXT("cases"),Cases);
        Report->SetBoolField(TEXT("originalSourceBytesUnchanged"),Before.Len()==64 && Before==NodeForgePrototype::FileSha256(File));
        if(!NodeForgePrototype::WriteJson(Report,ReportPath))AddError(TEXT("Partial fixture report could not be saved"));
        AddError(Reason);return false;
    };
    for(const TCHAR* Name:Names)
    {
        const FString Mode=Name,N=TEXT("NFBPExportNative")+Mode,P=TEXT("/Game/NodeForgeTests/")+N;
        if(FPackageName::DoesPackageExist(P) || FindPackage(nullptr,*P))return Fail(TEXT("Fixture target collision: ")+P);
        UPackage* Package=CreatePackage(*P);UBlueprint* BP=DuplicateObject<UBlueprint>(Original,Package,*N);
        auto* Graph=FBlueprintEditorUtils::FindEventGraph(BP);UK2Node_CallFunction* Print=nullptr;
        for(UEdGraphNode* Node:Graph->Nodes)
        {
            Node->NodeComment=TEXT("Native untagged source graph");
            if(auto* Call=Cast<UK2Node_CallFunction>(Node))if(Call->GetTargetFunction() && Call->GetTargetFunction()->GetName()==TEXT("PrintString"))Print=Call;
        }
        if(!Print)return Fail(TEXT("Fixture requires native PrintString"));
        if(Mode==TEXT("Changed") || Mode==TEXT("VariableFlags") || Mode==TEXT("VariableTarget"))
        {
            struct FVariable {const TCHAR* Name;FName Category;const TCHAR* Value;};
            for(const FVariable V:{FVariable{TEXT("ExportBool"),UEdGraphSchema_K2::PC_Boolean,TEXT("false")},
                FVariable{TEXT("ExportNumber"),UEdGraphSchema_K2::PC_Real,TEXT("0")},
                FVariable{TEXT("ExportText"),UEdGraphSchema_K2::PC_String,TEXT("Native saved default")},
                FVariable{TEXT("ExportVector"),UEdGraphSchema_K2::PC_Struct,TEXT("(X=1,Y=2,Z=3)")}})
            {
                FEdGraphPinType Type;Type.PinCategory=V.Category;
                if(V.Category==UEdGraphSchema_K2::PC_Real)Type.PinSubCategory=UEdGraphSchema_K2::PC_Double;
                if(V.Category==UEdGraphSchema_K2::PC_Struct)Type.PinSubCategoryObject=TBaseStructure<FVector>::Get();
                if(!FBlueprintEditorUtils::AddMemberVariable(BP,FName(V.Name),Type,V.Value))return Fail(TEXT("Native fixture variable creation failed"));
            }
            GetDefault<UEdGraphSchema_K2>()->TrySetDefaultValue(*Print->FindPin(TEXT("InString")),TEXT("Exported native changed graph"));
            Print->NodePosY+=140;
        }
        FBlueprintEditorUtils::MarkBlueprintAsModified(BP);FKismetEditorUtilities::CompileBlueprint(BP);
        if(BP->Status!=BS_UpToDate && BP->Status!=BS_UpToDateWithWarnings)return Fail(TEXT("Native fixture base compilation failed"));
        Graph=FBlueprintEditorUtils::FindEventGraph(BP);Print=nullptr;
        for(UEdGraphNode* Node:Graph->Nodes)if(auto* C=Cast<UK2Node_CallFunction>(Node))if(C->GetTargetFunction() && C->GetTargetFunction()->GetName()==TEXT("PrintString"))Print=C;
        if(!Print || !Print->FindPin(TEXT("InString")) || !Print->FindPin(TEXT("TextColor")))
        return Fail(TEXT("Actual compiled fixture PrintString pins differ; do not use guessed names"));
        // Unsupported fixtures intentionally alter native saved source structure/configuration;
        // no claim their altered graph has separately compiled or run.
        if(Mode==TEXT("Unknown"))
        // Sequence became supported in its own adapter; keep this guard genuinely unsupported.
        {auto* Node=NewObject<UK2Node_Knot>(Graph);Graph->AddNode(Node,false,false);Node->CreateNewGuid();Node->AllocateDefaultPins();}
        if(Mode==TEXT("Subgraph"))Graph->SubGraphs.Add(FBlueprintEditorUtils::CreateNewGraph(Graph,TEXT("Extra"),UEdGraph::StaticClass(),UEdGraphSchema_K2::StaticClass()));
        if(Mode==TEXT("RootSetting"))CastChecked<USceneComponent>(BP->SimpleConstructionScript->GetDefaultSceneRootNode()->ComponentTemplate)->SetRelativeScale3D(FVector(2));
        if(Mode==TEXT("ActorSetting"))CastChecked<AActor>(BP->GeneratedClass->GetDefaultObject())->SetCanBeDamaged(false);
        if(Mode==TEXT("VariableFlags"))BP->NewVariables[0].PropertyFlags|=CPF_BlueprintReadOnly;
        if(Mode==TEXT("ColorDefault"))GetDefault<UEdGraphSchema_K2>()->TrySetDefaultValue(*Print->FindPin(TEXT("TextColor")),TEXT("(R=1,G=0,B=0,A=1)"));
        if(Mode==TEXT("Disabled"))Print->SetEnabledState(ENodeEnabledState::Disabled);
        if(Mode==TEXT("PinName"))Print->FindPin(TEXT("InString"))->PinName=TEXT("BogusPin");
        if(Mode==TEXT("CompileMode"))BP->CompileMode=EBlueprintCompileMode::Development;
        if(Mode==TEXT("VariableTarget"))
        {
            auto* Node=NewObject<UK2Node_VariableGet>(Graph);Node->VariableReference.SetSelfMember(TEXT("ExportBool"));
            Graph->AddNode(Node,false,false);Node->CreateNewGuid();Node->AllocateDefaultPins();
            auto* Self=Node->FindPin(UEdGraphSchema_K2::PN_Self,EGPD_Input);
            if(!Self)return Fail(TEXT("Native self-variable pin missing"));
            Self->DefaultObject=AActor::StaticClass()->GetDefaultObject();
        }
        if(Mode==TEXT("ConstructionLocal"))
        {
            auto* CS=FBlueprintEditorUtils::FindUserConstructionScript(BP);
            auto* Entry=CS && !CS->Nodes.IsEmpty()?Cast<UK2Node_FunctionEntry>(CS->Nodes[0]):nullptr;
            if(!Entry)return Fail(TEXT("Native construction entry unavailable"));
            FBPVariableDescription V;V.VarName=TEXT("HiddenLocal");V.VarType.PinCategory=UEdGraphSchema_K2::PC_Boolean;Entry->LocalVariables.Add(V);
        }
        BP->SetFlags(RF_Public|RF_Standalone);FSavePackageArgs Save;Save.TopLevelFlags=RF_Public|RF_Standalone;Save.SaveFlags=SAVE_NoError;
        const FString Destination=FPackageName::LongPackageNameToFilename(P,FPackageName::GetAssetPackageExtension());
        if(!UPackage::SavePackage(Package,BP,*Destination,Save))return Fail(TEXT("Native fixture save failed: ")+P);
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("case"),Name);Row->SetStringField(TEXT("source"),BP->GetPathName());
        Row->SetStringField(TEXT("sourceAssetSha256"),NodeForgePrototype::FileSha256(Destination));Row->SetStringField(TEXT("expected"),Mode==TEXT("Untagged") || Mode==TEXT("Changed")?TEXT("passed"):TEXT("rejected"));
        Cases.Add(MakeShared<FJsonValueObject>(Row));
    }
    auto Config=MakeShared<FJsonObject>();Config->SetStringField(TEXT("source"),Source);Config->SetStringField(TEXT("requestId"),TEXT("BPExportDirty"));
    Config->SetStringField(TEXT("assetPath"),TEXT("/Game/NodeForgeTests/NFBPExportDirtyTarget"));
    Original->GetPackage()->SetDirtyFlag(true);TSharedPtr<FJsonObject> Draft,Evidence;FString Error;
    const bool Dirty=!NodeForgeBlueprintGraph::Export(Config,TEXT("notNeededForDirtyReject"),Draft,Evidence,Error) && !Draft && !Evidence && !Error.IsEmpty();
    const bool DirtyPreserved=Original->GetPackage()->IsDirty();
    Original->GetPackage()->SetDirtyFlag(false);const bool Unchanged=Before.Len()==64 && Before==NodeForgePrototype::FileSha256(File);
    auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("status"),Dirty && DirtyPreserved && Unchanged?TEXT("passed"):TEXT("failed"));Report->SetArrayField(TEXT("cases"),Cases);
    Report->SetBoolField(TEXT("dirtySourceRejected"),Dirty);Report->SetBoolField(TEXT("originalSourceBytesUnchanged"),Unchanged);
    Report->SetBoolField(TEXT("dirtySourceFlagPreservedDuringExport"),DirtyPreserved);
    Report->SetStringField(TEXT("scope"),TEXT("Native saved isolated blueprint fixture setup; positive untagged graphs compiled; altered unsupported negative configurations not claimed compiled; readonly export tested in fresh processes"));
    if(!NodeForgePrototype::WriteJson(Report,ReportPath)){AddError(TEXT("Fresh report write failed"));return false;}
    if(!Dirty || !DirtyPreserved || !Unchanged)AddError(TEXT("Dirty/source guard failed"));return !HasAnyErrors();
}
#endif
