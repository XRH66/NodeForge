#include "NodeForgeResourceSource.h"
#include "NodeForgePrototype.h"
#include "NodeForgeInstallation.h"
#include "NodeForgeNodeDefinitions.h"
#include "NodeForgeGraphCodec.h"
#include "NodeForgeEditorControlProtocol.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialFunction.h"
#include "Engine/Blueprint.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialExpressionTextureBase.h"
#include "UObject/UnrealType.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/EngineVersion.h"
#include "Misc/ScopeExit.h"
#include "Windows/WindowsHWrapper.h"

namespace NodeForgeResourceSource
{
namespace
{
struct FOrigin { TWeakObjectPtr<UObject> Object; FString Sha256; };
TMap<FString, FOrigin> Origins; // GT-owned, bounded to this process and exact object identities.
constexpr int32 QueryDirectoryLength = 256, QueryNameLength = 128, QueryCursorLength = 512;
constexpr int32 QueryDefaultLimit = 50, QueryMaximumLimit = 200;
FString QueryClass(const FString& Kind)
{
    // Literal class paths keep discovery free of native class/CDO construction.
    return Kind == TEXT("Texture2D") ? TEXT("/Script/Engine.Texture2D")
        : Kind == TEXT("MaterialFunction") ? TEXT("/Script/Engine.MaterialFunction")
        : Kind == TEXT("Material") ? TEXT("/Script/Engine.Material") : TEXT("");
}
bool RejectReparse(const FString& Filename, FString& Error)
{
    FString Parent = Filename;
    while (!Parent.IsEmpty())
    {
        const DWORD Attributes = GetFileAttributesW(*Parent);
        if (Attributes != INVALID_FILE_ATTRIBUTES && (Attributes & FILE_ATTRIBUTE_REPARSE_POINT))
        { Error = TEXT("sourceReparseRejected"); return false; }
        const FString Next = FPaths::GetPath(Parent); if (Next == Parent) break; Parent = Next;
    }
    return true;
}
}
bool ReadPath(const FString& Path, bool Directory, const FString& Kind, int32 ReadRevision,
    FString& Filename, FString& Error)
{
    check(IsInGameThread()); Filename.Empty();
    FString Package = Path, Name;
    if (Path.Len() > (Directory ? 256 : 512) || Path.Contains(TEXT("..")) || Path.Contains(TEXT(":"))
        || Path.Contains(TEXT("\\")) || Path.Contains(TEXT("//")) || Path.EndsWith(TEXT("/"))
        || (!Directory && (!Path.Split(TEXT("."), &Package, &Name) || Name.Contains(TEXT("."))
            || Name != FPackageName::GetShortName(Package)))
        || !FPackageName::IsValidLongPackageName(Package)
        || !(Package.StartsWith(TEXT("/Game/"), ESearchCase::CaseSensitive)
            || Package.StartsWith(TEXT("/Engine/"), ESearchCase::CaseSensitive)))
    { Error = TEXT("sourcePathRejected"); return false; }
    auto Within = [&](const FString& Root)
    { return (Directory && Package == Root) || Package.StartsWith(Root + TEXT("/"), ESearchCase::CaseSensitive); };
    bool Allowed = false;
    if (Kind == TEXT("Blueprint") || ReadRevision == 0)
        Allowed = Within(TEXT("/Game/NodeForgeTests")) || (Kind != TEXT("Blueprint") && Within(TEXT("/Engine")));
    else if (ReadRevision == 1)
    {
        NodeForgeInstallation::FBinding Binding;
        if (!NodeForgeInstallation::Read(Binding, Error)) return false;
        for (const FString& Root : Binding.ReadRoots) Allowed |= Within(Root);
    }
    if (!Allowed) { Error = TEXT("sourceOutsideReadRoots"); return false; }
    FString Mapped;
    if (!FPackageName::TryConvertLongPackageNameToFilename(Package, Mapped, Directory ? TEXT("") : FPackageName::GetAssetPackageExtension()))
    { Error = TEXT("sourceMountUnavailable"); return false; }
    Filename = FPaths::ConvertRelativePathToFull(Mapped); FPaths::NormalizeFilename(Filename);
    const FString Root = FPaths::ConvertRelativePathToFull(Package.StartsWith(TEXT("/Engine/"))
        ? FPaths::EngineContentDir() : FPaths::ProjectContentDir());
    if (!FPaths::IsUnderDirectory(Filename, Root)) { Error = TEXT("sourceFileOutsideMappedRoot"); return false; }
    return RejectReparse(Filename, Error);
}
bool QualifyTexture(UObject* Object, FString& Error)
{
    auto* Texture = Cast<UTexture2D>(Object);
    if (!Texture || Texture->GetClass() != UTexture2D::StaticClass() || Texture->VirtualTextureStreaming)
    { Error = TEXT("sourceTextureClassOrVirtualRejected"); return false; }
    return RejectDirty(Texture, Error);
}
TSharedRef<FJsonObject> QueryArgumentsSchema()
{
    auto Schema = MakeShared<FJsonObject>();
    Schema->SetStringField(TEXT("type"), TEXT("object"));
    Schema->SetBoolField(TEXT("additionalProperties"), false);
    auto Properties = MakeShared<FJsonObject>();
    auto Field = [&](const TCHAR* Name, const TCHAR* Type)
    {
        auto Value = MakeShared<FJsonObject>(); Value->SetStringField(TEXT("type"), Type);
        Properties->SetObjectField(Name, Value); return Value;
    };
    Field(TEXT("kind"), TEXT("string"))->SetArrayField(TEXT("enum"),
        {MakeShared<FJsonValueString>(TEXT("Texture2D")), MakeShared<FJsonValueString>(TEXT("MaterialFunction")), MakeShared<FJsonValueString>(TEXT("Material"))});
    auto Directory = Field(TEXT("directory"), TEXT("string"));
    Directory->SetNumberField(TEXT("minLength"), 1); Directory->SetNumberField(TEXT("maxLength"), QueryDirectoryLength);
    auto Name = Field(TEXT("nameContains"), TEXT("string"));
    Name->SetNumberField(TEXT("maxLength"), QueryNameLength); Name->SetStringField(TEXT("default"), TEXT(""));
    Field(TEXT("recursive"), TEXT("boolean"))->SetBoolField(TEXT("default"), false);
    auto Limit = Field(TEXT("limit"), TEXT("integer"));
    Limit->SetNumberField(TEXT("minimum"), 1); Limit->SetNumberField(TEXT("maximum"), QueryMaximumLimit);
    Limit->SetNumberField(TEXT("default"), QueryDefaultLimit);
    auto After = Field(TEXT("after"), TEXT("string"));
    After->SetNumberField(TEXT("maxLength"), QueryCursorLength); After->SetStringField(TEXT("default"), TEXT(""));
    Schema->SetObjectField(TEXT("properties"), Properties);
    Schema->SetArrayField(TEXT("required"), {MakeShared<FJsonValueString>(TEXT("kind")), MakeShared<FJsonValueString>(TEXT("directory"))});
    return Schema;
}
bool EffectiveQueryArguments(const TSharedPtr<FJsonObject>& Config,
    TSharedPtr<FJsonObject>& Effective, FString& Error)
{
    Effective.Reset(); Error.Empty();
    FString Kind, Directory, NameContains, After; bool Recursive = false; double LimitNumber = QueryDefaultLimit;
    if (!Config || !Config->TryGetStringField(TEXT("kind"), Kind)
        || !Config->TryGetStringField(TEXT("directory"), Directory))
    { Error = TEXT("resourceQueryArgumentsRejected"); return false; }
    for (const auto& Field : Config->Values)
        if (Field.Key != TEXT("kind") && Field.Key != TEXT("directory") && Field.Key != TEXT("nameContains")
            && Field.Key != TEXT("recursive") && Field.Key != TEXT("limit") && Field.Key != TEXT("after"))
        { Error = TEXT("resourceQueryUnknownField"); return false; }
    if ((Config->HasField(TEXT("nameContains")) && !Config->TryGetStringField(TEXT("nameContains"), NameContains))
        || (Config->HasField(TEXT("after")) && !Config->TryGetStringField(TEXT("after"), After))
        || (Config->HasField(TEXT("recursive")) && !Config->TryGetBoolField(TEXT("recursive"), Recursive))
        || (Config->HasField(TEXT("limit")) && !Config->TryGetNumberField(TEXT("limit"), LimitNumber))
        || Directory.IsEmpty() || Directory.Len() > QueryDirectoryLength
        || NameContains.Len() > QueryNameLength || After.Len() > QueryCursorLength || !FMath::IsFinite(LimitNumber)
        || LimitNumber < 1 || LimitNumber > QueryMaximumLimit || FMath::FloorToDouble(LimitNumber) != LimitNumber)
    { Error = TEXT("resourceQueryArgumentsRejected"); return false; }
    if (QueryClass(Kind).IsEmpty()) { Error = TEXT("resourceQueryKindRejected"); return false; }
    Effective = MakeShared<FJsonObject>();
    Effective->SetStringField(TEXT("kind"), Kind); Effective->SetStringField(TEXT("directory"), Directory);
    Effective->SetStringField(TEXT("nameContains"), NameContains); Effective->SetStringField(TEXT("after"), After);
    Effective->SetBoolField(TEXT("recursive"), Recursive); Effective->SetNumberField(TEXT("limit"), LimitNumber);
    return true;
}
bool Query(const TSharedPtr<FJsonObject>& Config, const FString& CatalogDigest,
    TSharedPtr<FJsonObject>& Evidence, FString& Error)
{
    check(IsInGameThread()); Evidence.Reset();
    TSharedPtr<FJsonObject> Filter;
    if (!EffectiveQueryArguments(Config, Filter, Error)) return false;
    if (CatalogDigest.Len() != 64) { Error = TEXT("resourceQueryCatalogRejected"); return false; }
    const FString Kind = Filter->GetStringField(TEXT("kind")), Directory = Filter->GetStringField(TEXT("directory"));
    const FString NameContains = Filter->GetStringField(TEXT("nameContains")), After = Filter->GetStringField(TEXT("after"));
    const bool Recursive = Filter->GetBoolField(TEXT("recursive"));
    const int32 Limit = static_cast<int32>(Filter->GetNumberField(TEXT("limit")));
    const FString Class = QueryClass(Kind); FString Physical;
    if (!ReadPath(Directory, true, Kind, 1, Physical, Error)) return false;
    if (!After.IsEmpty())
    {
        FString Ignored, Package, Name;
        if (!ReadPath(After, false, Kind, 1, Ignored, Error) || !After.Split(TEXT("."), &Package, &Name)
            || !(Package.StartsWith(Directory + TEXT("/"), ESearchCase::CaseSensitive))
            || (!Recursive && FPackageName::GetLongPackagePath(Package) != Directory)
            || (!NameContains.IsEmpty() && !Name.Contains(NameContains, ESearchCase::IgnoreCase)))
        { Error = TEXT("resourceQueryCursorRejected"); return false; }
    }
    NodeForgeInstallation::FBinding Binding;
    if (!NodeForgeInstallation::Read(Binding, Error)) return false;
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    const bool ScanningBefore = Registry.IsLoadingAssets();
    int32 Entries = 0, Directories = 0; FString Incomplete;
    TArray<FAssetData> Matches;
    TFunction<bool(const FString&, const FString&, int32)> Visit;
    Visit = [&](const FString& NativeDirectory, const FString& PackageDirectory, int32 Depth)
    {
        if (Directories >= QueryMaximumDirectories || Depth > QueryMaximumDepth) { Incomplete = TEXT("scanLimit"); return false; }
        ++Directories;
        if (!RejectReparse(NativeDirectory, Error)) { Incomplete = TEXT("reparse"); Error.Empty(); return false; }
        WIN32_FIND_DATAW Found;
        HANDLE Search = FindFirstFileW(*(NativeDirectory / TEXT("*")), &Found);
        if (Search == INVALID_HANDLE_VALUE)
        {
            if (GetLastError() != ERROR_FILE_NOT_FOUND) Incomplete = TEXT("directoryIO");
            return Incomplete.IsEmpty();
        }
        ON_SCOPE_EXIT { FindClose(Search); };
        do
        {
            const FString Name(Found.cFileName);
            if (Name == TEXT(".") || Name == TEXT("..")) continue;
            if (Entries >= QueryMaximumEntries) { Incomplete = TEXT("scanLimit"); return false; }
            ++Entries;
            if (Found.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) { Incomplete = TEXT("reparse"); return false; }
            if (Found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (Recursive && !Visit(NativeDirectory / Name, PackageDirectory / Name, Depth + 1)) return false;
                continue;
            }
            if (!Name.EndsWith(TEXT(".uasset"), ESearchCase::IgnoreCase)) continue;
            const FString AssetName = FPaths::GetBaseFilename(Name), Package = PackageDirectory / AssetName;
            if (!FPackageName::IsValidLongPackageName(Package)) continue;
            const FString ObjectPath = Package + TEXT(".") + AssetName;
            const FAssetData Data = Registry.GetAssetByObjectPath(FSoftObjectPath(ObjectPath), true, true);
            if (!Data.IsValid()) { if (Incomplete.IsEmpty()) Incomplete = TEXT("metadataUnavailable"); continue; }
            if (Data.IsRedirector() || Data.GetObjectPathString() != ObjectPath || Data.AssetClassPath.ToString() != Class
                || (!NameContains.IsEmpty() && !AssetName.Contains(NameContains, ESearchCase::IgnoreCase))) continue;
            Matches.Add(Data);
        } while (FindNextFileW(Search, &Found));
        if (GetLastError() != ERROR_NO_MORE_FILES) { Incomplete = TEXT("directoryIO"); return false; }
        return true;
    };
    Visit(Physical, Directory, 0);
    const bool Scanning = ScanningBefore || Registry.IsLoadingAssets();
    const bool Complete = !Scanning && Incomplete.IsEmpty();
    Matches.Sort([](const FAssetData& A, const FAssetData& B)
        { return A.GetObjectPathString().Compare(B.GetObjectPathString(), ESearchCase::CaseSensitive) < 0; });
    TArray<TSharedPtr<FJsonValue>> Rows; FString Next; int32 Remaining = 0;
    for (const FAssetData& Data : Matches)
    {
        const FString ObjectPath = Data.GetObjectPathString();
        if (!After.IsEmpty() && ObjectPath.Compare(After, ESearchCase::CaseSensitive) <= 0) continue;
        ++Remaining;
        if (Rows.Num() >= Limit) continue;
        auto Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("objectPath"), ObjectPath); Row->SetStringField(TEXT("packagePath"), Data.PackagePath.ToString());
        Row->SetStringField(TEXT("class"), Data.AssetClassPath.ToString()); Row->SetStringField(TEXT("name"), Data.AssetName.ToString());
        Row->SetBoolField(TEXT("discovered"), true); Row->SetBoolField(TEXT("policyAllowed"), true);
        Row->SetBoolField(TEXT("inspected"), false); Row->SetStringField(TEXT("graphExportable"), TEXT("unknown"));
        Row->SetBoolField(TEXT("requiresInspect"), true); Rows.Add(MakeShared<FJsonValueObject>(Row)); Next = ObjectPath;
    }
    Evidence = MakeShared<FJsonObject>(); Evidence->SetObjectField(TEXT("filter"), Filter);
    Evidence->SetStringField(TEXT("catalogDigest"), CatalogDigest); Evidence->SetStringField(TEXT("binarySha256"), Binding.BinarySha256);
    Evidence->SetStringField(TEXT("installationBindingDigest"), Binding.Digest); Evidence->SetNumberField(TEXT("readPolicyRevision"), 1);
    Evidence->SetBoolField(TEXT("registryScanning"), Scanning); Evidence->SetBoolField(TEXT("complete"), Complete);
    Evidence->SetBoolField(TEXT("truncated"), !Complete || Remaining > Rows.Num());
    Evidence->SetStringField(TEXT("nextAfter"), Complete && Remaining > Rows.Num() ? Next : TEXT(""));
    Evidence->SetStringField(TEXT("incompleteReason"), Scanning ? TEXT("registryScanning") : Incomplete);
    Evidence->SetNumberField(TEXT("entriesVisited"), Entries); Evidence->SetNumberField(TEXT("directoriesVisited"), Directories);
    Evidence->SetNumberField(TEXT("matches"), Matches.Num()); Evidence->SetNumberField(TEXT("returned"), Rows.Num());
    Evidence->SetArrayField(TEXT("rows"), Rows);
    Evidence->SetStringField(TEXT("paginationPolicy"), TEXT("stateless sorted path cursor; directory changes can repeat/omit rows; incomplete traversal has no continuation, narrow the directory"));
    return true;
}
bool InspectTexture(const TSharedPtr<FJsonObject>& Config, TSharedPtr<FJsonObject>& Evidence, FString& Error)
{
    FString Source; Evidence.Reset();
    if (!Config || Config->Values.Num() != 1 || !Config->TryGetStringField(TEXT("texture"), Source))
    { Error = TEXT("textureInspectArgumentsRejected"); return false; }
    FCapture Capture;
    if (!Capture.Begin(Source, TEXT("Texture2D"), Error)) return false;
    auto* Texture = CastChecked<UTexture2D>(Capture.Object());
    const auto* Property = FindFProperty<FByteProperty>(UMaterialExpressionTextureBase::StaticClass(), TEXT("SamplerType"));
    const FString Sampler = Property->Enum->GetNameStringByValue(UMaterialExpressionTextureBase::GetSamplerTypeForTexture(Texture));
    auto Properties = MakeShared<FJsonObject>(); Properties->SetStringField(TEXT("Texture"), Source); Properties->SetStringField(TEXT("SamplerType"), Sampler);
    Evidence = MakeShared<FJsonObject>(); Evidence->SetStringField(TEXT("source"), Source);
    Evidence->SetStringField(TEXT("savedSourceSha256"), Capture.Sha256()); Evidence->SetStringField(TEXT("class"), Texture->GetClass()->GetPathName());
    Evidence->SetNumberField(TEXT("width"), Texture->GetSizeX()); Evidence->SetNumberField(TEXT("height"), Texture->GetSizeY());
    Evidence->SetBoolField(TEXT("sRGB"), Texture->SRGB); Evidence->SetNumberField(TEXT("compressionSettings"), Texture->CompressionSettings);
    Evidence->SetBoolField(TEXT("virtualTextureStreaming"), Texture->VirtualTextureStreaming); Evidence->SetStringField(TEXT("nativeSamplerType"), Sampler);
    Evidence->SetStringField(TEXT("sampleQualification"), TEXT("saved clean exact nonvirtual Texture2D; native inferred sampler; final shader/property suitability requires native graph compilation"));
    Evidence->SetStringField(TEXT("originProof"), TEXT("controlledSavedLoad")); Evidence->SetBoolField(TEXT("sourceFileHeldReadOnly"), true);
    Evidence->SetObjectField(TEXT("properties"), Properties);
    return Capture.Verify(Error);
}
bool RejectDirty(UObject* Object, FString& Error)
{
    check(IsInGameThread());
    if (Object && Object->GetOutermost()->IsDirty())
    { Error = TEXT("sourceDirty"); return false; }
    return true;
}
bool ReadText(const FString& Path, uint32 MaxBytes, FString& Text, FString& Hash, FString& Error)
{
    HANDLE File = CreateFileW(*Path, FILE_READ_DATA | FILE_READ_ATTRIBUTES, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (File == INVALID_HANDLE_VALUE) { Error = TEXT("inputOpenRejected"); return false; }
    ON_SCOPE_EXIT { CloseHandle(File); };
    BY_HANDLE_FILE_INFORMATION Info; LARGE_INTEGER Size; WCHAR Final[32768];
    const DWORD Count = GetFinalPathNameByHandleW(File, Final, UE_ARRAY_COUNT(Final), FILE_NAME_NORMALIZED);
    FString Actual = Count && Count < UE_ARRAY_COUNT(Final) ? FString(Final) : FString();
    Actual.RemoveFromStart(TEXT("\\\\?\\")); FPaths::NormalizeFilename(Actual);
    if (!GetFileInformationByHandle(File, &Info) || (Info.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY))
        || !Actual.Equals(Path, ESearchCase::IgnoreCase) || !GetFileSizeEx(File, &Size)
        || Size.QuadPart <= 0 || Size.QuadPart > MaxBytes)
    { Error = TEXT("inputIdentityOrSizeRejected"); return false; }
    TArray<uint8> Bytes; Bytes.SetNumUninitialized(static_cast<int32>(Size.QuadPart)); DWORD Read = 0;
    if (!ReadFile(File, Bytes.GetData(), Bytes.Num(), &Read, nullptr) || Read != static_cast<DWORD>(Bytes.Num())
        || !NodeForgeEditorControl::DecodeUtf8(Bytes, Text, MaxBytes))
    { Error = TEXT("inputReadOrEncodingRejected"); return false; }
    Hash = NodeForgePrototype::TextSha256(Text); return true;
}
bool ReadExportOrigin(const FString& Requested, const FString& ReportSha256, const FString& DraftText,
    bool RequireOriginalPayload, TSharedPtr<FJsonObject>& Origin, FString& Error)
{
    check(IsInGameThread()); Origin.Reset();
    FString Path = FPaths::ConvertRelativePathToFull(Requested); FPaths::NormalizeFilename(Path);
    if (Requested.IsEmpty() || FPaths::IsRelative(Requested) || !FPaths::CollapseRelativeDirectories(Path)
        || !FPaths::IsUnderDirectory(Path, NodeForgeInstallation::StateDirectory() / TEXT("Reports"))
        || FPaths::IsUnderDirectory(Path, NodeForgeInstallation::StateDirectory() / TEXT("Reports/PluginControlSessions")))
    { Error = TEXT("exportReportPathRejected"); return false; }
    FString Parent = Path;
    while (!Parent.IsEmpty())
    {
        const DWORD Attributes = GetFileAttributesW(*Parent);
        if (Attributes != INVALID_FILE_ATTRIBUTES && (Attributes & FILE_ATTRIBUTE_REPARSE_POINT))
        { Error = TEXT("exportReportReparseRejected"); return false; }
        const FString Next = FPaths::GetPath(Parent); if (Next == Parent) break; Parent = Next;
    }
    FString Raw, Hash;
    if (!ReadText(Path, NodeForgeEditorControl::MaxReportBytes, Raw, Hash, Error) || Hash != ReportSha256)
    { if (Error.IsEmpty()) Error = TEXT("exportReportHashMismatch"); return false; }
    auto Parse = [](const FString& Text, TSharedPtr<FJsonObject>& Out, uint32 MaxBytes)
    { FTCHARToUTF8 Utf8(*Text); return NodeForgeEditorControl::ParseObject(MakeArrayView(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length()), Out, MaxBytes); };
    TSharedPtr<FJsonObject> Report, Draft;
    const TSharedPtr<FJsonObject>* Payload = nullptr; const TSharedPtr<FJsonObject>* Evidence = nullptr;
    const TSharedPtr<FJsonObject>* Read = nullptr; const TSharedPtr<FJsonObject>* Fingerprint = nullptr;
    FString Mode, Status, Overall, Engine, Project, Text, TextHash, Source, SourceHash, Kind, BinaryHash;
    FString PayloadKind, DraftKind, Catalog, ReportCatalog, DraftCatalog, Version, Session, RequestId, Target, Format, Proof;
    double Pid=0, GraphVersion=0;bool BytesUnchanged=false,Clean=false,Held=false;
    if (!Parse(Raw, Report, NodeForgeEditorControl::MaxReportBytes) || !Parse(DraftText, Draft, NodeForgeEditorControl::MaxGraphBytes)
        || !Report->TryGetStringField(TEXT("mode"), Mode)
        || (Mode != TEXT("MaterialGraphExport") && Mode != TEXT("BlueprintGraphExport"))
        || !Report->TryGetStringField(TEXT("operationStatus"), Status) || Status != TEXT("passed")
        || !Report->TryGetStringField(TEXT("overallStatus"), Overall) || Overall != TEXT("succeeded")
        || !Report->TryGetStringField(TEXT("reportVersion"),Version) || Version!=TEXT("ResultContractDraft1")
        || !Report->TryGetStringField(TEXT("sessionId"),Session) || Session.IsEmpty()
        || !Report->TryGetNumberField(TEXT("pid"),Pid) || Pid<1 || !FMath::IsFinite(Pid)
        || !Report->TryGetStringField(TEXT("engine"), Engine) || Engine != FEngineVersion::Current().ToString()
        || !Report->TryGetStringField(TEXT("project"), Project)
        || !FPaths::ConvertRelativePathToFull(Project).Equals(FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()), ESearchCase::IgnoreCase)
        || !Report->TryGetObjectField(TEXT("readOnlyExecutionFingerprint"), Fingerprint)
        || !(*Fingerprint)->TryGetStringField(TEXT("binarySha256"), BinaryHash)
        || BinaryHash.Len()!=64
        || BinaryHash != NodeForgePrototype::FileSha256(FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor")))
        || !Report->TryGetObjectField(TEXT("exportedRequest"), Payload) || !Report->TryGetObjectField(TEXT("exportEvidence"), Evidence)
        || !Report->TryGetObjectField(TEXT("resourceRead"), Read)
        || !(*Read)->TryGetStringField(TEXT("source"), Source) || !(*Read)->TryGetStringField(TEXT("savedSourceSha256"), SourceHash)
        || !(*Read)->TryGetStringField(TEXT("kind"), Kind)
        || !(*Read)->TryGetStringField(TEXT("originProof"),Proof) || Proof!=TEXT("controlledSavedLoad")
        || !(*Read)->TryGetBoolField(TEXT("sourceFileHeldReadOnly"),Held) || !Held
        || !(*Evidence)->HasTypedField<EJson::String>(TEXT("source")) || (*Evidence)->GetStringField(TEXT("source")) != Source
        || !(*Evidence)->HasTypedField<EJson::String>(TEXT("sourceAssetSha256")) || (*Evidence)->GetStringField(TEXT("sourceAssetSha256")) != SourceHash
        || !(*Evidence)->TryGetBoolField(TEXT("sourceBytesUnchanged"),BytesUnchanged) || !BytesUnchanged
        || !(*Evidence)->TryGetBoolField(TEXT("sourceRemainsClean"),Clean) || !Clean
        || SourceHash.Len() != 64 || Kind != (Mode == TEXT("MaterialGraphExport") ? TEXT("Material") : TEXT("Blueprint"))
        || !Report->TryGetStringField(TEXT("exportedRequestText"), Text) || FTCHARToUTF8(*Text).Length() > NodeForgeEditorControl::MaxGraphBytes
        || !Report->TryGetStringField(TEXT("exportedRequestSha256"), TextHash) || TextHash != NodeForgePrototype::TextSha256(Text)
        || !(*Payload)->TryGetStringField(TEXT("catalogDigest"), Catalog)
        || !Report->TryGetStringField(TEXT("catalogDigest"), ReportCatalog) || ReportCatalog != Catalog
        || !Draft->TryGetStringField(TEXT("catalogDigest"), DraftCatalog) || DraftCatalog != Catalog)
    { Error = TEXT("exportReportIdentityRejected"); return false; }
    TSharedPtr<FJsonObject> TextObject;
    if (!Parse(Text, TextObject, NodeForgeEditorControl::MaxGraphBytes) || NodeForgeEditorControl::SemanticDigest(TextObject.ToSharedRef()) != NodeForgeEditorControl::SemanticDigest((*Payload).ToSharedRef()))
    { Error = TEXT("exportPayloadMismatch"); return false; }
    // Wire conversion is considered only after original report/text/payload bytes
    // and current native provenance have passed. Raw proof digests stay unchanged.
    NodeForgeGraphDocument::FGraphDocument PayloadDocument,DraftDocument;
    if(!NodeForgeGraphCodec::DecodeText(Text,PayloadDocument,Error)
        || !NodeForgeGraphCodec::DecodeText(DraftText,DraftDocument,Error)
        || PayloadDocument.AssetType()!=Kind || DraftDocument.AssetType()!=Kind)
    {Error=TEXT("exportReportIdentityRejected");return false;}
    const bool Original=NodeForgeGraphCodec::EqualOriginal(PayloadDocument,DraftDocument);
    if(RequireOriginalPayload && !Original){Error=TEXT("exportPayloadMismatch");return false;}
    RequestId=PayloadDocument.RequestId;Target=PayloadDocument.Target;
    Origin = MakeShared<FJsonObject>(); Origin->SetStringField(TEXT("reportPath"), Path); Origin->SetStringField(TEXT("reportSha256"), Hash);
    Origin->SetStringField(TEXT("source"), Source); Origin->SetStringField(TEXT("sourceAssetSha256"), SourceHash); Origin->SetStringField(TEXT("assetKind"), Kind);
    Origin->SetStringField(TEXT("payloadSha256"), TextHash); Origin->SetStringField(TEXT("payloadSemanticDigest"), NodeForgeEditorControl::SemanticDigest((*Payload).ToSharedRef()));
    Origin->SetStringField(TEXT("originalRequestId"), RequestId);
    Origin->SetStringField(TEXT("originalAssetPath"), Target);
    Origin->SetStringField(TEXT("catalogDigest"), PayloadDocument.CatalogDigest);
    Origin->SetBoolField(TEXT("derived"), !Original);
    return true;
}
FCapture::~FCapture() { if (FileHandle) CloseHandle(static_cast<HANDLE>(FileHandle)); }
bool FCapture::Begin(const FString& Source, const FString& Kind, FString& Error, int32 ReadRevision, bool RequireOrigin)
{
    check(IsInGameThread()); check(!FileHandle); Error.Empty();
    UClass* Expected = Kind == TEXT("Material") ? UMaterial::StaticClass()
        : Kind == TEXT("MaterialFunction") ? UMaterialFunction::StaticClass()
        : Kind == TEXT("Texture2D") ? UTexture2D::StaticClass()
        : Kind == TEXT("Blueprint") ? UBlueprint::StaticClass() : nullptr;
    FString Package, Name;
    if (!Expected || !ReadPath(Source, false, Kind, ReadRevision, Filename, Error)
        || !Source.Split(TEXT("."), &Package, &Name))
    { if (Error.IsEmpty()) Error = TEXT("sourcePathRejected"); return false; }
    UObject* Existing = FindObject<UObject>(nullptr, *Source);
    if (!RejectDirty(Existing, Error)) return false;
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    FAssetData Data;
    const auto Exists = Registry.TryGetAssetByObjectPath(FSoftObjectPath(Source), Data);
    if (Exists == UE::AssetRegistry::EExists::Unknown) { Error = TEXT("sourceNotReady"); return false; }
    if (Exists != UE::AssetRegistry::EExists::Exists) { Error = TEXT("sourceNotFound"); return false; }
    if (!Data.IsValid() || Data.IsRedirector() || Data.GetObjectPathString() != Source
        || Data.AssetClassPath != Expected->GetClassPathName())
    { Error = TEXT("sourceClassOrRedirectRejected"); return false; }
    HANDLE File = CreateFileW(*Filename, FILE_READ_DATA | FILE_READ_ATTRIBUTES, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (File == INVALID_HANDLE_VALUE) { Error = TEXT("sourceSavedFileOpenRejected"); return false; }
    FileHandle = File;
    BY_HANDLE_FILE_INFORMATION Info; LARGE_INTEGER Size; WCHAR Final[32768];
    const DWORD Count = GetFinalPathNameByHandleW(File, Final, UE_ARRAY_COUNT(Final), FILE_NAME_NORMALIZED);
    FString Actual = Count && Count < UE_ARRAY_COUNT(Final) ? FString(Final) : FString();
    Actual.RemoveFromStart(TEXT("\\\\?\\")); FPaths::NormalizeFilename(Actual);
    if (!GetFileInformationByHandle(File, &Info) || (Info.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY))
        || !Actual.Equals(Filename, ESearchCase::IgnoreCase) || !GetFileSizeEx(File, &Size) || Size.QuadPart <= 0)
    { Error = TEXT("sourceSavedFileIdentityRejected"); return false; }
    FileDigest = NodeForgePrototype::FileSha256(Filename);
    if (FileDigest.Len() != 64) { Error = TEXT("sourceHashUnavailable"); return false; }
    const FOrigin* Origin = Origins.Find(Source);
    if (RequireOrigin && Existing && (!Origin || Origin->Object.Get() != Existing || Origin->Sha256 != FileDigest))
    { Error = TEXT("sourceMemoryUnproven"); return false; }
    if (RequireOrigin && !Existing && FindPackage(nullptr, *Package))
    { Error = TEXT("sourceMemoryUnproven"); return false; }
    UObject* Loaded = Existing ? Existing : LoadObject<UObject>(nullptr, *Source, nullptr, LOAD_NoWarn);
    if (!Loaded || Loaded->GetClass() != Expected || Loaded->GetPathName() != Source || Loaded->GetOutermost()->GetName() != Package)
    { Error = TEXT("sourceLoadedIdentityRejected"); return false; }
    if (!RejectDirty(Loaded, Error)) return false;
    if (Kind == TEXT("Texture2D") && !QualifyTexture(Loaded, Error)) return false;
    if (Kind == TEXT("Blueprint"))
    {
        const UBlueprint* BP = CastChecked<UBlueprint>(Loaded);
        if (!BP->GeneratedClass || (BP->Status != BS_UpToDate && BP->Status != BS_UpToDateWithWarnings))
        { Error = TEXT("sourceBlueprintNotCompiled"); return false; }
    }
    Captured = Loaded; SourcePath = Source;
    if (!Verify(Error)) return false;
    if (RequireOrigin)
    {
        for (auto It = Origins.CreateIterator(); It; ++It) if (!It.Value().Object.IsValid()) It.RemoveCurrent();
        if (!Origin && Origins.Num() >= 128) { Error = TEXT("sourceOriginCapacity"); return false; }
        Origins.Add(Source, FOrigin{Loaded, FileDigest});
    }
    return true;
}
bool FCapture::Verify(FString& Error) const
{
    check(IsInGameThread());
    UObject* Actual = Captured.Get();
    if (!Actual || Actual->GetPathName() != SourcePath || !RejectDirty(Actual, Error))
    { if (Error.IsEmpty()) Error = TEXT("sourceObjectIdentityChanged"); return false; }
    if (!FileHandle || NodeForgePrototype::FileSha256(Filename) != FileDigest)
    { Error = TEXT("sourceSavedBytesChanged"); return false; }
    return true;
}
}
