// Native test of the production selection dispatcher; no mouse/clipboard automation.
#include "CoreMinimal.h"
#include "NodeForgePanel.h"
#include "NodeForgePrototype.h"
#include "NodeForgeResultView.h"
#include "NodeForgeExportFixtureChecks.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "AssetRegistry/AssetData.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"

namespace
{
TSharedPtr<FJsonObject> ResourceJson(const FString& Path,bool IsInput=false)
{
    FString Text,Digest,Error; TSharedPtr<FJsonObject> Json;
    const bool Read=IsInput?NodeForgePrototype::ReadInputFile(Path,Text,Digest,Error)
        :NodeForgeResultView::Read(Path,Text,Error);
    if(Read) FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Json);
    return Json;
}
FString JsonText(const TSharedPtr<FJsonObject>& Json)
{
    FString Text;
    if(Json.IsValid()) FJsonSerializer::Serialize(Json.ToSharedRef(),TJsonWriterFactory<>::Create(&Text));
    return Text;
}
bool ResourceName(const FString& Name)
{
    if(Name.IsEmpty() || Name.Len()>24) return false;
    for(TCHAR C:Name) if((!FChar::IsAlpha(C) && !FChar::IsDigit(C)) || C>127) return false;
    return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeResourceSelectionTest,"NodeForge.ResourceSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNodeForgeResourceSelectionTest::RunTest(const FString& Parameters)
{
    FString ConfigPath,ReportPath;
    if(!FParse::Value(FCommandLine::Get(),TEXT("NFResourceConfig="),ConfigPath)
        || !FParse::Value(FCommandLine::Get(),TEXT("NFResourceReport="),ReportPath)
        || IFileManager::Get().FileExists(*ReportPath))
    { AddError(TEXT("Fresh resource test config/report required")); return false; }
    FString SetupError,FullConfig=FPaths::ConvertRelativePathToFull(ConfigPath);
    if(!NodeForgeExportFixtureChecks::ValidateSetup(ReportPath,{},TEXT(""),SetupError)
        || !FPaths::CollapseRelativeDirectories(FullConfig)
        || !FPaths::IsUnderDirectory(FullConfig,NodeForgePrototype::WorkspaceDirectory()/TEXT("Reports"))
        || !FPaths::GetExtension(FullConfig).Equals(TEXT("json"),ESearchCase::CaseSensitive))
    { AddError(TEXT("Resource test requires own Reports paths and an isolated RegressionRuns project: ")+SetupError);return false; }
    auto Config=ResourceJson(FullConfig,true);
    const TArray<TSharedPtr<FJsonValue>>* Cases=nullptr;
    if(!Config.IsValid() || !Config->TryGetArrayField(TEXT("cases"),Cases) || Cases->Num()<1 || Cases->Num()>32)
    { AddError(TEXT("Bounded resource test cases required")); return false; }
    TArray<TSharedPtr<FJsonValue>> Results;
    TSet<FString> Names;
    for(const auto& Value:*Cases)
    {
        TSharedPtr<FJsonObject> Case=Value.IsValid()&&Value->Type==EJson::Object?Value->AsObject():nullptr;
        FString Name,Expected;
        const TArray<TSharedPtr<FJsonValue>>* Paths=nullptr;
        if(!Case.IsValid() || !Case->TryGetStringField(TEXT("name"),Name) || !ResourceName(Name) || Names.Contains(Name)
            || !Case->TryGetStringField(TEXT("expected"),Expected) || !Case->TryGetArrayField(TEXT("selection"),Paths) || Paths->Num()>2)
        { AddError(TEXT("Invalid resource case")); return false; }
        Names.Add(Name);
        TArray<FAssetData> Selection;
        TArray<UPackage*> Packages;
        TArray<bool> DirtyBefore;
        TArray<FString> Files,BeforeHashes;
        bool Dirty=false;Case->TryGetBoolField(TEXT("dirty"),Dirty);
        for(const auto& PathValue:*Paths)
        {
            FString Path;
            if(!PathValue.IsValid() || !PathValue->TryGetString(Path) || !(Path.StartsWith(TEXT("/Engine/")) || Path.StartsWith(TEXT("/Game/NodeForgeTests/"))))
            { AddError(TEXT("Resource test paths must be engine or own test assets")); return false; }
            UObject* Asset=LoadObject<UObject>(nullptr,*Path);
            if(!Asset) { AddError(TEXT("Actual resource missing: ")+Path); return false; }
            Selection.Add(FAssetData(Asset));
            UPackage* Package=Asset->GetOutermost();
            Packages.Add(Package);DirtyBefore.Add(Package->IsDirty());
            const FString File=FPackageName::LongPackageNameToFilename(Package->GetName(),FPackageName::GetAssetPackageExtension());
            Files.Add(File);BeforeHashes.Add(NodeForgePrototype::FileSha256(File));
        }
        // Dirty-case mutation is memory-only in this isolated copied test process.
        if(Dirty)
        {
            if(Selection.Num()!=1 || !Selection[0].GetObjectPathString().StartsWith(TEXT("/Game/NodeForgeTests/")))
            { AddError(TEXT("Dirty test only permits one copied project asset")); return false; }
            Packages[0]->SetDirtyFlag(true);
        }
        const FString Report=FPaths::GetPath(ReportPath)/(Name+TEXT(".json"));
        FString Error;
        TArray<bool> DirtyAtDispatch;
        for(UPackage* Package:Packages) DirtyAtDispatch.Add(Package->IsDirty());
        const int32 Code=NodeForgePanel::InspectSelection(Selection,Report,Error);
        bool DirtyPreserved=true;
        for(int32 I=0;I<Packages.Num();++I)
        {
            DirtyPreserved &= Packages[I]->IsDirty()==DirtyAtDispatch[I];
            Packages[I]->SetDirtyFlag(DirtyBefore[I]);
        }
        auto Data=ResourceJson(Report);
        bool Good=false;
        if(Expected==TEXT("notExecuted")) Good=Code<0 && !Error.IsEmpty() && !Data.IsValid();
        else if(Expected==TEXT("rejected")) Good=Code>0 && Data.IsValid()
            && Data->GetStringField(TEXT("overallStatus"))==TEXT("rejected")
            && Data->GetObjectField(TEXT("residue"))->GetStringField(TEXT("diskState"))==TEXT("notWritten");
        else if(Expected==TEXT("passed"))
        {
            Good=Code==0 && Data.IsValid() && Data->GetStringField(TEXT("operationStatus"))==TEXT("passed");
            if(Good && Data->GetStringField(TEXT("mode"))==TEXT("MaterialFunction"))
            {
                const auto Signature=Data->GetObjectField(TEXT("functionSignature"));
                const auto Node=Data->GetObjectField(TEXT("nodeTemplate"));
                Good=Node->GetStringField(TEXT("class"))==TEXT("/Script/Engine.MaterialExpressionMaterialFunctionCall")
                    && JsonText(Node->GetObjectField(TEXT("properties")))==JsonText(Signature->GetObjectField(TEXT("properties")))
                    && Signature->GetObjectField(TEXT("properties"))->GetStringField(TEXT("MaterialFunction"))==Selection[0].GetObjectPathString()
                    && Signature->GetObjectField(TEXT("properties"))->GetStringField(TEXT("FunctionAssetSha256"))==BeforeHashes[0];
                // Same bound native signature via CLI/backend: selection cannot invent ports.
                const FString DirectInput=FPaths::GetPath(ReportPath)/(Name+TEXT("-DirectInput.json"));
                const FString DirectReport=FPaths::GetPath(ReportPath)/(Name+TEXT("-Direct.json"));
                auto Input=MakeShared<FJsonObject>();Input->SetStringField(TEXT("function"),Selection[0].GetObjectPathString());
                const bool Wrote=NodeForgePrototype::WriteJson(Input,DirectInput);
                const int32 DirectCode=Wrote?NodeForgePrototype::Execute(TEXT("MaterialFunction"),DirectInput,DirectReport):-1;
                const auto Direct=ResourceJson(DirectReport);
                Good=Good && DirectCode==0 && Direct.IsValid() && JsonText(Signature)==JsonText(Direct->GetObjectField(TEXT("functionSignature")));
            }
        }
        bool BytesUnchanged=true,DirtyRestored=true;
        for(int32 I=0;I<Files.Num();++I)
        {
            BytesUnchanged &= BeforeHashes[I].Len()==64 && NodeForgePrototype::FileSha256(Files[I])==BeforeHashes[I];
            DirtyRestored &= Packages[I]->IsDirty()==DirtyBefore[I];
        }
        bool OverwriteRejected=true;
        if(Code>=0)
        {
            const FString Before=NodeForgePrototype::FileSha256(Report);
            FString RepeatError;
            OverwriteRejected=NodeForgePanel::InspectSelection(Selection,Report,RepeatError)<0 && !RepeatError.IsEmpty()
                && Before.Len()==64 && NodeForgePrototype::FileSha256(Report)==Before;
        }
        Good &= BytesUnchanged && DirtyPreserved && DirtyRestored && OverwriteRejected;
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("case"),Name);
        Row->SetStringField(TEXT("result"),Good?TEXT("passed"):TEXT("failed"));Row->SetStringField(TEXT("expected"),Expected);
        Row->SetNumberField(TEXT("exitCode"),Code);Row->SetStringField(TEXT("selectionError"),Error);
        Row->SetStringField(TEXT("report"),Code>=0?Report:FString());Row->SetBoolField(TEXT("selectedAssetBytesUnchanged"),BytesUnchanged);
        Row->SetBoolField(TEXT("dirtyFlagsRestored"),DirtyRestored);Row->SetBoolField(TEXT("reportOverwriteRejected"),OverwriteRejected);
        Row->SetBoolField(TEXT("dirtyFlagsPreservedDuringDispatch"),DirtyPreserved);
        Results.Add(MakeShared<FJsonValueObject>(Row));
        if(!Good) AddError(TEXT("Resource selection failed: ")+Name);
    }
    auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("test"),TEXT("NodeForge.ResourceSelection"));
    Report->SetStringField(TEXT("status"),HasAnyErrors()?TEXT("failed"):TEXT("passed"));Report->SetArrayField(TEXT("cases"),Results);
    Report->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());
    Report->SetStringField(TEXT("scope"),TEXT("Production selection dispatcher on actual FAssetData; native signature/fragment parity, rejection and byte/dirty-state protection; no human clicks/content-browser selection/clipboard proof"));
    if(!NodeForgePrototype::WriteJson(Report,ReportPath)) {AddError(TEXT("Cannot save resource test evidence"));return false;}
    return !HasAnyErrors();
}
#endif
