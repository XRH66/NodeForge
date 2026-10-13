# 让 AI 编写 NodeForge JSON

将本页、当前插件导出的节点规则和你的需求一起交给 AI。它们说明可以使用哪些节点、怎样填写参数，以及怎样连接节点。

本页主要介绍 Graph JSON v2。v3 结构与完整字段见 [JSON 格式参考](../Docs/Versions/v0.1.0/Reference/GraphAuthoring.md)。首次使用建议先运行插件 ZIP 附带的 `Examples/FirstMaterial.json`，按 [入门教程](../Docs/GettingStarted.md) 完成一次生成。

## 先取得当前规则

在 NodeForge 面板点击 **导出全部节点规则**，再点击 **复制读取内容**。使用 AI 接入时，让 AI 查询当前目录和任务所需节点。

也可以查阅 [v0.1.0 Catalog.json](../Docs/Versions/v0.1.0/Catalog.json)。目录文件外层的 `catalogDigest` 是输入需要使用的摘要；`catalog.material.nodes` 和 `catalog.blueprint.nodes` 保存节点内容。不要用整个文件的 SHA256 替代目录摘要。

本目录部分 JSON 是开发时保存的结构示例，可能带有不同摘要或资源占位值。阅读后应按当前目录重新核对字段、节点与资源，不能只替换摘要就当成当前版本输入。

## 输出格式

一个资产对应一个完整 JSON 对象。不要添加注释、尾逗号、重复键或额外说明字段。文件使用 UTF-8，最多 65,536 字节、深度最多 32 层。

v2 的根字段为：

| 字段 | 内容 |
| --- | --- |
| `format` | `NodeForgeGraph` |
| `version` | `2` |
| `requestId` | 本次生成的新请求编号 |
| `assetType` | `Material`、`Blueprint` 或 `MaterialInstance` |
| `assetPath` | `/Game/NodeForgeTests/` 下尚未使用的目标路径 |
| `catalogDigest` | 当前目录返回的摘要 |
| `nodes` | 节点数组 |
| `edges` | 连线数组 |
| `settings` | 对应资产类型的设置 |

新名称必须符合协议的字符和长度限制。建议通过 **准备新副本** 或 AI 的副本准备工具取得新目标，避免复用已使用的请求编号或资产名。

## 节点与连线

节点包含 `id`、`class`、`properties`，可按规则提供 `defaults` 和 `position`。`id` 在图中唯一，其他节点通过它连接。类路径、属性和引脚均使用目录的准确拼写。

单个材质节点的结构示例：

```json
{"id":"Scale","class":"/Script/Engine.MaterialExpressionMultiply","properties":{},"defaults":{"B":0.4},"position":[-300,0]}
```

这只是节点片段，不能作为完整资产直接生成。

每条边包含 `from`、`output`、`to`、`input`。材质输出使用数字索引字符串，例如 `"0"`；蓝图输出使用真实引脚名，例如 `then` 或 `ReturnValue`。

同一个输入只能有一个数据来源，不能既连接又设置默认值。插件不会自动选第一个引脚、添加类型转换、删除不支持的节点或调整图结构来凑出结果。

`position` 是可选的两个整数坐标。省略时插件安排布局；节点在数组中的先后不表示执行顺序。

## 材质

根据目录填写材质域、混合模式、着色模型及其他允许设置。所有表达式都应连接到当前模式允许的材质输出；孤立节点、环和无效输出会被拒绝。

纹理和材质函数必须引用真实、允许读取的资源。函数引脚需先读取具体函数，不能凭名称猜测。具体写法见 [材质参考](../Docs/Versions/v0.1.0/Reference/Material.md)、[纹理与 UV](TextureFunctionsAiRules.md) 和 [材质函数](MaterialFunctionsAiRules.md)。

## Actor 蓝图

父类为普通 Actor，图为单个 EventGraph。函数节点使用真实的 `owner` 和 `member`，执行顺序由 `execute`、`then`、事件及分支出口的连线决定。

成员变量先在 `settings.variables` 中声明，再通过变量节点读取或写入。变量节点的数据引脚名随实际成员名变化，目录中的样本名不能直接照抄。

支持范围、类型与可选属性见 [蓝图参考](../Docs/Versions/v0.1.0/Reference/Blueprint.md)。数组、循环、多图、任意对象、额外组件和自定义父类目前未支持。

## 给 AI 的任务示例

> 根据我提供的当前 NodeForge 节点规则，为下面的需求编写完整 Graph JSON。先确认所需节点、参数和资源受支持。只使用规则列出的类、函数和引脚，保留真实资源路径，使用新的资产名称和请求编号。如果缺少资源或功能，请指出具体缺项。不要把查询报告或单节点片段当成完整输入。
>
> 我的需求：创建一个带颜色和粗糙度参数的普通材质。

得到 JSON 后，在面板检查输入，再生成资产。保存成功后查看材质效果或运行蓝图；AI 写出 JSON 本身不代表已经生成或验证了资产。

[结构示例目录](GraphV2Samples/Readme.md) · [节点范围](../Catalog/NodeSupportStatus.md) · [连接 AI](../Docs/AiInstallation.md)
