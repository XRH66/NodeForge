// Project-owned isolated native fixture preparation and read-only protection test.
#include "CoreMinimal.h"
#include "NodeForgeMaterialGraph.h"
#include "NodeForgePrototype.h"
#include "NodeForgeExportFixtureChecks.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionClamp.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeMaterialExportFixtures,"NodeForge.MaterialExportFixtures",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNodeForgeMaterialExportFixtures::RunTest(const FString& Parameters)
{
    FString Source,ReportPath,SetupError;
    const TArray<const TCHAR*> Names={TEXT("Untagged"),TEXT("Changed"),TEXT("Unknown"),TEXT("Disconnected"),TEXT("Mask"),TEXT("RootConstant"),TEXT("OtherOutput"),TEXT("ClampMode"),TEXT("OtherSetting")};
    if(!FParse::Value(FCommandLine::Get(),TEXT("NFExportSource="),Source)
        || !FParse::Value(FCommandLine::Get(),TEXT("NFExportFixtureReport="),ReportPath)
        || !Source.StartsWith(TEXT("/Game/NodeForgeTests/"),ESearchCase::CaseSensitive))
    {AddError(TEXT("Own isolated source and fresh report required"));return false;}
    if(!NodeForgeExportFixtureChecks::ValidateSetup(ReportPath,Names,TEXT("NFExportNative"),SetupError))
    {AddError(SetupError);return false;}
    UMaterial* Original=LoadObject<UMaterial>(nullptr,*Source);
    if(!Original || !Original->GetPathName().Equals(Source,ESearchCase::CaseSensitive)
        || Original->GetPackage()->IsDirty()) {AddError(TEXT("Clean canonical copied source material required"));return false;}
    const FString File=FPackageName::LongPackageNameToFilename(Original->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension());
    const FString Before=NodeForgePrototype::FileSha256(File);
    TArray<TSharedPtr<FJsonValue>> Cases;
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
        const FString N=TEXT("NFExportNative")+FString(Name), P=TEXT("/Game/NodeForgeTests/")+N;
        if(FPackageName::DoesPackageExist(P) || FindPackage(nullptr,*P)) return Fail(TEXT("Fixture collision: ")+P);
        UPackage* Package=CreatePackage(*P);
        UMaterial* M=DuplicateObject<UMaterial>(Original,Package,*N);
        UMaterialExpressionScalarParameter* Rough=nullptr;
        UMaterialExpressionVectorParameter* Color=nullptr;
        for(UMaterialExpression* E:M->GetExpressions())
        {
            E->Desc=TEXT("Manually authored fixture, no NodeForge identity");
            if(auto* S=Cast<UMaterialExpressionScalarParameter>(E)) Rough=S;
            if(auto* V=Cast<UMaterialExpressionVectorParameter>(E)) Color=V;
        }
        if(!Rough || !Color)return Fail(TEXT("This fixture requires StaticColor's two parameters"));
        const FString Mode=Name;
        if(Mode==TEXT("Changed")) {Color->DefaultValue=FLinearColor(.7f,.1f,.3f,1);Rough->DefaultValue=.65f;Color->MaterialExpressionEditorX-=120;}
        if(Mode==TEXT("Unknown"))
        {
            // Rotator is supported. Do not silently keep a newly supported class
            // as an unsupported fixture when the bounded catalog grows.
            UClass* Class=LoadObject<UClass>(nullptr,TEXT("/Script/Engine.MaterialExpressionCustom"));
            UMaterialExpression* Expression=Class?UMaterialEditingLibrary::CreateMaterialExpression(M,Class):nullptr;
            TArray<UMaterialExpression*> Supported;FString Rejection;
            if(!Expression || !M->GetExpressions().Contains(Expression)
                || NodeForgeMaterialGraph::ExportExpressions(M,Supported,Rejection)
                || Rejection!=TEXT("MaterialGraph: unsupported source expression: ")+GetPathNameSafe(Expression))
                return Fail(TEXT("Unsupported fixture no longer matches the current native export guard"));
        }
        if(Mode==TEXT("Disconnected")) UMaterialEditingLibrary::CreateMaterialExpression(M,LoadObject<UClass>(nullptr,TEXT("/Script/Engine.MaterialExpressionConstant")));
        // Toggle a real channel bit. Setting Mask=1 was a no-op for the native
        // VectorParameter RGB output and is retained as a failed fixture history.
        if(Mode==TEXT("Mask")) M->GetEditorOnlyData()->BaseColor.MaskR^=1;
        if(Mode==TEXT("RootConstant")) M->GetEditorOnlyData()->Roughness.UseConstant=true;
        if(Mode==TEXT("OtherOutput")) Color->ConnectExpression(M->GetExpressionInputForProperty(MP_Refraction),0);
        if(Mode==TEXT("OtherSetting")) M->bUseMaterialAttributes=true;
        if(Mode==TEXT("ClampMode"))
        {
            auto* Clamp=Cast<UMaterialExpressionClamp>(UMaterialEditingLibrary::CreateMaterialExpression(M,UMaterialExpressionClamp::StaticClass(),100,100));
            if(!Clamp)return Fail(TEXT("Native Clamp creation failed"));
            Rough->ConnectExpression(&Clamp->Input,0);Clamp->ConnectExpression(M->GetExpressionInputForProperty(MP_Roughness),0);Clamp->ClampMode=CMODE_ClampMin;
        }
        M->SetFlags(RF_Public|RF_Standalone);FSavePackageArgs Save;Save.TopLevelFlags=RF_Public|RF_Standalone;Save.SaveFlags=SAVE_NoError;
        const FString Destination=FPackageName::LongPackageNameToFilename(P,FPackageName::GetAssetPackageExtension());
        if(!UPackage::SavePackage(Package,M,*Destination,Save))return Fail(TEXT("Native fixture save failed: ")+P);
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("case"),Name);Row->SetStringField(TEXT("source"),M->GetPathName());
        Row->SetStringField(TEXT("sourceAssetSha256"),NodeForgePrototype::FileSha256(Destination));
        Row->SetStringField(TEXT("expected"),(Mode==TEXT("Untagged") || Mode==TEXT("Changed"))?TEXT("passed"):TEXT("rejected"));
        Cases.Add(MakeShared<FJsonValueObject>(Row));
    }
    auto Config=MakeShared<FJsonObject>();Config->SetStringField(TEXT("source"),Source);Config->SetStringField(TEXT("requestId"),TEXT("GraphExportDirty"));
    Config->SetStringField(TEXT("assetPath"),TEXT("/Game/NodeForgeTests/NFExportDirtyTarget"));
    Original->GetPackage()->SetDirtyFlag(true);TSharedPtr<FJsonObject> Draft,Evidence;FString Error;
    const bool DirtyRejected=!NodeForgeMaterialGraph::Export(Config,TEXT("notUsedForDirtyRejection"),Draft,Evidence,Error) && !Draft && !Evidence && !Error.IsEmpty();
    const bool DirtyPreserved=Original->GetPackage()->IsDirty();
    Original->GetPackage()->SetDirtyFlag(false);
    const bool Unchanged=Before.Len()==64 && Before==NodeForgePrototype::FileSha256(File);
    auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("status"),DirtyRejected && DirtyPreserved && Unchanged?TEXT("passed"):TEXT("failed"));
    Report->SetArrayField(TEXT("cases"),Cases);Report->SetBoolField(TEXT("dirtySourceRejected"),DirtyRejected);Report->SetBoolField(TEXT("originalSourceBytesUnchanged"),Unchanged);
    Report->SetBoolField(TEXT("dirtySourceFlagPreservedDuringExport"),DirtyPreserved);
    Report->SetStringField(TEXT("scope"),TEXT("Saved isolated native fixture setup, including untagged/changed graph and unsupported configurations; export tested in fresh separate processes"));
    if(!NodeForgePrototype::WriteJson(Report,ReportPath)){AddError(TEXT("Fresh fixture report write failed"));return false;}
    if(!DirtyRejected || !DirtyPreserved || !Unchanged) AddError(TEXT("Dirty/source protection failed"));return !HasAnyErrors();
}
#endif
