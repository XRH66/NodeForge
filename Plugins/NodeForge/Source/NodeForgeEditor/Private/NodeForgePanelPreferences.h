#pragma once
#include "CoreMinimal.h"

// UI preferences only. Graph fields, source proofs and transport credentials stay
// in their existing shared modules, never in this project-local preferences file.
struct FNodeForgeClientDefinition
{
    FString Id, Name, Description, IconPath;
    TArray<uint8> PendingIconPng; // Draft only; persisted in project Settings on Apply.
};
struct FNodeForgePanelPreferences
{
    FString Directory = TEXT("/Game/NodeForgeTests");
    FString Client = TEXT("codex");
    TArray<FNodeForgeClientDefinition> CustomClients;
    bool Load(FString& Error);
    bool Save(FString& Error);
    bool Validate(FString& Error) const;
    bool Equals(const FNodeForgePanelPreferences& Other) const;
    TArray<FNodeForgeClientDefinition> Clients() const;
    static bool DecodeIcon(const TArray<uint8>& Bytes, TArray<uint8>& Png, FString& Error);
};
