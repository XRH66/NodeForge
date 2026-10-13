// Project-owned checks of real compiled CDO and K2 pin defaults; no engine implementation copied.
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
#include "Misc/DefaultValueHelper.h"
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
bool NormalizeRotatorGuardPath(FString& Path)
{
    Path = FPaths::ConvertRelativePathToFull(Path);
    FPaths::NormalizeFilename(Path);
    return FPaths::CollapseRelativeDirectories(Path);
}

TMap<FString, FString> RotatorGuardAssetSnapshot()
{
    TMap<FString, FString> Result;
    TArray<FString> Files;
    IFileManager::Get().FindFilesRecursive(Files, *FPaths::ProjectContentDir(), TEXT("*"), true, false);
    for (const FString& File : Files)
        if (File.EndsWith(TEXT(".uasset")) || File.EndsWith(TEXT(".umap")))
            Result.Add(File, NodeForgePrototype::FileSha256(File));
    return Result;
}

bool RotatorGuardSameAssets(const TMap<FString, FString>& Before, const TMap<FString, FString>& After)
{
    if (Before.Num() != After.Num()) return false;
    for (const auto& Pair : Before)
    {
        const FString* Found = After.Find(Pair.Key);
        if (!Found || Pair.Value.Len() != 64 || *Found != Pair.Value) return false;
    }
    return true;
}

