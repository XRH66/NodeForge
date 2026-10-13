// UI selection and name preparation only; native read/report policy is shared with the API.
#include "NodeForgePanel.h"
#include "NodeForgeEditorControlService.h"
#include "NodeForgePrototype.h"
#include "NodeForgeInstallation.h"
#include "AssetRegistry/AssetData.h"
#include "Materials/Material.h"
#include "Materials/MaterialFunction.h"
#include "Engine/Blueprint.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace
{
bool FreshSelectionReport(const FString& Requested, const TCHAR* Suffix, FString& Report, FString& Input, FString& Error)
{
    Report=FPaths::ConvertRelativePathToFull(Requested);FPaths::NormalizeFilename(Report);
    if(!FPaths::CollapseRelativeDirectories(Report)
        || !FPaths::IsUnderDirectory(Report,NodeForgeInstallation::StateDirectory()/TEXT("Reports"))
        || FPaths::GetExtension(Report)!=TEXT("json"))
    {Error=TEXT("需要Reports内的新JSON报告路径。");return false;}
    Input=FPaths::GetPath(Report)/(FPaths::GetBaseFilename(Report)+Suffix);
    if(IFileManager::Get().FileExists(*Report) || IFileManager::Get().FileExists(*Input))
    {Error=TEXT("已有报告或输入拒绝覆盖。");return false;}
    return true;
}
}
int32 NodeForgePanel::InspectSelection(const TArray<FAssetData>& Selection,const FString& ReportPath,FString& Error)
{
    Error.Empty();
    if(Selection.Num()!=1 || !Selection[0].IsValid())
    {Error=TEXT("请只选中一个已保存的普通材质或材质函数。");return -1;}
    const bool Function=Selection[0].AssetClassPath==UMaterialFunction::StaticClass()->GetClassPathName();
    if(!Function && Selection[0].AssetClassPath!=UMaterial::StaticClass()->GetClassPathName())
    {Error=TEXT("该资源类型尚不支持只读检查。");return -1;}
    FString Report,Input,Hash;
    if(!FreshSelectionReport(ReportPath,TEXT("-SelectionInput.json"),Report,Input,Error))return -1;
    auto Args=MakeShared<FJsonObject>();Args->SetStringField(TEXT("kind"),Function?TEXT("MaterialFunction"):TEXT("MaterialParameters"));
    Args->SetStringField(TEXT("source"),Selection[0].GetObjectPathString());
    return FNodeForgeEditorControlService::ExecuteReadAt(TEXT("resources.inspect"),Args,Input,Report,Hash,Error);
}
int32 NodeForgePanel::ExportSelection(const TArray<FAssetData>& Selection,const FString& ReportPath,FString& Error)
{
    Error.Empty();
    if(Selection.Num()!=1 || !Selection[0].IsValid())
    {Error=TEXT("只读导出需选中一个已保存的普通材质或Actor蓝图。");return -1;}
    const bool Blueprint=Selection[0].AssetClassPath==UBlueprint::StaticClass()->GetClassPathName();
    if(!Blueprint && Selection[0].AssetClassPath!=UMaterial::StaticClass()->GetClassPathName())
    {Error=TEXT("该资源类型尚不支持只读构图导出。");return -1;}
    FString Report,Input,Hash,Target,Id;
    if(!FreshSelectionReport(ReportPath,TEXT("-ExportInput.json"),Report,Input,Error))return -1;
    const FString Source=Selection[0].GetObjectPathString();
    for(int32 Attempt=0;Attempt<16;++Attempt)
    {
        Id=TEXT("Export")+FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(16);
        const FString Seed=FString(TEXT("/Game/NodeForgeTests/"))+(Blueprint?TEXT("Blueprint"):TEXT("Material"))+TEXT("/UiExports/")+Id;
        if(!NodeForgePrototype::NewCopyTarget(Seed,Target,Error,32))return -1;
        if(NodeForgePrototype::ValidateNewTargetIdentity(Source,Id,Target,Error))break;
        Id.Empty();
    }
    if(Id.IsEmpty()){Error=TEXT("无法准备未占用的新导出身份；没有预留或生成资产。");return -1;}
    auto Args=MakeShared<FJsonObject>();Args->SetStringField(TEXT("assetKind"),Blueprint?TEXT("Blueprint"):TEXT("Material"));
    Args->SetStringField(TEXT("source"),Source);Args->SetStringField(TEXT("requestId"),Id);Args->SetStringField(TEXT("assetPath"),Target);
    Args->SetNumberField(TEXT("formatVersion"),3);
    return FNodeForgeEditorControlService::ExecuteReadAt(TEXT("graphs.export"),Args,Input,Report,Hash,Error);
}
