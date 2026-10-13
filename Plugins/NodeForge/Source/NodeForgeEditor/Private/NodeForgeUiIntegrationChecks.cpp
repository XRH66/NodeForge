// Finite observation of production Slate widgets in one explicitly owned Editor.
#include "NodeForgePanel.h"
#include "NodeForgePrototype.h"
#include "NodeForgeInstallation.h"
#include "NodeForgeEditorControlService.h"
#include "NodeForgePanelPreferences.h"
#include "NodeForgeControlOperations.h"
#include "NodeForgeEditorControlChannel.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Modules/ModuleManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBar.h"
#include "Widgets/Images/SImage.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Misc/Base64.h"
#include "ImageUtils.h"
#include "IAutomationDriverModule.h"
#include "IAutomationDriver.h"
#include "IDriverSequence.h"
#include "DriverConfiguration.h"
#include "Async/IAsyncTask.h"
#include "LocateBy.h"

namespace
{
class FUiPresentationCommand : public IAutomationLatentCommand
{
public:
    FUiPresentationCommand(FAutomationTestBase* InTest,FString InOutput):Test(InTest),Output(MoveTemp(InOutput)),Started(FPlatformTime::Seconds())
    {
        InputOnly=FParse::Param(FCommandLine::Get(),TEXT("NFUiInputInteractionChecks"));
        SettingsOnly=FParse::Param(FCommandLine::Get(),TEXT("NFUiSettingsChecks"));
        ClientOnly=FParse::Param(FCommandLine::Get(),TEXT("NFUiClientChecks"));
        FrameworkOnly=FParse::Param(FCommandLine::Get(),TEXT("NFUiFrameworkChecks"));
        ReadErrorOnly=FParse::Param(FCommandLine::Get(),TEXT("NFUiReadErrorChecks"));
        if(ReadErrorOnly)Steps={{TEXT("NodeForge.ExportGraph"),TEXT("ActualExportSelectionError")},{TEXT("NodeForge.InspectSelection"),TEXT("ActualInspectSelectionError")}};
        if(FrameworkOnly)
        {
            FString Error;Check(TEXT("OriginalFrameworkSelectionReadable"),OriginalSettings.Load(Error));
            Steps={{TEXT("NodeForge.AiMenu"),TEXT("ClaudeMenu")},{TEXT("NodeForge.ClientAction.claude"),TEXT("ClaudeMcpConfig")},{TEXT("NodeForge.CopyMcpConfig"),TEXT("ClaudeConfigCopied")},{TEXT("NodeForge.SettingsClose"),TEXT("")},
                {TEXT("NodeForge.AiMenu"),TEXT("DeepSeekMenu")},{TEXT("NodeForge.ClientAction.deepseek"),TEXT("DeepSeekMcpConfig")},{TEXT("NodeForge.CopyMcpConfig"),TEXT("DeepSeekConfigCopied")},{TEXT("NodeForge.SettingsClose"),TEXT("")},
                {TEXT("NodeForge.Settings"),TEXT("")},{TEXT("NodeForge.Settings.ai"),TEXT("PresetSelection")},{TEXT("NodeForge.Client.codex"),TEXT("CodexDraftSelection")},{TEXT("NodeForge.SettingsCancel"),TEXT("FrameworkDraftCancelled")}};
        }
        if(ClientOnly)
        {FString Error;Check(TEXT("OriginalClientSelectionReadable"),OriginalSettings.Load(Error));Steps={{TEXT("NodeForge.AiMenu"),TEXT("ActualConnectedClients")},{TEXT("NodeForge.ClientAction.codex"),TEXT("CodexDisconnectedOtherPeerPreserved")}};}
        if(SettingsOnly)
        {
            FString Error;Check(TEXT("OriginalSettingsReadable"),OriginalSettings.Load(Error));
            SettingsPath=NodeForgeInstallation::StateDirectory()/TEXT("Settings/Panel.json");OriginalSettingsSha=NodeForgePrototype::FileSha256(SettingsPath);
            TArray<uint8> Bytes;const FString Icon=FPaths::ProjectDir()/TEXT("Saved/NodeForge/UiIntegrationChecks/IconFixtures/Png.png");
            Check(TEXT("RealIconFixtureReadable"),FFileHelper::LoadFileToArray(Bytes,*Icon));
            const FString Name=TEXT("UiClient")+FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8);
            CustomName=Name;auto Client=MakeShared<FJsonObject>();Client->SetStringField(TEXT("name"),Name);Client->SetStringField(TEXT("description"),TEXT("自有验收客户端"));
            Client->SetStringField(TEXT("icon"),TEXT("data:image/png;base64,")+FBase64::Encode(Bytes));FJsonSerializer::Serialize(Client,TJsonWriterFactory<>::Create(&ClientImportText));
            Steps={{TEXT("NodeForge.Settings"),TEXT("")},{TEXT("NodeForge.SettingsDirectory"),TEXT("InvalidDirectory")},{TEXT("NodeForge.SettingsApply"),TEXT("InvalidSettingsRejected")},
                {TEXT("NodeForge.SettingsDirectory"),TEXT("ValidUnapplied")},{TEXT("NodeForge.Settings.about"),TEXT("UnappliedAbout")},{TEXT("NodeForge.SettingsClose"),TEXT("")},
                {TEXT("NodeForge.Settings"),TEXT("CancelledSettingsReopened")},{TEXT("NodeForge.SettingsDirectory"),TEXT("ValidForApply")},{TEXT("NodeForge.SettingsApply"),TEXT("AppliedSettings")},
                {TEXT("NodeForge.Settings.ai"),TEXT("")},{TEXT("NodeForge.ImportClient"),TEXT("")},{TEXT("NodeForge.ClientImport"),TEXT("ActualClientImport")},{TEXT("NodeForge.ClientAdd"),TEXT("CustomClientDraft")},
                {TEXT("NodeForge.SettingsApply"),TEXT("CustomClientApplied")},{TEXT("NodeForge.SettingsClose"),TEXT("")},{TEXT("NodeForge.Settings"),TEXT("PersistedSettingsReopened")},{TEXT("NodeForge.SettingsClose"),TEXT("")},
                {TEXT("NodeForge.LoadFile"),TEXT("")},{TEXT("NodeForge.LoadExample"),TEXT("ExampleUsesAppliedDirectory")},{TEXT("NodeForge.RestoreInput"),TEXT("RestoreEmptyAfterExample")}};
        }
        if(InputOnly)Steps={{TEXT("NodeForge.ResizeInput"),TEXT("AfterInputResize")},{TEXT("NodeForge.Paste"),TEXT("LongInput")},
            {TEXT("NodeForge.Input"),TEXT("LongInputScrolled")},{TEXT("NodeForge.RestoreInput"),TEXT("RestoredEmptyInput")},
            {TEXT("NodeForge.LoadFile"),TEXT("LoadDialog")},{TEXT("NodeForge.DialogClose"),TEXT("")}};
        if(FParse::Param(FCommandLine::Get(),TEXT("NFUiExampleChecks")))
            Steps={{TEXT("NodeForge.LoadFile"),TEXT("")},{TEXT("NodeForge.LoadExample"),TEXT("ExampleUsesAppliedDirectory")},{TEXT("NodeForge.RestoreInput"),TEXT("RestoreEmptyAfterExample")}};
    }
    ~FUiPresentationCommand() override { Release(); }
    bool Update() override
    {
        if(FPlatformTime::Seconds()-Started>90){Check(TEXT("Deadline"),false);return Finish();}
        if(Phase==0)
        {
            NodeForgeInstallation::FBinding Binding;FString Error;
            if(!Check(TEXT("OwnedBoundInstallation"),NodeForgeInstallation::Read(Binding,Error)))return Finish();
            Panel=NodeForgePanel::Create();
            Window=SNew(SWindow).Title(FText::FromString(TEXT("NodeForge UI integration"))).ClientSize(FVector2D(1200,760))
                .ScreenPosition(FVector2D(40,40)).AutoCenter(EAutoCenter::None).SupportsMaximize(false).SupportsMinimize(false)[Panel.ToSharedRef()];
            FSlateApplication::Get().AddWindow(Window.ToSharedRef());Window->BringToFront(true);
            Phase=(InputOnly||SettingsOnly||ClientOnly||FrameworkOnly)?4:1;ResizeStep=(InputOnly||SettingsOnly||ClientOnly||FrameworkOnly)?8:0;Changed=FPlatformTime::Seconds();return false;
        }
        if(FPlatformTime::Seconds()-Changed<0.35)return false;
        if(Phase>=1&&Phase<=3)
        {
            const int32 Widths[]={1200,800,500};Observe(Widths[Phase-1],FString::Printf(TEXT("Panel%d"),Widths[Phase-1]));
            if(Phase<3)Window->Resize(FVector2D(Widths[Phase],760));
            ++Phase;Changed=FPlatformTime::Seconds();return false;
        }
        if(Phase==4)
        {
            if(ClientOnly)
            {
                const auto Channel=NodeForgePanel::GetControlChannel();
                if(!Channel||!Channel->HasConnectedClient(TEXT("codex")))return false;
                const auto Clients=Channel->ConnectedClients();bool Peer=false;
                for(const auto& C:Clients)if(C->AsObject()->GetStringField(TEXT("name"))==TEXT("NodeForgeUiPeer"))Peer=true;
                if(!Peer)return false;
                BeforeClients=Clients;Check(TEXT("ActualAuthenticatedCodexAndPeer"),Clients.Num()==2);
                FNodeForgePanelPreferences Saved;FString Error;Check(TEXT("HandshakeDoesNotStealSelection"),Saved.Load(Error)&&Saved.Client==OriginalSettings.Client);
            }
            if(ResizeStep<8){const int32 Width=500+ResizeStep*100;Window->Resize(FVector2D(Width,760));++ResizeStep;Changed=FPlatformTime::Seconds();return false;}
            Observe(1200,TEXT("AfterContinuousResize"));
            auto& Module=IAutomationDriverModule::Get();
            if(!Check(TEXT("DriverAvailable"),!Module.IsEnabled()))return Finish();
            Module.Enable();OwnedDriver=true;auto Config=MakeShared<FDriverConfiguration,ESPMode::ThreadSafe>();Config->ImplicitWait=FTimespan::FromSeconds(3);
            Driver=Module.CreateAsyncDriver(Config);Phase=5;
        }
        if(Phase==5)
        {
            if(Step>=Steps.Num())return Finish();
            const FName Tag(*Steps[Step].Key);const TWeakPtr<SWindow> OwnedWindow=Window;
            const auto Locator=By::WidgetLambda([OwnedWindow,Tag](TArray<TSharedRef<SWidget>>& Out)
            {
                const auto Parent=OwnedWindow.Pin();if(!Parent)return;
                if(auto Control=NodeForgePanel::FindTaggedForChecks(Parent.ToSharedRef(),Tag))Out.Add(Control.ToSharedRef());
                for(const auto& Child:Parent->GetChildWindows())if(Child->IsVisible())
                    if(auto Control=NodeForgePanel::FindTaggedForChecks(Child,Tag))Out.Add(Control.ToSharedRef());
            });
            Sequence=Driver->CreateSequence();
            if(InputOnly&&Tag==TEXT("NodeForge.ResizeInput"))
                Sequence->Actions().MoveToElement(Locator).Press(EMouseButtons::Left).MoveByOffset(0,80).Release(EMouseButtons::Left);
            else
            {
                if(InputOnly&&Tag==TEXT("NodeForge.Paste"))
                {FString Long;for(int32 I=0;I<100;++I)Long+=FString::Printf(TEXT("line %d: %s\n"),I,*FString::ChrN(50,TEXT('a')));FPlatformApplicationMisc::ClipboardCopy(*Long);}
                Sequence->Actions().Click(Locator);
                if(Tag==TEXT("NodeForge.Input"))
                {if(InputOnly)Sequence->Actions().ScrollBy(Locator,-12);else Sequence->Actions().Type(TEXT("{"));}
            }
            Pending=Sequence->Perform();Phase=6;return false;
        }
        if(Phase==6)
        {
            if(!Pending.GetFuture().IsReady())return false;
            if(!Check(*(TEXT("Click.")+Steps[Step].Key),Pending.GetFuture().Get()))return Finish();
            Phase=7;Changed=FPlatformTime::Seconds();return false;
        }
        if(Phase==7)
        {
            if(ReadErrorOnly)
            {
                const auto Service=NodeForgePanel::GetProductService();
                const auto Source=NodeForgePanel::FindTaggedForChecks(Panel.ToSharedRef(),TEXT("NodeForge.InputSource"));
                const FString Expected=Steps[Step].Key==TEXT("NodeForge.ExportGraph")?TEXT("构图导出未执行："):TEXT("只读检查未执行：");
                Check(TEXT("ActualSelectionErrorVisibleWithoutReport"),Service&&Source&&Service->State().LastReportPath.IsEmpty()
                    &&Service->State().ResultText.StartsWith(Expected)
                    &&StaticCastSharedPtr<STextBlock>(Source)->GetText().ToString().StartsWith(Expected));
                Check(TEXT("SelectionErrorDoesNotGenerateOrChangeDraft"),Service&&Service->State().LastAssetPath.IsEmpty()&&Service->State().Input.Draft().Text.IsEmpty());
            }
            if(FrameworkOnly&&Steps[Step].Value.EndsWith(TEXT("ConfigCopied")))
            {
                FString Copied;FPlatformApplicationMisc::ClipboardPaste(Copied);TSharedPtr<FJsonObject> Config;
                TSharedPtr<SWidget> Text;
                for(const auto& Child:Window->GetChildWindows())if(Child->IsVisible())Text=NodeForgePanel::FindTaggedForChecks(Child,TEXT("NodeForge.McpConfigText"));
                Check(TEXT("ActualFrameworkConfigurationCopied"),Text&&Copied==StaticCastSharedPtr<SMultiLineEditableTextBox>(Text)->GetText().ToString()
                    &&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Copied),Config)&&Config&&Config->HasTypedField<EJson::Object>(TEXT("mcpServers")));
            }
            if(FrameworkOnly&&Steps[Step].Value==TEXT("FrameworkDraftCancelled"))
            {FNodeForgePanelPreferences Saved;FString Error;Check(TEXT("FrameworkViewAndCancelledSelectionPreserveUser"),Saved.Load(Error)&&Saved.Equals(OriginalSettings));}
            if(ClientOnly&&Steps[Step].Value==TEXT("CodexDisconnectedOtherPeerPreserved"))
            {
                const auto Channel=NodeForgePanel::GetControlChannel();
                const auto Clients=Channel?Channel->ConnectedClients():TArray<TSharedPtr<FJsonValue>>();bool Peer=false;
                for(const auto& Before:BeforeClients)if(Before->AsObject()->GetStringField(TEXT("name"))==TEXT("NodeForgeUiPeer"))
                    for(const auto& After:Clients)if(After->AsObject()->GetStringField(TEXT("id"))==Before->AsObject()->GetStringField(TEXT("id")))Peer=true;
                Check(TEXT("CodexOnlyDisconnected"),Channel&&!Channel->IsStopped()&&!Channel->HasConnectedClient(TEXT("codex"))&&Peer&&Clients.Num()==1);
                FNodeForgePanelPreferences Saved;FString Error;Check(TEXT("DisconnectDoesNotStealSelection"),Saved.Load(Error)&&Saved.Client==OriginalSettings.Client);
                AfterClients=Clients;
            }
            if(SettingsOnly)
            {
                const FString Name=Steps[Step].Value;
                if(Name==TEXT("ExampleUsesAppliedDirectory"))
                {auto Detail=MakeShared<FJsonObject>();Detail->SetStringField(TEXT("case"),TEXT("ExampleActualNotice"));Detail->SetBoolField(TEXT("passed"),true);Detail->SetStringField(TEXT("notice"),NodeForgePanel::GetProductService()->State().ResultNotice);Rows.Add(MakeShared<FJsonValueObject>(Detail));}
                // UE5.3's driver leaves platform message handling enabled, so its
                // tracked modifier state is not returned by GetModifierKeys.
                // Send actual Slate key events to the driver-focused control;
                // never replace its text or call the settings handler directly.
                if(Steps[Step].Key==TEXT("NodeForge.SettingsDirectory")||Steps[Step].Key==TEXT("NodeForge.ClientImport"))
                {
                    auto& App=FSlateApplication::Get();const bool Import=Steps[Step].Key==TEXT("NodeForge.ClientImport");
                    if(Import)FPlatformApplicationMisc::ClipboardCopy(*ClientImportText);
                    const FKey Key=Import?EKeys::V:EKeys::A;
                    const FModifierKeysState Ctrl(false,false,true,false,false,false,false,false,false);
                    Check(TEXT("ActualSlateShortcutConsumed"),App.ProcessKeyDownEvent(FKeyEvent(Key,Ctrl,0,false,0,0)));
                    App.ProcessKeyUpEvent(FKeyEvent(Key,FModifierKeysState(),0,false,0,0));
                    if(!Import)
                    {
                        const FString Text=Name==TEXT("InvalidDirectory")?TEXT("/Game/Outside"):TEXT("/Game/NodeForgeTests/UiSettings");
                        for(TCHAR C:Text)App.ProcessKeyCharEvent(FCharacterEvent(C,FModifierKeysState(),0,false));
                    }
                }
                if(Name==TEXT("InvalidSettingsRejected")||Name==TEXT("UnappliedAbout")||Name==TEXT("CancelledSettingsReopened"))
                    Check(TEXT("InvalidCancelAndAboutPreserveActualSettings"),NodeForgePrototype::FileSha256(SettingsPath)==OriginalSettingsSha);
                if(Name==TEXT("AppliedSettings"))
                {FNodeForgePanelPreferences Saved;FString Error;Check(TEXT("ApplyPersistsActualDirectory"),Saved.Load(Error)&&Saved.Directory==TEXT("/Game/NodeForgeTests/UiSettings"));}
                if(Name==TEXT("PersistedSettingsReopened"))
                {
                    FNodeForgePanelPreferences Saved;FString Error;bool HasIcon=false;
                    Check(TEXT("ReopenLoadsAppliedCustomClient"),Saved.Load(Error));
                    for(const auto& C:Saved.CustomClients)if(C.Name==CustomName)HasIcon=!C.IconPath.IsEmpty()&&FPaths::IsUnderDirectory(C.IconPath,NodeForgeInstallation::StateDirectory()/TEXT("Settings/ClientIcons"))&&FPaths::FileExists(C.IconPath)&&C.PendingIconPng.IsEmpty();
                    Check(TEXT("ImportedIconOwnedAndPersisted"),HasIcon&&Saved.Client.StartsWith(TEXT("custom-")));
                }
                if(Name==TEXT("ExampleUsesAppliedDirectory"))
                {TSharedPtr<FJsonObject> Graph;const auto S=NodeForgePanel::GetProductService();Check(TEXT("ActualExampleUsesAppliedDirectoryWithoutGenerate"),S&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S->State().Input.Draft().Text),Graph)&&Graph
                    &&Graph->GetStringField(TEXT("assetPath")).StartsWith(TEXT("/Game/NodeForgeTests/UiSettings/"))&&S->State().LastAssetPath.IsEmpty());}
                if(Name==TEXT("RestoreEmptyAfterExample"))Check(TEXT("LoadedExampleRestoresEmpty"),NodeForgePanel::GetProductService()->State().Input.Draft().Text.IsEmpty());
            }
            if(InputOnly)
            {
                const auto Input=NodeForgePanel::FindTaggedForChecks(Panel.ToSharedRef(),TEXT("NodeForge.Input"));
                if(Steps[Step].Key==TEXT("NodeForge.ResizeInput"))Check(TEXT("NativeDragIncreasesInputBy80"),Input&&FMath::Abs(Input->GetCachedGeometry().GetLocalSize().Y-323)<2);
                if(Steps[Step].Value==TEXT("LongInputScrolled"))
                {
                    const auto Bar=NodeForgePanel::FindTaggedForChecks(Panel.ToSharedRef(),TEXT("NodeForge.InputScrollbar"));
                    Check(TEXT("ActualWheelScrollMovesNativeThumb"),Bar&&StaticCastSharedPtr<SScrollBar>(Bar)->IsNeeded()&&StaticCastSharedPtr<SScrollBar>(Bar)->DistanceFromTop()>0);
                }
                if(Steps[Step].Key==TEXT("NodeForge.RestoreInput"))Check(TEXT("RestoresActualEmptyDraft"),Input&&StaticCastSharedPtr<SMultiLineEditableTextBox>(Input)->GetText().IsEmpty());
            }
            if(Steps[Step].Key==TEXT("NodeForge.CopyReport"))
            {
                FString Copied,Actual;FPlatformApplicationMisc::ClipboardPaste(Copied);
                const auto Service=NodeForgePanel::GetProductService();
                Check(TEXT("ClipboardContainsFullActualReport"),Service&&FFileHelper::LoadFileToString(Actual,*Service->State().LastReportPath)&&Actual==Copied&&!Copied.IsEmpty());
            }
            if(!Steps[Step].Value.IsEmpty())
            {
                auto Visible=Window;
                for(const auto& Child:Window->GetChildWindows())if(Child->IsVisible())Visible=Child;
                if(Steps[Step].Value==TEXT("LoadDialog"))
                {
                    const auto Icon=NodeForgePanel::FindTaggedForChecks(Visible.ToSharedRef(),TEXT("NodeForge.BrowseIcon"));
                    Check(TEXT("ActualBrowseIconHasGeometry"),Icon&&Icon->GetCachedGeometry().GetLocalSize().GetMin()>8);
                    if(Icon){const auto Brush=FAppStyle::GetBrush(TEXT("Icons.FolderOpen"));auto Info=MakeShared<FJsonObject>();
                        Info->SetStringField(TEXT("brush"),Brush?Brush->GetResourceName().ToString():TEXT("absent"));
                        Info->SetNumberField(TEXT("drawAs"),Brush?int32(Brush->DrawAs):-1);Info->SetNumberField(TEXT("width"),Icon->GetCachedGeometry().GetLocalSize().X);
                        Info->SetNumberField(TEXT("height"),Icon->GetCachedGeometry().GetLocalSize().Y);Images.Add(MakeShared<FJsonValueObject>(Info));}
                }
                Picture(Visible.ToSharedRef(),Steps[Step].Value);
            }
            ++Step;Phase=5;return false;
        }
        return false;
    }
