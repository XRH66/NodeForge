#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
class FNodeForgeEditorBusinessGate;
class FNodeForgeEditorControlService;

// Module-owned transport. Workers hold immutable JSON text, never a service/widget/UObject.
class FNodeForgeEditorControlChannel
{
public:
    FNodeForgeEditorControlChannel();
    ~FNodeForgeEditorControlChannel();
    bool Start(FString& Error);
    void SetBusinessGate(const TSharedRef<FNodeForgeEditorBusinessGate>& Gate);
    void Drain(FNodeForgeEditorControlService& Service);
    void Publish(const TSharedRef<FJsonObject>& State, const TSharedRef<FJsonObject>& Draft);
    bool Stop(); // Bounded; pending I/O and its storage remain owned if completion is not proved.
    void RequestStop(); // Also safe from a synchronous GT business callback; never joins there.
    bool HasStopRequest() const;
    bool IsStopped() const;
    TSharedRef<FJsonObject> PublicIdentity() const;
    bool CheckRestrictedConnection(TSharedRef<FJsonObject>& Evidence) const;
#if WITH_DEV_AUTOMATION_TESTS
    void SetBeforeDrainProbeForChecks(TFunction<void()> Probe);
#endif
private:
    struct FImpl;
    TUniquePtr<FImpl> Impl;
};
