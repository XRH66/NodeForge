// NodeForge project code. Uses installed UE APIs; no third-party source is copied.
#include "CoreMinimal.h"
#include "NodeForgePanel.h"
#include "NodeForgePrototype.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Layout/Children.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/DefaultValueHelper.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "Serialization/JsonSerializer.h"
#include "Tests/AutomationEditorCommon.h"
#include "UObject/Script.h"
#include "UObject/Stack.h"
#include "Widgets/Docking/SDockTab.h"

namespace NodeForgeRuntimeTest
{
// Each queued test owns its expectations; optional graph tests cannot change the
// legacy defaults for another test in the same Editor automation process.
struct FRuntimeExpectations
{
    FString TestMap = TEXT("/Game/NodeForgeTests/NFBlueprintTestMap");
    FString Blueprint = TEXT("/Game/NodeForgeTests/NFMinimalBlueprint");
    FString TestActorClass = TEXT("/Game/NodeForgeTests/NFMinimalBlueprint.NFMinimalBlueprint_C");
    FString ExpectedPayload = TEXT("NodeForge Blueprint Runtime OK");
    bool bContinuousX = false;
};
static constexpr double ObserveSeconds = 2.0;
static constexpr double StopDeadlineSeconds = 55.0;
static constexpr double HardDeadlineSeconds = 60.0;

static bool IsTestAssetPath(const FString& Path)
{
    FString Error;
    return NodeForgePrototype::ValidateTargetPath(Path,Error);
}

// This is a bounded three-string test configuration, not a graph importer.
static bool ReadRuntimeConfig(const FString& Path, FRuntimeExpectations& Out, FString& Error)
{
    const int64 Size = IFileManager::Get().FileSize(*Path);
    if (Path.IsEmpty() || Size < 1 || Size > 8192)
    {
        Error = TEXT("NFGraphRuntimeConfig must name an existing UTF-8 file of 1..8192 bytes.");
        return false;
    }
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *Path) || Bytes.Num() != Size)
    {
        Error = TEXT("The runtime configuration could not be read completely.");
        return false;
    }
    const FUTF8ToTCHAR Decoded(reinterpret_cast<const ANSICHAR*>(Bytes.GetData()), Bytes.Num());
    const FString Text(Decoded.Length(), Decoded.Get());
    const FTCHARToUTF8 Encoded(*Text, Text.Len());
    if (Encoded.Length() != Bytes.Num() || FMemory::Memcmp(Encoded.Get(), Bytes.GetData(), Bytes.Num()) != 0)
    {
        Error = TEXT("The runtime configuration is not valid UTF-8.");
        return false;
    }
    TMap<FString, FString> Fields;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
    EJsonNotation Notation;
    bool bRootStarted = false;
    bool bRootFinished = false;
    while (Reader->ReadNext(Notation))
    {
        if (!bRootStarted && Notation == EJsonNotation::ObjectStart)
        {
            bRootStarted = true;
        }
        else if (bRootStarted && !bRootFinished && Notation == EJsonNotation::ObjectEnd)
        {
            bRootFinished = true;
        }
        else if (bRootStarted && !bRootFinished && Notation == EJsonNotation::String &&
            !Fields.Contains(Reader->GetIdentifier()) &&
            (Reader->GetIdentifier() == TEXT("map") || Reader->GetIdentifier() == TEXT("blueprint") ||
             Reader->GetIdentifier() == TEXT("expectedMessage") || Reader->GetIdentifier() == TEXT("mode")))
        {
            Fields.Add(Reader->GetIdentifier(), Reader->GetValueAsString());
        }
        else
        {
            Error = TEXT("The runtime configuration must contain exactly map, blueprint, expectedMessage as unique string fields.");
            return false;
        }
    }
    if (!Reader->GetErrorMessage().IsEmpty() || !bRootFinished || (Fields.Num() != 3 && Fields.Num() != 4) || (Fields.Contains(TEXT("mode")) && Fields.FindRef(TEXT("mode"))!=TEXT("ContinuousX")) ||
        !IsTestAssetPath(Fields.FindRef(TEXT("map"))) || !IsTestAssetPath(Fields.FindRef(TEXT("blueprint"))))
    {
        Error = TEXT("Runtime map and blueprint must use /Game/NodeForgeTests/ followed by one 1..64 character ASCII alphanumeric name.");
        return false;
    }
    const FString Message = Fields.FindRef(TEXT("expectedMessage"));
    if (Message.IsEmpty() || Message.Len() > 4096)
    {
        Error = TEXT("expectedMessage must contain 1..4096 characters.");
        return false;
    }
    for (const TCHAR C : Message)
    {
        if (C < 0x20 || C == 0x7f)
        {
            Error = TEXT("expectedMessage must be a single line without control characters.");
            return false;
        }
    }
    FRuntimeExpectations Candidate;
    Candidate.TestMap = Fields.FindRef(TEXT("map"));
    Candidate.Blueprint = Fields.FindRef(TEXT("blueprint"));
    Candidate.TestActorClass = Candidate.Blueprint + TEXT(".") + FPackageName::GetShortName(Candidate.Blueprint) + TEXT("_C");
    Candidate.ExpectedPayload = Message;
    Candidate.bContinuousX = Fields.Contains(TEXT("mode"));
    Out = MoveTemp(Candidate);
    return true;
}

