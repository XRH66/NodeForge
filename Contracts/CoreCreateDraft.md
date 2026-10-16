# CoreCreateDraft 设计格式

`CoreCreateDraft1` 是开发时研究过的输入格式，没有作为 NodeForge v1.1.0 的公开输入实现。它使用的 `format=NodeForge`、`schemaVersion`、`capabilityProfile`、`catalogEntryId` 等字段不能直接交给插件。

编写实际请求请使用 [NodeForgeGraph v2/v3](GraphProtocol.md)，并参考[完整示例](../Docs/Versions/v1.1.0/Ai/JsonArchitectureExamples.md)。

这个设计曾讨论将解析、字段校验、引用解析、UE 类型检查、编译、保存和运行检查分开。当前实现也分别报告这些步骤的结果，但其实际字段、容量、状态与错误码应查阅[结果说明](ResultContractDraft.md)和本版本的 Schema。

设计中讨论的 8 MiB 输入、2,000 节点、10,000 连线、任意容器类型、自动迁移和通用幂等恢复并未成为当前支持范围。当前图输入最大为 64 KiB，节点及连线限制按资产类型区分。
