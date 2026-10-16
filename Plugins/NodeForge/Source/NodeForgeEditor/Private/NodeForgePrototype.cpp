// NodeForge minimal prototype. Blueprint native construction/diagnostic/save idioms
// adapted from ZiggyMar/Unreal-MCP MCPCommandHandler.cpp, commit d1a85396d9c62328521256005fe7c420725651cc.
// Copyright (c) 2026 Ziggy. MIT notice: ../../../Licenses/ZiggyMarMIT.txt.
// Strict input, ownership, staging, reports, material and persistence logic are new project code.
#include "NodeForgePrototype.h"
#include "NodeForgeGraphCodec.h"
#include "NodeForgeAssetDefinitions.h"
#include "NodeForgeNodeDefinitions.h"
#include "NodeForgeResourceSource.h"
#include "NodeForgeInstallation.h"
#include "NodeForgeEditorControlProtocol.h"
#include "NodeForgeTaskRules.h"
#include "NodeForgeMaterialGraph.h"
#include "NodeForgeMaterialInstances.h"
#include "NodeForgeMaterialFunctions.h"
#include "NodeForgeBlueprintGraph.h"
#include "NodeForgeGraphDiff.h"
#include "NodeForgeLayoutChecks.h"
#include "Modules/ModuleManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Dom/JsonObject.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "NodeForgeVisualChecks.h"
#include "NodeForgeGraphChecks.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformMisc.h"
#include "Interfaces/IPluginManager.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "MaterialShared.h"
#include "MaterialDomain.h"
#include "ShaderCompiler.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "RHI.h"
#include "Serialization/Archive.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Windows/WindowsHWrapper.h"
#include <bcrypt.h>

namespace NodeForgePrototype
{
using FObject = TSharedPtr<FJsonObject>;
static const FString Adapter = TEXT("NodeForge-UE532-Minimal-1");
static const FString GraphAdapter = TEXT("NodeForge-UE532-Graph-3");
static const FString Payload = TEXT("NodeForge Blueprint Runtime OK");
static const FString Root = TEXT("/Game/NodeForgeTests/");

static bool WriteJsonWithFormat(const TSharedRef<FJsonObject>& Object, const FString& Path, bool Compact)
{
    if (Path.IsEmpty() || IFileManager::Get().FileExists(*Path)) return false;
    FString Text;
    const bool Serialized = Compact
        ? FJsonSerializer::Serialize(Object, TJsonWriterFactory<TCHAR,TCondensedJsonPrintPolicy<TCHAR>>::Create(&Text))
        : FJsonSerializer::Serialize(Object, TJsonWriterFactory<>::Create(&Text));
    if(!Serialized || !IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true)) return false;
    // UE 5.3.2's file-writer flag alone failed the actual concurrent-writer test.
    // Write a unique sibling, close it, then use native Move with Replace=false.
    // On this Windows baseline the lower-level move calls MoveFileW, which
    // refuses an occupied destination. Readers never see a partial final JSON.
    const FString Pending=Path+TEXT(".")+FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT(".pending");
    if(IFileManager::Get().FileExists(*Pending))return false;
    const bool Saved=FFileHelper::SaveStringToFile(Text,*Pending,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    const bool Published=Saved && IFileManager::Get().Move(*Path,*Pending,false,false,false,true);
    if(!Published && IFileManager::Get().FileExists(*Pending)
        && !IFileManager::Get().Delete(*Pending,false,false,true))
        UE_LOG(LogTemp,Warning,TEXT("NodeForge retained its failed report staging file: %s"),*Pending);
    return Published;
}

bool WriteJson(const TSharedRef<FJsonObject>& Object, const FString& Path)
{
    return WriteJsonWithFormat(Object,Path,false);
}

static FString Digest(const TArray<uint8>& Bytes)
{
    BCRYPT_ALG_HANDLE Algorithm = nullptr;
    BCRYPT_HASH_HANDLE Hash = nullptr;
    DWORD ObjectSize = 0, ResultSize = 0;
    TArray<uint8> State, Output;
    bool Ok = BCryptOpenAlgorithmProvider(&Algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0;
    if (Ok) Ok = BCryptGetProperty(Algorithm, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&ObjectSize), sizeof(ObjectSize), &ResultSize, 0) >= 0;
    State.SetNumZeroed(ObjectSize); Output.SetNumZeroed(32);
    if (Ok) Ok = BCryptCreateHash(Algorithm, &Hash, State.GetData(), State.Num(), nullptr, 0, 0) >= 0;
    if (Ok) Ok = BCryptHashData(Hash, const_cast<PUCHAR>(Bytes.GetData()), Bytes.Num(), 0) >= 0;
    if (Ok) Ok = BCryptFinishHash(Hash, Output.GetData(), Output.Num(), 0) >= 0;
    if (Hash) BCryptDestroyHash(Hash);
    if (Algorithm) BCryptCloseAlgorithmProvider(Algorithm, 0);
    return Ok ? BytesToHex(Output.GetData(), Output.Num()).ToLower() : FString();
}
static FString FileDigest(const FString& Path)
{
    TArray<uint8> Bytes;
    return FFileHelper::LoadFileToArray(Bytes, *Path) ? Digest(Bytes) : FString();
}
FString FileSha256(const FString& Path) { return FileDigest(Path); }
FString TextSha256(const FString& Text)
{
    FTCHARToUTF8 Utf8(*Text);
    TArray<uint8> Bytes; Bytes.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
    return Digest(Bytes); // Existing SHA implementation; no graph/parser behavior changes.
}
static FString JsonDigest(const TSharedRef<FJsonObject>& Object)
{
    FString Text;
    FJsonSerializer::Serialize(Object, TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Text));
    FTCHARToUTF8 Utf8(*Text);
    TArray<uint8> Bytes; Bytes.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
    return Digest(Bytes);
}
static FObject ReadJson(const FString& Path)
{
    FString Text; FObject Result;
    if (FFileHelper::LoadFileToString(Text, *Path)) FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Result);
    return Result;
}
// External graph input is bounded before allocating or reading its payload.
// Read exactly the admitted size; a growing file cannot trigger an unbounded read.
static bool ReadInputText(const FString& Path, FString& Text, FString& Error, TArray<uint8>* Original = nullptr)
{
    if (Path.IsEmpty()) { Error = TEXT("Input missing or outside 1..65536 byte limit"); return false; }
    TUniquePtr<FArchive> File(IFileManager::Get().CreateFileReader(*Path));
    const int64 Size = File ? File->TotalSize() : -1;
    if (Size < 1 || Size > 65536)
    { Error = TEXT("Input missing or outside 1..65536 byte limit"); return false; }
    TArray<uint8> Bytes; Bytes.SetNumUninitialized(static_cast<int32>(Size));
    File->Serialize(Bytes.GetData(), Size);
    const bool ReadOk = !File->IsError() && File->Tell() == Size && IFileManager::Get().FileSize(*Path) == Size;
    const bool Closed = File->Close();
    if (!ReadOk || !Closed) { Error = TEXT("Input read failed or file size changed during bounded read"); return false; }
    FUTF8ToTCHAR Decoded(reinterpret_cast<const ANSICHAR*>(Bytes.GetData()), Bytes.Num());
    Text = FString(Decoded.Length(), Decoded.Get()); FTCHARToUTF8 Encoded(*Text);
    if ((Bytes.Num() >= 3 && Bytes[0] == 0xef && Bytes[1] == 0xbb && Bytes[2] == 0xbf)
        || Encoded.Length() != Bytes.Num() || FMemory::Memcmp(Encoded.Get(), Bytes.GetData(), Bytes.Num()) != 0)
    { Error = TEXT("Input must be valid UTF-8 without BOM or embedded NUL"); return false; }
    if (Original) *Original = MoveTemp(Bytes);
    return true;
}
bool ReadInputFile(const FString& Path, FString& Text, FString& InputDigest, FString& Error)
{
    Text.Empty(); InputDigest.Empty(); Error.Empty(); TArray<uint8> Bytes;
    if (!ReadInputText(Path, Text, Error, &Bytes)) { Text.Empty(); return false; }
    InputDigest = Digest(Bytes);
    return true;
}
static bool ExactFields(const FObject& Object, std::initializer_list<const TCHAR*> Names)
{
    if (!Object || Object->Values.Num() != static_cast<int32>(Names.size())) return false;
    for (const TCHAR* Name : Names)
    {
        bool Found = false;
        for (const auto& Pair : Object->Values) if (Pair.Key.Equals(Name, ESearchCase::CaseSensitive)) { Found = true; break; }
        if (!Found) return false;
    }
    return true;
}
static bool StringIs(const FObject& Object, const TCHAR* Key, const FString& Expected)
{
    FString Value; return Object && Object->HasTypedField<EJson::String>(Key) && Object->TryGetStringField(Key, Value) && Value.Equals(Expected, ESearchCase::CaseSensitive);
}
static bool NumberIs(const FObject& Object, const TCHAR* Key, double Expected)
{
    double Value; return Object && Object->HasTypedField<EJson::Number>(Key) && Object->TryGetNumberField(Key, Value) && FMath::IsFinite(Value) && Value == Expected;
}
static bool SampleConstant(const FObject& Object, double& Value)
{
    return Object && Object->HasTypedField<EJson::Number>(TEXT("value")) && Object->TryGetNumberField(TEXT("value"),Value)
        && FMath::IsFinite(Value) && Value>=0.0 && Value<=4.0;
}
static bool BoolIs(const FObject& Object, const TCHAR* Key, bool Expected)
{
    bool Value; return Object && Object->HasTypedField<EJson::Boolean>(Key) && Object->TryGetBoolField(Key, Value) && Value == Expected;
}
static bool SafeName(const FString& Name)
{
    if (Name.IsEmpty() || Name.Len() > 64) return false;
    for (TCHAR Ch : Name) if (!((Ch >= 'A' && Ch <= 'Z') || (Ch >= 'a' && Ch <= 'z') || (Ch >= '0' && Ch <= '9'))) return false;
    return true;
}
bool ValidateTargetPath(const FString& PackageName, FString& Error)
{
    Error.Empty();
    TArray<FString> Parts;
    if (!PackageName.StartsWith(Root, ESearchCase::CaseSensitive))
    { Error = TEXT("Only targets under /Game/NodeForgeTests/ are allowed"); return false; }
    PackageName.Mid(Root.Len()).ParseIntoArray(Parts, TEXT("/"), false);
    if (Parts.IsEmpty() || Parts.Num() > 3 || !SafeName(Parts.Last()))
    { Error = TEXT("Target requires a 1-64 ASCII alphanumeric leaf and at most two subdirectories"); return false; }
    for (int32 Index = 0; Index < Parts.Num() - 1; ++Index)
        if (Parts[Index].Len() > 16 || !SafeName(Parts[Index]))
        { Error = TEXT("Target subdirectories require 1-16 ASCII alphanumeric characters"); return false; }
    if (!FPackageName::IsValidLongPackageName(PackageName))
    { Error = TEXT("Target is not a valid mounted UE package name"); return false; }
    const FString Filename = FPaths::ConvertRelativePathToFull(
        FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension()));
    return ValidateTargetFilePath(Filename, Error);
}
bool ValidateTargetFilePath(const FString& Filename, FString& Error)
{
    const int32 PathLimit = FMath::Min(260, FPlatformMisc::GetMaxPathLength());
    if (Filename.Len() + 1 > PathLimit || !FPaths::ValidatePath(Filename)
        || !FPaths::IsUnderDirectory(Filename, FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir())))
    { Error = TEXT("Target file path exceeds the portable/native path boundary or leaves project Content"); return false; }
    FString Parent = FPaths::GetPath(Filename);
    while (!Parent.IsEmpty())
    {
        const DWORD Attributes = GetFileAttributesW(*Parent);
        if (Attributes != INVALID_FILE_ATTRIBUTES && (Attributes & FILE_ATTRIBUTE_REPARSE_POINT))
        { Error = TEXT("Target file path contains a reparse directory"); return false; }
        const FString Next = FPaths::GetPath(Parent); if (Next == Parent) break; Parent = Next;
    }
    return true;
}
FString WorkspaceDirectory()
{
    NodeForgeInstallation::FBinding Binding; FString Error;
    return NodeForgeInstallation::Read(Binding,Error) ? Binding.InstallationRoot : FString();
}
static bool AllowedProject()
{
    NodeForgeInstallation::FBinding Binding; FString Error;
    const bool Allowed=NodeForgeInstallation::Read(Binding,Error);
    if(!Allowed) UE_LOG(LogTemp,Warning,TEXT("NodeForge installation guard: %s"),*Error);
    return Allowed;
}

