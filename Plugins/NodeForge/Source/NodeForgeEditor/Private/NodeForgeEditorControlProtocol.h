#pragma once
#include "CoreMinimal.h"
#include "Containers/ArrayView.h"
#include "NodeForgeControlOperations.h"
#include "Dom/JsonObject.h"

// One profile for server, description and bounded client bootstrap. No graph schema here.
namespace NodeForgeEditorControl
{
    constexpr uint32 MaxFrameBytes = 524288;
    constexpr uint32 MaxGraphBytes = 65536;
    constexpr uint32 MaxReportBytes = 1048576;
    constexpr uint32 MaxDescriptorBytes = 65536;
    constexpr int32 MaxJsonDepth = 32;
    constexpr int32 MaxConnections = 2;
    constexpr int32 MaxBusinessCalls = 128;
    constexpr double HelloSeconds = 5, FrameSeconds = 10, CallSeconds = 180;
    constexpr double QuerySeconds = 5, IdleSeconds = 60, StopSeconds = 10, ExitSeconds = 60;
    constexpr const TCHAR* ApiVersion = TEXT("1.0");
    constexpr const TCHAR* Profile = TEXT("nodeforge.editor.v1");

    enum class EFrameState : uint8 { NeedMore, Complete, Rejected };
    // An absolute deadline is fixed at construction; partial progress never renews it.
    // Transport reads only RemainingBytes(), so a connection cannot buffer another request.
    class FFrameReader
    {
    public:
        explicit FFrameReader(double AbsoluteDeadline) : Deadline(AbsoluteDeadline) {}
        EFrameState Feed(TConstArrayView<uint8> Bytes, double Now);
        EFrameState CheckDeadline(double Now);
        void RestrictDeadline(double Earlier) { Deadline = FMath::Min(Deadline, Earlier); }
        int32 RemainingBytes() const;
        EFrameState State() const { return Current; }
        const TArray<uint8>& Body() const { check(Current == EFrameState::Complete); return Payload; }
        const FString& Error() const { return ErrorCode; }
        int32 AllocatedBodyBytes() const { return Payload.Num(); }
    private:
        EFrameState Reject(const TCHAR* Code);
        uint8 Header[4] = {}; int32 HeaderRead = 0, BodyRead = 0;
        TArray<uint8> Payload;
        double Deadline;
        EFrameState Current = EFrameState::NeedMore;
        FString ErrorCode;
    };

    struct FRequest
    {
        FString Id, Method;
        TSharedPtr<FJsonObject> Context, Arguments;
        const NodeForgeControlOperations::FOperationDefinition* Definition = nullptr;
        uint64 ExpectedRevision = 0;
    };
    struct FProtocolError
    {
        int32 RpcCode = -32700;
        FString Code = TEXT("parseError");
        FString Id; // Empty means the standard null id, never a fabricated call id.
        bool bReply = true, bClose = false;
    };
    bool IsCallId(const FString& Text);
    bool DecimalUint64(const FString& Text, uint64& Value);
    bool DecodeUtf8(TConstArrayView<uint8> Bytes, FString& Text, uint32 MaxBytes = MaxFrameBytes);
    bool ParseObject(TConstArrayView<uint8> Bytes, TSharedPtr<FJsonObject>& Object, uint32 MaxBytes = MaxFrameBytes);
    bool ParseRequest(TConstArrayView<uint8> Bytes, uint8 EnabledFeatures, FRequest& Request, FProtocolError& Error);
    TSharedRef<FJsonObject> Result(const FString& Id, const TSharedRef<FJsonObject>& Data);
    TSharedRef<FJsonObject> Failure(const FProtocolError& Error);
    bool EncodeFrame(const TSharedRef<FJsonObject>& Json, TArray<uint8>& Frame);
    FString Fingerprint(const FRequest& Request); // Decoded, sorted, exact, with no hello credential.
    FString SemanticDigest(const TSharedRef<FJsonObject>& Object); // Reuses canonical JSON; no graph fields added.
    TSharedRef<FJsonObject> Limits();
}
