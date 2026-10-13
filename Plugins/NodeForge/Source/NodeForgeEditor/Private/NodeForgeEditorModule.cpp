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

class SNodeForgePrototypePanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SNodeForgePrototypePanel) {}
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
        ChildSlot
        [
            SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(0)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(14,10,14,8)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight()
                    [SNew(STextBlock).Text(FText::FromString(TEXT("NodeForge · 节点熔炉")))
                     .Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))]
                    + SVerticalBox::Slot().AutoHeight().Padding(0,2,0,0)
                    [SNew(STextBlock).Text(FText::FromString(TEXT("把构图数据变成可编辑的蓝图与材质"))).ColorAndOpacity(Muted).AutoWrapText(true)]
                    + SVerticalBox::Slot().AutoHeight().Padding(0,4,0,0)
                    [SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)
                        [SNew(SButton).Tag(TEXT("NodeForge.AiStart"))
                            .Text(FText::FromString(TEXT("连接 AI")))
                            .ToolTipText(FText::FromString(TEXT("需要先完成一次 AI 配套安装；只启动当前工程的同一控制通道。")))
                            .OnClicked_Lambda([this]() { NodeForgeInstallation::OnReady().Broadcast();
                                State().ResultNotice = ProductChannel.IsValid() ? TEXT("AI 已连接当前工程。") : TEXT("AI 通道未启动，请检查一次安装是否完成及当前安装身份。");
                                return FReply::Handled(); })]
                        + SHorizontalBox::Slot().AutoWidth()
                        [SNew(SButton).Tag(TEXT("NodeForge.AiStop"))
                            .Text(FText::FromString(TEXT("停止本工程 AI")))
                            .IsEnabled_Lambda([]() { return ProductChannel.IsValid(); })
                            .OnClicked_Lambda([]() { if (const auto Channel = ProductChannel.Pin()) Channel->RequestStop(); return FReply::Handled(); })]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0,2,0,0)
                    [SNew(STextBlock).Text_Lambda([]()
                    {
                        const TSharedPtr<IPlugin> Plugin=IPluginManager::Get().FindPlugin(TEXT("NodeForge"));
                        return FText::FromString(FString::Printf(TEXT("%s · UE %s · 项目：%s"),
                            Plugin ? *Plugin->GetDescriptor().VersionName : TEXT("版本不可读"),
                            *FEngineVersion::Current().ToString(EVersionComponent::Patch),
                            *FPaths::GetBaseFilename(FPaths::GetProjectFilePath())));
                    }).ColorAndOpacity(Muted)]
                ]
                + SVerticalBox::Slot().FillHeight(1.0f)
                [
                        SAssignNew(InputBody,SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight().Padding(14,0,14,6)
                        [SNew(STextBlock).Text(FText::FromString(TEXT("构图输入"))).Font(FAppStyle::GetFontStyle("NormalFontBold"))]
                        + SVerticalBox::Slot().FillHeight(1.0f).Padding(14,0)
                        [
                            SNew(SBox).MinDesiredHeight(176)
                            [SAssignNew(PasteBox,SMultiLineEditableTextBox).Tag(FName(TEXT("NodeForge.Input")))
                             .Text(FText::FromString(State().Input.Draft().Text))
                             .HintText(FText::FromString(TEXT("在这里粘贴 NodeForgeGraph JSON\n使用当前节点规则；输入上限 64 KiB。")))
                             .Margin(FMargin(10)).IsEnabled_Lambda([this](){return !Service->IsBusy();})
                             .OnTextChanged(this,&SNodeForgePrototypePanel::DraftChanged)]
                        ]
                        + SVerticalBox::Slot().AutoHeight().Padding(14,4,14,0)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
                            [SNew(STextBlock).Tag(FName(TEXT("NodeForge.InputSource"))).AutoWrapText(true).ColorAndOpacity(Muted)
                             .Text_Lambda([this](){return FText::FromString(State().Input.DescribeSource());})]
                            + SHorizontalBox::Slot().AutoWidth().Padding(8,0,0,0)
                            [SNew(SButton).Tag(FName(TEXT("NodeForge.RestoreInput"))).Text(FText::FromString(TEXT("恢复上次输入")))
                             .ToolTipText(FText::FromString(TEXT("恢复最近一次整体载入前的草稿；旧检查和旧预览仍需重做。")))
                             .ContentPadding(FMargin(8,3)).IsEnabled_Lambda([this](){return !Service->IsBusy() && State().Input.CanRestore();})
                             .OnClicked_Lambda([this]()
                             {
                                 FString Error; if(Service->RestoreDraft(Error)) {PasteBox->SetText(FText::FromString(State().Input.Draft().Text));State().ResultNotice=TEXT("已恢复上次输入；请重新检查或预览。");}
                                 return FReply::Handled();
                             })]
                        ]
                        + SVerticalBox::Slot().AutoHeight().Padding(14,6,14,4)
                        [
                            SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(8,8))
                            + SWrapBox::Slot()
                            [SNew(SButton).Tag(FName(TEXT("NodeForge.Paste"))).Text(FText::FromString(TEXT("粘贴剪贴板"))).ContentPadding(FMargin(12,5))
                             .IsEnabled_Lambda([this](){return !Service->IsBusy();})
                             .OnClicked_Lambda([this]()
                             {
                                 FNodeForgeEditorBusinessScope Guard(*Service); if (!Guard) return FReply::Handled();
                                 FString Text;FPlatformApplicationMisc::ClipboardPaste(Text);
                                 if(ReplaceInput(Text,TEXT("clipboard")))State().ResultNotice=TEXT("已替换输入区，尚未执行生成。");
                                 return FReply::Handled();
                             })]
                            + SWrapBox::Slot()
                            [SNew(SButton).Tag(FName(TEXT("NodeForge.Preflight"))).Text(FText::FromString(TEXT("检查输入"))).ContentPadding(FMargin(12,5))
                             .ToolTipText(FText::FromString(TEXT("只检查输入与目标可用性，不生成或预留资产。")))
                             .IsEnabled_Lambda([this](){return !Service->IsBusy();}).OnClicked(this,&SNodeForgePrototypePanel::PreflightGraph)]
                            + SWrapBox::Slot()
                            [SNew(SButton).Tag(FName(TEXT("NodeForge.Generate"))).ButtonStyle(FAppStyle::Get(),"FlatButton.Primary")
                             .Text(FText::FromString(TEXT("生成新资产"))).ContentPadding(FMargin(18,5))
                             .ToolTipText(FText::FromString(TEXT("使用输入区 JSON 校验、构图、编译并保存。已有同名目标会被拒绝。")))
                             .IsEnabled_Lambda([this](){return !Service->IsBusy();}).OnClicked(this,&SNodeForgePrototypePanel::ImportPaste)]
                            + SWrapBox::Slot()
                            [SNew(SButton).Tag(FName(TEXT("NodeForge.LoadFile"))).Text(FText::FromString(TEXT("载入 JSON 文件"))).ContentPadding(FMargin(12,5))
                             .ToolTipText(FText::FromString(TEXT("先载入同一个输入区，可检查或编辑；选择文件不会生成资产。")))
                             .IsEnabled_Lambda([this](){return !Service->IsBusy();}).OnClicked(this,&SNodeForgePrototypePanel::ImportJson)]
                            + SWrapBox::Slot()
                            [SNew(SButton).Tag(FName(TEXT("NodeForge.PrepareCopy"))).Text(FText::FromString(TEXT("准备新副本"))).ContentPadding(FMargin(12,5))
                             .ToolTipText(FText::FromString(TEXT("用当前输入准备安全的新名称和新请求ID，只改身份、保留图形。放回输入区，确认后再明确生成；不会覆盖原资产。")))
                             .IsEnabled_Lambda([this](){return !Service->IsBusy() && !State().Input.Draft().Text.TrimStartAndEnd().IsEmpty();})
                             .OnClicked(this,&SNodeForgePrototypePanel::PrepareNewCopy)]
                        ]
                        + SVerticalBox::Slot().AutoHeight().Padding(14,0,14,4)
                        [
                            SAssignNew(AdvancedBox,SBox).MaxDesiredHeight(this,&SNodeForgePrototypePanel::GetAdvancedMaxHeight)
                            [
                                SNew(SScrollBox).Tag(FName(TEXT("NodeForge.AdvancedScroll")))
                                + SScrollBox::Slot()
                                [
                                    SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)
                        [
                            SNew(SExpandableArea).Tag(FName(TEXT("NodeForge.RuleSection"))).InitiallyCollapsed(true).AreaTitle(FText::FromString(TEXT("节点规则与资源签名")))
                            .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(FMargin(8))
                            .BodyContent()
                            [
                                SNew(SVerticalBox)
                                + SVerticalBox::Slot().AutoHeight().Padding(0,2,0,8)
                                [SAssignNew(RuleQueryBox,SEditableTextBox)
                                 .HintText(FText::FromString(TEXT("节点或函数名称；留空查询该类全部规则")))
                                 .IsEnabled_Lambda([this](){return !Service->IsBusy();})]
                                + SVerticalBox::Slot().AutoHeight()
                                [
                                    SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(8,8))
                                    + SWrapBox::Slot()
                                    [SNew(SButton).Tag(FName(TEXT("NodeForge.RulesMaterial"))).Text(FText::FromString(TEXT("查询材质规则"))).ContentPadding(FMargin(10,6))
                                     .IsEnabled_Lambda([this](){return !Service->IsBusy();}).OnClicked_Lambda([this](){return RunTaskRules(TEXT("Material"));})]
                                    + SWrapBox::Slot()
                                    [SNew(SButton).Text(FText::FromString(TEXT("查询蓝图规则"))).ContentPadding(FMargin(10,6))
                                     .IsEnabled_Lambda([this](){return !Service->IsBusy();}).OnClicked_Lambda([this](){return RunTaskRules(TEXT("Blueprint"));})]
                                    + SWrapBox::Slot()
                                    [SNew(SButton).Tag(FName(TEXT("NodeForge.Catalog"))).Text(FText::FromString(TEXT("导出全部节点规则"))).ContentPadding(FMargin(10,6))
                                     .IsEnabled_Lambda([this](){return !Service->IsBusy();}).OnClicked_Lambda([this](){return RunReadOnly();})]
                                    + SWrapBox::Slot()
                                    [SNew(SButton).Tag(FName(TEXT("NodeForge.InspectSelection"))).Text(FText::FromString(TEXT("读取选中材质 / 函数"))).ContentPadding(FMargin(10,6))
                                     .ToolTipText(FText::FromString(TEXT("先在内容浏览器选中一个已保存材质或材质函数；只读提取真实参数或引脚。")))
                                     .IsEnabled_Lambda([this](){return !Service->IsBusy();}).OnClicked(this,&SNodeForgePrototypePanel::InspectSelectedResource)]
                                    + SWrapBox::Slot()
                                    [SNew(SButton).Tag(FName(TEXT("NodeForge.CopyReadOnly"))).Text(FText::FromString(TEXT("复制读取内容"))).ContentPadding(FMargin(10,6))
                                     .ToolTipText(FText::FromString(TEXT("复制最近读取的完整规则、签名或导出构图；不是结果摘要。")))
                                     .IsEnabled_Lambda([this](){return !Service->IsBusy() && !State().LastReadOnlyData.IsEmpty();})
                                     .OnClicked_Lambda([this](){FNodeForgeEditorBusinessScope Guard(*Service);if(!Guard)return FReply::Handled();FPlatformApplicationMisc::ClipboardCopy(*State().LastReadOnlyData);State().ResultNotice=TEXT("完整读取内容已复制。");return FReply::Handled();})]
                                ]
                            ]
                        ]
                        + SVerticalBox::Slot().AutoHeight()
                        [
                            SNew(SExpandableArea).Tag(FName(TEXT("NodeForge.CopySection"))).InitiallyCollapsed(true).AreaTitle(FText::FromString(TEXT("已有构图与新副本")))
                            .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(FMargin(8))
                            .BodyContent()
                            [
                                SNew(SVerticalBox)
                                + SVerticalBox::Slot().AutoHeight().Padding(0,2,0,8)
                                [SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(Muted)
                                 .Text(FText::FromString(TEXT("选中构图 → 只读导出到输入区 → 编辑新目标 → 预览差异 → 生成新资产。")))]
                                + SVerticalBox::Slot().AutoHeight()
                                [
                                    SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(8,8))
                                    + SWrapBox::Slot()
                                    [SNew(SButton).Tag(FName(TEXT("NodeForge.ExportGraph"))).Text(FText::FromString(TEXT("只读导出选中构图"))).ContentPadding(FMargin(10,6))
                                     .ToolTipText(FText::FromString(TEXT("读取实际图并放入输入区，不修改源资产。无法完整表达的配置会拒绝。")))
                                     .IsEnabled_Lambda([this](){return !Service->IsBusy();}).OnClicked(this,&SNodeForgePrototypePanel::ExportSelectedGraph)]
                                    + SWrapBox::Slot()
                                    [SNew(SButton).Tag(FName(TEXT("NodeForge.PreviewGraph"))).Text(FText::FromString(TEXT("预览新副本差异"))).ContentPadding(FMargin(10,6))
                                     .ToolTipText(FText::FromString(TEXT("比较当前输入与它绑定的可靠导出源图；仅预览，不生成。")))
                                     .IsEnabled_Lambda([this](){return !Service->IsBusy() && State().Input.Draft().ExportOrigin.IsValid();}).OnClicked(this,&SNodeForgePrototypePanel::PreviewGraph)]
                                    + SWrapBox::Slot()
                                    [SNew(SButton).Tag(FName(TEXT("NodeForge.Relayout"))).Text(FText::FromString(TEXT("按上次成功输入重排新副本"))).ContentPadding(FMargin(10,6))
                                     .ToolTipText(FText::FromString(TEXT("重新生成新目标，移除上次输入坐标；不包含导入后的手工编辑，不修改原资产。")))
                                     .IsEnabled_Lambda([this](){return !Service->IsBusy() && !State().LastImportInputPath.IsEmpty();})
                                     .OnClicked(this,&SNodeForgePrototypePanel::RelayoutSuccess)]
                                ]
                                + SVerticalBox::Slot().AutoHeight().Padding(0,4,0,0)
                                [SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(Muted).Text_Lambda([this]()
                                {return FText::FromString(State().LastSuccessAssetPath.IsEmpty()?TEXT("尚无可重排的成功输入快照。"):
                                    TEXT("重排基于：")+FPaths::GetCleanFilename(State().LastSuccessAssetPath)+TEXT("（归档成功输入，不读取外部原文件）"));})]
                            ]
                        ]
                                ]
                            ]
                        ]
                    ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(FMargin(14,8))
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)
                        [SNew(STextBlock).Font(FAppStyle::GetFontStyle("NormalFontBold"))
                         .Text_Lambda([this](){return FText::FromString(Service->IsBusy()?TEXT("正在处理…"):
                             ResultIsStale()?TEXT("上次输入的结果（已过期）"):
                             State().bResultRelayout?TEXT("重排副本结果（基于成功快照）"):TEXT("最近结果"));})]
                        + SVerticalBox::Slot().AutoHeight()
                        [SNew(SBox).HeightOverride(96)
                         [SNew(SMultiLineEditableTextBox).Tag(FName(TEXT("NodeForge.ResultSummary"))).IsReadOnly(true).AutoWrapText(true).Margin(FMargin(10))
                          .Text_Lambda([this](){return FText::FromString(State().ResultText);})]]
                        + SVerticalBox::Slot().AutoHeight().Padding(0,4,0,0)
                        [SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(Muted)
                         .Text_Lambda([this](){return FText::FromString(State().ResultNotice);})
                         .Visibility_Lambda([this](){return State().ResultNotice.IsEmpty()?EVisibility::Collapsed:EVisibility::Visible;})]
                        + SVerticalBox::Slot().AutoHeight().Padding(0,6,0,0)
                        [
                            SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(8,8))
                            + SWrapBox::Slot()
                            [SNew(SButton).Tag(FName(TEXT("NodeForge.LocateAsset"))).Text(FText::FromString(TEXT("定位资产"))).ContentPadding(FMargin(10,6))
                             .IsEnabled_Lambda([this](){return !Service->IsBusy() && !ResultIsStale() && !State().LastAssetPath.IsEmpty();}).OnClicked(this,&SNodeForgePrototypePanel::LocateAsset)]
                            + SWrapBox::Slot()
                            [SNew(SButton).Tag(FName(TEXT("NodeForge.OpenAsset"))).Text(FText::FromString(TEXT("打开资产"))).ContentPadding(FMargin(10,6))
                             .IsEnabled_Lambda([this](){return !Service->IsBusy() && !ResultIsStale() && !State().LastAssetPath.IsEmpty();}).OnClicked(this,&SNodeForgePrototypePanel::OpenAsset)]
                            + SWrapBox::Slot()
                            [SNew(SButton).Text(FText::FromString(TEXT("打开完整报告"))).ContentPadding(FMargin(10,6))
                             .IsEnabled_Lambda([this](){return !Service->IsBusy() && HasReport();}).OnClicked(this,&SNodeForgePrototypePanel::OpenReport)]
                            + SWrapBox::Slot()
                            [SNew(SButton).Tag(FName(TEXT("NodeForge.CopyReport"))).Text(FText::FromString(TEXT("复制完整报告"))).ContentPadding(FMargin(10,6))
                             .ToolTipText(FText::FromString(TEXT("复制报告原文及诊断交给 AI；报告不是可导入的构图数据。")))
                             .IsEnabled_Lambda([this](){return !Service->IsBusy() && HasReport();}).OnClicked(this,&SNodeForgePrototypePanel::CopyResultReport)]
                        ]
                    ]
                ]
            ]
        ];
    }

    virtual ~SNodeForgePrototypePanel() override { if(Service) Service->DetachPanel(); }
    virtual void Tick(const FGeometry& Geometry, const double Time, const float DeltaTime) override
    {
        SCompoundWidget::Tick(Geometry, Time, DeltaTime);
        SyncDraftToView();
    }

