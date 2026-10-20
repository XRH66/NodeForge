#include "NodeForgeEditorControlChannel.h"
#include "NodeForgeEditorControlProtocol.h"
#include "NodeForgePrototype.h"
#include "NodeForgeInstallation.h"
#include "NodeForgeEditorControlService.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformProcess.h"
#include "Misc/CommandLine.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"
#include "ILiveCodingModule.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Windows/WindowsHWrapper.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include <sddl.h>
#include <aclapi.h>
#include <bcrypt.h>
#include "Windows/HideWindowsPlatformTypes.h"
#include <atomic>
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#endif

namespace NodeForgeControlWindows
{
struct FHandle
{
    HANDLE Value = nullptr;
    FHandle() = default;
    explicit FHandle(HANDLE In) : Value(In) {}
    ~FHandle() { if (Valid()) CloseHandle(Value); }
    void Reset() { if (Valid()) CloseHandle(Value); Value = nullptr; }
    bool Valid() const { return Value && Value != INVALID_HANDLE_VALUE; }
    FHandle(const FHandle&) = delete; FHandle& operator=(const FHandle&) = delete;
};
FString Full(FString Path)
{ Path = FPaths::ConvertRelativePathToFull(Path); FPaths::NormalizeFilename(Path); FPaths::CollapseRelativeDirectories(Path); return Path; }
bool SafePath(const FString& Path, const FString& Root)
{
    if (Path.IsEmpty() || FPaths::IsRelative(Path) || !FPaths::IsUnderDirectory(Full(Path), Full(Root))) return false;
    FString Parent = Full(Path);
    while (!Parent.IsEmpty())
    {
        DWORD Attr = GetFileAttributesW(*Parent);
        if (Attr != INVALID_FILE_ATTRIBUTES && (Attr & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
        const FString Next = FPaths::GetPath(Parent); if (Next == Parent) break; Parent = Next;
    }
    return true;
}
FString JsonText(const TSharedRef<FJsonObject>& Json)
{ FString Text; FJsonSerializer::Serialize(Json, TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Text)); return Text; }
bool ReadObject(const FString& Path, TSharedPtr<FJsonObject>& Json, uint32 Limit)
{
    FHandle File(CreateFileW(*Path, FILE_READ_DATA | FILE_READ_ATTRIBUTES, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    if (!File.Valid()) return false;
    BY_HANDLE_FILE_INFORMATION Info; LARGE_INTEGER Size;
    if (!GetFileInformationByHandle(File.Value, &Info) || (Info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        || !GetFileSizeEx(File.Value, &Size) || Size.QuadPart < 1 || Size.QuadPart > Limit) return false;
    TCHAR Final[32768]; DWORD FinalSize = GetFinalPathNameByHandleW(File.Value, Final, UE_ARRAY_COUNT(Final), FILE_NAME_NORMALIZED);
    FString FinalPath(Final); FinalPath.RemoveFromStart(TEXT("\\\\?\\"));
    if (!FinalSize || FinalSize >= UE_ARRAY_COUNT(Final) || !Full(FinalPath).Equals(Full(Path), ESearchCase::IgnoreCase)) return false;
    TArray<uint8> Bytes; Bytes.SetNumUninitialized(static_cast<int32>(Size.QuadPart)); DWORD Read = 0;
    return ReadFile(File.Value, Bytes.GetData(), Bytes.Num(), &Read, nullptr) && Read == static_cast<DWORD>(Bytes.Num()) && NodeForgeEditorControl::ParseObject(Bytes, Json, Limit);
}
bool TokenBytes(HANDLE Token, TOKEN_INFORMATION_CLASS Kind, TArray<uint8>& Bytes)
{
    DWORD Size = 0; GetTokenInformation(Token, Kind, nullptr, 0, &Size); if (!Size || Size > 65536) return false;
    Bytes.SetNumUninitialized(Size); return GetTokenInformation(Token, Kind, Bytes.GetData(), Size, &Size) != 0;
}
FString SidText(PSID Sid)
{ LPWSTR Text = nullptr; if (!ConvertSidToStringSidW(Sid, &Text)) return FString(); FString Result(Text); LocalFree(Text); return Result; }
struct FSecurity
{
    TArray<uint8> User, Logon; FString UserText, LogonText;
    PSECURITY_DESCRIPTOR FileSd = nullptr, PipeSd = nullptr, ContainerSd = nullptr;
    ~FSecurity() { if (FileSd) LocalFree(FileSd); if (PipeSd) LocalFree(PipeSd); if (ContainerSd) LocalFree(ContainerSd); }
    bool Initialize()
    {
        FHandle Token; if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &Token.Value)) return false;
        TArray<uint8> UserInfo, Groups;
        if (!TokenBytes(Token.Value, TokenUser, UserInfo) || !TokenBytes(Token.Value, TokenGroups, Groups)) return false;
        PSID UserSid = reinterpret_cast<TOKEN_USER*>(UserInfo.GetData())->User.Sid;
        User.SetNumUninitialized(GetLengthSid(UserSid)); if (!CopySid(User.Num(), User.GetData(), UserSid)) return false;
        const auto* Values = reinterpret_cast<TOKEN_GROUPS*>(Groups.GetData());
        for (DWORD I = 0; I < Values->GroupCount; ++I)
            if ((Values->Groups[I].Attributes & SE_GROUP_LOGON_ID) == SE_GROUP_LOGON_ID)
            { Logon.SetNumUninitialized(GetLengthSid(Values->Groups[I].Sid)); if (!CopySid(Logon.Num(), Logon.GetData(), Values->Groups[I].Sid)) return false; break; }
        if (Logon.IsEmpty()) return false; UserText = SidText(User.GetData()); LogonText = SidText(Logon.GetData());
        // Duplex server instances need mapped generic read/write and CREATE_PIPE_INSTANCE (same bit as append).
        // The client asks only READ_DATA|WRITE_DATA|SYNCHRONIZE; no same-logon adversary-resistance claim.
        const FString Files = TEXT("D:P(A;OICI;FA;;;SY)(A;OICI;FA;;;") + LogonText + TEXT(")");
        const FString Pipes = TEXT("D:P(A;;0x0012019f;;;SY)(A;;0x0012019f;;;") + LogonText + TEXT(")");
        // The shared container has user lifetime and never grants inherited access
        // to a credential child. Each child keeps its protected logon-only DACL.
        const FString Container = TEXT("D:P(A;;FA;;;SY)(A;;FA;;;") + UserText + TEXT(")");
        return ConvertStringSecurityDescriptorToSecurityDescriptorW(*Files, SDDL_REVISION_1, &FileSd, nullptr)
            && ConvertStringSecurityDescriptorToSecurityDescriptorW(*Pipes, SDDL_REVISION_1, &PipeSd, nullptr)
            && ConvertStringSecurityDescriptorToSecurityDescriptorW(*Container, SDDL_REVISION_1, &ContainerSd, nullptr);
    }
    bool Inspect(HANDLE Handle, SE_OBJECT_TYPE Kind, DWORD ExpectedMask, TSharedRef<FJsonObject>& Evidence, bool UserContainer = false) const
    {
        PSID Owner = nullptr; PACL Dacl = nullptr; PSECURITY_DESCRIPTOR Sd = nullptr;
        const DWORD Result = GetSecurityInfo(Handle, Kind, OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION, &Owner, nullptr, &Dacl, nullptr, &Sd);
        if (Result != ERROR_SUCCESS) return false;
        SECURITY_DESCRIPTOR_CONTROL Control = 0; DWORD Revision = 0;
        bool Good = Owner && EqualSid(Owner, const_cast<uint8*>(User.GetData())) && Dacl && Dacl->AceCount == 2
            && GetSecurityDescriptorControl(Sd, &Control, &Revision) && (Control & SE_DACL_PROTECTED);
        const FString Principal = UserContainer ? UserText : LogonText;
        TArray<TSharedPtr<FJsonValue>> Aces; TSet<FString> Principals;
        for (DWORD I = 0; Dacl && I < Dacl->AceCount; ++I)
        {
            PVOID Ace = nullptr; if (!GetAce(Dacl, I, &Ace)) { Good = false; continue; }
            const auto* Allowed = reinterpret_cast<ACCESS_ALLOWED_ACE*>(Ace);
            PSID Sid = const_cast<DWORD*>(&Allowed->SidStart); const FString Name = SidText(Sid);
            Good &= Allowed->Header.AceType == ACCESS_ALLOWED_ACE_TYPE && !(Allowed->Header.AceFlags & INHERITED_ACE)
                && Allowed->Mask == ExpectedMask && (Name == Principal || Name == TEXT("S-1-5-18"))
                && (!UserContainer || Allowed->Header.AceFlags == 0);
            Principals.Add(Name);
            auto Row = MakeShared<FJsonObject>(); Row->SetStringField(TEXT("sid"), Name); Row->SetNumberField(TEXT("mask"), Allowed->Mask);
            Row->SetNumberField(TEXT("flags"), Allowed->Header.AceFlags); Aces.Add(MakeShared<FJsonValueObject>(Row));
        }
        Good &= Principals.Num() == 2 && Principals.Contains(Principal) && Principals.Contains(TEXT("S-1-5-18"));
        Evidence->SetStringField(TEXT("scope"), UserContainer ? TEXT("userContainer") : TEXT("logonSession"));
        Evidence->SetStringField(TEXT("owner"), Owner ? SidText(Owner) : FString()); Evidence->SetStringField(TEXT("logonSid"), LogonText);
        Evidence->SetBoolField(TEXT("protectedDacl"), (Control & SE_DACL_PROTECTED) != 0); Evidence->SetArrayField(TEXT("aces"), Aces);
        Evidence->SetBoolField(TEXT("passed"), Good); LocalFree(Sd); return Good;
    }
    bool Directory(const FString& Path, TSharedRef<FJsonObject>& Evidence)
    {
        SECURITY_ATTRIBUTES Attributes = {sizeof(SECURITY_ATTRIBUTES), ContainerSd, 0};
        if (!CreateDirectoryW(*Path, &Attributes) && GetLastError() != ERROR_ALREADY_EXISTS) return false;
        auto Validate = [&]()
        {
            FHandle Dir(CreateFileW(*Path, READ_CONTROL | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
            BY_HANDLE_FILE_INFORMATION Info;
            return Dir.Valid() && GetFileInformationByHandle(Dir.Value, &Info)
                && (Info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !(Info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
                && Inspect(Dir.Value, SE_FILE_OBJECT, FILE_ALL_ACCESS, Evidence, true);
        };
        if (Validate()) { Evidence->SetBoolField(TEXT("migratedLegacyParent"), false); return true; }
        // A previous logon's DACL can deny FILE_READ_ATTRIBUTES even to the
        // owner. Inspect its exact directory entry and named security metadata;
        // never recurse, read old credentials, follow a reparse point or take ownership.
        WIN32_FIND_DATAW Entry; HANDLE Find = FindFirstFileW(*Path, &Entry);
        if (Find == INVALID_HANDLE_VALUE) return false;
        FindClose(Find);
        if (!(Entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || (Entry.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
        PSID Owner = nullptr; PACL Acl = nullptr; PSECURITY_DESCRIPTOR Sd = nullptr;
        const DWORD ReadCode = GetNamedSecurityInfoW(*Path, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
            &Owner, nullptr, &Acl, nullptr, &Sd);
        if (ReadCode != ERROR_SUCCESS) { SetLastError(ReadCode); return false; }
        SECURITY_DESCRIPTOR_CONTROL Control = 0; DWORD Revision = 0; TSet<FString> Names; FString OldLogon;
        bool Legacy = Owner && EqualSid(Owner, User.GetData()) && Acl && Acl->AceCount == 2
            && GetSecurityDescriptorControl(Sd, &Control, &Revision) && (Control & SE_DACL_PROTECTED);
        for (DWORD I = 0; Legacy && I < Acl->AceCount; ++I)
        {
            PVOID Ace = nullptr;
            if (!GetAce(Acl, I, &Ace)) { Legacy = false; break; }
            const auto* Allowed = reinterpret_cast<ACCESS_ALLOWED_ACE*>(Ace);
            PSID Sid = const_cast<DWORD*>(&Allowed->SidStart); const FString Name = SidText(Sid);
            const bool LogonSid = IsWellKnownSid(Sid, WinLogonIdsSid) != 0;
            Legacy &= Allowed->Header.AceType == ACCESS_ALLOWED_ACE_TYPE
                && Allowed->Header.AceFlags == (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE)
                && Allowed->Mask == FILE_ALL_ACCESS && (Name == TEXT("S-1-5-18") || LogonSid);
            Names.Add(Name); if (LogonSid) OldLogon = Name;
        }
        Legacy &= Names.Num() == 2 && Names.Contains(TEXT("S-1-5-18")) && !OldLogon.IsEmpty();
        LocalFree(Sd);
        if (!Legacy) { SetLastError(ERROR_INVALID_SECURITY_DESCR); return false; }
        BOOL Present = false, Defaulted = false; PACL NewAcl = nullptr;
        if (!GetSecurityDescriptorDacl(ContainerSd, &Present, &NewAcl, &Defaulted) || !Present || !NewAcl) return false;
        const DWORD WriteCode = SetNamedSecurityInfoW(const_cast<LPWSTR>(*Path), SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, nullptr, nullptr, NewAcl, nullptr);
        if (WriteCode != ERROR_SUCCESS) { SetLastError(WriteCode); return false; }
        const bool Good = Validate();
        Evidence->SetBoolField(TEXT("migratedLegacyParent"), Good);
        Evidence->SetStringField(TEXT("previousLogonSid"), OldLogon);
        return Good;
    }
    bool SessionDirectory(const FString& Path, TSharedRef<FJsonObject>& Evidence)
    {
        SECURITY_ATTRIBUTES Attributes = {sizeof(SECURITY_ATTRIBUTES), FileSd, 0};
        if (!CreateDirectoryW(*Path, &Attributes) && GetLastError() != ERROR_ALREADY_EXISTS) return false;
        FHandle Dir(CreateFileW(*Path, READ_CONTROL | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
        BY_HANDLE_FILE_INFORMATION Info;
        return Dir.Valid() && GetFileInformationByHandle(Dir.Value, &Info) && !(Info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
            && Inspect(Dir.Value, SE_FILE_OBJECT, FILE_ALL_ACCESS, Evidence);
    }
    bool WritePrivate(const FString& Path, const FString& Text, TSharedRef<FJsonObject>& Evidence)
    {
        SECURITY_ATTRIBUTES Attributes = {sizeof(SECURITY_ATTRIBUTES), FileSd, 0};
        FHandle File(CreateFileW(*Path, FILE_WRITE_DATA | READ_CONTROL, 0, &Attributes, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
        if (!File.Valid() || !Inspect(File.Value, SE_FILE_OBJECT, FILE_ALL_ACCESS, Evidence)) return false;
        FTCHARToUTF8 Bytes(*Text); DWORD Written = 0;
        return WriteFile(File.Value, Bytes.Get(), Bytes.Length(), &Written, nullptr) && Written == static_cast<DWORD>(Bytes.Length());
    }
    bool Peer(HANDLE Pipe) const
    {
        ULONG Pid = 0; if (!GetNamedPipeClientProcessId(Pipe, &Pid) || !Pid) return false;
        FHandle Process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, Pid)); FHandle Token;
        if (!Process.Valid() || !OpenProcessToken(Process.Value, TOKEN_QUERY, &Token.Value)) return false;
        TArray<uint8> UserInfo, Groups;
        if (!TokenBytes(Token.Value, TokenUser, UserInfo) || !TokenBytes(Token.Value, TokenGroups, Groups)
            || !EqualSid(reinterpret_cast<TOKEN_USER*>(UserInfo.GetData())->User.Sid, const_cast<uint8*>(User.GetData()))) return false;
        const auto* Values = reinterpret_cast<TOKEN_GROUPS*>(Groups.GetData());
        for (DWORD I = 0; I < Values->GroupCount; ++I)
            if ((Values->Groups[I].Attributes & SE_GROUP_LOGON_ID) == SE_GROUP_LOGON_ID
                && EqualSid(Values->Groups[I].Sid, const_cast<uint8*>(Logon.GetData()))) return true;
        return false;
    }
};
}

struct FNodeForgeEditorControlChannel::FImpl
{
    using FHandle = NodeForgeControlWindows::FHandle;
    struct FWorker : FRunnable
    {
        FImpl& Owner; FHandle Pipe, IoEvent, Done, ClientStop{CreateEventW(nullptr,1,0,nullptr)};
        FString ClientId,ClientName,ClientVersion; // Protected by Owner.ClientLock.
        TUniquePtr<FRunnableThread> Thread; OVERLAPPED Overlap = {};
        TArray<uint8> IoBuffer; // Also survives a failed cancellation/stop observation.
        bool bPending = false;
        std::atomic<uint32> CancelRequested{0}, CancelCompleted{0}, ConnectPending{0}, ReadPending{0}, WritePending{0};
        std::atomic<uint32> CurrentIo{0}, LastCancelledKind{0}, LastCancelledCode{0};
        explicit FWorker(FImpl& In) : Owner(In), IoEvent(CreateEventW(nullptr, 1, 0, nullptr)), Done(CreateEventW(nullptr, 1, 0, nullptr)) {}
        ~FWorker() override { check(!bPending); }
        bool WaitIo(DWORD& Count, double Deadline)
        {
            HANDLE Events[] = {Owner.StopEvent.Value, ClientStop.Value, IoEvent.Value};
            const double Left = Deadline - FPlatformTime::Seconds();
            DWORD Wait = WaitForMultipleObjects(3, Events, 0, Left <= 0 ? 0 : static_cast<DWORD>(FMath::Min(Left * 1000, double(MAX_uint32 - 1))));
            if (Wait != WAIT_OBJECT_0 + 2)
            {
                CancelIoEx(Pipe.Value, &Overlap); ++CancelRequested;
                // Cancellation request is not completion. Keep all storage until the actual event completes.
                DWORD Complete = WaitForSingleObject(IoEvent.Value, static_cast<DWORD>(NodeForgeEditorControl::StopSeconds * 1000));
                if (Complete != WAIT_OBJECT_0)
                {
                    Owner.bIoCompletionUnknown.store(true);
                    // Worker exits without freeing member storage; Stop retains the entire instance.
                    return false;
                }
                BOOL Result = GetOverlappedResult(Pipe.Value, &Overlap, &Count, 0);
                const DWORD Error = Result ? ERROR_SUCCESS : GetLastError();
                if (Error == ERROR_IO_INCOMPLETE) { Owner.bIoCompletionUnknown.store(true); return false; }
                LastCancelledKind.store(CurrentIo.load()); LastCancelledCode.store(Error);
                bPending = false; CurrentIo.store(0); ++CancelCompleted;
                Owner.LastIoCode.store(Error); return false;
            }
            BOOL Result = GetOverlappedResult(Pipe.Value, &Overlap, &Count, 0);
            const DWORD Error = Result ? ERROR_SUCCESS : GetLastError(); Owner.LastIoCode.store(Error);
            if (Error == ERROR_IO_INCOMPLETE) { Owner.bIoCompletionUnknown.store(true); return false; }
            bPending = false; CurrentIo.store(0); return Result != 0;
        }
        void BeginIo() { check(!bPending); ResetEvent(IoEvent.Value); Overlap = {}; Overlap.hEvent = IoEvent.Value; }
        bool Transfer(bool Write, uint8* Data, DWORD Size, DWORD& Count, double Deadline)
        {
            if (FPlatformTime::Seconds() >= Deadline || WaitForSingleObject(Owner.StopEvent.Value, 0) == WAIT_OBJECT_0
                ||WaitForSingleObject(ClientStop.Value,0)==WAIT_OBJECT_0) return false;
            IoBuffer.SetNumUninitialized(Size); if (Write) FMemory::Memcpy(IoBuffer.GetData(), Data, Size);
            BeginIo(); Count = 0;
            const BOOL Ok = Write ? WriteFile(Pipe.Value, IoBuffer.GetData(), Size, &Count, &Overlap) : ReadFile(Pipe.Value, IoBuffer.GetData(), Size, &Count, &Overlap);
            if (Ok) { if (!Write && Count) FMemory::Memcpy(Data, IoBuffer.GetData(), Count); return Count > 0; }
            if (GetLastError() != ERROR_IO_PENDING) return false;
            bPending = true; CurrentIo.store(Write ? 3 : 2); if (Write) ++WritePending; else ++ReadPending;
            const bool Complete = WaitIo(Count, Deadline) && Count > 0;
            if (Complete && !Write) FMemory::Memcpy(Data, IoBuffer.GetData(), Count);
            return Complete;
        }
        bool Send(const TSharedRef<FJsonObject>& Response, double Deadline)
        {
            TArray<uint8> Frame; if (!NodeForgeEditorControl::EncodeFrame(Response, Frame)) return false;
            DWORD Offset = 0;
            while (Offset < static_cast<DWORD>(Frame.Num()))
            { DWORD Count = 0; if (!Transfer(true, Frame.GetData() + Offset, Frame.Num() - Offset, Count, Deadline)) return false; Offset += Count; }
            return true;
        }
        void ObserveClientClose(double Deadline)
        {
            // DisconnectNamedPipe discards unread output. After a final error reply, allow the
            // peer to consume it and close; one bounded OVERLAPPED read, no blocking Flush.
            uint8 Ignored = 0; DWORD Count = 0;
            Transfer(false, &Ignored, 1, Count, Deadline); // Never parsed or dispatched.
        }
        bool Receive(NodeForgeEditorControl::FFrameReader& Frame, double FirstByteDeadline, double HardDeadline)
        {
            TArray<uint8> Chunk; bool First = true; double Deadline = FirstByteDeadline;
            while (Frame.State() == NodeForgeEditorControl::EFrameState::NeedMore)
            {
                Chunk.SetNumUninitialized(FMath::Min(Frame.RemainingBytes(), 65536)); DWORD Count = 0;
                if (!Transfer(false, Chunk.GetData(), Chunk.Num(), Count, Deadline)) return false;
                if (First) { First = false; Deadline = FMath::Min(FPlatformTime::Seconds() + NodeForgeEditorControl::FrameSeconds, HardDeadline); Frame.RestrictDeadline(Deadline); }
                if (Frame.Feed(MakeArrayView(Chunk.GetData(), static_cast<int32>(Count)), FPlatformTime::Seconds()) == NodeForgeEditorControl::EFrameState::Rejected) return false;
            }
            return true;
        }
        uint32 Run() override
        {
            using namespace NodeForgeEditorControl;
            while (WaitForSingleObject(Owner.StopEvent.Value, 0) != WAIT_OBJECT_0)
            {
                BeginIo(); BOOL Connected = ConnectNamedPipe(Pipe.Value, &Overlap); DWORD Code = Connected ? ERROR_SUCCESS : GetLastError();
                if (!Connected && Code == ERROR_IO_PENDING) { bPending = true; CurrentIo.store(1); ++ConnectPending; DWORD Count = 0; Connected = WaitIo(Count, FPlatformTime::Seconds() + IdleSeconds); }
                else if (Code == ERROR_PIPE_CONNECTED) Connected = 1;
                if (!Connected) { if (bPending) break; DisconnectNamedPipe(Pipe.Value); continue; }
                if (!Owner.Security.Peer(Pipe.Value)) { DisconnectNamedPipe(Pipe.Value); continue; }
                bool Authenticated = false; const double HelloDeadline = FPlatformTime::Seconds() + HelloSeconds;
                while (WaitForSingleObject(Owner.StopEvent.Value, 0) != WAIT_OBJECT_0)
                {
                    const double Now = FPlatformTime::Seconds();
                    const double Hard = Authenticated ? Now + IdleSeconds + FrameSeconds : HelloDeadline;
                    FFrameReader Frame(Hard);
                    if (!Receive(Frame, Authenticated ? Now + IdleSeconds : HelloDeadline, Hard)) break;
                    FRequest Request; FProtocolError Error;
                    const bool Parsed = ParseRequest(Frame.Body(), Owner.EnabledFeatures, Request, Error);
                    if (!Parsed)
                    {
                        if (!Authenticated) { Error.RpcCode = -32010; Error.Code = TEXT("sessionMismatch"); Error.bClose = true; }
                        const double ErrorDeadline = Authenticated ? FPlatformTime::Seconds() + FrameSeconds : HelloDeadline;
                        const bool Sent = Error.bReply && Send(Failure(Error), ErrorDeadline);
                        if (Error.bClose) { if (Sent) ObserveClientClose(ErrorDeadline); break; }
                        if (!Error.bReply || !Sent) break;
                        continue;
                    }
                    TSharedRef<FJsonObject> Response = MakeShared<FJsonObject>();
                    const double ReplyDeadline = Authenticated ? FPlatformTime::Seconds() + FrameSeconds : HelloDeadline;
                    if (!Authenticated)
                    {
                        if (!Owner.Hello(Request))
                        { Error.RpcCode = -32010; Error.Code = TEXT("sessionMismatch"); if (Send(Failure(Error), HelloDeadline)) ObserveClientClose(HelloDeadline); break; }
                        Authenticated = true;
                        if(Request.Arguments->HasField(TEXT("clientId")))
                        {
                            FScopeLock Lock(&Owner.ClientLock);
                            ClientId=Request.Arguments->GetStringField(TEXT("clientId"));
                            ClientName=Request.Arguments->GetStringField(TEXT("clientName"));
                            ClientVersion=Request.Arguments->GetStringField(TEXT("clientVersion"));
                        }
                        TSharedPtr<FJsonObject> LocalIdentity;
                        if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Owner.IdentityCache), LocalIdentity) || !LocalIdentity) break;
                        Response = LocalIdentity.ToSharedRef();
                    }
                    else if (Request.Method == TEXT("session.hello") || Request.Context->GetStringField(TEXT("controlSessionId")) != Owner.Session)
                    { Error.RpcCode = -32010; Error.Code = TEXT("sessionMismatch"); if (!Send(Failure(Error), FPlatformTime::Seconds() + FrameSeconds)) break; continue; }
                    else if (Request.Method == TEXT("operations.describe"))
                    {
                        TSharedPtr<FJsonObject> LocalDescription;
                        if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Owner.DescriptionCache), LocalDescription) || !LocalDescription) break;
                        Response = LocalDescription.ToSharedRef();
                    }
                    else if (Request.Method == TEXT("control.disable"))
                    {
                        ++Owner.ManagementReplies;
                        Owner.bStopping.store(true); Owner.bStopRequested.store(true);
                        Response->SetStringField(TEXT("state"), TEXT("stoppingRequested"));
                        Response->SetStringField(TEXT("controlSessionId"), Owner.Session);
                        // The GT stop pump cannot cancel this response while it is being sent.
                        const bool Sent = Send(Result(Request.Id, Response), ReplyDeadline);
                        if (Sent) ObserveClientClose(ReplyDeadline);
                        --Owner.ManagementReplies; break;
                    }
                    else if (Request.Method == TEXT("calls.releaseCompleted"))
                    {
                        TSharedPtr<FJsonObject> Receipt;
                        if (!Owner.ReleaseCompleted(Request, Receipt, Error))
                        { if (!Send(Failure(Error), ReplyDeadline)) break; continue; }
                        Response = Receipt.ToSharedRef();
                    }
                    else if (Request.Definition->Dispatch == NodeForgeControlOperations::EDispatch::GameThread
                        || Request.Method == TEXT("calls.get"))
                    {
                        TSharedPtr<FJsonObject> Call;
                        if (!Owner.Call(Request, Call, Error))
                        { if (!Send(Failure(Error), FPlatformTime::Seconds() + FrameSeconds)) break; continue; }
                        Response = Call.ToSharedRef();
                    }
                    else
                    {
                        FString Cached;
                        { FScopeLock Lock(&Owner.CacheLock); Cached = Request.Method == TEXT("draft.get") ? Owner.DraftCache : Owner.StateCache; }
                        TSharedPtr<FJsonObject> CachedObject;
                        if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Cached), CachedObject) || !CachedObject) break;
                        Response = CachedObject.ToSharedRef();
                        if (Request.Method == TEXT("editor.getState"))
                        {
                            TSharedPtr<FJsonObject> LocalIdentity;
                            if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Owner.IdentityCache), LocalIdentity) || !LocalIdentity) break;
                            Response->SetObjectField(TEXT("identity"), LocalIdentity.ToSharedRef());
                            Response->SetStringField(TEXT("state"), Owner.bStopping.load() ? TEXT("stopping") : Owner.Gate->IsBusy() ? TEXT("busy") : TEXT("idle"));
                            FScopeLock Lock(&Owner.RegistryLock);
                            Response->SetObjectField(TEXT("callCapacity"), Owner.Capacity());
                            Response->SetStringField(TEXT("activeCallId"), Owner.ActiveCall.IsEmpty() ? Owner.PendingCall : Owner.ActiveCall);
                            Response->SetStringField(TEXT("lastPublishedRevision"), Response->GetStringField(TEXT("draftRevision")));
                        }
                    }
                    if (!Send(Result(Request.Id, Response), ReplyDeadline)) break;
                }
                {FScopeLock Lock(&Owner.ClientLock);ClientId.Empty();ClientName.Empty();ClientVersion.Empty();}
                if (bPending) break;
                DisconnectNamedPipe(Pipe.Value);
                ResetEvent(ClientStop.Value);
            }
            SetEvent(Done.Value); return 0;
        }
    };
    NodeForgeControlWindows::FSecurity Security;
    TSharedPtr<FNodeForgeEditorBusinessGate> Gate;
    struct FCall
    {
        // Only value strings/scalars cross the worker -> GT boundary. FJsonObject uses
        // non-thread-safe reference counts, so retain no I/O request DOM here.
        FString Id, Method, Arguments, Fingerprint, Cache;
        uint64 ExpectedRevision = 0;
        bool bPublishing = true;
    };
    FCriticalSection RegistryLock; TMap<FString, FCall> Calls;
    FCriticalSection ClientLock;
    static constexpr int32 AutomaticCleanupBatch = 50;
    int32 AutomaticCleanupBatches = 0;
    FString AutomaticCleanupState = TEXT("idle"), AutomaticCleanupCallId, AutomaticCleanupCode;
    FString PendingCall, ActiveCall;
    std::atomic<bool> bStopping{false}, bStopRequested{false};
    std::atomic<uint32> ManagementReplies{0};
    FHandle StopEvent{CreateEventW(nullptr, 1, 0, nullptr)};
    TArray<TUniquePtr<FWorker>> Workers;
    FString Session, Credential, PipeName, SessionDirectory, PublicDirectory, DescriptorPath, DiscoveryPath;
    TSharedPtr<FJsonObject> Identity, Description, SecurityEvidence;
    FCriticalSection CacheLock; FString StateCache, DraftCache, IdentityCache, DescriptionCache;
    uint8 EnabledFeatures = static_cast<uint8>(NodeForgeControlOperations::EEditorFeature::Read)
        | static_cast<uint8>(NodeForgeControlOperations::EEditorFeature::Draft) | static_cast<uint8>(NodeForgeControlOperations::EEditorFeature::Stop)
        | static_cast<uint8>(NodeForgeControlOperations::EEditorFeature::Graph);
    std::atomic<bool> bIoCompletionUnknown{false}; std::atomic<uint32> LastIoCode{0};
    bool bStarted = false, bStopped = false, bTransportStopped = false, bStopSignaled = false;
    bool bLiveCodingStartupPending = false;
    double StopDeadline = 0;
    bool bDraining = false; // Explicit GT non-reentry, including nested native message dispatch.
#if WITH_DEV_AUTOMATION_TESTS
    TFunction<void()> BeforeDrainProbe;
#endif
    bool Hello(const NodeForgeEditorControl::FRequest& Request) const
    {
        if (Request.Method != TEXT("session.hello")) return false; const auto& Params = Request.Arguments;
        return Params->GetStringField(TEXT("controlSessionId")) == Session && Params->GetStringField(TEXT("credential")) == Credential
            && Params->GetStringField(TEXT("expectedProject")).Equals(Identity->GetStringField(TEXT("project")), ESearchCase::IgnoreCase)
            && Params->GetNumberField(TEXT("expectedPid")) == Identity->GetNumberField(TEXT("pid"))
            && Params->GetStringField(TEXT("expectedProcessStart")) == Identity->GetStringField(TEXT("processStart"))
            && Params->GetStringField(TEXT("expectedBinarySha256")) == Identity->GetStringField(TEXT("binarySha256"));
    }
    TSharedRef<FJsonObject> Record(const NodeForgeEditorControl::FRequest& Request, const FString& Hash, const TCHAR* State) const
    {
        auto Json = MakeShared<FJsonObject>(); Json->SetStringField(TEXT("callId"), Request.Id);
        Json->SetStringField(TEXT("controlSessionId"), Session); Json->SetStringField(TEXT("operation"), Request.Method);
        Json->SetStringField(TEXT("fingerprint"), Hash); Json->SetStringField(TEXT("state"), State);
        Json->SetStringField(TEXT("expectedDraftRevision"), LexToString(Request.ExpectedRevision));
        Json->SetStringField(TEXT("recordedAtUtc"), FDateTime::UtcNow().ToIso8601());
        if (Request.Method == TEXT("draft.loadFile")) Json->SetStringField(TEXT("sourcePath"), Request.Arguments->GetStringField(TEXT("path")));
        if (Request.Method == TEXT("draft.replace")) Json->SetStringField(TEXT("textDigest"), NodeForgePrototype::TextSha256(Request.Arguments->GetStringField(TEXT("text"))));
        return Json; // No credential, entire draft, or arbitrary client report fields.
    }
    // RegistryLock protects every caller of these helpers. No UObject or game-thread work.
    bool Reclaimable(const FCall& Call) const
    {
        if (Call.bPublishing) return false;
        TSharedPtr<FJsonObject> Row;
        if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Call.Cache), Row) || !Row) return false;
        FString State; double Code = 5; bool Review = false;
        Row->TryGetStringField(TEXT("state"), State); Row->TryGetNumberField(TEXT("cliExitCode"), Code);
        Row->TryGetBoolField(TEXT("requiresReview"), Review);
        return !Review && ((State == TEXT("completed") && Code == 0)
            || ((State == TEXT("rejected") || State == TEXT("cancelled")) && Code == 2));
    }
    TSharedRef<FJsonObject> Capacity() const
    {
        auto Value = MakeShared<FJsonObject>(); int32 Eligible = 0;
        for (const auto& Pair : Calls) if (Reclaimable(Pair.Value)) ++Eligible;
        Value->SetNumberField(TEXT("retained"), Calls.Num());
        Value->SetNumberField(TEXT("available"), NodeForgeEditorControl::MaxBusinessCalls - Calls.Num());
        Value->SetNumberField(TEXT("capacity"), NodeForgeEditorControl::MaxBusinessCalls);
        Value->SetNumberField(TEXT("reclaimable"), Eligible);
        auto Automatic = MakeShared<FJsonObject>(); Automatic->SetBoolField(TEXT("enabled"), true);
        Automatic->SetNumberField(TEXT("threshold"), AutomaticCleanupBatch); Automatic->SetNumberField(TEXT("batchSize"), AutomaticCleanupBatch);
        Automatic->SetNumberField(TEXT("completedBatches"), AutomaticCleanupBatches);
        Automatic->SetStringField(TEXT("status"), AutomaticCleanupState); Automatic->SetStringField(TEXT("lastCallId"), AutomaticCleanupCallId);
        Automatic->SetStringField(TEXT("code"), AutomaticCleanupCode); Value->SetObjectField(TEXT("autoCleanup"), Automatic);
        return Value;
    }
    bool ReadHistory(const FString& Id, TSharedPtr<FJsonObject>& Row, bool& Exists) const
    {
        using namespace NodeForgeControlWindows;
        const FString Base = PublicDirectory / TEXT("Calls") / Id;
        const FString ArchivePath = Base + TEXT("Archived.json");
        const FString Maintenance = Base + (IFileManager::Get().FileExists(*(Base + TEXT("MaintenanceFinished.json"))) ? TEXT("MaintenanceFinished.json") : TEXT("MaintenanceAccepted.json"));
        Exists = IFileManager::Get().FileExists(*ArchivePath) || IFileManager::Get().FileExists(*Maintenance)
            || IFileManager::Get().FileExists(*(Base + TEXT("Accepted.json"))) || IFileManager::Get().FileExists(*(Base + TEXT("Finished.json")));
        if (!Exists) return true;
        TSharedPtr<FJsonObject> Marker;
        if (IFileManager::Get().FileExists(*Maintenance))
        {
            if (!SafePath(Maintenance, PublicDirectory) || !ReadObject(Maintenance, Row, NodeForgeEditorControl::MaxReportBytes)) return false;
        }
        else
        {
            const FString Finished = Base + TEXT("Finished.json"), Accepted = Base + TEXT("Accepted.json");
            if (!SafePath(ArchivePath, PublicDirectory) || !SafePath(Finished, PublicDirectory) || !SafePath(Accepted, PublicDirectory)
                || !ReadObject(ArchivePath, Marker, 4096)
                || Marker->GetStringField(TEXT("controlSessionId")) != Session || Marker->GetStringField(TEXT("callId")) != Id
                || NodeForgePrototype::FileSha256(Finished) != Marker->GetStringField(TEXT("finishedSha256"))
                || NodeForgePrototype::FileSha256(Accepted) != Marker->GetStringField(TEXT("acceptedSha256"))
                || !ReadObject(Finished, Row, NodeForgeEditorControl::MaxReportBytes)) return false;
        }
        return Row && Row->GetStringField(TEXT("controlSessionId")) == Session && Row->GetStringField(TEXT("callId")) == Id
            && Row->HasTypedField<EJson::String>(TEXT("fingerprint"));
    }
    bool ReleaseCompleted(const NodeForgeEditorControl::FRequest& Request, TSharedPtr<FJsonObject>& Response, NodeForgeEditorControl::FProtocolError& Error)
    {
        using namespace NodeForgeControlWindows;
        auto Reject = [&](int32 Rpc, const TCHAR* Code) { Error.RpcCode = Rpc; Error.Code = Code; return false; };
        const FString Hash = NodeForgeEditorControl::Fingerprint(Request);
        FScopeLock Lock(&RegistryLock);
        if (const FCall* Existing = Calls.Find(Request.Id))
        {
            if (Existing->Fingerprint != Hash) return Reject(-32013, TEXT("callConflict"));
            return Reject(-32015, TEXT("registrationPending"));
        }
        bool Exists = false;
        if (!ReadHistory(Request.Id, Response, Exists)) return Reject(-32603, TEXT("callArchiveInvalid"));
        if (Exists) return Response->GetStringField(TEXT("fingerprint")) == Hash ? true : Reject(-32013, TEXT("callConflict"));
        if (bStopping.load()) return Reject(-32017, TEXT("stopping"));
        if (!PendingCall.IsEmpty() || !ActiveCall.IsEmpty() || Gate->IsBusy()) return Reject(-32011, TEXT("busy"));
        TArray<FString> Ids;
        for (const auto& Pair : Calls)
        {
            // Refuse the whole cleanup if any native operation still needs reconciliation.
            if (!Reclaimable(Pair.Value)) return Reject(-32015, TEXT("unresolvedCalls"));
            Ids.Add(Pair.Key);
        }
        Ids.Sort(); Ids.SetNum(FMath::Min(Ids.Num(), static_cast<int32>(Request.Arguments->GetNumberField(TEXT("maxCount")))));
        if (!Gate->ReserveControl()) return Reject(-32011, TEXT("busy"));
        // Journal intent first. A lost response is queried using this same callId, never replayed.
        const FString ReceiptPath = PublicDirectory / TEXT("Calls") / (Request.Id + TEXT("MaintenanceAccepted.json"));
        Response = Record(Request, Hash, TEXT("unknown")); Response->SetNumberField(TEXT("cliExitCode"), 5);
        Response->SetStringField(TEXT("code"), TEXT("cleanupIncomplete"));
        auto Failed = [&]() { bStopping.store(true); Gate->EndControl(); return Reject(-32603, TEXT("cleanupRegistrationFailed")); };
        if (!SafePath(ReceiptPath, PublicDirectory) || !NodeForgePrototype::WriteJson(Response.ToSharedRef(), ReceiptPath)) return Failed();
        TArray<TSharedPtr<FJsonValue>> Archived;
        for (const FString& Id : Ids)
        {
            const FCall& Call = Calls.FindChecked(Id);
            const FString Base = PublicDirectory / TEXT("Calls") / Id;
            const FString Finished = Base + TEXT("Finished.json"), Accepted = Base + TEXT("Accepted.json"), Archive = Base + TEXT("Archived.json");
            TSharedPtr<FJsonObject> Final, Initial, Cached;
            if (!SafePath(Finished, PublicDirectory) || !SafePath(Accepted, PublicDirectory) || !SafePath(Archive, PublicDirectory)
                || !ReadObject(Finished, Final, NodeForgeEditorControl::MaxReportBytes) || !ReadObject(Accepted, Initial, NodeForgeEditorControl::MaxReportBytes)
                || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Call.Cache), Cached) || !Cached
                || NodeForgeEditorControl::SemanticDigest(Final.ToSharedRef()) != NodeForgeEditorControl::SemanticDigest(Cached.ToSharedRef())
                || Initial->GetStringField(TEXT("fingerprint")) != Call.Fingerprint
                || Initial->GetStringField(TEXT("controlSessionId")) != Session || Initial->GetStringField(TEXT("callId")) != Id) return Failed();
            auto Marker = MakeShared<FJsonObject>(); Marker->SetStringField(TEXT("controlSessionId"), Session); Marker->SetStringField(TEXT("callId"), Id);
            Marker->SetStringField(TEXT("finishedSha256"), NodeForgePrototype::FileSha256(Finished));
            Marker->SetStringField(TEXT("acceptedSha256"), NodeForgePrototype::FileSha256(Accepted));
            if (!NodeForgePrototype::WriteJson(Marker, Archive)) return Failed();
            Archived.Add(MakeShared<FJsonValueString>(Id));
        }
        auto Payload = MakeShared<FJsonObject>(); Payload->SetStringField(TEXT("state"), TEXT("completed")); Payload->SetNumberField(TEXT("cliExitCode"), 0);
        Payload->SetNumberField(TEXT("released"), Ids.Num()); Payload->SetNumberField(TEXT("retained"), Calls.Num() - Ids.Num());
        Payload->SetNumberField(TEXT("available"), NodeForgeEditorControl::MaxBusinessCalls - Calls.Num() + Ids.Num());
        Payload->SetNumberField(TEXT("capacity"), NodeForgeEditorControl::MaxBusinessCalls); Payload->SetBoolField(TEXT("recordsPreserved"), true);
        Payload->SetArrayField(TEXT("archivedCallIds"), Archived);
        auto Finished = Record(Request, Hash, TEXT("completed")); Finished->SetNumberField(TEXT("cliExitCode"), 0); Finished->SetObjectField(TEXT("result"), Payload);
        if (!NodeForgePrototype::WriteJson(Finished, PublicDirectory / TEXT("Calls") / (Request.Id + TEXT("MaintenanceFinished.json")))) return Failed();
        // No fallible work after the committed receipt. Registry lock prevents observing intermediate counts.
        for (const FString& Id : Ids) Calls.Remove(Id);
        Calls.Compact(); Calls.Shrink();
        Response = Finished; Gate->EndControl(); return true;
    }

    bool Call(const NodeForgeEditorControl::FRequest& Request, TSharedPtr<FJsonObject>& Response, NodeForgeEditorControl::FProtocolError& Error)
    {
        auto Reject = [&](int32 Rpc, const TCHAR* Code) { Error.RpcCode = Rpc; Error.Code = Code; return false; };
        if (Request.Method == TEXT("calls.get"))
        {
            FString Cached;
            { FScopeLock Lock(&RegistryLock); const FCall* Found = Calls.Find(Request.Arguments->GetStringField(TEXT("callId")));
                if (!Found) { bool Exists = false; if (!ReadHistory(Request.Arguments->GetStringField(TEXT("callId")), Response, Exists)) return Reject(-32603, TEXT("callArchiveInvalid"));
                    return Exists ? true : Reject(-32018, TEXT("unknownCall")); }
                if (Found->bPublishing) return Reject(-32015, TEXT("registrationPending")); Cached = Found->Cache; }
            return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Cached), Response) && Response.IsValid();
        }
        const FString Hash = NodeForgeEditorControl::Fingerprint(Request); FString OldCache;
        {
            FScopeLock Lock(&RegistryLock);
            if (const FCall* Found = Calls.Find(Request.Id))
            {
                if (Found->Fingerprint != Hash) return Reject(-32013, TEXT("callConflict"));
                if (Found->bPublishing) return Reject(-32015, TEXT("registrationPending"));
                OldCache = Found->Cache;
            }
            else
            {
                bool Exists = false;
                if (!ReadHistory(Request.Id, Response, Exists)) return Reject(-32603, TEXT("callArchiveInvalid"));
                if (Exists) return Response->GetStringField(TEXT("fingerprint")) == Hash ? true : Reject(-32013, TEXT("callConflict"));
                if (bStopping.load()) return Reject(-32017, TEXT("stopping"));
                if (Calls.Num() >= NodeForgeEditorControl::MaxBusinessCalls) return Reject(-32014, TEXT("callCapacity"));
                if (!Gate->ReserveControl()) return Reject(-32011, TEXT("busy"));
                FCall New; New.Id = Request.Id; New.Method = Request.Method; New.ExpectedRevision = Request.ExpectedRevision;
                New.Arguments = NodeForgeControlWindows::JsonText(Request.Arguments.ToSharedRef());
                New.Fingerprint = Hash; Calls.Add(Request.Id, MoveTemp(New));
            }
        }
        if (!OldCache.IsEmpty()) return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(OldCache), Response) && Response.IsValid();
        auto Accepted = Record(Request, Hash, TEXT("accepted"));
        // The journal precedes the queue. File I/O is outside the registry lock.
        const bool Saved = NodeForgePrototype::WriteJson(Accepted, PublicDirectory / TEXT("Calls") / (Request.Id + TEXT("Accepted.json")));
        auto Queued = Record(Request, Hash, Saved ? TEXT("queued") : TEXT("unknown"));
        if (!Saved) Queued->SetStringField(TEXT("code"), TEXT("registrationFailed"));
        const FString Cached = NodeForgeControlWindows::JsonText(Queued);
        { FScopeLock Lock(&RegistryLock); FCall& Call = Calls.FindChecked(Request.Id); Call.bPublishing = false; Call.Cache = Cached;
            if (Saved) PendingCall = Request.Id; }
        if (!Saved) { bStopping.store(true); Gate->EndControl(); } // Kept ID/unknown record; no execution or later automatic writes.
        Response = Queued; return true;
    }
    void ReleaseCompletedAutomatically()
    {
        // Called only on the existing GT completion/idle path. No timer, caller counter or second cleanup implementation.
        {
            FScopeLock Lock(&RegistryLock);
            if (Calls.Num() < AutomaticCleanupBatch) return;
            if (bStopping.load()) { if (AutomaticCleanupState != TEXT("unknown")) AutomaticCleanupState = TEXT("blockedStopping"); return; }
            AutomaticCleanupCallId.Empty(); AutomaticCleanupCode.Empty();
            if (!PendingCall.IsEmpty() || !ActiveCall.IsEmpty() || Gate->IsBusy())
            { AutomaticCleanupState = TEXT("deferredBusy"); return; }
            for (const auto& Pair : Calls) if (!Reclaimable(Pair.Value))
            { AutomaticCleanupState = TEXT("blockedUnresolved"); return; }
        }
        NodeForgeEditorControl::FRequest Maintenance;
        Maintenance.Id = TEXT("AutoCleanup-") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
        Maintenance.Method = TEXT("calls.releaseCompleted");
        Maintenance.Definition = NodeForgeControlOperations::Find(Maintenance.Method, NodeForgeControlOperations::EHost::ActiveEditor);
        Maintenance.Context = MakeShared<FJsonObject>(); Maintenance.Context->SetStringField(TEXT("controlSessionId"), Session);
        Maintenance.Arguments = MakeShared<FJsonObject>(); Maintenance.Arguments->SetNumberField(TEXT("maxCount"), AutomaticCleanupBatch);
        TSharedPtr<FJsonObject> Receipt; NodeForgeEditorControl::FProtocolError Error;
        const bool Released = ReleaseCompleted(Maintenance, Receipt, Error);
        FScopeLock Lock(&RegistryLock);
        AutomaticCleanupCallId = Receipt.IsValid() ? Maintenance.Id : FString();
        if (Released) { AutomaticCleanupState = TEXT("completed"); ++AutomaticCleanupBatches; }
        else
        {
            AutomaticCleanupCode = Error.Code;
            AutomaticCleanupState = bStopping.load() ? TEXT("unknown")
                : Error.Code == TEXT("busy") ? TEXT("deferredBusy") : TEXT("blockedUnresolved");
        }
    }

    void FinishCall(const NodeForgeEditorControl::FRequest& Request, const FString& Hash, const TSharedRef<FJsonObject>& Result)
    {
        if (Result->GetNumberField(TEXT("cliExitCode")) == 5) bStopping.store(true); // Unknown writes require read-only reconciliation, never another automatic business call.
        auto Finished = Record(Request, Hash, *Result->GetStringField(TEXT("state"))); Finished->SetObjectField(TEXT("result"), Result);
        Finished->SetNumberField(TEXT("cliExitCode"), Result->GetNumberField(TEXT("cliExitCode")));
        if (!NodeForgePrototype::WriteJson(Finished, PublicDirectory / TEXT("Calls") / (Request.Id + TEXT("Finished.json"))))
        { Finished->SetStringField(TEXT("state"), TEXT("unknown")); Finished->SetStringField(TEXT("code"), TEXT("finishRegistrationFailed")); Finished->SetNumberField(TEXT("cliExitCode"), 5); bStopping.store(true); }
        const FString Cached = NodeForgeControlWindows::JsonText(Finished);
        { FScopeLock Lock(&RegistryLock); Calls.FindChecked(Request.Id).Cache = Cached; ActiveCall.Empty(); }
        Gate->EndControl();
        ReleaseCompletedAutomatically();
    }
};

