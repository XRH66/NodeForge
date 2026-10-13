// Production rule-query lifecycle regression. Reuses native Execute/Select/catalog and GC.
#include "NodeForgePrototype.h"
#include "NodeForgeTaskRules.h"
#include "NodeForgeGraphDiff.h"
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
#include "EdGraph/EdGraph.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintTypePromotion.h"
#include "UObject/GarbageCollection.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/WeakObjectPtrTemplates.h"

namespace
{
using FLifetimeObject=TSharedPtr<FJsonObject>;
FLifetimeObject LifetimeJson(const FString& Path)
{
    FString Text;FLifetimeObject Value;
    if(FFileHelper::LoadFileToString(Text,*Path))FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Value);
    return Value;
}
bool LifetimeFresh(const TSharedRef<FJsonObject>& Value,const FString& Path)
{
    return !IFileManager::Get().FileExists(*Path)&&NodeForgePrototype::WriteJson(Value,Path);
}
bool LifetimeEqual(const FLifetimeObject& A,const FLifetimeObject& B)
{
    return A&&B&&NodeForgeGraphDiff::EqualValue(MakeShared<FJsonValueObject>(A),MakeShared<FJsonValueObject>(B));
}
// Adapted from NodeForgeValidationChecks' project content/reservation byte guard.
TSharedRef<FJsonObject> LifetimeAssets()
{
    const auto Snapshot=MakeShared<FJsonObject>();
    for(const FString& Root:{FPaths::ProjectContentDir(),FPaths::ProjectSavedDir()/TEXT("NodeForge/Requests")})
    {
        TArray<FString> Files;IFileManager::Get().FindFilesRecursive(Files,*Root,TEXT("*"),true,false);Files.Sort();
        for(const FString& File:Files)Snapshot->SetStringField(File,NodeForgePrototype::FileSha256(File));
    }
    return Snapshot;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeRuleLifetimeTest,"NodeForge.RuleLifetime",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNodeForgeRuleLifetimeTest::RunTest(const FString& Parameters)
{
    FString Output;
    const FString Root=NodeForgePrototype::WorkspaceDirectory();
    if(!FParse::Value(FCommandLine::Get(),TEXT("NFReport="),Output)
        ||IFileManager::Get().FileExists(*Output)
        ||!FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Output),Root/TEXT("Reports"))
        ||!FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()),Root/TEXT("ValidationProject/RegressionRuns"))
        ||!IsInGameThread()||IsGarbageCollecting())
    {AddError(TEXT("RuleLifetime requires a fresh Reports output, game thread and independent RegressionRuns project."));return false;}
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Output),true);
    const FString Directory=FPaths::GetPath(Output);
    const FString Binary=FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"));
    const FString BinaryHash=NodeForgePrototype::FileSha256(Binary);
    if(BinaryHash.Len()!=64){AddError(TEXT("Cannot fingerprint actual rule-query module."));return false;}
    const auto AssetsBefore=LifetimeAssets();
    TArray<TSharedPtr<FJsonValue>> Rows,GcRows;
    TMap<FString,FString> StableSummaries;
    FString FullDigest;
    FLifetimeObject BaselineCatalog;
    FLifetimeObject RefreshEvidence;
    int32 RefreshCalls=0;

    auto Started=[&](const FString& Name,const FString& Mode,const FString& Input)->bool
    {
        const auto Marker=MakeShared<FJsonObject>();Marker->SetStringField(TEXT("step"),Name);
        Marker->SetStringField(TEXT("mode"),Mode);Marker->SetStringField(TEXT("input"),Input);
        Marker->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());
        Marker->SetStringField(TEXT("moduleFilename"),Binary);Marker->SetStringField(TEXT("moduleFileSha256"),BinaryHash);
        Marker->SetStringField(TEXT("timeUtc"),FDateTime::UtcNow().ToIso8601());
        Marker->SetStringField(TEXT("state"),TEXT("aboutToExecute; absence of terminal evidence is not a pass"));
        return LifetimeFresh(Marker,Directory/(Name+TEXT("Started.json")));
    };
    auto CatalogStep=[&](const FString& Name,bool First)->bool
    {
        const FString Report=Directory/(Name+TEXT(".json"));
        if(IFileManager::Get().FileExists(*Report)||!Started(Name,TEXT("GraphCatalog"),FString()))return false;
        const int32 Code=NodeForgePrototype::Execute(TEXT("GraphCatalog"),FString(),Report);
        const auto Data=LifetimeJson(Report);const FLifetimeObject* Catalog=nullptr;FString Digest,Operation;
        bool Good=Code==0&&Data&&Data->TryGetObjectField(TEXT("catalog"),Catalog)
            &&Data->TryGetStringField(TEXT("catalogDigest"),Digest)&&Digest.Len()==64
            &&Data->TryGetStringField(TEXT("operationStatus"),Operation)&&Operation==TEXT("passed");
        if(Good)
        {
            for(const TCHAR* Kind:{TEXT("material"),TEXT("blueprint")})
            {
                const FLifetimeObject* Section=nullptr;const TArray<TSharedPtr<FJsonValue>>* Nodes=nullptr;
                Good&=(*Catalog)->TryGetObjectField(Kind,Section)&&(*Section)->TryGetArrayField(TEXT("nodes"),Nodes)&&!Nodes->IsEmpty();
            }
            if(First){BaselineCatalog=*Catalog;FullDigest=Digest;}
            else Good&=Digest.Equals(FullDigest,ESearchCase::CaseSensitive)&&LifetimeEqual(*Catalog,BaselineCatalog);
        }
        Good&=LifetimeEqual(AssetsBefore,LifetimeAssets())&&NodeForgePrototype::FileSha256(Binary)==BinaryHash;
        TestTrue(Name,Good);
        const auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("step"),Name);Row->SetBoolField(TEXT("passed"),Good);
        Row->SetStringField(TEXT("report"),Report);Row->SetNumberField(TEXT("exitCode"),Code);Row->SetStringField(TEXT("catalogDigest"),Digest);
        Rows.Add(MakeShared<FJsonValueObject>(Row));
        return LifetimeFresh(Row,Directory/(Name+TEXT("Assertions.json")))&&Good;
    };
    auto QueryStep=[&](const FString& Name,const FString& Kind,const FString& Query)->bool
    {
        const FString Input=Directory/(Name+TEXT("Input.json")),Report=Directory/(Name+TEXT(".json"));
        const auto Config=MakeShared<FJsonObject>();Config->SetStringField(TEXT("assetKind"),Kind);Config->SetStringField(TEXT("query"),Query);
        if(!LifetimeFresh(Config,Input)||IFileManager::Get().FileExists(*Report)||!Started(Name,TEXT("GraphRules"),Input))return false;
        const FString InputHash=NodeForgePrototype::FileSha256(Input);
        // Execute reconstructs native expression/Blueprint instance catalogs on EACH call.
        // Holding only JSON here neither roots spawners nor bypasses that production path.
        const int32 Code=NodeForgePrototype::Execute(TEXT("GraphRules"),Input,Report);
        const auto Data=LifetimeJson(Report);const FLifetimeObject* Rules=nullptr;const FLifetimeObject* Fingerprint=nullptr;
        FLifetimeObject Expected;FString Error,Operation,Overall,Digest,ActualBinary,ActualHash,Summary;
        bool Good=Code==0&&Data&&Data->TryGetObjectField(TEXT("taskRules"),Rules)
            &&Data->TryGetStringField(TEXT("operationStatus"),Operation)&&Operation==TEXT("passed")
            &&Data->TryGetStringField(TEXT("overallStatus"),Overall)&&Overall==TEXT("succeeded")
            &&Data->TryGetStringField(TEXT("catalogDigest"),Digest)&&Digest.Equals(FullDigest,ESearchCase::CaseSensitive)
            &&Data->TryGetObjectField(TEXT("readOnlyExecutionFingerprint"),Fingerprint)
            &&(*Fingerprint)->TryGetStringField(TEXT("binary"),ActualBinary)&&FPaths::IsSamePath(ActualBinary,Binary)
            &&(*Fingerprint)->TryGetStringField(TEXT("binarySha256"),ActualHash)&&ActualHash==BinaryHash;
        double Matches=0,Count=0;
        if(Good)
        {
            Good&=NodeForgeTaskRules::Select(Config,BaselineCatalog.ToSharedRef(),FullDigest,Expected,Error)
                &&LifetimeEqual(*Rules,Expected)&&(*Rules)->TryGetNumberField(TEXT("matchedNodeCount"),Matches)&&Matches>0
                &&(*Rules)->TryGetNumberField(TEXT("sourceNodeCount"),Count)&&Count>=Matches;
            if(Query.IsEmpty())Good&=Matches==Count;
            const FLifetimeObject* Capabilities=nullptr;const TArray<TSharedPtr<FJsonValue>>* Nodes=nullptr;
            Good&=(*Rules)->TryGetObjectField(TEXT("capabilities"),Capabilities)
                &&(*Capabilities)->TryGetArrayField(TEXT("nodes"),Nodes)&&Nodes->Num()==Matches;
            const FLifetimeObject* Checks=nullptr;const FLifetimeObject* Residue=nullptr;
            Good&=Data->TryGetObjectField(TEXT("checks"),Checks)&&Data->TryGetObjectField(TEXT("residue"),Residue);
            if(Checks&&Residue)
            {
                for(const TCHAR* Stage:{TEXT("build"),TEXT("initialReadback"),TEXT("compile"),TEXT("finalReadback"),TEXT("publish"),TEXT("save"),TEXT("persistence"),TEXT("runtime")})
                {
                    const FLifetimeObject* Check=nullptr;FString Status;
                    Good&=(*Checks)->TryGetObjectField(Stage,Check)&&(*Check)->TryGetStringField(TEXT("status"),Status)&&Status==TEXT("notRun");
                }
                bool Save=true,Present=true;FString Disk;
                Good&=(*Residue)->TryGetBoolField(TEXT("saveAttempted"),Save)&&!Save
                    &&(*Residue)->TryGetBoolField(TEXT("targetFilePresent"),Present)&&!Present
                    &&(*Residue)->TryGetStringField(TEXT("diskState"),Disk)&&Disk==TEXT("notWritten");
            }
            Summary=NodeForgeResultView::Compact(Data.ToSharedRef());
            Good&=!Summary.IsEmpty()&&Summary.Len()<=1200&&Summary.Contains(TEXT("规则匹配"));
            const FString Key=Kind+TEXT("|")+Query;
            if(const FString* Previous=StableSummaries.Find(Key))Good&=Previous->Equals(Summary,ESearchCase::CaseSensitive);
            else StableSummaries.Add(Key,Summary);
        }
        const bool AssetsUnchanged=LifetimeEqual(AssetsBefore,LifetimeAssets());
        Good&=AssetsUnchanged&&InputHash.Len()==64&&NodeForgePrototype::FileSha256(Input)==InputHash
            &&NodeForgePrototype::FileSha256(Binary)==BinaryHash;
        TestTrue(Name,Good);
        const auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("step"),Name);Row->SetBoolField(TEXT("passed"),Good);
        Row->SetStringField(TEXT("assetKind"),Kind);Row->SetStringField(TEXT("query"),Query);Row->SetStringField(TEXT("report"),Report);
        Row->SetNumberField(TEXT("exitCode"),Code);Row->SetStringField(TEXT("catalogDigest"),Digest);Row->SetStringField(TEXT("summary"),Summary);
        Row->SetNumberField(TEXT("matchedNodeCount"),Matches);Row->SetNumberField(TEXT("sourceNodeCount"),Count);
        Row->SetBoolField(TEXT("projectAssetsAndReservationsUnchanged"),AssetsUnchanged);Row->SetStringField(TEXT("inputSha256"),InputHash);
        Row->SetStringField(TEXT("expectedSelectionError"),Error);Rows.Add(MakeShared<FJsonValueObject>(Row));
        return LifetimeFresh(Row,Directory/(Name+TEXT("Assertions.json")))&&Good;
    };
    auto GarbageCollectStep=[&](const FString& Name)->bool
    {
        TWeakObjectPtr<UEdGraph> Sentinel=NewObject<UEdGraph>(GetTransientPackage(),NAME_None,RF_Transient);
        const bool WasValid=Sentinel.IsValid();
        if(!Started(Name,TEXT("CollectGarbage"),FString()))return false;
        CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS,true);
        const bool Reclaimed=!Sentinel.IsValid();
        const bool Good=WasValid&&Reclaimed&&LifetimeEqual(AssetsBefore,LifetimeAssets())
            &&NodeForgePrototype::FileSha256(Binary)==BinaryHash;
        TestTrue(Name,Good);
        const auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("step"),Name);Row->SetBoolField(TEXT("passed"),Good);
        Row->SetBoolField(TEXT("collectGarbageReturned"),true);Row->SetBoolField(TEXT("fullPurge"),true);
        Row->SetBoolField(TEXT("weakSentinelWasValid"),WasValid);Row->SetBoolField(TEXT("weakSentinelReclaimed"),Reclaimed);
        Row->SetNumberField(TEXT("keepFlags"),static_cast<uint32>(GARBAGE_COLLECTION_KEEPFLAGS));
        Row->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());GcRows.Add(MakeShared<FJsonValueObject>(Row));
        return LifetimeFresh(Row,Directory/(Name+TEXT("Completed.json")))&&Good;
    };

    auto NativeRefreshSetup=[&]()->bool
    {
        const FString Name=TEXT("NativePromotionRefreshSetup");
        if(!Started(Name,TEXT("RefreshPromotionTables"),FString()))return false;
        // This ONE test setup mirrors UE's module/reload refresh lifecycle. It is
        // deliberately not a query/GC workaround and is absent from production code.
        auto& Database=FBlueprintActionDatabase::Get();
        FTypePromotion::Get();
        const bool EnabledBefore=TypePromoDebug::IsTypePromoEnabled();
        const int32 ActionsBefore=Database.GetAllActions().Num();
        int32 RegisteredBefore=0,RegisteredAfter=0;
        for(const FName Op:FTypePromotion::GetAllOpNames())
            if(FTypePromotion::GetOperatorSpawner(Op)!=nullptr)++RegisteredBefore;
        FTypePromotion::RefreshPromotionTables();++RefreshCalls;
        for(const FName Op:FTypePromotion::GetAllOpNames())
            if(FTypePromotion::GetOperatorSpawner(Op)!=nullptr)++RegisteredAfter;
        const bool EnabledAfter=TypePromoDebug::IsTypePromoEnabled();
        const bool Good=EnabledBefore&&EnabledBefore==EnabledAfter&&RegisteredAfter==0
            &&ActionsBefore>0&&ActionsBefore==Database.GetAllActions().Num()
            &&RefreshCalls==1&&LifetimeEqual(AssetsBefore,LifetimeAssets())
            &&NodeForgePrototype::FileSha256(Binary)==BinaryHash;
        TestTrue(TEXT("One native type-promotion refresh setup with unchanged editor setting and action database"),Good);
        RefreshEvidence=MakeShared<FJsonObject>();RefreshEvidence->SetStringField(TEXT("step"),Name);
        RefreshEvidence->SetBoolField(TEXT("passed"),Good);RefreshEvidence->SetNumberField(TEXT("refreshCalls"),RefreshCalls);
        RefreshEvidence->SetBoolField(TEXT("typePromotionEnabledBefore"),EnabledBefore);
        RefreshEvidence->SetBoolField(TEXT("typePromotionEnabledAfter"),EnabledAfter);
        RefreshEvidence->SetNumberField(TEXT("actionDatabaseEntriesBefore"),ActionsBefore);
        RefreshEvidence->SetNumberField(TEXT("actionDatabaseEntriesAfter"),Database.GetAllActions().Num());
        RefreshEvidence->SetNumberField(TEXT("operatorSpawnersRegisteredBefore"),RegisteredBefore);
        RefreshEvidence->SetNumberField(TEXT("operatorSpawnersRegisteredAfter"),RegisteredAfter);
        RefreshEvidence->SetStringField(TEXT("scope"),TEXT("Test-only single public native RefreshPromotionTables after database initialization; mirrors module/reload lifecycle. No per-query reset, AddToRoot, error suppression, setting change or production fix."));
        RefreshEvidence->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());
        return LifetimeFresh(RefreshEvidence.ToSharedRef(),Directory/(Name+TEXT("Completed.json")))&&Good;
    };

    bool Completed=CatalogStep(TEXT("CatalogBefore"),true);
    if(Completed)Completed=NativeRefreshSetup();
    for(int32 Cycle=1;Cycle<=3&&Completed;++Cycle)
        for(const TCHAR* Kind:{TEXT("Blueprint"),TEXT("Material")})
        {
            const FString Prefix=FString::Printf(TEXT("Cycle%d%s"),Cycle,Kind);
            Completed=QueryStep(Prefix+TEXT("Add"),Kind,TEXT("add"))
                &&GarbageCollectStep(Prefix+TEXT("Gc"))&&QueryStep(Prefix+TEXT("Blank"),Kind,FString());
            if(!Completed)break;
        }
    if(Completed)Completed=CatalogStep(TEXT("CatalogAfter"),false);
    const bool AssetsUnchanged=LifetimeEqual(AssetsBefore,LifetimeAssets());
    Completed&=Rows.Num()==14&&GcRows.Num()==6&&StableSummaries.Num()==4&&AssetsUnchanged&&RefreshCalls==1;
    TestTrue(TEXT("All lifecycle rounds completed without replacing rules or changing assets"),Completed);
    const auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("test"),TEXT("NodeForge.RuleLifetime"));
    Report->SetStringField(TEXT("status"),Completed&&!HasAnyErrors()?TEXT("passed"):TEXT("failed"));
    Report->SetArrayField(TEXT("queriesAndCatalogs"),Rows);Report->SetArrayField(TEXT("garbageCollections"),GcRows);
    if(RefreshEvidence)Report->SetObjectField(TEXT("nativeRefreshSetup"),RefreshEvidence);
    Report->SetNumberField(TEXT("nativeRefreshSetupCalls"),RefreshCalls);
    Report->SetStringField(TEXT("catalogDigest"),FullDigest);Report->SetStringField(TEXT("moduleFilename"),Binary);
    Report->SetStringField(TEXT("moduleFileSha256"),BinaryHash);Report->SetStringField(TEXT("project"),FPaths::GetProjectFilePath());
    Report->SetStringField(TEXT("actualCatalog"),Directory/TEXT("CatalogAfter.json"));
    Report->SetStringField(TEXT("actualCatalogSha256"),NodeForgePrototype::FileSha256(Directory/TEXT("CatalogAfter.json")));
    Report->SetStringField(TEXT("baselineCatalog"),Directory/TEXT("CatalogBefore.json"));
    Report->SetStringField(TEXT("baselineCatalogSha256"),NodeForgePrototype::FileSha256(Directory/TEXT("CatalogBefore.json")));
    Report->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());Report->SetStringField(TEXT("timeUtc"),FDateTime::UtcNow().ToIso8601());
    Report->SetBoolField(TEXT("projectAssetsAndReservationsUnchanged"),AssetsUnchanged);
    Report->SetStringField(TEXT("scope"),TEXT("One native process: one explicit native type-promotion refresh setup after database initialization, then real catalog reconstruction and add -> full GC -> blank production rule queries, three rounds per family; exact catalog/subset and compact-summary stability. No asset generation, UI click, clipboard, import, save, runtime or packaging proof. No per-query/per-GC cache reset, global setting changes, test-owned spawner roots or error suppression."));
    return LifetimeFresh(Report,Output)&&Completed&&!HasAnyErrors();
}
#endif
