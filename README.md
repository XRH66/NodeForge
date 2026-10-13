# NodeForge 节点熔炉

NodeForge是Unreal Engine编辑器插件，将受控JSON转换为可编辑的蓝图和材质资产，并提供来源读取、完整导出、副本准备、差异预览及有界资源查询。已有资产受保护，支持范围以实际目录和版本资料为准。

交付采用**一套插件核心、两个安装入口、同一仓库与同一Release**。普通用户下载插件；AI用户通过统一工具安装匹配的可选接入部分。

| 内容 | 准确版本与状态 |
| --- | --- |
| 普通插件 | Build137 Repair5，Windows x64、官方UE5.3.2；复制到工程并首次确认，无需Python/MCP |
| 可选AI接入包 | Build137Repair5Ai1；自带Python3.12.4/SDK2.3.0及锁定依赖，匹配137同核心；新装/补装与真实只读连接通过 |
| 历史AI完整候选 | Build136 Repair6 / 桥0.3.6 / SDK2.3.0，已有有限验收；DLL与137不同，不作为137配套附件 |
| 配套AI资料 | Mcp10R1；功能正文继承Mcp9R19，安装身份绑定137；136历史产品验收保留原身份 |

从[Build137 Repair5 Release](https://github.com/XRH66/NodeForge/releases/tag/build137-repair5)下载 `NodeForgeBuild137Repair5PluginCandidate.zip`，按[普通安装指南](Docs/Installation.md)使用。GitHub自动源码ZIP不包含编译插件。

仓库当前私有，下载与指南需要仓库访问权限。文档站仍仅所有者可访问，公开安装指南、下载地址和网站AI安装指令尚未开放。

AI下载同Release的`NodeForgeBuild137Repair5Ai1.zip`，先核对`AiReleaseManifest.json`和`AiChecksums.sha256`，调用包内`Scripts/InstallNodeForgeAi.ps1`。工具复用原安装验证，支持明确指定的MCP JSON或Codex TOML配置；`-Connect`实际启动专用编辑器并检查连接。已有普通插件只补AI部分，复用同一DLL与绑定。AI须具备本机执行和配置写入权限，见[AI安装指南](Docs/AiInstallation.md)。

| 路径 | 用途 |
| --- | --- |
| `Plugins/NodeForge/Source/` | 与137普通候选对应的83个native源码文件 |
| `Scripts/` | CLI/MCP桥、安装验证、构建与历史验收工具 |
| `Contracts/`、`Catalog/`、`Examples/`、`Tests/` | 协议、目录、JSON样例与测试来源 |
| `Tools/`、`Docs/PluginControl/DocumentationSources/` | 固定依赖/许可、文档渲染与正文来源 |
| `Docs/Versions/Build136/` | R13已验历史AI资料与R19阅读资料，不冒充137 AI验收 |
| `Docs/Versions/Build137/Mcp10R1/` | 137配套MCP资料、准确核心目录和安装说明；452个可读取页面 |
| `Releases/Build137Repair5/` | 精确包清单、兼容基线及发布说明 |

[仓库与Release方案](Docs/GitHubReleaseDeliveryPlan.md)定义统一交付门槛；[开发说明](Docs/RepositoryGuide.md)说明本机历史脚本、编译和测试前提。验证工程、历史输出、缓存、私人会话与工作管理记录留本机，Git保留测试程序和JSON。

目录仍experimental，包含71材质类、327蓝图签名；不宣称所有参数、组合与平台已验证。实际范围见[发布说明](Releases/Build137Repair5/ReleaseNotes.md)。统一开源许可尚未选择，实际第三方许可保存在插件Licenses/ThirdPartyNotices及工具许可文件。