private:
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
    FOptionalSize GetAdvancedMaxHeight() const
    {
        if(!InputBody || !AdvancedBox)return FOptionalSize(0.0f);
        // The body receives the actual remainder after header/footer allocation.
        // Reserve input space BEFORE giving the AutoHeight advanced area its budget;
        // MinDesiredHeight alone cannot protect a FillHeight slot from fixed siblings.
        float FixedHeight=0.0f;
        const FChildren* Children=InputBody->GetChildren();
        for(int32 Index=0;Index<Children->Num();++Index)
        {
            const auto& Slot=InputBody->GetSlot(Index);
            const auto& Widget=Slot.GetWidget();
            if(Widget->GetVisibility()==EVisibility::Collapsed)continue;
            const FMargin Padding=Slot.GetPadding();
            FixedHeight+=Padding.Top+Padding.Bottom;
            if(Slot.GetSizeRule()==FSizeParam::SizeRule_Auto && &Widget.Get()!=AdvancedBox.Get())
                FixedHeight+=Widget->GetDesiredSize().Y;
        }
        // Cached body geometry is independent of the advanced child's desired height.
        // Subsequent native layout frames settle wrapping and resize without feedback.
        const float BodyHeight=InputBody->GetCachedGeometry().GetLocalSize().Y;
        return FOptionalSize(FMath::Clamp(BodyHeight-FixedHeight-176.0f,0.0f,160.0f));
    }
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
        Service->PreviewFromUI();
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
        FScopedSlowTask Progress(1.0f,FText::FromString(TEXT("正在校验节点、构图并编译…")));
        Service->GenerateFromUI([&Progress](){Progress.MakeDialog(false);});
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
        const void* Parent = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(AsShared());
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
        FNodeForgeEditorBusinessScope Guard(*Service); if (!Guard) return FReply::Handled();
        if (!State().LastReportPath.IsEmpty() && IFileManager::Get().FileExists(*State().LastReportPath) &&
            !FPlatformProcess::LaunchFileInDefaultExternalApplication(*State().LastReportPath))
        {
            State().ResultNotice=TEXT("无法使用默认应用打开报告，请使用“复制完整报告”。");
        }
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

    TSharedPtr<SVerticalBox> InputBody;
    TSharedPtr<SBox> AdvancedBox;
    TSharedPtr<SMultiLineEditableTextBox> PasteBox;
    TSharedPtr<SEditableTextBox> RuleQueryBox;
    TSharedPtr<FNodeForgeEditorControlService> Service;
    bool bSyncingInput = false;
    uint64 DisplayedRevision = MAX_uint64;
};

TSharedPtr<FNodeForgeEditorControlService> NodeForgePanel::GetProductService() { check(IsInGameThread()); return ProductService.Pin(); }
TSharedPtr<FNodeForgeEditorControlChannel> NodeForgePanel::GetControlChannel() { check(IsInGameThread()); return ProductChannel.Pin(); }
TSharedRef<SWidget> NodeForgePanel::Create()
{
    return NodeForgeInstallationPanel::Create([]() -> TSharedRef<SWidget>
    { return SNew(SNodeForgePrototypePanel).SharedService(GetProductService()); });
}
TSharedRef<SWidget> NodeForgePanel::CreateForReview(const FString& ReportPath)
{ return SNew(SNodeForgePrototypePanel).ReviewReportPath(ReportPath); }

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
