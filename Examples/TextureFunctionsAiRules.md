# 纹理对象、UV 与材质函数

本页说明 NodeForge v0.1.0 的 Graph JSON v2 写法。先读取[通用规则](GraphAiRules.md)和[当前节点目录](../Docs/Versions/v0.1.0/Catalog.json)，从实际目录取得 `catalogDigest`、节点类、属性和引脚。

类路径前缀为 `/Script/Engine.MaterialExpression`。

| 类后缀 | 输入 | 常用属性 |
| --- | --- | --- |
| `TextureObject` | 无 | `Texture`、`SamplerType` |
| `TextureObjectParameter` | 无可连接的采样输入 | `Texture`、`SamplerType`、`ParameterName` |
| `Rotator` | `Coordinate`、`Time` | `CenterX`、`CenterY`、`Speed`、`ConstCoordinate` |
| `Distance` | `A`、`B` | 无 |
| `Step` | `Y`、`X` | `ConstX`、`ConstY` |
| `SmoothStep` | `Min`、`Max`、`Value` | `ConstMin`、`ConstMax`、`ConstValue` |

TextureObject 输出的是纹理对象，应接到 TextureSample 的 TextureObject 或函数的纹理输入，不能直接当颜色接入 EmissiveColor。纹理必须是真实存在、允许读取的普通非虚拟 Texture2D。

Rotator 的输入名是单数 `Coordinate`。TextureObjectParameter 不使用采样输入，不能连接 `Coordinates` 或 `Apply View MipBias`。

## 使用 WorldAlignedTexture 等函数

先读取具体 MaterialFunction 的签名和 SHA256，再按[材质函数规则](MaterialFunctionsAiRules.md)创建调用节点。函数路径、输入名和输出索引以实际读取为准，不复制其他机器保存的哈希。

可组合纹理对象、纹理尺寸和世界位置，实现原生函数提供的效果；这仍是一组普通节点与连线。

## 结构示例

- [世界对齐纹理](../Tests/TextureFunctions/WorldTexture.json)
- [可替换纹理参数](../Tests/TextureFunctions/WorldParameter.json)
- [旋转 UV](../Tests/TextureFunctions/RotatingTexture.json)
- [柔边圆形遮罩](../Tests/TextureFunctions/RadialMask.json)
- [阶跃条纹](../Tests/TextureFunctions/StepBands.json)

这些示例可以帮助理解连线关系。实际资源、采样器类型和输出材质模式仍需逐项检查；同一组节点可以因参数和连线不同产生不同效果。

本文链接的测试 JSON 用于阅读结构，可能含目录摘要或资源占位值，不能保证原样导入当前版本。请核对当前规则、填写真实资源，并使用新的请求编号和资产名。第一次使用推荐插件包附带的 `Examples/FirstMaterial.json`，见[入门教程](../Docs/GettingStarted.md)。
