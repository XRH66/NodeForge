#include "NodeForgeControlOperations.h"
#include "NodeForgePrototype.h"
#include "NodeForgeInstallation.h"
#include "NodeForgeResourceSource.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/EngineVersion.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace NodeForgeControlOperations
{
namespace
{
constexpr const TCHAR* ApiVersion = TEXT("1.0");
// The only business operation definition; describe and dispatch consume this table.
const FOperationDefinition Operations[] = {
    {TEXT("operations.describe"), nullptr, false, false, TEXT("capabilityDescription"), TEXT("reportOnly"), 3, EArguments::None, EDispatch::ReadOnly, false, EEditorFeature::Read},
    {TEXT("graphs.generateFromFile"), TEXT("Import"), true, false, TEXT("generationReceipt"), TEXT("createNewAssetAndRequestReceipt"), 1, EArguments::None, EDispatch::GameThread, false, EEditorFeature::Graph},
    {TEXT("graphs.preflightFromFile"), TEXT("GraphPreflight"), true, false, TEXT("preflightReceipt"), TEXT("reportOnly"), 1, EArguments::None, EDispatch::GameThread, false, EEditorFeature::Graph},
    {TEXT("graphs.verifySaved"), TEXT("Verify"), true, true, TEXT("verificationReceipt"), TEXT("loadAndCompileWithoutResave"), 1, EArguments::None, EDispatch::GameThread, false, EEditorFeature::Graph},
    {TEXT("session.hello"), nullptr, false, false, TEXT("sessionIdentity"), TEXT("bindConnection"), 2, EArguments::Hello, EDispatch::Handshake, false, EEditorFeature::Read},
    {TEXT("editor.getState"), nullptr, false, false, TEXT("publishedState"), TEXT("none"), 2, EArguments::None, EDispatch::ReadOnly, false, EEditorFeature::Read},
    {TEXT("draft.get"), nullptr, false, false, TEXT("publishedDraft"), TEXT("none"), 2, EArguments::None, EDispatch::ReadOnly, false, EEditorFeature::Read},
    {TEXT("draft.loadFile"), nullptr, false, false, TEXT("draftReceipt"), TEXT("replaceDraft"), 2, EArguments::Path, EDispatch::GameThread, true, EEditorFeature::Draft},
    {TEXT("draft.replace"), nullptr, false, false, TEXT("draftReceipt"), TEXT("replaceDraft"), 2, EArguments::Text, EDispatch::GameThread, true, EEditorFeature::Draft},
    {TEXT("draft.restore"), nullptr, false, false, TEXT("draftReceipt"), TEXT("restoreDraft"), 2, EArguments::None, EDispatch::GameThread, true, EEditorFeature::Draft},
    {TEXT("draft.prepareCopy"), TEXT("PrepareNewCopy"), false, false, TEXT("copyPreparationReceipt"), TEXT("prepareInputReportAndReplaceDraftWithoutReservation"), 2, EArguments::None, EDispatch::GameThread, true, EEditorFeature::Graph},
    {TEXT("graphs.preview"), TEXT("GraphPreview"), false, false, TEXT("graphPreviewReceipt"), TEXT("reportAndDisplayOnly"), 2, EArguments::None, EDispatch::GameThread, true, EEditorFeature::Graph},
    {TEXT("graphs.preflight"), TEXT("GraphPreflight"), false, false, TEXT("preflightReceipt"), TEXT("reportOnly"), 2, EArguments::None, EDispatch::GameThread, true, EEditorFeature::Graph},
    {TEXT("graphs.generate"), TEXT("Import"), false, false, TEXT("generationReceipt"), TEXT("createNewAssetAndRequestReceipt"), 2, EArguments::None, EDispatch::GameThread, true, EEditorFeature::Graph},
    {TEXT("rules.query"), TEXT("GraphRules"), false, false, TEXT("rulesReceipt"), TEXT("queryReportAndDisplayOnly"), 2, EArguments::Query, EDispatch::GameThread, true, EEditorFeature::Read},
    {TEXT("catalog.get"), TEXT("GraphCatalog"), false, false, TEXT("catalogReceipt"), TEXT("queryReportAndDisplayOnly"), 2, EArguments::None, EDispatch::GameThread, true, EEditorFeature::Read},
    {TEXT("resources.inspect"), nullptr, false, false, TEXT("resourceReceipt"), TEXT("queryReportAndDisplayOnly"), 2, EArguments::Resource, EDispatch::GameThread, true, EEditorFeature::Graph},
    {TEXT("resources.query"), TEXT("ResourceQuery"), true, false, TEXT("resourceQueryReceipt"), TEXT("metadataQueryReportOnly"), 3, EArguments::ResourceQuery, EDispatch::GameThread, true, EEditorFeature::Read},
    {TEXT("graphs.export"), nullptr, false, false, TEXT("exportReceipt"), TEXT("queryReportAndDisplayOnly"), 2, EArguments::Export, EDispatch::GameThread, true, EEditorFeature::Graph},
    {TEXT("calls.releaseCompleted"), nullptr, false, false, TEXT("callCleanupReceipt"), TEXT("archiveCompletedCallsKeepEvidence"), 2, EArguments::ReleaseCompleted, EDispatch::Management, false, EEditorFeature::Draft},
    {TEXT("calls.get"), nullptr, false, false, TEXT("callRecord"), TEXT("none"), 2, EArguments::CallId, EDispatch::ReadOnly, false, EEditorFeature::Draft},
    {TEXT("control.disable"), nullptr, false, false, TEXT("stoppingRequested"), TEXT("stopChannel"), 2, EArguments::None, EDispatch::Management, false, EEditorFeature::Stop}
};

FString FullPath(FString Path)
{
    Path = FPaths::ConvertRelativePathToFull(Path);
    FPaths::NormalizeFilename(Path);
    FPaths::CollapseRelativeDirectories(Path);
    return Path;
}

bool Under(const FString& Path, const FString& Directory)
{
    return FullPath(Path).StartsWith(FullPath(Directory) + TEXT("/"), ESearchCase::IgnoreCase);
}

int32 Reject(const FString& Operation, const FString& ReportPath, const TCHAR* Code,
    const TCHAR* Field, const FString& Message, bool Environment = false)
{
    const TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("apiVersion"), ApiVersion);
    Report->SetStringField(TEXT("operation"), Operation);
    Report->SetStringField(TEXT("executionState"), Environment ? TEXT("environmentBlocked") : TEXT("rejected"));
    Report->SetStringField(TEXT("overallStatus"), Environment ? TEXT("environmentBlocked") : TEXT("rejected"));
    Report->SetStringField(TEXT("errorCode"), Code);
    Report->SetStringField(TEXT("reason"), Message);
    Report->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
    const TSharedRef<FJsonObject> Error = MakeShared<FJsonObject>();
    Error->SetStringField(TEXT("code"), Code);
    Error->SetStringField(TEXT("field"), Field);
    Error->SetStringField(TEXT("message"), Message);
    Report->SetArrayField(TEXT("errors"), {MakeShared<FJsonValueObject>(Error)});
    // Missing/unwritable report still has a structured console result, never a smoke success.
    FString Text;
    FJsonSerializer::Serialize(Report, TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Text));
    UE_LOG(LogTemp, Display, TEXT("NFCONTROL_RESULT=%s"), *Text);
    const FString Workspace = NodeForgeInstallation::StateDirectory();
    if (!ReportPath.IsEmpty() && !Workspace.IsEmpty() && Under(ReportPath, Workspace / TEXT("Reports")))
        NodeForgePrototype::WriteJson(Report, FullPath(ReportPath));
    return Environment ? 3 : 2;
}

