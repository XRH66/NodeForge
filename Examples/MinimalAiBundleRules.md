# 最小固定格式示例

`MinimalAiBundle` 保存 `NodeForgePrototype` v1 格式的两个固定结构，用于理解和测试最小输入解析。它们属于开发示例，不是供用户选择的另一发行版本。

新任务请使用 [Graph 编写规则](GraphAiRules.md) 或 [v0.1.0 JSON 参考](../Docs/Versions/v0.1.0/Reference/GraphAuthoring.md)。第一次使用推荐插件包附带的 `Examples/FirstMaterial.json`。

## 文件

- [BlueprintTemplate.json](MinimalAiBundle/BlueprintTemplate.json)：BeginPlay 连接 PrintString。
- [MaterialTemplate.json](MinimalAiBundle/MaterialTemplate.json)：两个常量相乘后连接自发光输出。
- [RealCatalog.json](MinimalAiBundle/RealCatalog.json)：这组固定结构使用的目录数据。
- [Manifest.json](MinimalAiBundle/Manifest.json)：示例文件信息。

## v1 的固定要求

根字段为 `format`、`version`、`requestId`、`assetType`、`assetPath`、`catalogDigest`、`nodes`、`edges` 和 `settings`，其中 `format=NodeForgePrototype`、`version=1`。摘要与节点顺序按这组模板本身读取，不能把 v2 或 v3 字段混入。

蓝图模板的节点顺序是 Begin、Print，连接为 `Begin.then → Print.execute`，打印设置固定在模板中；这个格式不能自由添加事件、函数或变量。

材质模板使用 First、Second、Multiply，常量值为 0–4 内的有限数字。两条输入连接到 Multiply 的 A、B，输出 `"0"` 连接 Material.EmissiveColor；设置为 Surface / Opaque / Unlit。

这组模板不能代表当前插件的全部能力，也不能据此推断其他节点可用于 v1。作为解析用例使用时，先确认测试对象支持这个格式；不要只换一个摘要后当成当前 Graph 请求。

## 名称与结果

任何实际生成仍需使用未占用的资产名称和请求编号。同名拒绝时先检查原结果，不删除已有资产绕过冲突。`waitingPersistence` 表示资产已保存，尚未在重启编辑器后检查。
