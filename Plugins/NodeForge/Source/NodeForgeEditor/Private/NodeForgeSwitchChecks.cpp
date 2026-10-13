// Project-owned transient negative checks of the actual native Switch instance.
#include "CoreMinimal.h"
#include "NodeForgeBlueprintGraph.h"
#include "NodeForgeGraphCodec.h"
#include "NodeForgeBlueprintSwitches.h"
#include "NodeForgePrototype.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "K2Node_SwitchName.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeSwitchConfigurationGuards, "NodeForge.SwitchConfigurationGuards",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNodeForgeSwitchConfigurationGuards::RunTest(const FString& Parameters)
{
    FString Input, Output;
    auto Normalize = [](FString& Path) { Path = FPaths::ConvertRelativePathToFull(Path); FPaths::NormalizeFilename(Path); return FPaths::CollapseRelativeDirectories(Path); };
    FString Project = FPaths::GetProjectFilePath(), Reports = NodeForgePrototype::WorkspaceDirectory() / TEXT("Reports"),
        Projects = NodeForgePrototype::WorkspaceDirectory() / TEXT("ValidationProject/RegressionRuns");
    if (!FParse::Value(FCommandLine::Get(), TEXT("NFInput="), Input) || !FParse::Value(FCommandLine::Get(), TEXT("NFReport="), Output)
        || !Normalize(Input) || !Normalize(Output) || !Normalize(Project) || !Normalize(Reports) || !Normalize(Projects)
        || !FPaths::IsUnderDirectory(Input, Reports) || !FPaths::IsUnderDirectory(Output, Reports) || !FPaths::IsUnderDirectory(Project, Projects)
        || !Input.EndsWith(TEXT(".json")) || !Output.EndsWith(TEXT(".json")) || !IFileManager::Get().FileExists(*Input)
        || IFileManager::Get().FileExists(*Output))
    { AddError(TEXT("Explicit bounded Reports input, fresh Reports output and isolated project required")); return false; }
    auto Snapshot = []()
    {
        TMap<FString, FString> Result; TArray<FString> Files;
        IFileManager::Get().FindFilesRecursive(Files, *FPaths::ProjectContentDir(), TEXT("*"), true, false);
        for (const auto& File : Files) if (File.EndsWith(TEXT(".uasset")) || File.EndsWith(TEXT(".umap"))) Result.Add(File, NodeForgePrototype::FileSha256(File));
        return Result;
    };
    const auto Before = Snapshot();
    const FString InputHash = NodeForgePrototype::FileSha256(Input), Module = FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor")),
        ModuleHash = NodeForgePrototype::FileSha256(Module);
    auto Report = MakeShared<FJsonObject>(); TArray<TSharedPtr<FJsonValue>> Rows;
    bool Good = true;
    auto Row = [&](const FString& Name, bool Accepted, bool Expected, const FString& Error)
    {
        auto Item = MakeShared<FJsonObject>(); Item->SetStringField(TEXT("case"), Name); Item->SetBoolField(TEXT("accepted"), Accepted);
        Item->SetBoolField(TEXT("expectedAccepted"), Expected); Item->SetBoolField(TEXT("passed"), Accepted == Expected); Item->SetStringField(TEXT("reason"), Error);
        Rows.Add(MakeShared<FJsonValueObject>(Item)); Good &= TestEqual(Name, Accepted, Expected);
    };
    auto Finish=[&](bool Passed,const FString& Reason)
    {
        const auto After=Snapshot();bool AssetsSame=Before.Num()==After.Num();
        for(const auto& Pair:Before){const FString* Found=After.Find(Pair.Key);AssetsSame &= Found && Pair.Value.Len()==64 && Pair.Value.Equals(*Found,ESearchCase::CaseSensitive);}
        const bool FilesSame=AssetsSame && InputHash.Len()==64 && ModuleHash.Len()==64
            && InputHash.Equals(NodeForgePrototype::FileSha256(Input),ESearchCase::CaseSensitive)
            && ModuleHash.Equals(NodeForgePrototype::FileSha256(Module),ESearchCase::CaseSensitive);
        Passed=Passed && Good && FilesSame && !HasAnyErrors();
        Report->SetStringField(TEXT("test"),TEXT("NodeForge.SwitchConfigurationGuards"));
        Report->SetStringField(TEXT("status"),Passed?TEXT("passed"):TEXT("failed"));Report->SetStringField(TEXT("reason"),Reason);
        Report->SetArrayField(TEXT("cases"),Rows);Report->SetStringField(TEXT("moduleFilename"),Module);Report->SetStringField(TEXT("moduleFileSha256"),ModuleHash);
        Report->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());
        Report->SetBoolField(TEXT("assetFilesUnchanged"),AssetsSame);Report->SetBoolField(TEXT("inputAndModuleUnchanged"),FilesSame);
        Report->SetStringField(TEXT("scope"),TEXT("Explicit fixture validation, transient compiled name-Switch and uncompiled integer Switch mutation/restoration; no save, runtime, UI or packaging"));
        if(!Passed)AddError(TEXT("Switch configuration guard failed: ")+Reason);
        return NodeForgePrototype::WriteJson(Report,Output) && Passed;
    };
    FString Text, Error, ReadDigest; TSharedPtr<FJsonObject> Request;
    if (!NodeForgePrototype::ReadInputFile(Input,Text,ReadDigest,Error) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Request) || !Request)
        return Finish(false,TEXT("Cannot read bounded own input JSON"));
    NodeForgeGraphDocument::FGraphDocument Document;
    if(!NodeForgeGraphCodec::DecodeText(Text,Document,Error))return Finish(false,TEXT("Fixture codec: ")+Error);
    if(!NodeForgeBlueprintGraph::Validate(Document,Error))return Finish(false,TEXT("Invalid Switch fixture: ")+Error);
    UBlueprint* BP = NodeForgeBlueprintGraph::Build(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(), TEXT("NodeForgeSwitchGuard")).ToString(), Document, Error);
    if (!BP) return Finish(false,Error);
    BP->ClearFlags(RF_Public | RF_Standalone); BP->SetFlags(RF_Transient);
    ON_SCOPE_EXIT{BP->MarkAsGarbage();};
    FCompilerResultsLog Compiler; FKismetEditorUtilities::CompileBlueprint(BP, EBlueprintCompileOptions::None, &Compiler);
    Good &= TestEqual(TEXT("Compiled transient errors"), Compiler.NumErrors, 0);
    Report->SetNumberField(TEXT("compilerErrors"),Compiler.NumErrors);Report->SetNumberField(TEXT("compilerWarnings"),Compiler.NumWarnings);
    if(Compiler.NumErrors || !BP->GeneratedClass)return Finish(false,TEXT("Native Switch fixture compilation failed"));
    auto* Graph = FBlueprintEditorUtils::FindEventGraph(BP);
    UK2Node_SwitchName* Select = nullptr; TSharedPtr<FJsonObject> Props;
    for (const auto& Value : Request->GetArrayField(TEXT("nodes")))
        if (Value->AsObject()->GetStringField(TEXT("id")) == TEXT("Select")) Props = Value->AsObject()->GetObjectField(TEXT("properties"));
    if (Graph) for (UEdGraphNode* Node : Graph->Nodes) if (Node && Node->NodeComment == TEXT("NodeForge:Select")) Select = Cast<UK2Node_SwitchName>(Node);
    if (!Select || !Props || Select->PinNames.IsEmpty())return Finish(false,TEXT("Expected own named Switch Select"));
    auto Verify = [&](const FString& Name, bool Expected)
    {
        FString Reason; const bool Accepted = NodeForgeBlueprintSwitches::Verify(Select, Props->Values, Reason); Row(Name, Accepted, Expected, Reason);
    };
    Verify(TEXT("baseline"), true);
    const FName OriginalName = Select->PinNames[0];
    Select->PinNames[0] = FName(*OriginalName.ToString().ToLower());
    const bool CaseDisplayChanged = !Select->PinNames[0].ToString().Equals(OriginalName.ToString(), ESearchCase::CaseSensitive);
    Report->SetBoolField(TEXT("withCasePreservingName"), WITH_CASE_PRESERVING_NAME != 0);
    Report->SetBoolField(TEXT("caseOnlyMutationConstructible"), CaseDisplayChanged);
    Row(TEXT("casePreservingMutationReallyChangedDisplay"), CaseDisplayChanged, true, TEXT(""));
    Verify(TEXT("changedPropertyDisplayOnly"), false);
    Select->PinNames[0] = OriginalName; Verify(TEXT("restoredPropertyDisplay"), true);
    // Independently reject a case-only mismatch in the requested exact signature.
    auto LowerProps = MakeShared<FJsonObject>(); LowerProps->Values = Props->Values;
    TArray<TSharedPtr<FJsonValue>> LowerNames = Props->GetArrayField(TEXT("cases"));
    LowerNames[0] = MakeShared<FJsonValueString>(OriginalName.ToString().ToLower()); LowerProps->SetArrayField(TEXT("cases"), LowerNames);
    FString SpellingError;
    Row(TEXT("differentRequestedDisplaySpelling"), NodeForgeBlueprintSwitches::Verify(Select, LowerProps->Values, SpellingError), false, SpellingError);
    Select->PinNames[0] = FName(*(OriginalName.ToString() + TEXT("Changed")));
    Row(TEXT("propertyMutationReallyChangedDisplay"), !Select->PinNames[0].ToString().Equals(OriginalName.ToString(), ESearchCase::CaseSensitive), true, TEXT(""));
    Verify(TEXT("changedPropertyName"), false);
    Select->PinNames[0] = OriginalName; Verify(TEXT("restoredPropertyName"), true);
    UEdGraphPin* Case = Select->FindPin(OriginalName, EGPD_Output);
    if (!Case)return Finish(false,TEXT("Actual case pin absent"));
    Case->bAdvancedView = true; Verify(TEXT("advancedCasePin"), false); Case->bAdvancedView = false; Verify(TEXT("restoredAdvancedPin"), true);
    const FString AutoDefault = Case->AutogeneratedDefaultValue;
    Case->AutogeneratedDefaultValue = TEXT("unexpected"); Verify(TEXT("autogeneratedExecDefault"), false);
    Case->AutogeneratedDefaultValue = AutoDefault; Verify(TEXT("restoredExecDefault"), true);
    UEdGraphPin* Function = Select->GetFunctionPin();
    if(!Function)return Finish(false,TEXT("Actual hidden function pin absent"));
    UObject* Context = Function->DefaultObject;
    Function->DefaultObject = nullptr; Verify(TEXT("missingHiddenFunctionContext"), false);
    Function->DefaultObject = Context; Verify(TEXT("restoredFunctionContext"), true);
    const auto Pins = Select->Pins;
    Select->Pins.Reset();
    const auto BrokenProps = NodeForgeBlueprintSwitches::Properties(Select);
    FString BrokenError; Row(TEXT("emptySourcePinsRejectedWithoutCrash"), NodeForgeBlueprintSwitches::Verify(Select, BrokenProps->Values, BrokenError), false, BrokenError);
    Select->Pins = Pins; Verify(TEXT("restoredPins"), true);
    NodeForgeBlueprintGraph::FObject Evidence;
    FString ReadError; Row(TEXT("wholeGraphReadbackAfterRestore"), NodeForgeBlueprintGraph::Readback(BP, Document, Evidence, ReadError), true, ReadError);
    // Integer Properties counts native pins; exercise its separate empty-pin path.
    auto IntegerProps = MakeShared<FJsonObject>();
    IntegerProps->SetNumberField(TEXT("startIndex"), 0); IntegerProps->SetNumberField(TEXT("caseCount"), 2);
    IntegerProps->SetBoolField(TEXT("hasDefault"), true);
    FString IntegerError;
    UEdGraphNode* Integer = NodeForgeBlueprintSwitches::Spawn(Graph, BP, TEXT("/Script/BlueprintGraph.K2Node_SwitchInteger"), IntegerProps->Values, FVector2D(0, 0), IntegerError);
    Row(TEXT("transientIntegerSpawn"), Integer != nullptr, true, IntegerError);
    if (Integer)
    {
        const auto IntegerPins = Integer->Pins; Integer->Pins.Reset();
        const auto EmptyProperties = NodeForgeBlueprintSwitches::Properties(Integer);
        FString Reason;
        Row(TEXT("emptyIntegerPinsRejectedWithoutCrash"), NodeForgeBlueprintSwitches::Verify(Integer, EmptyProperties->Values, Reason), false, Reason);
        Integer->Pins = IntegerPins; Reason.Reset();
        Row(TEXT("restoredIntegerPins"), NodeForgeBlueprintSwitches::Verify(Integer, IntegerProps->Values, Reason), true, Reason);
    }
    return Finish(Good,TEXT("All native Switch mutations rejected and restored"));
}
#endif
