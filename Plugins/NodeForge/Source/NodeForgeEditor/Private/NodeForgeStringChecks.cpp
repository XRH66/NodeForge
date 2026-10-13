// Project-owned native mutation/recovery checks. No engine implementation is copied.
#include "CoreMinimal.h"
#include "NodeForgeBlueprintGraph.h"
#include "NodeForgeGraphCodec.h"
#include "NodeForgePrototype.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "EdGraph/EdGraph.h"
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
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace
{
bool StringGuardPath(FString& Path)
{
    Path = FPaths::ConvertRelativePathToFull(Path); FPaths::NormalizeFilename(Path);
    return FPaths::CollapseRelativeDirectories(Path);
}
TMap<FString,FString> StringGuardAssets()
{
    TMap<FString,FString> Result; TArray<FString> Files;
    IFileManager::Get().FindFilesRecursive(Files,*FPaths::ProjectContentDir(),TEXT("*"),true,false);
    for(const FString& File:Files) if(File.EndsWith(TEXT(".uasset")) || File.EndsWith(TEXT(".umap")))
        Result.Add(File,NodeForgePrototype::FileSha256(File));
    return Result;
}
bool StringGuardSameAssets(const TMap<FString,FString>& A,const TMap<FString,FString>& B)
{
    if(A.Num()!=B.Num())return false;
    for(const auto& Pair:A){const FString* Found=B.Find(Pair.Key);if(!Found || Pair.Value.Len()!=64 || !Found->Equals(Pair.Value,ESearchCase::CaseSensitive))return false;}
    return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeStringIntegrityGuards,"NodeForge.StringIntegrityGuards",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNodeForgeStringIntegrityGuards::RunTest(const FString& Parameters)
{
    FString Input,ReportPath,Source;FString Project=FPaths::GetProjectFilePath();
    FString Reports=NodeForgePrototype::WorkspaceDirectory()/TEXT("Reports");
    FString Projects=NodeForgePrototype::WorkspaceDirectory()/TEXT("ValidationProject/RegressionRuns");
    if(!FParse::Value(FCommandLine::Get(),TEXT("NFInput="),Input) || !FParse::Value(FCommandLine::Get(),TEXT("NFReport="),ReportPath)
        || !FParse::Value(FCommandLine::Get(),TEXT("NFExportSource="),Source)
        || !StringGuardPath(Project) || !StringGuardPath(Reports) || !StringGuardPath(Projects) || !StringGuardPath(Input) || !StringGuardPath(ReportPath)
        || !FPaths::IsUnderDirectory(Project,Projects) || !FPaths::IsUnderDirectory(Input,Reports) || !FPaths::IsUnderDirectory(ReportPath,Reports)
        || !Input.EndsWith(TEXT(".json")) || !ReportPath.EndsWith(TEXT(".json"))
        || !IFileManager::Get().FileExists(*Project) || !IFileManager::Get().FileExists(*Input) || IFileManager::Get().FileExists(*ReportPath)
        || !Source.StartsWith(TEXT("/Game/NodeForgeTests/"),ESearchCase::CaseSensitive) || !FPackageName::IsValidObjectPath(Source))
    {AddError(TEXT("Fresh Reports input/output and explicit own saved source in an isolated RegressionRuns project required"));return false;}
    const auto Before=StringGuardAssets();const FString InputSha=NodeForgePrototype::FileSha256(Input);
    const FString Module=FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"));const FString ModuleSha=NodeForgePrototype::FileSha256(Module);
    auto Report=MakeShared<FJsonObject>();TArray<TSharedPtr<FJsonValue>> Rows;
    Report->SetStringField(TEXT("test"),TEXT("NodeForge.StringIntegrityGuards"));Report->SetStringField(TEXT("project"),Project);
    Report->SetStringField(TEXT("input"),Input);Report->SetStringField(TEXT("inputSha256"),InputSha);
    Report->SetStringField(TEXT("moduleFilename"),Module);Report->SetStringField(TEXT("moduleFileSha256"),ModuleSha);
    Report->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());
    Report->SetStringField(TEXT("scope"),TEXT("Real compiled transient CDO/pins/identities and clean saved source metadata temporarily changed and restored; no asset save, PIE, UI or packaging"));
    auto Finish=[&](bool Good,const FString& Reason)
    {
        const bool AssetsSame=StringGuardSameAssets(Before,StringGuardAssets());
        const bool InputSame=InputSha.Len()==64 && InputSha.Equals(NodeForgePrototype::FileSha256(Input),ESearchCase::CaseSensitive);
        const bool ModuleSame=ModuleSha.Len()==64 && ModuleSha.Equals(NodeForgePrototype::FileSha256(Module),ESearchCase::CaseSensitive);
        Good=Good && AssetsSame && InputSame && ModuleSame && !HasAnyErrors();
        Report->SetStringField(TEXT("status"),Good?TEXT("passed"):TEXT("failed"));Report->SetStringField(TEXT("reason"),Reason);
        Report->SetArrayField(TEXT("cases"),Rows);Report->SetBoolField(TEXT("assetFilesUnchanged"),AssetsSame);
        Report->SetBoolField(TEXT("inputUnchanged"),InputSame);Report->SetBoolField(TEXT("moduleUnchanged"),ModuleSame);
        if(!Good)AddError(TEXT("String integrity guard failed: ")+Reason);
        return NodeForgePrototype::WriteJson(Report,ReportPath) && Good;
    };
    auto Row=[&](const FString& Name,bool Good,const FString& Reason,bool Changed=false)
    {
        auto V=MakeShared<FJsonObject>();V->SetStringField(TEXT("name"),Name);V->SetBoolField(TEXT("passed"),Good);
        V->SetBoolField(TEXT("actualCaseOnlyChange"),Changed);V->SetStringField(TEXT("reason"),Reason);Rows.Add(MakeShared<FJsonValueObject>(V));
        return Good;
    };
    FString Text,Error,ReadDigest;NodeForgeBlueprintGraph::FObject Request,Evidence;
    if(!NodeForgePrototype::ReadInputFile(Input,Text,ReadDigest,Error)
        || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Request) || !Request)return Finish(false,TEXT("Invalid bounded JSON"));
    NodeForgeGraphDocument::FGraphDocument Document;
    auto ValidateFixture=[&](const NodeForgeBlueprintGraph::FObject& Input,FString& Reason)
    {return NodeForgeGraphCodec::Decode(Input,Document,Reason) && NodeForgeBlueprintGraph::Validate(Document,Reason);};
    if(!ValidateFixture(Request,Error))return Finish(false,TEXT("Invalid string guard fixture: ")+Error);
    const TArray<TSharedPtr<FJsonValue>>* Definitions=nullptr;const TArray<TSharedPtr<FJsonValue>>* Edges=nullptr;
    if(!Request->TryGetArrayField(TEXT("nodes"),Definitions) || Definitions->Num()<2
        || !Request->TryGetArrayField(TEXT("edges"),Edges) || Edges->IsEmpty())
        return Finish(false,TEXT("String guard requires its explicit Print node and execution edge fixture"));
    const auto PrintDefinition=(*Definitions)[1]->AsObject();
    const TSharedPtr<FJsonObject>* Properties=nullptr;FString PrintId,Owner;
    if(!PrintDefinition || !PrintDefinition->TryGetStringField(TEXT("id"),PrintId)
        || !PrintId.Equals(TEXT("Print"),ESearchCase::CaseSensitive)
        || !PrintDefinition->TryGetObjectField(TEXT("properties"),Properties)
        || !(*Properties)->TryGetStringField(TEXT("owner"),Owner))
        return Finish(false,TEXT("String guard requires the canonical Print function owner fixture"));
    const auto PrintProperties=*Properties;
    const auto SavedOwner=PrintProperties->Values.FindChecked(TEXT("owner"));
    PrintProperties->RemoveField(TEXT("owner"));PrintProperties->SetField(TEXT("Owner"),SavedOwner);
    const bool FieldRejected=!ValidateFixture(Request,Error) && !Error.IsEmpty();const FString FieldError=Error;
    PrintProperties->RemoveField(TEXT("Owner"));PrintProperties->SetField(TEXT("owner"),SavedOwner);Error.Reset();
    if(!Row(TEXT("exactPropertyKey"),FieldRejected && ValidateFixture(Request,Error),FieldError,true))return Finish(false,TEXT("exact property key guard"));
    const auto Edge=(*Edges)[0]->AsObject();const FString From=Edge->GetStringField(TEXT("from"));
    Edge->SetStringField(TEXT("from"),From.ToLower());Error.Reset();
    const bool EndpointRejected=!ValidateFixture(Request,Error) && !Error.IsEmpty();const FString EndpointError=Error;
    Edge->SetStringField(TEXT("from"),From);Error.Reset();
    if(!Row(TEXT("exactLogicalEndpoint"),EndpointRejected && ValidateFixture(Request,Error),EndpointError,true))return Finish(false,TEXT("exact logical endpoint guard"));
    UBlueprint* BP=NodeForgeBlueprintGraph::Build(GetTransientPackage(),MakeUniqueObjectName(GetTransientPackage(),UBlueprint::StaticClass(),TEXT("NodeForgeStringGuard")).ToString(),Document,Error);
    if(!BP)return Finish(false,TEXT("Transient build: ")+Error);
    BP->ClearFlags(RF_Public|RF_Standalone);BP->SetFlags(RF_Transient);ON_SCOPE_EXIT{BP->MarkAsGarbage();};
    FCompilerResultsLog Compile;FKismetEditorUtilities::CompileBlueprint(BP,EBlueprintCompileOptions::None,&Compile);
    Report->SetNumberField(TEXT("compilerErrors"),Compile.NumErrors);Report->SetNumberField(TEXT("compilerWarnings"),Compile.NumWarnings);
    Report->SetNumberField(TEXT("blueprintStatus"),static_cast<int32>(BP->Status));
    if(Compile.NumErrors || !BP->GeneratedClass || BP->GeneratedClass->ClassGeneratedBy!=BP
        || (BP->Status!=BS_UpToDate && BP->Status!=BS_UpToDateWithWarnings))return Finish(false,TEXT("Native compilation failed"));
    if(!Row(TEXT("baselineReadback"),NodeForgeBlueprintGraph::Readback(BP,Document,Evidence,Error),Error))return Finish(false,Error);
    UObject* CDO=BP->GeneratedClass->GetDefaultObject();
    auto* StringProperty=FindFProperty<FStrProperty>(BP->GeneratedClass,TEXT("Message"));
    auto* NameProperty=FindFProperty<FNameProperty>(BP->GeneratedClass,TEXT("Label"));
    auto* Graph=FBlueprintEditorUtils::FindEventGraph(BP);UEdGraphNode* Print=nullptr;UEdGraphNode* Begin=nullptr;
    if(Graph)for(UEdGraphNode* N:Graph->Nodes)
    {if(N && N->NodeComment.Equals(TEXT("NodeForge:Print"),ESearchCase::CaseSensitive))Print=N;
     if(N && N->NodeComment.Equals(TEXT("NodeForge:Begin"),ESearchCase::CaseSensitive))Begin=N;}
    UEdGraphPin* InString=Print?Print->FindPin(TEXT("InString"),EGPD_Input):nullptr;
    UEdGraphPin* Then=Begin?Begin->FindPin(TEXT("then"),EGPD_Output):nullptr;
    auto* Variable=BP->NewVariables.FindByPredicate([](const FBPVariableDescription& V){return V.VarName.ToString().Equals(TEXT("Message"),ESearchCase::CaseSensitive);});
    if(!StringProperty || !NameProperty || !Print || !Begin || !InString || !Then || !Variable || !InString->LinkedTo.IsEmpty())return Finish(false,TEXT("Expected actual string/name CDO and Print.InString/Begin.then required"));
    const FString SavedString=StringProperty->GetPropertyValue_InContainer(CDO),SavedPin=InString->DefaultValue,SavedComment=Print->NodeComment;
    const FName SavedName=NameProperty->GetPropertyValue_InContainer(CDO),SavedThen=Then->PinName,SavedVariable=Variable->VarName;
    if(!SavedString.Equals(TEXT("AbC"),ESearchCase::CaseSensitive) || !SavedPin.Equals(TEXT("AbC"),ESearchCase::CaseSensitive)
        || !SavedName.ToString().Equals(TEXT("AbC"),ESearchCase::CaseSensitive))return Finish(false,TEXT("Explicit mixed-case native fixtures required"));
    ON_SCOPE_EXIT{StringProperty->SetPropertyValue_InContainer(CDO,SavedString);NameProperty->SetPropertyValue_InContainer(CDO,SavedName);
        InString->DefaultValue=SavedPin;Print->NodeComment=SavedComment;Then->PinName=SavedThen;Variable->VarName=SavedVariable;};
    auto Mutate=[&](const TCHAR* Name,TFunction<void()> Apply,TFunction<void()> Restore,TFunction<bool()> Changed)
    {
        Apply();const bool Actual=Changed();Error.Reset();Evidence.Reset();
        const bool Rejected=!NodeForgeBlueprintGraph::Readback(BP,Document,Evidence,Error) && !Error.IsEmpty();const FString Rejection=Error;
        Restore();Error.Reset();Evidence.Reset();const bool Restored=NodeForgeBlueprintGraph::Readback(BP,Document,Evidence,Error);
        return Row(Name,Actual && Rejected && Restored,Rejected?Rejection:TEXT("case-only mutation was accepted"),Actual);
    };
    if(!Mutate(TEXT("compiledStringCase"),[&]{StringProperty->SetPropertyValue_InContainer(CDO,TEXT("abc"));},[&]{StringProperty->SetPropertyValue_InContainer(CDO,SavedString);},[&]{return StringProperty->GetPropertyValue_InContainer(CDO).Equals(TEXT("abc"),ESearchCase::CaseSensitive);}))return Finish(false,TEXT("compiled string case guard"));
    if(!Mutate(TEXT("compiledNameDisplayCase"),[&]{NameProperty->SetPropertyValue_InContainer(CDO,FName(TEXT("abc")));},[&]{NameProperty->SetPropertyValue_InContainer(CDO,SavedName);},[&]{return NameProperty->GetPropertyValue_InContainer(CDO).ToString().Equals(TEXT("abc"),ESearchCase::CaseSensitive);}))return Finish(false,TEXT("compiled name display guard"));
    if(!Mutate(TEXT("nativeStringPinCase"),[&]{InString->DefaultValue=TEXT("abc");},[&]{InString->DefaultValue=SavedPin;},[&]{return InString->DefaultValue.Equals(TEXT("abc"),ESearchCase::CaseSensitive);}))return Finish(false,TEXT("string pin case guard"));
    if(!Mutate(TEXT("logicalIdCase"),[&]{Print->NodeComment=TEXT("NodeForge:print");},[&]{Print->NodeComment=SavedComment;},[&]{return Print->NodeComment.Equals(TEXT("NodeForge:print"),ESearchCase::CaseSensitive);}))return Finish(false,TEXT("logical ID guard"));
    if(!Mutate(TEXT("logicalPrefixCase"),[&]{Print->NodeComment=TEXT("nodeforge:Print");},[&]{Print->NodeComment=SavedComment;},[&]{return Print->NodeComment.Equals(TEXT("nodeforge:Print"),ESearchCase::CaseSensitive);}))return Finish(false,TEXT("logical prefix guard"));
    if(!Mutate(TEXT("nativeLinkedOutputCase"),[&]{Then->PinName=FName(TEXT("Then"));},[&]{Then->PinName=SavedThen;},[&]{return Then->PinName.ToString().Equals(TEXT("Then"),ESearchCase::CaseSensitive);}))return Finish(false,TEXT("linked output case guard"));
    if(!Mutate(TEXT("declaredVariableDisplayCase"),[&]{Variable->VarName=FName(TEXT("message"));},[&]{Variable->VarName=SavedVariable;},[&]{return Variable->VarName.ToString().Equals(TEXT("message"),ESearchCase::CaseSensitive);}))return Finish(false,TEXT("variable display guard"));

    UBlueprint* Saved=LoadObject<UBlueprint>(nullptr,*Source);
    if(!Saved || !Saved->GetPathName().Equals(Source,ESearchCase::CaseSensitive) || Saved->GetPackage()->IsDirty())return Finish(false,TEXT("Clean saved exact source required"));
    const FString SourceFile=FPackageName::LongPackageNameToFilename(Saved->GetPackage()->GetName(),FPackageName::GetAssetPackageExtension());
    const FString SourceSha=NodeForgePrototype::FileSha256(SourceFile);
    const FString Target=TEXT("/Game/NodeForgeTests/NFStringGuardExportCopy");
    if(SourceSha.Len()!=64 || FPackageName::DoesPackageExist(Target) || FindPackage(nullptr,*Target))return Finish(false,TEXT("Source fingerprint and fresh export target required"));
    auto Config=MakeShared<FJsonObject>();Config->SetStringField(TEXT("source"),Source);Config->SetStringField(TEXT("requestId"),TEXT("StringGuardExport"));Config->SetStringField(TEXT("assetPath"),Target);
    NodeForgeBlueprintGraph::FObject Exported,ExportEvidence;
    auto Export=[&]{Error.Reset();return NodeForgeBlueprintGraph::Export(Config,Request->GetStringField(TEXT("catalogDigest")),Exported,ExportEvidence,Error);};
    if(!Row(TEXT("baselineSavedExport"),Export(),Error))return Finish(false,TEXT("baseline saved export: ")+Error);
    auto* SavedVariableDescription=Saved->NewVariables.FindByPredicate([](const FBPVariableDescription& V){return V.VarName.ToString().Equals(TEXT("Message"),ESearchCase::CaseSensitive);});
    if(!SavedVariableDescription || SavedVariableDescription->FriendlyName.IsEmpty())return Finish(false,TEXT("Nonempty actual FriendlyName required"));
    auto* Meta=SavedVariableDescription->MetaDataArray.FindByPredicate([](const FBPVariableMetaDataEntry& V){return V.DataKey.ToString().Equals(TEXT("MultiLine"),ESearchCase::CaseSensitive);});
    if(!Meta || !Meta->DataValue.Equals(TEXT("true"),ESearchCase::CaseSensitive))return Finish(false,TEXT("Native MultiLine=true metadata required"));
    const FString Friendly=SavedVariableDescription->FriendlyName,Metadata=Meta->DataValue;
    ON_SCOPE_EXIT{SavedVariableDescription->FriendlyName=Friendly;Meta->DataValue=Metadata;};
    const auto* FriendlyProperty=FindFProperty<FStrProperty>(FBPVariableDescription::StaticStruct(),TEXT("FriendlyName"));
    const FBPVariableDescription BaselineDescription=*SavedVariableDescription;
    auto ExportMutation=[&](const TCHAR* Name,TFunction<void()> Apply,TFunction<void()> Restore,TFunction<bool()> Changed)
    {
        Apply();const bool Actual=Changed();const bool Rejected=!Export() && !Error.IsEmpty();const FString Reason=Error;
        Restore();const bool Restored=Export();
        return Row(Name,Actual && Rejected && Restored,Rejected?Reason:TEXT("case-only metadata mutation was accepted"),Actual);
    };
    if(!ExportMutation(TEXT("savedFriendlyNameCase"),[&]{SavedVariableDescription->FriendlyName=Friendly.ToLower();},[&]{SavedVariableDescription->FriendlyName=Friendly;},[&]{
        const bool NativeEqual=FriendlyProperty && FriendlyProperty->Identical_InContainer(SavedVariableDescription,&BaselineDescription);
        Report->SetBoolField(TEXT("nativeFStrPropertyIdenticalIgnoresCaseObserved"),NativeEqual);
        return NativeEqual && !SavedVariableDescription->FriendlyName.Equals(Friendly,ESearchCase::CaseSensitive);}))return Finish(false,TEXT("saved FriendlyName case guard"));
    if(!ExportMutation(TEXT("savedMetadataValueCase"),[&]{Meta->DataValue=TEXT("TRUE");},[&]{Meta->DataValue=Metadata;},[&]{return Meta->DataValue.Equals(TEXT("TRUE"),ESearchCase::CaseSensitive);}))return Finish(false,TEXT("saved metadata case guard"));
    if(!Row(TEXT("finalSourceAndTargetProtection"),SourceSha.Equals(NodeForgePrototype::FileSha256(SourceFile),ESearchCase::CaseSensitive)
        && !Saved->GetPackage()->IsDirty() && !FPackageName::DoesPackageExist(Target) && !FindPackage(nullptr,*Target),TEXT("source bytes/clean state unchanged; export creates no target asset")))return Finish(false,TEXT("export source protection"));
    return Finish(true,TEXT("All native case-only mutations rejected and original state recovered"));
}
#endif
