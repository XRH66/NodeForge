#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "NodeForgeGraphDocument.h"

class UClass;
class UFunction;

// Shared production definitions, not a separate generator or MCP allowlist.
namespace NodeForgeNodeDefinitions
{
enum class EContext : uint8 { External, NativeCatalogProbe };
enum class EAdmission : uint8 { PendingNative, Admitted, Rejected };

struct FCapabilities
{
    int32 MaterialRevision = -1;
    int32 BlueprintRevision = -1;
    bool AllowsMaterialInstance = false;
    int32 MaximumGraphVersion = 2;
    int32 ResourceReadRevision = 0;
    bool IsKnown() const { return MaterialRevision >= 0 && BlueprintRevision >= 0; }
};

struct FBlueprintEventDefinition
{
    UClass* Owner;
    FString Member;
    int32 MinimumRevision;
    bool DerivesCanEverTick;
};

struct FBlueprintFunctionDefinition
{
    UClass* Owner;
    FString Member;
    FString Family;
    int32 MinimumRevision = 0;
    bool IsBulk = false;
    EAdmission Admission = EAdmission::Admitted;
    FString QualificationError;
};

struct FMaterialDefinition
{
    FString Class;
    TArray<FString> Properties;
    int32 MinimumRevision = 0;
    bool RequiresFunctionBinding = false;
    bool ConsumesInputs = true;
    FString Family;
    bool IsBulk = false;
    EAdmission Admission = EAdmission::Admitted;
    FString QualificationError;
};

struct FSpecialDefinition
{
    FString Class;
    int32 MinimumRevision;
    NodeForgeGraphDocument::EBindingKind Binding = NodeForgeGraphDocument::EBindingKind::None;
};

struct FMemberDefinition
{
    FString Type;
    int32 MinimumRevision;
};

const TArray<FBlueprintEventDefinition>& Events();
const FBlueprintEventDefinition* Event(const FString& Owner, const FString& Member);
const TArray<FBlueprintFunctionDefinition>& Functions();
const TArray<FMaterialDefinition>& Materials();
const TArray<FSpecialDefinition>& Specials();
const TArray<FMemberDefinition>& Members();
const FBlueprintFunctionDefinition* Function(const FString& Owner, const FString& Member);
const FMaterialDefinition* Material(const FString& Class);
const FSpecialDefinition* Special(const FString& Class);
int32 MemberMinimumRevision(const FString& Type);
bool EligibleExistingFunction(const UFunction* Function);
void QualifyFunction(const FString& Owner, const FString& Member, const FString& Error);
void QualifyMaterial(const FString& Class, const FString& Error);
TArray<TSharedPtr<FJsonValue>> Exclusions(const FString& AssetType);
FCapabilities Capabilities(const FString& Digest, EContext Context = EContext::External);
bool AllowsGraphVersion(const FString& Digest, int32 Version, EContext Context = EContext::External);
bool AllowsRequest(const FString& Digest, const FString& AssetType, int32 Version,
    const FString& CurrentDigest);
void RegisterCurrentDigest(const FString& Digest);
}
