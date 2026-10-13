#include "NodeForgeInstallationPanel.h"
#include "NodeForgeInstallation.h"
#include "Misc/Paths.h"
#include "Misc/EngineVersion.h"
#include "Styling/AppStyle.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"

class SNodeForgeInstallationPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SNodeForgeInstallationPanel) {}
    SLATE_END_ARGS()
    void Construct(const FArguments&, TFunction<TSharedRef<SWidget>()> InProductPanel)
    {
        ProductPanel = MoveTemp(InProductPanel);
        ChildSlot [SAssignNew(Content, SBox)];
        NodeForgeInstallation::FBinding Existing; FString Error, Catalog;
        if (NodeForgeInstallation::Read(Existing, Error) && NodeForgeInstallation::CatalogIdentity(Catalog, Error))
        {
            Content->SetContent(ProductPanel()); return;
        }
        Available = NodeForgeInstallation::InspectCurrent(Preview, Error);
        const bool Conflict = FPaths::FileExists(FPaths::ProjectDir() / TEXT("Config/NodeForgeInstallation.json"));
        Available = Available && !Conflict;
        if (Conflict) Error = TEXT("installationExistingBindingConflict");
        Status = Available ? TEXT("确认后只在当前工程保存配置，不修改资产。以后打开此工程可直接使用。") : Explain(Error);
        Content->SetContent(
            SNew(SBorder).Padding(24)
            [SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14)
                [SNew(STextBlock).Text(FText::FromString(TEXT("欢迎使用 NodeForge"))).Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
                [SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("已自动识别当前工程和正在运行的引擎。确认后进入功能页，不需要填写路径或运行命令。")))]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
                [SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(FString::Printf(TEXT("工程：%s\n%s\n\n引擎：UE %s\n%s\n\n插件：%s"),
                    *FPaths::GetBaseFilename(FPaths::GetProjectFilePath()), *FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()),
                    *FEngineVersion::Current().ToString(EVersionComponent::Patch), *Preview.EngineRoot, *Preview.PluginDirectory)))]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
                [SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("默认读取工程测试内容、引擎基础资源和有限材质函数目录。生成仍限于 /Game/NodeForgeTests/，只创建新资产。AI 接入可在安装完成后另行配置。")))]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
                [SNew(SButton).Tag(TEXT("NodeForge.InstallationConfirm"))
                    .Text(FText::FromString(TEXT("确认当前工程并开始使用"))).IsEnabled(Available)
                    .OnClicked_Lambda([this]() { return Confirm(); })]
                + SVerticalBox::Slot().AutoHeight()
                [SNew(STextBlock).Tag(TEXT("NodeForge.InstallationStatus"))
                    .AutoWrapText(true).Text_Lambda([this]() { return FText::FromString(Status); })]
            ]);
    }
private:
    TFunction<TSharedRef<SWidget>()> ProductPanel;
    TSharedPtr<SBox> Content;
    NodeForgeInstallation::FBinding Preview;
    FString Status;
    bool Available = false;
    static FString Explain(const FString& Error)
    {
        if (Error == TEXT("installationPackageMissing")) return TEXT("当前插件缺少完整安装资料。请使用完整的 NodeForge 插件包，不能只复制 DLL 或旧 PluginStage。");
        if (Error == TEXT("installationExistingBindingConflict")) return TEXT("当前工程已有不同或无效的 NodeForge 配置。原配置已保留，请按原安装记录处理冲突后再打开；这里不会覆盖它。");
        return TEXT("安装检查未通过，尚未写入配置。请检查插件包、引擎版本与是否存在重复安装。诊断：") + Error;
    }
    FReply Confirm()
    {
        FString Receipt, Error;
        if (NodeForgeInstallation::ConfirmCurrent(Receipt, Error)) Content->SetContent(ProductPanel());
        else Status = Explain(Error) + (Receipt.IsEmpty() ? FString() : TEXT("\n安装记录：") + Receipt);
        return FReply::Handled();
    }
};
TSharedRef<SWidget> NodeForgeInstallationPanel::Create(TFunction<TSharedRef<SWidget>()> ProductPanel)
{
    return SNew(SNodeForgeInstallationPanel, MoveTemp(ProductPanel));
}
