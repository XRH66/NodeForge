#include "NodeForgePanel.h"
#include "Modules/ModuleManager.h"
#include "NodeForgePrototype.h"
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
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SScrollBar.h"
#include "Widgets/SWindow.h"
#include "Brushes/SlateImageBrush.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateNoResource.h"
#include "InputCoreTypes.h"
#include "NodeForgeProductInfo.h"
#include "NodeForgePanelPreferences.h"
#include "NodeForgeGraphCodec.h"
#include "Misc/Base64.h"
#include "Windows/WindowsHWrapper.h"

// The image widget owns its brush, including in independently closing windows.
class SNodeForgeClientIcon : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SNodeForgeClientIcon) : _Size(22,22) {}
        SLATE_ARGUMENT(FString, Path)
        SLATE_ARGUMENT(FString, Letter)
        SLATE_ARGUMENT(FVector2D, Size)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        if(!Args._Path.IsEmpty()&&IFileManager::Get().FileExists(*Args._Path))
        {
            if(FPaths::GetExtension(Args._Path)==TEXT("svg"))Brush=MakeShared<FSlateVectorImageBrush>(Args._Path,Args._Size);
            else Brush=MakeShared<FSlateDynamicImageBrush>(FName(*Args._Path),Args._Size);
            ChildSlot[SNew(SBox).WidthOverride(Args._Size.X).HeightOverride(Args._Size.Y)[SNew(SImage).Image(Brush.Get())]];
        }
        else ChildSlot[SNew(SBox).WidthOverride(Args._Size.X).HeightOverride(Args._Size.Y).HAlign(HAlign_Center).VAlign(VAlign_Center)
            [SNew(STextBlock).Text(FText::FromString(Args._Letter.Left(1)))]];
    }
private:
    TSharedPtr<FSlateBrush> Brush;
};

// Animate only positional reflow. Layout, text size and hit targets stay native.
class SNodeForgeMotion : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SNodeForgeMotion) {} SLATE_DEFAULT_SLOT(FArguments, Content) SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        BOOL Animate=1;
        bAnimate=SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION,0,&Animate,0)&&Animate;
        ChildSlot[Args._Content.Widget];
    }
    virtual void Tick(const FGeometry& Geometry,double Time,float Delta) override
    {
        SCompoundWidget::Tick(Geometry,Time,Delta);
        const FVector2D Next=Geometry.GetAccumulatedLayoutTransform().GetTranslation();
        const float Scale=Geometry.GetAccumulatedLayoutTransform().GetScale();
        if(bInitialized&&bAnimate&&Scale>0&&!Next.Equals(Previous,0.5f))
        { Offset=Offset+(Previous-Next)/Scale;Started=Time; }
        Previous=Next;bInitialized=true;
        const float Progress=FMath::Clamp(float((Time-Started)/0.14),0.0f,1.0f);
        const float Remaining=FMath::Pow(1.0f-Progress,3.0f);
        SetRenderTransform(FSlateRenderTransform(Offset*Remaining));
        if(Progress>=1)Offset=FVector2D::ZeroVector;
    }
private:
    FVector2D Previous=FVector2D::ZeroVector,Offset=FVector2D::ZeroVector;
    double Started=0;
    bool bAnimate=true,bInitialized=false;
};


// Shared by the header, input tools and inline results. Narrowing only reflows
// the two groups; their individual left/right alignment never changes.
class SNodeForgeAlignedRow : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SNodeForgeAlignedRow) : _LeftWidthHint(180.0f) {}
        SLATE_NAMED_SLOT(FArguments, Left)
        SLATE_NAMED_SLOT(FArguments, Right)
        SLATE_ARGUMENT(float, LeftWidthHint)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Left=Args._Left.Widget;Right=Args._Right.Widget;LeftWidthHint=Args._LeftWidthHint;Rebuild(false);
    }
    virtual void Tick(const FGeometry& Geometry,double Time,float Delta) override
    {
        SCompoundWidget::Tick(Geometry,Time,Delta);
        const float Width=Geometry.GetLocalSize().X;
        if(Width<=0)return;
        const bool Wrap=Width<LeftWidthHint+Right->GetDesiredSize().X+12;
        if(Wrap!=bWrapped)Rebuild(Wrap);
    }
private:
    void Rebuild(bool Wrap)
    {
        bWrapped=Wrap;
        if(Wrap)ChildSlot[SNew(SVerticalBox)
            +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Fill)[Left.ToSharedRef()]
            +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0,6,0,0)[Right.ToSharedRef()]];
        else ChildSlot[SNew(SHorizontalBox)
            +SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Fill).VAlign(VAlign_Center).Padding(0,0,12,0)[Left.ToSharedRef()]
            +SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Right).VAlign(VAlign_Center)[Right.ToSharedRef()]];
    }
    TSharedPtr<SWidget> Left,Right;
    float LeftWidthHint=180;
    bool bWrapped=false;
};

class SNodeForgePanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SNodeForgePanel) {}
        SLATE_ARGUMENT(FString, ReviewReportPath)
        SLATE_ARGUMENT(TSharedPtr<FNodeForgeEditorControlService>, SharedService)
    SLATE_END_ARGS()

    void Construct(const FArguments& Arguments)
    {
        Service = Arguments._SharedService.IsValid() ? Arguments._SharedService : MakeShared<FNodeForgeEditorControlService>();
        Service->AttachPanel();
        if(!Arguments._ReviewReportPath.IsEmpty())
        {
            State().LastReportPath=Arguments._ReviewReportPath;TSharedPtr<FJsonObject> Report;
            if(!ReadCurrentReport(Report))State().LastReportPath.Empty();
        }
        const FSlateColor Muted=FSlateColor::UseSubduedForeground();
        PreferencesLoaded=Preferences.Load(PreferencesError);
        BrowseButtonStyle=FAppStyle::Get().GetWidgetStyle<FButtonStyle>(TEXT("Button"));
        BrowseButtonStyle.SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
        const auto Plugin=IPluginManager::Get().FindPlugin(TEXT("NodeForge"));
        const FString Resources=Plugin ? Plugin->GetBaseDir()/TEXT("Resources/Ui") : FString();
        Brand=MakeShared<FSlateDynamicImageBrush>(FName(*(Resources/TEXT("BrandIcon.png"))),FVector2D(23,23));
        Wordmark=MakeShared<FSlateVectorImageBrush>(Resources/TEXT("WordmarkDark.svg"),FVector2D(176,25));
        InputStyle=FAppStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>(TEXT("NormalEditableTextBox"));
        InputStyle.BackgroundImageNormal=FSlateNoResource();InputStyle.BackgroundImageHovered=FSlateNoResource();
        InputStyle.BackgroundImageFocused=FSlateNoResource();InputStyle.BackgroundImageReadOnly=FSlateNoResource();
        InputStyle.TextStyle.SetFont(FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>(TEXT("MonospacedText")).Font);
        SAssignNew(InputScrollbar,SScrollBar).Tag(TEXT("NodeForge.InputScrollbar")).Thickness(FVector2D(6,6)).AlwaysShowScrollbar(false)
            .AlwaysShowScrollbarTrack(false).HideWhenNotInUse(true);
        SAssignNew(PasteBox,SMultiLineEditableTextBox).Tag(TEXT("NodeForge.Input")).Style(&InputStyle)
            .Text(FText::FromString(State().Input.Draft().Text))
            .HintText(FText::FromString(TEXT("在这里粘贴 NodeForgeGraph JSON\n也可以载入 JSON 文件。")))
            .Padding(0).Margin(FMargin(12,8)).LineHeightPercentage(1.5f).VScrollBarPadding(FMargin(0)).VScrollBar(InputScrollbar).AutoWrapText(false)
            .IsEnabled_Lambda([this](){return !Service->IsBusy();}).OnTextChanged(this,&SNodeForgePanel::DraftChanged);
        const auto Paste=Action(TEXT("粘贴"),TEXT("NodeForge.Paste"),FOnClicked::CreateSP(this,&SNodeForgePanel::PasteInput),[this](){return !Service->IsBusy();});
        const auto Load=Action(TEXT("载入"),TEXT("NodeForge.LoadFile"),FOnClicked::CreateSP(this,&SNodeForgePanel::ShowLoadDialog),[this](){return !Service->IsBusy();});
        const auto MainActions=SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6,6))
            +SWrapBox::Slot()[Action(TEXT("恢复输入"),TEXT("NodeForge.RestoreInput"),FOnClicked::CreateSP(this,&SNodeForgePanel::RestoreInput),[this](){return !Service->IsBusy()&&State().Input.CanRestore();})]
            +SWrapBox::Slot()[Action(TEXT("新建副本"),TEXT("NodeForge.PrepareCopy"),FOnClicked::CreateSP(this,&SNodeForgePanel::PrepareNewCopy),[this](){return CanUseDraft();})]
            +SWrapBox::Slot()[Action(TEXT("检查"),TEXT("NodeForge.Preflight"),FOnClicked::CreateSP(this,&SNodeForgePanel::PreflightGraph),[this](){return CanUseDraft();})]
            +SWrapBox::Slot()[Action(TEXT("生成"),TEXT("NodeForge.Generate"),FOnClicked::CreateSP(this,&SNodeForgePanel::ImportPaste),[this](){return CanUseDraft();},true)];
        const auto Copies=SNew(SWrapBox).Tag(TEXT("NodeForge.CopySection")).UseAllottedSize(true).InnerSlotPadding(FVector2D(6,6))
            +SWrapBox::Slot()[Action(TEXT("导出构图"),TEXT("NodeForge.ExportGraph"),FOnClicked::CreateSP(this,&SNodeForgePanel::ExportSelectedGraph),[this](){return !Service->IsBusy();})]
            +SWrapBox::Slot()[Action(TEXT("查看差异"),TEXT("NodeForge.PreviewGraph"),FOnClicked::CreateSP(this,&SNodeForgePanel::PreviewGraph),[this](){return !Service->IsBusy()&&State().Input.Draft().ExportOrigin.IsValid();})]
            +SWrapBox::Slot()[Action(TEXT("复用上次构图"),TEXT("NodeForge.Relayout"),FOnClicked::CreateSP(this,&SNodeForgePanel::RelayoutSuccess),[this](){return !Service->IsBusy()&&!State().LastImportInputPath.IsEmpty();})];
        const auto Rules=SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6,6))
            +SWrapBox::Slot()[Action(TEXT("材质规则"),TEXT("NodeForge.RulesMaterial"),FOnClicked::CreateSP(this,&SNodeForgePanel::MaterialRules),[this](){return !Service->IsBusy();})]
            +SWrapBox::Slot()[Action(TEXT("蓝图规则"),TEXT("NodeForge.RulesBlueprint"),FOnClicked::CreateSP(this,&SNodeForgePanel::BlueprintRules),[this](){return !Service->IsBusy();})]
            +SWrapBox::Slot()[Action(TEXT("导出规则"),TEXT("NodeForge.Catalog"),FOnClicked::CreateSP(this,&SNodeForgePanel::RunReadOnly),[this](){return !Service->IsBusy();})]
            +SWrapBox::Slot()[Action(TEXT("读取资源"),TEXT("NodeForge.InspectSelection"),FOnClicked::CreateSP(this,&SNodeForgePanel::InspectSelectedResource),[this](){return !Service->IsBusy();})]
            +SWrapBox::Slot()[Action(TEXT("复制内容"),TEXT("NodeForge.CopyReadOnly"),FOnClicked::CreateSP(this,&SNodeForgePanel::CopyReadOnly),[this](){return !Service->IsBusy()&&!State().LastReadOnlyData.IsEmpty();})];
        ChildSlot
        [SNew(SBorder).BorderImage(&Background).Padding(12)
            [SNew(SScrollBox).Tag(TEXT("NodeForge.PanelScroll"))
                +SScrollBox::Slot()
                [SNew(SVerticalBox)
                    +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,6)
                    [SNew(SNodeForgeAlignedRow).LeftWidthHint(208)
                        .Left()
                        [SNew(SHorizontalBox)
                            +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,9,0)[SNew(SImage).Image(Brand.Get())]
                            +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SImage).Image(Wordmark.Get())]]
                        .Right()
                        [SNew(SHorizontalBox)
                            +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,10,0)
                            [TextAction(TEXT("使用手册"),TEXT("NodeForge.Manual"),FOnClicked::CreateSP(this,&SNodeForgePanel::OpenManual),[](){return true;})]
                            +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,6,0)
                            [SNew(SBox).MinDesiredWidth(112).MaxDesiredWidth(200).HeightOverride(24)
                             [SAssignNew(AiButton,SComboButton).Tag(TEXT("NodeForge.AiMenu")).HasDownArrow(true)
                                .ContentPadding(FMargin(6,2)).OnGetMenuContent(this,&SNodeForgePanel::BuildAiMenu)
                                .ButtonContent()[SNew(SHorizontalBox)
                                    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,5,0)[SAssignNew(CurrentIconBox,SBox).WidthOverride(16).HeightOverride(16)]
                                    +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[SNew(STextBlock).Text_Lambda([this](){return FText::FromString(CurrentClientName());})]
                                    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5,0,0,0)
                                        [SNew(STextBlock).Text(FText::FromString(TEXT("●"))).ColorAndOpacity_Lambda([this](){const auto Channel=NodeForgePanel::GetControlChannel();
                                            return Channel&&Channel->HasConnectedClient(Preferences.Client)?FSlateColor(FLinearColor::FromSRGBColor(FColor(70,174,118))):FSlateColor::UseSubduedForeground();})]]]]
                            +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                            [SNew(SBox).WidthOverride(24).HeightOverride(24)
                                [SNew(SButton).Tag(TEXT("NodeForge.Settings")).ButtonStyle(FAppStyle::Get(),TEXT("SimpleButton"))
                                    .ToolTipText(FText::FromString(TEXT("NodeForge 设置"))).ContentPadding(3)
                                    .OnClicked(this,&SNodeForgePanel::OpenSettings)
                                    [SNew(SImage).Image(FAppStyle::GetBrush(TEXT("Icons.Settings")))]]]]]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,2,0,8)
                    [SNew(STextBlock).ColorAndOpacity(Muted).AutoWrapText(true).Text_Lambda([]()
                        {return FText::FromString(FString::Printf(TEXT("工程：%s · UE %s"),*FPaths::GetBaseFilename(FPaths::GetProjectFilePath()),
                            *FEngineVersion::Current().ToString(EVersionComponent::Patch)));})]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,4)
                    [SNew(SNodeForgeAlignedRow).LeftWidthHint(100)
                        .Left()[SNew(STextBlock).Text(FText::FromString(TEXT("构图输入")))]
                        .Right()[SNew(SWrapBox).PreferredSize(230).HAlign(HAlign_Right).InnerSlotPadding(FVector2D(6,6))
                            +SWrapBox::Slot()[Paste]+SWrapBox::Slot()[Load]]]
                    +SVerticalBox::Slot().AutoHeight()
                    [SNew(SBox).HeightOverride_Lambda([this](){return InputHeight;})
                        [SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("WhiteBrush"))).Padding(1)
                            .BorderBackgroundColor_Lambda([this](){return PasteBox->HasAnyUserFocus()?FLinearColor::FromSRGBColor(FColor(0,112,224)):FLinearColor::FromSRGBColor(FColor(56,56,56));})
                            [SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("WhiteBrush"))).BorderBackgroundColor(FLinearColor::FromSRGBColor(FColor(15,15,15))).Padding(0)
                             [PasteBox.ToSharedRef()]]]]
                    +SVerticalBox::Slot().AutoHeight()
                    [SAssignNew(ResizeHandle,SBorder).Tag(TEXT("NodeForge.ResizeInput")).BorderImage(FAppStyle::GetBrush(TEXT("NoBorder")))
                        .Padding(0).Cursor(EMouseCursor::ResizeUpDown)
                        .OnMouseButtonDown_Lambda([this](const FGeometry&,const FPointerEvent& E)
                            {if(E.GetEffectingButton()!=EKeys::LeftMouseButton)return FReply::Unhandled();ResizeStart=E.GetScreenSpacePosition().Y;ResizeInitial=InputHeight;return FReply::Handled().CaptureMouse(ResizeHandle.ToSharedRef());})
                        .OnMouseMove_Lambda([this](const FGeometry& G,const FPointerEvent& E)
                            {if(!ResizeHandle->HasMouseCapture())return FReply::Unhandled();InputHeight=FMath::Clamp(ResizeInitial+(E.GetScreenSpacePosition().Y-ResizeStart)/G.Scale,180.0f,1400.0f);return FReply::Handled();})
                        .OnMouseButtonUp_Lambda([](const FGeometry&,const FPointerEvent&){return FReply::Handled().ReleaseMouseCapture();})
                        [SNew(SBox).HeightOverride(4)]]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,2,0,4)
                    [SNew(SNodeForgeAlignedRow).LeftWidthHint(180)
                        .Left()
                        [SNew(STextBlock).Tag(TEXT("NodeForge.InputSource")).AutoWrapText(true).ColorAndOpacity(Muted)
                            .Text_Lambda([this](){return FText::FromString(InputSummary());})]
                        .Right()[SNew(SWrapBox).PreferredSize(250).HAlign(HAlign_Right).InnerSlotPadding(FVector2D(16,4))
                        +SWrapBox::Slot()[TextAction(TEXT("打开生成资产"),TEXT("NodeForge.OpenAsset"),FOnClicked::CreateSP(this,&SNodeForgePanel::OpenAsset),[this](){return CanUseResultAsset();})]
                        +SWrapBox::Slot()[TextAction(TEXT("定位生成资产"),TEXT("NodeForge.LocateAsset"),FOnClicked::CreateSP(this,&SNodeForgePanel::LocateAsset),[this](){return CanUseResultAsset();})]
                        +SWrapBox::Slot()[TextAction(TEXT("报告"),TEXT("NodeForge.Report"),FOnClicked::CreateSP(this,&SNodeForgePanel::OpenReport),[this](){return !Service->IsBusy()&&HasReport();})]]]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,6,0,8)[MainActions]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,4)[SNew(STextBlock).Text(FText::FromString(TEXT("构图与副本")))]
                    +SVerticalBox::Slot().AutoHeight()[Copies]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,4)[SNew(STextBlock).Tag(TEXT("NodeForge.RuleSection")).Text(FText::FromString(TEXT("规则与资源")))]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,4)
                    [SNew(SBox).HeightOverride(24)[SAssignNew(RuleQueryBox,SEditableTextBox)
                        .HintText(FText::FromString(TEXT("节点或函数名称；留空查询该类全部规则")))
                        .IsEnabled_Lambda([this](){return !Service->IsBusy();})]]
                    +SVerticalBox::Slot().AutoHeight()[Rules]
                ]
            ]
        ];
        RefreshClientIcon();
    }

    virtual ~SNodeForgePanel() override { if(auto Window=DialogWindow.Pin())Window->RequestDestroyWindow(); if(Service) Service->DetachPanel(); }
    virtual void Tick(const FGeometry& Geometry, const double Time, const float DeltaTime) override
    {
        SCompoundWidget::Tick(Geometry, Time, DeltaTime);
        SyncDraftToView();
    }

