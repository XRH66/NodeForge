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
    TArray<TSharedPtr<FJsonValue>> ConnectedClients() const;
    bool HasConnectedClient(const FString& Client) const;
    void DisconnectClient(const FString& Client); // Ends only matching authenticated MCP peers, never the project service.
    bool CheckRestrictedConnection(TSharedRef<FJsonObject>& Evidence) const;
#if WITH_DEV_AUTOMATION_TESTS
    void SetBeforeDrainProbeForChecks(TFunction<void()> Probe);
#endif
private:
    friend class FNodeForgeCallCapacityTest;
    friend class FNodeForgeAutoCallCapacityTest;
    struct FImpl;
    TUniquePtr<FImpl> Impl;
};
