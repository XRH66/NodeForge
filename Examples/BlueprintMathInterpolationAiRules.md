# 数学与插值函数

本页说明 NodeForge v0.1.0 的 Graph JSON v2 写法。先读取[通用规则](GraphAiRules.md)和[当前节点目录](../Docs/Versions/v0.1.0/Catalog.json)，从实际目录取得 `catalogDigest`、节点类、属性和引脚。

这些函数使用 `/Script/BlueprintGraph.K2Node_CallFunction`，`owner` 为 `/Script/Engine.KismetMathLibrary`。

| member | 输入 | 输出 |
| --- | --- | --- |
| `Abs` | `A` | `ReturnValue` |
| `FMin`、`FMax` | `A`、`B` | `ReturnValue` |
| `Lerp` | `A`、`B`、`Alpha` | `ReturnValue` |
| `DegSin`、`DegCos` | `A`，单位为度 | `ReturnValue` |
| `RLerp` | `A`、`B`、`Alpha`、`bShortestPath` | `ReturnValue`，类型为 rotator |

`RLerp` 的 `A` 和 `B` 默认值是含 `pitch`、`yaw`、`roll` 的对象，单位为度；`Alpha` 的实际类型为 float，`bShortestPath` 为布尔。其他函数的精确类型以本版目录为准。

插值比例通常取 0–1；插件不会替你把超出范围的值夹回这个区间。角度函数 `DegSin` 和 `DegCos` 接受度数，不要与弧度函数混用。

可把插值结果连接到目录允许的 Actor 位置或旋转函数。执行连接与数值连接分别填写，不能把 `ReturnValue` 猜成 `Result`，也不能自动增加隐式类型转换。

结构示例见[数学与插值测试数据](../Tests/BlueprintMathInterpolation/Readme.md)。

本文链接的测试 JSON 用于阅读结构，可能含目录摘要或资源占位值，不能保证原样导入当前版本。请核对当前规则、填写真实资源，并使用新的请求编号和资产名。第一次使用推荐插件包附带的 `Examples/FirstMaterial.json`，见[入门教程](../Docs/GettingStarted.md)。