private:
    bool Check(const TCHAR* Name,bool Passed)
    {
        All&=Passed;Test->TestTrue(Name,Passed);auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("case"),Name);Item->SetBoolField(TEXT("passed"),Passed);Rows.Add(MakeShared<FJsonValueObject>(Item));return Passed;
    }
    void Picture(const TSharedRef<SWidget>& Widget,const FString& Name)
    {
        const FString File=FPaths::GetPath(Output)/(Name+TEXT(".png"));TArray<FColor> Pixels;FIntVector Size=FIntVector::ZeroValue;
        if(!Check(*(TEXT("Capture.")+Name),!FPaths::FileExists(File)&&FSlateApplication::Get().TakeScreenshot(Widget,Pixels,Size)&&Size.X>0&&Size.Y>0))return;
        TArray64<uint8> Bytes;FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,Bytes);Check(*(TEXT("Save.")+Name),FFileHelper::SaveArrayToFile(Bytes,*File));
        auto Image=MakeShared<FJsonObject>();Image->SetStringField(TEXT("name"),Name);Image->SetStringField(TEXT("path"),File);Image->SetNumberField(TEXT("pixelWidth"),Size.X);Image->SetNumberField(TEXT("pixelHeight"),Size.Y);Images.Add(MakeShared<FJsonValueObject>(Image));
    }
    void Observe(int32 Requested,const FString& Name)
    {
        auto Find=[this](const TCHAR* Tag){return NodeForgePanel::FindTaggedForChecks(Panel.ToSharedRef(),FName(Tag));};
        const auto Input=Find(TEXT("NodeForge.Input"));
        Check(TEXT("EmptyRealInput"),Input&&StaticCastSharedPtr<SMultiLineEditableTextBox>(Input)->GetText().IsEmpty());
        Check(TEXT("InputHeight245"),Input&&FMath::Abs(Input->GetCachedGeometry().GetLocalSize().Y-243)<2);
        const auto Source=Find(TEXT("NodeForge.InputSource"));
        Check(TEXT("EmptySourceZeroBytes"),Source&&StaticCastSharedPtr<STextBlock>(Source)->GetText().ToString().Contains(TEXT("0 / 65536")));
        Check(TEXT("GroupsAlwaysPresent"),Find(TEXT("NodeForge.CopySection"))&&Find(TEXT("NodeForge.RuleSection")));
        auto Geometry=MakeShared<FJsonObject>();Geometry->SetStringField(TEXT("name"),Name);Geometry->SetNumberField(TEXT("requestedWidth"),Requested);
        Geometry->SetNumberField(TEXT("actualWidth"),Panel->GetCachedGeometry().GetLocalSize().X);Geometry->SetNumberField(TEXT("scale"),Panel->GetCachedGeometry().Scale);
        TArray<TSharedPtr<FJsonValue>> Controls;
        for(const TCHAR* Tag:{TEXT("NodeForge.Paste"),TEXT("NodeForge.LoadFile"),TEXT("NodeForge.RestoreInput"),TEXT("NodeForge.PrepareCopy"),TEXT("NodeForge.Preflight"),TEXT("NodeForge.Generate"),TEXT("NodeForge.ExportGraph"),TEXT("NodeForge.PreviewGraph"),TEXT("NodeForge.Relayout"),TEXT("NodeForge.RulesMaterial"),TEXT("NodeForge.RulesBlueprint"),TEXT("NodeForge.Catalog"),TEXT("NodeForge.InspectSelection"),TEXT("NodeForge.CopyReadOnly")})
        {
            const auto Widget=Find(Tag);if(!Check(Tag,Widget.IsValid()))continue;
            const auto G=Widget->GetCachedGeometry();const auto Start=Panel->GetCachedGeometry().AbsoluteToLocal(G.GetAbsolutePosition());const auto End=Panel->GetCachedGeometry().AbsoluteToLocal(G.LocalToAbsolute(G.GetLocalSize()));
            Check(TEXT("Control112x24"),FMath::Abs(G.GetLocalSize().X-112)<1&&FMath::Abs(G.GetLocalSize().Y-24)<1);
            Check(TEXT("NoHorizontalOverflow"),Start.X>=-0.5&&End.X<=Panel->GetCachedGeometry().GetLocalSize().X+0.5);
            auto Row=MakeShared<FJsonObject>();Row->SetStringField(TEXT("tag"),Tag);Row->SetNumberField(TEXT("left"),Start.X);Row->SetNumberField(TEXT("right"),End.X);Row->SetNumberField(TEXT("top"),Start.Y);Row->SetNumberField(TEXT("bottom"),End.Y);Row->SetBoolField(TEXT("enabled"),Widget->IsEnabled());Controls.Add(MakeShared<FJsonValueObject>(Row));
        }
        for(const TCHAR* Tag:{TEXT("NodeForge.RestoreInput"),TEXT("NodeForge.PrepareCopy"),TEXT("NodeForge.Preflight"),TEXT("NodeForge.Generate"),TEXT("NodeForge.PreviewGraph"),TEXT("NodeForge.Relayout"),TEXT("NodeForge.CopyReadOnly")})
        {const auto Widget=Find(Tag);Check(TEXT("EmptyDependenciesDisabled"),Widget&&!Widget->IsEnabled());}
        Geometry->SetArrayField(TEXT("controls"),Controls);Geometries.Add(MakeShared<FJsonValueObject>(Geometry));Picture(Panel.ToSharedRef(),Name);
    }
    void Release()
    {
        if(!OwnedDriver)return;
        if(Pending.GetFuture().IsValid()&&!Pending.GetFuture().IsReady())if(auto Task=Pending.GetTask())Task->Cancel();
        Sequence.Reset();Driver.Reset();IAutomationDriverModule::Get().Disable();OwnedDriver=false;
    }
    bool Finish()
    {
        Release();All=All&&!Test->HasAnyErrors();auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("status"),All?TEXT("passed"):TEXT("failed"));Report->SetArrayField(TEXT("checks"),Rows);Report->SetArrayField(TEXT("geometry"),Geometries);Report->SetArrayField(TEXT("images"),Images);
        Report->SetNumberField(TEXT("pid"),FPlatformProcess::GetCurrentProcessId());const FString Module=FModuleManager::Get().GetModuleFilename(TEXT("NodeForgeEditor"));Report->SetStringField(TEXT("module"),Module);Report->SetStringField(TEXT("binarySha256"),NodeForgePrototype::FileSha256(Module));
        Report->SetStringField(TEXT("scope"),ClientOnly?TEXT("Actual authenticated Codex and SDK peer; production Slate disconnect and selection preservation, no assets."):TEXT("Actual production factory, native geometry and presentation/settings interactions; no assets or final release acceptance."));
        Report->SetObjectField(TEXT("editorOperations"),NodeForgeControlOperations::DescribeEditor(15));
        if(ClientOnly){Report->SetArrayField(TEXT("clientsBefore"),BeforeClients);Report->SetArrayField(TEXT("clientsAfter"),AfterClients);}
        Check(TEXT("WriteFreshReport"),NodeForgePrototype::WriteJson(Report,Output));if(Window)Window->RequestDestroyWindow();FPlatformMisc::RequestExitWithStatus(false,All?0:1);return true;
    }
    FAutomationTestBase* Test;FString Output;TSharedPtr<SWidget> Panel;TSharedPtr<SWindow> Window;
    TSharedPtr<IAsyncAutomationDriver,ESPMode::ThreadSafe> Driver;TSharedPtr<IAsyncDriverSequence,ESPMode::ThreadSafe> Sequence;TAsyncResult<bool> Pending;
    FNodeForgePanelPreferences OriginalSettings;
    FString SettingsPath,OriginalSettingsSha,CustomName,ClientImportText;
    TArray<TSharedPtr<FJsonValue>> Rows,Images,Geometries,BeforeClients,AfterClients;double Started,Changed=0;int32 Phase=0,ResizeStep=0,Step=0;bool All=true,OwnedDriver=false,InputOnly=false,SettingsOnly=false,ClientOnly=false,FrameworkOnly=false,ReadErrorOnly=false;
    TArray<TPair<FString,FString>> Steps={
        {TEXT("NodeForge.Settings"),TEXT("SettingsGeneral")},{TEXT("NodeForge.Settings.ai"),TEXT("SettingsAi")},
        {TEXT("NodeForge.AddClient"),TEXT("CustomClient")},{TEXT("NodeForge.ClientReturn"),TEXT("")},
        {TEXT("NodeForge.ImportClient"),TEXT("ImportClient")},{TEXT("NodeForge.ClientReturn"),TEXT("")},
        {TEXT("NodeForge.Settings.mcp"),TEXT("SettingsMcp")},{TEXT("NodeForge.Settings.about"),TEXT("SettingsAbout")},
        {TEXT("NodeForge.SettingsClose"),TEXT("")},
        {TEXT("NodeForge.Input"),TEXT("")},{TEXT("NodeForge.Preflight"),TEXT("RealPreflightFailure")},
        {TEXT("NodeForge.Report"),TEXT("RealReportDialog")},{TEXT("NodeForge.CopyReport"),TEXT("")},{TEXT("NodeForge.DialogClose"),TEXT("")},
        {TEXT("NodeForge.RestoreInput"),TEXT("")},{TEXT("NodeForge.LoadFile"),TEXT("LoadDialog")},{TEXT("NodeForge.DialogClose"),TEXT("")},
        {TEXT("NodeForge.AiMenu"),TEXT("AiDropdown")}};
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUiIntegrationPresentationTest,"NodeForge.UiIntegrationPresentation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FUiIntegrationPresentationTest::RunTest(const FString&)
{
    FString Output;
    if(IsRunningCommandlet()||!FSlateApplication::IsInitialized()||!FParse::Param(FCommandLine::Get(),TEXT("NFUiIntegrationChecks"))
        ||(FPaths::GetBaseFilename(FPaths::GetProjectFilePath())!=TEXT("NFCUiPreview")
            &&!(FParse::Param(FCommandLine::Get(),TEXT("NFUiProjectIsolationChecks"))&&FPaths::GetBaseFilename(FPaths::GetProjectFilePath())==TEXT("NFCUiPeer")))
        ||!FParse::Value(FCommandLine::Get(),TEXT("NFReport="),Output)||FPaths::FileExists(Output)
        ||!FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Output),FPaths::ProjectDir()/TEXT("Saved/NodeForge/UiIntegrationChecks")))
    {AddError(TEXT("Explicit owned NFCUiPreview Editor and fresh project-bounded report required."));return false;}
    ADD_LATENT_AUTOMATION_COMMAND(FUiPresentationCommand(this,Output));return true;
}
#endif
