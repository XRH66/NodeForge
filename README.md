# NodeForge 节点熔炉

NodeForge是Unreal Engine编辑器插件，将受控JSON转换为可编辑的蓝图和材质资产，并提供来源读取、完整导出、副本准备、差异预览及有界资源查询。已有资产受保护，支持范围以实际目录和版本资料为准。

交付采用**一套插件核心、两个安装入口、同一仓库与同一Release**。普通用户下载插件；未来AI用户通过统一工具安装匹配的可选接入部分。

| 内容 | 准确版本与状态 |
| --- | --- |
| 普通插件 | Build137 Repair5，Windows x64、官方UE5.3.2；复制到工程并首次确认，无需Python/MCP |
| 可选AI接入包 | 尚未交付；固定运行环境、统一下载与客户端配置仍待实现及同核心联调 |
| 历史AI完整候选 | Build136 Repair6 / 桥0.3.6 / SDK2.3.0，已有有限验收；DLL与137不同，不作为137配套附件 |
| 当前阅读资料 | Mcp9R19；原运行AI资料为Mcp9R13，均保留真实版本身份 |

从[Build137 Repair5 Release](https://github.com/XRH66/NodeForge/releases/tag/build137-repair5)下载 `NodeForgeBuild137Repair5PluginCandidate.zip`，按[普通安装指南](Docs/Installation.md)使用。GitHub自动源码ZIP不包含编译插件。

仓库当前私有，下载与指南需要仓库访问权限。文档站仍仅所有者可访问，公开安装指南、下载地址和网站AI安装指令尚未开放。

AI接入将由具备本机权限的AI读取公开指南，调用统一安装工具完成匹配版本下载、工程/UE识别、固定运行环境、客户端配置及实际连接检查。已安装普通插件的用户可补装AI部分，复用同一DLL与绑定。支持MCP本身不等于已有运行环境或自动写配置权限。当前没有可执行的一键安装命令，见[AI状态](Docs/AiInstallation.md)。

| 路径 | 用途 |
| --- | --- |
| `Plugins/NodeForge/Source/` | 与137普通候选对应的83个native源码文件 |
| `Scripts/` | CLI/MCP桥、安装验证、构建与历史验收工具 |
| `Contracts/`、`Catalog/`、`Examples/`、`Tests/` | 协议、目录、JSON样例与测试来源 |
| `Tools/`、`Docs/PluginControl/DocumentationSources/` | 固定依赖/许可、文档渲染与正文来源 |
| `Docs/Versions/Build136/` | R13已验历史AI资料与R19阅读资料，不冒充137 AI验收 |
| `Releases/Build137Repair5/` | 精确包清单、兼容基线及发布说明 |

[仓库与Release方案](Docs/GitHubReleaseDeliveryPlan.md)定义统一交付门槛；[开发说明](Docs/RepositoryGuide.md)说明本机历史脚本、编译和测试前提。验证工程、历史输出、缓存、私人会话与工作管理记录留本机，Git保留测试程序和JSON。

目录仍experimental，包含71材质类、327蓝图签名；不宣称所有参数、组合与平台已验证。实际范围见[发布说明](Releases/Build137Repair5/ReleaseNotes.md)。统一开源许可尚未选择，实际第三方许可保存在插件Licenses/ThirdPartyNotices及工具许可文件。
