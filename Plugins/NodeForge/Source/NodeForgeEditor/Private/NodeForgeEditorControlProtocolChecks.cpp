#include "NodeForgeEditorControlProtocol.h"
#include "NodeForgePrototype.h"
#include "NodeForgeEditorControlService.h"
#include "NodeForgeResourceSource.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeEditorControlProtocolTest, "NodeForge.EditorControlProtocol",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNodeForgeEditorControlProtocolTest::RunTest(const FString& Parameters)
{
    using namespace NodeForgeEditorControl;
    using namespace NodeForgeControlOperations;
    FString Output;
    if (!FParse::Value(FCommandLine::Get(), TEXT("NFEditorProtocolReport="), Output)
        || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Output), NodeForgePrototype::WorkspaceDirectory() / TEXT("Reports"))
        || IFileManager::Get().FileExists(*Output))
    { AddError(TEXT("Fresh Reports NFEditorProtocolReport required")); return false; }
    bool All = true; TArray<TSharedPtr<FJsonValue>> Rows;
    auto Row = [&](const TCHAR* Name, bool Passed)
    {
        All &= Passed; TestTrue(Name, Passed); auto Json = MakeShared<FJsonObject>();
        Json->SetStringField(TEXT("case"), Name); Json->SetBoolField(TEXT("passed"), Passed);
        Rows.Add(MakeShared<FJsonValueObject>(Json));
    };
    auto Bytes = [](const FString& Text)
    { FTCHARToUTF8 Utf8(*Text); TArray<uint8> Value; Value.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length()); return Value; };
    auto Wire = [](uint32 Length)
    { TArray<uint8> Data; for (int32 I = 0; I < 4; ++I) Data.Add((Length >> (I * 8)) & 0xff); return Data; };
    auto Full = Wire(2); Full.Append(Bytes(TEXT("{}")));
    bool SplitPassed = true;
    for (int32 Split = 1; Split < Full.Num(); ++Split)
    {
        FFrameReader Frame(10);
        SplitPassed &= Frame.Feed(MakeArrayView(Full.GetData(), Split), 0) == EFrameState::NeedMore;
        if (Split < 4) SplitPassed &= Frame.AllocatedBodyBytes() == 0;
        SplitPassed &= Frame.Feed(MakeArrayView(Full.GetData() + Split, Full.Num() - Split), 9) == EFrameState::Complete;
        SplitPassed &= Frame.Body() == Bytes(TEXT("{}"));
    }
    Row(TEXT("AllPartialHeaderAndBodySplitsRequireCompleteFrame"), SplitPassed);
    bool Lengths = true;
    for (uint32 Length : {0u, MaxFrameBytes + 1u, MAX_uint32})
    { FFrameReader Frame(10); Lengths &= Frame.Feed(Wire(Length), 0) == EFrameState::Rejected && Frame.AllocatedBodyBytes() == 0; }
    Row(TEXT("InvalidLengthRejectedBeforeBodyAllocation"), Lengths);
    FFrameReader MaxFrame(10);
    Row(TEXT("MaximumLegalLengthBoundedWithoutDispatch"), MaxFrame.Feed(Wire(MaxFrameBytes), 0) == EFrameState::NeedMore
        && MaxFrame.AllocatedBodyBytes() == static_cast<int32>(MaxFrameBytes) && MaxFrame.RemainingBytes() == static_cast<int32>(MaxFrameBytes));
    FFrameReader Deadline(10);
    Row(TEXT("PartialProgressNeverRenewsAbsoluteDeadline"), Deadline.Feed(MakeArrayView(Full.GetData(), 2), 0) == EFrameState::NeedMore
        && Deadline.Feed(MakeArrayView(Full.GetData() + 2, 2), 9.99) == EFrameState::NeedMore
        && Deadline.CheckDeadline(10) == EFrameState::Rejected && Deadline.Error() == TEXT("frameTimeout"));
    FFrameReader Trailing(10); auto Two = Full; Two.Append(Full);
    Row(TEXT("CoalescedExtraRequestCannotEnterSameFrame"), Trailing.Feed(Two, 0) == EFrameState::Rejected);
    auto Context = [](bool Revision)
    {
        auto Json = MakeShared<FJsonObject>(); Json->SetStringField(TEXT("controlSessionId"), FString::ChrN(32, 'a'));
        Json->SetStringField(TEXT("editorApiVersion"), NodeForgeEditorControl::ApiVersion); Json->SetStringField(TEXT("profile"), Profile);
        if (Revision) Json->SetStringField(TEXT("expectedDraftRevision"), TEXT("0")); return Json;
    };
    auto Rpc = [&](const TCHAR* Method, const TSharedRef<FJsonObject>& Args, bool Revision = false)
    {
        auto Json = MakeShared<FJsonObject>(); Json->SetStringField(TEXT("jsonrpc"), TEXT("2.0"));
        Json->SetStringField(TEXT("id"), TEXT("Call-1")); Json->SetStringField(TEXT("method"), Method);
        auto Params = MakeShared<FJsonObject>(); Params->SetObjectField(TEXT("context"), Context(Revision)); Params->SetObjectField(TEXT("arguments"), Args);
        Json->SetObjectField(TEXT("params"), Params); return Json;
    };
    auto JsonBytes = [&](const TSharedRef<FJsonObject>& Json)
    { FString Text; FJsonSerializer::Serialize(Json, TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Text)); return Bytes(Text); };
    constexpr uint8 Read = static_cast<uint8>(EEditorFeature::Read);
    constexpr uint8 AllFeatures = 15;
    FRequest Request; FProtocolError Error;
    auto Empty = MakeShared<FJsonObject>();
    const auto State = Rpc(TEXT("editor.getState"), Empty);
    Row(TEXT("StrictReadRequestMatchesSingleTable"), ParseRequest(JsonBytes(State), Read, Request, Error)
        && Request.Definition == Find(TEXT("editor.getState"), EHost::ActiveEditor));
    const FString FirstFingerprint = Fingerprint(Request);
    const FString Reordered = TEXT("{\"params\":{\"arguments\":{},\"context\":{\"profile\":\"nodeforge.editor.v1\",\"editorApiVersion\":\"1.0\",\"controlSessionId\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"}},\"method\":\"editor.getState\",\"id\":\"Call-1\",\"jsonrpc\":\"2.0\"}");
    Row(TEXT("DecodedFingerprintIndependentOfObjectOrder"), ParseRequest(Bytes(Reordered), Read, Request, Error)
        && FirstFingerprint.Len() == 64 && Fingerprint(Request) == FirstFingerprint);
    auto Rejected = [&](const TCHAR* Name, const TArray<uint8>& Input, int32 ExpectedCode)
    { const bool Accepted = ParseRequest(Input, AllFeatures, Request, Error); Row(Name, !Accepted && Error.RpcCode == ExpectedCode); };
    TArray<uint8> Bom = {0xef,0xbb,0xbf}; Bom.Append(JsonBytes(State)); Rejected(TEXT("Utf8BomRejected"), Bom, -32700);
    for (const TArray<uint8>& Invalid : {TArray<uint8>{0xc0,0x80}, TArray<uint8>{0xed,0xa0,0x80}, TArray<uint8>{0xf4,0x90,0x80,0x80}, TArray<uint8>{0xe2,0x82}, TArray<uint8>{0}})
        Rejected(TEXT("InvalidUtf8ScalarRejected"), Invalid, -32700);
    Rejected(TEXT("DecodedDuplicateKeysRejectedBeforeDom"), Bytes(TEXT("{\"jsonrpc\":\"2.0\",\"json\\u0072pc\":\"2.0\"}")), -32700);
    Rejected(TEXT("EscapedDuplicateKeysRejectedBeforeDom"), Bytes(TEXT("{\"x\":1,\"\\u0078\":2}")), -32700);
    Rejected(TEXT("NestedDecodedDuplicateKeysRejected"), Bytes(TEXT("{\"outer\":{\"x\":1,\"\\u0078\":2}}")), -32700);
    Rejected(TEXT("UnpairedHighSurrogateRejected"), Bytes(TEXT("{\"x\":\"\\ud800\"}")), -32700);
    Rejected(TEXT("UnpairedLowSurrogateRejected"), Bytes(TEXT("{\"x\":\"\\udc00\"}")), -32700);
    Rejected(TEXT("EscapedNulRejected"), Bytes(TEXT("{\"x\":\"\\u0000\"}")), -32700);
    Rejected(TEXT("NonFiniteNumberRejected"), Bytes(TEXT("{\"x\":1e9999}")), -32700);
    Rejected(TEXT("TrailingSecondJsonRejected"), Bytes(TEXT("{}{}")), -32700);
    Rejected(TEXT("MalformedJsonRejected"), Bytes(TEXT("{\"x\":")), -32700);
    FString Nested = TEXT("0"); for (int32 I = 0; I < 33; ++I) Nested = TEXT("{\"k\":") + Nested + TEXT("}");
    Rejected(TEXT("Depth33RejectedBeforeSchema"), Bytes(Nested), -32700);
    Nested = TEXT("0"); for (int32 I = 0; I < 32; ++I) Nested = TEXT("{\"k\":") + Nested + TEXT("}");
    Rejected(TEXT("Depth32ParsedThenRequestRejected"), Bytes(Nested), -32600);
    Rejected(TEXT("BatchCannotExecuteChildren"), Bytes(TEXT("[{\"id\":\"a\",\"method\":\"graphs.generate\"}]")), -32600);
    Rejected(TEXT("ValidScalarIsInvalidRequest"), Bytes(TEXT("true")), -32600);
    State->SetStringField(TEXT("unexpected"), TEXT("x")); Rejected(TEXT("UnknownEnvelopeFieldRejected"), JsonBytes(State), -32600); State->RemoveField(TEXT("unexpected"));
    State->RemoveField(TEXT("id"));
    Row(TEXT("NotificationNeverRepliesOrDispatches"), !ParseRequest(JsonBytes(State), AllFeatures, Request, Error) && !Error.bReply && Error.bClose);
    State->SetNumberField(TEXT("id"), 1); Rejected(TEXT("ProfileRejectsNumericId"), JsonBytes(State), -32600); State->SetStringField(TEXT("id"), TEXT("Call-1"));
    State->GetObjectField(TEXT("params"))->GetObjectField(TEXT("context"))->SetStringField(TEXT("extra"), TEXT("x"));
    Rejected(TEXT("UnknownContextFieldRejected"), JsonBytes(State), -32602);
    auto ReplaceArgs = MakeShared<FJsonObject>(); ReplaceArgs->SetStringField(TEXT("text"), TEXT("{\"public\":\"中文😀\"}"));
    auto Replace = Rpc(TEXT("draft.replace"), ReplaceArgs, true);
    Row(TEXT("ExternalDraftUsesExistingTextValidator"), ParseRequest(JsonBytes(Replace), AllFeatures, Request, Error) && Request.ExpectedRevision == 0);
    const FString ExactTextFingerprint = Fingerprint(Request);
    ReplaceArgs->SetStringField(TEXT("text"), TEXT("{\"public\": \"中文😀\"}"));
    Row(TEXT("FingerprintPreservesExactDraftText"), ParseRequest(JsonBytes(Replace), AllFeatures, Request, Error) && Fingerprint(Request) != ExactTextFingerprint);
    ReplaceArgs->SetStringField(TEXT("unknown"), TEXT("x")); Rejected(TEXT("SchemaRejectsUnknownArguments"), JsonBytes(Replace), -32602); ReplaceArgs->RemoveField(TEXT("unknown"));
    ReplaceArgs->SetStringField(TEXT("text"), TEXT(" \n ")); Rejected(TEXT("BlankExternalDraftRejected"), JsonBytes(Replace), -32602);
    ReplaceArgs->SetStringField(TEXT("text"), FString::ChrN(65537, 'x')); Rejected(TEXT("OversizeDraftNeverTruncated"), JsonBytes(Replace), -32602);
    ReplaceArgs->SetStringField(TEXT("text"), TEXT("{}"));
    Replace->GetObjectField(TEXT("params"))->GetObjectField(TEXT("context"))->SetStringField(TEXT("expectedDraftRevision"), TEXT("18446744073709551615"));
    Row(TEXT("MaximumUint64RevisionExact"), ParseRequest(JsonBytes(Replace), AllFeatures, Request, Error) && Request.ExpectedRevision == MAX_uint64);
    Replace->GetObjectField(TEXT("params"))->GetObjectField(TEXT("context"))->SetStringField(TEXT("expectedDraftRevision"), TEXT("18446744073709551616"));
    Rejected(TEXT("Uint64OverflowRejected"), JsonBytes(Replace), -32602);
    uint64 Value = 10; Row(TEXT("DecimalRevisionCanonical"), !DecimalUint64(TEXT("01"), Value) && !DecimalUint64(TEXT("+1"), Value) && !DecimalUint64(TEXT("-1"), Value));
    auto Generate = Rpc(TEXT("graphs.generate"), Empty, true);
    Row(TEXT("UnimplementedGraphFeatureUnavailable"), !ParseRequest(JsonBytes(Generate), Read, Request, Error) && Error.RpcCode == -32601);
    auto Unknown = Rpc(TEXT("console.execute"), Empty); Rejected(TEXT("ArbitraryMethodUnavailable"), JsonBytes(Unknown), -32601);
    int32 CommandletCount = 0, EditorCount = 0;
    for (const auto& Definition : Definitions()) { if (Supports(Definition, EHost::Commandlet)) ++CommandletCount; if (Supports(Definition, EHost::ActiveEditor)) ++EditorCount; }
    const auto Description = DescribeEditor(Read);
    bool NoGraph = true; for (const auto& Definition : Description->GetArrayField(TEXT("operations")))
        NoGraph &= !Definition->AsObject()->GetStringField(TEXT("name")).StartsWith(TEXT("graphs."));
    Row(TEXT("OneTableAddsOnlyResourceQueryToBothHostsAndReadProjection"), CommandletCount == 5 && EditorCount == 18
        && Description->GetArrayField(TEXT("operations")).Num() == 7 && NoGraph && !Find(TEXT("draft.replace"), EHost::Commandlet)
        && !Find(TEXT("rules.query"), EHost::Commandlet) && !Find(TEXT("catalog.get"), EHost::Commandlet));
    auto QueryArgs = MakeShared<FJsonObject>();
    QueryArgs->SetStringField(TEXT("kind"), TEXT("Texture2D"));
    QueryArgs->SetStringField(TEXT("directory"), TEXT("/Engine/EngineResources"));
    auto QueryRequest = Rpc(TEXT("resources.query"), QueryArgs, true);
    TSharedPtr<FJsonObject> Effective; FString QueryError;
    Row(TEXT("ResourceQuerySchemaAndCoreAgreeOnDefaults"), ParseRequest(JsonBytes(QueryRequest), Read, Request, Error)
        && NodeForgeResourceSource::EffectiveQueryArguments(QueryArgs, Effective, QueryError)
        && Effective->GetNumberField(TEXT("limit")) == 50 && !Effective->GetBoolField(TEXT("recursive"))
        && Effective->GetStringField(TEXT("after")).IsEmpty() && Effective->GetStringField(TEXT("nameContains")).IsEmpty());
    for (const double Limit : {0.0, 201.0, 1.5})
    {
        QueryArgs->SetNumberField(TEXT("limit"), Limit);
        Row(TEXT("ResourceQuerySchemaAndCoreRejectInvalidLimit"), !ParseRequest(JsonBytes(QueryRequest), Read, Request, Error)
            && !NodeForgeResourceSource::EffectiveQueryArguments(QueryArgs, Effective, QueryError));
    }
    QueryArgs->RemoveField(TEXT("limit")); QueryArgs->SetStringField(TEXT("kind"), TEXT("Texture"));
    Row(TEXT("ResourceQueryRequiresExactKind"), !ParseRequest(JsonBytes(QueryRequest), Read, Request, Error)
        && !NodeForgeResourceSource::EffectiveQueryArguments(QueryArgs, Effective, QueryError));
    QueryArgs->SetStringField(TEXT("kind"), TEXT("Texture2D")); QueryArgs->SetBoolField(TEXT("loadObjects"), true);
    Row(TEXT("ResourceQueryRejectsUnadvertisedLoadOption"), !ParseRequest(JsonBytes(QueryRequest), Read, Request, Error)
        && !NodeForgeResourceSource::EffectiveQueryArguments(QueryArgs, Effective, QueryError));
    auto Hello = MakeShared<FJsonObject>(); Hello->SetStringField(TEXT("jsonrpc"), TEXT("2.0")); Hello->SetStringField(TEXT("id"), TEXT("Hello-1")); Hello->SetStringField(TEXT("method"), TEXT("session.hello"));
    auto HelloArgs = MakeShared<FJsonObject>();
    HelloArgs->SetStringField(TEXT("editorApiVersion"), NodeForgeEditorControl::ApiVersion); HelloArgs->SetStringField(TEXT("profile"), Profile);
    HelloArgs->SetStringField(TEXT("controlSessionId"), FString::ChrN(32, 'a')); HelloArgs->SetStringField(TEXT("expectedProject"), TEXT("C:/NodeForgeExample/ValidationProject/RegressionRuns/Example/Example.uproject"));
    HelloArgs->SetNumberField(TEXT("expectedPid"), 1); HelloArgs->SetStringField(TEXT("expectedProcessStart"), TEXT("134000000000000000"));
    HelloArgs->SetStringField(TEXT("expectedBinarySha256"), FString::ChrN(64, 'b')); HelloArgs->SetStringField(TEXT("credential"), FString::ChrN(64, 'c')); Hello->SetObjectField(TEXT("params"), HelloArgs);
    Row(TEXT("HelloSchemaAcceptedWithoutCredentialFingerprint"), ParseRequest(JsonBytes(Hello), Read, Request, Error) && Fingerprint(Request).IsEmpty());
    HelloArgs->SetStringField(TEXT("credential"), TEXT("invalid-public-fixture"));
    Row(TEXT("RejectedHelloDoesNotReflectCredentialOrPath"), !ParseRequest(JsonBytes(Hello), Read, Request, Error)
        && !Failure(Error)->GetObjectField(TEXT("error"))->GetStringField(TEXT("message")).Contains(TEXT("invalid-public-fixture")));
    TArray<uint8> Encoded; const auto Ok = Result(TEXT("Call-1"), Empty); const auto Failed = Failure(Error);
    Row(TEXT("ResponseResultErrorExclusiveAndFramed"), Ok->HasField(TEXT("result")) && !Ok->HasField(TEXT("error"))
        && Failed->HasField(TEXT("error")) && !Failed->HasField(TEXT("result")) && EncodeFrame(Ok, Encoded));
    FFrameReader ResponseFrame(10);
    Row(TEXT("EncodedResponseLengthMatchesPayload"), ResponseFrame.Feed(Encoded, 0) == EFrameState::Complete
        && ResponseFrame.Body().Num() == Encoded.Num() - 4);
    auto Report = MakeShared<FJsonObject>(); Report->SetStringField(TEXT("operationStatus"), All ? TEXT("passed") : TEXT("failed"));
    Report->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
    const FString Module = FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"));
    Report->SetStringField(TEXT("module"), Module); Report->SetStringField(TEXT("moduleSha256"), NodeForgePrototype::FileSha256(Module));
    Report->SetStringField(TEXT("scope"), TEXT("Native framing/envelope/schema mechanism only; real pipe/ACL/identity/lifecycle notRun"));
    Report->SetObjectField(TEXT("limits"), Limits()); Report->SetArrayField(TEXT("checks"), Rows);
    return NodeForgePrototype::WriteJson(Report, Output) && All;
}
// C-only native cases: do not rerun the framing/security suite or generate fixture assets.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeCopyPreviewContractTest, "NodeForge.CopyPreviewContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNodeForgeCopyPreviewContractTest::RunTest(const FString& Parameters)
{
    using namespace NodeForgeEditorControl;
    using namespace NodeForgeControlOperations;
    FString Output;
    if (!FParse::Value(FCommandLine::Get(), TEXT("NFCopyPreviewReport="), Output)
        || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Output), NodeForgePrototype::WorkspaceDirectory() / TEXT("Reports"))
        || IFileManager::Get().FileExists(*Output))
    { AddError(TEXT("Fresh Reports NFCopyPreviewReport required")); return false; }
    bool All = true; TArray<TSharedPtr<FJsonValue>> Rows;
    auto Row = [&](const TCHAR* Name, bool Passed)
    {
        All &= Passed; TestTrue(Name, Passed); auto Json = MakeShared<FJsonObject>();
        Json->SetStringField(TEXT("case"), Name); Json->SetBoolField(TEXT("passed"), Passed);
        Rows.Add(MakeShared<FJsonValueObject>(Json));
    };
    auto Rpc = [](const TCHAR* Method, const TSharedRef<FJsonObject>& Args, bool Revision = true)
    {
        auto Request = MakeShared<FJsonObject>(); Request->SetStringField(TEXT("jsonrpc"), TEXT("2.0"));
        Request->SetStringField(TEXT("id"), TEXT("CopyContract1")); Request->SetStringField(TEXT("method"), Method);
        auto Context = MakeShared<FJsonObject>(); Context->SetStringField(TEXT("controlSessionId"), FString::ChrN(32, 'a'));
        Context->SetStringField(TEXT("editorApiVersion"), ApiVersion); Context->SetStringField(TEXT("profile"), Profile);
        if (Revision) Context->SetStringField(TEXT("expectedDraftRevision"), TEXT("0"));
        auto Params = MakeShared<FJsonObject>(); Params->SetObjectField(TEXT("context"), Context); Params->SetObjectField(TEXT("arguments"), Args);
        Request->SetObjectField(TEXT("params"), Params); return Request;
    };
    auto Accepted = [&](const TSharedRef<FJsonObject>& Json)
    {
        FString Text; FJsonSerializer::Serialize(Json, TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Text));
        FTCHARToUTF8 Utf8(*Text); FRequest Request; FProtocolError Error;
        return ParseRequest(MakeArrayView(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length()), 15, Request, Error);
    };
    auto Args = MakeShared<FJsonObject>(); Args->SetStringField(TEXT("text"), TEXT("{}"));
    Row(TEXT("RetainOmittedKeepsOriginalReplaceSchema"), Accepted(Rpc(TEXT("draft.replace"), Args)));
    Args->SetBoolField(TEXT("retainExportOrigin"), false);
    Row(TEXT("RetainFalseIsBoolean"), Accepted(Rpc(TEXT("draft.replace"), Args)));
    Args->SetBoolField(TEXT("retainExportOrigin"), true);
    Row(TEXT("RetainTrueIsBoolean"), Accepted(Rpc(TEXT("draft.replace"), Args)));
    Args->SetStringField(TEXT("retainExportOrigin"), TEXT("true"));
    Row(TEXT("RetainStringRejectedByNative"), !Accepted(Rpc(TEXT("draft.replace"), Args)));
    for (int32 Number : {0, 1})
    { Args->SetNumberField(TEXT("retainExportOrigin"), Number); Row(TEXT("RetainNumberRejectedByNative"), !Accepted(Rpc(TEXT("draft.replace"), Args))); }
    Args->SetBoolField(TEXT("retainExportOrigin"), true);
    Args->SetStringField(TEXT("exportReportPath"), TEXT("C:/NodeForgeExample/Reports/CopyContract.json"));
    Args->SetStringField(TEXT("exportReportSha256"), FString::ChrN(64, 'b'));
    Row(TEXT("RetainCannotReplaceOrigin"), !Accepted(Rpc(TEXT("draft.replace"), Args)));
    Args->SetBoolField(TEXT("retainExportOrigin"), false);
    Row(TEXT("FalseAllowsOriginalPairedLoad"), Accepted(Rpc(TEXT("draft.replace"), Args)));
    auto Empty = MakeShared<FJsonObject>();
    for (const TCHAR* Method : {TEXT("draft.prepareCopy"), TEXT("graphs.preview")})
    {
        const auto* Definition = Find(Method, EHost::ActiveEditor);
        Row(TEXT("NewOperationHasGTRevisionAndNoCommandlet"), Definition && Definition->Dispatch == EDispatch::GameThread
            && Definition->RevisionRequired && !Find(Method, EHost::Commandlet));
        Row(TEXT("NewOperationEmptyArgumentsAccepted"), Accepted(Rpc(Method, Empty)));
        Row(TEXT("NewOperationRequiresRevision"), !Accepted(Rpc(Method, Empty, false)));
        auto Extra = MakeShared<FJsonObject>(); Extra->SetStringField(TEXT("previousReport"), TEXT("arbitrary"));
        Row(TEXT("NewOperationRejectsCallerBaseline"), !Accepted(Rpc(Method, Extra)));
    }
    auto Service = MakeShared<FNodeForgeEditorControlService>(); FString Error;
    Row(TEXT("FreshOrdinaryDraftCreated"), Service->ReplaceDraft(TEXT("{}"), TEXT("program"), FString(), FString(), Error));
    const uint64 Revision = Service->State().Input.Revision();
    const FString Text = Service->State().Input.Draft().Text;
    Args = MakeShared<FJsonObject>(); Args->SetStringField(TEXT("text"), TEXT("{\"changed\":true}")); Args->SetBoolField(TEXT("retainExportOrigin"), true);
    Row(TEXT("RetainGuardOwnsRealControlSlot"), Service->BusinessGate()->ReserveControl());
    Row(TEXT("RetainWithoutOriginRejectsWithoutMutation"), Service->ExecuteDraftCall(TEXT("draft.replace"), Args, Revision)->GetStringField(TEXT("reason")) == TEXT("exportOriginRequired")
        && Service->State().Input.Revision() == Revision && Service->State().Input.Draft().Text == Text);
    Service->BusinessGate()->EndControl();
    const FString Session = FString::ChrN(32, 'a');
    Row(TEXT("PreviewWithoutOriginRejectsBeforeSnapshot"), Service->ExecuteGraphCall(TEXT("graphs.preview"), Revision, Session, TEXT("NoOrigin1"), FString())->GetStringField(TEXT("code")) == TEXT("exportOriginRequired"));
    Row(TEXT("PrepareInvalidGraphRejectsBeforeSnapshot"), Service->ExecuteGraphCall(TEXT("draft.prepareCopy"), Revision, Session, TEXT("NoGraph1"), FString())->GetStringField(TEXT("code")) == TEXT("copyGraphRequired"));
    for (const TCHAR* Method : {TEXT("draft.prepareCopy"), TEXT("graphs.preview")})
        Row(TEXT("StaleNewOperationRejectsBeforeSnapshot"), Service->ExecuteGraphCall(Method, Revision - 1, Session, TEXT("Stale1"), FString())->GetStringField(TEXT("code")) == TEXT("staleRevision"));
    Row(TEXT("UiWithoutOriginUsesSameClosedGuard"), Service->PreviewFromUI() == 2 && Service->State().Input.Revision() == Revision);
    Row(TEXT("DraftMetadataNamesPreparationEvenWhenAbsent"), Service->GetDraft()->HasTypedField<EJson::String>(TEXT("copyPreparationReport")));
    auto Report = MakeShared<FJsonObject>(); Report->SetStringField(TEXT("operationStatus"), All ? TEXT("passed") : TEXT("failed"));
    Report->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
    const FString Module = FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"));
    Report->SetStringField(TEXT("module"), Module); Report->SetStringField(TEXT("moduleSha256"), NodeForgePrototype::FileSha256(Module));
    Report->SetStringField(TEXT("scope"), TEXT("C native schema and pre-dispatch state only; real source/edit/prepare/preview/UI buttons/asset/IPC notRun"));
    Report->SetArrayField(TEXT("checks"), Rows); return NodeForgePrototype::WriteJson(Report, Output) && All;
}
#endif
