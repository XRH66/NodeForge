// Finite native checks, scheduled inside the one B3 editor. No saved fixture assets.
#include "NodeForgeEditorControlService.h"
#include "NodeForgeEditorControlProtocol.h"
#include "NodeForgePrototype.h"
#include "NodeForgeResourceSource.h"
#include "NodeForgeMaterialGraph.h"
#include "NodeForgeBlueprintGraph.h"
#include "NodeForgeGraphCodec.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformProcess.h"
#include "Modules/ModuleManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "MaterialEditingLibrary.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_Knot.h"
#include "UObject/Package.h"
#include "Misc/ScopeExit.h"
#include "Serialization/JsonSerializer.h"

namespace
{
bool ReadCheckJson(const FString& Path,TSharedPtr<FJsonObject>& Json)
{
    FString Text,Hash,Error,Normalized=FPaths::ConvertRelativePathToFull(Path);FPaths::NormalizeFilename(Normalized);
    if(!FPaths::IsUnderDirectory(Normalized,NodeForgePrototype::WorkspaceDirectory()/TEXT("Reports"))
        || !NodeForgeResourceSource::ReadText(Normalized,NodeForgeEditorControl::MaxReportBytes,Text,Hash,Error))return false;
    FTCHARToUTF8 Utf8(*Text);
    return NodeForgeEditorControl::ParseObject(MakeArrayView(reinterpret_cast<const uint8*>(Utf8.Get()),Utf8.Length()),Json,NodeForgeEditorControl::MaxReportBytes);
}
bool CheckUnsupportedMaterialExpression(FAutomationTestBase& Test,TSharedPtr<FJsonObject> Observations=nullptr)
{
    // Rotator is supported by the current Specs. Custom is a real UE5.3.2
    // expression outside the bounded catalog; exercise the production guard.
    UMaterial* Material=NewObject<UMaterial>(GetTransientPackage(),NAME_None,RF_Transient);
    ON_SCOPE_EXIT {Material->MarkAsGarbage();};
    UClass* Class=LoadObject<UClass>(nullptr,TEXT("/Script/Engine.MaterialExpressionCustom"));
    UMaterialExpression* Expression=Class?UMaterialEditingLibrary::CreateMaterialExpression(Material,Class):nullptr;
    TArray<UMaterialExpression*> Expressions;FString Error;
    const bool Registered=Expression && Material->GetExpressions().Contains(Expression);
    const bool ExactClass=Expression && Expression->GetClass()->GetPathName()==TEXT("/Script/Engine.MaterialExpressionCustom");
    const bool Rejected=Expression && !NodeForgeMaterialGraph::ExportExpressions(Material,Expressions,Error);
    const FString ExpectedError=TEXT("MaterialGraph: unsupported source expression: ")+GetPathNameSafe(Expression);
    const bool Passed=Registered && ExactClass && Rejected && Error==ExpectedError;
    if(Observations)
    {
        Observations->SetBoolField(TEXT("expressionRegistered"),Registered);
        Observations->SetBoolField(TEXT("exactNativeClass"),ExactClass);
        Observations->SetBoolField(TEXT("productionExporterRejected"),Rejected);
        Observations->SetStringField(TEXT("actualError"),Error);
        Observations->SetStringField(TEXT("expectedError"),ExpectedError);
        Observations->SetNumberField(TEXT("actualExpressionCount"),Material->GetExpressions().Num());
    }
    Test.AddInfo(TEXT("Unsupported native expression guard returned: ")+Error);
    Test.TestTrue(TEXT("MaterialActualUnsupportedExpressionGuard"),Passed);
    return Passed;
}
bool RunAssetReadNative(FAutomationTestBase& Test,const FString& InputsPath,const FString& Output)
{
    TSharedPtr<FJsonObject> Inputs,Materials;
    if(!ReadCheckJson(InputsPath,Inputs) || !ReadCheckJson(NodeForgePrototype::WorkspaceDirectory()/TEXT("Reports/PluginControlStageTwo/AssetRead1/B0/Materials.json"),Materials))
    {Test.AddError(TEXT("Exact prepared native inputs and B0 materials required"));return false;}
    TArray<TSharedPtr<FJsonValue>> Rows;bool All=true;
    auto Row=[&](const FString& Name,bool Passed)
    {Test.TestTrue(Name,Passed);All &= Passed;auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("case"),Name);R->SetBoolField(TEXT("passed"),Passed);Rows.Add(MakeShared<FJsonValueObject>(R));};
    const auto& Sources=Materials->GetArrayField(TEXT("sources"));
    {
        const FString Large=FString::ChrN(600000,' ')+TEXT("{}");FTCHARToUTF8 Utf8(*Large);TSharedPtr<FJsonObject> Parsed;
        const auto Bytes=MakeArrayView(reinterpret_cast<const uint8*>(Utf8.Get()),Utf8.Length());
        Row(TEXT("ReportLimitOneMiBAndOriginalFrameLimitPreserved"),NodeForgeEditorControl::ParseObject(Bytes,Parsed,NodeForgeEditorControl::MaxReportBytes)
            && !NodeForgeEditorControl::ParseObject(Bytes,Parsed));
    }
    if(Sources.Num()!=3){Test.AddError(TEXT("Frozen three-source material plan changed"));return false;}
    const auto MatPlan=Sources[0]->AsObject(),BPPlan=Sources[1]->AsObject(),FnPlan=Sources[2]->AsObject();
    UMaterial* Material=FindObject<UMaterial>(nullptr,*MatPlan->GetStringField(TEXT("source")));
    UBlueprint* Blueprint=FindObject<UBlueprint>(nullptr,*BPPlan->GetStringField(TEXT("source")));
    UMaterialFunction* Function=FindObject<UMaterialFunction>(nullptr,*FnPlan->GetStringField(TEXT("source")));
    Row(TEXT("FourPositiveReadsPreparedSourcesBeforeNativeCheck"),Material && Blueprint && Function);
    if(!Material || !Blueprint || !Function)return false;
    auto Observations=MakeShared<FJsonObject>();auto MatObserved=MakeShared<FJsonObject>();
    TArray<FMaterialParameterInfo> ScalarInfos,VectorInfos,TextureInfos;TArray<FGuid> Guids;
    Material->GetAllScalarParameterInfo(ScalarInfos,Guids);Guids.Reset();Material->GetAllVectorParameterInfo(VectorInfos,Guids);
    Guids.Reset();Material->GetAllTextureParameterInfo(TextureInfos,Guids);
    Row(TEXT("IndependentNativeParameterCounts"),ScalarInfos.Num()==1 && VectorInfos.Num()==1 && TextureInfos.IsEmpty());
    float Roughness=0;FLinearColor Color;
    const bool Rough=Material->GetScalarParameterValue(FHashedMaterialParameterInfo(FMaterialParameterInfo(FName(TEXT("SurfaceRoughness")))),Roughness);
    const bool Vector=Material->GetVectorParameterValue(FHashedMaterialParameterInfo(FMaterialParameterInfo(FName(TEXT("SurfaceColor")))),Color);
    Row(TEXT("IndependentNativeParameterDefaults"),Rough && Vector && FMath::IsNearlyEqual(Roughness,.3f,1.e-6f)
        && FMath::IsNearlyEqual(Color.R,.05f,1.e-6f) && FMath::IsNearlyEqual(Color.G,.55f,1.e-6f)
        && FMath::IsNearlyEqual(Color.B,.8f,1.e-6f) && FMath::IsNearlyEqual(Color.A,1.f,1.e-6f));
    MatObserved->SetNumberField(TEXT("SurfaceRoughness"),Roughness);MatObserved->SetArrayField(TEXT("SurfaceColor"),
        {MakeShared<FJsonValueNumber>(Color.R),MakeShared<FJsonValueNumber>(Color.G),MakeShared<FJsonValueNumber>(Color.B),MakeShared<FJsonValueNumber>(Color.A)});
    Observations->SetObjectField(TEXT("material"),MatObserved);
    // Native function definition, not NodeForge Inspect, Signature or nodeTemplate.
    TArray<FFunctionExpressionInput> FnInputs;TArray<FFunctionExpressionOutput> FnOutputs;
    Function->GetInputsAndOutputs(FnInputs,FnOutputs);
    const auto Fixed=FnPlan->GetObjectField(TEXT("fixedSignature"));const auto& ExpectedInputs=Fixed->GetArrayField(TEXT("inputs"));
    const auto& ExpectedOutputs=Fixed->GetArrayField(TEXT("outputs"));TArray<TSharedPtr<FJsonValue>> FnObserved;
    bool Signature=FnInputs.Num()==ExpectedInputs.Num() && FnOutputs.Num()==ExpectedOutputs.Num();
    for(int32 I=0;Signature && I<FnInputs.Num();++I)
    {
        const auto& Native=FnInputs[I];const auto Wanted=ExpectedInputs[I]->AsObject();auto* E=Native.ExpressionInput.Get();
        const FString Display=Wanted->GetStringField(TEXT("name"));const auto& Values=Wanted->GetArrayField(TEXT("nativePreviewValue"));
        Signature=E && Display.EndsWith(TEXT(" (S)")) && E->InputType==FunctionInput_Scalar
            && Native.Input.InputName.ToString()==Display.LeftChop(4) && Native.ExpressionInputId.ToString(EGuidFormats::DigitsWithHyphens)==Wanted->GetStringField(TEXT("guid"))
            && E->GetInputType(0)==Wanted->GetNumberField(TEXT("nativeType"))
            && bool(E->bUsePreviewValueAsDefault)==Wanted->GetBoolField(TEXT("usesNativePreviewDefault"))
            && !bool(E->bUsePreviewValueAsDefault)==Wanted->GetBoolField(TEXT("required")) && Values.Num()==4;
        if(Signature)
            for(int32 C=0;C<4;++C)Signature &= FMath::IsNearlyEqual(double(E->PreviewValue[C]),Values[C]->AsNumber(),1.e-6);
        auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("rawName"),Native.Input.InputName.ToString());
        R->SetStringField(TEXT("guid"),Native.ExpressionInputId.ToString(EGuidFormats::DigitsWithHyphens));FnObserved.Add(MakeShared<FJsonValueObject>(R));
    }
    for(int32 I=0;Signature && I<FnOutputs.Num();++I)
    {const auto Wanted=ExpectedOutputs[I]->AsObject();Signature=FnOutputs[I].ExpressionOutput && FnOutputs[I].Output.OutputName.ToString()==Wanted->GetStringField(TEXT("name"))
        && FnOutputs[I].ExpressionOutputId.ToString(EGuidFormats::DigitsWithHyphens)==Wanted->GetStringField(TEXT("guid")) && FString::FromInt(I)==Wanted->GetStringField(TEXT("index"));}
    Row(TEXT("IndependentNativeFunctionSignature"),Signature);Observations->SetArrayField(TEXT("functionInputs"),FnObserved);

    {
        NodeForgeResourceSource::FCapture WrongClass,Outside;
        FString Reason;
        Row(TEXT("WrongExplicitClassRejected"),!WrongClass.Begin(MatPlan->GetStringField(TEXT("source")),TEXT("MaterialFunction"),Reason)
            && Reason==TEXT("sourceClassOrRedirectRejected"));
        Row(TEXT("OutsideAllowedRootRejectedBeforeLoad"),!Outside.Begin(TEXT("/Game/Outside.Example"),TEXT("Material"),Reason)
            && Reason==TEXT("sourcePathRejected"));
    }
    {
        const FString Source=Inputs->GetStringField(TEXT("memoryUnprovenSource"));
        const FString File=FPackageName::LongPackageNameToFilename(FPackageName::ObjectPathToPackageName(Source),FPackageName::GetAssetPackageExtension());
        const FString Before=NodeForgePrototype::FileSha256(File);FString Reason;
        UMaterial* Unproven=Source.StartsWith(TEXT("/Game/NodeForgeTests/Material/"))?LoadObject<UMaterial>(nullptr,*Source):nullptr;
        NodeForgeResourceSource::FCapture Capture;
        Row(TEXT("PreloadedSavedSourceWithoutControlledOriginRejected"),Unproven && Before==Inputs->GetStringField(TEXT("memoryUnprovenSha256"))
            && !Unproven->GetOutermost()->IsDirty() && !Capture.Begin(Source,TEXT("Material"),Reason) && Reason==TEXT("sourceMemoryUnproven")
            && NodeForgePrototype::FileSha256(File)==Before && !Unproven->GetOutermost()->IsDirty());
    }

    const FString Unique=FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(10);FString Error;
    for(const FString Mode:{FString(TEXT("MaterialParameters")),FString(TEXT("MaterialFunction")),FString(TEXT("MaterialGraphExport")),FString(TEXT("BlueprintGraphExport"))})
    {
        const FString Name=TEXT("Dirty")+Mode.Left(12)+Unique;
        const FString PackagePath=TEXT("/Game/NodeForgeTests/Boundary/")+Name;
        if(FindPackage(nullptr,*PackagePath) || FPackageName::DoesPackageExist(PackagePath)){Row(TEXT("DirtyFixtureCollision"),false);break;}
        UPackage* Package=CreatePackage(*PackagePath);
        UClass* Class=Mode==TEXT("MaterialFunction")?UMaterialFunction::StaticClass():Mode==TEXT("BlueprintGraphExport")?UBlueprint::StaticClass():UMaterial::StaticClass();
        UObject* Own=NewObject<UObject>(Package,Class,FName(*Name),RF_Transient);Package->SetDirtyFlag(true);
        ON_SCOPE_EXIT {Own->MarkAsGarbage();Package->MarkAsGarbage();};
        auto Args=MakeShared<FJsonObject>();Args->SetStringField(TEXT("source"),Own->GetPathName());
        const bool Export=Mode.EndsWith(TEXT("GraphExport"));
        Args->SetStringField(Export?TEXT("assetKind"):TEXT("kind"),Export?(Mode==TEXT("BlueprintGraphExport")?TEXT("Blueprint"):TEXT("Material")):*Mode);
        if(Export){Args->SetStringField(TEXT("requestId"),Name);Args->SetStringField(TEXT("assetPath"),TEXT("/Game/NodeForgeTests/Boundary/Out")+Unique+Mode.Left(2));}
        const FString Stem=FPaths::GetPath(Output)/(Mode+Unique);FString Digest;
        const int32 Code=FNodeForgeEditorControlService::ExecuteReadAt(Export?TEXT("graphs.export"):TEXT("resources.inspect"),Args,Stem+TEXT("Input.json"),Stem+TEXT("Report.json"),Digest,Error);
        TSharedPtr<FJsonObject> Native;
        Row(Mode+TEXT("ActualDirtyGuardAndResultMapping"),Code==2 && ReadCheckJson(Stem+TEXT("Report.json"),Native)
            && Native->GetStringField(TEXT("nextAction"))==TEXT("sourceDirty") && Native->GetStringField(TEXT("overallStatus"))==TEXT("rejected")
            && Package->IsDirty() && !Native->HasField(TEXT("exportedRequest")));
    }
    Row(TEXT("MaterialActualUnsupportedExpressionGuard"),CheckUnsupportedMaterialExpression(Test));
    UEdGraph* Graph=NewObject<UEdGraph>(GetTransientPackage());UK2Node_Knot* Knot=NewObject<UK2Node_Knot>(Graph);
    ON_SCOPE_EXIT {Knot->MarkAsGarbage();Graph->MarkAsGarbage();};
    const auto BlueprintRequest=BPPlan->GetObjectField(TEXT("independentExpectedGraph"));auto Negative=MakeShared<FJsonObject>();Negative->Values=BlueprintRequest->Values;
    auto Nodes=BlueprintRequest->GetArrayField(TEXT("nodes"));auto Node=MakeShared<FJsonObject>();Node->SetStringField(TEXT("id"),TEXT("UnsupportedKnot"));
    Node->SetStringField(TEXT("class"),Knot->GetClass()->GetPathName());Node->SetObjectField(TEXT("properties"),MakeShared<FJsonObject>());
    Nodes.Add(MakeShared<FJsonValueObject>(Node));Negative->SetArrayField(TEXT("nodes"),Nodes);
    NodeForgeGraphDocument::FGraphDocument NegativeDocument;
    Row(TEXT("BlueprintActualUnsupportedShapeGuard"),!NodeForgeGraphCodec::Decode(Negative,NegativeDocument,Error) && Error.Contains(TEXT("Unsupported blueprint node class")));

    for(const FString Method:{FString(TEXT("resources.inspect")),FString(TEXT("graphs.export"))})
    {
        FNodeForgeEditorControlService Isolated;auto Args=MakeShared<FJsonObject>();
        const FString Session=TEXT("0123456789abcdef0123456789abcdef");
        Isolated.BusinessGate()->ReserveControl();
        auto Result=Isolated.ExecuteQueryCall(Method,Args,MAX_uint64,Session,TEXT("NativeStale"),FPaths::GetPath(Output));Isolated.BusinessGate()->EndControl();
        Row(Method+TEXT("StaleRevisionDoesNotDispatch"),Result->GetStringField(TEXT("code"))==TEXT("staleRevision") && !Result->HasField(TEXT("nativeReport")));
        {FNodeForgeEditorBusinessScope Busy(Isolated);Result=Isolated.ExecuteQueryCall(Method,Args,0,Session,TEXT("NativeBusy"),FPaths::GetPath(Output));
        Row(Method+TEXT("BusyDoesNotDispatch"),Result->GetStringField(TEXT("code"))==TEXT("busy") && !Result->HasField(TEXT("nativeReport")));}
    }
    TSharedPtr<FJsonObject> MatExport;FString ExportPath=Inputs->GetStringField(TEXT("materialExport"));FPaths::NormalizeFilename(ExportPath);
    if(!ReadCheckJson(ExportPath,MatExport)){Row(TEXT("ExactMcpExportReportReadable"),false);return false;}
    {
        int32 Code=0;FString Reason;
        auto Incomplete=MakeShared<FJsonObject>();Incomplete->Values=MatExport->Values;Incomplete->RemoveField(TEXT("exportedRequestSha256"));
        Row(TEXT("MissingExportPayloadHashRemainsUnknown"),!FNodeForgeEditorControlService::ValidateQueryReport(TEXT("MaterialGraphExport"),0,Incomplete,Code,Reason) && Code==5);
        auto MissingRead=MakeShared<FJsonObject>();MissingRead->Values=MatExport->Values;
        auto Read=MakeShared<FJsonObject>();Read->Values=MatExport->GetObjectField(TEXT("resourceRead"))->Values;Read->RemoveField(TEXT("configurationDigest"));MissingRead->SetObjectField(TEXT("resourceRead"),Read);
        Row(TEXT("MissingConfigurationIdentityRemainsUnknown"),!FNodeForgeEditorControlService::ValidateQueryReport(TEXT("MaterialGraphExport"),0,MissingRead,Code,Reason) && Code==5);
        TSharedPtr<FJsonObject> FnReport;
        if(!ReadCheckJson(Inputs->GetStringField(TEXT("functionReport")),FnReport)){Row(TEXT("OriginalCliFunctionReportAvailable"),false);return false;}
        FnReport->RemoveField(TEXT("nodeTemplate"));
        Row(TEXT("MissingFunctionTemplateRemainsUnknown"),!FNodeForgeEditorControlService::ValidateQueryReport(TEXT("MaterialFunction"),0,FnReport,Code,Reason) && Code==5);
    }
    FNodeForgeEditorControlService Draft;const FString Text=MatExport->GetStringField(TEXT("exportedRequestText"));const FString ReportHash=NodeForgePrototype::FileSha256(ExportPath);
    {
        FNodeForgeEditorBusinessScope UI(Draft);const uint64 Revision=Draft.State().Input.Revision();
        Row(TEXT("SameRevisionUiExportCommitsVerifiedOrigin"),Draft.CommitExportFromUI(Text,ExportPath,ReportHash,Revision,Error)
            && Draft.State().Input.Draft().ExportOrigin && Draft.State().LastExportReportPath==ExportPath);
        Draft.MutableState().Input.Edit(Text+TEXT(" "));
        Row(TEXT("LateUiExportDoesNotReplaceNewDraft"),!Draft.CommitExportFromUI(Text,ExportPath,ReportHash,Revision,Error)
            && Error==TEXT("staleRevision") && Draft.State().Input.Draft().Text==Text+TEXT(" "));
    }
    Row(TEXT("SameSourceEditKeepsAccurateBaseline"),Draft.State().Input.Draft().ExportOrigin && Draft.State().LastExportReportPath==ExportPath);
    Row(TEXT("UnrelatedReplaceClearsOriginAndBaseline"),Draft.ReplaceDraft(TEXT("{}"),TEXT("program"),FString(),FString(),Error)
        && !Draft.State().Input.Draft().ExportOrigin && Draft.State().LastExportReportPath.IsEmpty());
    Row(TEXT("RestoreRecoversMatchingOrigin"),Draft.RestoreDraft(Error) && Draft.State().Input.Draft().ExportOrigin && Draft.State().LastExportReportPath==ExportPath);
    TSharedPtr<FJsonObject> RejectedOrigin;
    Row(TEXT("BadReportHashRejectedWithoutRewrite"),!NodeForgeResourceSource::ReadExportOrigin(ExportPath,FString::ChrN(64,'0'),Text,true,RejectedOrigin,Error)
        && NodeForgePrototype::FileSha256(ExportPath)==ReportHash);
    for(const auto& Source:Sources)
    {
        const auto Plan=Source->AsObject();FString File=Plan->GetStringField(TEXT("file"));
        if(FPaths::IsRelative(File))File=NodeForgePrototype::WorkspaceDirectory()/File;
        Row(Plan->GetStringField(TEXT("kind"))+TEXT("OriginalBytesPreserved"),NodeForgePrototype::FileSha256(File)==Plan->GetStringField(TEXT("sha256")));
    }
    Row(TEXT("OriginalSourcePackagesRemainClean"),!Material->GetOutermost()->IsDirty() && !Blueprint->GetOutermost()->IsDirty() && !Function->GetOutermost()->IsDirty());
    auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("status"),All?TEXT("passed"):TEXT("failed"));Report->SetArrayField(TEXT("cases"),Rows);
    Report->SetObjectField(TEXT("independentObservations"),Observations);Report->SetBoolField(TEXT("savedFixtureAssetsCreated"),false);
    Report->SetStringField(TEXT("scope"),TEXT("Native production dirty/unsupported guards plus wiring/result mapping; not dirty or unsupported saved-source CLI end-to-end. UI state checks are native service checks, not actual button clicks. No source compiled or saved."));
    if(!NodeForgePrototype::WriteJson(Report,Output)){Test.AddError(TEXT("New native report write failed"));return false;}
    return All;
}
class FWaitAssetReadInputs : public IAutomationLatentCommand
{
public:
    FWaitAssetReadInputs(FAutomationTestBase& In,const FString& Input,const FString& Report):Test(In),Inputs(Input),Output(Report),Deadline(FPlatformTime::Seconds()+600){}
    bool Update() override
    {
        if(IFileManager::Get().FileExists(*Inputs)){RunAssetReadNative(Test,Inputs,Output);return true;}
        if(FPlatformTime::Seconds()<Deadline)return false;
        Test.AddError(TEXT("Prepared AssetRead inputs did not arrive; no native business started"));
        auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("status"),TEXT("notRun"));Report->SetStringField(TEXT("reason"),TEXT("preparedInputsTimeout"));
        NodeForgePrototype::WriteJson(Report,Output);return true;
    }