static FString OriginalPackageName(const FString& PackageName)
{
    return UWorld::RemovePIEPrefix(PackageName);
}

static int32 FindPIEWorlds(UWorld*& OutWorld)
{
    OutWorld = nullptr;
    int32 Count = 0;
    if (GEngine)
    {
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
        {
            if (Context.WorldType == EWorldType::PIE && Context.World())
            {
                OutWorld = Context.World();
                ++Count;
            }
        }
    }
    return Count;
}

static int32 FindTestActors(UWorld* World, AActor*& OutActor, const FString& ExpectedClass)
{
    int32 Count = 0;
    OutActor = nullptr;
    if (World)
    {
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            if (OriginalPackageName(It->GetClass()->GetPathName()) == ExpectedClass)
            {
                OutActor = *It;
                ++Count;
            }
        }
    }
    return Count;
}

// This observes the engine's PrintString log. It never emits the expected text.
class FRuntimeMessageCapture final : public FOutputDevice
{
public:
    explicit FRuntimeMessageCapture(const FRuntimeExpectations& InExpectations) : Expectations(InExpectations) {}

    virtual ~FRuntimeMessageCapture() override
    {
        Stop();
    }

    void Start()
    {
        if (!bRegistered && GLog)
        {
            GLog->AddOutputDevice(this);
            bRegistered = true;
        }
    }

    void Stop()
    {
        if (bRegistered && GLog)
        {
            GLog->RemoveOutputDevice(this);
        }
        bRegistered = false;
    }

    bool IsRegistered() const { return bRegistered; }

    // UE 5.3's redirector buffers devices without this flag on its logging thread.
    // Synchronous delivery is necessary to inspect the real Blueprint call stack.
    virtual bool CanBeUsedOnAnyThread() const override { return true; }
    virtual bool CanBeUsedOnMultipleThreads() const override { return true; }

    virtual void Serialize(const TCHAR* Text, ELogVerbosity::Type Verbosity, const FName& Category) override
    {
        if (Category != FName(TEXT("LogBlueprintUserMessages")) || Verbosity != ELogVerbosity::Log)
        {
            return;
        }

        const FString Raw(Text);
        FString Payload = Raw;
        if (Raw.StartsWith(TEXT("[")))
        {
            const int32 SourceEnd = Raw.Find(TEXT("] "));
            if (SourceEnd != INDEX_NONE)
            {
                Payload = Raw.Mid(SourceEnd + 2);
            }
        }
        if (!Payload.Equals(Expectations.ExpectedPayload, ESearchCase::CaseSensitive))
        {
            return;
        }

        FScopeLock Lock(&Mutex);
        ++MessageCount;
        RawMessages.Add(Raw);
        if (!IsInGameThread())
        {
            AttributionFailures.Add(TEXT("Matching message was not delivered synchronously on the game thread."));
            return;
        }

#if DO_BLUEPRINT_GUARD
        const FBlueprintContextTracker* Tracker = FBlueprintContextTracker::TryGet();
        if (Tracker)
        {
            const TArrayView<const FFrame* const> Stack = Tracker->GetCurrentScriptStack();
            for (int32 Index = Stack.Num() - 1; Index >= 0; --Index)
            {
                const AActor* SourceActor = Stack[Index] ? Cast<AActor>(Stack[Index]->Object) : nullptr;
                if (!SourceActor || OriginalPackageName(SourceActor->GetClass()->GetPathName()) != Expectations.TestActorClass)
                {
                    continue;
                }
                const UWorld* SourceWorld = SourceActor->GetWorld();
                const FString NativeSourcePrefix = FString::Printf(TEXT("[%s] "), *SourceActor->GetName());
                if (!SourceWorld || SourceWorld->WorldType != EWorldType::PIE ||
                    OriginalPackageName(SourceWorld->GetOutermost()->GetName()) != Expectations.TestMap ||
                    (Raw != Expectations.ExpectedPayload && Raw != NativeSourcePrefix + Expectations.ExpectedPayload))
                {
                    AttributionFailures.Add(TEXT("Matching script object had a different world, map, or native log prefix."));
                    return;
                }
                ++AttributedMessageCount;
                SourceActorPath = SourceActor->GetPathName();
                SourceClassPath = SourceActor->GetClass()->GetPathName();
                SourceWorldPath = SourceWorld->GetPathName();
                SourceFunction = Stack[Index]->Node ? Stack[Index]->Node->GetName() : FString();
                return;
            }
        }
        AttributionFailures.Add(TEXT("Matching message had no target Actor in the live Blueprint script stack."));
#else
        AttributionFailures.Add(TEXT("DO_BLUEPRINT_GUARD is disabled; live script attribution is unavailable."));
#endif
    }

    int32 MessageCount = 0;
    int32 AttributedMessageCount = 0;
    TArray<FString> RawMessages;
    TArray<FString> AttributionFailures;
    FString SourceActorPath;
    FString SourceClassPath;
    FString SourceWorldPath;
    FString SourceFunction;

private:
    const FRuntimeExpectations& Expectations;
    FCriticalSection Mutex;
    bool bRegistered = false;
};

