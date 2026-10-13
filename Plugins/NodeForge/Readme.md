# NodeForge 插件

NodeForge 是 UE 编辑器插件，用 JSON 描述并生成可编辑的材质和 Actor 蓝图，也支持查询节点、读取资源和生成新副本。

普通用户请下载 [v1.0.0 插件包](https://github.com/XRH66/NodeForge/releases/tag/v1.0.0)，按 [安装指南](../../Docs/Installation.md) 操作。此目录主要保存插件源码；Git 仓库不包含可直接运行的编译 DLL。

## 开始使用

本版支持 Windows 64 位和官方 UE 5.3.2。

1. 把 Release ZIP 中的 `NodeForge` 文件夹复制到引擎的 `Engine/Plugins/Marketplace`；工程级 `Plugins/NodeForge` 保留为高级用法。
2. 在 UE 中启用插件，从 **窗口（Window）→ NodeForge** 打开面板。
3. 首次打开时确认工程和引擎位置。
4. 点击 **载入 → 载入案例**，再点击 **检查** 和 **生成**；也可选择包内 `Examples/FirstMaterial.json`。

详细操作见 [生成第一个材质](../../Docs/GettingStarted.md)。需要 AI 直接操作时，按 [连接 AI](../../Docs/AiInstallation.md) 补装接入包；普通使用无需 Python 或 MCP。

生成的新资产位于 `Content/NodeForgeTests` 下，同名资产不会被覆盖。具体支持的资产、节点和操作见 [用户手册](../../Docs/Versions/v1.0.0/User/Readme.md) 与 [已知限制](../../Docs/Versions/v1.0.0/Version/KnownIssues.md)。

## 源码入口

| 文件 | 负责什么 |
| --- | --- |
| [NodeForgeEditorModule.cpp](Source/NodeForgeEditor/Private/NodeForgeEditorModule.cpp) | 模块与编辑器菜单注册 |
| [NodeForgePanel.cpp](Source/NodeForgeEditor/Private/NodeForgePanel.cpp) | 原生紧凑面板、次级窗口与真实动作接入 |
| [NodeForgePanelPreferences.cpp](Source/NodeForgeEditor/Private/NodeForgePanelPreferences.cpp) | 工程独立设置草稿、应用与持久化 |
| [NodeForgeInstallation.cpp](Source/NodeForgeEditor/Private/NodeForgeInstallation.cpp) | 安装位置、工程和版本检查 |
| [NodeForgeInstallationPanel.cpp](Source/NodeForgeEditor/Private/NodeForgeInstallationPanel.cpp) | 首次安装确认界面 |
| [NodeForgePrototype.cpp](Source/NodeForgeEditor/Private/NodeForgePrototype.cpp) | 请求处理及构图入口 |
| [NodeForgeMaterialGraph.cpp](Source/NodeForgeEditor/Private/NodeForgeMaterialGraph.cpp) | 材质图生成 |
| [NodeForgeBlueprintGraph.cpp](Source/NodeForgeEditor/Private/NodeForgeBlueprintGraph.cpp) | 蓝图生成 |
| [NodeForgeGraphCodec.cpp](Source/NodeForgeEditor/Private/NodeForgeGraphCodec.cpp) | 图数据读取与导出 |
| [NodeForgeGraphDiff.cpp](Source/NodeForgeEditor/Private/NodeForgeGraphDiff.cpp) | 新副本差异比较 |
| [NodeForgeEditorControlService.cpp](Source/NodeForgeEditor/Private/NodeForgeEditorControlService.cpp) | 面板与程序接口共用的编辑器操作 |

## 编译

使用自己的官方 UE 5.3.2 C++ 工程，将此目录放入该工程的 `Plugins/NodeForge`，生成工程文件并构建 Development Editor / Win64 目标。

仓库中的维护脚本可能依赖固定的本机工程和工具链，不能把克隆仓库当成完整 UE 工程直接构建。自编译 DLL 的安装信息必须与实际产物一致，不能复用发行包的文件哈希。具体入口和打包参数见 [源码仓库说明](../../Docs/RepositoryGuide.md)。

## 协议与许可

- [Graph JSON](../../Contracts/GraphProtocol.md)：图的字段、节点、连线和资源约束。
- [编辑器控制协议](../../Contracts/EditorControlProtocol.md)：CLI 与编辑器之间的调用。
- [v1.0.0 节点目录](../../Docs/Versions/v1.0.0/Catalog.json)：本版的精确节点和引脚。
- [第三方许可](ThirdPartyNotices.md)：引用代码的来源与许可说明。
