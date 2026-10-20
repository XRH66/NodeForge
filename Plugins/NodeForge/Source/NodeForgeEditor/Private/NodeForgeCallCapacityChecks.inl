// Compiled with the real FImpl in NodeForgeEditorControlChannel.cpp; isolated fixture only.
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeCallCapacityTest, "NodeForge.Control.CallCapacity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNodeForgeCallCapacityTest::RunTest(const FString&)
{
    using namespace NodeForgeEditorControl;
    FNodeForgeEditorControlChannel::FImpl Core;
    Core.Gate = MakeShared<FNodeForgeEditorBusinessGate>(); Core.Session = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    Core.PublicDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("NodeForge/Reports/CallCapacityChecks") / Core.Session);
    auto Request = [&](const FString& Id, const FString& Method)
    {
        FRequest R; R.Id = Id; R.Method = Method; R.Definition = NodeForgeControlOperations::Find(Method, NodeForgeControlOperations::EHost::ActiveEditor);
        R.Context = MakeShared<FJsonObject>(); R.Context->SetStringField(TEXT("controlSessionId"), Core.Session);
        R.Arguments = MakeShared<FJsonObject>();
        if (Method == TEXT("draft.replace")) R.Arguments->SetStringField(TEXT("text"), TEXT("{}"));
        if (Method == TEXT("calls.releaseCompleted")) R.Arguments->SetNumberField(TEXT("maxCount"), 50);
        return R;
    };
    // Seed the terminal registry for explicit-maintenance boundary tests; automatic FinishCall is covered separately.
    auto SeedTerminal = [&](const FRequest& R, const FString& Hash, const TSharedRef<FJsonObject>& Result)
    {
        auto Finished = Core.Record(R, Hash, *Result->GetStringField(TEXT("state")));
        Finished->SetObjectField(TEXT("result"), Result); Finished->SetNumberField(TEXT("cliExitCode"), Result->GetNumberField(TEXT("cliExitCode")));
        TestTrue(TEXT("Durable maintenance fixture"), NodeForgePrototype::WriteJson(Finished, Core.PublicDirectory / TEXT("Calls") / (R.Id + TEXT("Finished.json"))));
        Core.Calls.FindChecked(R.Id).Cache = NodeForgeControlWindows::JsonText(Finished); Core.ActiveCall.Empty(); Core.Gate->EndControl();
    };
    FProtocolError Error; TSharedPtr<FJsonObject> Row; TMap<FString,FString> OriginalHashes;
    for (int32 I = 0; I < 128; ++I)
    {
        auto R = Request(FString::Printf(TEXT("Capacity-%03d"), I), TEXT("draft.replace"));
        TestTrue(TEXT("Actual Call admission"), Core.Call(R, Row, Error));
        Core.PendingCall.Empty(); TestTrue(TEXT("Actual control gate"), Core.Gate->BeginControl());
        auto Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("state"), TEXT("completed")); Result->SetNumberField(TEXT("cliExitCode"), 0);
        SeedTerminal(R, Fingerprint(R), Result);
        for (const TCHAR* Suffix : {TEXT("Accepted.json"), TEXT("Finished.json")})
        { const FString Path = Core.PublicDirectory / TEXT("Calls") / (R.Id + Suffix); OriginalHashes.Add(Path, NodeForgePrototype::FileSha256(Path)); }
    }
    auto Next = Request(TEXT("AfterFull"), TEXT("draft.replace"));
    TestFalse(TEXT("128 blocks new business"), Core.Call(Next, Row, Error)); TestEqual(TEXT("Exact capacity failure"), Error.Code, FString(TEXT("callCapacity")));
    auto Cleanup = Request(TEXT("Cleanup-1"), TEXT("calls.releaseCompleted"));
    TestTrue(TEXT("Cleanup admitted while full"), Core.ReleaseCompleted(Cleanup, Row, Error));
    TestEqual(TEXT("Actually released 50"), Core.Calls.Num(), 78);
    TestEqual(TEXT("Receipt count"), Row->GetObjectField(TEXT("result"))->GetIntegerField(TEXT("released")), 50);
    TestTrue(TEXT("Same cleanup ID returns original receipt"), Core.ReleaseCompleted(Cleanup, Row, Error));
    TestEqual(TEXT("Cleanup replay does not remove more"), Core.Calls.Num(), 78);
    auto Lookup = Request(TEXT("Read-1"), TEXT("calls.get")); Lookup.Arguments->SetStringField(TEXT("callId"), TEXT("Capacity-000"));
    TestTrue(TEXT("Archived original remains queryable"), Core.Call(Lookup, Row, Error));
    TestEqual(TEXT("Archived original is completed"), Row->GetStringField(TEXT("state")), FString(TEXT("completed")));
    auto Original = Request(TEXT("Capacity-000"), TEXT("draft.replace"));
    TestTrue(TEXT("Archived same ID returns original without execution"), Core.Call(Original, Row, Error));
    TestEqual(TEXT("No slot used by replay"), Core.Calls.Num(), 78);
    Original.Arguments->SetStringField(TEXT("text"), TEXT("changed"));
    TestFalse(TEXT("Archived changed args refused"), Core.Call(Original, Row, Error)); TestEqual(TEXT("Conflict preserved"), Error.Code, FString(TEXT("callConflict")));
    auto More = Request(TEXT("Cleanup-2"), TEXT("calls.releaseCompleted"));
    TestTrue(TEXT("UI reserves real shared gate"), Core.Gate->TryUI());
    TestFalse(TEXT("UI busy rejects cleanup"), Core.ReleaseCompleted(More, Row, Error)); Core.Gate->EndUI();
    TestTrue(TEXT("Capacity actually allows next business"), Core.Call(Next, Row, Error));
    TestFalse(TEXT("Queued business rejects cleanup"), Core.ReleaseCompleted(More, Row, Error));
    Core.PendingCall.Empty(); Core.Gate->BeginControl();
    TestFalse(TEXT("Running business rejects cleanup"), Core.ReleaseCompleted(More, Row, Error));
    auto Done = MakeShared<FJsonObject>(); Done->SetStringField(TEXT("state"), TEXT("completed")); Done->SetNumberField(TEXT("cliExitCode"), 0);
    SeedTerminal(Next, Fingerprint(Next), Done);
    auto& Held = Core.Calls.FindChecked(TEXT("AfterFull")); const FString Good = Held.Cache;
    Held.bPublishing = true; TestFalse(TEXT("Publishing refuses cleanup"), Core.ReleaseCompleted(More, Row, Error)); Held.bPublishing = false;
    for (const TCHAR* State : {TEXT("queued"), TEXT("running"), TEXT("pending"), TEXT("unknown"), TEXT("failed")})
    {
        auto Unsafe = MakeShared<FJsonObject>(); Unsafe->SetStringField(TEXT("state"), State); Unsafe->SetNumberField(TEXT("cliExitCode"), 5);
        Held.Cache = NodeForgeControlWindows::JsonText(Unsafe);
        TestFalse(FString::Printf(TEXT("%s not reclaimed"), State), Core.ReleaseCompleted(More, Row, Error));
        TestEqual(TEXT("No partial cleanup on unresolved state"), Core.Calls.Num(), 79);
    }
    Held.Cache = Good;
    TestTrue(TEXT("Next batch succeeds after terminal confirmation"), Core.ReleaseCompleted(More, Row, Error));
    TestEqual(TEXT("Capacity count after second cleanup"), Core.Calls.Num(), 29);
    for (const auto& Pair : OriginalHashes) TestEqual(TEXT("Original evidence bytes preserved"), NodeForgePrototype::FileSha256(Pair.Key), Pair.Value);
    const FString Tamper = Core.PublicDirectory / TEXT("Calls/Capacity-000Finished.json");
    FFileHelper::SaveStringToFile(TEXT("{}"), *Tamper);
    TestFalse(TEXT("Tampered archive rejected"), Core.Call(Lookup, Row, Error));
    auto Evidence = MakeShared<FJsonObject>(); Evidence->SetBoolField(TEXT("passed"), !HasAnyErrors()); Evidence->SetObjectField(TEXT("capacity"), Core.Capacity());
    Evidence->SetStringField(TEXT("scope"), TEXT("Real native registry and disk journal in isolated test fixture; no target calls modified"));
    NodeForgePrototype::WriteJson(Evidence, Core.PublicDirectory / TEXT("Report.json"));
    return !HasAnyErrors();
}
#endif
