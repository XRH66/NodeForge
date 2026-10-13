// Project-owned regression of actual visual failure cleanup in one isolated UE process.
#include "CoreMinimal.h"
#include "NodeForgeGraphChecks.h"
#include "NodeForgePrototype.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Materials/MaterialInterface.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UObjectIterator.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeVisualCleanupGuards,"NodeForge.VisualCleanupGuards",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNodeForgeVisualCleanupGuards::RunTest(const FString& Parameters)
{
    FString Input,Output,Text,Digest,Error;const FString Root=NodeForgePrototype::WorkspaceDirectory();
    if(!FParse::Value(FCommandLine::Get(),TEXT("NFInput="),Input)
        || !FParse::Value(FCommandLine::Get(),TEXT("NFReport="),Output)
        || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Input),Root/TEXT("Reports"))
        || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Output),Root/TEXT("Reports"))
        || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()),Root/TEXT("ValidationProject/RegressionRuns"))
        || IFileManager::Get().FileExists(*Output)
        || !NodeForgePrototype::ReadInputFile(Input,Text,Digest,Error))
    {AddError(TEXT("Explicit fresh Reports evidence and isolated visual project required"));return false;}
    TSharedPtr<FJsonObject> Config;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Config) || !Config)
    {AddError(TEXT("Explicit visual config required"));return false;}
    FString Map,Material;
    if(!Config->TryGetStringField(TEXT("map"),Map) || !Config->TryGetStringField(TEXT("material"),Material)
        || !Map.StartsWith(TEXT("/Game/NodeForgeTests/"),ESearchCase::CaseSensitive)
        || !Material.StartsWith(TEXT("/Game/NodeForgeTests/"),ESearchCase::CaseSensitive))
    {AddError(TEXT("Saved own map/material required"));return false;}
    auto Assets=[]()
    {
        TMap<FString,FString> Result;TArray<FString> Files;
        IFileManager::Get().FindFilesRecursive(Files,*FPaths::ProjectContentDir(),TEXT("*"),true,false);
        for(const FString& File:Files)if(File.EndsWith(TEXT(".uasset")) || File.EndsWith(TEXT(".umap")))
            Result.Add(File,NodeForgePrototype::FileSha256(File));
        return Result;
    };
    auto RootedTargets=[]() {int32 Count=0;for(TObjectIterator<UTextureRenderTarget2D> It;It;++It)if(It->IsRooted())++Count;return Count;};
    const auto Before=Assets();TArray<TSharedPtr<FJsonValue>> Rows;bool Good=true;
    for(const TCHAR* Name:{TEXT("UnknownScalar"),TEXT("UnknownVector"),TEXT("ValidAfterFailures")})
    {
        auto Case=MakeShared<FJsonObject>();Case->Values=Config->Values;const FString Label=Name;
        if(Label==TEXT("UnknownScalar"))
        {auto Changes=MakeShared<FJsonObject>();Changes->SetNumberField(TEXT("NightMissingScalar"),1);Case->SetObjectField(TEXT("scalarChanges"),Changes);}
        if(Label==TEXT("UnknownVector"))
        {auto Changes=MakeShared<FJsonObject>();Changes->SetArrayField(TEXT("NightMissingVector"),{MakeShared<FJsonValueNumber>(1),MakeShared<FJsonValueNumber>(0),MakeShared<FJsonValueNumber>(0),MakeShared<FJsonValueNumber>(1)});Case->SetObjectField(TEXT("vectorChanges"),Changes);}
        const FString CaseInput=Output+TEXT(".")+Label+TEXT(".Input.json"),CaseReport=Output+TEXT(".")+Label+TEXT(".json");
        if(!NodeForgePrototype::WriteJson(Case,CaseInput)) {AddError(TEXT("Fresh visual guard input write failed"));return false;}
        const int32 RootedBefore=RootedTargets();const int32 Code=NodeForgeGraphChecks::Execute(TEXT("GraphVisualCheck"),CaseInput,CaseReport);
        const int32 RootedAfter=RootedTargets();int32 Captures=0;bool Assignment=false;
        UWorld* World=GEditor?GEditor->GetEditorWorldContext().World():nullptr;
        if(World)
        {
            for(TActorIterator<ASceneCapture2D> It(World);It;++It)if(!It->IsActorBeingDestroyed())++Captures;
            for(TActorIterator<AStaticMeshActor> It(World);It;++It)
                if(It->GetActorLabel().Equals(TEXT("GeneratedSurface"),ESearchCase::CaseSensitive)
                    && It->GetStaticMeshComponent() && It->GetStaticMeshComponent()->GetMaterial(0))
                    Assignment=It->GetStaticMeshComponent()->GetMaterial(0)->GetPathName().Equals(Material+TEXT(".")+FPackageName::GetShortName(Material),ESearchCase::CaseSensitive);
        }
        TSharedPtr<FJsonObject> Actual;FString ActualText,ActualDigest,ActualError,Reason;
        const bool Read=NodeForgePrototype::ReadInputFile(CaseReport,ActualText,ActualDigest,ActualError)
            && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(ActualText),Actual) && Actual;
        if(Read)Actual->TryGetStringField(TEXT("reason"),Reason);
        const bool Positive=Label==TEXT("ValidAfterFailures");
        const bool Expected=Read && (Positive?Code==0 && Actual->GetStringField(TEXT("result"))==TEXT("passed"):
            Code==1 && Actual->GetStringField(TEXT("result"))==TEXT("failed") && Reason.Contains(Label==TEXT("UnknownScalar")?TEXT("scalar"):TEXT("vector")));
        const bool Passed=Expected && RootedBefore==RootedAfter && Captures==0 && Assignment;
        Good &= TestTrue(Label,Passed);
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("case"),Label);Row->SetBoolField(TEXT("passed"),Passed);
        Row->SetNumberField(TEXT("code"),Code);Row->SetNumberField(TEXT("rootedTargetsBefore"),RootedBefore);Row->SetNumberField(TEXT("rootedTargetsAfter"),RootedAfter);
        Row->SetNumberField(TEXT("liveCaptureActorsAfter"),Captures);Row->SetBoolField(TEXT("originalAssignmentRestored"),Assignment);
        Row->SetStringField(TEXT("report"),CaseReport);Row->SetStringField(TEXT("reason"),Reason);Rows.Add(MakeShared<FJsonValueObject>(Row));
    }
    const auto After=Assets();bool AssetsSame=Before.Num()==After.Num();
    for(const auto& Pair:Before){const FString* Found=After.Find(Pair.Key);AssetsSame &= Found && Pair.Value.Len()==64 && Pair.Value.Equals(*Found,ESearchCase::CaseSensitive);}
    const bool InputSame=Digest.Equals(NodeForgePrototype::FileSha256(Input),ESearchCase::CaseSensitive);
    Good &= TestTrue(TEXT("Saved assets and source config unchanged"),AssetsSame && InputSame);
    auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("test"),TEXT("NodeForge.VisualCleanupGuards"));
    Report->SetStringField(TEXT("status"),Good && !HasAnyErrors()?TEXT("passed"):TEXT("failed"));Report->SetArrayField(TEXT("cases"),Rows);
    Report->SetBoolField(TEXT("assetFilesUnchanged"),AssetsSame);Report->SetBoolField(TEXT("inputUnchanged"),InputSame);
    const FString Module=FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"));
    Report->SetStringField(TEXT("moduleFilename"),Module);Report->SetStringField(TEXT("moduleFileSha256"),NodeForgePrototype::FileSha256(Module));
    Report->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());
    Report->SetStringField(TEXT("scope"),TEXT("Two actual parameter failures then valid D3D11 capture in one process; inspect released roots/captures and restored assignment; saved assets never resaved, no human UI"));
    return NodeForgePrototype::WriteJson(Report,Output) && Good && !HasAnyErrors();
}
#endif
