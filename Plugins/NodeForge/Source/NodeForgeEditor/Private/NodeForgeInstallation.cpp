#include "NodeForgeInstallation.h"
#include "NodeForgeResourceSource.h"
#include "NodeForgePrototype.h"
#include "NodeForgeEditorControlProtocol.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"
#include "Misc/EngineVersion.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/Guid.h"
#include "Misc/Base64.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonSerializer.h"
#include "Windows/WindowsHWrapper.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include <aclapi.h>
#include "Windows/HideWindowsPlatformTypes.h"

namespace NodeForgeInstallation
{
FSimpleMulticastDelegate& OnReady() { static FSimpleMulticastDelegate Ready; return Ready; }
namespace
{
bool Fail(FString& Error, const TCHAR* Reason) { Error = Reason; return false; }
bool Absolute(const FString& Input, FString& Result, FString& Error)
{
    FString Normal = Input; FPaths::NormalizeFilename(Normal);
    TArray<FString> Parts; Normal.ParseIntoArray(Parts, TEXT("/"), false);
    if (Input.IsEmpty() || FPaths::IsRelative(Input) || Parts.Contains(TEXT("..")) || Parts.Contains(TEXT(".")))
        return Fail(Error, TEXT("installationPathRejected"));
    Result = FPaths::ConvertRelativePathToFull(Normal); FPaths::NormalizeFilename(Result);
    FPaths::NormalizeDirectoryName(Result);
    FString Parent = Result;
    while (!Parent.IsEmpty())
    {
        const DWORD Attributes = GetFileAttributesW(*Parent);
        if (Attributes != INVALID_FILE_ATTRIBUTES && (Attributes & FILE_ATTRIBUTE_REPARSE_POINT))
            return Fail(Error, TEXT("installationReparseRejected"));
        const FString Next = FPaths::GetPath(Parent); if (Next == Parent) break; Parent = Next;
    }
    return true;
}
bool Object(const FString& Path, TSharedPtr<FJsonObject>& Out, FString& Digest, FString& Error, uint32 Max = 65536)
{
    FString Text, Canonical;
    if (!Absolute(Path, Canonical, Error) || !NodeForgeResourceSource::ReadText(Canonical, Max, Text, Digest, Error)) return false;
    FTCHARToUTF8 Utf8(*Text);
    if (!NodeForgeEditorControl::ParseObject(MakeArrayView(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length()), Out, Max))
        return Fail(Error, TEXT("installationJsonRejected"));
    return true;
}
bool String(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, FString& Out, FString& Error)
{
    return Object->TryGetStringField(Key, Out) && !Out.IsEmpty() ? true : Fail(Error, TEXT("installationFieldRejected"));
}
}
static bool ValidateLocal(const TSharedPtr<FJsonObject>& Local, const FString& Digest, FBinding& Binding, FString& Error)
{
    check(IsInGameThread()); Binding = {}; Error.Empty();
    FString SelectedProject;
    if (!Absolute(FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()), SelectedProject, Error)) return false;
    FString Ignored;
    double Version = 0; FString Format, Root, Project, Plugin, Engine, ActualEngineDirectory;
    const TArray<TSharedPtr<FJsonValue>>* Roots = nullptr;
    if (Local->Values.Num() != 8 || !String(Local, TEXT("format"), Format, Error) || Format != TEXT("NodeForgeInstallationBinding")
        || !Local->TryGetNumberField(TEXT("version"), Version) || Version != 1
        || !String(Local, TEXT("installationRoot"), Root, Error) || !Absolute(Root, Binding.InstallationRoot, Error)
        || !String(Local, TEXT("project"), Project, Error) || !Absolute(Project, Binding.Project, Error)
        || !String(Local, TEXT("pluginDirectory"), Plugin, Error) || !Absolute(Plugin, Binding.PluginDirectory, Error)
        || !String(Local, TEXT("engineRoot"), Engine, Error) || !Absolute(Engine, Binding.EngineRoot, Error)
        || !String(Local, TEXT("binarySha256"), Binding.BinarySha256, Error) || Binding.BinarySha256.Len() != 64
        || !Local->TryGetArrayField(TEXT("readRoots"), Roots) || Roots->Num() < 1 || Roots->Num() > 32
        || !FPaths::IsSamePath(Binding.Project, SelectedProject)
        || !Absolute(FPaths::ConvertRelativePathToFull(FPaths::EngineDir()), ActualEngineDirectory, Error)
        || !FPaths::IsSamePath(Binding.EngineRoot / TEXT("Engine"), ActualEngineDirectory))
        return Fail(Error, TEXT("installationBindingIdentityRejected"));
    const auto Loaded = IPluginManager::Get().FindPlugin(TEXT("NodeForge"));
    int32 Discoveries = 0;
    for (const auto& PluginInfo : IPluginManager::Get().GetDiscoveredPlugins())
        if (PluginInfo->GetName() == TEXT("NodeForge")) ++Discoveries;
    const FString Binary = Binding.PluginDirectory / TEXT("Binaries/Win64/UnrealEditor-NodeForgeEditor.dll");
    if (!Absolute(Binary, Ignored, Error) || !Loaded || Discoveries != 1 || !FPaths::IsSamePath(Loaded->GetBaseDir(), Binding.PluginDirectory)
        || !FPaths::IsSamePath(FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor")), Binary)
        || NodeForgePrototype::FileSha256(Binary) != Binding.BinarySha256)
        return Fail(Error, TEXT("installationLoadedPluginMismatch"));
    TSharedPtr<FJsonObject> Manifest; FString ManifestDigest, Relative, DeclaredSha, ModuleSha, DescriptorSha, Product, EngineVersion, BuildId;
    double Schema = 0;
    FString Layout;
    const bool ProjectPlugin = FPaths::IsSamePath(Binding.PluginDirectory, FPaths::GetPath(SelectedProject) / TEXT("Plugins/NodeForge"));
    const bool EnginePlugin = FPaths::IsSamePath(Binding.PluginDirectory, Binding.EngineRoot / TEXT("Engine/Plugins/NodeForge"))
        || FPaths::IsSamePath(Binding.PluginDirectory, Binding.EngineRoot / TEXT("Engine/Plugins/Marketplace/NodeForge"));
    const bool InstalledPlugin = ProjectPlugin || EnginePlugin;
    if (!Object(Binding.InstallationRoot / TEXT("InstallationManifest.json"), Manifest, ManifestDigest, Error)
        || !String(Manifest, TEXT("product"), Product, Error) || Product != TEXT("NodeForge")
        || !String(Manifest, TEXT("buildId"), BuildId, Error)
        || !Manifest->TryGetNumberField(TEXT("installationSchemaVersion"), Schema) || Schema != 1
        || !String(Manifest, TEXT("pluginDirectory"), Relative, Error) || !FPaths::IsRelative(Relative)
        || !(InstalledPlugin
            ? (Relative == TEXT(".") && Manifest->TryGetStringField(TEXT("installLayout"), Layout)
                && (Layout == TEXT("portablePlugin") || (ProjectPlugin && Layout == TEXT("projectPlugin")))
                && FPaths::IsSamePath(Binding.InstallationRoot, Binding.PluginDirectory))
            : (Absolute(Binding.InstallationRoot / Relative, Ignored, Error) && FPaths::IsUnderDirectory(Ignored, Binding.InstallationRoot)
                && FPaths::IsSamePath(Ignored, Binding.PluginDirectory)))
        || !String(Manifest, TEXT("binarySha256"), DeclaredSha, Error) || DeclaredSha != Binding.BinarySha256
        || !String(Manifest, TEXT("moduleManifestSha256"), ModuleSha, Error)
        || ModuleSha != NodeForgePrototype::FileSha256(Binding.PluginDirectory / TEXT("Binaries/Win64/UnrealEditor.modules"))
        || !String(Manifest, TEXT("pluginDescriptorSha256"), DescriptorSha, Error)
        || DescriptorSha != NodeForgePrototype::FileSha256(Binding.PluginDirectory / TEXT("NodeForge.uplugin"))
        || !String(Manifest, TEXT("engineVersion"), EngineVersion, Error)
        || EngineVersion != FString::Printf(TEXT("%d.%d.%d"), FEngineVersion::Current().GetMajor(), FEngineVersion::Current().GetMinor(), FEngineVersion::Current().GetPatch()))
        return Fail(Error, TEXT("installationManifestMismatch"));
    TSharedPtr<FJsonObject> Modules, EngineModules, Descriptor; FString Hash, PluginBuild, EngineBuild;
    if (!Object(Binding.PluginDirectory / TEXT("Binaries/Win64/UnrealEditor.modules"), Modules, Hash, Error)
        || !Object(Binding.EngineRoot / TEXT("Engine/Binaries/Win64/UnrealEditor.modules"), EngineModules, Hash, Error, 262144)
        || !String(Modules, TEXT("BuildId"), PluginBuild, Error) || !String(EngineModules, TEXT("BuildId"), EngineBuild, Error) || PluginBuild != EngineBuild
        || !Object(SelectedProject, Descriptor, Hash, Error)) return Fail(Error, TEXT("installationEngineOrProjectMismatch"));
    const TSharedPtr<FJsonObject>* ModuleNames = nullptr; FString Filename;
    if (!Modules->TryGetObjectField(TEXT("Modules"),ModuleNames) || (*ModuleNames)->Values.Num()!=1
        || !(*ModuleNames)->TryGetStringField(TEXT("NodeForgeEditor"),Filename) || Filename!=TEXT("UnrealEditor-NodeForgeEditor.dll"))
        return Fail(Error,TEXT("installationModuleManifestRejected"));
    const TArray<TSharedPtr<FJsonValue>>* Enabled = nullptr; const TArray<TSharedPtr<FJsonValue>>* Directories = nullptr;
    int32 References = 0, Selections = 0;
    if (!Descriptor->TryGetArrayField(TEXT("Plugins"), Enabled)
        || (Descriptor->HasField(TEXT("AdditionalPluginDirectories")) && !Descriptor->TryGetArrayField(TEXT("AdditionalPluginDirectories"), Directories))
        || (!InstalledPlugin && !Directories))
        return Fail(Error, TEXT("installationProjectReferenceMissing"));
    for (const auto& Value : *Enabled)
    {
        const TSharedPtr<FJsonObject>* Entry = nullptr; FString Name; bool Active = false;
        if (!Value->TryGetObject(Entry) || !(*Entry)->TryGetStringField(TEXT("Name"), Name)) return Fail(Error, TEXT("installationProjectPluginsRejected"));
        if (Name == TEXT("NodeForge"))
        {
            if (!(*Entry)->TryGetBoolField(TEXT("Enabled"), Active) || !Active) return Fail(Error, TEXT("installationProjectDisabled"));
            ++References;
        }
    }
    if (Directories) for (const auto& Value : *Directories)
    {
        FString Directory;
        if (!Value->TryGetString(Directory)) return Fail(Error, TEXT("installationProjectDirectoriesRejected"));
        if (FPaths::IsSamePath(FPaths::ConvertRelativePathToFull(FPaths::GetPath(SelectedProject), Directory), FPaths::GetPath(Binding.PluginDirectory))) ++Selections;
    }
    if (References != 1 || Selections != (InstalledPlugin ? 0 : 1)
        || (!ProjectPlugin && FPaths::FileExists(FPaths::GetPath(SelectedProject) / TEXT("Plugins/NodeForge/NodeForge.uplugin")))
        || (!FPaths::IsSamePath(Binding.PluginDirectory, Binding.EngineRoot / TEXT("Engine/Plugins/NodeForge"))
            && FPaths::FileExists(Binding.EngineRoot / TEXT("Engine/Plugins/NodeForge/NodeForge.uplugin")))
        || (!FPaths::IsSamePath(Binding.PluginDirectory, Binding.EngineRoot / TEXT("Engine/Plugins/Marketplace/NodeForge"))
            && FPaths::FileExists(Binding.EngineRoot / TEXT("Engine/Plugins/Marketplace/NodeForge/NodeForge.uplugin"))))
        return Fail(Error, TEXT("installationProjectAmbiguous"));
    for (const auto& Value : *Roots)
    {
        FString RootPath, Mapped;
        if (!Value->TryGetString(RootPath) || RootPath.Len() > 256 || RootPath.Contains(TEXT("..")) || RootPath.Contains(TEXT("\\"))
            || RootPath.Contains(TEXT("//")) || RootPath.EndsWith(TEXT("/")) || !FPackageName::IsValidLongPackageName(RootPath)
            || !(RootPath.StartsWith(TEXT("/Game/"), ESearchCase::CaseSensitive) || RootPath.StartsWith(TEXT("/Engine/"), ESearchCase::CaseSensitive))
            || Binding.ReadRoots.Contains(RootPath) || !FPackageName::TryConvertLongPackageNameToFilename(RootPath, Mapped)
            || !Absolute(FPaths::ConvertRelativePathToFull(Mapped), Ignored, Error)) return Fail(Error, TEXT("installationReadRootRejected"));
        const FString Content = FPaths::ConvertRelativePathToFull(RootPath.StartsWith(TEXT("/Game/")) ? FPaths::ProjectContentDir() : FPaths::EngineContentDir());
        if (!FPaths::IsUnderDirectory(Ignored, Content)) return Fail(Error, TEXT("installationReadRootMountMismatch"));
        Binding.ReadRoots.Add(RootPath);
    }
    Binding.Digest = Digest;
    return true;
}
static bool CatalogForBinding(const FBinding& Binding, FString& CatalogDigest, FString& Error)
{
    CatalogDigest.Empty();
    TSharedPtr<FJsonObject> Manifest, Catalog; FString Hash, ExpectedHash, BinaryHash, Engine;
    if(!Object(Binding.InstallationRoot / TEXT("InstallationManifest.json"),Manifest,Hash,Error)
        || !String(Manifest,TEXT("catalogSha256"),ExpectedHash,Error)
        || !Object(Binding.InstallationRoot / TEXT("Metadata/Catalog.json"),Catalog,Hash,Error,NodeForgeEditorControl::MaxReportBytes)
        || Hash!=ExpectedHash || !String(Catalog,TEXT("catalogDigest"),CatalogDigest,Error) || CatalogDigest.Len()!=64
        || !String(Catalog,TEXT("binarySha256"),BinaryHash,Error) || BinaryHash!=Binding.BinarySha256
        || !String(Catalog,TEXT("engine"),Engine,Error) || Engine!=FEngineVersion::Current().ToString())
        return Fail(Error,TEXT("installationCatalogIdentityRejected"));
    for(TCHAR C:CatalogDigest) if(!((C>='0' && C<='9') || (C>='a' && C<='f')))
        return Fail(Error,TEXT("installationCatalogIdentityRejected"));
    const TSharedPtr<FJsonObject>* Payload=nullptr;const TSharedPtr<FJsonObject>* Material=nullptr;double Revision=0;
    if(!Catalog->TryGetObjectField(TEXT("catalog"),Payload) || !(*Payload)->TryGetObjectField(TEXT("material"),Material)
        || !(*Material)->TryGetNumberField(TEXT("resourceReadRevision"),Revision) || Revision!=1)
        return Fail(Error,TEXT("installationCatalogCapabilityRejected"));
    return true;
}
bool Read(FBinding& Binding, FString& Error)
{
    check(IsInGameThread()); Binding = {}; Error.Empty();
    FString Project, Digest; TSharedPtr<FJsonObject> Local;
    if (!Absolute(FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()), Project, Error)
        || !Object(FPaths::GetPath(Project) / TEXT("Config/NodeForgeInstallation.json"), Local, Digest, Error)) return false;
    return ValidateLocal(Local, Digest, Binding, Error);
}
FString StateDirectory(const FBinding& Binding)
{
    FString Path, Error;
    return Absolute(FPaths::GetPath(Binding.Project) / TEXT("Saved/NodeForge"), Path, Error) ? Path : FString();
}
FString StateDirectory()
{
    FBinding Binding; FString Error;
    return Read(Binding, Error) ? StateDirectory(Binding) : FString();
}
bool CatalogIdentity(FString& CatalogDigest, FString& Error)
{
    CatalogDigest.Empty();
    FBinding Binding;
    return Read(Binding, Error) && CatalogForBinding(Binding, CatalogDigest, Error);
}
bool AiConfiguration(const FBinding& Binding, TSharedPtr<FJsonObject>& Configuration,
                     FString& UserDirectory, FString& Error)
{
    FString LocalAppData, Path, Digest;
    if (!Absolute(FPlatformMisc::GetEnvironmentVariable(TEXT("USERPROFILE")), LocalAppData, Error)
        || !Absolute(LocalAppData / TEXT("NodeForge/Installations") /
            NodeForgePrototype::TextSha256(Binding.PluginDirectory.ToLower()), UserDirectory, Error)
        || !Absolute(UserDirectory / TEXT("Installation.json"), Path, Error)) return false;
    // A persistent opt-in belongs to the Windows user, not one logon generation.
    PSID Owner = nullptr; PSECURITY_DESCRIPTOR Security = nullptr; HANDLE Token = nullptr;
    bool Owned = false;
    if (GetNamedSecurityInfoW(*Path, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, &Owner, nullptr, nullptr, nullptr, &Security) == ERROR_SUCCESS
        && OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &Token))
    {
        DWORD Size = 0; GetTokenInformation(Token, TokenUser, nullptr, 0, &Size);
        TArray<uint8> Bytes; Bytes.SetNumUninitialized(Size);
        if (Size && GetTokenInformation(Token, TokenUser, Bytes.GetData(), Size, &Size))
            Owned = EqualSid(Owner, reinterpret_cast<TOKEN_USER*>(Bytes.GetData())->User.Sid) != 0;
    }
    if (Token) CloseHandle(Token); if (Security) LocalFree(Security);
    if (!Owned || !Object(Path, Configuration, Digest, Error) || Configuration->Values.Num() != 9)
        return Fail(Error, TEXT("aiInstallationNotEnabled"));
    for (const TCHAR* Key : {TEXT("format"), TEXT("pluginDirectory"), TEXT("engineRoot"), TEXT("binarySha256"),
                            TEXT("bundleId"), TEXT("toolContractSha256"), TEXT("docsManifestSha256")})
        if (!Configuration->HasTypedField<EJson::String>(Key)) return Fail(Error, TEXT("aiInstallationInvalid"));
    if (!Configuration->HasTypedField<EJson::Number>(TEXT("version"))
        || Configuration->GetStringField(TEXT("format")) != TEXT("NodeForgeAiInstallation1")
        || Configuration->GetNumberField(TEXT("version")) != 1
        || !Configuration->HasTypedField<EJson::Boolean>(TEXT("enabled")) || !Configuration->GetBoolField(TEXT("enabled"))
        || Configuration->GetStringField(TEXT("pluginDirectory")) != Binding.PluginDirectory
        || Configuration->GetStringField(TEXT("engineRoot")) != Binding.EngineRoot
        || Configuration->GetStringField(TEXT("binarySha256")) != Binding.BinarySha256
        || Configuration->GetStringField(TEXT("toolContractSha256")) != NodeForgePrototype::FileSha256(Binding.InstallationRoot / TEXT("Tools/NodeForgeMcp/ToolContract.json")))
        return Fail(Error, TEXT("aiInstallationNotEnabled"));
    return true;
}
static TSharedRef<FJsonObject> BindingObject(const FBinding& Binding)
{
    auto Local = MakeShared<FJsonObject>();
    Local->SetStringField(TEXT("format"), TEXT("NodeForgeInstallationBinding")); Local->SetNumberField(TEXT("version"), 1);
    Local->SetStringField(TEXT("installationRoot"), Binding.InstallationRoot); Local->SetStringField(TEXT("project"), Binding.Project);
    Local->SetStringField(TEXT("pluginDirectory"), Binding.PluginDirectory); Local->SetStringField(TEXT("engineRoot"), Binding.EngineRoot);
    Local->SetStringField(TEXT("binarySha256"), Binding.BinarySha256);
    TArray<TSharedPtr<FJsonValue>> Roots; for (const FString& Root : Binding.ReadRoots) Roots.Add(MakeShared<FJsonValueString>(Root));
    Local->SetArrayField(TEXT("readRoots"), Roots); return Local;
}
bool InspectCurrent(FBinding& Binding, FString& Error)
{
    check(IsInGameThread()); Binding = {}; Error.Empty();
    const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("NodeForge"));
    FString EngineDirectory;
    if (!Absolute(FPaths::ConvertRelativePathToFull(FPaths::EngineDir()), EngineDirectory, Error)) return false;
    if (!Plugin || !Absolute(FPaths::ConvertRelativePathToFull(Plugin->GetBaseDir()), Binding.PluginDirectory, Error)
        || !Absolute(FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()), Binding.Project, Error)
        || !Absolute(FPaths::GetPath(EngineDirectory), Binding.EngineRoot, Error)) return false;
    // Search only this loaded plugin and its immediate package ancestors; never search user disks.
    FString Root = Binding.PluginDirectory;
    for (int32 Depth = 0; Depth < 4; ++Depth)
    {
        if (FPaths::FileExists(Root / TEXT("InstallationManifest.json"))) { Binding.InstallationRoot = Root; break; }
        const FString Parent = FPaths::GetPath(Root); if (Parent == Root) break; Root = Parent;
    }
    if (Binding.InstallationRoot.IsEmpty()) return Fail(Error, TEXT("installationPackageMissing"));
    Binding.BinarySha256 = NodeForgePrototype::FileSha256(Binding.PluginDirectory / TEXT("Binaries/Win64/UnrealEditor-NodeForgeEditor.dll"));
    Binding.ReadRoots = {TEXT("/Game/NodeForgeTests"), TEXT("/Engine/EngineResources"), TEXT("/Engine/Functions/Engine_MaterialFunctions02")};
    FBinding Checked; FString Catalog;
    if (!ValidateLocal(BindingObject(Binding), FString(), Checked, Error) || !CatalogForBinding(Checked, Catalog, Error)) return false;
    Binding = Checked; return true;
}
bool ConfirmCurrent(FString& ReceiptPath, FString& Error)
{
    check(IsInGameThread()); ReceiptPath.Empty();
    FBinding Binding;
    if (!InspectCurrent(Binding, Error)) return false;
    const FString Config = FPaths::GetPath(Binding.Project) / TEXT("Config/NodeForgeInstallation.json");
    FString Canonical;
    if (!Absolute(Config, Canonical, Error)) return false;
    // This first-use operation never replaces an existing binding, including an invalid one.
    if (FPaths::FileExists(Config)) return Fail(Error, TEXT("installationExistingBindingConflict"));
    TArray<uint8> ProjectBytes;
    if (!FFileHelper::LoadFileToArray(ProjectBytes, *Binding.Project) || ProjectBytes.Num() > 65536) return Fail(Error, TEXT("installationProjectReadFailed"));
    const FString ProjectSha = NodeForgePrototype::FileSha256(Binding.Project);
    const FString StateRoot = StateDirectory(Binding);
    if (StateRoot.IsEmpty()) return Fail(Error, TEXT("installationStatePathRejected"));
    const FString ReportRoot = StateRoot / TEXT("Reports/Installations") / FGuid::NewGuid().ToString(EGuidFormats::Digits);
    if (!Absolute(ReportRoot, Canonical, Error)) return false;
    auto Receipt = MakeShared<FJsonObject>();
    Receipt->SetStringField(TEXT("format"), TEXT("NodeForgeInstallationReceipt")); Receipt->SetNumberField(TEXT("version"), 1);
    Receipt->SetStringField(TEXT("action"), TEXT("bind")); Receipt->SetStringField(TEXT("installationRoot"), Binding.InstallationRoot);
    Receipt->SetStringField(TEXT("project"), Binding.Project); Receipt->SetStringField(TEXT("engineRoot"), Binding.EngineRoot);
    Receipt->SetStringField(TEXT("startedAtUtc"), FDateTime::UtcNow().ToIso8601()); Receipt->SetStringField(TEXT("status"), TEXT("preparing"));
    Receipt->SetBoolField(TEXT("requiresReview"), false);
    auto Before = MakeShared<FJsonObject>(); Before->SetStringField(TEXT("path"), Binding.Project); Before->SetBoolField(TEXT("exists"), true);
    Before->SetStringField(TEXT("sha256"), ProjectSha); Before->SetStringField(TEXT("bytesBase64"), FBase64::Encode(ProjectBytes));
    auto ProjectChange = MakeShared<FJsonObject>(); ProjectChange->SetStringField(TEXT("path"), Binding.Project);
    ProjectChange->SetObjectField(TEXT("before"), Before); ProjectChange->SetStringField(TEXT("afterSha256"), ProjectSha);
    auto ConfigBefore = MakeShared<FJsonObject>(); ConfigBefore->SetStringField(TEXT("path"), Config); ConfigBefore->SetBoolField(TEXT("exists"), false);
    ConfigBefore->SetField(TEXT("sha256"), MakeShared<FJsonValueNull>()); ConfigBefore->SetField(TEXT("bytesBase64"), MakeShared<FJsonValueNull>());
    auto ConfigChange = MakeShared<FJsonObject>(); ConfigChange->SetStringField(TEXT("path"), Config); ConfigChange->SetObjectField(TEXT("before"), ConfigBefore);
    // Preserve the exact prospective bytes and SHA using the existing create-only publisher.
    const FString Prepared = ReportRoot / TEXT("BindingPrepared.json");
    const auto Local = BindingObject(Binding);
    if (!NodeForgePrototype::WriteJson(Local, Prepared)) return Fail(Error, TEXT("installationPreparationWriteFailed"));
    ConfigChange->SetStringField(TEXT("afterSha256"), NodeForgePrototype::FileSha256(Prepared));
    Receipt->SetArrayField(TEXT("changes"), {MakeShared<FJsonValueObject>(ProjectChange), MakeShared<FJsonValueObject>(ConfigChange)});
    Receipt->SetArrayField(TEXT("completedWrites"), {});
    if (!NodeForgePrototype::WriteJson(Receipt, ReportRoot / TEXT("ReceiptPreparing.json"))) return Fail(Error, TEXT("installationReceiptWriteFailed"));
    ReceiptPath = ReportRoot / TEXT("Receipt.json");
    const bool Written = NodeForgePrototype::FileSha256(Binding.Project) == ProjectSha
        && NodeForgePrototype::WriteJson(Local, Config);
    FBinding Actual; FString Catalog;
    const bool Valid = Written && Read(Actual, Error) && CatalogForBinding(Actual, Catalog, Error)
        && NodeForgePrototype::FileSha256(Config) == ConfigChange->GetStringField(TEXT("afterSha256"));
    Receipt->SetStringField(TEXT("status"), Valid ? TEXT("passed") : TEXT("failed"));
    Receipt->SetBoolField(TEXT("requiresReview"), Written && !Valid);
    Receipt->SetStringField(TEXT("finishedAtUtc"), FDateTime::UtcNow().ToIso8601());
    if (Written) Receipt->SetArrayField(TEXT("completedWrites"), {MakeShared<FJsonValueString>(Config)});
    if (!NodeForgePrototype::WriteJson(Receipt, ReceiptPath)) return Fail(Error, TEXT("installationReceiptCompletionUnknown"));
    if (!Valid && Error.IsEmpty()) Error = TEXT("installationBindingWriteRejected");
    if (Valid) OnReady().Broadcast();
    return Valid;
}
}
