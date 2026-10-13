#include "NodeForgePanelPreferences.h"
#include "NodeForgeInstallation.h"
#include "NodeForgePrototype.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/SecureHash.h"

namespace
{
FString SettingsPath() { return NodeForgeInstallation::StateDirectory() / TEXT("Settings/Panel.json"); }
FString IconPath(const TArray<uint8>& Bytes)
{
    FSHAHash Hash;FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),Hash.Hash);
    return FPaths::ConvertRelativePathToFull(NodeForgeInstallation::StateDirectory()/TEXT("Settings/ClientIcons")/(Hash.ToString()+TEXT(".png")));
}
TSharedRef<FJsonObject> Json(const FNodeForgePanelPreferences& Value)
{
    auto Out=MakeShared<FJsonObject>(); Out->SetStringField(TEXT("format"),TEXT("NodeForgePanelPreferences1"));
    Out->SetStringField(TEXT("directory"),Value.Directory); Out->SetStringField(TEXT("client"),Value.Client);
    TArray<TSharedPtr<FJsonValue>> Clients;
    for(const auto& C:Value.CustomClients)
    {
        auto Item=MakeShared<FJsonObject>(); Item->SetStringField(TEXT("id"),C.Id); Item->SetStringField(TEXT("name"),C.Name);
        Item->SetStringField(TEXT("description"),C.Description); Item->SetStringField(TEXT("icon"),C.PendingIconPng.IsEmpty()?C.IconPath:IconPath(C.PendingIconPng));
        Clients.Add(MakeShared<FJsonValueObject>(Item));
    }
    Out->SetArrayField(TEXT("clients"),Clients); return Out;
}
}
TArray<FNodeForgeClientDefinition> FNodeForgePanelPreferences::Clients() const
{
    const auto Plugin=IPluginManager::Get().FindPlugin(TEXT("NodeForge"));
    const FString Resources=Plugin ? Plugin->GetBaseDir()/TEXT("Resources/Ui") : FString();
    TArray<FNodeForgeClientDefinition> Result={
        {TEXT("deepseek"),TEXT("DeepSeek"),TEXT("DeepSeek Harness"),Resources/TEXT("DeepSeek.png")},
        {TEXT("codex"),TEXT("Codex"),TEXT("OpenAI Codex"),Resources/TEXT("CodexOpenAi.png")},
        {TEXT("claude"),TEXT("Claude"),TEXT("Claude Code / Desktop"),Resources/TEXT("Claude.png")}};
    Result.Append(CustomClients); return Result;
}
bool FNodeForgePanelPreferences::Validate(FString& Error) const
{
    Error.Empty();
    FString Trimmed=Directory.TrimStartAndEnd();
    if(Trimmed!=Directory||!NodeForgePrototype::ValidateTargetPath(Directory/TEXT("MExample"),Error))
        {Error=TEXT("默认目录不符合当前生成目标规则：")+Error;return false;}
    if(CustomClients.Num()>32){Error=TEXT("自定义客户端最多32项。");return false;}
    for(const auto& C:CustomClients)
    {
        FGuid Id;const FString IconRoot=FPaths::ConvertRelativePathToFull(NodeForgeInstallation::StateDirectory()/TEXT("Settings/ClientIcons"));
        if(!C.Id.StartsWith(TEXT("custom-"))||!FGuid::ParseExact(C.Id.Mid(7),EGuidFormats::Digits,Id)
            ||(!C.IconPath.IsEmpty()&&(FPaths::IsRelative(C.IconPath)||!FPaths::IsUnderDirectory(C.IconPath,IconRoot)||FPaths::GetExtension(C.IconPath)!=TEXT("png"))))
            {Error=TEXT("客户端图标必须来自本工程应用后的图像；原设置保持。");return false;}
    }
    TSet<FString> Ids,Names;
    for(const auto& C:Clients())
    {
        if(C.Id.IsEmpty()||C.Name.IsEmpty()||C.Name!=C.Name.TrimStartAndEnd()||C.Name.Len()>40||C.Description.Len()>256
            ||Ids.Contains(C.Id)||Names.Contains(C.Name.ToLower()))
            {Error=TEXT("客户端名称为空、重复或过长。");return false;}
        Ids.Add(C.Id);Names.Add(C.Name.ToLower());
    }
    if(!Ids.Contains(Client)){Error=TEXT("请选择实际存在的客户端。");return false;}
    return true;
}
bool FNodeForgePanelPreferences::Load(FString& Error)
{
    Error.Empty(); const FString Path=SettingsPath();
    if(!IFileManager::Get().FileExists(*Path))return true;
    if(IFileManager::Get().FileSize(*Path)>65536){Error=TEXT("设置文件超限；原文件保留。");return false;}
    FString Text,Format;TSharedPtr<FJsonObject> Object;const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;
    FNodeForgePanelPreferences Loaded;
    if(!FFileHelper::LoadFileToString(Text,*Path)||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Object)||!Object
        ||!Object->TryGetStringField(TEXT("format"),Format)||Format!=TEXT("NodeForgePanelPreferences1")
        ||!Object->TryGetStringField(TEXT("directory"),Loaded.Directory)||!Object->TryGetStringField(TEXT("client"),Loaded.Client)
        ||!Object->TryGetArrayField(TEXT("clients"),Values)||Values->Num()>32)
        {Error=TEXT("设置文件无法读取；原文件保留，未覆盖。");return false;}
    for(const auto& Item:*Values)
    {
        const TSharedPtr<FJsonObject>* C=nullptr;FNodeForgeClientDefinition LoadedClient;
        if(!Item->TryGetObject(C)||!C||!(*C)->TryGetStringField(TEXT("id"),LoadedClient.Id)||!(*C)->TryGetStringField(TEXT("name"),LoadedClient.Name)
            ||!(*C)->TryGetStringField(TEXT("description"),LoadedClient.Description)||!(*C)->TryGetStringField(TEXT("icon"),LoadedClient.IconPath))
            {Error=TEXT("客户端设置格式有误；原文件保留。");return false;}
        Loaded.CustomClients.Add(MoveTemp(LoadedClient));
    }
    if(!Loaded.Validate(Error))return false;*this=MoveTemp(Loaded);return true;
}
bool FNodeForgePanelPreferences::Save(FString& Error)
{
    if(!Validate(Error))return false;
    auto Saved=*this;
    for(auto& C:Saved.CustomClients)if(!C.PendingIconPng.IsEmpty())
    {
        C.IconPath=IconPath(C.PendingIconPng);
        TArray<uint8> Existing;
        if(IFileManager::Get().FileExists(*C.IconPath))
        {if(!FFileHelper::LoadFileToArray(Existing,*C.IconPath)||Existing!=C.PendingIconPng){Error=TEXT("已有图标字节冲突；设置未提交。");return false;}}
        else
        {
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(C.IconPath),true);
            if(!FFileHelper::SaveArrayToFile(C.PendingIconPng,*C.IconPath)){Error=TEXT("图标保存失败；设置未提交。");return false;}
        }
        C.PendingIconPng.Empty();
    }
    const FString Path=SettingsPath(),Temporary=Path+TEXT(".")+FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT(".tmp");
    if(!NodeForgePrototype::WriteJson(Json(Saved),Temporary)||!IFileManager::Get().Move(*Path,*Temporary,true,true))
        {Error=TEXT("设置未保存；保留原设置和当前草稿。");return false;}
    *this=MoveTemp(Saved);return true;
}
bool FNodeForgePanelPreferences::Equals(const FNodeForgePanelPreferences& Other) const
{
    FString A,B;FJsonSerializer::Serialize(Json(*this),TJsonWriterFactory<>::Create(&A));
    FJsonSerializer::Serialize(Json(Other),TJsonWriterFactory<>::Create(&B));return A==B;
}
