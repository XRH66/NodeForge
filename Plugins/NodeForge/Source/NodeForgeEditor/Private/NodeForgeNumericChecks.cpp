// Project-owned regression of compiled defaults. Reuses the existing Rotator guard test pattern.
#include "CoreMinimal.h"
#include "NodeForgeBlueprintGraph.h"
#include "NodeForgeGraphCodec.h"
#include "NodeForgePrototype.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
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
#include "UObject/NoExportTypes.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace
{
bool NumericGuardPath(FString& Path)
{
    Path = FPaths::ConvertRelativePathToFull(Path);
    FPaths::NormalizeFilename(Path);
    return FPaths::CollapseRelativeDirectories(Path);
}
TMap<FString, FString> NumericGuardAssets()
{
    TMap<FString, FString> Result;
    TArray<FString> Files;
    IFileManager::Get().FindFilesRecursive(Files, *FPaths::ProjectContentDir(), TEXT("*"), true, false);
    for (const FString& File : Files)
        if (File.EndsWith(TEXT(".uasset")) || File.EndsWith(TEXT(".umap")))
            Result.Add(File, NodeForgePrototype::FileSha256(File));
    return Result;
}
bool SameNumericAssets(const TMap<FString, FString>& A, const TMap<FString, FString>& B)
{
    if (A.Num() != B.Num()) return false;
    for (const auto& Pair : A)
        if (Pair.Value.Len() != 64 || !B.Contains(Pair.Key) || B[Pair.Key] != Pair.Value) return false;
    return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeNumericDefaultGuards, "NodeForge.NumericDefaultGuards",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNodeForgeNumericDefaultGuards::RunTest(const FString& Parameters)
{
    FString Input, Output, Project = FPaths::GetProjectFilePath();
    FString Reports = NodeForgePrototype::WorkspaceDirectory() / TEXT("Reports");
    FString Projects = NodeForgePrototype::WorkspaceDirectory() / TEXT("ValidationProject/RegressionRuns");
    if (!FParse::Value(FCommandLine::Get(), TEXT("NFInput="), Input)
        || !FParse::Value(FCommandLine::Get(), TEXT("NFReport="), Output)
        || !NumericGuardPath(Input) || !NumericGuardPath(Output) || !NumericGuardPath(Project)
        || !NumericGuardPath(Reports) || !NumericGuardPath(Projects)
        || !FPaths::IsUnderDirectory(Input, Reports) || !FPaths::IsUnderDirectory(Output, Reports)
        || !FPaths::IsUnderDirectory(Project, Projects) || !Input.EndsWith(TEXT(".json"))
        || !Output.EndsWith(TEXT(".json")) || !IFileManager::Get().FileExists(*Project)
        || !IFileManager::Get().FileExists(*Input) || IFileManager::Get().FileExists(*Output))
    { AddError(TEXT("Fresh Reports paths and isolated RegressionRuns project required")); return false; }
    const auto Assets = NumericGuardAssets();
    const FString InputSha = NodeForgePrototype::FileSha256(Input);
    const FString Module = FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"));
    const FString ModuleSha = NodeForgePrototype::FileSha256(Module);
    auto Report = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> Rows;
    Report->SetStringField(TEXT("test"), TEXT("NodeForge.NumericDefaultGuards"));
    Report->SetStringField(TEXT("project"), Project);
    Report->SetStringField(TEXT("inputSha256"), InputSha);
    Report->SetStringField(TEXT("moduleFilename"), Module);
    Report->SetStringField(TEXT("moduleFileSha256"), ModuleSha);
    Report->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
    Report->SetStringField(TEXT("scope"), TEXT("Actual native compiled double/vector CDO and K2 defaults; transient corruptions/restorations, no save, PIE or UI"));
    auto Finish = [&](bool Good, const FString& Reason)
    {
        const bool SafeAssets = SameNumericAssets(Assets, NumericGuardAssets());
        const bool SafeInput = InputSha.Len() == 64 && InputSha == NodeForgePrototype::FileSha256(Input);
        const bool SafeModule = ModuleSha.Len() == 64 && ModuleSha == NodeForgePrototype::FileSha256(Module);
        Good = Good && SafeAssets && SafeInput && SafeModule && !HasAnyErrors();
        Report->SetStringField(TEXT("status"), Good ? TEXT("passed") : TEXT("failed"));
        Report->SetStringField(TEXT("reason"), Reason);
        Report->SetArrayField(TEXT("cases"), Rows);
        Report->SetBoolField(TEXT("assetFilesUnchanged"), SafeAssets);
        Report->SetBoolField(TEXT("inputUnchanged"), SafeInput);
        Report->SetBoolField(TEXT("moduleUnchanged"), SafeModule);
        if (!Good) AddError(TEXT("Numeric guard failed: ") + Reason);
        return NodeForgePrototype::WriteJson(Report, Output) && Good;
    };
    FString Text, Error, ReadDigest;
    TSharedPtr<FJsonObject> Request;
    if (!NodeForgePrototype::ReadInputFile(Input,Text,ReadDigest,Error)
        || !ReadDigest.Equals(InputSha,ESearchCase::CaseSensitive)
        || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Request) || !Request)
        return Finish(false, TEXT("Invalid bounded input"));
    NodeForgeGraphDocument::FGraphDocument Document;
    if(!NodeForgeGraphCodec::DecodeText(Text,Document,Error))return Finish(false,TEXT("Fixture codec: ")+Error);
    UBlueprint* BP = NodeForgeBlueprintGraph::Build(GetTransientPackage(),
        MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(), TEXT("NodeForgeNumericGuard")).ToString(), Document, Error);
    if (!BP) return Finish(false, TEXT("Transient Build: ") + Error);
    BP->ClearFlags(RF_Public | RF_Standalone); BP->SetFlags(RF_Transient);
    ON_SCOPE_EXIT { BP->MarkAsGarbage(); };
    FCompilerResultsLog Log;
    FKismetEditorUtilities::CompileBlueprint(BP, EBlueprintCompileOptions::None, &Log);
    Report->SetNumberField(TEXT("compilerErrors"), Log.NumErrors);
    if (Log.NumErrors || !BP->GeneratedClass || BP->GeneratedClass->ClassGeneratedBy != BP
        || (BP->Status != BS_UpToDate && BP->Status != BS_UpToDateWithWarnings))
        return Finish(false, TEXT("Native compilation failed"));
    FDoubleProperty* Amount = FindFProperty<FDoubleProperty>(BP->GeneratedClass, TEXT("Amount"));
    FStructProperty* Offset = FindFProperty<FStructProperty>(BP->GeneratedClass, TEXT("Offset"));
    if (!Amount || !Offset || Offset->Struct != TBaseStructure<FVector>::Get())
        return Finish(false, TEXT("Exact native double/vector CDO required"));
    UObject* CDO = BP->GeneratedClass->GetDefaultObject();
    FVector* Vector = Offset->ContainerPtrToValuePtr<FVector>(CDO);
    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(BP);
    UEdGraphPin* ScalarPin = nullptr; UEdGraphPin* VectorPin = nullptr;
    if (Graph) for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (Node && Node->NodeComment.Equals(TEXT("NodeForge:SetAmount"), ESearchCase::CaseSensitive)) ScalarPin = Node->FindPin(TEXT("Amount"), EGPD_Input);
        if (Node && Node->NodeComment.Equals(TEXT("NodeForge:SetOffset"), ESearchCase::CaseSensitive)) VectorPin = Node->FindPin(TEXT("Offset"), EGPD_Input);
    }
    if (!Vector || !ScalarPin || !VectorPin || ScalarPin->LinkedTo.Num() || VectorPin->LinkedTo.Num())
        return Finish(false, TEXT("Unconnected scalar/vector defaults required"));
    const double SavedAmount = Amount->GetPropertyValue_InContainer(CDO);
    const FVector SavedVector = *Vector;
    const FString SavedScalarPin = ScalarPin->DefaultValue, SavedVectorPin = VectorPin->DefaultValue;
    if (SavedAmount != 999999900000.0 || SavedVector.X != SavedAmount || SavedVector.Y != -SavedAmount || SavedVector.Z != 123.125)
        return Finish(false, TEXT("Fixture must preserve exact baseline values"));
    ON_SCOPE_EXIT { Amount->SetPropertyValue_InContainer(CDO, SavedAmount); *Vector = SavedVector; ScalarPin->DefaultValue = SavedScalarPin; VectorPin->DefaultValue = SavedVectorPin; };
    auto Check = [&](const FString& Name, bool Expected, const FString& ExpectedError)
    {
        TSharedPtr<FJsonObject> Evidence; FString ReadError;
        const bool Actual = NodeForgeBlueprintGraph::Readback(BP, Document, Evidence, ReadError);
        const bool Good = Actual == Expected && (Expected ? Evidence.IsValid() : ReadError.Contains(ExpectedError));
        auto Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("case"), Name); Row->SetBoolField(TEXT("expectedReadback"), Expected);
        Row->SetBoolField(TEXT("actualReadback"), Actual); Row->SetStringField(TEXT("result"), Good ? TEXT("passed") : TEXT("failed"));
        Row->SetStringField(TEXT("error"), ReadError);
        Row->SetNumberField(TEXT("actualAmount"), Amount->GetPropertyValue_InContainer(CDO));
        Row->SetArrayField(TEXT("actualOffset"), {MakeShared<FJsonValueNumber>(Vector->X), MakeShared<FJsonValueNumber>(Vector->Y), MakeShared<FJsonValueNumber>(Vector->Z)});
        Row->SetStringField(TEXT("scalarPin"), ScalarPin->DefaultValue); Row->SetStringField(TEXT("vectorPin"), VectorPin->DefaultValue);
        Rows.Add(MakeShared<FJsonValueObject>(Row));
        if (!Good) AddError(TEXT("Numeric readback: ") + Name + TEXT(" / ") + ReadError);
    };
    Check(TEXT("Original"), true, FString());
    Amount->SetPropertyValue_InContainer(CDO, SavedAmount + 65536);
    Check(TEXT("CdoAmountPlus65536"), false, TEXT("Actual variable default differs: Amount"));
    Amount->SetPropertyValue_InContainer(CDO, SavedAmount); Check(TEXT("AmountRestored"), true, FString());
    Vector->X += 65536;
    Check(TEXT("CdoVectorXPlus65536"), false, TEXT("Actual variable default differs: Offset"));
    *Vector = SavedVector; Check(TEXT("VectorRestored"), true, FString());
    ScalarPin->DefaultValue = FString::Printf(TEXT("%.17g"), SavedAmount + 65536);
    Check(TEXT("PinAmountPlus65536"), false, TEXT("Actual default differs from JSON: Amount"));
    ScalarPin->DefaultValue = SavedScalarPin; Check(TEXT("ScalarPinRestored"), true, FString());
    VectorPin->DefaultValue = FString::Printf(TEXT("%.17g,%.17g,%.17g"), SavedVector.X + 65536, SavedVector.Y, SavedVector.Z);
    Check(TEXT("PinVectorXPlus65536"), false, TEXT("Actual default differs from JSON: Offset"));
    VectorPin->DefaultValue = SavedVectorPin; Check(TEXT("VectorPinRestored"), true, FString());
    const bool Restored = Amount->GetPropertyValue_InContainer(CDO) == SavedAmount && *Vector == SavedVector
        && ScalarPin->DefaultValue == SavedScalarPin && VectorPin->DefaultValue == SavedVectorPin;
    Report->SetBoolField(TEXT("inMemoryDefaultsRestored"), Restored);
    return Finish(Restored && Rows.Num() == 9 && BP->GetOutermost() == GetTransientPackage(), TEXT("Four actual corruptions and restoration checks completed"));
}
#endif
