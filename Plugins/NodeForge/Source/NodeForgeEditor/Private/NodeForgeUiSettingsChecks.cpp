#include "NodeForgePanelPreferences.h"
#include "NodeForgeInstallation.h"
#include "NodeForgePrototype.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformProcess.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNodeForgeUiSettingsTest,"NodeForge.UiSettingsCore",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FNodeForgeUiSettingsTest::RunTest(const FString&)
{
    FString Output;
    if(FPaths::GetBaseFilename(FPaths::GetProjectFilePath())!=TEXT("NFCUiPreview")
        ||!FParse::Param(FCommandLine::Get(),TEXT("NFUiSettingsChecks"))||!FParse::Value(FCommandLine::Get(),TEXT("NFSettingsReport="),Output)
        ||FPaths::FileExists(Output)||!FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Output),NodeForgeInstallation::StateDirectory()/TEXT("UiIntegrationChecks")))
        {AddError(TEXT("Owned settings project, explicit flag and fresh bounded report required."));return false;}
    TArray<TSharedPtr<FJsonValue>> Rows;bool All=true;
    auto Row=[&](const TCHAR* Name,bool Passed){TestTrue(Name,Passed);All&=Passed;auto R=MakeShared<FJsonObject>();R->SetStringField(TEXT("case"),Name);R->SetBoolField(TEXT("passed"),Passed);Rows.Add(MakeShared<FJsonValueObject>(R));};
    const FString Fixtures=NodeForgeInstallation::StateDirectory()/TEXT("UiIntegrationChecks/IconFixtures");
    for(const TCHAR* Name:{TEXT("Png.png"),TEXT("Jpeg.jpg"),TEXT("Webp.webp")})
    {
        TArray<uint8> Bytes,Png;FString Error;
        Row(*(FString(TEXT("NativeDecode."))+Name),FFileHelper::LoadFileToArray(Bytes,*(Fixtures/Name))&&FNodeForgePanelPreferences::DecodeIcon(Bytes,Png,Error)&&Png.Num()>8&&Png[0]==137&&Png[1]==80);
    }
    TArray<uint8> Invalid,Decoded;Invalid.AddZeroed(100);FString Error;
    Row(TEXT("InvalidImageRejectedWithoutOutput"),!FNodeForgePanelPreferences::DecodeIcon(Invalid,Decoded,Error)&&Decoded.IsEmpty());
    Invalid.AddZeroed(204800);Row(TEXT("OversizedImageRejected"),!FNodeForgePanelPreferences::DecodeIcon(Invalid,Decoded,Error));
    FNodeForgePanelPreferences Prefs;Row(TEXT("DefaultsAdmitted"),Prefs.Validate(Error));
    Prefs.Directory=TEXT("/Game/Outside");Row(TEXT("InvalidTargetDirectoryRejectedByProductionRule"),!Prefs.Validate(Error));Prefs.Directory=TEXT("/Game/NodeForgeTests");
    FNodeForgeClientDefinition Client;Client.Id=TEXT("custom-")+FGuid::NewGuid().ToString(EGuidFormats::Digits);Client.Name=TEXT("Codex");
    Prefs.CustomClients.Add(Client);Row(TEXT("DuplicatePresetNameRejected"),!Prefs.Validate(Error));
    auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("status"),All&&!HasAnyErrors()?TEXT("passed"):TEXT("failed"));Report->SetArrayField(TEXT("checks"),Rows);
    Report->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());Report->SetStringField(TEXT("binarySha256"),NodeForgePrototype::FileSha256(FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"))));
    Report->SetStringField(TEXT("scope"),TEXT("Production Windows image decode and shared target/name validation; actual settings UI and two-project persistence require separate evidence."));
    return NodeForgePrototype::WriteJson(Report,Output)&&All;
}
#endif
