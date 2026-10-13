# 文字、名称与标识符的大小写

本页说明 NodeForge v0.1.0 的 Graph JSON v2 写法。先读取[通用规则](GraphAiRules.md)和[当前节点目录](../Docs/Versions/v0.1.0/Catalog.json)，从实际目录取得 `catalogDigest`、节点类、属性和引脚。

JSON 字段、类路径、函数和引脚名都需要使用准确拼写。例如 `PrintString`、`InString` 不能写成 `printString`、`inString`。

- 节点 `id` 与连线的 `from`、`to` 必须逐字对应，不能创建只有大小写不同的重复节点标识。
- 成员变量 `Message` 必须以 `Message` 引用，不能写成 `message`。
- `string` 默认值保留字面内容，`AbC` 和 `abc` 是不同的字符串。
- `name` 成员的空名称写作 `None`，不是空串、`none` 或 `NONE`。

UE 的 FName 在某些运行时比较中忽略大小写，但这不改变 NodeForge JSON 对字段、引脚和声明引用的精确拼写要求。

变量可选属性使用本版支持的字段，见[资产设置](../Docs/Versions/v0.1.0/Ai/AssetSettings.md)。不要猜测不存在的 FriendlyName 或任意元数据字段。

结构示例见[文字与标识测试数据](../Tests/BlueprintStringIdentity/Readme.md)。

本文链接的测试 JSON 用于阅读结构，可能含目录摘要或资源占位值，不能保证原样导入当前版本。请核对当前规则、填写真实资源，并使用新的请求编号和资产名。第一次使用推荐插件包附带的 `Examples/FirstMaterial.json`，见[入门教程](../Docs/GettingStarted.md)。
