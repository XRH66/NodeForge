# 更新日志

记录每个发布版本的用户可见功能和变化。

## [0.2.0] - 2026-10-14

- 同包支持官方 UE 5.3.2 的引擎 Marketplace、直接 Plugins 与工程级安装。
- AI 环境补装到同一位置，客户端一次配置，之后正常打开工程连接。
- 按用户、工程和会话隔离；唯一/明确工程连接，多候选需选择，关闭重开拒绝旧句柄。
- 状态移到工程或用户目录，日常无需写引擎；保留原资产、来源和未知写入保障。

[下载 v0.2.0](https://github.com/XRH66/NodeForge/releases/tag/v0.2.0) · [安装](Docs/Installation.md) · [AI 接入](Docs/AiInstallation.md)

## [0.1.0] - 2026-10-13

NodeForge 的首个可用版本。通过 JSON 创建可编辑的 Unreal Engine 材质和 Actor 蓝图，并提供可选的 AI 接入。

### 功能

- **从 JSON 生成资产**：载入或粘贴节点描述，检查节点、参数和连线，再生成材质或 Actor 蓝图。
- **查询节点规则**：查看支持的节点、属性和引脚，也可以导出规则供 AI 编写 JSON。
- **读取与导出**：查询允许目录中的资源，读取材质参数、材质函数与贴图信息，将支持的材质和蓝图导出为 JSON。
- **创建新副本**：根据已有图准备新名称，预览修改差异，再生成另一份资产。
- **连接 AI**：通过 MCP 读取文档、查询规则、操作草稿、生成资产和查看结果。

### 安装

- 插件 ZIP 复制到工程的 `Plugins` 目录后即可启用，首次在面板确认工程位置。
- AI 接入包包含连接服务、Python 运行环境和安装工具，支持首次安装及补装。
- 安装工具支持 `mcpServers` JSON 和 Codex TOML 配置，并提供连接检查。

### 支持环境与限制

- Windows 64 位、官方 Unreal Engine 5.3.2。
- 在 `Content/NodeForgeTests` 中创建新资产，同名资产不会被覆盖。
- 蓝图限普通 Actor 的单个 EventGraph；其他图类型、自定义父类和新增组件尚不支持。
- 节点目录为实验支持，具体节点参数与限制见[使用手册](Docs/Versions/v0.1.0/User/Readme.md)。

[下载本版][0.1.0] · [安装插件](Docs/Installation.md) · [快速开始](Docs/GettingStarted.md) · [连接 AI](Docs/AiInstallation.md)

[0.1.0]: https://github.com/XRH66/NodeForge/releases/tag/v0.1.0