private:FAutomationTestBase& Test;FString Inputs,Output;double Deadline;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeAssetReadNativeTest,"NodeForge.AssetReadNative",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNodeForgeAssetReadNativeTest::RunTest(const FString& Parameters)
{
    FString Inputs,Output;const FString Root=NodeForgePrototype::WorkspaceDirectory();
    if(!FParse::Value(FCommandLine::Get(),TEXT("NFAssetReadCheckInputs="),Inputs) || !FParse::Value(FCommandLine::Get(),TEXT("NFAssetReadCheckReport="),Output)
        || !FPaths::IsUnderDirectory(Inputs,Root/TEXT("Reports/PluginControlStageTwo/AssetRead1"))
        || !FPaths::IsUnderDirectory(Output,Root/TEXT("Reports/PluginControlStageTwo/AssetRead1")) || IFileManager::Get().FileExists(*Output)
        || FPaths::GetBaseFilename(FPaths::GetProjectFilePath())!=TEXT("NFCUiApiLab"))
    {AddError(TEXT("Explicit finite B3 inputs/report and reused NFCUiApiLab required"));return false;}
    if(FParse::Param(FCommandLine::Get(),TEXT("NFAssetReadUnsupportedOnly")))
    {
        auto Observations=MakeShared<FJsonObject>();
        const bool Passed=CheckUnsupportedMaterialExpression(*this,Observations);
        auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("status"),Passed?TEXT("passed"):TEXT("failed"));
        Report->SetStringField(TEXT("case"),TEXT("MaterialActualUnsupportedExpressionGuard"));
        Report->SetStringField(TEXT("expressionClass"),TEXT("/Script/Engine.MaterialExpressionCustom"));
        Report->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());
        Report->SetStringField(TEXT("moduleSha256"),NodeForgePrototype::FileSha256(FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"))));
        Report->SetBoolField(TEXT("savedFixtureAssetsCreated"),false);
        Report->SetObjectField(TEXT("observations"),Observations);
        Report->SetStringField(TEXT("scope"),TEXT("Only the corrected unsupported-expression oracle; other passed checks require explicit source-continuity review."));
        return NodeForgePrototype::WriteJson(Report,Output) && Passed;
    }
    AddCommand(new FWaitAssetReadInputs(*this,Inputs,Output));return true;
}
#endif