struct FBlueprintParts
{
    UBlueprint* Asset = nullptr;
    UEdGraph* Graph = nullptr;
    UK2Node_Event* Event = nullptr;
    UK2Node_CallFunction* Print = nullptr;
};
static FBlueprintParts Blueprint(UPackage* Package, const FString& Name)
{
    FBlueprintParts Parts;
    Parts.Asset = FKismetEditorUtilities::CreateBlueprint(AActor::StaticClass(), Package, FName(*Name), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    if (!Parts.Asset) return Parts;
    // Initialize the native Actor editor scaffolding before saving a new asset.
    if (!Parts.Asset->SimpleConstructionScript)
        Parts.Asset->SimpleConstructionScript=NewObject<USimpleConstructionScript>(Parts.Asset->GeneratedClass,NAME_None,RF_Transactional);
    Parts.Asset->SimpleConstructionScript->ValidateSceneRootNodes();
    if (!FBlueprintEditorUtils::FindUserConstructionScript(Parts.Asset))
        FKismetEditorUtilities::CreateUserConstructionScript(Parts.Asset);
    Parts.Graph = FBlueprintEditorUtils::FindEventGraph(Parts.Asset);
    if (!Parts.Graph)
    {
        Parts.Graph = FBlueprintEditorUtils::CreateNewGraph(Parts.Asset, TEXT("EventGraph"), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
        FBlueprintEditorUtils::AddUbergraphPage(Parts.Asset, Parts.Graph);
    }
    const TArray<TObjectPtr<UEdGraphNode>> FactoryNodes = Parts.Graph->Nodes;
    for (UEdGraphNode* Node : FactoryNodes) FBlueprintEditorUtils::RemoveNode(Parts.Asset, Node, true);
    Parts.Event = NewObject<UK2Node_Event>(Parts.Graph,NAME_None,RF_Transactional);
    Parts.Event->EventReference.SetExternalMember(TEXT("ReceiveBeginPlay"), AActor::StaticClass());
    Parts.Event->bOverrideFunction = true;
    Parts.Graph->AddNode(Parts.Event, false, false);
    Parts.Event->CreateNewGuid(); Parts.Event->PostPlacedNewNode(); Parts.Event->AllocateDefaultPins();
    Parts.Print = NewObject<UK2Node_CallFunction>(Parts.Graph,NAME_None,RF_Transactional);
    Parts.Print->SetFromFunction(UKismetSystemLibrary::StaticClass()->FindFunctionByName(TEXT("PrintString")));
    Parts.Graph->AddNode(Parts.Print, false, false);
    Parts.Print->CreateNewGuid(); Parts.Print->PostPlacedNewNode(); Parts.Print->AllocateDefaultPins(); Parts.Print->NodePosX = 320;
    return Parts;
}
static FObject Pins(UEdGraphNode* Node)
{
    FObject Object = MakeShared<FJsonObject>();
    Object->SetStringField(TEXT("class"), Node->GetClass()->GetPathName());
    TArray<TSharedPtr<FJsonValue>> Values;
    for (UEdGraphPin* Pin : Node->Pins)
    {
        FObject Value = MakeShared<FJsonObject>();
        Value->SetStringField(TEXT("name"), Pin->PinName.ToString());
        Value->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
        Value->SetStringField(TEXT("category"), Pin->PinType.PinCategory.ToString());
        Value->SetStringField(TEXT("subcategory"), Pin->PinType.PinSubCategory.ToString());
        Value->SetStringField(TEXT("subtype"), GetPathNameSafe(Pin->PinType.PinSubCategoryObject.Get()));
        Value->SetBoolField(TEXT("hidden"), Pin->bHidden);
        Values.Add(MakeShared<FJsonValueObject>(Value));
    }
    Object->SetArrayField(TEXT("pins"), Values);
    return Object;
}
static FObject ExpressionSignature(UMaterialExpression* Expression)
{
    FObject Object = MakeShared<FJsonObject>();
    Object->SetStringField(TEXT("class"), Expression->GetClass()->GetPathName());
    TArray<TSharedPtr<FJsonValue>> Inputs, Outputs;
    for (int32 I = 0; I < Expression->GetInputsView().Num(); ++I)
        Inputs.Add(MakeShared<FJsonValueString>(Expression->GetInputName(I).ToString()));
    for (const FExpressionOutput& Output : Expression->GetOutputs())
    {
        FObject Value = MakeShared<FJsonObject>();
        Value->SetStringField(TEXT("name"), Output.OutputName.ToString());
        Value->SetNumberField(TEXT("mask"), Output.Mask);
        Value->SetNumberField(TEXT("maskR"), Output.MaskR); Value->SetNumberField(TEXT("maskG"), Output.MaskG);
        Value->SetNumberField(TEXT("maskB"), Output.MaskB); Value->SetNumberField(TEXT("maskA"), Output.MaskA);
        Outputs.Add(MakeShared<FJsonValueObject>(Value));
    }
    Object->SetArrayField(TEXT("inputs"), Inputs); Object->SetArrayField(TEXT("outputs"), Outputs);
    return Object;
}
static TSharedRef<FJsonObject> Catalog()
{
    // Real temporary instances, never research fixtures. These are not published or saved.
    FBlueprintParts Parts = Blueprint(GetTransientPackage(), TEXT("NFCatalog") + FGuid::NewGuid().ToString(EGuidFormats::Digits));
    UMaterial* Material = NewObject<UMaterial>(GetTransientPackage());
    auto* Constant = Cast<UMaterialExpressionConstant>(UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionConstant::StaticClass()));
    auto* Multiply = Cast<UMaterialExpressionMultiply>(UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionMultiply::StaticClass()));
    TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("engine"), FEngineVersion::Current().ToString());
    Result->SetStringField(TEXT("adapter"), Adapter);
    Result->SetStringField(TEXT("evidenceLevel"), TEXT("constructible: actual temporary UE instances; graph verification is reported separately"));
    Result->SetObjectField(TEXT("beginPlay"), Pins(Parts.Event));
    Result->SetObjectField(TEXT("printString"), Pins(Parts.Print));
    Result->SetObjectField(TEXT("constant"), ExpressionSignature(Constant));
    Result->SetObjectField(TEXT("multiply"), ExpressionSignature(Multiply));
    Parts.Asset->ClearFlags(RF_Public | RF_Standalone); Parts.Asset->MarkAsGarbage(); Material->MarkAsGarbage();
    return Result;
}

