#pragma once
#include "CoreMinimal.h"

// Product destinations shared by the panel, settings and installation help.
namespace NodeForgeProductInfo
{
    inline const TCHAR* Website() { return TEXT("https://nodeforge-docs-xrh66.yellowcarp128.chatgpt.site/"); }
    inline FString Manual() { return FString(Website()) + TEXT("User/Readme.html"); }
    inline FString AiGuide() { return FString(Website()) + TEXT("User/AiInstallation.html"); }
    inline const TCHAR* Repository() { return TEXT("https://github.com/XRH66/NodeForge"); }
    inline const TCHAR* Name() { return TEXT("NodeForge · 节点熔炉"); }
    inline const TCHAR* Description() { return TEXT("从 JSON 创建可编辑的材质与蓝图，读取资源和节点规则，并在保护原资产的前提下准备新副本。支持官方 UE5.3.2 / Windows 64 位。"); }
}
