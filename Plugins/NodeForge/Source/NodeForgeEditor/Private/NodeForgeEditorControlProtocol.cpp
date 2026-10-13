#include "NodeForgeEditorControlProtocol.h"
#include "NodeForgePrototype.h"
#include "NodeForgePanelState.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace NodeForgeEditorControl
{
EFrameState FFrameReader::Reject(const TCHAR* Code)
{ ErrorCode = Code; Current = EFrameState::Rejected; return Current; }
EFrameState FFrameReader::CheckDeadline(double Now)
{
    if (Current == EFrameState::NeedMore && (!FMath::IsFinite(Now) || Now >= Deadline)) Reject(TEXT("frameTimeout"));
    return Current;
}
int32 FFrameReader::RemainingBytes() const
{ return Current != EFrameState::NeedMore ? 0 : HeaderRead < 4 ? 4 - HeaderRead : Payload.Num() - BodyRead; }
EFrameState FFrameReader::Feed(TConstArrayView<uint8> Bytes, double Now)
{
    if (CheckDeadline(Now) != EFrameState::NeedMore)
        return Bytes.Num() && Current == EFrameState::Complete ? Reject(TEXT("frameTrailingBytes")) : Current;
    int32 Cursor = 0;
    while (HeaderRead < 4 && Cursor < Bytes.Num()) Header[HeaderRead++] = Bytes[Cursor++];
    if (HeaderRead < 4) return Current;
    if (Payload.IsEmpty())
    {
        const uint32 Length = uint32(Header[0]) | (uint32(Header[1]) << 8) | (uint32(Header[2]) << 16) | (uint32(Header[3]) << 24);
        if (!Length || Length > MaxFrameBytes) return Reject(TEXT("frameLength"));
        Payload.SetNumUninitialized(static_cast<int32>(Length)); // Only after all four validated header bytes.
    }
    const int32 Count = Bytes.Num() - Cursor;
    if (Count > Payload.Num() - BodyRead) return Reject(TEXT("frameTrailingBytes"));
    if (Count) FMemory::Memcpy(Payload.GetData() + BodyRead, Bytes.GetData() + Cursor, Count);
    BodyRead += Count;
    if (BodyRead == Payload.Num()) Current = EFrameState::Complete;
    return Current;
}

bool IsCallId(const FString& Text)
{
    if (Text.Len() < 1 || Text.Len() > 64) return false;
    for (TCHAR C : Text)
        if (!((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9') || C == '-')) return false;
    return true;
}
bool DecimalUint64(const FString& Text, uint64& Value)
{
    if (Text.IsEmpty() || Text.Len() > 20 || (Text.Len() > 1 && Text[0] == '0')) return false;
    uint64 Parsed = 0;
    for (TCHAR C : Text)
    {
        if (C < '0' || C > '9' || Parsed > (MAX_uint64 - uint64(C - '0')) / 10) return false;
        Parsed = Parsed * 10 + uint64(C - '0');
    }
    Value = Parsed; return true;
}
bool DecodeUtf8(TConstArrayView<uint8> Bytes, FString& Text, uint32 MaxBytes)
{
    if (!MaxBytes || MaxBytes > MaxReportBytes || Bytes.IsEmpty() || Bytes.Num() > static_cast<int32>(MaxBytes)
        || (Bytes.Num() >= 3 && Bytes[0] == 0xef && Bytes[1] == 0xbb && Bytes[2] == 0xbf)) return false;
    // Scalar validation before UE conversion: rejects overlong encodings, surrogates, NUL and >U+10FFFF.
    for (int32 I = 0; I < Bytes.Num();)
    {
        const uint8 First = Bytes[I++]; uint32 Scalar = First; int32 Extra = 0; uint32 Minimum = 0;
        if (First < 0x80) { if (!First) return false; continue; }
        if (First >= 0xc2 && First <= 0xdf) { Extra = 1; Scalar &= 0x1f; Minimum = 0x80; }
        else if (First >= 0xe0 && First <= 0xef) { Extra = 2; Scalar &= 0x0f; Minimum = 0x800; }
        else if (First >= 0xf0 && First <= 0xf4) { Extra = 3; Scalar &= 0x07; Minimum = 0x10000; }
        else return false;
        if (Bytes.Num() - I < Extra) return false;
        for (int32 J = 0; J < Extra; ++J)
        { const uint8 Next = Bytes[I++]; if ((Next & 0xc0) != 0x80) return false; Scalar = (Scalar << 6) | (Next & 0x3f); }
        if (Scalar < Minimum || Scalar > 0x10ffff || (Scalar >= 0xd800 && Scalar <= 0xdfff)) return false;
    }
    FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(Bytes.GetData()), Bytes.Num());
    Text = FString(Converted.Length(), Converted.Get()); return true;
}
namespace
{
bool Hex(const FString& Text, int32 Length)
{
    if (Text.Len() != Length) return false;
    for (TCHAR C : Text) if (!((C >= '0' && C <= '9') || (C >= 'a' && C <= 'f'))) return false;
    return true;
}
int32 HexDigit(TCHAR C)
{ return C >= '0' && C <= '9' ? C - '0' : C >= 'a' && C <= 'f' ? C - 'a' + 10 : C >= 'A' && C <= 'F' ? C - 'A' + 10 : -1; }
bool HexUnit(const FString& Text, int32 Start, uint32& Unit)
{
    if (Start + 4 > Text.Len()) return false;
    Unit = 0;
    for (int32 J = 0; J < 4; ++J) { const int32 Digit = HexDigit(Text[Start + J]); if (Digit < 0) return false; Unit = (Unit << 4) | Digit; }
    return true;
}
bool EscapedScalars(const FString& Text)
{
    // JSON syntax remains the UE reader's job. This pass only checks escaped scalar pairs/NUL.
    bool InString = false;
    for (int32 I = 0; I < Text.Len(); ++I)
    {
        if (Text[I] == '"') { InString = !InString; continue; }
        if (!InString || Text[I] != '\\') continue;
        if (++I >= Text.Len()) return false;
        if (Text[I] != 'u') continue;
        uint32 Unit;
        if (!HexUnit(Text, I + 1, Unit) || !Unit) return false;
        I += 4;
        if (Unit >= 0xdc00 && Unit <= 0xdfff) return false;
        if (Unit >= 0xd800 && Unit <= 0xdbff)
        {
            if (I + 6 >= Text.Len() || Text[I + 1] != '\\' || Text[I + 2] != 'u') return false;
            uint32 Low; if (!HexUnit(Text, I + 3, Low) || Low < 0xdc00 || Low > 0xdfff) return false;
            I += 6;
        }
    }
    return true;
}
bool StrictJson(const FString& Text, TSharedPtr<FJsonObject>& Json)
{
    if (!EscapedScalars(Text)) return false;
    const FString Trimmed = Text.TrimStartAndEnd();
    const bool WrappedScalar = !Trimmed.IsEmpty() && Trimmed[0] != '{' && Trimmed[0] != '[';
    const FString ParseText = WrappedScalar ? TEXT("[") + Text + TEXT("]") : Text;
    auto Reader = TJsonReaderFactory<>::Create(ParseText);
    EJsonNotation Notation; TArray<TSet<FString>> Keys; TArray<bool> Objects;
    bool ObjectRoot = false, Started = false; int32 WrappedChildren = 0;
    // Same UE tokenizer/decoded-key technique as the existing graph parser; graph parser is untouched.
    while (Reader->ReadNext(Notation))
    {
        if (Notation == EJsonNotation::Error) return false;
        if (!Started) { Started = true; ObjectRoot = Notation == EJsonNotation::ObjectStart; }
        if (Notation == EJsonNotation::ObjectEnd || Notation == EJsonNotation::ArrayEnd)
        { if (!Keys.Num()) return false; Keys.Pop(); Objects.Pop(); continue; }
        if (WrappedScalar && Keys.Num() == 1) ++WrappedChildren;
        if (Objects.Num() && Objects.Last())
        {
            const FString Key = Reader->GetIdentifier();
            if (Keys.Last().Contains(Key)) return false;
            Keys.Last().Add(Key);
        }
        if (Notation == EJsonNotation::Number && !FMath::IsFinite(Reader->GetValueAsNumber())) return false;
        if (Notation == EJsonNotation::ObjectStart || Notation == EJsonNotation::ArrayStart)
        {
            if (Keys.Num() >= MaxJsonDepth + (WrappedScalar ? 1 : 0)) return false;
            Keys.AddDefaulted(); Objects.Add(Notation == EJsonNotation::ObjectStart);
        }
    }
    if (!Reader->GetErrorMessage().IsEmpty() || !Started || Keys.Num() || (WrappedScalar && WrappedChildren != 1)) return false;
    // Valid arrays/scalars are Invalid Request rather than malformed JSON. A null DOM distinguishes them.
    if (!ObjectRoot) { Json.Reset(); return true; }
    return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) && Json.IsValid();
}
bool Exact(const TSharedPtr<FJsonObject>& Object, std::initializer_list<const TCHAR*> Fields)
{
    if (!Object || Object->Values.Num() != static_cast<int32>(Fields.size())) return false;
    for (const TCHAR* Field : Fields) if (!Object->HasField(Field)) return false;
    return true;
}
bool SchemaMatches(const TSharedPtr<FJsonObject>& Object, const TSharedRef<FJsonObject>& Schema)
{
    const auto Properties = Schema->GetObjectField(TEXT("properties"));
    if (!Object) return false;
    for (const auto& Required : Schema->GetArrayField(TEXT("required"))) if (!Object->HasField(Required->AsString())) return false;
    for (const auto& Item : Object->Values)
    {
        const auto* Property = Properties->Values.Find(Item.Key);
        if (!Property || !Item.Value.IsValid()) return false;
        const auto* Value = &Item.Value;
        const auto Definition = (*Property)->AsObject();
        const FString Type = Definition->GetStringField(TEXT("type"));
        if (Type == TEXT("string")) { if ((*Value)->Type != EJson::String) return false; }
        else if (Type == TEXT("boolean")) { if ((*Value)->Type != EJson::Boolean) return false; }
        else if (Type == TEXT("integer"))
        { if ((*Value)->Type != EJson::Number || !FMath::IsFinite((*Value)->AsNumber())
            || FMath::FloorToDouble((*Value)->AsNumber()) != (*Value)->AsNumber()) return false; }
        else return false;
        const TArray<TSharedPtr<FJsonValue>>* Allowed = nullptr;
        if (Definition->TryGetArrayField(TEXT("enum"), Allowed))
        {
            bool Match = false; for (const auto& Candidate : *Allowed) Match |= Candidate->AsString() == (*Value)->AsString();
            if (!Match) return false;
        }
        double MaxLength;
        if (Definition->TryGetNumberField(TEXT("maxLength"), MaxLength) && (*Value)->AsString().Len() > MaxLength) return false;
        double MinLength, Minimum, Maximum;
        if (Definition->TryGetNumberField(TEXT("minLength"), MinLength) && (*Value)->AsString().Len() < MinLength) return false;
        if (Definition->TryGetNumberField(TEXT("minimum"), Minimum) && (*Value)->AsNumber() < Minimum) return false;
        if (Definition->TryGetNumberField(TEXT("maximum"), Maximum) && (*Value)->AsNumber() > Maximum) return false;
    }
    return true;
}
FString Canonical(const TSharedPtr<FJsonValue>& Value)
{
    if (Value->Type == EJson::Object)
    {
        TArray<FString> Keys; Value->AsObject()->Values.GetKeys(Keys); Keys.Sort(); FString Text = TEXT("{"); bool First = true;
        for (const FString& Key : Keys)
        {
            auto KeyObject = MakeShared<FJsonObject>(); KeyObject->SetStringField(TEXT("k"), Key);
            FString Encoded; FJsonSerializer::Serialize(KeyObject, TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Encoded));
            // {"k":<quoted key>} comes from UE's escaping; preserve its quoted portion.
            if (!First) Text += TEXT(","); First = false;
            Text += Encoded.Mid(5, Encoded.Len() - 6) + TEXT(":") + Canonical(Value->AsObject()->Values[Key]);
        }
        return Text + TEXT("}");
    }
    if (Value->Type == EJson::Array)
    {
        FString Text = TEXT("["); bool First = true;
        for (const auto& Child : Value->AsArray()) { if (!First) Text += TEXT(","); First = false; Text += Canonical(Child); }
        return Text + TEXT("]");
    }
    auto Wrapper = MakeShared<FJsonObject>(); Wrapper->SetField(TEXT("v"), Value); FString Encoded;
    FJsonSerializer::Serialize(Wrapper, TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Encoded));
    return Encoded.Mid(5, Encoded.Len() - 6);
}
}
bool ParseObject(TConstArrayView<uint8> Bytes, TSharedPtr<FJsonObject>& Object, uint32 MaxBytes)
{
    FString Text; Object.Reset();
    return DecodeUtf8(Bytes, Text, MaxBytes) && StrictJson(Text, Object) && Object.IsValid();
}
FString SemanticDigest(const TSharedRef<FJsonObject>& Object)
{ return NodeForgePrototype::TextSha256(Canonical(MakeShared<FJsonValueObject>(Object))); }
bool ParseRequest(TConstArrayView<uint8> Bytes, uint8 EnabledFeatures, FRequest& Request, FProtocolError& Error)
{
    Request = FRequest(); Error = FProtocolError(); FString Text; TSharedPtr<FJsonObject> Root;
    auto Reject = [&](int32 RpcCode, const TCHAR* Code) { Error.RpcCode = RpcCode; Error.Code = Code; return false; };
    if (!DecodeUtf8(Bytes, Text) || !StrictJson(Text, Root)) return Reject(-32700, TEXT("parseError"));
    if (!Root) return Reject(-32600, TEXT("invalidRequest"));
    if (!Root->HasField(TEXT("id")))
    { Error.bReply = false; Error.bClose = true; return Reject(-32600, TEXT("notificationUnsupported")); }
    if (!Exact(Root, {TEXT("jsonrpc"), TEXT("id"), TEXT("method"), TEXT("params")})
        || !Root->HasTypedField<EJson::String>(TEXT("id")) || !IsCallId(Root->GetStringField(TEXT("id")))
        || !Root->HasTypedField<EJson::String>(TEXT("jsonrpc")) || Root->GetStringField(TEXT("jsonrpc")) != TEXT("2.0")
        || !Root->HasTypedField<EJson::String>(TEXT("method")) || !Root->HasTypedField<EJson::Object>(TEXT("params")))
        return Reject(-32600, TEXT("invalidRequest"));
    Request.Id = Root->GetStringField(TEXT("id")); Error.Id = Request.Id;
    Request.Method = Root->GetStringField(TEXT("method"));
    Request.Definition = NodeForgeControlOperations::Find(Request.Method, NodeForgeControlOperations::EHost::ActiveEditor);
    if (!Request.Definition || !(EnabledFeatures & static_cast<uint8>(Request.Definition->EditorFeature)))
        return Reject(-32601, TEXT("operationUnavailable"));
    const auto Params = Root->GetObjectField(TEXT("params"));
    if (Request.Definition->Dispatch == NodeForgeControlOperations::EDispatch::Handshake) Request.Arguments = Params;
    else
    {
        if (!Exact(Params, {TEXT("context"), TEXT("arguments")})
            || !Params->HasTypedField<EJson::Object>(TEXT("context")) || !Params->HasTypedField<EJson::Object>(TEXT("arguments")))
            return Reject(-32602, TEXT("invalidParams"));
        Request.Context = Params->GetObjectField(TEXT("context")); Request.Arguments = Params->GetObjectField(TEXT("arguments"));
        const auto& Context = Request.Context;
        if (!(Request.Definition->RevisionRequired
            ? Exact(Context, {TEXT("controlSessionId"), TEXT("editorApiVersion"), TEXT("profile"), TEXT("expectedDraftRevision")})
            : Exact(Context, {TEXT("controlSessionId"), TEXT("editorApiVersion"), TEXT("profile")})))
            return Reject(-32602, TEXT("invalidContext"));
        for (const auto& Pair : Context->Values) if (Pair.Value->Type != EJson::String) return Reject(-32602, TEXT("invalidContext"));
        if (!Hex(Context->GetStringField(TEXT("controlSessionId")), 32)
            || Context->GetStringField(TEXT("editorApiVersion")) != ApiVersion || Context->GetStringField(TEXT("profile")) != Profile)
            return Reject(-32010, TEXT("sessionMismatch"));
        if (Request.Definition->RevisionRequired && !DecimalUint64(Context->GetStringField(TEXT("expectedDraftRevision")), Request.ExpectedRevision))
            return Reject(-32602, TEXT("invalidRevision"));
    }
    if (!SchemaMatches(Request.Arguments, NodeForgeControlOperations::ArgumentsSchema(*Request.Definition)))
        return Reject(-32602, TEXT("invalidParams"));
    using NodeForgeControlOperations::EArguments;
    if (Request.Definition->Arguments == EArguments::Path || Request.Definition->Arguments == EArguments::Text)
    {
        const bool HasPath = Request.Arguments->HasField(TEXT("exportReportPath"));
        const bool HasHash = Request.Arguments->HasField(TEXT("exportReportSha256"));
        bool Retain = false;
        if (Request.Arguments->TryGetBoolField(TEXT("retainExportOrigin"), Retain) && Retain && (HasPath || HasHash))
            return Reject(-32602, TEXT("invalidExportReference"));
        if (HasPath != HasHash || (HasPath && (Request.Arguments->GetStringField(TEXT("exportReportPath")).IsEmpty()
            || !Hex(Request.Arguments->GetStringField(TEXT("exportReportSha256")), 64)))) return Reject(-32602, TEXT("invalidExportReference"));
    }
    switch (Request.Definition->Arguments)
    {
    case EArguments::Hello:
    {
        const auto& Args = Request.Arguments; uint64 Start = 0; const double Pid = Args->GetNumberField(TEXT("expectedPid"));
        const bool HasClient=Args->HasField(TEXT("clientId"));
        if(HasClient!=Args->HasField(TEXT("clientName"))||HasClient!=Args->HasField(TEXT("clientVersion"))
            ||(HasClient&&!Hex(Args->GetStringField(TEXT("clientId")),32)))return Reject(-32602,TEXT("invalidClientIdentity"));
        if (Args->GetStringField(TEXT("editorApiVersion")) != ApiVersion || Args->GetStringField(TEXT("profile")) != Profile
            || !Hex(Args->GetStringField(TEXT("controlSessionId")), 32) || !Hex(Args->GetStringField(TEXT("credential")), 64)
            || !Hex(Args->GetStringField(TEXT("expectedBinarySha256")), 64) || Args->GetStringField(TEXT("expectedProject")).IsEmpty()
            || !DecimalUint64(Args->GetStringField(TEXT("expectedProcessStart")), Start) || !Start || Pid < 1 || Pid > MAX_uint32)
            return Reject(-32010, TEXT("sessionMismatch"));
        break;
    }
    case EArguments::Text:
    { FString Detail; if (!FNodeForgePanelState::ValidateExternalText(Request.Arguments->GetStringField(TEXT("text")), Detail)) return Reject(-32602, TEXT("invalidDraft")); break; }
    case EArguments::Path:
        if (Request.Arguments->GetStringField(TEXT("path")).IsEmpty()) return Reject(-32602, TEXT("invalidPath")); break;
    case EArguments::CallId:
        if (!IsCallId(Request.Arguments->GetStringField(TEXT("callId")))) return Reject(-32602, TEXT("invalidCallId")); break;
    default: break;
    }
    return true;
}
TSharedRef<FJsonObject> Result(const FString& Id, const TSharedRef<FJsonObject>& Data)
{
    auto Envelope = MakeShared<FJsonObject>(); Envelope->SetStringField(TEXT("jsonrpc"), TEXT("2.0"));
    Envelope->SetStringField(TEXT("id"), Id); Envelope->SetObjectField(TEXT("result"), Data); return Envelope;
}
TSharedRef<FJsonObject> Failure(const FProtocolError& Error)
{
    auto Envelope = MakeShared<FJsonObject>(); Envelope->SetStringField(TEXT("jsonrpc"), TEXT("2.0"));
    if (Error.Id.IsEmpty()) Envelope->SetField(TEXT("id"), MakeShared<FJsonValueNull>()); else Envelope->SetStringField(TEXT("id"), Error.Id);
    auto Details = MakeShared<FJsonObject>(); Details->SetNumberField(TEXT("code"), Error.RpcCode);
    Details->SetStringField(TEXT("message"), TEXT("Request rejected"));
    auto Data = MakeShared<FJsonObject>(); Data->SetStringField(TEXT("code"), Error.Code);
    Details->SetObjectField(TEXT("data"), Data); Envelope->SetObjectField(TEXT("error"), Details); return Envelope;
}
bool EncodeFrame(const TSharedRef<FJsonObject>& Json, TArray<uint8>& Frame)
{
    FString Text;
    if (!FJsonSerializer::Serialize(Json, TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Text))) return false;
    FTCHARToUTF8 Bytes(*Text); if (Bytes.Length() < 1 || Bytes.Length() > static_cast<int32>(MaxFrameBytes)) return false;
    Frame.SetNumUninitialized(4 + Bytes.Length());
    for (int32 I = 0; I < 4; ++I) Frame[I] = (static_cast<uint32>(Bytes.Length()) >> (8 * I)) & 0xff;
    FMemory::Memcpy(Frame.GetData() + 4, Bytes.Get(), Bytes.Length()); return true;
}
FString Fingerprint(const FRequest& Request)
{
    if (!Request.Definition || Request.Definition->Dispatch == NodeForgeControlOperations::EDispatch::Handshake) return FString();
    auto Payload = MakeShared<FJsonObject>(); Payload->SetStringField(TEXT("method"), Request.Method);
    Payload->SetObjectField(TEXT("context"), Request.Context.ToSharedRef()); Payload->SetObjectField(TEXT("arguments"), Request.Arguments.ToSharedRef());
    return NodeForgePrototype::TextSha256(Canonical(MakeShared<FJsonValueObject>(Payload)));
}
TSharedRef<FJsonObject> Limits()
{
    auto Json = MakeShared<FJsonObject>();
    Json->SetNumberField(TEXT("frameBytes"), MaxFrameBytes); Json->SetNumberField(TEXT("graphBytes"), MaxGraphBytes);
    Json->SetNumberField(TEXT("reportBytes"), MaxReportBytes); Json->SetNumberField(TEXT("descriptorBytes"), MaxDescriptorBytes);
    Json->SetNumberField(TEXT("jsonDepth"), MaxJsonDepth); Json->SetNumberField(TEXT("connections"), MaxConnections);
    Json->SetNumberField(TEXT("businessSlots"), 1); Json->SetNumberField(TEXT("retainedCalls"), MaxBusinessCalls);
    Json->SetNumberField(TEXT("helloSeconds"), HelloSeconds); Json->SetNumberField(TEXT("frameSeconds"), FrameSeconds);
    Json->SetNumberField(TEXT("callSeconds"), CallSeconds); Json->SetNumberField(TEXT("querySeconds"), QuerySeconds);
    Json->SetNumberField(TEXT("idleSeconds"), IdleSeconds); Json->SetNumberField(TEXT("stopSeconds"), StopSeconds);
    Json->SetNumberField(TEXT("exitSeconds"), ExitSeconds); return Json;
}
}