private:
    TSharedRef<SWidget> Action(const TCHAR* Label,const TCHAR* WidgetTag,const FOnClicked& Click,TFunction<bool()> Enabled,bool Primary=false)
    {
        return SNew(SNodeForgeMotion)[SNew(SBox).WidthOverride(112).HeightOverride(24)
            [SNew(SButton).Tag(FName(WidgetTag)).ButtonStyle(FAppStyle::Get(),Primary?TEXT("PrimaryButton"):TEXT("Button"))
                .ContentPadding(FMargin(6,2)).OnClicked(Click).IsEnabled_Lambda(MoveTemp(Enabled))
                [SNew(STextBlock).Text(FText::FromString(Label)).Font(FAppStyle::GetFontStyle(TEXT("NormalFont")))]]];
    }
    TSharedRef<SWidget> TextAction(const TCHAR* Label,const TCHAR* WidgetTag,const FOnClicked& Click,TFunction<bool()> Enabled)
    {
        const bool DependsOnResult=FString(WidgetTag).Contains(TEXT("Asset"))||FString(WidgetTag)==TEXT("NodeForge.Report");
        return SNew(SButton).Tag(FName(WidgetTag)).ButtonStyle(FAppStyle::Get(),TEXT("SimpleButton")).ContentPadding(FMargin(0,3))
            .OnClicked(Click).IsEnabled_Lambda([Enabled](){return Enabled();})
            .Visibility_Lambda([Enabled,DependsOnResult](){return DependsOnResult&&!Enabled()?EVisibility::Collapsed:EVisibility::Visible;})
            [SNew(STextBlock).Text(FText::FromString(Label)).ColorAndOpacity(FSlateColor::UseSubduedForeground())];
    }
    bool CanUseDraft() const { return !Service->IsBusy()&&!State().Input.Draft().Text.TrimStartAndEnd().IsEmpty(); }
    bool CanUseResultAsset() const { return !Service->IsBusy()&&!ResultIsStale()&&!State().LastAssetPath.IsEmpty(); }
    FString InputSummary() const
    {
        if(Service->IsBusy())return TEXT("正在处理…");
        if(ResultIsStale())return TEXT("输入已更新；上次结果已过期。 ")+State().Input.DescribeSource();
        if(!State().ResultText.IsEmpty())
        {
            FString Status=State().ResultText;int32 End;
            if(Status.FindChar(TEXT('\n'),End))Status.LeftInline(End);
            return State().ResultNotice.IsEmpty()?Status:Status+TEXT(" ")+State().ResultNotice;
        }
        if(!State().ResultNotice.IsEmpty())return State().ResultNotice;
        return State().Input.DescribeSource();
    }
    FReply PasteInput()
    {
        FNodeForgeEditorBusinessScope Guard(*Service);if(!Guard)return FReply::Handled();
        FString Text;FPlatformApplicationMisc::ClipboardPaste(Text);ReplaceInput(Text,TEXT("clipboard"));return FReply::Handled();
    }
    FReply RestoreInput()
    {
        FString Error;if(Service->RestoreDraft(Error))SyncDraftToView();else State().ResultNotice=Error;
        return FReply::Handled();
    }
    FReply CopyReadOnly()
    {
        FNodeForgeEditorBusinessScope Guard(*Service);if(!Guard||State().LastReadOnlyData.IsEmpty())return FReply::Handled();
        FPlatformApplicationMisc::ClipboardCopy(*State().LastReadOnlyData);State().ResultNotice=TEXT("完整读取内容已复制。");return FReply::Handled();
    }
    FReply MaterialRules() { return RunTaskRules(TEXT("Material")); }
    FReply BlueprintRules() { return RunTaskRules(TEXT("Blueprint")); }
    FReply OpenUrl(const FString& Url)
    {
        FString Error;FPlatformProcess::LaunchURL(*Url,nullptr,&Error);if(!Error.IsEmpty())State().ResultNotice=TEXT("网页未打开：")+Error;
        return FReply::Handled();
    }
    FReply OpenManual() { return OpenUrl(NodeForgeProductInfo::Manual()); }
    void ShowWindow(const FString& Title,const TSharedRef<SWidget>& Body,const TSharedRef<SWidget>& Footer)
    {
        if(auto Old=DialogWindow.Pin())Old->RequestDestroyWindow();
        auto Window=SNew(SWindow).Title(FText::FromString(Title)).ClientSize(FVector2D(520,360)).SizingRule(ESizingRule::Autosized)
            .SupportsMaximize(false).SupportsMinimize(false)
            [SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("WhiteBrush"))).BorderBackgroundColor(FLinearColor::FromSRGBColor(FColor(36,36,36))).Padding(FMargin(10,8))
                [SNew(SVerticalBox)+SVerticalBox::Slot().AutoHeight()[SNew(SBox).WidthOverride(620)[Body]]
                    +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0,6,0,0)[Footer]]];
        DialogWindow=Window;const auto Parent=FSlateApplication::Get().FindWidgetWindow(AsShared());
        if(Parent)FSlateApplication::Get().AddWindowAsNativeChild(Window,Parent.ToSharedRef());else FSlateApplication::Get().AddWindow(Window);
    }
    FReply CloseDialog() { if(auto Window=DialogWindow.Pin())Window->RequestDestroyWindow();return FReply::Handled(); }
    TWeakPtr<SNodeForgePanel> WeakPanel() { return StaticCastSharedRef<SNodeForgePanel>(AsShared()); }
    FOnClicked WeakClick(TFunction<FReply(SNodeForgePanel&)> Click)
    {
        const auto Weak=WeakPanel();return FOnClicked::CreateLambda([Weak,Click](){const auto P=Weak.Pin();return P?Click(*P):FReply::Handled();});
    }
    TSharedRef<SWidget> DialogButton(const TCHAR* Label,const TCHAR* WidgetTag,const FOnClicked& Click,bool Primary=false,float Width=88)
    {
        return SNew(SBox).WidthOverride(Width).HeightOverride(24)
            [SNew(SButton).Tag(FName(WidgetTag)).ButtonStyle(FAppStyle::Get(),Primary?TEXT("PrimaryButton"):TEXT("Button"))
                .ContentPadding(FMargin(6,2)).Text(FText::FromString(Label)).OnClicked(Click)];
    }
    void ShowReportText(const FString& Title,const FString& Text,const FString& Path)
    {
        const FString Hash=NodeForgePrototype::FileSha256(Path);
        auto Footer=SNew(SHorizontalBox)
            +SHorizontalBox::Slot().AutoWidth().Padding(0,0,6,0)[DialogButton(TEXT("复制报告"),TEXT("NodeForge.CopyReport"),WeakClick([Text,Path,Hash](SNodeForgePanel& P)
                {FNodeForgeEditorBusinessScope Guard(*P.Service);if(!Guard)return FReply::Handled();
                 FString Actual,Error;if(NodeForgePrototype::FileSha256(Path)!=Hash||!NodeForgeResultView::Read(Path,Actual,Error)||Actual!=Text)
                     {P.State().ResultNotice=TEXT("报告已变化，未复制旧内容。");return FReply::Handled();}
                 FPlatformApplicationMisc::ClipboardCopy(*Actual);return FReply::Handled();}))]
            +SHorizontalBox::Slot().AutoWidth()[DialogButton(TEXT("关闭"),TEXT("NodeForge.DialogClose"),WeakClick([](SNodeForgePanel& P){return P.CloseDialog();}))];
        ShowWindow(Title,SNew(SBox).WidthOverride(620).HeightOverride(240)
            [SNew(SMultiLineEditableTextBox).Tag(TEXT("NodeForge.ReportText")).IsReadOnly(true)
                .Text(FText::FromString(Text)).AutoWrapText(true)],Footer);
    }
    FReply ShowLoadDialog()
    {
        auto Body=SNew(SVerticalBox)
            +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,6)[SNew(STextBlock).Text(FText::FromString(TEXT("构图文件")))]
            +SVerticalBox::Slot().AutoHeight()
            [SNew(SHorizontalBox)
                +SHorizontalBox::Slot().FillWidth(1)[SNew(SEditableTextBox).IsReadOnly(true).HintText(FText::FromString(TEXT("JSON 文件路径")))]
                +SHorizontalBox::Slot().AutoWidth().Padding(0,0,0,0)
                [SNew(SBox).WidthOverride(26).HeightOverride(24)
                    [SNew(SButton).Tag(TEXT("NodeForge.BrowseFile")).ButtonStyle(&BrowseButtonStyle).ContentPadding(3).ToolTipText(FText::FromString(TEXT("浏览 JSON 文件")))
                        .OnClicked(WeakClick([](SNodeForgePanel& P)
                            {const uint64 Before=P.State().Input.Revision();P.ImportJson();if(P.State().Input.Revision()!=Before)P.CloseDialog();return FReply::Handled();}))
                        [SNew(SImage).Tag(TEXT("NodeForge.BrowseIcon")).Image(FAppStyle::GetBrush(TEXT("Icons.FolderOpen"))).ColorAndOpacity(FLinearColor::White)]]]]
            +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0,4,12,0)
            [TextAction(TEXT("载入案例"),TEXT("NodeForge.LoadExample"),WeakClick([](SNodeForgePanel& P){return P.LoadExample();}),[](){return true;})];
        ShowWindow(TEXT("载入构图文件"),Body,DialogButton(TEXT("关闭"),TEXT("NodeForge.DialogClose"),WeakClick([](SNodeForgePanel& P){return P.CloseDialog();})));
        return FReply::Handled();
    }
    FReply LoadExample()
    {
        FNodeForgeEditorBusinessScope Guard(*Service);if(!Guard)return FReply::Handled();
        FString Text,Error,Digest,Catalog;const FString Path=NodeForgePrototype::WorkspaceDirectory()/TEXT("Examples/FirstMaterial.json");
        // Cold sessions must qualify the current native catalog before decoding.
        // Reading the installation metadata alone cannot register a capability.
        const FString CatalogReport=Workspace()/TEXT("Reports/UiRules")/(FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT(".json"));
        if(NodeForgePrototype::Execute(TEXT("GraphCatalog"),FString(),CatalogReport)!=0)
        {State().LastReportPath=CatalogReport;State().ResultNotice=TEXT("案例未载入：当前原生目录检查失败，请查看报告。");return FReply::Handled();}
        NodeForgeGraphDocument::FGraphDocument Document;
        if(!NodeForgePrototype::ReadInputFile(Path,Text,Digest,Error)||!NodeForgeGraphCodec::DecodeText(Text,Document,Error)
            ||!NodeForgeInstallation::CatalogIdentity(Catalog,Error))
            {State().ResultNotice=TEXT("案例未载入：")+Error;return FReply::Handled();}
        if(Document.CatalogDigest!=Catalog)
            {State().ResultNotice=TEXT("案例目录身份与当前安装不一致；请使用匹配安装包的案例。");return FReply::Handled();}
        Document.RequestId=TEXT("Example")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
        Document.Target=Preferences.Directory/TEXT("MExample")+FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8);
        if(NodeForgeGraphCodec::EncodeText(Document,Document.WireVersion,Text,Error)&&ReplaceInput(Text,TEXT("example")))
            {State().ResultNotice=TEXT("案例已载入，尚未生成。");CloseDialog();}
        else State().ResultNotice=TEXT("案例未载入：")+Error;
        return FReply::Handled();
    }
    FString CurrentClientName() const
    {
        for(const auto& C:Preferences.Clients())if(C.Id==Preferences.Client)return C.Name;
        return TEXT("Codex");
    }
    void RefreshClientIcon()
    {
        if(!CurrentIconBox)return;
        for(const auto& C:Preferences.Clients())if(C.Id==Preferences.Client)
        {CurrentIconBox->SetContent(SNew(SNodeForgeClientIcon).Path(C.IconPath).Letter(C.Name).Size(FVector2D(16,16)));break;}
    }
    TSharedRef<SWidget> BuildAiMenu()
    {
        auto Menu=SNew(SVerticalBox);
        for(const auto& Client:Preferences.Clients())
        {
            Menu->AddSlot().AutoHeight().Padding(8,6)
                [SNew(SHorizontalBox)
                    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,6,0)[SNew(SNodeForgeClientIcon).Path(Client.IconPath).Letter(Client.Name)]
                    +SHorizontalBox::Slot().FillWidth(1)
                    [SNew(SButton).ButtonStyle(FAppStyle::Get(),TEXT("SimpleButton")).ContentPadding(0)
                        .OnClicked(WeakClick([Client](SNodeForgePanel& P){FString Error;auto Updated=P.Preferences;Updated.Client=Client.Id; if(!P.PreferencesLoaded){P.State().ResultNotice=P.PreferencesError;return FReply::Handled();}
                            if(Updated.Save(Error)){P.Preferences=MoveTemp(Updated);P.RefreshClientIcon();}else P.State().ResultNotice=Error;return FReply::Handled();}))
                        [SNew(SVerticalBox)
                            +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Client.Name))]
                            +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).ColorAndOpacity(FSlateColor::UseSubduedForeground()).Text(FText::FromString(Client.Description))]]]
                    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8,0)
                    [SNew(STextBlock).Text_Lambda([Client](){const auto Channel=NodeForgePanel::GetControlChannel();return FText::FromString(Client.Id!=TEXT("codex")?TEXT("未检测"):
                        Channel&&Channel->HasConnectedClient(Client.Id)?TEXT("已连接"):Channel?TEXT("服务就绪"):TEXT("未连接"));})]
                    +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                    [SNew(SBox).WidthOverride(88).HeightOverride(24)
                        [SNew(SButton).Tag(FName(TEXT("NodeForge.ClientAction.")+Client.Id)).IsEnabled_Lambda([Weak=WeakPanel()](){const auto P=Weak.Pin();return P&&!P->Service->IsBusy();})
                            .Text_Lambda([Client](){const auto Channel=NodeForgePanel::GetControlChannel();return FText::FromString(Client.Id==TEXT("codex")?(Channel&&Channel->HasConnectedClient(Client.Id)?TEXT("断开"):TEXT("连接")):TEXT("配置"));})
                            .OnClicked(WeakClick([Client](SNodeForgePanel& P){const auto Channel=NodeForgePanel::GetControlChannel();
                                if(Client.Id==TEXT("codex")&&Channel&&Channel->HasConnectedClient(Client.Id)){Channel->DisconnectClient(Client.Id);return FReply::Handled();}
                                return P.OpenClientSettings(Client.Id);}))]]];
        }
        Menu->AddSlot().AutoHeight().Padding(8,4)
            [SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6,6))
                +SWrapBox::Slot()[DialogButton(TEXT("自定义客户端"),TEXT("NodeForge.CustomClient"),WeakClick([](SNodeForgePanel& P){return P.OpenClientSettings(TEXT("custom"));}))]
                +SWrapBox::Slot()[DialogButton(TEXT("MCP 设置"),TEXT("NodeForge.McpSettings"),WeakClick([](SNodeForgePanel& P){return P.OpenClientSettings(P.Preferences.Client);}))]];
        return SNew(SBox).WidthOverride(420)[Menu];
    }
    FReply OpenSettings() { return ShowSettings(TEXT("general")); }
    FReply OpenClientSettings(const FString& Client)
    {
        if(AiButton)AiButton->SetIsOpen(false);
        return ShowSettings(Client==TEXT("custom")?TEXT("ai"):TEXT("mcp"),Client);
    }

    FReply ShowSettings(const FString& Page,const FString& ViewingClient=FString())
    {
        SettingsDraft=MakeShared<FNodeForgePanelPreferences>(Preferences);SettingsError=PreferencesError;
        McpViewingClient=ViewingClient.IsEmpty()?Preferences.Client:ViewingClient;
        auto Tabs=SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6,6));
        for(const auto& Tab:TArray<TPair<FString,FString>>{{TEXT("general"),TEXT("常规")},{TEXT("ai"),TEXT("AI 客户端")},{TEXT("mcp"),TEXT("MCP")},{TEXT("about"),TEXT("关于")}})
            Tabs->AddSlot()[SNew(SBox).WidthOverride(88).HeightOverride(24)
                [SNew(SButton).Tag(FName(TEXT("NodeForge.Settings.")+Tab.Key)).ButtonStyle(FAppStyle::Get(),TEXT("SimpleButton")).ContentPadding(0)
                    .OnClicked(WeakClick([Key=Tab.Key](SNodeForgePanel& P){P.RenderSettings(Key);return FReply::Handled();}))
                    [SNew(SVerticalBox)
                        +SVerticalBox::Slot().FillHeight(1).HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(Tab.Value))]
                        +SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(1)
                            [SNew(SBorder).Padding(0).BorderImage(FAppStyle::GetBrush(TEXT("WhiteBrush")))
                                .BorderBackgroundColor_Lambda([Weak=WeakPanel(),Key=Tab.Key](){const auto P=Weak.Pin();return FLinearColor::FromSRGBColor(P&&P->SettingsPage==Key?FColor(0,112,224):FColor(56,56,56));})]]]]];
        auto Body=SNew(SVerticalBox)
            +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)[Tabs]
            +SVerticalBox::Slot().AutoHeight()[SAssignNew(SettingsContent,SBox).MinDesiredWidth(420)]
            +SVerticalBox::Slot().AutoHeight().Padding(0,6)
            [SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(FLinearColor::FromSRGBColor(FColor(242,140,123)))
                .Text_Lambda([Weak=WeakPanel()](){const auto P=Weak.Pin();return FText::FromString(P?P->SettingsError:FString());})];
        auto Footer=SNew(SHorizontalBox)
            +SHorizontalBox::Slot().AutoWidth().Padding(0,0,6,0)[DialogButton(TEXT("取消"),TEXT("NodeForge.SettingsCancel"),WeakClick([](SNodeForgePanel& P){return P.CloseDialog();}))]
            +SHorizontalBox::Slot().AutoWidth().Padding(0,0,6,0)[SNew(SBox).IsEnabled_Lambda([Weak=WeakPanel()](){const auto P=Weak.Pin();return P&&P->PreferencesLoaded&&P->SettingsDraft&&!P->SettingsDraft->Equals(P->Preferences);})
                [DialogButton(TEXT("应用"),TEXT("NodeForge.SettingsApply"),WeakClick([](SNodeForgePanel& P){return P.ApplySettings();}),true)]]
            +SHorizontalBox::Slot().AutoWidth()[DialogButton(TEXT("关闭"),TEXT("NodeForge.SettingsClose"),WeakClick([](SNodeForgePanel& P){return P.CloseDialog();}))];
        RenderSettings(Page);ShowWindow(TEXT("NodeForge 设置"),Body,Footer);return FReply::Handled();
    }
    FReply ApplySettings()
    {
        if(!SettingsDraft||!PreferencesLoaded)return FReply::Handled();
        FNodeForgePanelPreferences Disk;
        if(!Disk.Load(SettingsError)||!Disk.Equals(Preferences))
            {if(SettingsError.IsEmpty())SettingsError=TEXT("设置已在其它窗口更新；当前草稿保留，请关闭后重开核对。");return FReply::Handled();}
        if(SettingsDraft->Save(SettingsError)){Preferences=*SettingsDraft;RefreshClientIcon();State().ResultNotice=TEXT("设置已应用。");}
        return FReply::Handled();
    }
    void RenderSettings(const FString& Page)
    {
        if(!SettingsContent||!SettingsDraft)return;
        SettingsPage=Page;
        auto Content=SNew(SVerticalBox);
        if(Page==TEXT("about"))
        {
            const auto Plugin=IPluginManager::Get().FindPlugin(TEXT("NodeForge"));
            Content->AddSlot().AutoHeight().Padding(0,4,0,8)
                [SNew(SHorizontalBox)+SHorizontalBox::Slot().AutoWidth().Padding(0,0,9,0)[SNew(SNodeForgeClientIcon).Path(Brand->GetResourceName().ToString()).Size(FVector2D(23,23))]
                    +SHorizontalBox::Slot().AutoWidth()[SNew(SNodeForgeClientIcon).Path(Wordmark->GetResourceName().ToString()).Size(FVector2D(176,25))]];
            Content->AddSlot().AutoHeight()[SNew(STextBlock).Tag(TEXT("NodeForge.AboutName")).Text(FText::FromString(NodeForgeProductInfo::Name()))];
            Content->AddSlot().AutoHeight().Padding(0,6)[SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(NodeForgeProductInfo::Description()))];
            Content->AddSlot().AutoHeight()[SNew(STextBlock).Tag(TEXT("NodeForge.AboutVersion")).Text(FText::FromString(TEXT("版本：")+(Plugin?Plugin->GetDescriptor().VersionName:TEXT("不可读"))))];
            Content->AddSlot().AutoHeight().Padding(0,6)
                [SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(12,4))
                    +SWrapBox::Slot()[TextAction(TEXT("官网"),TEXT("NodeForge.AboutWebsite"),WeakClick([](SNodeForgePanel& P){return P.OpenUrl(NodeForgeProductInfo::Website());}),[](){return true;})]
                    +SWrapBox::Slot()[TextAction(TEXT("GitHub 项目"),TEXT("NodeForge.AboutGithub"),WeakClick([](SNodeForgePanel& P){return P.OpenUrl(NodeForgeProductInfo::Repository());}),[](){return true;})]];
        }
        else if(Page==TEXT("general"))
        {
            Content->AddSlot().AutoHeight()
                [SNew(SHorizontalBox)+SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)
                    [SNew(SBox).WidthOverride(108)[SNew(STextBlock).Text(FText::FromString(TEXT("默认目录")))]]
                    +SHorizontalBox::Slot().FillWidth(1)
                    [SNew(SEditableTextBox).Tag(TEXT("NodeForge.SettingsDirectory")).Text(FText::FromString(SettingsDraft->Directory))
                        .OnTextChanged_Lambda([Weak=WeakPanel()](const FText& Text){if(auto P=Weak.Pin())if(P->SettingsDraft){P->SettingsDraft->Directory=Text.ToString();P->SettingsError.Empty();}})]];
            Content->AddSlot().AutoHeight().Padding(0,6)[SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())
                .Text(FText::FromString(TEXT("用于主动载入案例等新目标准备；已有 JSON 的明确目标保持不变。")))];
        }
        else if(Page==TEXT("ai"))
        {
            Content->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("当前客户端")))];
            for(const auto& C:SettingsDraft->Clients())Content->AddSlot().AutoHeight().Padding(0,3)
                [SNew(SButton).Tag(FName(TEXT("NodeForge.Client.")+C.Id)).ButtonStyle(FAppStyle::Get(),TEXT("SimpleButton"))
                    .OnClicked(WeakClick([C](SNodeForgePanel& P){P.SettingsDraft->Client=C.Id;return FReply::Handled();}))
                    [SNew(STextBlock).Text_Lambda([Weak=WeakPanel(),C](){const auto P=Weak.Pin();return FText::FromString((P&&P->SettingsDraft&&P->SettingsDraft->Client==C.Id?TEXT("● "):TEXT("○ "))+C.Name);})]];
            Content->AddSlot().AutoHeight().Padding(0,6)
                [SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6,6))
                    +SWrapBox::Slot()[DialogButton(TEXT("添加客户端"),TEXT("NodeForge.AddClient"),WeakClick([](SNodeForgePanel& P){P.RenderClientEditor(false);return FReply::Handled();}))]
                    +SWrapBox::Slot()[DialogButton(TEXT("导入信息"),TEXT("NodeForge.ImportClient"),WeakClick([](SNodeForgePanel& P){P.RenderClientEditor(true);return FReply::Handled();}))]];
        }
        else if(Page==TEXT("mcp"))
        {
            NodeForgeInstallation::FBinding Binding;FString Error,UserDirectory;TSharedPtr<FJsonObject> Configuration;
            const bool Bound=NodeForgeInstallation::Read(Binding,Error);
            const bool Enabled=Bound&&NodeForgeInstallation::AiConfiguration(Binding,Configuration,UserDirectory,Error);
            const FString Python=Binding.InstallationRoot/TEXT("Tools/NodeForgeMcp/Runtime/python.exe"),Launcher=Binding.InstallationRoot/TEXT("Scripts/NodeForgeAiLaunch.py");
            const bool Installed=Bound&&IFileManager::Get().FileExists(*Python)&&IFileManager::Get().FileExists(*Launcher);
            auto JsonString=[](const FString& Value){FString Text;auto Json=MakeShared<FJsonObject>();Json->SetStringField(TEXT("v"),Value);
                FJsonSerializer::Serialize(Json,TJsonWriterFactory<TCHAR,TCondensedJsonPrintPolicy<TCHAR>>::Create(&Text));return Text.Mid(5,Text.Len()-6);};
            FString Config;
            if(Installed)
            {
                if(McpViewingClient==TEXT("codex"))Config=TEXT("[mcp_servers.NodeForge]\ncommand = ")+JsonString(Python)+TEXT("\nargs = [")+JsonString(Launcher)+TEXT("]\ncwd = ")+JsonString(Binding.InstallationRoot);
                else {auto Server=MakeShared<FJsonObject>();Server->SetStringField(TEXT("command"),Python);Server->SetArrayField(TEXT("args"),{MakeShared<FJsonValueString>(Launcher)});Server->SetStringField(TEXT("cwd"),Binding.InstallationRoot);
                    auto Servers=MakeShared<FJsonObject>();Servers->SetObjectField(TEXT("NodeForge"),Server);auto Whole=MakeShared<FJsonObject>();Whole->SetObjectField(TEXT("mcpServers"),Servers);FJsonSerializer::Serialize(Whole,TJsonWriterFactory<>::Create(&Config));}
            }
            Content->AddSlot().AutoHeight()[SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(!Installed?TEXT("当前安装尚无可用 AI 配套配置，请按接入流程补装。"):Enabled?TEXT("配套安装已启用。配置完成与实际客户端在线分别显示。"):TEXT("配套文件存在，但当前用户未启用；请完成一次 AI 接入。")))];
            if(McpViewingClient!=TEXT("codex"))Content->AddSlot().AutoHeight().Padding(0,6)
                [SNew(STextBlock).Text(FText::FromString(TEXT("状态：未检测。本期保留标准 MCP 配置，不检测客户端在线。")))];
            if(Installed)Content->AddSlot().AutoHeight().Padding(0,6)
                [SNew(SBox).HeightOverride(160)[SNew(SMultiLineEditableTextBox).Tag(TEXT("NodeForge.McpConfigText")).IsReadOnly(true).Text(FText::FromString(Config))]];
            Content->AddSlot().AutoHeight().Padding(0,6)
                [SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6,6))
                    +SWrapBox::Slot()[SNew(SBox).IsEnabled(Installed)[DialogButton(TEXT("复制配置"),TEXT("NodeForge.CopyMcpConfig"),WeakClick([Config](SNodeForgePanel&){FPlatformApplicationMisc::ClipboardCopy(*Config);return FReply::Handled();}))]]
                    +SWrapBox::Slot()[DialogButton(TEXT("接入流程"),TEXT("NodeForge.AiGuide"),WeakClick([](SNodeForgePanel& P){return P.OpenUrl(NodeForgeProductInfo::AiGuide());}))]
                    +SWrapBox::Slot()[SNew(SBox).IsEnabled(Enabled)[DialogButton(TEXT("启用服务"),TEXT("NodeForge.AiStart"),WeakClick([](SNodeForgePanel& P){NodeForgeInstallation::OnReady().Broadcast();P.State().ResultNotice=NodeForgePanel::GetControlChannel().IsValid()?TEXT("本工程服务已启用；客户端在线另按握手确认。"):TEXT("服务未启用，请查看实际安装状态。");return FReply::Handled();}))]]
                    +SWrapBox::Slot()[SNew(SBox).IsEnabled_Lambda([](){return NodeForgePanel::GetControlChannel().IsValid();})
                        [DialogButton(TEXT("停止本工程服务"),TEXT("NodeForge.AiStop"),WeakClick([](SNodeForgePanel&){if(auto Channel=NodeForgePanel::GetControlChannel())Channel->RequestStop();return FReply::Handled();}),false,144)]]];
            Content->AddSlot().AutoHeight().Padding(0,6)[SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())
                .Text(FText::FromString(TEXT("停止服务会结束本工程的全部 AI 会话；其它工程保持。选择客户端不表示已连接。")))];
        }
        SettingsContent->SetContent(Content);
    }
    void RenderClientEditor(bool Importing)
    {
        ClientNameBox.Reset();ClientDetailBox.Reset();ClientImportBox.Reset();ClientIconPath.Empty();ClientIconPng.Empty();
        auto Content=SNew(SVerticalBox);
        if(Importing)Content->AddSlot().AutoHeight()
            [SNew(SBox).HeightOverride(140)[SAssignNew(ClientImportBox,SMultiLineEditableTextBox).Tag(TEXT("NodeForge.ClientImport"))
                .HintText(FText::FromString(TEXT("{\"name\":\"我的 AI\",\"description\":\"自定义客户端\"}")))]];
        else
        {
            Content->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("显示名称")))];
            Content->AddSlot().AutoHeight().Padding(0,4,0,6)[SAssignNew(ClientNameBox,SEditableTextBox).Tag(TEXT("NodeForge.ClientName"))];
            Content->AddSlot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("客户端说明")))];
            Content->AddSlot().AutoHeight().Padding(0,4,0,6)[SAssignNew(ClientDetailBox,SEditableTextBox).Tag(TEXT("NodeForge.ClientDescription"))];
            Content->AddSlot().AutoHeight().HAlign(HAlign_Left)[DialogButton(TEXT("浏览图标"),TEXT("NodeForge.ClientIconBrowse"),WeakClick([](SNodeForgePanel& P){return P.BrowseClientIcon();}))];
            Content->AddSlot().AutoHeight()[SNew(STextBlock).AutoWrapText(true).Text_Lambda([Weak=WeakPanel()](){const auto P=Weak.Pin();return FText::FromString(P?P->ClientIconPath:FString());})];
        }
        Content->AddSlot().AutoHeight().Padding(0,8)
            [SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6,6))
                +SWrapBox::Slot()[DialogButton(TEXT("返回"),TEXT("NodeForge.ClientReturn"),WeakClick([](SNodeForgePanel& P){P.RenderSettings(TEXT("ai"));return FReply::Handled();}))]
                +SWrapBox::Slot()[DialogButton(TEXT("添加"),TEXT("NodeForge.ClientAdd"),WeakClick([Importing](SNodeForgePanel& P){return P.AddClient(Importing);}),true)]];
        SettingsContent->SetContent(Content);
    }
    FReply BrowseClientIcon()
    {
        auto Desktop=FDesktopPlatformModule::TryGet();if(!Desktop)return FReply::Handled();TArray<FString> Files;
        if(Desktop->OpenFileDialog(FSlateApplication::Get().FindBestParentWindowHandleForDialogs(DialogWindow.Pin()),TEXT("选择客户端图标"),FString(),FString(),
            TEXT("图像 (*.png;*.jpg;*.jpeg;*.webp)|*.png;*.jpg;*.jpeg;*.webp"),EFileDialogFlags::None,Files)&&Files.Num()==1)
        {
            TArray<uint8> Bytes;
            if(IFileManager::Get().FileSize(*Files[0])<=0||IFileManager::Get().FileSize(*Files[0])>204800||!FFileHelper::LoadFileToArray(Bytes,*Files[0]))SettingsError=TEXT("图标应为不超过200 KB的可读图像。");
            else if(FNodeForgePanelPreferences::DecodeIcon(Bytes,ClientIconPng,SettingsError))ClientIconPath=FPaths::GetCleanFilename(Files[0]);
        }
        return FReply::Handled();
    }
    FReply AddClient(bool Importing)
    {
        if(!SettingsDraft)return FReply::Handled();FNodeForgeClientDefinition Client;
        Client.Id=TEXT("custom-")+FGuid::NewGuid().ToString(EGuidFormats::Digits);Client.PendingIconPng=ClientIconPng;
        if(Importing)
        {
            TSharedPtr<FJsonObject> Object;const FString Text=ClientImportBox->GetText().ToString();
            if(FTCHARToUTF8(*Text).Length()>307200||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Object)||!Object
                ||!Object->TryGetStringField(TEXT("name"),Client.Name))
                {SettingsError=TEXT("请输入含name字段的有效客户端JSON，最多300 KiB。");return FReply::Handled();}
            Object->TryGetStringField(TEXT("description"),Client.Description);
            if(Object->HasField(TEXT("icon")))
            {
                FString Data;TArray<uint8> Bytes;
                if(!Object->TryGetStringField(TEXT("icon"),Data)
                    ||!(Data.StartsWith(TEXT("data:image/png;base64,"))||Data.StartsWith(TEXT("data:image/jpeg;base64,"))||Data.StartsWith(TEXT("data:image/webp;base64,"))))
                    {SettingsError=TEXT("图标需要PNG、JPEG或WebP的base64数据URI。");return FReply::Handled();}
                int32 Comma=INDEX_NONE;Data.FindChar(',',Comma);
                if(!FBase64::Decode(Data.Mid(Comma+1),Bytes)||!FNodeForgePanelPreferences::DecodeIcon(Bytes,Client.PendingIconPng,SettingsError))
                    {if(SettingsError.IsEmpty())SettingsError=TEXT("图标base64无效。");return FReply::Handled();}
            }
        }
        else {Client.Name=ClientNameBox->GetText().ToString();Client.Description=ClientDetailBox->GetText().ToString();}
        Client.Name=Client.Name.TrimStartAndEnd();if(Client.Description.IsEmpty())Client.Description=TEXT("自定义客户端");
        auto Candidate=*SettingsDraft;Candidate.CustomClients.Add(Client);Candidate.Client=Client.Id;
        if(!Candidate.Validate(SettingsError))return FReply::Handled();*SettingsDraft=MoveTemp(Candidate);RenderSettings(TEXT("ai"));return FReply::Handled();
    }

    FNodeForgeEditorState& State() const { return Service->MutableState(); }
    void SyncDraftToView()
    {
        if(!PasteBox || DisplayedRevision == State().Input.Revision()) return;
        TGuardValue<bool> Guard(bSyncingInput, true);
        PasteBox->SetText(FText::FromString(State().Input.Draft().Text));
        DisplayedRevision = State().Input.Revision();
    }
    void DraftChanged(const FText& Text)
    {
        if(!bSyncingInput) Service->EditFromUI(Text.ToString());
        DisplayedRevision = Text.ToString() == State().Input.Draft().Text ? State().Input.Revision() : MAX_uint64;
    }
    bool ReplaceInput(const FString& Text,const FString& Origin,const FString& SourcePath=FString(),const FString& SourceDigest=FString())
    {
        FString Error;
        if(!Service->ReplaceFromUIWhileBusy(Text,Origin,SourcePath,SourceDigest,Error)){State().ResultNotice=Error;return false;}
        PasteBox->SetText(FText::FromString(State().Input.Draft().Text));
        return true;
    }
    bool ResultIsStale() const { return State().bResultInputBound && State().ResultRevision!=State().Input.Revision(); }
    bool SaveDraftSnapshot(FString& Path,FString& Hash,FString& Error)
    { return Service->SaveDraftSnapshot(Path,Hash,Error); }
    FString Workspace() const
    {
        return NodeForgeInstallation::StateDirectory();
    }
    bool HasReport() const
    {
        return !State().LastReportPath.IsEmpty() && IFileManager::Get().FileExists(*State().LastReportPath);
    }
    void ResetResult(bool InputBound=false,bool Relayout=false) { Service->ResetResult(InputBound,Relayout); }
    bool ReadCurrentReport(TSharedPtr<FJsonObject>& Report,FString* Original=nullptr) { return Service->ReadCurrentReport(Report,Original); }
    FReply InspectSelectedResource()
    {
        if(Service->IsBusy()) return FReply::Handled();
        FNodeForgeEditorBusinessScope Guard(*Service); if (!Guard) return FReply::Handled();
        ResetResult();
        TArray<FAssetData> Selected;
        FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser")).Get().GetSelectedAssets(Selected);
        State().LastReportPath=Workspace()/TEXT("Reports/UiRules")/(FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT(".json"));
        FString Error;
        const int32 Code=NodeForgePanel::InspectSelection(Selected,State().LastReportPath,Error);
        if(Code<0) {State().ResultText=TEXT("只读检查未执行：")+Error;State().LastReportPath.Empty();return FReply::Handled();}
        return ShowReadOnlyResult(Code);
    }
    FReply RunReadOnly()
    {
        Service->QueryFromUI(TEXT("catalog.get"), MakeShared<FJsonObject>());
        return FReply::Handled();
    }
    FReply ExportSelectedGraph()
    {
        if(Service->IsBusy())return FReply::Handled();FNodeForgeEditorBusinessScope Guard(*Service);if(!Guard)return FReply::Handled();
        const uint64 ExpectedRevision=State().Input.Revision();ResetResult();
        TArray<FAssetData> Selected;FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser")).Get().GetSelectedAssets(Selected);
        const FString ExportReport=Workspace()/TEXT("Reports/UiRules")/(FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT(".json"));
        State().LastReportPath=ExportReport;
        FString Error;const int32 Code=NodeForgePanel::ExportSelection(Selected,ExportReport,Error);
        if(Code<0)
        {
            if(State().Input.Revision()==ExpectedRevision)State().LastExportReportPath.Empty();
            State().ResultText=TEXT("构图导出未执行：")+Error;State().LastReportPath.Empty();return FReply::Handled();
        }
        TSharedPtr<FJsonObject> Report;
        if(!ReadCurrentReport(Report))return FReply::Handled();
        FString Status,Text;Report->TryGetStringField(TEXT("operationStatus"),Status);
        if(Code==0 && Status==TEXT("passed") && Report->TryGetStringField(TEXT("exportedRequestText"),Text))
        {
            if(!Service->CommitExportFromUI(Text,ExportReport,NodeForgePrototype::FileSha256(ExportReport),ExpectedRevision,Error))
            {State().ResultNotice=TEXT("导出结果已保留，但未载入草稿：")+Error;return FReply::Handled();}
            State().LastReadOnlyData=Text;SyncDraftToView();
            State().ResultNotice=TEXT("原导出JSON及来源已载入。可编辑同来源草稿、预览差异，再单独生成新资产。");
            return FReply::Handled();
        }
        if(State().Input.Revision()==ExpectedRevision)State().LastExportReportPath.Empty();
        State().ResultNotice=Code==5?TEXT("导出结果未知；保留原报告，停止依赖操作。") :TEXT("本次没有可用导出结果，当前稿的旧预览基线已失效。");
        return FReply::Handled();
    }

    FReply PreviewGraph()
    {
        const int32 Code=Service->PreviewFromUI();
        if(Code!=5&&HasReport())
        {
            FString Text,Error;
            if(NodeForgeResultView::Read(State().LastReportPath,Text,Error))ShowReportText(TEXT("构图差异"),Text,State().LastReportPath);
            else State().ResultNotice=Error;
        }
        return FReply::Handled();
    }
    FReply PreflightGraph() { Service->PreflightFromUI(); return FReply::Handled(); }
    FReply RunTaskRules(const FString& Kind)
    {
        auto Config=MakeShared<FJsonObject>();Config->SetStringField(TEXT("assetKind"),Kind);
        Config->SetStringField(TEXT("query"),RuleQueryBox->GetText().ToString());
        Service->QueryFromUI(TEXT("rules.query"),Config);
        return FReply::Handled();
    }
    FReply ShowReadOnlyResult(int32 Code)
    {
        FString Json,Status;TSharedPtr<FJsonObject> Report;
        State().LastAssetPath.Empty();State().LastReadOnlyData.Empty();
        if(!ReadCurrentReport(Report,&Json))return FReply::Handled();
        if(Code!=0 || !Report->TryGetStringField(TEXT("operationStatus"),Status) || Status!=TEXT("passed"))
        {
            State().ResultNotice=FString::Printf(TEXT("本次只读操作未成功（返回码 %d），未保留旧读取内容。"),Code);
            return FReply::Handled();
        }
        State().LastReadOnlyData=MoveTemp(Json);
        State().ResultNotice=TEXT("完整规则或签名可通过“复制读取内容”交给 AI；规则和报告不是构图输入。");
        return FReply::Handled();
    }
    FReply ImportPaste()
    {
        if(Service->IsBusy())return FReply::Handled();
        Service->GenerateFromUI();
        return FReply::Handled();
    }
    FReply PrepareNewCopy()
    {
        if (Service->PrepareCopyFromUI() == 0) SyncDraftToView();
        return FReply::Handled();
    }
    FReply ImportJson()
    {
        FNodeForgeEditorBusinessScope Guard(*Service); if (!Guard) return FReply::Handled();
        IDesktopPlatform* Desktop = FDesktopPlatformModule::TryGet();
        const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("NodeForge"));
        if (!Desktop || !Plugin.IsValid())
        {
            State().ResultNotice = TEXT("未载入：文件选择接口或插件目录不可用；原输入和结果保留。");
            return FReply::Handled();
        }

        const FString WorkspaceRoot = Workspace();
        TArray<FString> SelectedFiles;
        const void* Parent = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(DialogWindow.Pin());
        bool Selected=false;
        {
            Selected=Desktop->OpenFileDialog(Parent,TEXT("载入 NodeForge JSON（不会生成资产）"),
                FPaths::Combine(WorkspaceRoot,TEXT("Tests/Valid")),FString(),
                TEXT("JSON 文件 (*.json)|*.json"),EFileDialogFlags::None,SelectedFiles);
        }
        if(!Selected)
        {
            State().ResultNotice = TEXT("已取消载入；原输入、检查和结果保留。");
            return FReply::Handled();
        }
        if (SelectedFiles.Num() != 1 || !FPaths::GetExtension(SelectedFiles[0]).Equals(TEXT("json"), ESearchCase::IgnoreCase))
        {
            State().ResultNotice = TEXT("未载入：必须选择一个 JSON 文件；原输入和结果保留。");
            return FReply::Handled();
        }

        const FString Path=FPaths::ConvertRelativePathToFull(SelectedFiles[0]);
        if(FPaths::IsUnderDirectory(Path,WorkspaceRoot/TEXT("Reports/PluginControlSessions")))
        { State().ResultNotice=TEXT("会话私有文件不能作为构图输入。原稿保持。"); return FReply::Handled(); }
        FString Text,Hash,Error;
        if(!NodeForgePrototype::ReadInputFile(Path,Text,Hash,Error))
        {State().ResultNotice=TEXT("未载入，原输入和结果保留：")+Error;return FReply::Handled();}
        if(ReplaceInput(Text,TEXT("file"),Path,Hash))
            State().ResultNotice=TEXT("文件已载入输入区，尚未生成；可检查或编辑，再点“生成新资产”。");
        return FReply::Handled();
    }

    FReply RelayoutSuccess()
    {
        if(Service->IsBusy())return FReply::Handled();
        FScopedSlowTask Progress(1.0f,FText::FromString(TEXT("正在校验节点、构图并编译…")));
        Service->RelayoutFromUI([&Progress](){Progress.MakeDialog(false);});
        return FReply::Handled();
    }
    UObject* ResultAsset() const
    {
        return State().LastAssetPath.IsEmpty() || ResultIsStale() ? nullptr : LoadObject<UObject>(nullptr,*(State().LastAssetPath+TEXT(".")+FPaths::GetCleanFilename(State().LastAssetPath)));
    }
    FReply LocateAsset()
    {
        FNodeForgeEditorBusinessScope Guard(*Service); if (!Guard) return FReply::Handled();
        if (UObject* Asset=ResultAsset())
        { TArray<FAssetData> Assets; Assets.Add(FAssetData(Asset)); FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser")).Get().SyncBrowserToAssets(Assets);State().ResultNotice=TEXT("已在内容浏览器定位本次生成资产。"); }
        else {State().LastAssetPath.Empty();State().ResultNotice=TEXT("目标资产当前不可定位，请查看结果报告。");}
        return FReply::Handled();
    }
    FReply OpenAsset()
    {
        FNodeForgeEditorBusinessScope Guard(*Service); if (!Guard) return FReply::Handled();
        if (UObject* Asset=ResultAsset()) GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Asset);
        else {State().LastAssetPath.Empty();State().ResultNotice=TEXT("目标资产无法打开，请查看结果报告。");}
        return FReply::Handled();
    }

    FReply OpenReport()
    {
        if(Service->IsBusy())return FReply::Handled();FString Text,Error;
        if(NodeForgeResultView::Read(State().LastReportPath,Text,Error))ShowReportText(TEXT("报告"),Text,State().LastReportPath);
        else State().ResultNotice=Error;
        return FReply::Handled();
    }

    FReply CopyResultReport()
    {
        FNodeForgeEditorBusinessScope Guard(*Service); if (!Guard) return FReply::Handled();
        FString Json,Description;
        if(NodeForgeResultView::Read(State().LastReportPath,Json,Description))
        {
            FPlatformApplicationMisc::ClipboardCopy(*Json);
            TSharedPtr<FJsonObject> Report;
            if(FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Report) && Report)State().ResultText=NodeForgeResultView::Compact(Report.ToSharedRef());
            State().ResultNotice=TEXT("完整报告原文已复制，可与输入和节点规则一起交给 AI。请勿将报告作为构图输入。");
        }
        else
        {
            State().LastAssetPath.Empty();State().LastReadOnlyData.Empty();
            State().ResultText=TEXT("未复制结果报告：")+Description;State().ResultNotice.Empty();
            if(!HasReport())State().LastReportPath.Empty();
        }
        return FReply::Handled();
    }

    FSlateColorBrush Background{FLinearColor::FromSRGBColor(FColor(36,36,36))};
    FEditableTextBoxStyle InputStyle;
    FNodeForgePanelPreferences Preferences;
    TSharedPtr<FNodeForgePanelPreferences> SettingsDraft;
    TSharedPtr<SBox> SettingsContent;
    FString SettingsError,ClientIconPath,SettingsPage,McpViewingClient;
    TArray<uint8> ClientIconPng;
    TSharedPtr<SEditableTextBox> ClientNameBox,ClientDetailBox;
    TSharedPtr<SMultiLineEditableTextBox> ClientImportBox;
    bool PreferencesLoaded=false;
    FString PreferencesError;
    TSharedPtr<FSlateDynamicImageBrush> Brand;
    TSharedPtr<FSlateVectorImageBrush> Wordmark;
    TSharedPtr<SBox> CurrentIconBox;
    TSharedPtr<SScrollBar> InputScrollbar;
    TSharedPtr<SComboButton> AiButton;
    TSharedPtr<SBorder> ResizeHandle;
    TWeakPtr<SWindow> DialogWindow;
    float InputHeight=245.0f,ResizeInitial=245.0f,ResizeStart=0.0f;
    FButtonStyle BrowseButtonStyle;
    TSharedPtr<SMultiLineEditableTextBox> PasteBox;
    TSharedPtr<SEditableTextBox> RuleQueryBox;
    TSharedPtr<FNodeForgeEditorControlService> Service;
    bool bSyncingInput = false;
    uint64 DisplayedRevision = MAX_uint64;
};

TSharedRef<SWidget> NodeForgePanel::Create()
{
    return NodeForgeInstallationPanel::Create([]() -> TSharedRef<SWidget>
    { return SNew(SNodeForgePanel).SharedService(NodeForgePanel::GetProductService()); });
}
TSharedRef<SWidget> NodeForgePanel::CreateForReview(const FString& ReportPath)
{ return SNew(SNodeForgePanel).ReviewReportPath(ReportPath); }

