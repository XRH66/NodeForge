// Project-owned fixed visual fixture. No third-party implementation is copied.
// Saved actors/materials are native Engine assets; this Editor helper is never a runtime dependency.
#include "NodeForgeVisualChecks.h"
#include "NodeForgePrototype.h"
#include "NodeForgeResourceSource.h"
#include "NodeForgeEditorControlProtocol.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetCompilingManager.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "ImageUtils.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "MaterialShared.h"
#include "MaterialDomain.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/ScopeExit.h"
#include "RenderingThread.h"
#include "ShaderCompiler.h"
#include "DynamicRHI.h"
#include "HAL/PlatformProcess.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

namespace NodeForgeVisualChecks
{
static const FString Folder=TEXT("/Game/NodeForgeTests/");
static const FString Map=Folder+TEXT("NFVisualGallery");
static const TCHAR* Materials[]={TEXT("NFUiMaterialPaste"),TEXT("NFReferenceBright"),TEXT("NFUiMaterialFileDark"),TEXT("NFReferenceDark")};
static const TCHAR* Labels[]={TEXT("GeneratedBright"),TEXT("ReferenceBright"),TEXT("GeneratedDark"),TEXT("ReferenceDark")};
static bool SaveNew(UObject* Asset)
{
    const FString Target=Asset->GetOutermost()->GetName();
    const FString File=FPackageName::LongPackageNameToFilename(Target,FPackageName::GetAssetPackageExtension());
    if (FPackageName::DoesPackageExist(Target) || IFileManager::Get().FileExists(*File)) return false;
    FAssetRegistryModule::AssetCreated(Asset);
    FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone;
    return UPackage::SavePackage(Asset->GetOutermost(),Asset,*File,Args);
}
static UMaterial* Reference(const FString& Name,float Value)
{
    const FString Path=Folder+Name;
    if (FPackageName::DoesPackageExist(Path))
    {
        // A failed prior gallery save may leave these project-owned references.
        // Reuse only after exact structural readback; never mutate or overwrite them.
        auto* Existing=LoadObject<UMaterial>(nullptr,*(Path+TEXT(".")+Name));
        if (!Existing || Existing->MaterialDomain!=MD_Surface || Existing->BlendMode!=BLEND_Opaque ||
            !Existing->GetShadingModels().HasShadingModel(MSM_Unlit) || Existing->GetExpressions().Num()!=1) return nullptr;
        auto* Constant=Cast<UMaterialExpressionConstant>(Existing->GetExpressions()[0]);
        if (!Constant || !FMath::IsNearlyEqual(Constant->R,Value) ||
            Existing->GetEditorOnlyData()->EmissiveColor.Expression!=Constant) return nullptr;
        return Existing;
    }
    if (FindPackage(nullptr,*Path)) return nullptr;
    auto* M=NewObject<UMaterial>(CreatePackage(*Path),*Name,RF_Public|RF_Standalone|RF_Transactional);
    M->MaterialDomain=MD_Surface; M->BlendMode=BLEND_Opaque; M->SetShadingModel(MSM_Unlit);
    auto* C=Cast<UMaterialExpressionConstant>(UMaterialEditingLibrary::CreateMaterialExpression(M,UMaterialExpressionConstant::StaticClass(),-200,0));
    if (!C) return nullptr;
    C->R=Value;
    if (!UMaterialEditingLibrary::ConnectMaterialProperty(C,TEXT(""),MP_EmissiveColor)) return nullptr;
    M->PostEditChange(); M->CacheShaders(EMaterialShaderPrecompileMode::Default);
    if (auto* R=M->GetMaterialResource(GMaxRHIFeatureLevel,EMaterialQualityLevel::High)) R->FinishCompilation();
    return SaveNew(M)?M:nullptr;
}
// Internal delivery fixture only: one transient mesh, fixed geometry/camera/ROIs.
// It neither authors a material nor exposes a general scene API through Operations.
static int32 CheckCandidate(const FString& InputPath,const FString& ReportPath)
{
    auto R=MakeShared<FJsonObject>(); R->SetStringField(TEXT("mode"),TEXT("CandidateVisualCheck"));
    R->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());
    R->SetStringField(TEXT("project"),FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()));
    R->SetStringField(TEXT("binarySha256"),NodeForgePrototype::FileSha256(FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"))));
    R->SetBoolField(TEXT("assetSaveAttempted"),false);
    auto Finish=[&](bool Good,const FString& Reason)
    {
        R->SetStringField(TEXT("result"),Good?TEXT("passed"):TEXT("failed")); R->SetStringField(TEXT("reason"),Reason);
        return NodeForgePrototype::WriteJson(R,ReportPath)?(Good?0:1):3;
    };
    FString Text,InputHash,Error,Source,Sha;TSharedPtr<FJsonObject> Config;
    if(!GEditor || !NodeForgePrototype::ReadInputFile(InputPath,Text,InputHash,Error))return Finish(false,TEXT("Dedicated editor and bounded fixture input required"));
    if(!GDynamicRHI || FString(GDynamicRHI->GetName())!=TEXT("D3D11") || GMaxRHIFeatureLevel!=ERHIFeatureLevel::SM5)
        return Finish(false,TEXT("Frozen visual fixture requires actual D3D11/SM5"));
    R->SetStringField(TEXT("actualRhi"),GDynamicRHI->GetName());R->SetStringField(TEXT("featureLevel"),TEXT("SM5"));
    FTCHARToUTF8 Utf8(*Text);
    if(!NodeForgeEditorControl::ParseObject(MakeArrayView(reinterpret_cast<const uint8*>(Utf8.Get()),Utf8.Length()),Config,16384)
        || Config->Values.Num()!=2 || !Config->TryGetStringField(TEXT("material"),Source)
        || !Config->TryGetStringField(TEXT("materialSha256"),Sha) || Sha.Len()!=64)
        return Finish(false,TEXT("Fixture requires exact material object path and saved-file SHA"));
    NodeForgeResourceSource::FCapture MaterialCapture;
    if(!MaterialCapture.Begin(Source,TEXT("Material"),Error,1,false) || MaterialCapture.Sha256()!=Sha)
        return Finish(false,Error.IsEmpty()?TEXT("Fixture material saved bytes changed"):Error);
    auto* Material=Cast<UMaterial>(MaterialCapture.Object());
    if(!Material || Material->MaterialDomain!=MD_Surface || Material->BlendMode!=BLEND_Masked
        || !Material->GetShadingModels().HasShadingModel(MSM_Unlit) || Material->OpacityMaskClipValue!=0.5f
        || !Material->TwoSided || Material->DitherOpacityMask || Material->DitheredLODTransition || Material->bAllowNegativeEmissiveColor)
        return Finish(false,TEXT("Fixed fixture requires Surface/Masked/Unlit/clip0.5"));
    R->SetStringField(TEXT("material"),Source);R->SetStringField(TEXT("materialSha256"),Sha);
    R->SetStringField(TEXT("configurationDigest"),InputHash);
    auto* PlaneMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
    if(!PlaneMesh)return Finish(false,TEXT("Installed native Plane required"));
    UWorld* World=GEditor->NewMap();
    if(!World || !World->Scene)return Finish(false,TEXT("Transient editor scene unavailable"));
    auto* Plane=World->SpawnActor<AStaticMeshActor>(FVector::ZeroVector,FRotator::ZeroRotator);
    if(!Plane || !Plane->GetStaticMeshComponent())return Finish(false,TEXT("Transient plane spawn failed"));
    Plane->SetActorScale3D(FVector(5));Plane->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);
    Plane->GetStaticMeshComponent()->SetMaterial(0,Material);
    ASceneCapture2D* Capture=nullptr;UTextureRenderTarget2D* RT=nullptr;
    ON_SCOPE_EXIT
    {
        if(IsValid(Capture))World->DestroyActor(Capture);
        if(IsValid(Plane))World->DestroyActor(Plane);
        if(RT && RT->IsRooted())RT->RemoveFromRoot();
    };
    Material->CacheShaders(EMaterialShaderPrecompileMode::Default);
    const double Deadline=FPlatformTime::Seconds()+120.0;
    while((GShaderCompilingManager && GShaderCompilingManager->IsCompiling())
        || FAssetCompilingManager::Get().GetNumRemainingAssets()>0)
    {
        if(FPlatformTime::Seconds()>=Deadline)return Finish(false,TEXT("Shader/mesh preparation timed out; no pixels treated as valid"));
        if(GShaderCompilingManager)GShaderCompilingManager->ProcessAsyncResults(true,false);
        FAssetCompilingManager::Get().ProcessAsyncTasks(true);FPlatformProcess::Sleep(0.01f);
    }
    auto* Resource=Material->GetMaterialResource(GMaxRHIFeatureLevel,EMaterialQualityLevel::High);
    if(!Resource || !Resource->IsCompilationFinished() || !Resource->GetCompileErrors().IsEmpty()
        || PlaneMesh->IsCompiling() || !PlaneMesh->HasValidRenderData())return Finish(false,TEXT("Actual material shaders and mesh must be ready"));
    R->SetBoolField(TEXT("shaderCompilationFinished"),true);
    World->UpdateWorldComponents(false,false);Plane->GetStaticMeshComponent()->MarkRenderStateDirty();
    RT=NewObject<UTextureRenderTarget2D>();if(!RT)return Finish(false,TEXT("Transient render target unavailable"));
    RT->AddToRoot();RT->ClearColor=FLinearColor::Black;RT->InitCustomFormat(256,256,PF_FloatRGBA,true);RT->UpdateResourceImmediate(true);
    Capture=World->SpawnActor<ASceneCapture2D>(FVector(0,0,1000),FRotator(-90,-90,0));
    if(!Capture || !Capture->GetCaptureComponent2D())return Finish(false,TEXT("Transient capture unavailable"));
    auto* C=Capture->GetCaptureComponent2D();C->TextureTarget=RT;C->ProjectionType=ECameraProjectionMode::Orthographic;C->OrthoWidth=600;
    C->bCaptureEveryFrame=false;C->bCaptureOnMovement=false;C->CaptureSource=SCS_SceneColorHDRNoAlpha;
    C->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;C->ShowOnlyActors.Add(Plane);
    C->ShowFlags.SetTonemapper(false);C->ShowFlags.SetEyeAdaptation(false);C->ShowFlags.SetBloom(false);
    C->ShowFlags.SetAntiAliasing(false);C->ShowFlags.SetMotionBlur(false);
    C->PostProcessSettings.bOverride_AutoExposureMethod=true;C->PostProcessSettings.AutoExposureMethod=AEM_Manual;
    C->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;C->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure=false;
    World->SendAllEndOfFrameUpdates();FlushRenderingCommands();
    if(!Plane->GetStaticMeshComponent()->SceneProxy || !C->IsRegistered())return Finish(false,TEXT("Actual scene proxy/capture registration required"));
    C->CaptureScene();FlushRenderingCommands();TArray<FLinearColor> Pixels;
    if(!RT->GameThread_GetRenderTargetResource()->ReadLinearColorPixels(Pixels) || Pixels.Num()!=256*256)
        return Finish(false,TEXT("Actual GPU pixel readback failed"));
    if(!MaterialCapture.Verify(Error))return Finish(false,Error);
    TArray<FColor> Preview;Preview.Reserve(Pixels.Num());for(const auto& P:Pixels)Preview.Add(P.ToFColorSRGB());
    TArray<TSharedPtr<FJsonValue>> Patches;
    auto Patch=[&](const TCHAR* Name,int32 X,int32 Y,int32 Width,int32 Height,bool On)
    {
        int32 Matching=0;
        for(int32 Dy=0;Dy<Height;++Dy)for(int32 Dx=0;Dx<Width;++Dx)
        {
            const auto& P=Preview[(Y+Dy)*256+X+Dx];
            Matching+=On?(P.R>=32 && P.G<=0.7*P.R && P.B<=0.7*P.R):(FMath::Max3(P.R,P.G,P.B)<=12);
        }
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("region"),Name);
        Row->SetNumberField(TEXT("x"),X);Row->SetNumberField(TEXT("y"),Y);Row->SetNumberField(TEXT("width"),Width);Row->SetNumberField(TEXT("height"),Height);
        Row->SetNumberField(TEXT("matchingPixels"),Matching);Row->SetNumberField(TEXT("pixels"),Width*Height);
        const bool Passed=Matching*10>=Width*Height*9;Row->SetBoolField(TEXT("passed"),Passed);Patches.Add(MakeShared<FJsonValueObject>(Row));return Passed;
    };
    const bool Off=Patch(TEXT("off"),64,96,32,64,false),On=Patch(TEXT("on"),160,96,32,64,true),Outside=Patch(TEXT("outside"),4,4,16,16,false);
    TArray64<uint8> Png;FImageUtils::PNGCompressImageArray(256,256,Preview,Png);const FString Image=ReportPath+TEXT(".png");
    if(IFileManager::Get().FileExists(*Image) || !FFileHelper::SaveArrayToFile(Png,*Image))return Finish(false,TEXT("Fresh PNG evidence required"));
    R->SetArrayField(TEXT("regions"),Patches);R->SetStringField(TEXT("image"),Image);
    R->SetStringField(TEXT("imageSha256"),NodeForgePrototype::FileSha256(Image));
    R->SetStringField(TEXT("fixture"),TEXT("transient Plane origin scale5; capture [0,0,1000], rotation[-90,-90,0], ortho600; 256 square HDR readback/sRGB preview; show-only plane, black, no exposure/tonemap/bloom/AA; no map/material saved"));
    return Finish(Off && On && Outside,TEXT("Frozen independent off/on/outside ROIs each require >=90%; absence alone cannot pass"));
}
int32 Execute(const FString& Mode,const FString& InputPath,const FString& ReportPath)
{
    if(Mode==TEXT("CandidateVisualCheck"))return CheckCandidate(InputPath,ReportPath);
    auto R=MakeShared<FJsonObject>();
    R->SetStringField(TEXT("mode"),Mode); R->SetStringField(TEXT("map"),Map);
    auto Finish=[&](bool Good,const FString& Reason) { R->SetStringField(TEXT("result"),Good?TEXT("passed"):TEXT("failed")); R->SetStringField(TEXT("reason"),Reason); return NodeForgePrototype::WriteJson(R,ReportPath)?(Good?0:1):3; };
    if (!GEditor) return Finish(false,TEXT("Editor is required"));
    UWorld* W=nullptr;
    if (Mode==TEXT("VisualCreate"))
    {
        const FString MapFile=FPackageName::LongPackageNameToFilename(Map,FPackageName::GetMapPackageExtension());
        if (FPackageName::DoesPackageExist(Map)) return Finish(false,TEXT("Gallery exists; refusing overwrite"));
        UMaterial* Bright=LoadObject<UMaterial>(nullptr,*(Folder+TEXT("NFUiMaterialPaste.NFUiMaterialPaste")));
        UMaterial* Dark=LoadObject<UMaterial>(nullptr,*(Folder+TEXT("NFUiMaterialFileDark.NFUiMaterialFileDark")));
        UBlueprint* BP=LoadObject<UBlueprint>(nullptr,*(Folder+TEXT("NFUiBlueprintReady2.NFUiBlueprintReady2")));
        UStaticMesh* Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
        if (!Bright || !Dark || !BP || !BP->GeneratedClass || !Plane) return Finish(false,TEXT("Required actual UI assets or native mesh missing"));
        UMaterial* RefBright=Reference(TEXT("NFReferenceBright"),1.0f);
        UMaterial* RefDark=Reference(TEXT("NFReferenceDark"),0.125f);
        if (!RefBright || !RefDark) return Finish(false,TEXT("Reference creation failed; check partial new assets; never overwrite"));
        UMaterial* Values[]={Bright,RefBright,Dark,RefDark};
        W=GEditor->NewMap();
        if (!W) return Finish(false,TEXT("NewMap failed"));
        for (int32 I=0;I<4;++I)
        {
            const FVector P(I<2?-150:150,I%2==0?-150:150,0);
            auto* A=W->SpawnActor<AStaticMeshActor>(P,FRotator::ZeroRotator);
            if(!A || !A->GetStaticMeshComponent())return Finish(false,TEXT("Gallery mesh actor spawn failed"));
            A->SetActorLabel(Labels[I]); A->SetActorScale3D(FVector(2));
            A->GetStaticMeshComponent()->SetStaticMesh(Plane); A->GetStaticMeshComponent()->SetMaterial(0,Values[I]);
        }
        auto* Camera=W->SpawnActor<ACameraActor>(FVector(0,0,600),FRotator(-90,0,0));
        if(!Camera || !Camera->GetCameraComponent())return Finish(false,TEXT("Gallery camera spawn failed"));
        Camera->SetActorLabel(TEXT("FixedCamera"));
        FByteProperty* Activation=FindFProperty<FByteProperty>(Camera->GetClass(),TEXT("AutoActivateForPlayer"));
        if (!Activation) return Finish(false,TEXT("Native camera activation property unavailable"));
        Activation->SetPropertyValue_InContainer(Camera,static_cast<uint8>(EAutoReceiveInput::Player0));
        auto* C=Camera->GetCameraComponent(); C->ProjectionMode=ECameraProjectionMode::Orthographic; C->OrthoWidth=600; C->AspectRatio=1.0f; C->bConstrainAspectRatio=true;
        C->PostProcessSettings.bOverride_AutoExposureMethod=true; C->PostProcessSettings.AutoExposureMethod=AEM_Manual;
        C->PostProcessSettings.bOverride_AutoExposureBias=true; C->PostProcessSettings.AutoExposureBias=0;
        C->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure=true; C->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure=false;
        C->PostProcessSettings.bOverride_BloomIntensity=true; C->PostProcessSettings.BloomIntensity=0;
        C->PostProcessSettings.bOverride_MotionBlurAmount=true; C->PostProcessSettings.MotionBlurAmount=0;
        if (Camera->GetAutoActivatePlayerIndex()!=0) return Finish(false,TEXT("Camera activation readback failed"));
        if (!W->SpawnActor<AActor>(BP->GeneratedClass,FVector(0,0,-100),FRotator::ZeroRotator)) return Finish(false,TEXT("Generated blueprint instance failed"));
        if (!UEditorLoadingAndSavingUtils::SaveMap(W,Map)) return Finish(false,TEXT("Gallery save failed; check partial new map"));
        return Finish(true,TEXT("Saved new native gallery with exactly checked independent reference materials and UI blueprint instance; visual execution not yet verified"));
    }
    W=UEditorLoadingAndSavingUtils::LoadMap(Map);
    if (!W) return Finish(false,TEXT("Gallery reload failed"));
    FAssetCompilingManager::Get().FinishAllCompilation();
    FlushRenderingCommands();
    W->UpdateWorldComponents(false,false);
    int32 MeshCount=0,CameraCount=0; ACameraActor* Camera=nullptr;TSet<int32> Roles;
    TArray<TSharedPtr<FJsonValue>> Assignments;
    for (TActorIterator<AStaticMeshActor> It(W);It;++It)
    {
        int32 Index=INDEX_NONE; for(int32 I=0;I<4;++I) if(It->GetActorLabel().Equals(Labels[I],ESearchCase::CaseSensitive)) Index=I;
        if (Index==INDEX_NONE) return Finish(false,TEXT("Unexpected mesh actor"));
        if(Roles.Contains(Index))return Finish(false,TEXT("Duplicate gallery role; all four independent roles are required"));
        Roles.Add(Index);
        auto* MeshComponent=It->GetStaticMeshComponent();
        if(!MeshComponent || !MeshComponent->GetStaticMesh())return Finish(false,TEXT("Gallery role has no native static mesh"));
        auto* M=MeshComponent->GetMaterial(0);
        if (!M || M->GetPathName()!=Folder+Materials[Index]+TEXT(".")+Materials[Index]) return Finish(false,TEXT("Actual mesh material assignment mismatch"));
        auto Entry=MakeShared<FJsonObject>(); Entry->SetStringField(TEXT("actor"),Labels[Index]); Entry->SetStringField(TEXT("material"),M->GetPathName());
        Entry->SetBoolField(TEXT("registered"),It->GetStaticMeshComponent()->IsRegistered());
        Entry->SetBoolField(TEXT("renderStateCreated"),It->GetStaticMeshComponent()->IsRenderStateCreated());
        Entry->SetBoolField(TEXT("meshCompiling"),It->GetStaticMeshComponent()->GetStaticMesh()->IsCompiling());
        Entry->SetBoolField(TEXT("meshRenderDataValid"),It->GetStaticMeshComponent()->GetStaticMesh()->HasValidRenderData());
        Assignments.Add(MakeShared<FJsonValueObject>(Entry)); ++MeshCount;
        M->GetMaterial()->CacheShaders(EMaterialShaderPrecompileMode::Default);
        if(auto* Resource=M->GetMaterial()->GetMaterialResource(GMaxRHIFeatureLevel,EMaterialQualityLevel::High)) Resource->FinishCompilation();
        It->GetStaticMeshComponent()->MarkRenderStateDirty();
    }
    for(TActorIterator<ACameraActor> It(W);It;++It)
    {++CameraCount;if(It->GetActorLabel().Equals(TEXT("FixedCamera"),ESearchCase::CaseSensitive))Camera=*It;}
    if(MeshCount!=4 || Roles.Num()!=4 || CameraCount!=1 || !Camera || !Camera->GetCameraComponent()
        || Camera->GetAutoActivatePlayerIndex()!=0) return Finish(false,TEXT("Fixed geometry/camera mismatch"));
    R->SetArrayField(TEXT("actualAssignments"),Assignments);
    R->SetBoolField(TEXT("worldScenePresent"),W->Scene!=nullptr);
    auto* RT=NewObject<UTextureRenderTarget2D>();if(!RT)return Finish(false,TEXT("Gallery render target creation failed"));
    RT->AddToRoot();ASceneCapture2D* Capture=nullptr;
    auto Cleanup=[&]() {if(IsValid(Capture))W->DestroyActor(Capture);Capture=nullptr;if(RT->IsRooted())RT->RemoveFromRoot();};
    ON_SCOPE_EXIT {Cleanup();};
    RT->InitCustomFormat(512,512,PF_FloatRGBA,true); RT->UpdateResourceImmediate(true);
    Capture=W->SpawnActor<ASceneCapture2D>(Camera->GetActorLocation(),Camera->GetActorRotation());
    if(!Capture || !Capture->GetCaptureComponent2D())return Finish(false,TEXT("Gallery capture spawn failed"));
    auto* C=Capture->GetCaptureComponent2D(); C->TextureTarget=RT; C->ProjectionType=ECameraProjectionMode::Orthographic; C->OrthoWidth=600;
    C->bCaptureEveryFrame=false; C->bCaptureOnMovement=false; C->CaptureSource=SCS_SceneColorHDRNoAlpha;
    C->PostProcessSettings=Camera->GetCameraComponent()->PostProcessSettings;
    C->ShowFlags.SetTonemapper(false); C->ShowFlags.SetEyeAdaptation(false); C->ShowFlags.SetBloom(false);
    R->SetBoolField(TEXT("captureRegistered"),C->IsRegistered());
    W->SendAllEndOfFrameUpdates(); FlushRenderingCommands();
    int32 ProxyCount=0;
    for(TActorIterator<AStaticMeshActor> It(W);It;++It) if(It->GetStaticMeshComponent()->SceneProxy) ++ProxyCount;
    R->SetNumberField(TEXT("actualMeshSceneProxies"),ProxyCount);
    C->CaptureScene(); FlushRenderingCommands();
    TArray<FLinearColor> Pixels;
    const bool Read=RT->GameThread_GetRenderTargetResource()->ReadLinearColorPixels(Pixels);
    Cleanup();R->SetBoolField(TEXT("transientCaptureReleased"),!Capture && !RT->IsRooted());
    if (!Read || Pixels.Num()!=512*512) return Finish(false,TEXT("Actual GPU pixel readback failed"));
    TArray<TSharedPtr<FJsonValue>> Patches; int32 Bright=0,Dark=0;
    for(int32 Y : {128,384}) for(int32 X : {128,384})
    {
        FVector Sum(0); for(int32 Dy=-8;Dy<8;++Dy) for(int32 Dx=-8;Dx<8;++Dx) { const auto& P=Pixels[(Y+Dy)*512+X+Dx]; Sum+=FVector(P.R,P.G,P.B); }
        Sum/=256;
        auto P=MakeShared<FJsonObject>(); P->SetNumberField(TEXT("x"),X); P->SetNumberField(TEXT("y"),Y); P->SetNumberField(TEXT("r"),Sum.X); P->SetNumberField(TEXT("g"),Sum.Y); P->SetNumberField(TEXT("b"),Sum.Z);
        Patches.Add(MakeShared<FJsonValueObject>(P));
        if(Sum.Equals(FVector(1),0.02)) ++Bright;
        if(Sum.Equals(FVector(0.125),0.01)) ++Dark;
    }
    TArray<FColor> Preview; Preview.Reserve(Pixels.Num()); for(const auto& P : Pixels) Preview.Add(P.ToFColorSRGB());
    TArray64<uint8> Png; FImageUtils::PNGCompressImageArray(512,512,Preview,Png);
    const FString Image=ReportPath+TEXT(".png");
    if(IFileManager::Get().FileExists(*Image) || !FFileHelper::SaveArrayToFile(Png,*Image)) return Finish(false,TEXT("PNG evidence could not be saved"));
    R->SetStringField(TEXT("image"),Image); R->SetArrayField(TEXT("linearHdrPatches"),Patches);
    R->SetStringField(TEXT("capture"),TEXT("Actual D3D11 SceneCapture2D, PF_FloatRGBA 512 square, HDR SceneColor; tone mapping/exposure/bloom disabled; PNG is sRGB preview"));
    R->SetNumberField(TEXT("brightCount"),Bright); R->SetNumberField(TEXT("darkCount"),Dark);
    return Finish(Bright==2 && Dark==2,TEXT("Require two RGB=1 patches and two RGB=0.125 patches on four actual plane meshes, including independently authored constant references"));
}
}