TSharedRef<FJsonObject> Describe()
{
    const TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("apiVersion"), ApiVersion);
    Report->SetStringField(TEXT("operation"), TEXT("operations.describe"));
    Report->SetStringField(TEXT("executionState"), TEXT("finished"));
    Report->SetStringField(TEXT("overallStatus"), TEXT("succeeded"));
    Report->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
    const FString Module = FullPath(FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor")));
    const TSharedRef<FJsonObject> Environment = MakeShared<FJsonObject>();
    Environment->SetStringField(TEXT("platform"), TEXT("Windows"));
    Environment->SetStringField(TEXT("engine"), FEngineVersion::Current().ToString());
    Environment->SetStringField(TEXT("project"), FullPath(FPaths::GetProjectFilePath()));
    Environment->SetStringField(TEXT("module"), Module);
    Environment->SetStringField(TEXT("moduleSha256"), NodeForgePrototype::FileSha256(Module));
    Environment->SetStringField(TEXT("host"), TEXT("isolatedCommandlet"));
    Report->SetObjectField(TEXT("environment"), Environment);
    Report->SetStringField(TEXT("graphProtocol"), TEXT("Contracts/GraphProtocol.md"));
    Report->SetStringField(TEXT("nodeCatalog"), TEXT("Catalog/UE532Graph.json"));
    Report->SetStringField(TEXT("nativeResultContract"), TEXT("Contracts/ResultContractDraft.md"));
    TArray<TSharedPtr<FJsonValue>> DescribedOperations;
    for (const FOperationDefinition& Operation : Definitions())
    {
        if (!Supports(Operation, EHost::Commandlet)) continue;
        const TSharedRef<FJsonObject> Definition = MakeShared<FJsonObject>();
        Definition->SetStringField(TEXT("name"), Operation.Name);
        Definition->SetStringField(TEXT("implementationStatus"), TEXT("implemented"));
        Definition->SetStringField(TEXT("verificationStatus"), TEXT("notRun"));
        Definition->SetStringField(TEXT("verificationEvidence"), TEXT("Reports/20261010PluginControl/Readme.md"));
        Definition->SetStringField(TEXT("handler"), Operation.Mode ? TEXT("NodeForgePrototype::Execute") : TEXT("NodeForgeControlOperations::Describe"));
        Definition->SetStringField(TEXT("nativeMode"), Operation.Mode ? Operation.Mode : TEXT(""));
        Definition->SetStringField(TEXT("resultKind"), Operation.ResultKind);
        Definition->SetStringField(TEXT("sideEffect"), Operation.SideEffect);
        Definition->SetBoolField(TEXT("modifiesExistingAssets"), false);
        Definition->SetStringField(TEXT("requiredContext"), TEXT("exclusiveExplicitlyBoundProjectAndProductStage"));
        TArray<TSharedPtr<FJsonValue>> Parameters;
        if (Operation.InputRequired)
        {
            const TSharedRef<FJsonObject> Parameter = MakeShared<FJsonObject>();
            Parameter->SetStringField(TEXT("name"), TEXT("inputPath"));
            Parameter->SetStringField(TEXT("nativeArgument"), TEXT("NFInput"));
            Parameter->SetStringField(TEXT("type"), TEXT("filePath"));
            Parameter->SetBoolField(TEXT("required"), true);
            Parameters.Add(MakeShared<FJsonValueObject>(Parameter));
        }
        if (Operation.PreviousRequired)
        {
            const TSharedRef<FJsonObject> Parameter = MakeShared<FJsonObject>();
            Parameter->SetStringField(TEXT("name"), TEXT("generationReport"));
            Parameter->SetStringField(TEXT("nativeArgument"), TEXT("NFPrevious"));
            Parameter->SetStringField(TEXT("type"), TEXT("filePath"));
            Parameter->SetBoolField(TEXT("required"), true);
            Parameters.Add(MakeShared<FJsonValueObject>(Parameter));
        }
        Definition->SetArrayField(TEXT("parameters"), Parameters);
        DescribedOperations.Add(MakeShared<FJsonValueObject>(Definition));
    }
    Report->SetArrayField(TEXT("operations"), DescribedOperations);
    Report->SetStringField(TEXT("verificationNote"), TEXT("Implementation description is not test proof; consult stage-bound delivery evidence."));
    return Report;
}
}

