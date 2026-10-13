#include "Modules/ModuleManager.h"
#include "NodeForgePrototype.h"
#include "NodeForgePanel.h"
#include "NodeForgeInstallationPanel.h"
#include "NodeForgeInstallation.h"
#include "NodeForgeResultView.h"
#include "NodeForgePanelState.h"
#include "NodeForgeEditorControlService.h"
#include "NodeForgeEditorControlChannel.h"
#include "ILiveCodingModule.h"
#include "Containers/Ticker.h"
#include "Misc/CommandLine.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetData.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Interfaces/IPluginManager.h"
#include "ToolMenus.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopedSlowTask.h"
#include "Misc/EngineVersion.h"
#include "Serialization/Archive.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Styling/AppStyle.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Text/STextBlock.h"

namespace { TWeakPtr<FNodeForgeEditorControlService> ProductService; TWeakPtr<FNodeForgeEditorControlChannel> ProductChannel; }

TSharedPtr<FNodeForgeEditorControlService> NodeForgePanel::GetProductService() { check(IsInGameThread()); return ProductService.Pin(); }
TSharedPtr<FNodeForgeEditorControlChannel> NodeForgePanel::GetControlChannel() { check(IsInGameThread()); return ProductChannel.Pin(); }
class FNodeForgeEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogTemp, Display, TEXT("NodeForge Editor module loaded"));
        SharedService = MakeShared<FNodeForgeEditorControlService>();
        ProductService = SharedService;
        if (!IsRunningCommandlet())
        {
            FGlobalTabmanager::Get()->RegisterNomadTabSpawner("NodeForge",
                FOnSpawnTab::CreateLambda([](const FSpawnTabArgs&) {
                    return SNew(SDockTab).TabRole(ETabRole::NomadTab)
                        [NodeForgePanel::Create()];
                })).SetDisplayName(FText::FromString(TEXT("NodeForge"))).SetMenuType(ETabSpawnerMenuType::Hidden);
            UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FNodeForgeEditorModule::RegisterMenus));
            InstallationReady = NodeForgeInstallation::OnReady().AddRaw(this, &FNodeForgeEditorModule::StartControlIfEnabled);
            StartControlIfEnabled();
        }
    }
    void StartControlIfEnabled()
    {
        if (ControlChannel || IsRunningCommandlet()) return;
        if (!FParse::Param(FCommandLine::Get(), TEXT("NFControlEnable")))
        {
            NodeForgeInstallation::FBinding Binding; FString Error, UserDirectory; TSharedPtr<FJsonObject> Ai;
            if (!NodeForgeInstallation::Read(Binding, Error)
                || !NodeForgeInstallation::AiConfiguration(Binding, Ai, UserDirectory, Error)) return;
        }
        ControlChannel = MakeShared<FNodeForgeEditorControlChannel>();
        ControlChannel->SetBusinessGate(SharedService->BusinessGate());
        ControlChannel->Publish(SharedService->GetState(), SharedService->GetDraft());
        FString Error;
        if (ControlChannel->Start(Error))
        {
            if (!LiveCodingPatch.IsValid())
                LiveCodingPatch = FModuleManager::GetModuleChecked<ILiveCodingModule>(TEXT("LiveCoding"))
                    .GetOnPatchCompleteDelegate().AddLambda([this]() { if (ControlChannel) ControlChannel->RequestStop(); });
            ProductChannel = ControlChannel;
            ControlReportRoot = ControlChannel->PublicIdentity()->GetStringField(TEXT("reportRoot"));
            ControlTicker = FTSTicker::GetCoreTicker().AddTicker(TEXT("NodeForgeControlCache"), 0.0f, [this](float)
            {
                ControlChannel->Drain(*SharedService);
                ControlChannel->Publish(SharedService->GetState(), SharedService->GetDraft());
                if (ControlChannel->HasStopRequest() && ControlChannel->Stop())
                {
                    ProductChannel.Reset(); ControlTicker.Reset();
                    auto Released = MakeShared<FJsonObject>(); Released->SetStringField(TEXT("state"), TEXT("disabled"));
                    Released->SetBoolField(TEXT("tickerSelfRemovalRequested"), true); Released->SetBoolField(TEXT("productChannelDetached"), !ProductChannel.IsValid());
                    Released->SetBoolField(TEXT("productServiceRetained"), SharedService.IsValid());
                    NodeForgePrototype::WriteJson(Released, ControlReportRoot / TEXT("ControlDisabled.json"));
                    ControlChannel.Reset();
                    return false; // FTSTicker removes this delegate after its callback returns.
                }
                return true;
            });
        }
        else { UE_LOG(LogTemp, Error, TEXT("NodeForge control start rejected: %s"), *Error); ControlChannel.Reset(); }
    }
    virtual void ShutdownModule() override
    {
        NodeForgeInstallation::OnReady().Remove(InstallationReady);
        if (const auto LiveCoding = FModuleManager::GetModulePtr<ILiveCodingModule>(TEXT("LiveCoding")))
            LiveCoding->GetOnPatchCompleteDelegate().Remove(LiveCodingPatch);
        if (ControlTicker.IsValid()) { FTSTicker::RemoveTicker(ControlTicker); ControlTicker.Reset(); }
        bool ControlStopped = true;
        if (ControlChannel) { ControlChannel->RequestStop(); ControlChannel->Drain(*SharedService); ControlStopped = ControlChannel->Stop(); ControlChannel.Reset(); }
        ProductChannel.Reset();
        if (!IsRunningCommandlet())
        {
            UToolMenus::UnRegisterStartupCallback(this);
            UToolMenus::UnregisterOwner("NodeForge");
            FGlobalTabmanager::Get()->UnregisterNomadTabSpawner("NodeForge");
        }
        ProductService.Reset();
        SharedService.Reset();
        if (!ControlReportRoot.IsEmpty())
        {
            auto Receipt = MakeShared<FJsonObject>(); Receipt->SetStringField(TEXT("state"), ControlStopped ? TEXT("released") : TEXT("completionUnknown"));
            Receipt->SetBoolField(TEXT("tickerHandleCleared"), !ControlTicker.IsValid()); Receipt->SetBoolField(TEXT("channelModuleReferenceReleased"), !ControlChannel);
            Receipt->SetBoolField(TEXT("productServiceModuleReferenceReleased"), !SharedService && !ProductService.IsValid());
            Receipt->SetBoolField(TEXT("menuStartupAndOwnerUnregistered"), true); Receipt->SetBoolField(TEXT("tabSpawnerUnregistered"), true);
            NodeForgePrototype::WriteJson(Receipt, ControlReportRoot / TEXT("ModuleShutdown.json"));
        }
    }

private:
    virtual bool SupportsDynamicReloading() override { return false; }
    FDelegateHandle InstallationReady;
    FDelegateHandle LiveCodingPatch;
    TSharedPtr<FNodeForgeEditorControlChannel> ControlChannel;
    FTSTicker::FDelegateHandle ControlTicker;
    FString ControlReportRoot;
    TSharedPtr<FNodeForgeEditorControlService> SharedService;
    void RegisterMenus()
    {
        FToolMenuOwnerScoped Owner("NodeForge");
        UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
        Menu->FindOrAddSection("WindowLayout").AddMenuEntry("NodeForgeOpen",
            FText::FromString(TEXT("NodeForge")), FText::FromString(TEXT("打开 JSON 构图与结果窗口")),
            FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([]() {
                FGlobalTabmanager::Get()->TryInvokeTab(FName("NodeForge"));
            })));
    }
};
IMPLEMENT_MODULE(FNodeForgeEditorModule, NodeForgeEditor);
