# NodeForge v0.1.0 使用文档

NodeForge 是 Unreal Engine 编辑器插件。你可以用 JSON 描述节点和连线，生成可编辑的材质或 Actor 蓝图，也可以连接 AI 来完成这些操作。

本版适用于 **Windows 64 位和官方 UE 5.3.2**。这是 NodeForge 的第一个可用版本。

## 从这里开始

1. [安装插件](User/PluginInstallation.md)：下载插件，放入工程，在 UE 中启用。
2. [生成第一个材质](User/GettingStarted.md)：使用附带示例，完成一次检查和生成。
3. [连接 AI](User/AiInstallation.md)：可选；安装接入包，让 AI 读取规则并操作插件。

普通插件可以独立使用。AI 接入包已包含所需运行环境，可以在已有插件上补装。

## 按任务查阅

| 我想做什么 | 阅读这份说明 |
| --- | --- |
| 了解插件能做什么 | [功能介绍](User/Overview.md) |
| 查一个按钮的作用 | [界面与按钮](User/InterfaceGuide.md) |
| 载入 JSON、查询节点、读取资源 | [手动操作](User/ManualWorkflows.md) |
| 修改已有图并生成新资产 | [编辑新副本](User/CopyPreview.md) |
| 让 AI 帮我生成或修改节点图 | [使用 AI](User/AiWorkflows.md) |
| 查报错和连接问题 | [常见问题](User/Troubleshooting.md) |
| 查询具体节点及其引脚 | [材质节点](Reference/MaterialNodeIndex.md) · [蓝图节点](Reference/BlueprintNodeIndex.md) |
| 手写 JSON 或开发工具 | [JSON 格式](Reference/GraphAuthoring.md) · [开发指南](Developer/Readme.md) |

本版创建的新资产放在工程的 `Content/NodeForgeTests` 下，不会覆盖同名资产。具体支持范围见 [已知限制](Version/KnownIssues.md)。

[下载 v0.1.0](https://github.com/XRH66/NodeForge/releases/tag/v0.1.0) · [版本说明](Version/ReleaseNotes.md) · [用户手册目录](User/Readme.md)
