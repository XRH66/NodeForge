// Project-owned test harness, reusing NodeForgeVisualChecks' native scene/capture flow.
// Loads already generated assets; never constructs or changes a material graph.
#include "NodeForgeGraphChecks.h"
#include "NodeForgePrototype.h"
#include "AssetCompilingManager.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "ImageUtils.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MaterialShared.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/ScopeExit.h"
#include "RenderingThread.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"

namespace NodeForgeGraphChecks
{
static bool TestPath(const FString& Path)
{
    const FString Prefix=TEXT("/Game/NodeForgeTests/");
    if(!Path.StartsWith(Prefix,ESearchCase::CaseSensitive) || Path.Len()<=Prefix.Len() || Path.Len()>Prefix.Len()+64) return false;
    for(TCHAR Ch:Path.Mid(Prefix.Len())) if(!((Ch>='a'&&Ch<='z')||(Ch>='A'&&Ch<='Z')||(Ch>='0'&&Ch<='9'))) return false;
    return true;
}
static FString ObjectPath(const FString& Path) { return Path+TEXT(".")+FPackageName::GetShortName(Path); }
int32 Execute(const FString& Mode,const FString& InputPath,const FString& ReportPath)
{
    auto R=MakeShared<FJsonObject>(); R->SetStringField(TEXT("mode"),Mode); R->SetStringField(TEXT("input"),InputPath);
    auto Finish=[&](bool Good,const FString& Reason) {R->SetStringField(TEXT("result"),Good?TEXT("passed"):TEXT("failed")); R->SetStringField(TEXT("reason"),Reason); return NodeForgePrototype::WriteJson(R,ReportPath)?(Good?0:1):3;};
    FString Text,InputDigest,InputError; TSharedPtr<FJsonObject> Config;
    if(!NodeForgePrototype::ReadInputFile(InputPath,Text,InputDigest,InputError)
        || FTCHARToUTF8(*Text).Length()>16384
        || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Config) || !Config) return Finish(false,TEXT("Invalid bounded test config"));
    FString Map;
    if(!GEditor || !Config->TryGetStringField(TEXT("map"),Map) || !(Mode==TEXT("GraphMapCreate") ? NodeForgePrototype::ValidateTargetPath(Map,InputError) : TestPath(Map))) return Finish(false,TEXT("Editor and exact isolated test map required"));
    R->SetStringField(TEXT("map"),Map);
    const bool Create=Mode==TEXT("GraphVisualCreate") || Mode==TEXT("GraphMapCreate");
    if(Create && (FPackageName::DoesPackageExist(Map) || FindPackage(nullptr,*Map))) return Finish(false,TEXT("Test map exists; refusing overwrite"));
    if(Mode==TEXT("GraphMapCreate"))
    {
        FString Blueprint;
        if(!Config->TryGetStringField(TEXT("blueprint"),Blueprint) || !NodeForgePrototype::ValidateTargetPath(Blueprint,InputError)) return Finish(false,TEXT("Exact test blueprint required"));
        auto* BP=LoadObject<UBlueprint>(nullptr,*ObjectPath(Blueprint));
        if(!BP || !BP->GeneratedClass || BP->Status==BS_Error) return Finish(false,TEXT("Saved compiled blueprint required"));
        UWorld* W=GEditor->NewMap(); if(!W) return Finish(false,TEXT("NewMap failed"));
        auto* Actor=W->SpawnActor<AActor>(BP->GeneratedClass,FVector::ZeroVector,FRotator::ZeroRotator);
        if(!Actor) return Finish(false,TEXT("Blueprint actor spawn failed"));
        R->SetStringField(TEXT("actualClass"),Actor->GetClass()->GetPathName());
        return Finish(UEditorLoadingAndSavingUtils::SaveMap(W,Map),TEXT("Only new native map with one generated blueprint actor; runtime not tested by map creation"));
    }
    FString MaterialPath;
    if(!Config->TryGetStringField(TEXT("material"),MaterialPath) || !TestPath(MaterialPath)) return Finish(false,TEXT("Exact generated material required"));
    auto* M=LoadObject<UMaterialInterface>(nullptr,*ObjectPath(MaterialPath));
    if(!M) return Finish(false,TEXT("Saved generated material absent"));
    R->SetStringField(TEXT("material"),M->GetPathName());
    UWorld* W=nullptr;
    if(Create)
    {
        auto* Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
        auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
        if(!Plane || !Cube) return Finish(false,TEXT("Native test meshes absent"));
        W=GEditor->NewMap(); if(!W) return Finish(false,TEXT("NewMap failed"));
        auto Mesh=[&](const FString& Label,UStaticMesh* Shape,const FVector& P,const FVector& S,UMaterialInterface* Mat)->AStaticMeshActor* {
            auto* A=W->SpawnActor<AStaticMeshActor>(P,FRotator::ZeroRotator); if(!A) return nullptr;
            A->SetActorLabel(Label); A->GetStaticMeshComponent()->SetStaticMesh(Shape); A->SetActorScale3D(S); if(Mat) A->GetStaticMeshComponent()->SetMaterial(0,Mat); return A;};
        if(!Mesh(TEXT("GeneratedSurface"),Plane,FVector::ZeroVector,FVector(5),M)
            || !Mesh(TEXT("NativeFloor"),Cube,FVector(0,0,-55),FVector(6,6,0.5),nullptr))
            return Finish(false,TEXT("Native test surface/floor spawn failed"));
        for(int32 I=-2;I<=2;++I) if(!Mesh(FString::Printf(TEXT("NativeMarker%d"),I+2),Cube,FVector(I*90,0,-12),FVector(0.12,4.8,0.1),nullptr))
            return Finish(false,TEXT("Native test marker spawn failed"));
        auto* Light=W->SpawnActor<ADirectionalLight>(FVector(0,0,200),FRotator(-55,160,0));
        if(!Light || !Light->GetLightComponent())return Finish(false,TEXT("Native test light spawn failed"));
        Light->GetLightComponent()->SetIntensity(3.0f); Light->SetActorLabel(TEXT("FixedKeyLight"));
        auto* Camera=W->SpawnActor<ACameraActor>(FVector(380,0,430),FRotator(-48.5,180,0));
        if(!Camera || !Camera->GetCameraComponent())return Finish(false,TEXT("Native test camera spawn failed"));
        Camera->SetActorLabel(TEXT("FixedCamera"));
        auto* Activation=FindFProperty<FByteProperty>(Camera->GetClass(),TEXT("AutoActivateForPlayer"));
        if(!Activation) return Finish(false,TEXT("Camera activation absent"));
        Activation->SetPropertyValue_InContainer(Camera,static_cast<uint8>(EAutoReceiveInput::Player0));
        auto* C=Camera->GetCameraComponent(); C->ProjectionMode=ECameraProjectionMode::Orthographic; C->OrthoWidth=720; C->AspectRatio=1; C->bConstrainAspectRatio=true;
        C->PostProcessSettings.bOverride_AutoExposureMethod=true; C->PostProcessSettings.AutoExposureMethod=AEM_Manual;
        C->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure=true; C->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure=false;
        C->PostProcessSettings.bOverride_BloomIntensity=true; C->PostProcessSettings.BloomIntensity=0;
        C->PostProcessSettings.bOverride_MotionBlurAmount=true; C->PostProcessSettings.MotionBlurAmount=0;
        return Finish(UEditorLoadingAndSavingUtils::SaveMap(W,Map),TEXT("New saved scene references generated material and native meshes; no graph or project settings changed"));
    }
    W=UEditorLoadingAndSavingUtils::LoadMap(Map); if(!W) return Finish(false,TEXT("Saved map reload failed"));
    FAssetCompilingManager::Get().FinishAllCompilation(); W->UpdateWorldComponents(false,false);
    AStaticMeshActor* Surface=nullptr; ACameraActor* Camera=nullptr;
    for(TActorIterator<AStaticMeshActor> It(W);It;++It) if(It->GetActorLabel()==TEXT("GeneratedSurface")) Surface=*It;
    for(TActorIterator<ACameraActor> It(W);It;++It) if(It->GetActorLabel()==TEXT("FixedCamera")) Camera=*It;
    if(!Surface || !Camera || !Surface->GetStaticMeshComponent() || !Surface->GetStaticMeshComponent()->GetStaticMesh()
        || !Camera->GetCameraComponent() || Surface->GetStaticMeshComponent()->GetMaterial(0)!=M) return Finish(false,TEXT("Actual saved assignment/camera mismatch"));
    M->CacheShaders(EMaterialShaderPrecompileMode::Default); if(auto* Res=M->GetMaterialResource(GMaxRHIFeatureLevel,EMaterialQualityLevel::High)) Res->FinishCompilation();
    for(TActorIterator<AStaticMeshActor> It(W);It;++It) if(auto* MeshComponent=It->GetStaticMeshComponent())MeshComponent->MarkRenderStateDirty();
    auto* RT=NewObject<UTextureRenderTarget2D>();if(!RT)return Finish(false,TEXT("Native render target creation failed"));
    RT->AddToRoot();ASceneCapture2D* Capture=nullptr;
    UMaterialInterface* OriginalMaterial=Surface->GetStaticMeshComponent()->GetMaterial(0);
    auto Cleanup=[&]()
    {
        if(IsValid(Surface) && Surface->GetStaticMeshComponent())
        {Surface->GetStaticMeshComponent()->SetMaterial(0,OriginalMaterial);Surface->GetStaticMeshComponent()->MarkRenderStateDirty();}
        if(IsValid(Capture))W->DestroyActor(Capture);
        Capture=nullptr;if(RT->IsRooted())RT->RemoveFromRoot();
    };
    ON_SCOPE_EXIT {Cleanup();};
    RT->InitCustomFormat(640,640,PF_FloatRGBA,true); RT->UpdateResourceImmediate(true);
    Capture=W->SpawnActor<ASceneCapture2D>(Camera->GetActorLocation(),Camera->GetActorRotation());
    if(!Capture || !Capture->GetCaptureComponent2D())return Finish(false,TEXT("Native scene capture spawn failed"));
    auto* C=Capture->GetCaptureComponent2D();
    C->TextureTarget=RT; C->ProjectionType=ECameraProjectionMode::Orthographic; C->OrthoWidth=720; C->CaptureSource=SCS_SceneColorHDRNoAlpha;
    C->bCaptureEveryFrame=false; C->bCaptureOnMovement=false; C->PostProcessSettings=Camera->GetCameraComponent()->PostProcessSettings;
    C->ShowFlags.SetTonemapper(false); C->ShowFlags.SetEyeAdaptation(false); C->ShowFlags.SetBloom(false);
    TArray<TSharedPtr<FJsonValue>> Frames;
    auto Frame=[&](const FString& Name,TArray<FLinearColor>& Pixels)->bool {
        W->SendAllEndOfFrameUpdates(); FlushRenderingCommands(); C->CaptureScene(); FlushRenderingCommands();
        if(!RT->GameThread_GetRenderTargetResource()->ReadLinearColorPixels(Pixels) || Pixels.Num()!=640*640) return false;
        TArray<FColor> Preview; for(const auto& P:Pixels) Preview.Add(P.ToFColorSRGB()); TArray64<uint8> Bytes; FImageUtils::PNGCompressImageArray(640,640,Preview,Bytes);
        FString Path=ReportPath+TEXT(".")+Name+TEXT(".png"); if(IFileManager::Get().FileExists(*Path) || !FFileHelper::SaveArrayToFile(Bytes,*Path)) return false;
        auto E=MakeShared<FJsonObject>(); E->SetStringField(TEXT("frame"),Name); E->SetStringField(TEXT("image"),Path); E->SetNumberField(TEXT("worldTime"),W->GetTimeSeconds()); E->SetNumberField(TEXT("realTime"),W->GetRealTimeSeconds()); Frames.Add(MakeShared<FJsonValueObject>(E)); return true;};
    auto Advance=[&]() { for(int32 I=0;I<60;++I) W->Tick(LEVELTICK_All,1.0f/60.0f); };
    TArray<FLinearColor> Warm,A,B,D; Frame(TEXT("Warm"),Warm); Advance();
    bool Good=Frame(TEXT("TimeA"),A); Advance(); Good=Frame(TEXT("TimeB"),B)&&Good;
    auto* MID=UMaterialInstanceDynamic::Create(M,Surface);if(!MID)return Finish(false,TEXT("Transient material instance creation failed"));
    Surface->GetStaticMeshComponent()->SetMaterial(0,MID);
    const TSharedPtr<FJsonObject>* Scalars=nullptr; const TSharedPtr<FJsonObject>* Vectors=nullptr;
    if(!Config->TryGetObjectField(TEXT("scalarChanges"),Scalars) || !Config->TryGetObjectField(TEXT("vectorChanges"),Vectors)) return Finish(false,TEXT("Explicit parameter change objects required"));
    auto Applied=MakeShared<FJsonObject>();
    for(const auto& Pair:(*Scalars)->Values)
    {
        double Value=0; float Original=0; if(!Pair.Value->TryGetNumber(Value) || !FMath::IsFinite(Value) || !M->GetScalarParameterValue(FMaterialParameterInfo(*Pair.Key),Original)) return Finish(false,TEXT("Unknown/invalid scalar parameter"));
        MID->SetScalarParameterValue(*Pair.Key,Value); float Actual=0; if(!MID->GetScalarParameterValue(FMaterialParameterInfo(*Pair.Key),Actual) || !FMath::IsNearlyEqual(Actual,static_cast<float>(Value))) return Finish(false,TEXT("Scalar parameter readback mismatch")); Applied->SetNumberField(Pair.Key,Actual);
    }
    for(const auto& Pair:(*Vectors)->Values)
    {
        const TArray<TSharedPtr<FJsonValue>>* V=nullptr; FLinearColor Original; if(!Pair.Value->TryGetArray(V) || V->Num()!=4 || !M->GetVectorParameterValue(FMaterialParameterInfo(*Pair.Key),Original)) return Finish(false,TEXT("Unknown/invalid vector parameter"));
        double Values[4]; for(int32 I=0;I<4;++I) if(!(*V)[I]->TryGetNumber(Values[I]) || !FMath::IsFinite(Values[I])) return Finish(false,TEXT("Invalid color components"));
        FLinearColor Value(Values[0],Values[1],Values[2],Values[3]); MID->SetVectorParameterValue(*Pair.Key,Value); FLinearColor Actual;
        if(!MID->GetVectorParameterValue(FMaterialParameterInfo(*Pair.Key),Actual) || !Value.Equals(Actual)) return Finish(false,TEXT("Vector readback mismatch")); Applied->SetArrayField(Pair.Key,*V);
    }
    Surface->GetStaticMeshComponent()->MarkRenderStateDirty(); Good=Frame(TEXT("Parameters"),D)&&Good;
    auto Difference=[](const TArray<FLinearColor>& L,const TArray<FLinearColor>& Q) {double Sum=0; if(L.Num()!=640*640 || Q.Num()!=L.Num()) return -1.0; for(int32 Y=120;Y<520;++Y) for(int32 X=120;X<520;++X) {int32 I=Y*640+X; Sum+=FMath::Abs(L[I].R-Q[I].R)+FMath::Abs(L[I].G-Q[I].G)+FMath::Abs(L[I].B-Q[I].B);} return Sum/(400.0*400.0*3.0);};
    const double Animation=Difference(A,B),Parameters=Difference(B,D);
    R->SetArrayField(TEXT("frames"),Frames); R->SetObjectField(TEXT("actualParameters"),Applied); R->SetNumberField(TEXT("animationMeanAbsoluteDifference"),Animation); R->SetNumberField(TEXT("parameterMeanAbsoluteDifference"),Parameters);
    R->SetStringField(TEXT("capture"),TEXT("Actual D3D11 SceneCapture2D HDR; saved map reopened; world tick advanced 1 second; parameter changes transient only; PNG sRGB"));
    Cleanup();R->SetBoolField(TEXT("transientCaptureReleased"),!Capture && !RT->IsRooted());
    R->SetBoolField(TEXT("originalMaterialAssignmentRestored"),Surface->GetStaticMeshComponent()->GetMaterial(0)==OriginalMaterial);
    return Finish(Good && Animation>0.00001 && Parameters>0.0001,TEXT("Require actual time and explicit parameter pixel changes; visual quality is separately inspected from saved frames; no map/material resaved"));
}
}
