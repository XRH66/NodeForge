# 节点目录

NodeForge 使用节点目录说明当前插件接受的节点、参数、引脚和连接规则。编写 JSON 时，应使用与已安装插件相同版本的目录。

## v1.1.0 的入口

- [完整目录 JSON](../Docs/Versions/v1.1.0/Catalog.json)
- [材质节点索引](../Docs/Versions/v1.1.0/Reference/MaterialNodeIndex.md)
- [蓝图节点索引](../Docs/Versions/v1.1.0/Reference/BlueprintNodeIndex.md)
- [节点数量与限制](NodeSupportStatus.md)

也可以在 UE 面板点击 **导出全部节点规则**，或通过 AI 的目录查询工具读取当前运行插件的规则。查询方法见 [查找可用节点](../Docs/Versions/v1.1.0/User/NodeCoverage.md)。

## 怎样读取目录文件

完整目录文件包含外层信息和 `catalog` 内容：

| 字段 | 用途 |
| --- | --- |
| `engine` | 目录对应的 UE 版本 |
| `adapter` | 生成器使用的适配规则标识 |
| `catalogDigest` | 输入 JSON 中引用的目录摘要 |
| `catalog.material.nodes` | 材质节点类、属性、输入和输出 |
| `catalog.blueprint.nodes` | 蓝图节点、函数与具体配置的引脚 |

`catalogDigest` 不是整个 JSON 文件的 SHA256。编写请求时读取它的真实值，不要自行重新计算，也不要把文件哈希填进去。

同一个蓝图节点类可能有多个配置，例如 Sequence 的不同输出数量。选择时需要匹配类与配置，不能只取第一个同名条目。

## 本目录其他 JSON 的用途

`Catalog` 下的其他 `UE532*.json` 是开发时保留的目录样本，用于查看某类节点的结构或配合测试。它们可能只包含部分功能，摘要也可能与 v1.1.0 不同。它们不是供用户选择的其他发行版本。

新任务使用上面的 v1.1.0 目录，或从当前安装的插件重新导出。不要从多个目录样本拼接节点，再只替换一个摘要。

## 状态怎样理解

`discovered` 表示找到了该节点，`constructible` 表示可以创建相应原生节点实例。`experimental` 表示该节点已允许按目录规则使用，但所有参数组合和运行效果尚未逐一验证。

整张图仍需检查、生成和编译。保存成功后，请在自己的工程中检查蓝图行为或材质效果。目录中的条目数量不代表所有组合都已测试。

[JSON 编写规则](../Examples/GraphAiRules.md) · [支持范围](NodeSupportStatus.md)
