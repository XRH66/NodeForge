# 协议参考

这里面向需要编写 JSON、调用 CLI 或接入 MCP 的开发者。普通使用请从[安装](../Docs/Installation.md)和[入门教程](../Docs/GettingStarted.md)开始。

NodeForge v1.1.0 支持 Windows 64 位和官方 UE 5.3.2。产品版本、图形 JSON 格式版本和控制 API 版本分别管理：产品为 v1.1.0，图形格式支持 v2/v3，控制 API 为 1.0。

| 需要了解什么 | 文档 |
| --- | --- |
| 如何描述材质、蓝图和材质实例 | [图形 JSON 协议](GraphProtocol.md) |
| 如何从命令行预检、生成和检查保存文件 | [CLI 协议](ControlProtocol.md) |
| 如何操作正在运行的编辑器 | [编辑器控制协议](EditorControlProtocol.md) |
| 如何从 AI 客户端调用插件 | [MCP 协议](McpProtocol.md) |
| 如何解释状态、错误和部分写入 | [结果说明](ResultContractDraft.md) |

精确字段以本版本的 [GraphRequestSchema](../Docs/Versions/v1.1.0/GraphRequestSchema.json)、[Operations](../Docs/Versions/v1.1.0/Operations.json) 和 [ToolContract](../Releases/1.1.0/ToolContract.json) 为准。节点名称、引脚与可用属性见[参考手册](../Docs/Versions/v1.1.0/Reference/FeatureMap.md)。

[CoreCreateDraft](CoreCreateDraft.md)说明一个未采用的设计格式，不能作为当前插件的输入。
