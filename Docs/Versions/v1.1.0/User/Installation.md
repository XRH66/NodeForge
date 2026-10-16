# 选择安装方式

NodeForge v1.1.0 提供插件和可选的 AI 接入包。两者使用同一套插件功能。

| 使用方式 | 下载内容 | 安装后怎样使用 |
| --- | --- | --- |
| 在 UE 面板中操作 | 插件 ZIP | 载入 JSON、检查、生成资产 |
| 让 AI 操作插件 | 插件 ZIP 和 AI 接入 ZIP | 让支持 MCP 的 AI 客户端读取规则、查询资源和提交任务 |

本版适用于 **Windows 64 位、官方 UE 5.3.2**。文件与校验值见 [下载页面](Download.md)；下载需要登录有仓库访问权的 GitHub 账号，备用入口为 [v1.1.0 Release](https://github.com/XRH66/NodeForge/releases/tag/v1.1.0)。安装时选择插件或 AI 接入 ZIP；GitHub 自动生成的 `Source code` 是源码包。

## 在 UE 中手动使用

1. 下载 `NodeForge-1.1.0-UE5.3.2-Windows.zip`。
2. 关闭工程，将解压后的 `NodeForge` 文件夹放入引擎的 `Engine/Plugins/Marketplace` 文件夹。
3. 打开工程并启用插件，从 **窗口（Window）→ NodeForge** 打开面板。
4. 首次打开时确认工程和引擎位置。
5. 载入附带示例，检查，再生成资产。

这种方式无需安装 Python 或配置 MCP。完整步骤见 [安装插件](PluginInstallation.md) 和 [生成第一个材质](GettingStarted.md)。

## 让 AI 使用

1. 准备好官方 UE 5.3.2 引擎目录和 AI 客户端名称。
2. 下载 `NodeForge-AI-1.1.0-Windows.zip`，或者让有本机操作权限的 AI 下载。
3. 使用接入包中的安装工具完成插件安装、客户端一次配置和工具检查；之后正常打开工程连接。
4. 客户端加载配置后，先让 AI 读取插件说明并查询工程状态。

AI 接入包已包含 Python 和连接服务。已经安装普通插件时，只需补装 AI 部分；未安装时，工具会下载同版插件。执行安装的 AI 需要下载文件、运行本机程序和写入配置的权限。

可复制的安装提示词和命令见 [连接 AI](AiInstallation.md)。