TConstArrayView<FOperationDefinition> Definitions() { return MakeArrayView(Operations); }
bool Supports(const FOperationDefinition& Definition, EHost Host)
{ return (Definition.Hosts & static_cast<uint8>(Host)) != 0; }
const FOperationDefinition* Find(const FString& Name, EHost Host)
{
    for (const auto& Definition : Definitions())
        if (Supports(Definition, Host) && Name == Definition.Name) return &Definition;
    return nullptr;
}
TSharedRef<FJsonObject> ArgumentsSchema(const FOperationDefinition& Definition)
{
    auto Arguments = MakeShared<FJsonObject>();
    Arguments->SetStringField(TEXT("type"), TEXT("object"));
    Arguments->SetBoolField(TEXT("additionalProperties"), false);
    auto Properties = MakeShared<FJsonObject>(); TArray<TSharedPtr<FJsonValue>> Required;
    auto Parameter = [&](const TCHAR* Name, const TCHAR* Type, bool IsRequired = true)
    {
        auto Property = MakeShared<FJsonObject>(); Property->SetStringField(TEXT("type"), Type);
        Properties->SetObjectField(Name, Property); if (IsRequired) Required.Add(MakeShared<FJsonValueString>(Name));
    };
    switch (Definition.Arguments)
    {
    case EArguments::Path:
    case EArguments::Text:
        Parameter(Definition.Arguments == EArguments::Path ? TEXT("path") : TEXT("text"), TEXT("string"));
        Parameter(TEXT("exportReportPath"), TEXT("string"), false);
        Parameter(TEXT("exportReportSha256"), TEXT("string"), false);
        if (Definition.Arguments == EArguments::Text) Parameter(TEXT("retainExportOrigin"), TEXT("boolean"), false);
        break;
    case EArguments::Resource:
        Parameter(TEXT("kind"), TEXT("string")); Parameter(TEXT("source"), TEXT("string"));
        Properties->GetObjectField(TEXT("kind"))->SetArrayField(TEXT("enum"),
            {MakeShared<FJsonValueString>(TEXT("MaterialParameters")), MakeShared<FJsonValueString>(TEXT("MaterialFunction")), MakeShared<FJsonValueString>(TEXT("Texture2D"))});
        break;
    case EArguments::ResourceQuery:
        return NodeForgeResourceSource::QueryArgumentsSchema();
    case EArguments::Export:
        for (const TCHAR* Name : {TEXT("assetKind"), TEXT("source"), TEXT("requestId"), TEXT("assetPath")}) Parameter(Name, TEXT("string"));
        Parameter(TEXT("formatVersion"),TEXT("integer"),false);
        Properties->GetObjectField(TEXT("formatVersion"))->SetArrayField(TEXT("enum"),{MakeShared<FJsonValueNumber>(2),MakeShared<FJsonValueNumber>(3)});
        Properties->GetObjectField(TEXT("formatVersion"))->SetNumberField(TEXT("default"),2);
        Properties->GetObjectField(TEXT("assetKind"))->SetArrayField(TEXT("enum"),
            {MakeShared<FJsonValueString>(TEXT("Material")), MakeShared<FJsonValueString>(TEXT("Blueprint"))});
        break;
    case EArguments::ReleaseCompleted:
        Parameter(TEXT("maxCount"), TEXT("integer"));
        Properties->GetObjectField(TEXT("maxCount"))->SetNumberField(TEXT("minimum"), 1);
        Properties->GetObjectField(TEXT("maxCount"))->SetNumberField(TEXT("maximum"), 128);
        break;
    case EArguments::CallId: Parameter(TEXT("callId"), TEXT("string")); break;
    case EArguments::Query:
        Parameter(TEXT("assetKind"), TEXT("string")); Parameter(TEXT("query"), TEXT("string"));
        Properties->GetObjectField(TEXT("assetKind"))->SetArrayField(TEXT("enum"),
            {MakeShared<FJsonValueString>(TEXT("Material")), MakeShared<FJsonValueString>(TEXT("Blueprint"))});
        Properties->GetObjectField(TEXT("query"))->SetNumberField(TEXT("maxLength"), 256);
        Properties->GetObjectField(TEXT("query"))->SetNumberField(TEXT("x-maxWhitespaceTerms"), 8);
        break;
    case EArguments::Hello:
        for (const TCHAR* Name : {TEXT("editorApiVersion"), TEXT("profile"), TEXT("controlSessionId"),
            TEXT("expectedProject"), TEXT("expectedProcessStart"), TEXT("expectedBinarySha256"), TEXT("credential")})
            Parameter(Name, TEXT("string"));
        Parameter(TEXT("expectedPid"), TEXT("integer"));
        Parameter(TEXT("clientId"), TEXT("string"), false);
        Parameter(TEXT("clientName"), TEXT("string"), false);
        Parameter(TEXT("clientVersion"), TEXT("string"), false);
        Properties->GetObjectField(TEXT("clientId"))->SetStringField(TEXT("pattern"),TEXT("^[0-9a-f]{32}$"));
        Properties->GetObjectField(TEXT("clientName"))->SetNumberField(TEXT("minLength"),1);
        Properties->GetObjectField(TEXT("clientName"))->SetNumberField(TEXT("maxLength"),80);
        Properties->GetObjectField(TEXT("clientVersion"))->SetNumberField(TEXT("maxLength"),80);
        break;
    default: break;
    }
    Arguments->SetObjectField(TEXT("properties"), Properties); Arguments->SetArrayField(TEXT("required"), Required);
    if (Definition.Arguments == EArguments::Path || Definition.Arguments == EArguments::Text)
    {
        auto Dependencies = MakeShared<FJsonObject>();
        Dependencies->SetArrayField(TEXT("exportReportPath"), {MakeShared<FJsonValueString>(TEXT("exportReportSha256"))});
        Dependencies->SetArrayField(TEXT("exportReportSha256"), {MakeShared<FJsonValueString>(TEXT("exportReportPath"))});
        Arguments->SetObjectField(TEXT("dependentRequired"), Dependencies);
        Properties->GetObjectField(TEXT("exportReportSha256"))->SetStringField(TEXT("pattern"), TEXT("^[0-9a-f]{64}$"));
    }
    if (Definition.Arguments == EArguments::Text)
    {
        auto ConstTrue = MakeShared<FJsonObject>(); ConstTrue->SetBoolField(TEXT("const"), true);
        auto IfProperties = MakeShared<FJsonObject>(); IfProperties->SetObjectField(TEXT("retainExportOrigin"), ConstTrue);
        auto If = MakeShared<FJsonObject>(); If->SetObjectField(TEXT("properties"), IfProperties);
        If->SetArrayField(TEXT("required"), {MakeShared<FJsonValueString>(TEXT("retainExportOrigin"))});
        TArray<TSharedPtr<FJsonValue>> Forbidden;
        for (const TCHAR* Field : {TEXT("exportReportPath"), TEXT("exportReportSha256")})
        { auto Present = MakeShared<FJsonObject>(); Present->SetArrayField(TEXT("required"), {MakeShared<FJsonValueString>(Field)}); Forbidden.Add(MakeShared<FJsonValueObject>(Present)); }
        auto Any = MakeShared<FJsonObject>(); Any->SetArrayField(TEXT("anyOf"), Forbidden);
        auto Then = MakeShared<FJsonObject>(); Then->SetObjectField(TEXT("not"), Any);
        auto Rule = MakeShared<FJsonObject>(); Rule->SetObjectField(TEXT("if"), If); Rule->SetObjectField(TEXT("then"), Then);
        Arguments->SetArrayField(TEXT("allOf"), {MakeShared<FJsonValueObject>(Rule)});
    }
    return Arguments;
}
TSharedRef<FJsonObject> DescribeEditor(uint8 EnabledFeatures)
{
    auto Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("host"), TEXT("activeEditor"));
    Report->SetStringField(TEXT("editorApiVersion"), TEXT("1.0"));
    Report->SetStringField(TEXT("profile"), TEXT("nodeforge.editor.v1"));
    TArray<TSharedPtr<FJsonValue>> Rows;
    for (const auto& Definition : Definitions())
    {
        if (!Supports(Definition, EHost::ActiveEditor)
            || !(EnabledFeatures & static_cast<uint8>(Definition.EditorFeature))) continue;
        auto Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("name"), Definition.Name);
        Row->SetStringField(TEXT("implementationStatus"), TEXT("implemented"));
        Row->SetStringField(TEXT("resultKind"), Definition.ResultKind);
        Row->SetStringField(TEXT("sideEffect"), Definition.SideEffect);
        Row->SetBoolField(TEXT("expectedDraftRevisionRequired"), Definition.RevisionRequired);
        Row->SetStringField(TEXT("dispatch"), Definition.Dispatch == EDispatch::GameThread ? TEXT("gameThread")
            : Definition.Dispatch == EDispatch::Handshake ? TEXT("handshake")
            : Definition.Dispatch == EDispatch::Management ? TEXT("management") : TEXT("readOnlyCache"));
        Row->SetObjectField(TEXT("argumentsSchema"), ArgumentsSchema(Definition));
        Rows.Add(MakeShared<FJsonValueObject>(Row));
    }
    Report->SetArrayField(TEXT("operations"), Rows);
    return Report;
}

