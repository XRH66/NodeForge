# 常用材质节点规则

本页说明 NodeForge v0.1.0 的 Graph JSON v2 写法。先读取[通用规则](GraphAiRules.md)和[当前节点目录](../Docs/Versions/v0.1.0/Catalog.json)，从实际目录取得 `catalogDigest`、节点类、属性和引脚。

类路径前缀为 `/Script/Engine.MaterialExpression`。此表列出常用参数节点、数学节点与引脚，完整配置以当前目录为准。

| 类后缀 | 常用属性 | 输入 |
| --- | --- | --- |
| `TextureSampleParameter2D` | `Texture`、`ParameterName`，可选 `SamplerType`、`ConstCoordinate` | `Coordinates`、`Apply View MipBias` |
| `Fresnel` | `Exponent`、`BaseReflectFraction` | `ExponentIn`、`BaseReflectFractionIn`、`Normal` |
| `Saturate`、`Floor`、`Frac` | 无 | `Input` |
| `DotProduct`、`CrossProduct` | 无 | `A`、`B` |

纹理参数的 `Texture` 和 `ParameterName` 需要显式提供。其输出索引 0–5 分别为 RGB、R、G、B、A、RGBA；JSON 中写成字符串。它没有普通 TextureSample 的完整输入集合，不要直接复制另一个节点的引脚。

纹理必须是实际存在、允许读取的非虚拟 Texture2D。通常可省略 SamplerType，让 UE 根据资源推断；显式提供时必须与资源一致。法线采样需要真实的法线纹理和匹配采样器，不能只根据文件名判断。

参数名需要唯一并符合目录命名规则。所有数学节点都应连接到有效输出，不要留下与结果无关的节点。

结构示例见[材质节点测试数据](../Tests/MaterialFamilies/Cases.json)，更多写法见[纹理与 UV](TextureFunctionsAiRules.md)。

本文链接的测试 JSON 用于阅读结构，可能含目录摘要或资源占位值，不能保证原样导入当前版本。请核对当前规则、填写真实资源，并使用新的请求编号和资产名。第一次使用推荐插件包附带的 `Examples/FirstMaterial.json`，见[入门教程](../Docs/GettingStarted.md)。