class FRunBlueprintPIE final : public IAutomationLatentCommand
{
public:
    explicit FRunBlueprintPIE(FAutomationTestBase& InTest)
        : Test(InTest), Capture(Expectations), StartedAt(FPlatformTime::Seconds())
    {
        if (!FParse::Value(FCommandLine::Get(), TEXT("NFReport="), ReportPath))
        {
            ReportPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("NodeForge/BlueprintRuntime.json"));
        }
        ReportPath = FPaths::ConvertRelativePathToFull(ReportPath);
        bGraphConfigRequested = FParse::Value(FCommandLine::Get(), TEXT("NFGraphRuntimeConfig="), ConfigPath);
        if (bGraphConfigRequested && !ConfigPath.IsEmpty()) ConfigPath = FPaths::ConvertRelativePathToFull(ConfigPath);
        // Native FParse stops at commas by default; vector CSV must preserve all coordinates.
        bLocationRequested = FParse::Value(FCommandLine::Get(), TEXT("NFExpectedActorLocation="), ExpectedLocationText, false);
        bRotationRequested = FParse::Value(FCommandLine::Get(), TEXT("NFExpectedActorRotation="), ExpectedRotationText, false);
        bScaleRequested = FParse::Value(FCommandLine::Get(), TEXT("NFExpectedActorScale="), ExpectedScaleText, false);
    }

    virtual ~FRunBlueprintPIE() override
    {
        RemoveMotionObserver();
        Capture.Stop();
        if (!bFinished && bOwnsPlaySession && GEditor)
        {
            GEditor->RequestEndPlayMap();
        }
    }

    virtual bool Update() override
    {
        if (!bStarted)
        {
            return Start();
        }

        UWorld* PIEWorld = nullptr;
        const int32 PIEWorldCount = FindPIEWorlds(PIEWorld);
        const double Now = FPlatformTime::Seconds();

        if (bStopping)
        {
            if (PIEWorldCount == 0 && (!GEditor || !GEditor->PlayWorld))
            {
                bPIEStopped = true;
                return Finish();
            }
            if (Now - StartedAt >= HardDeadlineSeconds)
            {
                Fail(TEXT("PIE cleanup exceeded the 60 second test deadline."));
                if (GEditor && bOwnsPlaySession)
                {
                    GEditor->EndPlayMap();
                }
                bPIEStopped = FindPIEWorlds(PIEWorld) == 0 && (!GEditor || !GEditor->PlayWorld);
                return Finish();
            }
            return false;
        }

        if (Now - StartedAt >= StopDeadlineSeconds)
        {
            Fail(TEXT("PIE did not complete the required observation before the shutdown deadline."));
            BeginStop();
            return false;
        }
        if (PIEWorldCount > 1)
        {
            Fail(TEXT("The runtime test requires exactly one PIE world."));
            BeginStop();
            return false;
        }
        if (!PIEWorld)
        {
            return false;
        }

        ActualPIEMap = PIEWorld->GetOutermost()->GetName();
        WorldType = PIEWorld->WorldType == EWorldType::PIE ? TEXT("PIE") : TEXT("Other");
        AActor* Actor = nullptr;
        ActorCount = FindTestActors(PIEWorld, Actor, Expectations.TestActorClass);
        bActorBegunPlay = Actor && Actor->HasActorBegunPlay();
        if (Actor)
        {
            ActualActorPath = Actor->GetPathName();
            ActualClassPath = Actor->GetClass()->GetPathName();
            ActualLocation = Actor->GetActorLocation();
            ActualRotation = Actor->GetActorRotation();
            ActualScale = Actor->GetActorScale3D();
        }
        if (OriginalPackageName(ActualPIEMap) != Expectations.TestMap)
        {
            Fail(TEXT("PIE loaded a different map from the requested test map."));
            BeginStop();
            return false;
        }
        if (!PIEWorld->HasBegunPlay())
        {
            return false;
        }
        if (BeginPlayObservedAt < 0.0)
        {
            BeginPlayObservedAt = Now;
        }
        ObservedSeconds = Now - BeginPlayObservedAt;
        if (Expectations.bContinuousX)
        {
            if (!Reasons.IsEmpty()) {BeginStop();return false;}
            if (MotionSamples.Num()<3) return false;
            bObservationCompleted=true;BeginStop();return false;
        }
        if (ObservedSeconds < ObserveSeconds)
        {
            return false;
        }

        if (ActorCount != 1)
        {
            Fail(FString::Printf(TEXT("Expected one generated Actor in PIE; found %d."), ActorCount));
        }
        if (!bActorBegunPlay)
        {
            Fail(TEXT("The generated Actor has not completed BeginPlay."));
        }
        if (bLocationRequested && (!Actor || ActualLocation.ContainsNaN() || !ActualLocation.Equals(ExpectedLocation, 1.e-6)))
        {
            Fail(TEXT("The actual live Actor location differs from the JSON-driven runtime expectation."));
        }
        if (bRotationRequested && (!Actor || ActualRotation.ContainsNaN() || !ActualRotation.Equals(ExpectedRotation, 1.e-6)))
            Fail(TEXT("The actual live Actor rotation differs from the JSON-driven runtime expectation."));
        if (bScaleRequested && (!Actor || ActualScale.ContainsNaN() || !ActualScale.Equals(ExpectedScale, 1.e-6)))
            Fail(TEXT("The actual live Actor scale differs from the JSON-driven runtime expectation."));
        bObservationCompleted = true;
        BeginStop();
        return false;
    }