struct FRequest
{
    FString Id, Type, Path, DigestValue, CatalogHash;
    FObject Json;
    NodeForgeGraphDocument::FGraphDocument Document;
    int32 Version = 1;
    double First = 0.5, Second = 2.0;
};
static bool ReadRequest(const FString& Path, const FString& CatalogHash, FRequest& Request, FString& Error)
{
    TArray<uint8> Bytes; FString Text;
    if (!ReadInputText(Path, Text, Error, &Bytes)) return false;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Request.Json) || !Request.Json)
    { Error = TEXT("Malformed JSON"); return false; }
    Request.DigestValue = Digest(Bytes);
    FObject J = Request.Json;
    if (StringIs(J,TEXT("format"),TEXT("NodeForgeGraph")))
    {
        if (!NodeForgeGraphCodec::DecodeText(Text,Request.Document,Error)) return false;
        Request.Version=Request.Document.WireVersion;Request.Id=Request.Document.RequestId;Request.Type=Request.Document.AssetType();
        Request.Path=Request.Document.Target;Request.CatalogHash=Request.Document.CatalogDigest;
        if (!NodeForgeNodeDefinitions::AllowsRequest(Request.CatalogHash,Request.Type,Request.Version,CatalogHash)) {Error=TEXT("graph catalog capability mismatch");return false;}
        if (Request.Type==TEXT("Material")) return NodeForgeMaterialGraph::Validate(Request.Document,Error);
        if (Request.Type==TEXT("Blueprint")) return NodeForgeBlueprintGraph::Validate(Request.Document,Error);
        return NodeForgeMaterialInstances::Validate(Request.Document,Error);
    }
    // Detect duplicate object keys before the DOM parser can overwrite one.
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
    EJsonNotation Notation; TArray<TSet<FString>> Keys; TArray<bool> Objects;
    while (Reader->ReadNext(Notation))
    {
        if (Notation == EJsonNotation::ObjectEnd || Notation == EJsonNotation::ArrayEnd)
        { if (Keys.Num()) { Keys.Pop(); Objects.Pop(); } continue; }
        if (Objects.Num() && Objects.Last())
        {
            const FString Key = Reader->GetIdentifier();
            if (Keys.Last().Contains(Key)) { Error = TEXT("Duplicate JSON object key: ") + Key; return false; }
            Keys.Last().Add(Key);
        }
        if (Notation == EJsonNotation::ObjectStart || Notation == EJsonNotation::ArrayStart)
        {
            if (Keys.Num() >= 32) { Error=TEXT("JSON nesting exceeds 32 levels"); return false; }
            Keys.AddDefaulted(); Objects.Add(Notation == EJsonNotation::ObjectStart);
        }
    }
    // The original fixed v1 vocabulary and native execution remain separate.
    if (!ExactFields(J, {TEXT("format"),TEXT("version"),TEXT("requestId"),TEXT("assetType"),TEXT("assetPath"),TEXT("catalogDigest"),TEXT("nodes"),TEXT("edges"),TEXT("settings")})
        || !StringIs(J,TEXT("format"),TEXT("NodeForgePrototype")) || !NumberIs(J,TEXT("version"),1))
    { Error=TEXT("Unsupported format/version or root fields");return false; }
    if (!J->HasTypedField<EJson::String>(TEXT("requestId")) || !J->TryGetStringField(TEXT("requestId"),Request.Id) || !SafeName(Request.Id)
        || !J->TryGetStringField(TEXT("assetType"),Request.Type) || !J->TryGetStringField(TEXT("assetPath"),Request.Path)
        || !J->TryGetStringField(TEXT("catalogDigest"),Request.CatalogHash)
        || !NodeForgeNodeDefinitions::AllowsRequest(Request.CatalogHash,Request.Type,1,CatalogHash) || Request.DigestValue.IsEmpty())
    { Error=TEXT("Format, request ID, type or real catalog digest mismatch");return false; }
    if ((!StringIs(J,TEXT("assetType"),TEXT("Material")) && !StringIs(J,TEXT("assetType"),TEXT("Blueprint")))
        || !Request.CatalogHash.Equals(Request.CatalogHash.ToLower(),ESearchCase::CaseSensitive))
    { Error=TEXT("assetType or catalog spelling mismatch");return false; }
    if (!ValidateTargetPath(Request.Path,Error)) return false;
    const TArray<TSharedPtr<FJsonValue>>* Nodes = nullptr; const TArray<TSharedPtr<FJsonValue>>* Edges = nullptr;
    const FObject* Settings = nullptr;
    if (!J->TryGetArrayField(TEXT("nodes"),Nodes) || !J->TryGetArrayField(TEXT("edges"),Edges) || !J->TryGetObjectField(TEXT("settings"),Settings))
    { Error = TEXT("nodes/edges/settings have wrong types"); return false; }
    auto Obj = [](const TSharedPtr<FJsonValue>& V)->FObject { return V && V->Type == EJson::Object ? V->AsObject() : nullptr; };
    auto Edge = [&](int32 I, const FString& From, const FString& Output, const FString& To, const FString& Input)->bool {
        FObject E = Obj((*Edges)[I]);
        return ExactFields(E,{TEXT("from"),TEXT("output"),TEXT("to"),TEXT("input")}) && StringIs(E,TEXT("from"),From)
            && StringIs(E,TEXT("output"),Output) && StringIs(E,TEXT("to"),To) && StringIs(E,TEXT("input"),Input);
    };
    bool Valid = false;
    if (Request.Type == TEXT("Blueprint") && Nodes->Num() == 2 && Edges->Num() == 1)
    {
        FObject E = Obj((*Nodes)[0]), P = Obj((*Nodes)[1]);
        Valid = ExactFields(E,{TEXT("id"),TEXT("class"),TEXT("owner"),TEXT("member")})
            && StringIs(E,TEXT("id"),TEXT("Begin")) && StringIs(E,TEXT("class"),UK2Node_Event::StaticClass()->GetPathName())
            && StringIs(E,TEXT("owner"),AActor::StaticClass()->GetPathName()) && StringIs(E,TEXT("member"),TEXT("ReceiveBeginPlay"))
            && ExactFields(P,{TEXT("id"),TEXT("class"),TEXT("owner"),TEXT("member")})
            && StringIs(P,TEXT("id"),TEXT("Print")) && StringIs(P,TEXT("class"),UK2Node_CallFunction::StaticClass()->GetPathName())
            && StringIs(P,TEXT("owner"),UKismetSystemLibrary::StaticClass()->GetPathName()) && StringIs(P,TEXT("member"),TEXT("PrintString"))
            && Edge(0,TEXT("Begin"),UEdGraphSchema_K2::PN_Then.ToString(),TEXT("Print"),UEdGraphSchema_K2::PN_Execute.ToString())
            && ExactFields(*Settings,{TEXT("InString"),TEXT("bPrintToScreen"),TEXT("bPrintToLog")})
            && StringIs(*Settings,TEXT("InString"),Payload) && BoolIs(*Settings,TEXT("bPrintToScreen"),false) && BoolIs(*Settings,TEXT("bPrintToLog"),true);
    }
    else if (Request.Type == TEXT("Material") && Nodes->Num() == 3 && Edges->Num() == 3)
    {
        FObject A=Obj((*Nodes)[0]), B=Obj((*Nodes)[1]), M=Obj((*Nodes)[2]);
        Valid = ExactFields(A,{TEXT("id"),TEXT("class"),TEXT("value")}) && ExactFields(B,{TEXT("id"),TEXT("class"),TEXT("value")})
            && ExactFields(M,{TEXT("id"),TEXT("class")}) && StringIs(A,TEXT("id"),TEXT("First")) && StringIs(B,TEXT("id"),TEXT("Second")) && StringIs(M,TEXT("id"),TEXT("Multiply"))
            && StringIs(A,TEXT("class"),UMaterialExpressionConstant::StaticClass()->GetPathName()) && StringIs(B,TEXT("class"),UMaterialExpressionConstant::StaticClass()->GetPathName())
            && StringIs(M,TEXT("class"),UMaterialExpressionMultiply::StaticClass()->GetPathName()) && SampleConstant(A,Request.First) && SampleConstant(B,Request.Second)
            && Edge(0,TEXT("First"),TEXT("0"),TEXT("Multiply"),TEXT("A")) && Edge(1,TEXT("Second"),TEXT("0"),TEXT("Multiply"),TEXT("B"))
            && Edge(2,TEXT("Multiply"),TEXT("0"),TEXT("Material"),TEXT("EmissiveColor"))
            && ExactFields(*Settings,{TEXT("domain"),TEXT("blend"),TEXT("shading")}) && StringIs(*Settings,TEXT("domain"),TEXT("Surface"))
            && StringIs(*Settings,TEXT("blend"),TEXT("Opaque")) && StringIs(*Settings,TEXT("shading"),TEXT("Unlit"));
    }
    if (!Valid) Error = TEXT("Unsupported minimal graph, node, member, pin, edge, value or material setting");
    return Valid;
}
static bool Collision(const FString& PackageName)
{
    if (FindPackage(nullptr,*PackageName) || FPackageName::DoesPackageExist(PackageName)) return true;
    TArray<FAssetData> Assets;
    FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get().GetAssetsByPackageName(FName(*PackageName),Assets);
    return !Assets.IsEmpty();
}
bool ValidateNewTargetIdentity(const FString& SourceObjectPath, const FString& RequestId, const FString& Target, FString& Error)
{
    if (!SafeName(RequestId) || !ValidateTargetPath(Target, Error))
    { if (Error.IsEmpty()) Error=TEXT("targetIdentityRejected"); return false; }
    if (FPackageName::ObjectPathToPackageName(SourceObjectPath).Equals(Target, ESearchCase::IgnoreCase) || Collision(Target))
    { Error=TEXT("targetExistsOrEqualsSource"); return false; }
    if (IFileManager::Get().FileExists(*(FPaths::ProjectSavedDir()/TEXT("NodeForge/Requests")/(RequestId+TEXT(".json")))))
    { Error=TEXT("requestReceiptExists"); return false; }
    return true; // A suggestion, never a target or request reservation.
}
// Reuse native naming; only bridge the project's target/name-length and
// disk/in-memory protection contract. This suggests a name, never reserves it.
bool NewCopyTarget(const FString& Source, FString& Target, FString& Error, int32 MaxLeafLength)
{
    if (MaxLeafLength < 12 || MaxLeafLength > 64 || !ValidateTargetPath(Source, Error)) return false;
    const FString Parent = FPackageName::GetLongPackagePath(Source) + TEXT("/");
    FString Base=FPackageName::GetShortName(Source);
    int32 Digits=Base.Len();
    while(Digits>0 && Base[Digits-1]>='0' && Base[Digits-1]<='9')--Digits;
    if(Digits<Base.Len() && Digits>4 && Base.Mid(Digits-4,4)==TEXT("Copy"))Base=Base.Left(Digits-4);
    const FString Stem=Base.Left(MaxLeafLength-10)+TEXT("Copy");
    FString Seed=Parent+Stem+TEXT("1");
    IAssetTools& Tools=FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
    for(int32 Attempt=0;Attempt<32;++Attempt)
    {
        FString Name;
        Tools.CreateUniqueAssetName(Seed,TEXT(""),Target,Name);
        if(!SafeName(Name) || Name.Len()>MaxLeafLength || !Target.Equals(Parent+Name,ESearchCase::CaseSensitive)
            || !ValidateTargetPath(Target, Error))
        {Error=TEXT("无法在64位安全名称范围内准备副本；未生成资产。");return false;}
        if(!Target.Equals(Source,ESearchCase::IgnoreCase) && !Collision(Target))return true;
        const FString Number=Name.Mid(Stem.Len());
        if(!Name.StartsWith(Stem,ESearchCase::CaseSensitive) || Number.IsEmpty() || !Number.IsNumeric() || Number.Len()>6)
        {Error=TEXT("原生命名建议不符合副本规则；未生成资产。");return false;}
        Seed=Parent+Stem+LexToString(FCString::Atoi(*Number)+1);
    }
    Error=TEXT("连续副本名称被未登记的磁盘或内存对象占用；请核对测试目录，未覆盖资产。");
    return false;
}
static FString AssetFile(const FString& PackageName, bool Map = false)
{
    return FPackageName::LongPackageNameToFilename(PackageName,Map ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension());
}
static bool BlueprintReadback(UBlueprint* Asset, FObject& Evidence)
{
    Evidence = MakeShared<FJsonObject>();
    UEdGraph* Graph = Asset ? FBlueprintEditorUtils::FindEventGraph(Asset) : nullptr;
    if (!Graph || Asset->ParentClass != AActor::StaticClass() || Graph->Nodes.Num() != 2) return false;
    UK2Node_Event* Event = nullptr; UK2Node_CallFunction* Print = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes) { if (auto* E=Cast<UK2Node_Event>(Node)) Event=E; if (auto* P=Cast<UK2Node_CallFunction>(Node)) Print=P; }
    if (!Event || !Print || Event->EventReference.GetMemberName()!=TEXT("ReceiveBeginPlay") || !Event->bOverrideFunction
        || Event->EventReference.GetMemberParentClass()!=AActor::StaticClass() || Print->GetTargetFunction()!=UKismetSystemLibrary::StaticClass()->FindFunctionByName(TEXT("PrintString"))) return false;
    UEdGraphPin* Then=Event->FindPin(UEdGraphSchema_K2::PN_Then,EGPD_Output); UEdGraphPin* Execute=Print->FindPin(UEdGraphSchema_K2::PN_Execute,EGPD_Input);
    if (!Then || !Execute || Then->LinkedTo.Num()!=1 || Then->LinkedTo[0]!=Execute || Execute->LinkedTo.Num()!=1 || Execute->LinkedTo[0]!=Then) return false;
    int32 Links=0;
    for (UEdGraphNode* Node : Graph->Nodes) for (UEdGraphPin* Pin : Node->Pins) Links+=Pin->LinkedTo.Num();
    UEdGraphPin* String=Print->FindPin(TEXT("InString")), *Screen=Print->FindPin(TEXT("bPrintToScreen")), *Log=Print->FindPin(TEXT("bPrintToLog"));
    if (Links!=2 || !String || !Screen || !Log || !String->DefaultValue.Equals(Payload,ESearchCase::CaseSensitive) || !Screen->DefaultValue.Equals(TEXT("false"),ESearchCase::IgnoreCase) || !Log->DefaultValue.Equals(TEXT("true"),ESearchCase::IgnoreCase)) return false;
    Evidence->SetStringField(TEXT("asset"),Asset->GetPathName()); Evidence->SetNumberField(TEXT("nodeCount"),2); Evidence->SetNumberField(TEXT("edgeCount"),1);
    Evidence->SetStringField(TEXT("eventGuid"),Event->NodeGuid.ToString()); Evidence->SetStringField(TEXT("printGuid"),Print->NodeGuid.ToString());
    Evidence->SetStringField(TEXT("generatedClass"),GetPathNameSafe(Asset->GeneratedClass));
    Evidence->SetObjectField(TEXT("beginPlay"),Pins(Event)); Evidence->SetObjectField(TEXT("printString"),Pins(Print));
    FObject Defaults=MakeShared<FJsonObject>();
    for (UEdGraphPin* Pin : Print->Pins) if (Pin->Direction==EGPD_Input) Defaults->SetStringField(Pin->PinName.ToString(),Pin->DefaultValue);
    Evidence->SetObjectField(TEXT("actualDefaults"),Defaults);
    return true;
}
static bool InputMatches(const FExpressionInput& Input, UMaterialExpression* Source)
{
    if (!Source || Source->GetOutputs().Num()!=1 || Input.Expression!=Source || Input.OutputIndex!=0) return false;
    const FExpressionOutput& Output=Source->GetOutputs()[0];
    return Input.Mask==Output.Mask && Input.MaskR==Output.MaskR && Input.MaskG==Output.MaskG && Input.MaskB==Output.MaskB && Input.MaskA==Output.MaskA;
}
static bool MaterialReadback(UMaterial* Material, FObject& Evidence, const FRequest* Expected = nullptr)
{
    Evidence=MakeShared<FJsonObject>();
    if (!Material || Material->MaterialDomain!=MD_Surface || Material->BlendMode!=BLEND_Opaque || !Material->GetShadingModels().HasOnlyShadingModel(MSM_Unlit)) return false;
    const auto& Expressions=Material->GetExpressionCollection().Expressions;
    if (Expressions.Num()!=3) return false;
    UMaterialExpressionConstant* A=nullptr,*B=nullptr; UMaterialExpressionMultiply* M=nullptr;
    for (UMaterialExpression* Expression : Expressions)
    {
        if (auto* Multiply=Cast<UMaterialExpressionMultiply>(Expression)) M=Multiply;
    }
    if (M) { A=Cast<UMaterialExpressionConstant>(M->A.Expression); B=Cast<UMaterialExpressionConstant>(M->B.Expression); }
    if (!A || !B || A==B || !Expressions.Contains(A) || !Expressions.Contains(B)
        || A->R!=static_cast<float>(Expected?Expected->First:0.5) || B->R!=static_cast<float>(Expected?Expected->Second:2.0)) return false;
    FExpressionInput* Emissive=Material->GetExpressionInputForProperty(MP_EmissiveColor);
    if (!A || !B || !M || !Emissive || !InputMatches(M->A,A) || !InputMatches(M->B,B) || !InputMatches(*Emissive,M)) return false;
    Evidence->SetStringField(TEXT("asset"),Material->GetPathName()); Evidence->SetNumberField(TEXT("expressionCount"),3); Evidence->SetNumberField(TEXT("edgeCountIncludingOutput"),3);
    Evidence->SetStringField(TEXT("settings"),TEXT("Surface/Opaque/Unlit")); Evidence->SetNumberField(TEXT("firstValue"),A->R); Evidence->SetNumberField(TEXT("secondValue"),B->R);
    Evidence->SetStringField(TEXT("first"),A->GetPathName()); Evidence->SetStringField(TEXT("second"),B->GetPathName()); Evidence->SetStringField(TEXT("multiply"),M->GetPathName());
    Evidence->SetObjectField(TEXT("constantSignature"),ExpressionSignature(A)); Evidence->SetObjectField(TEXT("multiplySignature"),ExpressionSignature(M));
    Evidence->SetStringField(TEXT("edges"),TEXT("First[0]->Multiply.A;Second[0]->Multiply.B;Multiply[0]->EmissiveColor;all masks match actual output signatures"));
    return true;
}
static bool Readback(UObject* Asset, FObject& Evidence, const FRequest* Expected = nullptr)
{
    if (Expected && Expected->Version>=2)
    {
        FString Error;
        if (Expected->Type==TEXT("MaterialInstance"))
            return NodeForgeMaterialInstances::Readback(Cast<UMaterialInstanceConstant>(Asset),Expected->Document,Evidence,Error);
        const bool Ok=Expected->Type==TEXT("Blueprint")
            ? NodeForgeBlueprintGraph::Readback(Cast<UBlueprint>(Asset),Expected->Document,Evidence,Error)
            : NodeForgeMaterialGraph::Readback(Cast<UMaterial>(Asset),Expected->Document,Evidence,Error);
        if (!Evidence) Evidence=MakeShared<FJsonObject>();
        if (!Ok) Evidence->SetStringField(TEXT("validationError"),Error);
        return Ok;
    }
    return Cast<UBlueprint>(Asset) ? BlueprintReadback(Cast<UBlueprint>(Asset),Evidence) : MaterialReadback(Cast<UMaterial>(Asset),Evidence,Expected);
}
static bool Compile(UObject* Asset, FObject& Evidence)
{
    Evidence=MakeShared<FJsonObject>();
    if (UBlueprint* BP=Cast<UBlueprint>(Asset))
    {
        FCompilerResultsLog Log; FKismetEditorUtilities::CompileBlueprint(BP,EBlueprintCompileOptions::None,&Log);
        TArray<TSharedPtr<FJsonValue>> Messages;
        for (const auto& Message : Log.Messages)
        {
            FObject Entry=MakeShared<FJsonObject>(); Entry->SetStringField(TEXT("message"),Message->ToText().ToString()); Entry->SetNumberField(TEXT("severity"),static_cast<int32>(Message->GetSeverity()));
            Messages.Add(MakeShared<FJsonValueObject>(Entry));
        }
        Evidence->SetArrayField(TEXT("messages"),Messages); Evidence->SetNumberField(TEXT("errors"),Log.NumErrors); Evidence->SetNumberField(TEXT("warnings"),Log.NumWarnings);
        Evidence->SetNumberField(TEXT("blueprintStatus"),static_cast<int32>(BP->Status));
        return Log.NumErrors==0 && (BP->Status==BS_UpToDate || BP->Status==BS_UpToDateWithWarnings) && BP->GeneratedClass && BP->GeneratedClass->ClassGeneratedBy==BP;
    }
    UMaterialInstanceConstant* Instance=Cast<UMaterialInstanceConstant>(Asset);
    UMaterial* Material=Instance?Instance->GetMaterial():Cast<UMaterial>(Asset);
    if (!Material) return false;
    const bool ParentWasDirty=Material->GetPackage()->IsDirty();
    if (Instance) UMaterialEditingLibrary::UpdateMaterialInstance(Instance);
    else UMaterialEditingLibrary::RecompileMaterial(Material);
    Evidence->SetBoolField(TEXT("inheritsParentShader"),Instance!=nullptr);
    // UE 5.3 PostEditChange uses PrecompileMode::None (on-demand). The test
    // explicitly requests the complete target map before judging compilation.
    Material->CacheShaders(EMaterialShaderPrecompileMode::Default);
    FMaterialResource* Resource=Material->GetMaterialResource(GMaxRHIFeatureLevel,EMaterialQualityLevel::High);
    Evidence->SetStringField(TEXT("requestedQuality"),TEXT("High"));
    Evidence->SetStringField(TEXT("precompileMode"),TEXT("Default; explicit complete target map request"));
    Evidence->SetStringField(TEXT("shaderPlatform"),LegacyShaderPlatformToShaderFormat(GMaxRHIShaderPlatform).ToString());
    Evidence->SetNumberField(TEXT("featureLevel"),static_cast<int32>(GMaxRHIFeatureLevel));
    Evidence->SetBoolField(TEXT("resourcePresent"),Resource!=nullptr);
    if (!Resource) return false;
    Evidence->SetNumberField(TEXT("actualResourceQuality"),static_cast<int32>(Resource->GetQualityLevel()));
    if (!Resource->IsGameThreadShaderMapComplete())
        Resource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::High);
    Resource->FinishCompilation();
    TArray<TSharedPtr<FJsonValue>> Errors; for (const FString& Error : Resource->GetCompileErrors()) Errors.Add(MakeShared<FJsonValueString>(Error));
    Evidence->SetArrayField(TEXT("errors"),Errors);
    Evidence->SetBoolField(TEXT("compilationFinished"),Resource->IsCompilationFinished());
    Evidence->SetBoolField(TEXT("shaderMapPresent"),Resource->GetGameThreadShaderMap()!=nullptr);
    Evidence->SetBoolField(TEXT("shaderMapComplete"),Resource->IsGameThreadShaderMapComplete());
    const bool ParentStatePreserved=!Instance || Material->GetPackage()->IsDirty()==ParentWasDirty;
    Evidence->SetBoolField(TEXT("parentDirtyStatePreserved"),ParentStatePreserved);
    return ParentStatePreserved && Resource->IsCompilationFinished() && Errors.IsEmpty() && Resource->GetGameThreadShaderMap() && Resource->IsGameThreadShaderMapComplete();
}

