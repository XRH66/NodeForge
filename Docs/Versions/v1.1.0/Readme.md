# NodeForge v1.1.0

用 JSON 创建可编辑的 UE 材质和 Actor 蓝图，也可以连接 AI 来查询规则、读取资源和生成资产。

**Windows 64 位 · 官方 Unreal Engine 5.3.2**

## 开始使用

1. [下载](User/Download.md)并[安装插件](User/PluginInstallation.md)。普通使用无需 Python。
2. 按[入门教程](User/GettingStarted.md)生成第一个材质。
3. 需要 AI 时，再安装同版[AI 接入包](User/AiInstallation.md)。

## 按任务查阅

- [功能与范围](Reference/FeatureMap.md) · [界面操作](User/InterfaceGuide.md)
- [材质设置与输入](Reference/LayeredMaterials.md) · [JSON 格式](Reference/GraphAuthoring.md)
- [AI 调用流程](Ai/QuickStart.md) · [对话之间交接连接](Reference/SessionHandoff.md)
- [材质节点](Reference/MaterialNodeIndex.md) · [蓝图节点](Reference/BlueprintNodeIndex.md)
- [错误提示与处理](User/Troubleshooting.md) · [已知限制](Version/KnownIssues.md)
- [开发说明](Developer/Readme.md) · [版本变化](Version/ReleaseNotes.md)

插件在 `Content/NodeForgeTests` 中创建新资产，不覆盖同名资产。节点目录仍为实验支持，具体组合应先检查再生成。