bool IsFormalCall(const FString& Params)
{
    const TCHAR* Cursor = *Params;
    FString Token;
    while (FParse::Token(Cursor, Token, false))
        if (Token.Equals(TEXT("-NFOperation"), ESearchCase::IgnoreCase)
            || Token.StartsWith(TEXT("-NFOperation="), ESearchCase::IgnoreCase)
            || Token.StartsWith(TEXT("-NFControlApiVersion"), ESearchCase::IgnoreCase)) return true;
    return false;
}

int32 Execute(const FString& Params)
{
    // Parse each formal argument in its own token: FParse::Value on the whole
    // command line consumes the next switch when an explicit value is empty.
    TMap<FString, FString> Fields;
    FString ParseError;
    const TCHAR* Cursor = *Params;
    FString Token;
    while (FParse::Token(Cursor, Token, false))
    {
        FString Key, Value;
        if (!Token.Split(TEXT("="), &Key, &Value)) Key = Token;
        Key = Key.ToLower();
        if (!Key.StartsWith(TEXT("-nf"))) continue;
        if (Key != TEXT("-nfoperation") && Key != TEXT("-nfreport") && Key != TEXT("-nfinput")
            && Key != TEXT("-nfprevious") && Key != TEXT("-nfcontrolapiversion") && Key != TEXT("-nfmode"))
            ParseError = TEXT("Unknown formal argument: ") + Key;
        if (Fields.Contains(Key)) ParseError = TEXT("Duplicate formal argument: ") + Key;
        if (Value.Len() >= 2 && Value.StartsWith(TEXT("\"")) && Value.EndsWith(TEXT("\"")))
            Value = Value.Mid(1, Value.Len() - 2);
        Fields.Add(Key, Value);
    }
    const FString Name = Fields.FindRef(TEXT("-nfoperation"));
    const FString ReportPath = Fields.FindRef(TEXT("-nfreport"));
    const FString InputPath = Fields.FindRef(TEXT("-nfinput"));
    const FString Previous = Fields.FindRef(TEXT("-nfprevious"));
    const FString Version = Fields.FindRef(TEXT("-nfcontrolapiversion"));
    if (!ParseError.IsEmpty()) return Reject(Name, ReportPath, TEXT("NFCTRL_ARGUMENT"), TEXT("arguments"), ParseError);
    if (Name.IsEmpty()) return Reject(Name, ReportPath, TEXT("NFCTRL_MISSING_OPERATION"), TEXT("operation"), TEXT("NFOperation must name an explicit operation"));
    if (!Version.IsEmpty() && Version != ApiVersion)
        return Reject(Name, ReportPath, TEXT("NFCTRL_API_VERSION"), TEXT("apiVersion"), TEXT("Only Control API 1.0 is supported"));
    if (Fields.Contains(TEXT("-nfmode")))
        return Reject(Name, ReportPath, TEXT("NFCTRL_AMBIGUOUS_MODE"), TEXT("NFMode"), TEXT("Formal operation cannot be combined with legacy NFMode"));
    const FOperationDefinition* Selected = nullptr;
    Selected = Find(Name, EHost::Commandlet);
    if (!Selected) return Reject(Name, ReportPath, TEXT("NFCTRL_UNKNOWN_OPERATION"), TEXT("operation"), TEXT("Operation is not implemented by this DLL"));
    if (ReportPath.IsEmpty()) return Reject(Name, ReportPath, TEXT("NFCTRL_MISSING_REPORT"), TEXT("reportPath"), TEXT("NFReport is required"));
    NodeForgeInstallation::FBinding Binding; FString BindingError;
    if (!NodeForgeInstallation::Read(Binding,BindingError))
        return Reject(Name, FString(), TEXT("NFCTRL_ENVIRONMENT"), TEXT("project/stage"), BindingError, true);
    const FString Workspace = NodeForgeInstallation::StateDirectory(Binding);
    if (Workspace.IsEmpty()) return Reject(Name, FString(), TEXT("NFCTRL_ENVIRONMENT"), TEXT("stateRoot"), TEXT("Writable project state path rejected"), true);
    if (!Under(ReportPath, Workspace / TEXT("Reports")))
        return Reject(Name, ReportPath, TEXT("NFCTRL_REPORT_PATH"), TEXT("reportPath"), TEXT("Report must be an exclusive new file inside workspace Reports"), true);
    if (IFileManager::Get().FileExists(*FullPath(ReportPath)))
        return Reject(Name, FString(), TEXT("NFCTRL_REPORT_EXISTS"), TEXT("reportPath"), TEXT("Existing report protected; inspect prior run instead of overwriting"));
    if (Selected->InputRequired && InputPath.IsEmpty())
        return Reject(Name, ReportPath, TEXT("NFCTRL_MISSING_INPUT"), TEXT("inputPath"), TEXT("This operation requires NFInput"));
    if (!InputPath.IsEmpty() && !Under(InputPath, Workspace / TEXT("Reports"))
        && !Under(InputPath, Binding.InstallationRoot / TEXT("Examples"))
        && !Under(InputPath, Binding.InstallationRoot / TEXT("Tests")))
        return Reject(Name, ReportPath, TEXT("NFCTRL_INPUT_PATH"), TEXT("inputPath"), TEXT("Input must belong to this workspace"));
    if (Selected->PreviousRequired && Previous.IsEmpty())
        return Reject(Name, ReportPath, TEXT("NFCTRL_MISSING_PREVIOUS"), TEXT("generationReport"), TEXT("Verify requires the original generation report"));
    if (!Previous.IsEmpty() && !Under(Previous, Workspace / TEXT("Reports")))
        return Reject(Name, ReportPath, TEXT("NFCTRL_PREVIOUS_PATH"), TEXT("generationReport"), TEXT("Generation report must belong to workspace Reports"));
    if (!Selected->Mode)
        return NodeForgePrototype::WriteJson(Describe(), FullPath(ReportPath)) ? 0
            : Reject(Name, FString(), TEXT("NFCTRL_REPORT_WRITE"), TEXT("reportPath"), TEXT("Could not publish fresh capability report"), true);
    return NodeForgePrototype::Execute(Selected->Mode, InputPath, FullPath(ReportPath), Previous);
}
}