FNodeForgeEditorControlChannel::FNodeForgeEditorControlChannel() : Impl(MakeUnique<FImpl>()) {}
FNodeForgeEditorControlChannel::~FNodeForgeEditorControlChannel()
{
    if (!Stop()) { UE_LOG(LogTemp, Error, TEXT("NodeForge control completion unknown; owner retained until process exit; dynamic unload unsupported")); (void)Impl.Release(); }
}
bool FNodeForgeEditorControlChannel::Start(FString& Error)
{
    check(IsInGameThread()); using namespace NodeForgeControlWindows; using namespace NodeForgeEditorControl;
    NodeForgeInstallation::FBinding Binding;
    if(!NodeForgeInstallation::Read(Binding,Error)) return false;
    Error = TEXT("controlEnvironmentRejected"); FString RegistrationPath;
    const FString Root = NodeForgeInstallation::StateDirectory(Binding), Project = Full(FPaths::GetProjectFilePath());
    if (Root.IsEmpty()) return false;
    const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("NodeForge"));
    if (!Impl->Gate || IsRunningCommandlet()
        || FEngineVersion::Current().GetMajor() != 5 || FEngineVersion::Current().GetMinor() != 3 || FEngineVersion::Current().GetPatch() != 2
        || !Plugin) return false;
    // Command-line and ordinary startup enter the same channel. The opt-in is
    // per user/installation; fresh project/session records are made here.
    TSharedPtr<FJsonObject> Ai; FString UserDirectory;
    const bool Automatic = !FParse::Param(FCommandLine::Get(), TEXT("NFControlEnable"));
    if (Automatic)
    {
        if (!NodeForgeInstallation::AiConfiguration(Binding, Ai, UserDirectory, Error)) return false;
        TSharedPtr<FJsonObject> Contract, Docs;
        if (!ReadObject(Binding.InstallationRoot / TEXT("Tools/NodeForgeMcp/ToolContract.json"), Contract, 1048576)) return false;
        const FString DocsPath = Full(Binding.InstallationRoot / Contract->GetStringField(TEXT("docsManifest")));
        if (!SafePath(DocsPath, Binding.InstallationRoot / TEXT("Docs/Versions"))
            || NodeForgePrototype::FileSha256(DocsPath) != Ai->GetStringField(TEXT("docsManifestSha256"))
            || !ReadObject(DocsPath, Docs, 1048576)) return false;
        const FString Namespace = Full(Root / Docs->GetStringField(TEXT("reportNamespace")));
        if (!SafePath(Namespace, Root / TEXT("Reports/PluginControlStageTwo"))) return false;
        const FString Out = Namespace / (TEXT("Editor") + FGuid::NewGuid().ToString(EGuidFormats::Digits));
        RegistrationPath = Out / TEXT("Registration.json");
        auto Registration = MakeShared<FJsonObject>(); Registration->SetStringField(TEXT("project"), Project);
        Registration->SetStringField(TEXT("pluginDirectory"), Binding.PluginDirectory);
        Registration->SetStringField(TEXT("binarySha256"), Binding.BinarySha256); Registration->SetStringField(TEXT("reportDirectory"), Out);
        auto Config = MakeShared<FJsonObject>(); Config->SetStringField(TEXT("format"), TEXT("NodeForgeMcpConfig")); Config->SetNumberField(TEXT("version"), 1);
        for (const TCHAR* Key : {TEXT("bundleId"), TEXT("binarySha256"), TEXT("toolContractSha256"), TEXT("docsManifestSha256")})
            Config->SetStringField(Key, Ai->GetStringField(Key));
        Config->SetStringField(TEXT("project"), Project); Config->SetStringField(TEXT("pluginDirectory"), Binding.PluginDirectory);
        Config->SetStringField(TEXT("sessionPublic"), Out / TEXT("SessionPublic.json")); Config->SetStringField(TEXT("reportRoot"), Out);
        Config->SetStringField(TEXT("bridgeRunRoot"), Out / TEXT("Bridge"));
        if (!SafePath(RegistrationPath, Root / TEXT("Reports/PluginControlStageTwo"))
            || !NodeForgePrototype::WriteJson(Registration, RegistrationPath) || !NodeForgePrototype::WriteJson(Config, Out / TEXT("Config.json"))) return false;
        auto Receipt = MakeShared<FJsonObject>(); Receipt->SetStringField(TEXT("format"), TEXT("NodeForgeEditorPreparationReceipt")); Receipt->SetNumberField(TEXT("version"), 1);
        Receipt->SetStringField(TEXT("status"), TEXT("passed")); Receipt->SetBoolField(TEXT("requiresReview"), false);
        Receipt->SetStringField(TEXT("bindingSha256"), NodeForgePrototype::FileSha256(FPaths::GetPath(Project) / TEXT("Config/NodeForgeInstallation.json")));
        Receipt->SetStringField(TEXT("binarySha256"), Binding.BinarySha256); Receipt->SetStringField(TEXT("origin"), TEXT("normalEditorStartup"));
        TArray<TSharedPtr<FJsonValue>> Written;
        for (const FString& Path : {RegistrationPath, Out / TEXT("Config.json")})
        { auto Row = MakeShared<FJsonObject>(); Row->SetStringField(TEXT("path"), Path); Row->SetStringField(TEXT("sha256"), NodeForgePrototype::FileSha256(Path)); Written.Add(MakeShared<FJsonValueObject>(Row)); }
        Receipt->SetArrayField(TEXT("completedWrites"), Written);
        if (!NodeForgePrototype::WriteJson(Receipt, Out / TEXT("PreparationReceipt.json"))) return false;
    }
    else if (!FParse::Value(FCommandLine::Get(), TEXT("NFControlRegistration="), RegistrationPath)) return false;
    if (!SafePath(RegistrationPath, Root / TEXT("Reports/PluginControlStageTwo"))) return false;
    // No user-supplied -NoLiveCoding is needed. Preserve immutable DLL identity
    // by disabling patches for this explicitly AI-enabled session and monitoring it.
    ILiveCodingModule& LiveCoding = FModuleManager::LoadModuleChecked<ILiveCodingModule>(TEXT("LiveCoding"));
    if (LiveCoding.IsCompiling()) { Error = TEXT("controlLiveCodingBusy"); return false; }
    // UE5.3.2 ignores disabling while its automatic startup is still pending.
    // Pin that startup once it completes, without changing the user's settings.
    Impl->bLiveCodingStartupPending = LiveCoding.IsEnabledByDefault() && !LiveCoding.HasStarted();
    LiveCoding.EnableForSession(false);
    if (LiveCoding.IsEnabledForSession()) { Error = TEXT("controlLiveCodingEnabled"); return false; }
    TSharedPtr<FJsonObject> Registration;
    if (!ReadObject(RegistrationPath, Registration, MaxDescriptorBytes) || Registration->Values.Num() != 4
        || !Registration->HasTypedField<EJson::String>(TEXT("project")) || !Registration->HasTypedField<EJson::String>(TEXT("pluginDirectory"))
        || !Registration->HasTypedField<EJson::String>(TEXT("binarySha256")) || !Registration->HasTypedField<EJson::String>(TEXT("reportDirectory"))) return false;
    const FString Binary = Full(FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"))), BinaryHash = NodeForgePrototype::FileSha256(Binary);
    if (Registration->GetStringField(TEXT("project")) != Project || Full(Registration->GetStringField(TEXT("pluginDirectory"))) != Full(Plugin->GetBaseDir())
        || Binary != Full(Plugin->GetBaseDir() / TEXT("Binaries/Win64/UnrealEditor-NodeForgeEditor.dll")) || BinaryHash.Len() != 64
        || Registration->GetStringField(TEXT("binarySha256")) != BinaryHash
        || !SafePath(Registration->GetStringField(TEXT("reportDirectory")), Root / TEXT("Reports/PluginControlStageTwo"))) return false;
    Impl->PublicDirectory = Full(Registration->GetStringField(TEXT("reportDirectory")));
    if (IFileManager::Get().FileExists(*(Impl->PublicDirectory / TEXT("SessionPublic.json"))) || !Impl->Security.Initialize() || !Impl->StopEvent.Valid()) return false;
    Impl->Session = FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower(); Impl->SessionDirectory = Root / TEXT("Reports/PluginControlSessions") / Impl->Session;
    Impl->DescriptorPath = Impl->SessionDirectory / TEXT("Session.json");
    Impl->SecurityEvidence = MakeShared<FJsonObject>(); auto Parent = MakeShared<FJsonObject>(), Folder = MakeShared<FJsonObject>();
    if (!SafePath(Impl->SessionDirectory, Root / TEXT("Reports"))) { Error = TEXT("controlSessionPathRejected"); return false; }
    if (!Impl->Security.Directory(Root / TEXT("Reports/PluginControlSessions"), Parent))
    { Error = FString::Printf(TEXT("controlSessionContainerRejected:%u"), GetLastError()); return false; }
    if (!Impl->Security.SessionDirectory(Impl->SessionDirectory, Folder))
    { Error = FString::Printf(TEXT("controlSessionDirectoryRejected:%u"), GetLastError()); return false; }
    Impl->SecurityEvidence->SetObjectField(TEXT("parentDirectory"), Parent); Impl->SecurityEvidence->SetObjectField(TEXT("sessionDirectory"), Folder);
    uint8 Secret[32]; if (BCryptGenRandom(nullptr, Secret, sizeof(Secret), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) return false;
    Impl->Credential = BytesToHex(Secret, sizeof(Secret)).ToLower(); SecureZeroMemory(Secret, sizeof(Secret));
    const DWORD Pid = GetCurrentProcessId(); FILETIME Created, Exit, Kernel, User;
    if (!GetProcessTimes(GetCurrentProcess(), &Created, &Exit, &Kernel, &User)) return false;
    const uint64 Ticks = (uint64(Created.dwHighDateTime) << 32) | Created.dwLowDateTime;
    Impl->PipeName = FString::Printf(TEXT("\\\\.\\pipe\\NodeForge-%u-%s"), Pid, *Impl->Session);
    Impl->Identity = MakeShared<FJsonObject>(); const auto& Identity = Impl->Identity;
    Identity->SetStringField(TEXT("editorApiVersion"), NodeForgeEditorControl::ApiVersion); Identity->SetStringField(TEXT("profile"), Profile);
    Identity->SetStringField(TEXT("controlSessionId"), Impl->Session); Identity->SetNumberField(TEXT("pid"), Pid);
    Identity->SetStringField(TEXT("processStart"), LexToString(Ticks)); Identity->SetStringField(TEXT("project"), Project);
    Identity->SetStringField(TEXT("module"), Binary); Identity->SetStringField(TEXT("binarySha256"), BinaryHash);
    Identity->SetStringField(TEXT("engine"), FEngineVersion::Current().ToString()); Identity->SetStringField(TEXT("pipe"), Impl->PipeName);
    Identity->SetStringField(TEXT("sessionFile"), Impl->DescriptorPath); Identity->SetStringField(TEXT("reportRoot"), Impl->PublicDirectory);
    Identity->SetObjectField(TEXT("limits"), Limits()); Impl->Description = NodeForgeControlOperations::DescribeEditor(Impl->EnabledFeatures);
    Impl->Description->SetObjectField(TEXT("identity"), Identity.ToSharedRef()); Impl->Description->SetObjectField(TEXT("limits"), Limits());
    Impl->IdentityCache = JsonText(Identity.ToSharedRef()); Impl->DescriptionCache = JsonText(Impl->Description.ToSharedRef());
    SECURITY_ATTRIBUTES Attributes = {sizeof(SECURITY_ATTRIBUTES), Impl->Security.PipeSd, 0};
    for (int32 I = 0; I < MaxConnections; ++I)
    {
        auto Worker = MakeUnique<FImpl::FWorker>(*Impl); if (!Worker->IoEvent.Valid() || !Worker->Done.Valid() || !Worker->ClientStop.Valid()) return false;
        Worker->Pipe.Value = CreateNamedPipeW(*Impl->PipeName, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | (I == 0 ? FILE_FLAG_FIRST_PIPE_INSTANCE : 0),
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS, MaxConnections, 65536, 65536, 0, &Attributes);
        auto PipeEvidence = MakeShared<FJsonObject>();
        if (!Worker->Pipe.Valid() || !Impl->Security.Inspect(Worker->Pipe.Value, SE_KERNEL_OBJECT, 0x0012019f, PipeEvidence)) return false;
        Impl->SecurityEvidence->SetObjectField(FString::Printf(TEXT("pipe%d"), I), PipeEvidence); Impl->Workers.Add(MoveTemp(Worker));
    }
    Impl->bStarted = true;
    for (int32 I = 0; I < Impl->Workers.Num(); ++I)
    { Impl->Workers[I]->Thread.Reset(FRunnableThread::Create(Impl->Workers[I].Get(), *FString::Printf(TEXT("NodeForgeControl%d"), I))); if (!Impl->Workers[I]->Thread) return false; }
    auto Descriptor = MakeShared<FJsonObject>(*Identity); Descriptor->SetStringField(TEXT("credential"), Impl->Credential);
    auto FileEvidence = MakeShared<FJsonObject>(); if (!Impl->Security.WritePrivate(Impl->DescriptorPath, JsonText(Descriptor), FileEvidence)) return false;
    Impl->SecurityEvidence->SetObjectField(TEXT("sessionFile"), FileEvidence);
    TSharedPtr<FJsonObject> Readback; if (!ReadObject(Impl->DescriptorPath, Readback, MaxDescriptorBytes) || JsonText(Readback.ToSharedRef()) != JsonText(Descriptor)) return false;
    if (!NodeForgePrototype::WriteJson(Identity.ToSharedRef(), Impl->PublicDirectory / TEXT("SessionPublic.json"))
        || !NodeForgePrototype::WriteJson(Impl->SecurityEvidence.ToSharedRef(), Impl->PublicDirectory / TEXT("Security.json"))) return false;
    if (Automatic)
    {
        Impl->DiscoveryPath = UserDirectory / TEXT("Sessions") / (Impl->Session + TEXT(".json"));
        if (!SafePath(Impl->DiscoveryPath, UserDirectory / TEXT("Sessions"))
            || !NodeForgePrototype::WriteJson(Identity.ToSharedRef(), Impl->DiscoveryPath)) return false;
    }
    Error.Empty(); return true;
}
void FNodeForgeEditorControlChannel::SetBusinessGate(const TSharedRef<FNodeForgeEditorBusinessGate>& Gate)
{ check(IsInGameThread()); check(!Impl->bStarted); Impl->Gate = Gate; }
void FNodeForgeEditorControlChannel::Drain(FNodeForgeEditorControlService& Service)
{
    check(IsInGameThread()); NodeForgeEditorControl::FRequest Request; FString Hash;
    auto* LiveCoding = FModuleManager::GetModulePtr<ILiveCodingModule>(TEXT("LiveCoding"));
    if (LiveCoding)
    {
        if (LiveCoding->IsCompiling()) RequestStop();
        else
        {
            if (Impl->bLiveCodingStartupPending && LiveCoding->HasStarted())
            {
                LiveCoding->EnableForSession(false);
                Impl->bLiveCodingStartupPending = false;
            }
            if (LiveCoding->IsEnabledForSession()) RequestStop();
        }
    }
    if (Impl->bDraining) return;
    TGuardValue<bool> Draining(Impl->bDraining, true);
    Impl->ReleaseCompletedAutomatically(); // Idle retry only after the real shared gate becomes free.
#if WITH_DEV_AUTOMATION_TESTS
    bool HasPending = false;
    { FScopeLock Lock(&Impl->RegistryLock); HasPending = !Impl->PendingCall.IsEmpty(); }
    if (HasPending && Impl->BeforeDrainProbe) Impl->BeforeDrainProbe();
#endif
    {
        FScopeLock Lock(&Impl->RegistryLock);
        if (Impl->PendingCall.IsEmpty()) return;
        FImpl::FCall& Call = Impl->Calls.FindChecked(Impl->PendingCall);
        Request.Id = Call.Id; Request.Method = Call.Method; Request.ExpectedRevision = Call.ExpectedRevision; Hash = Call.Fingerprint;
        // Already validated and serialized on admission. This DOM belongs only to GT.
        const bool Parsed = FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Call.Arguments), Request.Arguments) && Request.Arguments.IsValid();
        if (!Parsed) { Impl->bStopping.store(true); return; } // Retain the queued ID/slot; never dispatch a damaged DTO.
        Impl->ActiveCall = Impl->PendingCall; Impl->PendingCall.Empty();
        Call.Cache = NodeForgeControlWindows::JsonText(Impl->Record(Request, Hash, TEXT("running")));
    }
    auto Result = MakeShared<FJsonObject>();
    if (Impl->bStopping.load())
    { Result->SetStringField(TEXT("state"), TEXT("cancelled")); Result->SetStringField(TEXT("code"), TEXT("stopping")); Result->SetNumberField(TEXT("cliExitCode"), 2); }
    else if (Request.Method == TEXT("graphs.preflight") || Request.Method == TEXT("graphs.generate")
        || Request.Method == TEXT("draft.prepareCopy") || Request.Method == TEXT("graphs.preview"))
        Result = Service.ExecuteGraphCall(Request.Method, Request.ExpectedRevision, Impl->Session, Request.Id, Impl->PublicDirectory);
    else if (Request.Method == TEXT("rules.query") || Request.Method == TEXT("catalog.get")
        || Request.Method == TEXT("resources.query") || Request.Method == TEXT("resources.inspect") || Request.Method == TEXT("graphs.export"))
        Result = Service.ExecuteQueryCall(Request.Method, Request.Arguments.ToSharedRef(), Request.ExpectedRevision,
            Impl->Session, Request.Id, Impl->PublicDirectory);
    else Result = Service.ExecuteDraftCall(Request.Method, Request.Arguments.ToSharedRef(), Request.ExpectedRevision);
    Publish(Service.GetState(), Service.GetDraft()); // Terminal call cannot precede its immutable state publication.
    Impl->FinishCall(Request, Hash, Result);
}
void FNodeForgeEditorControlChannel::Publish(const TSharedRef<FJsonObject>& State, const TSharedRef<FJsonObject>& Draft)
{
    check(IsInGameThread()); State->SetObjectField(TEXT("identity"), Impl->Identity.IsValid() ? Impl->Identity.ToSharedRef() : MakeShared<FJsonObject>());
    State->SetArrayField(TEXT("clients"),ConnectedClients());
    FString StateText = NodeForgeControlWindows::JsonText(State), DraftText = NodeForgeControlWindows::JsonText(Draft);
    FScopeLock Lock(&Impl->CacheLock); Impl->StateCache = MoveTemp(StateText); Impl->DraftCache = MoveTemp(DraftText);
}
bool FNodeForgeEditorControlChannel::Stop()
{
    check(IsInGameThread());
    if (!Impl) return true;
    if (Impl->bTransportStopped) return Impl->bStopped; // Never hide a cleanup/receipt failure on the second call.
    RequestStop();
    if (Impl->ManagementReplies.load() || (Impl->Gate && Impl->Gate->IsBusy())) return false;
    {
        FScopeLock Lock(&Impl->RegistryLock);
        if (!Impl->PendingCall.IsEmpty() || !Impl->ActiveCall.IsEmpty()) return false;
        for (const auto& Call : Impl->Calls) if (Call.Value.bPublishing) return false;
    }
    if (!Impl->bStopSignaled)
    { Impl->bStopSignaled = true; Impl->StopDeadline = FPlatformTime::Seconds() + NodeForgeEditorControl::StopSeconds; SetEvent(Impl->StopEvent.Value); }
    const double Deadline = Impl->StopDeadline; // Repeated observation does not renew the stop budget.
    bool Completed = true;
    for (const auto& Worker : Impl->Workers)
        if (Worker->Thread && WaitForSingleObject(Worker->Done.Value, static_cast<DWORD>(FMath::Max(0.0, Deadline - FPlatformTime::Seconds()) * 1000)) != WAIT_OBJECT_0) Completed = false;
    if (!Completed) return false; // Do not inspect worker-owned fields while any thread is still active.
    for (const auto& Worker : Impl->Workers)
        if (Worker->bPending)
        {
            DWORD Count = 0;
            if (WaitForSingleObject(Worker->IoEvent.Value, 0) == WAIT_OBJECT_0)
            {
                const BOOL Result = GetOverlappedResult(Worker->Pipe.Value, &Worker->Overlap, &Count, 0);
                const DWORD Error = Result ? ERROR_SUCCESS : GetLastError();
                if (Error == ERROR_IO_INCOMPLETE) { Completed = false; continue; }
                Worker->LastCancelledKind.store(Worker->CurrentIo.load()); Worker->LastCancelledCode.store(Error);
                Worker->bPending = false; Worker->CurrentIo.store(0); ++Worker->CancelCompleted;
            }
            else Completed = false;
        }
    if (!Completed) return false;
    auto Receipt = MakeShared<FJsonObject>(); TArray<TSharedPtr<FJsonValue>> Workers;
    for (auto& Worker : Impl->Workers)
    {
        auto Row = MakeShared<FJsonObject>(); Row->SetNumberField(TEXT("cancelRequested"), Worker->CancelRequested.load());
        Row->SetNumberField(TEXT("cancelCompleted"), Worker->CancelCompleted.load()); Row->SetNumberField(TEXT("pendingConnectCount"), Worker->ConnectPending.load());
        Row->SetNumberField(TEXT("pendingReadCount"), Worker->ReadPending.load()); Row->SetNumberField(TEXT("pendingWriteCount"), Worker->WritePending.load());
        Row->SetNumberField(TEXT("lastCancelledKind"), Worker->LastCancelledKind.load()); Row->SetNumberField(TEXT("lastCancelledWindowsCode"), Worker->LastCancelledCode.load());
        Row->SetBoolField(TEXT("pendingIo"), Worker->bPending); Row->SetBoolField(TEXT("threadDone"), true); Workers.Add(MakeShared<FJsonValueObject>(Row));
        Worker->Thread.Reset();
    }
    Impl->Workers.Empty(); Impl->Credential.Empty(); Impl->StopEvent.Reset(); Impl->bTransportStopped = true;
    const bool Deleted = Impl->DescriptorPath.IsEmpty() || !IFileManager::Get().FileExists(*Impl->DescriptorPath) || DeleteFileW(*Impl->DescriptorPath);
    Receipt->SetStringField(TEXT("controlSessionId"), Impl->Session); Receipt->SetStringField(TEXT("state"), Deleted ? TEXT("stopped") : TEXT("cleanupUnknown"));
    Receipt->SetBoolField(TEXT("descriptorDeleted"), Deleted); Receipt->SetBoolField(TEXT("ioCompletionUnknownObserved"), Impl->bIoCompletionUnknown.load()); Receipt->SetArrayField(TEXT("workers"), Workers);
    Receipt->SetBoolField(TEXT("transportHandlesReleased"), true);
    Impl->bStopped = (!Impl->bStarted || NodeForgePrototype::WriteJson(Receipt, Impl->PublicDirectory / TEXT("StopReceipt.json"))) && Deleted;
    if (Impl->bStopped && !Impl->DiscoveryPath.IsEmpty() && IFileManager::Get().FileExists(*Impl->DiscoveryPath))
    {
        TSharedPtr<FJsonObject> Own;
        if (!NodeForgeControlWindows::ReadObject(Impl->DiscoveryPath, Own, NodeForgeEditorControl::MaxDescriptorBytes)
            || Own->GetStringField(TEXT("controlSessionId")) != Impl->Session || !DeleteFileW(*Impl->DiscoveryPath))
            Impl->bStopped = false;
    }
    return Impl->bStopped;
}
void FNodeForgeEditorControlChannel::RequestStop()
{ Impl->bStopping.store(true); Impl->bStopRequested.store(true); }
bool FNodeForgeEditorControlChannel::HasStopRequest() const { return Impl->bStopRequested.load(); }
#if WITH_DEV_AUTOMATION_TESTS
void FNodeForgeEditorControlChannel::SetBeforeDrainProbeForChecks(TFunction<void()> Probe)
{ check(IsInGameThread()); Impl->BeforeDrainProbe = MoveTemp(Probe); }
#endif
bool FNodeForgeEditorControlChannel::IsStopped() const { return Impl->bStopped; }
TSharedRef<FJsonObject> FNodeForgeEditorControlChannel::PublicIdentity() const
{ check(IsInGameThread()); return Impl->Identity.IsValid() ? MakeShared<FJsonObject>(*Impl->Identity) : MakeShared<FJsonObject>(); }
TArray<TSharedPtr<FJsonValue>> FNodeForgeEditorControlChannel::ConnectedClients() const
{
    TArray<TSharedPtr<FJsonValue>> Rows;FScopeLock Lock(&Impl->ClientLock);
    for(const auto& Worker:Impl->Workers)if(!Worker->ClientId.IsEmpty()&&WaitForSingleObject(Worker->ClientStop.Value,0)!=WAIT_OBJECT_0)
    {
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("id"),Worker->ClientId);
        Row->SetStringField(TEXT("name"),Worker->ClientName);Row->SetStringField(TEXT("version"),Worker->ClientVersion);
        Rows.Add(MakeShared<FJsonValueObject>(Row));
    }
    return Rows;
}
namespace
{
bool MatchesClient(const FString& Name,const FString& Client)
{
    return Client==TEXT("codex")&&(Name==TEXT("codex")||Name==TEXT("codex-mcp-client")||Name==TEXT("Codex"));
}
}
bool FNodeForgeEditorControlChannel::HasConnectedClient(const FString& Client) const
{
    FScopeLock Lock(&Impl->ClientLock);
    for(const auto& Worker:Impl->Workers)if(!Worker->ClientId.IsEmpty()&&MatchesClient(Worker->ClientName,Client)
        &&WaitForSingleObject(Worker->ClientStop.Value,0)!=WAIT_OBJECT_0)return true;
    return false;
}
void FNodeForgeEditorControlChannel::DisconnectClient(const FString& Client)
{
    FScopeLock Lock(&Impl->ClientLock);
    for(const auto& Worker:Impl->Workers)if(!Worker->ClientId.IsEmpty()&&MatchesClient(Worker->ClientName,Client))SetEvent(Worker->ClientStop.Value);
}
bool FNodeForgeEditorControlChannel::CheckRestrictedConnection(TSharedRef<FJsonObject>& Evidence) const
{
    using namespace NodeForgeControlWindows;
    FHandle Token, Restricted; if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE, &Token.Value)) return false;
    uint8 RestrictedSid[SECURITY_MAX_SID_SIZE]; DWORD SidSize = sizeof(RestrictedSid);
    if (!CreateWellKnownSid(WinRestrictedCodeSid, nullptr, RestrictedSid, &SidSize)) return false;
    SID_AND_ATTRIBUTES Restriction = {RestrictedSid, 0};
    if (!CreateRestrictedToken(Token.Value, DISABLE_MAX_PRIVILEGE, 0, nullptr, 0, nullptr, 1, &Restriction, &Restricted.Value)) return false;
    if (!ImpersonateLoggedOnUser(Restricted.Value)) return false;
    HANDLE Attempt = CreateFileW(*Impl->PipeName, FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE, 0, nullptr, OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED | SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION, nullptr);
    const DWORD Code = Attempt == INVALID_HANDLE_VALUE ? GetLastError() : ERROR_SUCCESS;
    if (Attempt != INVALID_HANDLE_VALUE) CloseHandle(Attempt); const bool Reverted = RevertToSelf() != 0;
    Evidence->SetStringField(TEXT("test"), TEXT("actualRestrictedTokenCreateFile")); Evidence->SetNumberField(TEXT("windowsError"), Code);
    Evidence->SetBoolField(TEXT("reverted"), Reverted); Evidence->SetBoolField(TEXT("secondWindowsAccountTested"), false);
    return Code == ERROR_ACCESS_DENIED && Reverted;
}

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
bool SessionCheckDacl(const FString& Path, FString& Sddl)
{
    PSECURITY_DESCRIPTOR Sd = nullptr;
    if (GetNamedSecurityInfoW(*Path, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
        nullptr, nullptr, nullptr, nullptr, &Sd) != ERROR_SUCCESS) return false;
    LPWSTR Text = nullptr;
    const bool Good = ConvertSecurityDescriptorToStringSecurityDescriptorW(Sd, SDDL_REVISION_1,
        DACL_SECURITY_INFORMATION, &Text, nullptr) != 0;
    if (Good) { Sddl = Text; LocalFree(Text); }
    LocalFree(Sd); return Good;
}
bool SessionCheckSetDacl(const FString& Path, const FString& Sddl)
{
    PSECURITY_DESCRIPTOR Sd = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(*Sddl, SDDL_REVISION_1, &Sd, nullptr)) return false;
    BOOL Present = false, Defaulted = false; PACL Acl = nullptr;
    const bool Good = GetSecurityDescriptorDacl(Sd, &Present, &Acl, &Defaulted) && Present && Acl
        && SetNamedSecurityInfoW(const_cast<LPWSTR>(*Path), SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, nullptr, nullptr, Acl, nullptr) == ERROR_SUCCESS;
    LocalFree(Sd); return Good;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeSessionContainerLifetimeTest, "NodeForge.Security.SessionContainerLifetime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNodeForgeSessionContainerLifetimeTest::RunTest(const FString&)
{
    using namespace NodeForgeControlWindows;
    FSecurity Security;
    if (!TestTrue(TEXT("Current token initialized"), Security.Initialize())) return false;
    const FString Root = Full(FPaths::ProjectSavedDir() / TEXT("NodeForge/Reports/SessionContainerChecks")
        / FGuid::NewGuid().ToString(EGuidFormats::Digits));
    const FString Parent = Root / TEXT("Legacy"), Child = Parent / TEXT("OldSession"), Sentinel = Child / TEXT("Sentinel.txt");
    const FString Legacy = TEXT("D:P(A;OICI;FA;;;SY)(A;OICI;FA;;;S-1-5-5-0-1)");
    const FString Persistent = TEXT("D:P(A;;FA;;;SY)(A;;FA;;;") + Security.UserText + TEXT(")");
    if (!TestTrue(TEXT("Owned fixture directory created"), IFileManager::Get().MakeDirectory(*Child, true))
        || !TestTrue(TEXT("Noncredential sentinel created"), FFileHelper::SaveStringToFile(TEXT("test-only"), *Sentinel))
        || !TestTrue(TEXT("Legacy child protected"), SessionCheckSetDacl(Sentinel, Legacy) && SessionCheckSetDacl(Child, Legacy))
        || !TestTrue(TEXT("Legacy parent protected"), SessionCheckSetDacl(Parent, Legacy))) return false;
    FString ChildBefore, FileBefore;
    SessionCheckDacl(Child, ChildBefore); SessionCheckDacl(Sentinel, FileBefore);
    auto Evidence = MakeShared<FJsonObject>();
    const bool Migrated = Security.Directory(Parent, Evidence);
    TestTrue(TEXT("Actual native startup directory admits a previous-logon container"), Migrated);
    FString ParentAfter, ChildAfter, FileAfter;
    SessionCheckDacl(Parent, ParentAfter); SessionCheckDacl(Child, ChildAfter); SessionCheckDacl(Sentinel, FileAfter);
    // Windows may retain SE_DACL_AUTO_INHERITED on a protected object updated
    // through SetNamedSecurityInfo. Compare the grants, not that bookkeeping bit.
    TestEqual(TEXT("Parent is protected user container with no inherited grants"), ParentAfter.Replace(TEXT("D:PAI"), TEXT("D:P")), Persistent);
    TestEqual(TEXT("Old session directory ACL unchanged"), ChildAfter, ChildBefore);
    TestEqual(TEXT("Old session file ACL unchanged"), FileAfter, FileBefore);
    FHandle ReadOld(CreateFileW(*Sentinel, FILE_READ_DATA, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    TestFalse(TEXT("Previous-logon file remains unreadable"), ReadOld.Valid());
    if (!ReadOld.Valid()) TestEqual(TEXT("Old file denied by Windows"), GetLastError(), static_cast<DWORD>(ERROR_ACCESS_DENIED));
    auto Again = MakeShared<FJsonObject>();
    TestTrue(TEXT("Existing user container is idempotent"), Security.Directory(Parent, Again));
    FString ParentAgain; SessionCheckDacl(Parent, ParentAgain);
    TestEqual(TEXT("Repeated validation preserves parent ACL"), ParentAgain, ParentAfter);
    const FString Fresh = Root / TEXT("Fresh"); auto NewEvidence = MakeShared<FJsonObject>();
    TestTrue(TEXT("Fresh native parent created"), Security.Directory(Fresh, NewEvidence));
    FString FreshAcl; SessionCheckDacl(Fresh, FreshAcl);
    TestEqual(TEXT("Fresh parent has user lifetime"), FreshAcl, Persistent);
    const FString Unknown = Root / TEXT("Unknown");
    IFileManager::Get().MakeDirectory(*Unknown, true);
    TestTrue(TEXT("Unknown ACL fixture prepared"), SessionCheckSetDacl(Unknown, TEXT("D:P(A;OICI;FA;;;SY)(A;OICI;FA;;;WD)")));
    FString UnknownBefore, UnknownAfter; SessionCheckDacl(Unknown, UnknownBefore);
    auto Rejected = MakeShared<FJsonObject>();
    TestFalse(TEXT("Unknown ACL is rejected rather than repaired"), Security.Directory(Unknown, Rejected));
    SessionCheckDacl(Unknown, UnknownAfter); TestEqual(TEXT("Rejected ACL unchanged"), UnknownAfter, UnknownBefore);
    auto Report = MakeShared<FJsonObject>(); Report->SetStringField(TEXT("fixtureRoot"), Root);
    Report->SetBoolField(TEXT("passed"), !HasAnyErrors()); Report->SetObjectField(TEXT("parentEvidence"), Evidence);
    Report->SetStringField(TEXT("oldChildAclBefore"), ChildBefore); Report->SetStringField(TEXT("oldChildAclAfter"), ChildAfter);
    Report->SetStringField(TEXT("oldFileAclBefore"), FileBefore); Report->SetStringField(TEXT("oldFileAclAfter"), FileAfter);
    NodeForgePrototype::WriteJson(Report, Root / TEXT("Report.json"));
    return !HasAnyErrors();
}
#endif

#include "NodeForgeCallCapacityChecks.inl"

#include "NodeForgeAutoCallCapacityChecks.inl"