TSharedRef<FJsonObject> RotatorGuardComponents(const FRotator& Value)
{
    auto Result = MakeShared<FJsonObject>();
    Result->SetNumberField(TEXT("pitch"), Value.Pitch);
    Result->SetNumberField(TEXT("yaw"), Value.Yaw);
    Result->SetNumberField(TEXT("roll"), Value.Roll);
    return Result;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeRotatorDefaultGuards, "NodeForge.RotatorDefaultGuards",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNodeForgeRotatorDefaultGuards::RunTest(const FString& Parameters)
{
    FString InputPath, ReportPath;
    FString Project = FPaths::GetProjectFilePath();
    FString ReportsRoot = NodeForgePrototype::WorkspaceDirectory() / TEXT("Reports");
    FString ProjectsRoot = NodeForgePrototype::WorkspaceDirectory() / TEXT("ValidationProject/RegressionRuns");
    if (!FParse::Value(FCommandLine::Get(), TEXT("NFInput="), InputPath)
        || !FParse::Value(FCommandLine::Get(), TEXT("NFReport="), ReportPath)
        || !NormalizeRotatorGuardPath(Project) || !NormalizeRotatorGuardPath(ReportsRoot)
        || !NormalizeRotatorGuardPath(ProjectsRoot) || !NormalizeRotatorGuardPath(InputPath)
        || !NormalizeRotatorGuardPath(ReportPath)
        || !FPaths::IsUnderDirectory(Project, ProjectsRoot)
        || !FPaths::IsUnderDirectory(InputPath, ReportsRoot) || !FPaths::IsUnderDirectory(ReportPath, ReportsRoot)
        || !InputPath.EndsWith(TEXT(".json")) || !ReportPath.EndsWith(TEXT(".json"))
        || !IFileManager::Get().FileExists(*Project) || !IFileManager::Get().FileExists(*InputPath)
        || IFileManager::Get().FileExists(*ReportPath))
    { AddError(TEXT("Explicit Reports input, fresh Reports output and isolated RegressionRuns project required")); return false; }

    const auto AssetsBefore = RotatorGuardAssetSnapshot();
    const FString InputSha = NodeForgePrototype::FileSha256(InputPath);
    const FString ModuleFile = FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"));
    const FString ModuleSha = NodeForgePrototype::FileSha256(ModuleFile);
    auto Report = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> Rows;
    Report->SetStringField(TEXT("test"), TEXT("NodeForge.RotatorDefaultGuards"));
    Report->SetStringField(TEXT("project"), Project);
    Report->SetStringField(TEXT("input"), InputPath);
    Report->SetStringField(TEXT("inputSha256"), InputSha);
    Report->SetStringField(TEXT("moduleFilename"), ModuleFile);
    Report->SetStringField(TEXT("moduleFileSha256"), ModuleSha);
    Report->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
    Report->SetStringField(TEXT("scope"), TEXT("Transient Blueprint built from explicit JSON and compiled natively; actual CDO and K2 pin components temporarily changed by -360 one axis at a time; no asset save, PIE, UI or packaging"));
    auto Finish = [&](bool Good, const FString& Reason)
    {
        const bool AssetsUnchanged = RotatorGuardSameAssets(AssetsBefore, RotatorGuardAssetSnapshot());
        const bool InputUnchanged = InputSha.Len() == 64 && NodeForgePrototype::FileSha256(InputPath) == InputSha;
        const bool ModuleUnchanged = ModuleSha.Len() == 64 && NodeForgePrototype::FileSha256(ModuleFile) == ModuleSha;
        Good = Good && AssetsUnchanged && InputUnchanged && ModuleUnchanged && !HasAnyErrors();
        Report->SetStringField(TEXT("status"), Good ? TEXT("passed") : TEXT("failed"));
        Report->SetStringField(TEXT("reason"), Reason);
        Report->SetArrayField(TEXT("cases"), Rows);
        Report->SetBoolField(TEXT("assetFilesUnchanged"), AssetsUnchanged);
        Report->SetBoolField(TEXT("inputUnchanged"), InputUnchanged);
        Report->SetBoolField(TEXT("moduleUnchanged"), ModuleUnchanged);
        if (!Good) AddError(TEXT("Rotator default guard failed: ") + Reason);
        const bool Written = NodeForgePrototype::WriteJson(Report, ReportPath);
        if (!Written) AddError(TEXT("Unable to write fresh rotator guard report"));
        return Good && Written;
    };

    FString Text, Error, ReadDigest;
    TSharedPtr<FJsonObject> Request;
    if (!NodeForgePrototype::ReadInputFile(InputPath, Text, ReadDigest, Error)
        || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Request) || !Request)
        return Finish(false, TEXT("Invalid bounded input JSON"));
    NodeForgeGraphDocument::FGraphDocument Document;
    if(!NodeForgeGraphCodec::DecodeText(Text,Document,Error))return Finish(false,TEXT("Fixture codec: ")+Error);
    UBlueprint* BP = NodeForgeBlueprintGraph::Build(GetTransientPackage(),
        MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(), TEXT("NodeForgeRotatorGuard")).ToString(), Document, Error);
    if (!BP) return Finish(false, TEXT("Transient build failed: ") + Error);
    BP->ClearFlags(RF_Public | RF_Standalone);
    BP->SetFlags(RF_Transient);
    ON_SCOPE_EXIT { BP->MarkAsGarbage(); };
    FCompilerResultsLog CompilerLog;
    FKismetEditorUtilities::CompileBlueprint(BP, EBlueprintCompileOptions::None, &CompilerLog);
    Report->SetNumberField(TEXT("compilerErrors"), CompilerLog.NumErrors);
    Report->SetNumberField(TEXT("compilerWarnings"), CompilerLog.NumWarnings);
    Report->SetNumberField(TEXT("blueprintStatus"), static_cast<int32>(BP->Status));
    if (CompilerLog.NumErrors || !BP->GeneratedClass || BP->GeneratedClass->ClassGeneratedBy != BP
        || (BP->Status != BS_UpToDate && BP->Status != BS_UpToDateWithWarnings))
        return Finish(false, TEXT("Native compilation failed"));

    FStructProperty* Property = FindFProperty<FStructProperty>(BP->GeneratedClass, TEXT("Facing"));
    if (!Property || Property->Struct != TBaseStructure<FRotator>::Get())
        return Finish(false, TEXT("Compiled Facing must be an exact native Rotator property"));
    FRotator* CdoValue = Property->ContainerPtrToValuePtr<FRotator>(BP->GeneratedClass->GetDefaultObject());
    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(BP);
    UEdGraphPin* RotationPin = nullptr;
    if (Graph) for (UEdGraphNode* Node : Graph->Nodes)
        if (Node && Node->NodeComment == TEXT("NodeForge:Rotate"))
            RotationPin = Node->FindPin(TEXT("NewRotation"), EGPD_Input);
    if (!CdoValue || !RotationPin || !RotationPin->LinkedTo.IsEmpty()
        || RotationPin->PinType.PinSubCategoryObject != TBaseStructure<FRotator>::Get())
        return Finish(false, TEXT("Exact unconnected native Rotate.NewRotation pin required"));
    const FRotator SavedCdo = *CdoValue;
    const FString SavedPinText = RotationPin->DefaultValue;
    FRotator SavedPin;
    if (!FDefaultValueHelper::ParseRotator(SavedPinText, SavedPin)
        || SavedCdo.Pitch != 1.e12 || SavedCdo.Yaw != -1.e12 || SavedCdo.Roll != 123.125
        || SavedPin.Pitch != SavedCdo.Pitch || SavedPin.Yaw != SavedCdo.Yaw || SavedPin.Roll != SavedCdo.Roll)
        return Finish(false, TEXT("Input must supply both Facing and NewRotation as pitch=1e12,yaw=-1e12,roll=123.125"));
    ON_SCOPE_EXIT { *CdoValue = SavedCdo; RotationPin->DefaultValue = SavedPinText; };
    Report->SetObjectField(TEXT("compiledCdoBaseline"), RotatorGuardComponents(SavedCdo));
    Report->SetStringField(TEXT("nativePinBaseline"), SavedPinText);

    auto Check = [&](const FString& Name, bool Expected, const FString& Reason)
    {
        TSharedPtr<FJsonObject> Evidence;
        FString ReadError;
        const bool Actual = NodeForgeBlueprintGraph::Readback(BP, Document, Evidence, ReadError);
        const bool Good = Actual == Expected && (Expected ? Evidence.IsValid() : ReadError.Contains(Reason));
        auto Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("case"), Name);
        Row->SetBoolField(TEXT("expectedReadback"), Expected);
        Row->SetBoolField(TEXT("actualReadback"), Actual);
        Row->SetStringField(TEXT("result"), Good ? TEXT("passed") : TEXT("failed"));
        Row->SetStringField(TEXT("error"), ReadError);
        Row->SetObjectField(TEXT("actualCdo"), RotatorGuardComponents(*CdoValue));
        Row->SetStringField(TEXT("actualPinText"), RotationPin->DefaultValue);
        Rows.Add(MakeShared<FJsonValueObject>(Row));
        if (!Good) AddError(TEXT("Rotator readback guard failed: ") + Name + TEXT(" / ") + ReadError);
    };
    Check(TEXT("OriginalCompiled"), true, FString());
    for (const TCHAR* Axis : { TEXT("Pitch"), TEXT("Yaw"), TEXT("Roll") })
    {
        if (FString(Axis) == TEXT("Pitch")) CdoValue->Pitch -= 360;
        else if (FString(Axis) == TEXT("Yaw")) CdoValue->Yaw -= 360;
        else CdoValue->Roll -= 360;
        Check(FString(TEXT("Cdo")) + Axis + TEXT("Minus360"), false, TEXT("Actual variable default differs: Facing"));
        *CdoValue = SavedCdo;
        Check(FString(TEXT("Cdo")) + Axis + TEXT("Restored"), true, FString());
    }
    for (const TCHAR* Axis : { TEXT("Pitch"), TEXT("Yaw"), TEXT("Roll") })
    {
        FRotator Changed = SavedPin;
        if (FString(Axis) == TEXT("Pitch")) Changed.Pitch -= 360;
        else if (FString(Axis) == TEXT("Yaw")) Changed.Yaw -= 360;
        else Changed.Roll -= 360;
        // Alter the real native pin, without running schema normalization or recompiling away the fault.
        RotationPin->DefaultValue = FString::Printf(TEXT("%.17g,%.17g,%.17g"), Changed.Pitch, Changed.Yaw, Changed.Roll);
        Check(FString(TEXT("Pin")) + Axis + TEXT("Minus360"), false, TEXT("Actual default differs from JSON: NewRotation"));
        RotationPin->DefaultValue = SavedPinText;
        Check(FString(TEXT("Pin")) + Axis + TEXT("Restored"), true, FString());
    }
    Check(TEXT("FinalRestored"), true, FString());
    const bool Restored = CdoValue->Pitch == SavedCdo.Pitch && CdoValue->Yaw == SavedCdo.Yaw
        && CdoValue->Roll == SavedCdo.Roll && RotationPin->DefaultValue == SavedPinText;
    Report->SetBoolField(TEXT("inMemoryDefaultsRestored"), Restored);
    Report->SetBoolField(TEXT("transientBlueprint"), BP->GetOutermost() == GetTransientPackage() && BP->HasAnyFlags(RF_Transient));
    return Finish(Restored && Rows.Num() == 14 && BP->GetOutermost() == GetTransientPackage(),
        TEXT("Six native component corruptions and restoration readbacks completed; no asset save"));
}
#endif
