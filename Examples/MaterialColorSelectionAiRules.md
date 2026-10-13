# 材质条件选色与去色

本页说明 NodeForge v0.1.0 的 Graph JSON v2 写法。先读取[通用规则](GraphAiRules.md)和[当前节点目录](../Docs/Versions/v0.1.0/Catalog.json)，从实际目录取得 `catalogDigest`、节点类、属性和引脚。

以下类路径都以 `/Script/Engine.MaterialExpression` 开头。

| 类后缀 | 输入 | 属性 |
| --- | --- | --- |
| `Min`、`Max` | `A`、`B` | `ConstA`、`ConstB` |
| `If` | `A`、`B`、`AGreaterThanB`、`ALessThanB`、`AEqualsB` | `ConstB`、`EqualsThreshold` |
| `Desaturation` | `Input`、`Fraction` | `LuminanceFactors` |

`If.A`、`AGreaterThanB` 和 `ALessThanB` 必须连接，`AEqualsB` 可选；`B` 是否连接或填写默认值按目录规则处理。Desaturation 的 `Input` 和 `Fraction` 在当前适配中都需要连接。

输出使用数字字符串 `"0"`。默认值只用于目录允许的未连接输入，不要同时填写等价的 Const 属性和 defaults。

`LuminanceFactors` 使用四个有限数字的 RGBA 数组。Fraction 通常取 0–1，EqualsThreshold 通常取非负值；插件不会自动把权重归一或把数值夹回建议区间。

所有节点需要连接到当前材质模式允许的输出。想查看纯数学颜色，可以使用 Unlit 材质并连接 EmissiveColor；最终效果仍需在自己的场景中查看。

结构示例见[颜色选择测试数据](../Tests/MaterialColorSelection/Readme.md)。

本文链接的测试 JSON 用于阅读结构，可能含目录摘要或资源占位值，不能保证原样导入当前版本。请核对当前规则、填写真实资源，并使用新的请求编号和资产名。第一次使用推荐插件包附带的 `Examples/FirstMaterial.json`，见[入门教程](../Docs/GettingStarted.md)。
