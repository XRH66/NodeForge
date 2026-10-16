# 更新日志

## [1.1.1] - 2026-10-16

### 修复

- 新增 `calls.releaseCompleted` / `nodeforge_calls_releaseCompleted`，可在达到 128 条容量上限时回收已完成调用，并继续批量业务。
- 保留原输入、调用与报告；回收后的原 callId 仍可读取，重复提交继续核对身份和参数，未解决调用阻止回收。
- `editor.getState.callCapacity` 返回实际驻留、可用和可回收数量。
- 修正大型工具契约文件使用声明的文件解析上限；通信帧上限、身份、权限和租约保护保持。

### 使用

每 50 次业务调用主动清理并核对结果后继续。载入和预检分别计数；本版没有加入自动清理。pending/unknown 查询原 callId，不更换编号重发。插件与可选 AI 包必须同版，升级需要保存并正常关闭使用该插件的编辑器。

### 限制

仅 Windows 64 位、官方 UE5.3.2。节点目录仍为实验支持。接入向导草稿、其它平台、新节点及自动回收策略未纳入本版。

[下载 v1.1.1](https://github.com/XRH66/NodeForge/releases/tag/v1.1.1)

## [1.1.0] - 2026-10-16

### 新增

- 支持清漆着色模型、独立清漆粗糙度、各向异性、切线输入和 IOR 折射，贯通生成、读取与导出。
- 新增 `nodeforge_disconnect`，AI 可以正常释放自己的连接，供另一对话接手，无需关闭 UE。

### 修复

- 统一检查与生成对相同输入的判定，正确返回被拒绝的请求。
- 避免复杂材质共享节点在布局时反复递归。
- 修复多个 AI 客户端在线检测占用连接容量、以及代码刷新后的异常处理。
- 改进 Windows 工程进程识别，避免进程查询长时间等待或误判目标工程。
- 修复重新登录 Windows 后旧会话目录权限导致的连接失败。

### 限制

仅支持 Windows 64 位和官方 UE5.3.2。节点目录为实验支持，不支持任意节点或所有组合。Codex 四步引导、其他平台和其他客户端的专有检测不在本版范围。

[下载 v1.1.0](https://github.com/XRH66/NodeForge/releases/tag/v1.1.0)

## [1.0.0] - 2026-10-14

- 原生紧凑面板、停靠/浮动、响应式换行和可调输入。
- 完整接回真实构图、资源、副本、差异和报告操作，恢复空初态。
- 工程独立设置、客户端名称/图标、只读关于和官网手册入口。
- Codex 实际握手与定向断开；Claude/DeepSeek 仅配置与入口框架，检测延期。
- 保留官方 UE5.3.2 Win64、引擎级复制安装、AI 一次接入及原资产保护。

[下载 v1.0.0](https://github.com/XRH66/NodeForge/releases/tag/v1.0.0) · [界面](Docs/Versions/v1.0.0/User/InterfaceGuide.md)

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