private:
    bool Start()
    {
        bStarted = true;
        if (IFileManager::Get().FileExists(*ReportPath))
        {
            bEvidencePathOccupied = true;
            Fail(TEXT("The requested evidence file already exists; use a new NFReport path to preserve the earlier run."));
            return Finish();
        }
        if (bLocationRequested && (!FDefaultValueHelper::ParseVector(ExpectedLocationText, ExpectedLocation)
            || ExpectedLocation.ContainsNaN() || FMath::Abs(ExpectedLocation.X) > 1.e12
            || FMath::Abs(ExpectedLocation.Y) > 1.e12 || FMath::Abs(ExpectedLocation.Z) > 1.e12))
        {
            Fail(TEXT("NFExpectedActorLocation requires three bounded finite native vector coordinates."));
            return Finish();
        }
        if ((bRotationRequested && (!FDefaultValueHelper::ParseRotator(ExpectedRotationText, ExpectedRotation)
                || ExpectedRotation.ContainsNaN() || FMath::Abs(ExpectedRotation.Pitch) > 1.e12
                || FMath::Abs(ExpectedRotation.Yaw) > 1.e12 || FMath::Abs(ExpectedRotation.Roll) > 1.e12))
            || (bScaleRequested && (!FDefaultValueHelper::ParseVector(ExpectedScaleText, ExpectedScale)
                || ExpectedScale.ContainsNaN() || FMath::Abs(ExpectedScale.X) > 1.e12
                || FMath::Abs(ExpectedScale.Y) > 1.e12 || FMath::Abs(ExpectedScale.Z) > 1.e12)))
        {
            Fail(TEXT("Actor rotation/scale expectations require three bounded finite native coordinates."));
            return Finish();
        }
        if (bGraphConfigRequested)
        {
            FString ConfigError;
            if (!ReadRuntimeConfig(ConfigPath, Expectations, ConfigError))
            {
                ConfigValidation = TEXT("failed");
                Fail(ConfigError);
                return Finish();
            }
            ConfigValidation = TEXT("passed");
        }
        UWorld* ExistingPIEWorld = nullptr;
        if (!GEditor || !GEngine || IsRunningCommandlet())
        {
            Fail(TEXT("This test requires an initialized interactive Editor automation process."));
            return Finish();
        }
        if (FindPIEWorlds(ExistingPIEWorld) != 0 || GEditor->PlayWorld)
        {
            Fail(TEXT("A play session already exists; the test will not interrupt it."));
            return Finish();
        }
        FString MapFilename;
        if (!FPackageName::DoesPackageExist(Expectations.TestMap, &MapFilename))
        {
            Fail(TEXT("The independently saved test map does not exist on disk."));
            return Finish();
        }
        if (bGraphConfigRequested)
        {
            FString BlueprintFilename;
            if (!FPackageName::DoesPackageExist(Expectations.Blueprint, &BlueprintFilename))
            {
                Fail(TEXT("The configured Blueprint has not been independently saved on disk."));
                return Finish();
            }
            const FString BlueprintObjectPath = Expectations.Blueprint + TEXT(".") + FPackageName::GetShortName(Expectations.Blueprint);
            const UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintObjectPath);
            if (!Blueprint || !Blueprint->GeneratedClass || !Blueprint->GeneratedClass->IsChildOf(AActor::StaticClass()) ||
                Blueprint->GeneratedClass->GetPathName() != Expectations.TestActorClass)
            {
                Fail(TEXT("The configured Blueprint has no matching generated Actor class after loading."));
                return Finish();
            }
            Expectations.TestActorClass = Blueprint->GeneratedClass->GetPathName();
        }

        FAutomationEditorCommonUtils::LoadMap(MapFilename);
        UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
        ActualEditorMap = EditorWorld ? EditorWorld->GetOutermost()->GetName() : FString();
        if (!EditorWorld || ActualEditorMap != Expectations.TestMap)
        {
            Fail(TEXT("The requested test map could not be loaded into the Editor."));
            return Finish();
        }
        AActor* EditorActor = nullptr;
        if (FindTestActors(EditorWorld, EditorActor, Expectations.TestActorClass) != 1)
        {
            Fail(TEXT("The saved test map must contain exactly one generated Actor before PIE."));
            return Finish();
        }
        if (FPlatformTime::Seconds() - StartedAt >= StopDeadlineSeconds)
        {
            Fail(TEXT("Map loading exhausted the runtime test's startup deadline."));
            return Finish();
        }

        if (Expectations.bContinuousX)
        {
            if (bLocationRequested || bRotationRequested || bScaleRequested) {Fail(TEXT("ContinuousX uses the frozen independent frame recipe, not one-shot transform flags"));return Finish();}
            PreTickHandle=FWorldDelegates::OnWorldPreActorTick.AddRaw(this,&FRunBlueprintPIE::BeforeActorTick);
            PostTickHandle=FWorldDelegates::OnWorldPostActorTick.AddRaw(this,&FRunBlueprintPIE::AfterActorTick);
        }
        Capture.Start();
        if (!Capture.IsRegistered())
        {
            Fail(TEXT("Could not register the passive Blueprint log observer."));
            return Finish();
        }
        bOwnsPlaySession = true;
        FStartPIECommand StartCommand(false);
        StartCommand.Update();
        return false;
    }

    void RemoveMotionObserver()
    {
        if (PreTickHandle.IsValid()) FWorldDelegates::OnWorldPreActorTick.Remove(PreTickHandle);
        if (PostTickHandle.IsValid()) FWorldDelegates::OnWorldPostActorTick.Remove(PostTickHandle);
        PreTickHandle.Reset();PostTickHandle.Reset();
    }
    bool MotionActor(UWorld* World,ELevelTick Type,AActor*& Actor)
    {
        Actor=nullptr;
        if (!bOwnsPlaySession || bStopping || World->WorldType!=EWorldType::PIE || OriginalPackageName(World->GetOutermost()->GetName())!=Expectations.TestMap) return false;
        if (FindTestActors(World,Actor,Expectations.TestActorClass)!=1 || !Actor || !Actor->HasActorBegunPlay()) return false;
        const auto& Tick=Actor->PrimaryActorTick;
        if (Type!=LEVELTICK_All || World->IsPaused() || !Tick.bCanEverTick || !Tick.bStartWithTickEnabled
            || !Tick.IsTickFunctionRegistered() || !Tick.IsTickFunctionEnabled() || Tick.TickInterval!=0.f
            || Tick.TickGroup!=TG_PrePhysics || Tick.bRunOnAnyThread || Actor->CustomTimeDilation!=1.f)
        {Fail(TEXT("Actor effective per-frame Tick admission differs from frozen observer recipe"));return false;}
        return true;
    }
    void MotionSample(AActor* Actor)
    {
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("actor"),Actor->GetPathName());Row->SetStringField(TEXT("world"),Actor->GetWorld()->GetPathName());
        Row->SetNumberField(TEXT("effectiveDeltaAccumulated"),MotionDelta);Row->SetNumberField(TEXT("effectiveFrames"),MotionFrames);
        Row->SetNumberField(TEXT("worldGameTime"),Actor->GetWorld()->GetTimeSeconds());
        const auto V=Actor->GetActorLocation();Row->SetArrayField(TEXT("location"),{MakeShared<FJsonValueNumber>(V.X),MakeShared<FJsonValueNumber>(V.Y),MakeShared<FJsonValueNumber>(V.Z)});
        MotionSamples.Add(MakeShared<FJsonValueObject>(Row));
    }
    void BeforeActorTick(UWorld* World,ELevelTick Type,float Delta)
    {
        AActor* Actor=nullptr;if (!MotionActor(World,Type,Actor) || !Reasons.IsEmpty() || MotionSamples.Num()>=3) return;
        if (!FMath::IsFinite(Delta) || Delta<=0.f || Delta>.25f || MotionFrames>=4096) {Fail(TEXT("Frame Delta or observation bound exceeded frozen limit"));return;}
        if (!MotionTarget.IsValid())
        {
            MotionTarget=Actor;MotionWorld=World;MotionOrigin=Actor->GetActorLocation();MotionRotation=Actor->GetActorRotation();MotionScale=Actor->GetActorScale3D();MotionSample(Actor);
        }
        if (MotionTarget.Get()!=Actor || MotionWorld.Get()!=World || bMotionFrame) {Fail(TEXT("Actor/world/frame observer identity conflict"));return;}
        FrameDelta=Delta;bMotionFrame=true;
    }
    void AfterActorTick(UWorld* World,ELevelTick Type,float Delta)
    {
        if (!bMotionFrame || MotionWorld.Get()!=World) return;
        bMotionFrame=false;AActor* Actor=nullptr;
        if (!MotionActor(World,Type,Actor) || Actor!=MotionTarget.Get() || Delta!=FrameDelta) {Fail(TEXT("Post-frame admission or Delta differs from admitted actor frame"));return;}
        MotionDelta+=double(Delta);++MotionFrames;
        const auto Location=Actor->GetActorLocation();const FVector Wanted=MotionOrigin+FVector(100.*MotionDelta,0,0);
        if (Location.ContainsNaN() || !Location.Equals(Wanted,.001) || !Actor->GetActorRotation().Equals(MotionRotation,.000001) || !Actor->GetActorScale3D().Equals(MotionScale,.000001))
        {Fail(TEXT("Actual per-frame Actor displacement/transform differs from independently accumulated game Delta"));return;}
        auto Frame=MakeShared<FJsonObject>();Frame->SetNumberField(TEXT("frame"),MotionFrames);Frame->SetNumberField(TEXT("deltaSeconds"),Delta);Frame->SetNumberField(TEXT("x"),Location.X);
        MotionFrameEvidence.Add(MakeShared<FJsonValueObject>(Frame));
        if ((MotionSamples.Num()==1 && MotionDelta>=.5) || (MotionSamples.Num()==2 && MotionDelta>=1.))
        {
            const auto Previous=MotionSamples.Last()->AsObject();
            if (MotionFrames-Previous->GetNumberField(TEXT("effectiveFrames"))<2 || Location.X<=Previous->GetArrayField(TEXT("location"))[0]->AsNumber()) {Fail(TEXT("Two intervals must each show positive movement over multiple admitted frames"));return;}
            MotionSample(Actor);
        }
    }
    void BeginStop()
    {
        bStopping = true;
        if (GEditor && bOwnsPlaySession)
        {
            FEndPlayMapCommand EndCommand;
            EndCommand.Update();
        }
    }

    void Fail(const FString& Reason)
    {
        Reasons.Add(Reason);
        Test.AddError(Reason);
    }

    bool Finish()
    {
        RemoveMotionObserver();
        Capture.Stop();
        if (bObservationCompleted)
        {
            if (Capture.MessageCount != 1)
            {
                Fail(FString::Printf(TEXT("Expected exactly one engine Blueprint message; observed %d."), Capture.MessageCount));
            }
            if (Capture.AttributedMessageCount != 1 || Capture.AttributionFailures.Num() != 0)
            {
                Fail(TEXT("The observed message could not be uniquely attributed to the target Actor's live PIE script execution."));
            }
            if (!bPIEStopped)
            {
                Fail(TEXT("The test-owned PIE session was not cleanly stopped."));
            }
        }
        else if (Reasons.IsEmpty())
        {
            Fail(TEXT("The required runtime observation did not complete."));
        }
        if (Reasons.IsEmpty() && Test.HasAnyErrors())
        {
            Reasons.Add(TEXT("The automation framework recorded additional errors; consult the editor automation log."));
        }

        const bool bPassed = bObservationCompleted && bPIEStopped && Reasons.IsEmpty() && !Test.HasAnyErrors();
        TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
        Report->SetStringField(TEXT("test"), TEXT("NodeForge.BlueprintRuntime"));
        Report->SetStringField(TEXT("observationMode"),Expectations.bContinuousX?TEXT("ContinuousX"):TEXT("Once"));
        Report->SetStringField(TEXT("samplingPhase"),TEXT("matching OnWorldPreActorTick admission and OnWorldPostActorTick after all tick groups"));
        Report->SetArrayField(TEXT("trajectory"),MotionSamples);Report->SetArrayField(TEXT("effectiveFrameDeltas"),MotionFrameEvidence);
        Report->SetNumberField(TEXT("effectiveDeltaAccumulated"),MotionDelta);Report->SetNumberField(TEXT("positionToleranceCm"),.001);
        Report->SetBoolField(TEXT("observerDrivesActor"),false);Report->SetBoolField(TEXT("motionDelegatesRemoved"),!PreTickHandle.IsValid() && !PostTickHandle.IsValid());
        Report->SetStringField(TEXT("status"), bPassed ? TEXT("passed") : TEXT("failed"));
        Report->SetStringField(TEXT("check"), TEXT("runtime"));
        Report->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
        Report->SetStringField(TEXT("requestedMap"), Expectations.TestMap);
        Report->SetBoolField(TEXT("graphConfigRequested"), bGraphConfigRequested);
        Report->SetStringField(TEXT("configPath"), ConfigPath);
        Report->SetStringField(TEXT("configValidation"), ConfigValidation);
        Report->SetStringField(TEXT("blueprint"), Expectations.Blueprint);
        Report->SetStringField(TEXT("expectedMessage"), Expectations.ExpectedPayload);
        Report->SetStringField(TEXT("actualMap"), ActualEditorMap);
        Report->SetStringField(TEXT("actualPIEMap"), ActualPIEMap);
        Report->SetStringField(TEXT("worldType"), WorldType);
        Report->SetStringField(TEXT("expectedClass"), Expectations.TestActorClass);
        Report->SetStringField(TEXT("actualActor"), ActualActorPath);
        Report->SetStringField(TEXT("actualClass"), ActualClassPath);
        Report->SetNumberField(TEXT("actorCount"), ActorCount);
        Report->SetBoolField(TEXT("hasActorBegunPlay"), bActorBegunPlay);
        Report->SetBoolField(TEXT("locationCheckRequested"), bLocationRequested);
        auto VectorEvidence = [](const FVector& V) -> TArray<TSharedPtr<FJsonValue>>
        { return {MakeShared<FJsonValueNumber>(V.X), MakeShared<FJsonValueNumber>(V.Y), MakeShared<FJsonValueNumber>(V.Z)}; };
        Report->SetArrayField(TEXT("expectedActorLocation"), VectorEvidence(ExpectedLocation));
        Report->SetArrayField(TEXT("actualActorLocation"), VectorEvidence(ActualLocation));
        Report->SetBoolField(TEXT("rotationCheckRequested"), bRotationRequested);
        Report->SetArrayField(TEXT("expectedActorRotation"), VectorEvidence(FVector(ExpectedRotation.Pitch, ExpectedRotation.Yaw, ExpectedRotation.Roll)));
        Report->SetArrayField(TEXT("actualActorRotation"), VectorEvidence(FVector(ActualRotation.Pitch, ActualRotation.Yaw, ActualRotation.Roll)));
        Report->SetBoolField(TEXT("scaleCheckRequested"), bScaleRequested);
        Report->SetArrayField(TEXT("expectedActorScale"), VectorEvidence(ExpectedScale));
        Report->SetArrayField(TEXT("actualActorScale"), VectorEvidence(ActualScale));
        Report->SetNumberField(TEXT("messageCount"), Capture.MessageCount);
        Report->SetNumberField(TEXT("attributedMessageCount"), Capture.AttributedMessageCount);
        Report->SetStringField(TEXT("sourceActor"), Capture.SourceActorPath);
        Report->SetStringField(TEXT("sourceClass"), Capture.SourceClassPath);
        Report->SetStringField(TEXT("sourceWorld"), Capture.SourceWorldPath);
        Report->SetStringField(TEXT("sourceFunction"), Capture.SourceFunction);
        Report->SetNumberField(TEXT("observedSeconds"), ObservedSeconds);
        Report->SetNumberField(TEXT("elapsedSeconds"), FPlatformTime::Seconds() - StartedAt);
        Report->SetBoolField(TEXT("pieStopped"), bPIEStopped);
        Report->SetBoolField(TEXT("loggerRemoved"), !Capture.IsRegistered());
        Report->SetStringField(TEXT("reason"), bPassed ? TEXT("All runtime assertions passed.") : FString::Join(Reasons, TEXT("; ")));
        TArray<TSharedPtr<FJsonValue>> Messages;
        for (const FString& Raw : Capture.RawMessages)
        {
            Messages.Add(MakeShared<FJsonValueString>(Raw));
        }
        Report->SetArrayField(TEXT("engineMessages"), Messages);
        TArray<TSharedPtr<FJsonValue>> AttributionFailures;
        for (const FString& Reason : Capture.AttributionFailures)
        {
            AttributionFailures.Add(MakeShared<FJsonValueString>(Reason));
        }
        Report->SetArrayField(TEXT("attributionFailures"), AttributionFailures);

        if (bEvidencePathOccupied || !NodeForgePrototype::WriteJson(Report,ReportPath))
        {
            Test.AddError(FString::Printf(TEXT("Runtime evidence could not be written to %s."), *ReportPath));
        }
        bFinished = true;
        return true;
    }

    FDelegateHandle PreTickHandle,PostTickHandle;
    TWeakObjectPtr<AActor> MotionTarget;TWeakObjectPtr<UWorld> MotionWorld;
    FVector MotionOrigin=FVector::ZeroVector,MotionScale=FVector::OneVector;FRotator MotionRotation=FRotator::ZeroRotator;
    TArray<TSharedPtr<FJsonValue>> MotionSamples,MotionFrameEvidence;
    double MotionDelta=0.;float FrameDelta=0.;int32 MotionFrames=0;bool bMotionFrame=false;
    FAutomationTestBase& Test;
    FRuntimeExpectations Expectations;
    FRuntimeMessageCapture Capture;
    FString ReportPath;
    FString ConfigPath;
    FString ConfigValidation = TEXT("notRequested");
    FString ActualEditorMap;
    FString ActualPIEMap;
    FString ActualActorPath;
    FString ActualClassPath;
    FString WorldType = TEXT("notObserved");
    FString ExpectedLocationText;
    FVector ExpectedLocation = FVector::ZeroVector;
    FVector ActualLocation = FVector::ZeroVector;
    bool bLocationRequested = false;
    FString ExpectedRotationText, ExpectedScaleText;
    FRotator ExpectedRotation = FRotator::ZeroRotator, ActualRotation = FRotator::ZeroRotator;
    FVector ExpectedScale = FVector::OneVector, ActualScale = FVector::OneVector;
    bool bRotationRequested = false, bScaleRequested = false;
    TArray<FString> Reasons;
    double StartedAt = 0.0;
    double BeginPlayObservedAt = -1.0;
    double ObservedSeconds = 0.0;
    int32 ActorCount = 0;
    bool bActorBegunPlay = false;
    bool bStarted = false;
    bool bOwnsPlaySession = false;
    bool bStopping = false;
    bool bPIEStopped = false;
    bool bObservationCompleted = false;
    bool bFinished = false;
    bool bEvidencePathOccupied = false;
    bool bGraphConfigRequested = false;
};
} // namespace NodeForgeRuntimeTest

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeBlueprintRuntimeTest, "NodeForge.BlueprintRuntime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNodeForgeBlueprintRuntimeTest::RunTest(const FString& Parameters)
{
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<NodeForgeRuntimeTest::FRunBlueprintPIE>(*this));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeMinimalPanelTest, "NodeForge.MinimalPanel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNodeForgeMinimalPanelTest::RunTest(const FString& Parameters)
{
    FString ReportPath;
    if (!FParse::Value(FCommandLine::Get(), TEXT("NFPanelReport="), ReportPath))
    {
        ReportPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("NodeForge/MinimalPanel.json"));
    }
    ReportPath = FPaths::ConvertRelativePathToFull(ReportPath);
    if (IFileManager::Get().FileExists(*ReportPath))
    {
        AddError(TEXT("Panel evidence already exists; use a new NFPanelReport path to preserve the earlier run."));
        return false;
    }

    bool bTabOpened = false;
    bool bTabClosed = false;
    bool bContentValid = false;
    bool bExistingTabPreserved = true;
    int32 WidgetCount = 0;
    int32 ButtonCount = 0;
    int32 ResultFieldCount = 0;
    FString ContentType;
    FString Reason;

    if (IsRunningCommandlet() || !FSlateApplication::IsInitialized())
    {
        Reason = TEXT("An initialized Editor Slate process is required.");
    }
    else
    {
        const TSharedPtr<SDockTab> Existing=FGlobalTabmanager::Get()->FindExistingLiveTab(FName(TEXT("NodeForge")));
        const FName TestTab(*FString::Printf(TEXT("NodeForgePanelTest%s"),*FGuid::NewGuid().ToString(EGuidFormats::Digits)));
        FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TestTab,FOnSpawnTab::CreateLambda([](const FSpawnTabArgs&) {
            return SNew(SDockTab).TabRole(ETabRole::NomadTab)[NodeForgePanel::Create()];
        })).SetMenuType(ETabSpawnerMenuType::Hidden);
        TSharedPtr<SDockTab> Tab = FGlobalTabmanager::Get()->TryInvokeTab(TestTab);
        bTabOpened = Tab.IsValid();
        if (Tab.IsValid())
        {
            TSharedRef<SWidget> Content = Tab->GetContent();
            ContentType = Content->GetTypeAsString();
            bool bDepthExceeded = false;
            TFunction<void(const TSharedRef<SWidget>&, int32)> Visit;
            Visit = [&](const TSharedRef<SWidget>& Widget, int32 Depth)
            {
                if (Depth > 64)
                {
                    bDepthExceeded = true;
                    return;
                }
                ++WidgetCount;
                const FName Type = Widget->GetType();
                ButtonCount += Type == FName(TEXT("SButton")) ? 1 : 0;
                ResultFieldCount += Type == FName(TEXT("SMultiLineEditableTextBox")) ? 1 : 0;
                FChildren* Children = Widget->GetChildren();
                for (int32 Index = 0; Children && Index < Children->Num(); ++Index)
                {
                    Visit(Children->GetChildAt(Index), Depth + 1);
                }
            };
            Visit(Content, 0);
            bContentValid = ContentType == TEXT("SNodeForgePrototypePanel") && ButtonCount >= 2 &&
                ResultFieldCount >= 1 && !bDepthExceeded;
            const bool bCloseAccepted = Tab->RequestCloseTab();
            bTabClosed = bCloseAccepted && !FGlobalTabmanager::Get()->FindExistingLiveTab(TestTab).IsValid();
            if (!bContentValid)
            {
                Reason = TEXT("The tab did not construct the expected minimal panel, buttons, and result text field.");
            }
            else if (!bTabClosed)
            {
                Reason = TEXT("The test-created panel did not close cleanly.");
            }
        }
        else
        {
            Reason = TEXT("The registered NodeForge nomad tab could not be invoked.");
        }
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TestTab);
        bExistingTabPreserved=Existing==FGlobalTabmanager::Get()->FindExistingLiveTab(FName(TEXT("NodeForge")));
        if(!bExistingTabPreserved) Reason=TEXT("The pre-existing production tab identity changed unexpectedly.");
    }

    if (!Reason.IsEmpty())
    {
        AddError(Reason);
    }
    else if (HasAnyErrors())
    {
        Reason = TEXT("The automation framework recorded additional errors; consult the editor log.");
    }
    const bool bPassed = bTabOpened && bContentValid && bTabClosed && bExistingTabPreserved && !HasAnyErrors();
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("test"), TEXT("NodeForge.MinimalPanel"));
    Report->SetStringField(TEXT("status"), bPassed ? TEXT("passed") : TEXT("failed"));
    Report->SetStringField(TEXT("scope"), TEXT("Nomad tab invocation and Slate widget construction only; no file dialog interaction or UI import was executed."));
    Report->SetStringField(TEXT("widgetConstruction"), bTabOpened ? (bContentValid ? TEXT("passed") : TEXT("failed")) : TEXT("notRun"));
    Report->SetStringField(TEXT("nativeFileDialog"), TEXT("notRun"));
    Report->SetStringField(TEXT("importAction"), TEXT("notRun"));
    Report->SetStringField(TEXT("humanInteraction"), TEXT("notRun"));
    Report->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
    Report->SetStringField(TEXT("timeUtc"), FDateTime::UtcNow().ToIso8601());
    Report->SetBoolField(TEXT("tabOpened"), bTabOpened);
    Report->SetBoolField(TEXT("tabClosed"), bTabClosed);
    Report->SetBoolField(TEXT("preExistingProductionTabPreserved"),bExistingTabPreserved);
    Report->SetStringField(TEXT("constructionFactory"),TEXT("same NodeForgePanel::Create used by real spawner; only test-specific unique tab is closed"));
    Report->SetStringField(TEXT("contentType"), ContentType);
    Report->SetNumberField(TEXT("widgetCount"), WidgetCount);
    Report->SetNumberField(TEXT("buttonCount"), ButtonCount);
    Report->SetNumberField(TEXT("resultFieldCount"), ResultFieldCount);
    Report->SetStringField(TEXT("reason"), bPassed ? TEXT("The minimal Slate panel was constructed and closed; import interaction remains unverified.") : Reason);
    if (!NodeForgePrototype::WriteJson(Report,ReportPath))
    {
        AddError(FString::Printf(TEXT("Panel construction evidence could not be written to %s."), *ReportPath));
        return false;
    }
    return bPassed;
}

#endif