int32 Execute(const FString& Mode,const FString& InputPath,const FString& ReportPath,const FString& PreviousReport,const TSharedPtr<FJsonObject>& ReportContext)
{
    if (IFileManager::Get().FileExists(*ReportPath)) { UE_LOG(LogTemp,Error,TEXT("NodeForge refuses to overwrite report: %s"),*ReportPath); return 2; }
    TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
    // Only additive, explicitly named internal context. Never overwrite contract/check fields.
    if (ReportContext)
        for (const TCHAR* Key : {TEXT("inputMethod"), TEXT("uiInput"), TEXT("relayoutOperation"), TEXT("sourceInput"),
                                TEXT("sourceInputDigest"), TEXT("sourceAssetPath"), TEXT("derivedInputPath"), TEXT("exportOrigin")})
            if (ReportContext->HasField(Key)) Report->SetField(Key, ReportContext->Values.FindChecked(Key));
    FObject Checks=MakeShared<FJsonObject>();
    const TCHAR* Names[]={TEXT("validation"),TEXT("build"),TEXT("initialReadback"),TEXT("compile"),TEXT("finalReadback"),TEXT("publish"),TEXT("save"),TEXT("persistence"),TEXT("runtime")};
    for (const TCHAR* Name : Names) { FObject C=MakeShared<FJsonObject>(); C->SetStringField(TEXT("status"),TEXT("notRun")); Checks->SetObjectField(Name,C); }
    Report->SetStringField(TEXT("reportVersion"),TEXT("ResultContractDraft1")); Report->SetStringField(TEXT("mode"),Mode); Report->SetStringField(TEXT("engine"),FEngineVersion::Current().ToString());
    Report->SetStringField(TEXT("adapter"),Adapter); Report->SetStringField(TEXT("sessionId"),FGuid::NewGuid().ToString()); Report->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());
    Report->SetStringField(TEXT("capabilityRevision"),TEXT("UsablePrototype1: same two native structures; material constants finite 0..4; UI paste/file"));
    Report->SetStringField(TEXT("project"),FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath())); Report->SetStringField(TEXT("timeUtc"),FDateTime::UtcNow().ToIso8601());
    Report->SetObjectField(TEXT("checks"),Checks); Report->SetStringField(TEXT("requestOutcome"),TEXT("notBuilt")); Report->SetStringField(TEXT("overallStatus"),TEXT("inProgress"));
    TUniquePtr<NodeForgeResourceSource::FCapture> SourceCapture;
    TSharedPtr<FJsonObject> ExportOrigin;
    const bool ResourceReadMode = Mode==TEXT("TextureInspect") || Mode==TEXT("MaterialParameters") || Mode==TEXT("MaterialFunction") || Mode==TEXT("MaterialGraphExport") || Mode==TEXT("BlueprintGraphExport");
    FString Stage=TEXT("validation"); UObject* Owned=nullptr; FString Candidate,Target; bool Written=false,SaveAttempted=false;
    auto Check=[&](const FString& Name,const FString& Status,const FString& Reason=FString()) { FObject C=MakeShared<FJsonObject>(); C->SetStringField(TEXT("status"),Status); C->SetStringField(TEXT("reason"),Reason); C->SetStringField(TEXT("evidenceRef"),ReportPath); C->SetStringField(TEXT("source"),TEXT("thisSession")); Checks->SetObjectField(Name,C); };
    auto Finish=[&](int32 Code,FString Status,FString Reason)->int32 {
        FString SourceError;
        if (Code==0 && SourceCapture && (!SourceCapture->Verify(SourceError)
            || (ExportOrigin && FileSha256(ExportOrigin->GetStringField(TEXT("reportPath")))!=ExportOrigin->GetStringField(TEXT("reportSha256")))))
        {
            Code=ResourceReadMode?1:5; Status=ResourceReadMode?TEXT("rejected"):TEXT("unknown");
            Reason=SourceError.IsEmpty()?TEXT("exportReportChangedDuringExecution"):SourceError;
            if(ResourceReadMode)
                for(const TCHAR* Key:{TEXT("operationStatus"),TEXT("exportedRequest"),TEXT("exportEvidence"),TEXT("exportedRequestText"),TEXT("exportedRequestSha256"),TEXT("parameters"),TEXT("functionSignature"),TEXT("texture"),TEXT("nodeTemplate")})Report->RemoveField(Key);
            Report->SetStringField(TEXT("sourceValidationFailure"),Reason);
        }
        Report->SetStringField(TEXT("phase"),Status==TEXT("waitingPersistence")?TEXT("waitingPersistence"):TEXT("finished")); Report->SetStringField(TEXT("overallStatus"),Status);
        // A late source/evidence failure must not leave an earlier "passed"
        // operation field that the UI or a machine reader could trust alone.
        if (Code && StringIs(Report,TEXT("operationStatus"),TEXT("passed")))
            Report->SetStringField(TEXT("operationStatus"),Status==TEXT("unknown")?TEXT("unknown"):TEXT("failed"));
        Report->SetStringField(TEXT("nextAction"),Reason);
        if (Code) { Check(Stage,TEXT("failed"),Reason); if(!Report->HasField(TEXT("errorCode")))Report->SetStringField(TEXT("errorCode"),TEXT("NFERR-PROTOTYPE-001")); Report->SetStringField(TEXT("failedStage"),Stage); }
        FObject Residue=MakeShared<FJsonObject>(); Residue->SetStringField(TEXT("candidate"),Candidate); Residue->SetStringField(TEXT("target"),Target);
        Residue->SetStringField(TEXT("ownedObject"),GetPathNameSafe(Owned)); Residue->SetStringField(TEXT("diskState"),Written?TEXT("written"):(SaveAttempted?TEXT("unknown"):TEXT("notWritten")));
        Residue->SetBoolField(TEXT("saveAttempted"),SaveAttempted);
        Residue->SetBoolField(TEXT("targetFilePresent"),!Target.IsEmpty() && IFileManager::Get().FileExists(*AssetFile(Target)));
        Residue->SetStringField(TEXT("cleanup"),(Code && Owned)?(SaveAttempted?TEXT("Owned object and attempted target save require review; incomplete file ownership is unverified; no file deleted"):TEXT("Owned object retained only in this process at ownedObject path; this session did not attempt asset file save")):TEXT("noneRequired")); Report->SetObjectField(TEXT("residue"),Residue);
        if (!WriteJsonWithFormat(Report,ReportPath,Mode==TEXT("GraphRules"))) return 3;
        UE_LOG(LogTemp,Display,TEXT("NodeForge %s: %s (%s)"),*Mode,*Status,*ReportPath); return Code;
    };
    if (!AllowedProject()) return Finish(1,TEXT("rejected"),TEXT("Explicit selected-project/product installation binding required"));
    if(Mode==TEXT("ResourceQuery"))
    {
        FString InputText,InputHash,Error,CatalogHash;FObject Config,Evidence;
        if(!ReadInputFile(InputPath,InputText,InputHash,Error) || FTCHARToUTF8(*InputText).Length()>16384)
            return Finish(1,TEXT("rejected"),Error.IsEmpty()?TEXT("boundedResourceConfigurationRequired"):Error);
        FTCHARToUTF8 Utf8(*InputText);
        if(!NodeForgeEditorControl::ParseObject(MakeArrayView(reinterpret_cast<const uint8*>(Utf8.Get()),Utf8.Length()),Config,16384)
            || !NodeForgeInstallation::CatalogIdentity(CatalogHash,Error)
            || !NodeForgeResourceSource::Query(Config,CatalogHash,Evidence,Error))
            return Finish(1,TEXT("rejected"),Error.IsEmpty()?TEXT("resourceQueryArgumentsRejected"):Error);
        // Return before native catalog construction. This call only visits files
        // and reads registry metadata; it does not acquire source origins.
        Evidence->SetStringField(TEXT("configurationDigest"),InputHash);
        Report->SetStringField(TEXT("catalogDigest"),CatalogHash);Report->SetStringField(TEXT("operationStatus"),TEXT("passed"));
        Report->SetObjectField(TEXT("resourceQuery"),Evidence);Check(TEXT("validation"),TEXT("passed"));
        return Finish(0,TEXT("succeeded"),Evidence->GetBoolField(TEXT("complete"))
            ?TEXT("Metadata query finished; rows still require explicit inspection before use")
            :TEXT("Metadata query incomplete; inspect registry state or narrow directory; no continuation offered"));
    }
    if(Mode==TEXT("TextureInspect") || Mode==TEXT("MaterialFunction") || Mode==TEXT("MaterialParameters") || Mode==TEXT("GraphRules") || Mode==TEXT("MaterialGraphExport") || Mode==TEXT("BlueprintGraphExport") || Mode==TEXT("GraphPreview") || Mode==TEXT("GraphPreflight"))
    {
        const FString Binary=FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"));
        const FString Hash=FileDigest(Binary);
        if(Hash.IsEmpty()) return Finish(1,TEXT("rejected"),TEXT("Cannot fingerprint loaded read-only graph adapter"));
        Report->SetStringField(TEXT("adapter"),GraphAdapter);
        Report->SetStringField(TEXT("capabilityRevision"),TEXT("ReadOnlyGraphInspection2: native saved material parameters or bound function signatures; no asset construction"));
        auto Fingerprint=MakeShared<FJsonObject>();Fingerprint->SetStringField(TEXT("binary"),Binary);Fingerprint->SetStringField(TEXT("binarySha256"),Hash);
        Report->SetObjectField(TEXT("readOnlyExecutionFingerprint"),Fingerprint);
    }
    if(ResourceReadMode || (ReportContext && ReportContext->HasTypedField<EJson::Object>(TEXT("exportOrigin"))))
    {
        FString InputText, ConfigDigest, Error, Source, Kind;
        if(!ReadInputFile(InputPath,InputText,ConfigDigest,Error))return Finish(1,TEXT("rejected"),Error);
        // Bind early source rejections to the captured draft before graph parsing.
        if(!ResourceReadMode)Report->SetStringField(TEXT("inputDigest"),ConfigDigest);
        if(ResourceReadMode)
        {
            const auto Config=ReadJson(InputPath);
            const TCHAR* Key=Mode==TEXT("TextureInspect")?TEXT("texture"):Mode==TEXT("MaterialFunction")?TEXT("function"):Mode==TEXT("MaterialParameters")?TEXT("parent"):TEXT("source");
            Kind=Mode==TEXT("TextureInspect")?TEXT("Texture2D"):Mode==TEXT("MaterialFunction")?TEXT("MaterialFunction"):Mode==TEXT("BlueprintGraphExport")?TEXT("Blueprint"):TEXT("Material");
            if(FTCHARToUTF8(*InputText).Length()>16384 || !Config || !Config->TryGetStringField(Key,Source))
                return Finish(1,TEXT("rejected"),TEXT("boundedResourceConfigurationRequired"));
            if(Mode==TEXT("MaterialGraphExport") || Mode==TEXT("BlueprintGraphExport"))
            {
                FString Id,ExportTarget;
                if(!Config->TryGetStringField(TEXT("requestId"),Id) || !Config->TryGetStringField(TEXT("assetPath"),ExportTarget)
                    || !ValidateNewTargetIdentity(Source,Id,ExportTarget,Error))return Finish(1,TEXT("rejected"),Error.IsEmpty()?TEXT("targetIdentityRejected"):Error);
            }
        }
        else
        {
            const auto Ref=ReportContext->GetObjectField(TEXT("exportOrigin"));
            FString Path,Sha;
            if(!Ref->TryGetStringField(TEXT("reportPath"),Path) || !Ref->TryGetStringField(TEXT("reportSha256"),Sha)
                || !NodeForgeResourceSource::ReadExportOrigin(Path,Sha,InputText,false,ExportOrigin,Error))
                return Finish(1,TEXT("rejected"),Error.IsEmpty()?TEXT("exportOriginRejected"):Error);
            Source=ExportOrigin->GetStringField(TEXT("source"));Kind=ExportOrigin->GetStringField(TEXT("assetKind"));
            Report->SetObjectField(TEXT("exportOrigin"),ExportOrigin);
        }
        SourceCapture=MakeUnique<NodeForgeResourceSource::FCapture>();
        const int32 ReadRevision=ExportOrigin
            ? NodeForgeNodeDefinitions::Capabilities(ExportOrigin->GetStringField(TEXT("catalogDigest"))).ResourceReadRevision : 1;
        if(!SourceCapture->Begin(Source,Kind,Error,ReadRevision))return Finish(1,TEXT("rejected"),Error);
        if(ExportOrigin && SourceCapture->Sha256()!=ExportOrigin->GetStringField(TEXT("sourceAssetSha256")))
            return Finish(1,TEXT("rejected"),TEXT("exportSourceHashChanged"));
        auto Read=MakeShared<FJsonObject>();Read->SetStringField(TEXT("source"),Source);Read->SetStringField(TEXT("kind"),Kind);
        Read->SetStringField(TEXT("savedSourceSha256"),SourceCapture->Sha256());Read->SetStringField(TEXT("originProof"),TEXT("controlledSavedLoad"));
        Read->SetStringField(TEXT("configurationDigest"),ConfigDigest);Read->SetBoolField(TEXT("sourceFileHeldReadOnly"),true);
        Report->SetObjectField(ResourceReadMode?TEXT("resourceRead"):TEXT("exportSourceValidation"),Read);
    }
    if(Mode==TEXT("TextureInspect"))
    {
        FObject Evidence;FString Error;
        if(!NodeForgeResourceSource::InspectTexture(ReadJson(InputPath),Evidence,Error))return Finish(1,TEXT("rejected"),Error);
        Report->SetStringField(TEXT("operationStatus"),TEXT("passed"));Report->SetObjectField(TEXT("texture"),Evidence);
        return Finish(0,TEXT("succeeded"),TEXT("Saved Texture2D inspected; use properties.Texture and properties.SamplerType in existing graph fields"));
    }
    if(Mode==TEXT("MaterialFunction"))
    {
        const int64 Size=IFileManager::Get().FileSize(*InputPath);
        if(Size<1 || Size>16384) return Finish(1,TEXT("rejected"),TEXT("Bounded function inspection input required"));
        FObject Evidence;FString Error;
        if(!NodeForgeMaterialFunctions::Inspect(ReadJson(InputPath),Evidence,Error)) return Finish(1,TEXT("rejected"),Error);
        Report->SetStringField(TEXT("operationStatus"),TEXT("passed"));Report->SetObjectField(TEXT("functionSignature"),Evidence);
        auto Node=MakeShared<FJsonObject>();
        Node->SetStringField(TEXT("id"),TEXT("Function"));
        Node->SetStringField(TEXT("class"),TEXT("/Script/Engine.MaterialExpressionMaterialFunctionCall"));
        Node->SetObjectField(TEXT("properties"),Evidence->GetObjectField(TEXT("properties")));
        Report->SetObjectField(TEXT("nodeTemplate"),Node);
        Report->SetStringField(TEXT("templateScope"),TEXT("Single node fragment, not a complete import; use current GraphCatalog and protocol, native input names/output indices above; all required inputs must be connected; no per-call defaults or recursive dependency hash guarantee"));
        return Finish(0,TEXT("succeeded"),TEXT("Read-only native function call signature; no source asset changed"));
    }
    if (Mode==TEXT("MaterialParameters"))
    {
        const int64 Size=IFileManager::Get().FileSize(*InputPath);
        if(Size<1 || Size>16384) return Finish(1,TEXT("rejected"),TEXT("Bounded parent inspection input required"));
        FObject Config=ReadJson(InputPath),Evidence; FString Error;
        if (!NodeForgeMaterialInstances::Inspect(Config,Evidence,Error)) return Finish(1,TEXT("rejected"),Error);
        Report->SetStringField(TEXT("operationStatus"),TEXT("passed"));
        Report->SetObjectField(TEXT("parameters"),Evidence);
        return Finish(0,TEXT("succeeded"),TEXT("Read-only native global parent parameter inspection; no asset changed"));
    }
    if (Mode==TEXT("GraphLayoutInspect")) return NodeForgeLayoutChecks::Execute(InputPath,ReportPath);
    if (Mode==TEXT("VisualCreate") || Mode==TEXT("VisualCheck") || Mode==TEXT("CandidateVisualCheck")) return NodeForgeVisualChecks::Execute(Mode,InputPath,ReportPath);
    if (Mode==TEXT("GraphVisualCreate") || Mode==TEXT("GraphVisualCheck") || Mode==TEXT("GraphMapCreate")) return NodeForgeGraphChecks::Execute(Mode,InputPath,ReportPath);
    // Version dispatch is bounded and read-only. ReadRequest still performs the full
    // UTF-8, duplicate-key, type and schema checks before any candidate is created.
    FObject Envelope; FString EnvelopeText, EnvelopeError;
    const int64 InputSize=IFileManager::Get().FileSize(*InputPath);
    if (ReadInputText(InputPath, EnvelopeText, EnvelopeError))
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(EnvelopeText), Envelope);
    const bool IsGraph=Mode==TEXT("GraphCatalog") || Mode==TEXT("GraphRules") || Mode==TEXT("MaterialGraphExport") || Mode==TEXT("BlueprintGraphExport") || ((NumberIs(Envelope,TEXT("version"),2) || NumberIs(Envelope,TEXT("version"),3)) && StringIs(Envelope,TEXT("format"),TEXT("NodeForgeGraph")));
    TSharedRef<FJsonObject> ActualCatalog=MakeShared<FJsonObject>();
    if (IsGraph)
    {
        check(IsInGameThread());
        // One bounded native snapshot per loaded DLL/process; no per-import/background discovery.
        static TSharedPtr<FJsonObject> Cached;
        static bool Initializing = false;
        if (!Cached)
        {
            if (Initializing) return Finish(1,TEXT("rejected"),TEXT("Native catalog initialization recursion rejected"));
            TGuardValue<bool> Guard(Initializing,true);
            auto NativeSnapshot = MakeShared<FJsonObject>();
            NativeSnapshot->SetStringField(TEXT("engine"),FEngineVersion::Current().ToString());
            NativeSnapshot->SetStringField(TEXT("adapter"),GraphAdapter);
            const auto Material = NodeForgeMaterialGraph::Catalog();
            const auto Blueprint = NodeForgeBlueprintGraph::Catalog();
            if (!Material->GetBoolField(TEXT("nativeQualificationPassed")) || !Blueprint->GetBoolField(TEXT("nativeQualificationPassed")))
            {
                Report->SetObjectField(TEXT("materialQualification"),Material);
                Report->SetObjectField(TEXT("blueprintQualification"),Blueprint);
                return Finish(1,TEXT("rejected"),TEXT("Required existing native identities/signatures failed qualification; no partial catalog published"));
            }
            NativeSnapshot->SetObjectField(TEXT("material"),Material);
            NativeSnapshot->SetObjectField(TEXT("blueprint"),Blueprint);
            NativeSnapshot->SetObjectField(TEXT("materialInstance"),NodeForgeMaterialInstances::Catalog());
            NativeSnapshot->SetObjectField(TEXT("assetDefinitions"),NodeForgeAssetDefinitions::Metadata());
            NodeForgeNodeDefinitions::RegisterCurrentDigest(JsonDigest(NativeSnapshot));
            Cached = NativeSnapshot;
        }
        ActualCatalog = Cached.ToSharedRef();
        Report->SetStringField(TEXT("adapter"),GraphAdapter);
        Report->SetStringField(TEXT("capabilityRevision"),TEXT("Graph2: shared node definitions, exact archived capability profiles and bounded ordinary native discovery; v1 remains separately validated"));
    }
    else ActualCatalog=Catalog();
    const FString CatalogHash=JsonDigest(ActualCatalog);
    Report->SetStringField(TEXT("catalogDigest"),CatalogHash);
    if(Mode==TEXT("MaterialGraphExport") || Mode==TEXT("BlueprintGraphExport"))
    {
        FObject Request,Evidence;FString Error;
        const bool Good=InputSize>0 && InputSize<=16384 && (Mode==TEXT("MaterialGraphExport")
            ?NodeForgeMaterialGraph::Export(Envelope,CatalogHash,Request,Evidence,Error):NodeForgeBlueprintGraph::Export(Envelope,CatalogHash,Request,Evidence,Error));
        if(!Good)return Finish(1,TEXT("rejected"),Error.IsEmpty()?TEXT("Bounded graph export config required"):Error);
        FString Text;FJsonSerializer::Serialize(Request.ToSharedRef(),TJsonWriterFactory<>::Create(&Text));
        if(FTCHARToUTF8(*Text).Length()>65536) return Finish(1,TEXT("rejected"),TEXT("Exported graph exceeds import 64KiB limit; no truncated request returned"));
        Report->SetStringField(TEXT("capabilityRevision"),Mode==TEXT("MaterialGraphExport")?TEXT("ReadOnlyMaterialGraphExport1: actual graph to common description and selected v2/v3 request; native reconstruction guard"):TEXT("ReadOnlyBlueprintGraphExport1: actual Actor EventGraph and typed CDO defaults to common description and selected v2/v3 request; native reconstruction guard"));
        Report->SetStringField(TEXT("operationStatus"),TEXT("passed"));Report->SetObjectField(TEXT("exportedRequest"),Request);
        Report->SetObjectField(TEXT("exportEvidence"),Evidence);
        Report->SetStringField(TEXT("exportedRequestText"),Text);Report->SetStringField(TEXT("exportedRequestSha256"),TextSha256(Text));
        Report->SetNumberField(TEXT("graphFormatVersion"),Request->GetNumberField(TEXT("version")));
        return Finish(0,TEXT("succeeded"),TEXT("Read-only graph export; import exportedRequest separately to new asset then independently verify persistence; source not compiled or saved"));
    }
    if(Mode==TEXT("GraphRules"))
    {
        Report->SetStringField(TEXT("capabilityRevision"),TEXT("ReadOnlyTaskRules1: actual native catalog identity search and subset export; no asset mutation"));
        FObject Bundle;FString Error;
        if(InputSize<1 || InputSize>16384 || !NodeForgeTaskRules::Select(Envelope,ActualCatalog,CatalogHash,Bundle,Error))
            return Finish(1,TEXT("rejected"),Error.IsEmpty()?TEXT("Bounded task rules input required"):Error);
        Report->SetStringField(TEXT("reportVersion"),TEXT("NodeForgeTaskRules1"));
        Report->SetStringField(TEXT("operationStatus"),TEXT("passed"));Report->SetObjectField(TEXT("taskRules"),Bundle);
        return Finish(0,TEXT("succeeded"),TEXT("Read-only native capability subset; no asset created or changed; full catalog digest retained"));
    }
    if (Mode==TEXT("Catalog") || Mode==TEXT("GraphCatalog"))
    {
        Report->SetStringField(TEXT("reportVersion"),IsGraph?TEXT("NodeForgeActualCatalog2"):TEXT("NodeForgeActualCatalog1")); Report->RemoveField(TEXT("checks"));
        Report->RemoveField(TEXT("requestOutcome")); Report->RemoveField(TEXT("overallStatus"));
        Report->SetStringField(TEXT("operationStatus"),TEXT("passed")); Report->SetObjectField(TEXT("catalog"),ActualCatalog);
        return WriteJsonWithFormat(Report,ReportPath,IsGraph)?0:3;
    }
    if (Mode==TEXT("CreateMap"))
    {
        Report->SetStringField(TEXT("reportVersion"),TEXT("NodeForgeTestMapPreparation1"));
        Stage=TEXT("build"); Target=Root+TEXT("NFBlueprintTestMap");
        if (Collision(Target)) return Finish(1,TEXT("rejected"),TEXT("Test map already exists; refusing overwrite"));
        UBlueprint* BP=LoadObject<UBlueprint>(nullptr,*(Root+TEXT("NFMinimalBlueprint.NFMinimalBlueprint"))); FObject Evidence;
        if (!BlueprintReadback(BP,Evidence) || !BP->GeneratedClass) return Finish(1,TEXT("failed"),TEXT("Verified generated blueprint is required before map creation"));
        UWorld* World=GEditor->NewMap();
        AActor* Actor=World?World->SpawnActor<AActor>(BP->GeneratedClass,FTransform::Identity):nullptr;
        if (!Actor) return Finish(1,TEXT("failed"),TEXT("Test actor spawn failed"));
        Report->SetStringField(TEXT("requestOutcome"),TEXT("built"));
        Actor->SetActorLabel(TEXT("NFMinimalActor")); Check(TEXT("build"),TEXT("passed"));
        Stage=TEXT("save");
        if (FPackageName::DoesPackageExist(Target) || !UEditorLoadingAndSavingUtils::SaveMap(World,Target)) return Finish(1,TEXT("failed"),TEXT("New test map save failed"));
        Written=true; Check(TEXT("save"),TEXT("passed")); Report->SetStringField(TEXT("map"),Target); Report->SetStringField(TEXT("actorClass"),Actor->GetClass()->GetPathName());
        Report->SetStringField(TEXT("fileSha256"),FileDigest(AssetFile(Target,true))); return Finish(0,TEXT("waitingPersistence"),TEXT("Open this map in independent Editor process and execute NodeForge.BlueprintRuntime"));
    }
    FRequest Request; FString Error;
    if (!ReadRequest(InputPath,CatalogHash,Request,Error)) return Finish(1,TEXT("rejected"),Error);
    if (ReportContext && ReportContext->HasField(TEXT("uiInput")))
    {
        const FObject* UiInput=nullptr;FString ExpectedDigest;
        if (!ReportContext->TryGetObjectField(TEXT("uiInput"),UiInput) || !UiInput || !*UiInput
            || !(*UiInput)->TryGetStringField(TEXT("snapshotDigest"),ExpectedDigest)
            || !Request.DigestValue.Equals(ExpectedDigest,ESearchCase::CaseSensitive))
            return Finish(1,TEXT("rejected"),TEXT("UI snapshot digest differs from consumed input; no construction attempted"));
    }
    Report->SetStringField(TEXT("inputCatalogDigest"),Request.CatalogHash);
    Report->SetStringField(TEXT("catalogCompatibility"),Request.CatalogHash==CatalogHash?TEXT("current"):TEXT("archived v2 subset; original input unchanged"));
    Target=Request.Path; Report->SetStringField(TEXT("requestId"),Request.Id); Report->SetStringField(TEXT("inputDigest"),Request.DigestValue); Report->SetStringField(TEXT("assetType"),Request.Type); Report->SetStringField(TEXT("assetPath"),Target);
    auto ConflictReason=[&]()->FString
    {
        const FString Reservation=FPaths::ProjectSavedDir()/TEXT("NodeForge/Requests")/(Request.Id+TEXT(".json"));
        const bool HasTarget=Collision(Target),HasRequest=IFileManager::Get().FileExists(*Reservation);
        if(!HasTarget && !HasRequest)return FString();
        auto Details=MakeShared<FJsonObject>();
        Details->SetStringField(TEXT("kind"),HasTarget?(HasRequest?TEXT("targetAndRequestExist"):TEXT("targetExists")):TEXT("requestExists"));
        Details->SetBoolField(TEXT("targetExists"),HasTarget);Details->SetBoolField(TEXT("requestExists"),HasRequest);
        Details->SetStringField(TEXT("assetPath"),Target);Details->SetStringField(TEXT("requestId"),Request.Id);
        Details->SetStringField(TEXT("requestReceipt"),HasRequest?FPaths::ConvertRelativePathToFull(Reservation):FString());
        Details->SetStringField(TEXT("resolution"),TEXT("PrepareNewCopy; explicit new asset and request identity, never overwrite or auto-retry"));
        Report->SetObjectField(TEXT("conflict"),Details);Report->SetStringField(TEXT("requestOutcome"),TEXT("conflict"));
        Report->SetStringField(TEXT("errorCode"),HasTarget?(HasRequest?TEXT("NFERR-TARGET-AND-REQUEST-EXIST"):TEXT("NFERR-TARGET-EXISTS")):TEXT("NFERR-REQUEST-EXISTS"));
        return HasTarget
            ?TEXT("目标资产已存在，未覆盖。点击“准备新副本”获得新名称和新请求，再明确生成。")
            :TEXT("此请求ID已使用，本次未再次生成。点击“准备新副本”获得新请求和新目标，再明确生成。");
    };
    if(Mode==TEXT("GraphPreview"))
    {
        if(Request.Version<2 || (Request.Type!=TEXT("Blueprint") && Request.Type!=TEXT("Material")))
            return Finish(1,TEXT("rejected"),TEXT("Preview requires a v2/v3 Blueprint or Material request, not an instance or legacy template"));
        const FString PreviewConflict=ConflictReason();
        if(!PreviewConflict.IsEmpty())return Finish(1,TEXT("rejected"),PreviewConflict);
        FString PreviousPath=FPaths::ConvertRelativePathToFull(PreviousReport);FPaths::CollapseRelativeDirectories(PreviousPath);
        const FString ReportRoot=FPaths::ConvertRelativePathToFull(NodeForgeInstallation::StateDirectory()/TEXT("Reports"))+TEXT("/");
        const int64 PreviousSize=IFileManager::Get().FileSize(*PreviousPath);
        if(!PreviousPath.StartsWith(ReportRoot,ESearchCase::IgnoreCase) || PreviousSize<1 || PreviousSize>1048576)
            return Finish(1,TEXT("rejected"),TEXT("Preview requires a bounded previous actual graph export report under Reports"));
        FString Source,SourceHash;
        if(ExportOrigin)
        {
            if(!PreviousPath.Equals(ExportOrigin->GetStringField(TEXT("reportPath")),ESearchCase::IgnoreCase))
                return Finish(1,TEXT("rejected"),TEXT("previewOriginMismatch"));
            // The common production source guard already validated the report SHA,
            // current DLL/project/payload and held the actual saved source read-only.
            Source=ExportOrigin->GetStringField(TEXT("source"));SourceHash=ExportOrigin->GetStringField(TEXT("sourceAssetSha256"));
        }
        else
        {
            // Existing internal callers supply a report directly; no new API exposes this path.
            const FObject Previous=ReadJson(PreviousPath);const FObject* PreviousEvidence=nullptr;const FObject* PreviousFingerprint=nullptr;
            const FString ExpectedMode=Request.Type==TEXT("Blueprint")?TEXT("BlueprintGraphExport"):TEXT("MaterialGraphExport");
            if(!StringIs(Previous,TEXT("mode"),ExpectedMode) || !StringIs(Previous,TEXT("operationStatus"),TEXT("passed"))
                || !Previous->TryGetObjectField(TEXT("exportEvidence"),PreviousEvidence)
                || !Previous->TryGetObjectField(TEXT("readOnlyExecutionFingerprint"),PreviousFingerprint)
                || !StringIs(*PreviousFingerprint,TEXT("binarySha256"),FileDigest(FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor")))))
                return Finish(1,TEXT("rejected"),TEXT("Preview needs a matching successful source export from this actual DLL"));
            if(!(*PreviousEvidence)->TryGetStringField(TEXT("source"),Source) || !(*PreviousEvidence)->TryGetStringField(TEXT("sourceAssetSha256"),SourceHash) || SourceHash.IsEmpty())
                return Finish(1,TEXT("rejected"),TEXT("Source identity or saved SHA missing"));
        }
        auto Config=MakeShared<FJsonObject>();Config->SetStringField(TEXT("source"),Source);
        Config->SetStringField(TEXT("requestId"),Request.Id);Config->SetStringField(TEXT("assetPath"),Request.Path);
        Config->SetNumberField(TEXT("formatVersion"),Request.Version);
        FObject Baseline,Evidence;
        const bool Exported=Request.Type==TEXT("Blueprint")?NodeForgeBlueprintGraph::Export(Config,CatalogHash,Baseline,Evidence,Error):NodeForgeMaterialGraph::Export(Config,CatalogHash,Baseline,Evidence,Error);
        if(!Exported)return Finish(1,TEXT("rejected"),Error);
        if(!StringIs(Evidence,TEXT("sourceAssetSha256"),SourceHash))
            return Finish(1,TEXT("rejected"),TEXT("Source saved bytes changed after export; re-export before previewing"));
        Check(TEXT("validation"),TEXT("passed"),TEXT("Common modern validator and actual source exporter; fresh target and source snapshot checked"));
        NodeForgeGraphDocument::FGraphDocument BaselineDocument;
        if(!NodeForgeGraphCodec::Decode(Baseline,BaselineDocument,Error))return Finish(1,TEXT("rejected"),Error);
        Report->SetObjectField(TEXT("graphDifference"),NodeForgeGraphDiff::Compare(BaselineDocument,Request.Document));
        Report->SetObjectField(TEXT("sourceEvidence"),Evidence);Report->SetStringField(TEXT("previousExportReport"),PreviousPath);
        Report->SetStringField(TEXT("operationStatus"),TEXT("passed"));
        Report->SetStringField(TEXT("capabilityRevision"),TEXT("ReadOnlyGraphPreview1: actual saved source to validated new-target request delta; no persistent asset created"));
        return Finish(0,TEXT("succeeded"),TEXT("Preview only. Import the reviewed request separately to its new target; no source change, compile/save or runtime success implied"));
    }
    if (Mode==TEXT("RelayoutCopy"))
    {
        if (Request.Version<2 || Request.Type==TEXT("MaterialInstance")) return Finish(1,TEXT("rejected"),TEXT("RelayoutCopy requires a v2/v3 blueprint/material graph; instances have no graph"));
        const FString SourceTarget=Target, SourceDigest=Request.DigestValue;
        for (auto& Node : Request.Document.Graphs[0].Nodes) Node.Position.Reset();
        const FString CopyId=TEXT("GraphLayout")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
        Request.Document.RequestId=CopyId;
        const FString CopyTarget = FPackageName::GetLongPackagePath(SourceTarget) + TEXT("/") + CopyId;
        if (!ValidateTargetPath(CopyTarget, Error)) return Finish(1, TEXT("rejected"), Error);
        Request.Document.Target=CopyTarget;
        Request.Json=NodeForgeGraphCodec::Encode(Request.Document,Request.Version);
        const FString DerivedInput=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("NodeForge/RelayoutInputs")/(CopyId+TEXT(".json")));
        if (!WriteJson(Request.Json.ToSharedRef(),DerivedInput)) return Finish(1,TEXT("rejected"),TEXT("Cannot save new relayout input; no asset mutation attempted"));
        auto CopyContext=MakeShared<FJsonObject>();
        if (ReportContext) CopyContext->Values=ReportContext->Values;
        CopyContext->SetStringField(TEXT("relayoutOperation"),TEXT("Replay last input without positions into a new asset; original asset not edited or read as source"));
        CopyContext->SetStringField(TEXT("sourceInput"),InputPath);
        CopyContext->SetStringField(TEXT("sourceInputDigest"),SourceDigest);
        CopyContext->SetStringField(TEXT("sourceAssetPath"),SourceTarget);
        CopyContext->SetStringField(TEXT("derivedInputPath"),DerivedInput);
        if (ReportContext && ReportContext->HasTypedField<EJson::Object>(TEXT("uiInput")))
        {
            auto DerivedUi=MakeShared<FJsonObject>();DerivedUi->Values=ReportContext->GetObjectField(TEXT("uiInput"))->Values;
            DerivedUi->SetStringField(TEXT("sourceSnapshotPath"),InputPath);
            DerivedUi->SetStringField(TEXT("sourceSnapshotDigest"),SourceDigest);
            DerivedUi->SetStringField(TEXT("snapshotPath"),DerivedInput);
            DerivedUi->SetStringField(TEXT("snapshotDigest"),FileDigest(DerivedInput));
            CopyContext->SetObjectField(TEXT("uiInput"),DerivedUi);
        }
        return Execute(TEXT("Import"),DerivedInput,ReportPath,FString(),CopyContext);
    }
    FObject Fingerprint=MakeShared<FJsonObject>(); Fingerprint->SetStringField(TEXT("engine"),FEngineVersion::Current().ToString()); Fingerprint->SetStringField(TEXT("adapter"),IsGraph?GraphAdapter:Adapter); Fingerprint->SetStringField(TEXT("catalog"),CatalogHash); Fingerprint->SetStringField(TEXT("target"),Target); Fingerprint->SetStringField(TEXT("allowedRoot"),Root);
    if (IsGraph)
    {
        const FString BinaryHash=FileDigest(FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor")));
        if (BinaryHash.IsEmpty()) return Finish(1,TEXT("rejected"),TEXT("Cannot fingerprint the loaded graph adapter binary"));
        Fingerprint->SetStringField(TEXT("binarySha256"),BinaryHash);
        Fingerprint->SetNumberField(TEXT("protocolVersion"),Request.Version);
    }
    Report->SetObjectField(TEXT("executionFingerprint"),Fingerprint);
    const FString FingerprintHash=JsonDigest(Fingerprint.ToSharedRef()); Report->SetStringField(TEXT("executionFingerprintDigest"),FingerprintHash);
    Check(TEXT("validation"),TEXT("passed"),IsGraph?TEXT("Versioned graph schema, native capability catalog, properties/defaults/edges and input encoding checked"):TEXT("Exact minimal graph, actual catalog, UTF-8, duplicate keys, allowlist and types checked"));
    if(Mode==TEXT("PrepareNewCopy"))
    {
        FString NewTarget;
        if(!NewCopyTarget(Target,NewTarget,Error))return Finish(1,TEXT("rejected"),Error);
        FString NewId;
        for(int32 Attempt=0;Attempt<4;++Attempt)
        {
            NewId=TEXT("Copy")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
            if(!IFileManager::Get().FileExists(*(FPaths::ProjectSavedDir()/TEXT("NodeForge/Requests")/(NewId+TEXT(".json")))))break;
            NewId.Empty();
        }
        if(NewId.IsEmpty())return Finish(1,TEXT("rejected"),TEXT("无法取得新的请求身份；未生成资产。"));
        FObject Prepared=MakeShared<FJsonObject>();
        if(Request.Version>=2)
        {
            auto Copy=Request.Document;Copy.RequestId=NewId;Copy.Target=NewTarget;
            Prepared=NodeForgeGraphCodec::Encode(Copy,Request.Version);
        }
        else {Prepared->Values=Request.Json->Values;Prepared->SetStringField(TEXT("requestId"),NewId);Prepared->SetStringField(TEXT("assetPath"),NewTarget);}
        FString PreparedText;
        if(!FJsonSerializer::Serialize(Prepared.ToSharedRef(),TJsonWriterFactory<>::Create(&PreparedText)) || FTCHARToUTF8(*PreparedText).Length()>65536)
            return Finish(1,TEXT("rejected"),TEXT("准备的副本输入超过64 KiB；原输入保持，未截断或生成。"));
        const FString PreparedPath=NodeForgeInstallation::StateDirectory()/TEXT("Reports/PreparedCopies")/(NewId+TEXT(".json"));
        if(!WriteJson(Prepared.ToSharedRef(),PreparedPath))return Finish(1,TEXT("rejected"),TEXT("无法保存副本输入记录；原输入保持，未生成资产。"));
        if(!FileDigest(InputPath).Equals(Request.DigestValue,ESearchCase::CaseSensitive))
            return Finish(1,TEXT("rejected"),TEXT("准备期间源输入发生变化；派生记录保留检查，未生成资产。"));
        auto Evidence=MakeShared<FJsonObject>();
        Evidence->SetStringField(TEXT("sourceInput"),FPaths::ConvertRelativePathToFull(InputPath));
        Evidence->SetStringField(TEXT("sourceInputDigest"),Request.DigestValue);
        Evidence->SetStringField(TEXT("originalAssetPath"),Target);Evidence->SetStringField(TEXT("originalRequestId"),Request.Id);
        Evidence->SetStringField(TEXT("newAssetPath"),NewTarget);Evidence->SetStringField(TEXT("newRequestId"),NewId);
        Evidence->SetStringField(TEXT("preparedInputPath"),PreparedPath);Evidence->SetStringField(TEXT("preparedInputDigest"),FileDigest(PreparedPath));
        Evidence->SetStringField(TEXT("namingProvider"),TEXT("UE5.3.2 IAssetTools::CreateUniqueAssetName + existing disk/memory/registry guards"));
        Evidence->SetArrayField(TEXT("changedFields"),{MakeShared<FJsonValueString>(Request.Version==3?TEXT("target.assetPath"):TEXT("assetPath")),MakeShared<FJsonValueString>(TEXT("requestId"))});
        Evidence->SetBoolField(TEXT("sourceInputUnchanged"),true);Evidence->SetBoolField(TEXT("targetReserved"),false);
        Evidence->SetBoolField(TEXT("assetGenerated"),false);
        Report->SetObjectField(TEXT("copyPreparation"),Evidence);Report->SetStringField(TEXT("operationStatus"),TEXT("passed"));
        Report->SetStringField(TEXT("capabilityRevision"),TEXT("ExplicitNewCopy1: validated identity-only derived input; no asset construction, compilation, save or reservation"));
        return Finish(0,TEXT("succeeded"),TEXT("新副本输入已准备；确认新名称后明确生成。准备不预留名称，生成时再次校验。"));
    }
    if(Mode==TEXT("GraphPreflight"))
    {
        const FString PreflightConflict=ConflictReason();
        if(!PreflightConflict.IsEmpty())return Finish(1,TEXT("rejected"),PreflightConflict);
        // ReadRequest above is the same parser/domain validator used by Import.
        // Counts describe input plans, not constructed objects or compiled assets.
        const auto Summary=MakeShared<FJsonObject>();
        const auto Classes=MakeShared<FJsonObject>();
        TArray<TSharedPtr<FJsonValue>> Identities;
        int32 DefaultCount=0,ExplicitPositionCount=0;
        int32 EdgeCount=0;
        const auto Projection=Request.Version>=2?NodeForgeGraphCodec::Encode(Request.Document,2):Request.Json;
        if(Request.Version<2)
        {
            // Preserve the already-supported fixed v1 vocabulary. Do not turn
            // it into a modern graph or invent properties during a read-only check.
            for(const auto& Value:Request.Json->GetArrayField(TEXT("nodes")))
            {
                const auto Node=Value->AsObject();const FString Class=Node->GetStringField(TEXT("class"));
                double Count=0;Classes->TryGetNumberField(Class,Count);Classes->SetNumberField(Class,Count+1);
                Identities.Add(Value);
            }
            EdgeCount=Request.Json->GetArrayField(TEXT("edges")).Num();
        }
        for(const auto& Graph:Request.Document.Graphs)
        {
            EdgeCount+=Graph.Edges.Num();
            for(const auto& Node:Graph.Nodes)
            {
                double Count=0;Classes->TryGetNumberField(Node.Class,Count);Classes->SetNumberField(Node.Class,Count+1);
                auto Identity=MakeShared<FJsonObject>();Identity->SetStringField(TEXT("id"),Node.Id);Identity->SetStringField(TEXT("class"),Node.Class);
                const auto* LegacyNode=Projection->GetArrayField(TEXT("nodes")).FindByPredicate([&](const auto& Value){return Value->AsObject()->GetStringField(TEXT("id")).Equals(Node.Id,ESearchCase::CaseSensitive);});
                check(LegacyNode);Identity->SetObjectField(TEXT("properties"),(*LegacyNode)->AsObject()->GetObjectField(TEXT("properties")));
                Identities.Add(MakeShared<FJsonValueObject>(Identity));DefaultCount+=Node.Defaults.Num();if(Node.Position.IsSet())++ExplicitPositionCount;
            }
        }
        Summary->SetStringField(TEXT("format"),TEXT("NodeForgeGraphPreflight"));Summary->SetNumberField(TEXT("version"),1);
        Summary->SetNumberField(TEXT("plannedNodeCount"),Identities.Num());
        Summary->SetNumberField(TEXT("plannedEdgeCount"),EdgeCount);
        Summary->SetNumberField(TEXT("explicitDefaultCount"),DefaultCount);Summary->SetNumberField(TEXT("explicitPositionCount"),ExplicitPositionCount);
        Summary->SetObjectField(TEXT("nodeClassCounts"),Classes);Summary->SetArrayField(TEXT("nodeIdentities"),Identities);
        Summary->SetObjectField(TEXT("settings"),Projection->GetObjectField(TEXT("settings")));
        Summary->SetNumberField(TEXT("inputProtocolVersion"),Request.Version);
        Summary->SetBoolField(TEXT("targetAvailableAtCheck"),true);Summary->SetBoolField(TEXT("requestAvailableAtCheck"),true);Summary->SetBoolField(TEXT("requestReserved"),false);
        Summary->SetStringField(TEXT("scope"),TEXT("Same Import parser/domain validation and collision checks; native catalog/resource inspection may use transient instances. No target construction, compilation, reservation, publication or save. Availability may change; Import always validates again."));
        Report->SetStringField(TEXT("reportVersion"),TEXT("NodeForgeGraphPreflight1"));
        Report->SetStringField(TEXT("capabilityRevision"),TEXT("GraphPreflight1: same modern input validation and planned graph summary; no generation"));
        Report->SetStringField(TEXT("operationStatus"),TEXT("passed"));Report->SetObjectField(TEXT("preflightSummary"),Summary);
        return Finish(0,TEXT("succeeded"),TEXT("Input preflight passed only. Import separately to build/compile/save, then independently Verify and test runtime. Target/request availability is not reserved."));
    }
    if (Mode==TEXT("Verify"))
    {
        Stage=TEXT("persistence"); FObject Previous=ReadJson(PreviousReport);
        const FString SavedHash=FileDigest(AssetFile(Target));
        if (SavedHash.IsEmpty() || !Previous || !StringIs(Previous,TEXT("inputDigest"),Request.DigestValue) || !StringIs(Previous,TEXT("executionFingerprintDigest"),FingerprintHash)
            || !StringIs(Previous,TEXT("overallStatus"),TEXT("waitingPersistence")) || Previous->GetNumberField(TEXT("pid"))==FPlatformProcess::GetCurrentProcessId()
            || !StringIs(Previous,TEXT("fileSha256"),SavedHash))
            return Finish(1,TEXT("failed"),TEXT("Independent verification needs matching saved-generation receipt, bytes, fingerprint and different process"));
        const FObject* PreviousChecks=nullptr;
        if (!Previous->TryGetObjectField(TEXT("checks"),PreviousChecks)) return Finish(1,TEXT("failed"),TEXT("Prior check evidence missing"));
        for (int32 I=0;I<7;++I)
        {
            const FObject* Prior=nullptr;
            if (!(*PreviousChecks)->TryGetObjectField(Names[I],Prior) || !StringIs(*Prior,TEXT("status"),TEXT("passed"))) return Finish(1,TEXT("failed"),TEXT("Prior required generation check did not pass"));
            FObject C=MakeShared<FJsonObject>(); C->SetStringField(TEXT("status"),TEXT("passed")); C->SetStringField(TEXT("source"),TEXT("previousGenerationReceipt")); C->SetStringField(TEXT("evidenceRef"),PreviousReport); Checks->SetObjectField(Names[I],C);
        }
        if (FindPackage(nullptr,*Target)) return Finish(1,TEXT("failed"),TEXT("Target unexpectedly loaded before independent verification"));
        UObject* Asset=LoadObject<UObject>(nullptr,*(Target+TEXT(".")+FPackageName::GetShortName(Target))); FObject Evidence,Compilation;
        Written=true;
        const bool StructureOk=Readback(Asset,Evidence,&Request); Report->SetObjectField(TEXT("reopenedReadback"),Evidence);
        if (!StructureOk) return Finish(1,TEXT("failed"),TEXT("Reloaded asset structure failed"));
        const bool CompileOk=Compile(Asset,Compilation); Report->SetObjectField(TEXT("reopenedCompilation"),Compilation);
        if (!CompileOk) return Finish(1,TEXT("failed"),TEXT("Reloaded asset compilation failed"));
        const bool FinalOk=Readback(Asset,Evidence,&Request); Report->SetObjectField(TEXT("reopenedFinalReadback"),Evidence);
        if (!FinalOk) return Finish(1,TEXT("failed"),TEXT("Reopened postcompile structure failed"));
        Report->SetStringField(TEXT("previousReportRef"),PreviousReport);
        Report->SetStringField(TEXT("fileSha256"),FileDigest(AssetFile(Target))); Report->SetStringField(TEXT("requestOutcome"),TEXT("alreadyCompleted"));
        Report->SetStringField(TEXT("operation"),TEXT("verificationOnly; reused prior generation checks plus current identity/bytes/structure/compilation verification; no construction or resave"));
        Check(TEXT("persistence"),TEXT("passed"),TEXT("Fresh process loaded identical saved bytes, checked graph and compiled; no resave performed"));
        return Finish(0,TEXT("succeeded"),TEXT("Generation and independent persistence passed; runtime remains a separate test"));
    }
    if (Mode!=TEXT("Import")) return Finish(1,TEXT("rejected"),TEXT("Unknown operation"));
    const FString Receipt=FPaths::ProjectSavedDir()/TEXT("NodeForge/Requests")/(Request.Id+TEXT(".json"));
    const FString ImportConflict=ConflictReason();
    if(!ImportConflict.IsEmpty())return Finish(1,TEXT("rejected"),ImportConflict);
    Check(TEXT("validation"),TEXT("passed"),TEXT("All minimal input checks and disk/memory/registry/request collision checks passed"));
    // Reserve this exact request before mutation; failure retries require a new request and target.
    FObject Reservation=MakeShared<FJsonObject>(); Reservation->SetStringField(TEXT("inputDigest"),Request.DigestValue); Reservation->SetStringField(TEXT("reportRef"),ReportPath); Reservation->SetStringField(TEXT("target"),Target);
    if (!WriteJson(Reservation.ToSharedRef(),Receipt)) return Finish(1,TEXT("rejected"),TEXT("Cannot reserve request receipt"));
    Stage=TEXT("build"); Candidate=TEXT("/Game/NodeForgeStaging/Session")+FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT("/")+FPackageName::GetShortName(Target);
    UPackage* Package=CreatePackage(*Candidate); const FString Name=FPackageName::GetShortName(Target);
    Report->SetStringField(TEXT("requestOutcome"),TEXT("built"));
    if (Request.Version>=2)
    {
        if (Request.Type==TEXT("Blueprint")) Owned=NodeForgeBlueprintGraph::Build(Package,Name,Request.Document,Error);
        else if (Request.Type==TEXT("MaterialInstance")) Owned=NodeForgeMaterialInstances::Build(Package,Name,Request.Document,Error);
        else Owned=NodeForgeMaterialGraph::Build(Package,Name,Request.Document,Error);
        if (!Owned)
        {
            Owned=FindObject<UObject>(Package,*Name);
            return Finish(1,TEXT("cleanupRequired"),Error.IsEmpty()?TEXT("Graph construction failed"):Error);
        }
    }
    else if (Request.Type==TEXT("Blueprint"))
    {
        FBlueprintParts Parts=Blueprint(Package,Name); Owned=Parts.Asset;
        if (!Owned || !Parts.Event || !Parts.Print) return Finish(1,TEXT("cleanupRequired"),TEXT("Native blueprint construction failed"));
        const UEdGraphSchema_K2* Schema=GetDefault<UEdGraphSchema_K2>();
        Schema->TrySetDefaultValue(*Parts.Print->FindPin(TEXT("InString")),Payload); Schema->TrySetDefaultValue(*Parts.Print->FindPin(TEXT("bPrintToScreen")),TEXT("false")); Schema->TrySetDefaultValue(*Parts.Print->FindPin(TEXT("bPrintToLog")),TEXT("true"));
        UEdGraphPin* Then=Parts.Event->FindPin(UEdGraphSchema_K2::PN_Then); UEdGraphPin* Exec=Parts.Print->FindPin(UEdGraphSchema_K2::PN_Execute);
        if (!Then || !Exec || Schema->CanCreateConnection(Then,Exec).Response!=CONNECT_RESPONSE_MAKE || !Schema->TryCreateConnection(Then,Exec)) return Finish(1,TEXT("cleanupRequired"),TEXT("Native K2 connection refused or would perform implicit conversion"));
    }
    else
    {
        UMaterial* Material=NewObject<UMaterial>(Package,*Name,RF_Public|RF_Standalone|RF_Transactional); Owned=Material;
        Material->MaterialDomain=MD_Surface; Material->BlendMode=BLEND_Opaque; Material->SetShadingModel(MSM_Unlit);
        auto* A=Cast<UMaterialExpressionConstant>(UMaterialEditingLibrary::CreateMaterialExpression(Material,UMaterialExpressionConstant::StaticClass(),-500,-100));
        auto* B=Cast<UMaterialExpressionConstant>(UMaterialEditingLibrary::CreateMaterialExpression(Material,UMaterialExpressionConstant::StaticClass(),-500,100));
        auto* M=Cast<UMaterialExpressionMultiply>(UMaterialEditingLibrary::CreateMaterialExpression(Material,UMaterialExpressionMultiply::StaticClass(),-250,0));
        if (!A || !B || !M) return Finish(1,TEXT("cleanupRequired"),TEXT("Native material expression construction failed"));
        A->R=static_cast<float>(Request.First); B->R=static_cast<float>(Request.Second);
        if (A->GetOutputs().Num()!=1 || B->GetOutputs().Num()!=1 || M->GetOutputs().Num()!=1 || !A->GetOutputs()[0].OutputName.IsNone() || !B->GetOutputs()[0].OutputName.IsNone() || !M->GetOutputs()[0].OutputName.IsNone()
            || !UMaterialEditingLibrary::ConnectMaterialExpressions(A,TEXT(""),M,TEXT("A")) || !UMaterialEditingLibrary::ConnectMaterialExpressions(B,TEXT(""),M,TEXT("B")) || !UMaterialEditingLibrary::ConnectMaterialProperty(M,TEXT(""),MP_EmissiveColor))
            return Finish(1,TEXT("cleanupRequired"),TEXT("Actual unnamed output 0 mapping or explicit material connection failed"));
    }
    Check(TEXT("build"),TEXT("passed")); Stage=TEXT("initialReadback"); FObject Evidence;
    if (!Readback(Owned,Evidence,&Request)) { Report->SetObjectField(TEXT("initialReadback"),Evidence); return Finish(1,TEXT("cleanupRequired"),TEXT("Initial exact structure readback failed")); }
    Report->SetObjectField(TEXT("initialReadback"),Evidence); Check(Stage,TEXT("passed")); Stage=TEXT("compile"); FObject Compilation;
    const bool Compiled=Compile(Owned,Compilation); Report->SetObjectField(TEXT("compilation"),Compilation);
    if (!Compiled) return Finish(1,TEXT("cleanupRequired"),TEXT("Native compilation failed or target shader resource is unavailable/incomplete"));
    Check(Stage,TEXT("passed")); Stage=TEXT("finalReadback");
    if (!Readback(Owned,Evidence,&Request)) { Report->SetObjectField(TEXT("finalReadback"),Evidence); return Finish(1,TEXT("cleanupRequired"),TEXT("Postcompile structure readback failed")); }
    Report->SetObjectField(TEXT("finalReadback"),Evidence); Check(Stage,TEXT("passed")); Stage=TEXT("publish");
    if (Collision(Target)) return Finish(1,TEXT("cleanupRequired"),TEXT("Target appeared before publish; candidate remains unsaved"));
    UPackage* Destination=CreatePackage(*Target);
    if (!Owned->Rename(*Name,Destination,REN_DontCreateRedirectors|REN_NonTransactional)) return Finish(1,TEXT("cleanupRequired"),TEXT("Owned candidate rename failed"));
    if (UBlueprint* BP=Cast<UBlueprint>(Owned))
        if (!BP->GeneratedClass || BP->GeneratedClass->GetOutermost()!=Destination || !BP->SkeletonGeneratedClass || BP->SkeletonGeneratedClass->GetOutermost()!=Destination || BP->GeneratedClass->ClassGeneratedBy!=BP)
            return Finish(1,TEXT("cleanupRequired"),TEXT("Generated or skeleton class did not migrate to final package"));
    if (Owned->GetOutermost()!=Destination || !Readback(Owned,Evidence,&Request)) { Report->SetObjectField(TEXT("publishedReadback"),Evidence); return Finish(1,TEXT("cleanupRequired"),TEXT("Final identity or graph changed during publish")); }
    Package->SetDirtyFlag(false); // This session's now-empty staging package is never a user asset.
    Report->SetObjectField(TEXT("publishedReadback"),Evidence); FAssetRegistryModule::AssetCreated(Owned); Owned->MarkPackageDirty(); Check(Stage,TEXT("passed")); Stage=TEXT("save");
    const FString Filename=AssetFile(Target);
    if (FPackageName::DoesPackageExist(Target) || IFileManager::Get().FileExists(*Filename)) return Finish(1,TEXT("cleanupRequired"),TEXT("Target file appeared before save; refusing overwrite"));
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename),true);
    FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone;
    SaveAttempted=true;
    if (!UPackage::SavePackage(Destination,Owned,*Filename,Args) || IFileManager::Get().FileSize(*Filename)<=0) return Finish(1,TEXT("cleanupRequired"),TEXT("Synchronous package save failed; check partial residue"));
    Written=true; const FString SavedHash=FileDigest(Filename);
    if (SavedHash.IsEmpty()) return Finish(1,TEXT("cleanupRequired"),TEXT("Asset saved but disk SHA-256 evidence could not be read"));
    Check(Stage,TEXT("passed")); Report->SetStringField(TEXT("file"),FPaths::ConvertRelativePathToFull(Filename)); Report->SetStringField(TEXT("fileSha256"),SavedHash);
    return Finish(0,TEXT("waitingPersistence"),TEXT("Saved new asset; independently reopen with Verify and this generation report before calling generation succeeded"));
}
}
