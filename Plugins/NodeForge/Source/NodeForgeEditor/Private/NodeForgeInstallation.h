#pragma once
#include "CoreMinimal.h"

// Explicit selected-project binding; no permission inferred from a plugin path.
namespace NodeForgeInstallation
{
struct FBinding
{
    FString InstallationRoot, Project, PluginDirectory, EngineRoot, BinarySha256, Digest;
    TArray<FString> ReadRoots;
};
bool Read(FBinding& Binding, FString& Error);
// First-use UI uses the same identity checks as all native consumers.
bool InspectCurrent(FBinding& Binding, FString& Error);
bool ConfirmCurrent(FString& ReceiptPath, FString& Error);
// Published native catalog, hash-bound to this product/DLL. Metadata only;
// discovery must not construct graph nodes/CDOs to obtain an identity.
bool CatalogIdentity(FString& CatalogDigest, FString& Error);
}
