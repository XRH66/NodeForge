# 源码地图

下表使用 GitHub 仓库路径。插件 ZIP 的原生源码位于 `NodeForge/Source/NodeForgeEditor/Private/`，安装后位于实际插件位置的 `Source/NodeForgeEditor/Private/`（通常在引擎 `Engine/Plugins/Marketplace/NodeForge`；工程级为高级方式）。AI 接入包中的工具放在 `Scripts/`。

原生文件均在仓库的 [插件 Private 目录](https://github.com/XRH66/NodeForge/tree/main/Plugins/NodeForge/Source/NodeForgeEditor/Private)。下表省略该共同前缀。

| 功能 | 文件或入口 | 职责 |
| --- | --- | --- |
| JSON 图模型与版本 | `NodeForgeGraphDocument`、`NodeForgeGraphCodec` | 统一图模型，v2/v3 解析、序列化与语义比较 |
| 资产字段 | `NodeForgeAssetDefinitions` | 字段、值范围、省略规则和 Schema |
| 节点与函数 | `NodeForgeNodeDefinitions` | 节点发现、准确签名、类型和原生构造检查 |
| 操作定义 | `NodeForgeControlOperations` | 参数 Schema、host、dispatch 与结果定义 |
| 草稿与业务状态 | `NodeForgeEditorControlService`、`NodeForgeEditorBusinessGate` | 草稿修订、快照、串行执行、保留来源编辑、准备副本和生成 |
| 编辑器连接 | `NodeForgeEditorControlChannel`、`NodeForgeEditorControlProtocol` | 会话认证与控制通道 |
| 界面 | `NodeForgeEditorModule.cpp`、`NodeForgePanel.cpp`、`NodeForgePanelSelection.cpp`、`NodeForgePanelState` | 菜单、面板、选择与草稿状态 |
| 材质图 | `NodeForgeMaterialGraph` | 校验、构造、回读与导出 |
| 蓝图 | `NodeForgeBlueprintGraph`、`NodeForgeBlueprintSwitches` | 事件、函数、成员、Switch 与图生成 |
| 函数和实例 | `NodeForgeMaterialFunctions`、`NodeForgeMaterialInstances` | 函数签名、父材质参数与依赖检查 |
| 资源读取 | `NodeForgeResourceSource` | 查询、精确读取、文件保护、来源报告与 SHA 校验 |
| 规则与执行 | `NodeForgeTaskRules`、`NodeForgePrototype`、`NodeForgeCommandlet.cpp` | 规则查询、执行入口和独立进程操作 |
| 报告与布局 | `NodeForgeResultView`、`NodeForgeGraphDiff`、`NodeForgeGraphLayout` | 结果展示、图差异和节点排列 |
| 安装 | `NodeForgeInstallation`、`NodeForgeInstallationPanel` | 工程绑定、版本校验与安装界面 |

Python 入口在仓库的 [Scripts 目录](https://github.com/XRH66/NodeForge/tree/main/Scripts)：

| 文件 | 职责 |
| --- | --- |
| `NodeForgeControl.py` | CLI，调用编辑器通道或 Commandlet |
| `NodeForgeEditorTransport.py`、`RunProcessGuard.py` | 连接认证、工程检查和进程归属 |
| `NodeForgeMcp.py`、`NodeForgeMcpBridge.py` | MCP stdio 服务、工具 adapter、报告与会话管理 |
| `InstallNodeForgeAi.ps1`、`NodeForgeAiInstall.py` | 下载匹配插件、安装 AI 部分、配置客户端与连接检查 |
| `PrepareNodeForgeInstallation.py` | 绑定工程、准备连接、生成启动和停止命令 |
| `NodeForgeAiLaunch.py`、`NodeForgeAiCheck.py` | 客户端启动入口与连接检查 |
| `PackageNodeForgeAi.py` | 使用固定运行环境和依赖打包 AI 接入包 |
| `BuildNodeForgeUsageDocs.py` | 从目录、工具定义和文档源生成配套手册与清单 |

先按 [架构说明](Architecture.md)理解调用关系，再读取需要改动的模块。接口参数以 [API 参考](../Ai/ApiReference.md)和运行中的 `operations.describe` 为准。

界面设置与品牌的具体入口：`NodeForgePanelPreferences.cpp/.h` 管理真实设置，`NodeForgeClientIcons.cpp/.h` 负责图标校验，`NodeForgeProductInfo.h` 集中维护官网/手册/GitHub。`NodeForgeUiIntegrationChecks.cpp` 是开发观察器，点击生产 Slate 控件；不承担产品执行，也不提供第二套生成核心。
