# NodeForge v0.1.0

NodeForge 的首个可用版本。你可以在 UE 中载入 JSON 生成材质和 Actor 蓝图，也可以安装 AI 接入包，让支持 MCP 的客户端查询规则和操作插件。

## 环境要求

- Windows 64 位。
- 官方 Unreal Engine 5.3.2。
- 使用 AI 时，客户端需要支持 MCP；执行安装的 AI 还需要本机文件和程序操作权限。

## 本版功能

- 在 UE 面板中载入或粘贴 JSON，检查节点、参数和连线，生成可编辑资产。
- 查询材质与蓝图节点规则，或导出完整规则供 AI 使用。
- 读取支持的材质参数、材质函数签名和工程资源信息。
- 导出支持的材质或 Actor 蓝图，编辑后查看差异，生成新副本。
- 根据上次成功输入自动重排节点，生成另一份资产。
- 通过 MCP 查询文档和节点、读取资源、准备输入、生成资产及查询结果。

普通插件复制到工程 `Plugins` 后即可启用，首次在面板确认工程位置。AI 接入包自带运行环境，支持新装和在普通插件上补装，也提供客户端配置和连接检查。

## 下载

| 文件 | 用途 |
| --- | --- |
| `NodeForge-0.1.0-UE5.3.2-Windows.zip` | 插件、附带示例和普通安装所需文件 |
| `NodeForge-AI-0.1.0-Windows.zip` | 可选的 AI 连接服务、运行环境、安装工具和文档 |
| `ReleaseManifest.json` | 安装工具使用的版本、环境和文件信息 |
| `Checksums.sha256` | 下载文件的 SHA256 校验值 |

[打开下载页面](https://github.com/XRH66/NodeForge/releases/tag/v0.1.0)。普通用户从插件 ZIP 开始；GitHub 的 `Source code` 压缩包供开发者查看源码。

## 当前限制

本版在 `Content/NodeForgeTests` 中创建新资产，不覆盖已有资产。蓝图限普通 Actor 的单个 EventGraph；不支持循环、数组、多图、额外组件和自定义父类。材质实例没有专用 MCP 操作或整图导出。

节点目录中的条目为实验支持，并非所有节点组合、资源和运行效果都已逐一检查。请在自己的工程中查看生成后的材质效果或蓝图行为。其他 UE 版本、其他平台和定制引擎不包含在此下载包的支持范围内。

[安装插件](../User/PluginInstallation.md) · [生成第一个材质](../User/GettingStarted.md) · [连接 AI](../User/AiInstallation.md) · [完整限制](KnownIssues.md)

[更新日志](https://github.com/XRH66/NodeForge/blob/v0.1.0/CHANGELOG.md)
