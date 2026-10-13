#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

// A finite GT read guard, not an asset cache or a reload mechanism. Only loads
// performed by this chain establish saved-file origin for later source checks.
namespace NodeForgeResourceSource
{
constexpr int32 QueryMaximumEntries = 5000, QueryMaximumDirectories = 128, QueryMaximumDepth = 16;
// Revision zero is the archived Engine/test-root policy. Current calls use the
// explicitly bound read roots; write targets remain owned by the graph core.
bool ReadPath(const FString& Path, bool Directory, const FString& Kind, int32 ReadRevision,
    FString& Filename, FString& Error);
TSharedRef<FJsonObject> QueryArgumentsSchema();
bool EffectiveQueryArguments(const TSharedPtr<FJsonObject>& Config,
    TSharedPtr<FJsonObject>& Effective, FString& Error);
bool Query(const TSharedPtr<FJsonObject>& Config, const FString& CatalogDigest,
    TSharedPtr<FJsonObject>& Evidence, FString& Error);
bool InspectTexture(const TSharedPtr<FJsonObject>& Config, TSharedPtr<FJsonObject>& Evidence, FString& Error);
bool QualifyTexture(UObject* Object, FString& Error);
bool RejectDirty(UObject* Object, FString& Error);
bool ReadText(const FString& Path, uint32 MaxBytes, FString& Text, FString& Hash, FString& Error);
bool ReadExportOrigin(const FString& ReportPath, const FString& ReportSha256, const FString& DraftText,
    bool RequireOriginalPayload, TSharedPtr<FJsonObject>& Origin, FString& Error);
class FCapture
{
public:
    FCapture() = default;
    ~FCapture();
    FCapture(const FCapture&) = delete;
    FCapture& operator=(const FCapture&) = delete;
    bool Begin(const FString& Source, const FString& Kind, FString& Error,
        int32 ReadRevision = 1, bool RequireOrigin = true);
    bool Verify(FString& Error) const;
    UObject* Object() const { return Captured.Get(); }
    const FString& Path() const { return SourcePath; }
    const FString& Sha256() const { return FileDigest; }
private:
    void* FileHandle = nullptr;
    TWeakObjectPtr<UObject> Captured;
    FString SourcePath;
    FString Filename;
    FString FileDigest;
};
}
