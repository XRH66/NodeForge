// Real completion lifecycle; the caller never counts or invokes cleanup.
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeAutoCallCapacityTest, "NodeForge.Control.AutoCallCapacity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNodeForgeAutoCallCapacityTest::RunTest(const FString&)
{
    using namespace NodeForgeEditorControl;
    FNodeForgeEditorControlChannel::FImpl Core;
    Core.Gate = MakeShared<FNodeForgeEditorBusinessGate>(); Core.Session = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    Core.PublicDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("NodeForge/Reports/AutoCallCapacityChecks") / Core.Session);
    auto Request = [&](const FString& Id, const FString& Method)
    {
        FRequest R; R.Id = Id; R.Method = Method;
        R.Definition = NodeForgeControlOperations::Find(Method, NodeForgeControlOperations::EHost::ActiveEditor);
        R.Context = MakeShared<FJsonObject>(); R.Context->SetStringField(TEXT("controlSessionId"), Core.Session);
        R.Arguments = MakeShared<FJsonObject>();
        if (Method == TEXT("draft.replace")) R.Arguments->SetStringField(TEXT("text"), TEXT("{}"));
        return R;
    };
    FProtocolError Error; TSharedPtr<FJsonObject> Row; TMap<FString,FString> OriginalHashes;
    int32 FinishedCount = 0;
    for (int32 I = 0; I < 221; ++I)
    {
        auto R = Request(FString::Printf(TEXT("Automatic-%03d"), I), TEXT("draft.replace"));
        const bool Accepted = Core.Call(R, Row, Error);
        TestTrue(TEXT("Business continues beyond 128 with no caller cleanup"), Accepted);
        if (!Accepted) break;
        Core.PendingCall.Empty(); Core.ActiveCall = R.Id;
        TestTrue(TEXT("Actual native business gate"), Core.Gate->BeginControl());
        auto Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("state"), TEXT("completed")); Result->SetNumberField(TEXT("cliExitCode"), 0);
        Core.FinishCall(R, Fingerprint(R), Result); ++FinishedCount;
        TestEqual(TEXT("Native completion automatically bounds resident records"), Core.Calls.Num(), FinishedCount % 50);
        for (const TCHAR* Suffix : {TEXT("Accepted.json"), TEXT("Finished.json")})
        { const FString Path = Core.PublicDirectory / TEXT("Calls") / (R.Id + Suffix); OriginalHashes.Add(Path, NodeForgePrototype::FileSha256(Path)); }
    }
    TestEqual(TEXT("All business calls completed without manual cleanup"), FinishedCount, 221);
    auto Lookup = Request(TEXT("ReadArchived"), TEXT("calls.get")); Lookup.Arguments->SetStringField(TEXT("callId"), TEXT("Automatic-000"));
    TestTrue(TEXT("Automatically archived original remains queryable"), Core.Call(Lookup, Row, Error));
    if (Row.IsValid()) TestEqual(TEXT("Original state retained"), Row->GetStringField(TEXT("state")), FString(TEXT("completed")));
    const int32 Retained = Core.Calls.Num();
    auto Replay = Request(TEXT("Automatic-000"), TEXT("draft.replace"));
    TestTrue(TEXT("Automatically archived original replay is read-only"), Core.Call(Replay, Row, Error));
    TestEqual(TEXT("Replay uses no new capacity"), Core.Calls.Num(), Retained);
    Replay.Arguments->SetStringField(TEXT("text"), TEXT("changed"));
    TestFalse(TEXT("Automatically archived conflicting ID remains rejected"), Core.Call(Replay, Row, Error));
    TestEqual(TEXT("Original conflict protection"), Error.Code, FString(TEXT("callConflict")));
    // Delay a threshold crossing with an unresolved publishing row; no caller cleanup may bypass it.
    auto& Held = Core.Calls.FindChecked(TEXT("Automatic-200")); Held.bPublishing = true;
    for (int32 I = 221; I < 250; ++I)
    {
        auto R = Request(FString::Printf(TEXT("Automatic-%03d"), I), TEXT("draft.replace"));
        if (!TestTrue(TEXT("Business reaches guarded threshold"), Core.Call(R, Row, Error))) break;
        Core.PendingCall.Empty(); Core.ActiveCall = R.Id; Core.Gate->BeginControl();
        auto Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("state"), TEXT("completed")); Result->SetNumberField(TEXT("cliExitCode"), 0);
        Core.FinishCall(R, Fingerprint(R), Result);
    }
    TestEqual(TEXT("Publishing defers the entire automatic cleanup"), Core.Calls.Num(), 50);
    // A TMap insertion may relocate values; reacquire the held row after filling.
    auto& StableHeld = Core.Calls.FindChecked(TEXT("Automatic-200")); StableHeld.bPublishing = false; const FString Good = StableHeld.Cache;
    for (const TCHAR* State : {TEXT("queued"), TEXT("running"), TEXT("pending"), TEXT("unknown"), TEXT("failed")})
    {
        auto Unsafe = MakeShared<FJsonObject>(); Unsafe->SetStringField(TEXT("state"), State); Unsafe->SetNumberField(TEXT("cliExitCode"), 5);
        StableHeld.Cache = NodeForgeControlWindows::JsonText(Unsafe); Core.ReleaseCompletedAutomatically();
        TestEqual(TEXT("Unresolved rows are never automatically reclaimed"), Core.Calls.Num(), 50);
    }
    auto Review = MakeShared<FJsonObject>(); Review->SetStringField(TEXT("state"), TEXT("completed")); Review->SetNumberField(TEXT("cliExitCode"), 0); Review->SetBoolField(TEXT("requiresReview"), true);
    StableHeld.Cache = NodeForgeControlWindows::JsonText(Review); Core.ReleaseCompletedAutomatically();
    TestEqual(TEXT("Review-required row blocks automatic cleanup"), Core.Calls.Num(), 50); StableHeld.Cache = Good;
    Core.Gate->TryUI(); Core.ReleaseCompletedAutomatically();
    TestEqual(TEXT("UI busy safely defers cleanup"), Core.Calls.Num(), 50); Core.Gate->EndUI();
    Core.PendingCall = TEXT("QueuedFixture"); Core.ReleaseCompletedAutomatically();
    TestEqual(TEXT("Pending business is not bypassed"), Core.Calls.Num(), 50); Core.PendingCall.Empty();
    Core.ActiveCall = TEXT("ActiveFixture"); Core.ReleaseCompletedAutomatically();
    TestEqual(TEXT("Active business is not bypassed"), Core.Calls.Num(), 50); Core.ActiveCall.Empty();
    Core.Gate->ReserveControl(); Core.Gate->BeginControl(); Core.ReleaseCompletedAutomatically();
    TestEqual(TEXT("Running shared gate is not bypassed"), Core.Calls.Num(), 50); Core.Gate->EndControl();
    Core.ReleaseCompletedAutomatically();
    TestEqual(TEXT("Idle hook recovers automatically after busy condition ends"), Core.Calls.Num(), 0);
    TestEqual(TEXT("Five native automatic batches"), Core.AutomaticCleanupBatches, 5);
    for (const auto& Pair : OriginalHashes) TestEqual(TEXT("Automatic cleanup preserves original bytes"), NodeForgePrototype::FileSha256(Pair.Key), Pair.Value);
    FNodeForgeEditorControlChannel::FImpl Broken;
    Broken.Gate = MakeShared<FNodeForgeEditorBusinessGate>(); Broken.Session = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    Broken.PublicDirectory = Core.PublicDirectory / TEXT("ArchiveFailure");
    for (int32 I = 0; I < 50; ++I)
    {
        auto R = Request(FString::Printf(TEXT("Broken-%03d"), I), TEXT("draft.replace")); R.Context->SetStringField(TEXT("controlSessionId"), Broken.Session);
        if (!TestTrue(TEXT("Failure fixture admitted"), Broken.Call(R, Row, Error))) break;
        Broken.PendingCall.Empty(); Broken.ActiveCall = R.Id; Broken.Gate->BeginControl();
        if (I == 49) FFileHelper::SaveStringToFile(TEXT("{}"), *(Broken.PublicDirectory / TEXT("Calls/Broken-000Finished.json")));
        auto Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("state"), TEXT("completed")); Result->SetNumberField(TEXT("cliExitCode"), 0);
        Broken.FinishCall(R, Fingerprint(R), Result);
    }
    TestTrue(TEXT("Archive mismatch stops automatic writes"), Broken.bStopping.load());
    TestEqual(TEXT("Archive failure retains all resident records"), Broken.Calls.Num(), 50);
    const FString UnknownCleanup = Broken.AutomaticCleanupCallId;
    TestFalse(TEXT("Unknown automatic cleanup retains its original ID"), UnknownCleanup.IsEmpty());
    Broken.ReleaseCompletedAutomatically();
    TestEqual(TEXT("Idle observation never replaces unresolved cleanup ID"), Broken.AutomaticCleanupCallId, UnknownCleanup);
    auto Recovery = Request(TEXT("RecoverAutomaticCleanup"), TEXT("calls.get")); Recovery.Arguments->SetStringField(TEXT("callId"), UnknownCleanup);
    TestTrue(TEXT("Automatic cleanup intent is queryable under original ID"), Broken.Call(Recovery, Row, Error));
    if (Row.IsValid()) TestEqual(TEXT("Unresolved automatic receipt stays unknown"), Row->GetStringField(TEXT("state")), FString(TEXT("unknown")));
    auto Rejected = Request(TEXT("AfterArchiveFailure"), TEXT("draft.replace"));
    TestFalse(TEXT("No new business after failed automatic archive"), Broken.Call(Rejected, Row, Error));
    auto Evidence = MakeShared<FJsonObject>(); Evidence->SetBoolField(TEXT("passed"), !HasAnyErrors());
    Evidence->SetNumberField(TEXT("businessCalls"), FinishedCount); Evidence->SetNumberField(TEXT("manualCleanupCalls"), 0);
    Evidence->SetObjectField(TEXT("capacity"), Core.Capacity());
    NodeForgePrototype::WriteJson(Evidence, Core.PublicDirectory / TEXT("Report.json"));
    return !HasAnyErrors();
}
#endif
