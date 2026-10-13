# 纹理对象、原生函数与UV效果：外部AI规则

对应UE5.3.2、隔离Build38，使用[实际目录](../Catalog/UE532TextureFunctions.json)。根运行插件Build2和冻结RC1没有自动升级，不能把本规则交给旧DLL导入。后续构建是否兼容由其实际目录/版本规则决定。

沿用[Graph v2协议](../Contracts/GraphProtocol.md)。根字段为format、version、requestId、assetType、assetPath、catalogDigest、nodes、edges、settings；format=`NodeForgeGraph`、version=2、材质assetType=`Material`，本目录摘要为`340e22d25dbc9c24271c5e05c89c5043d366f765a5e8b9fc4bc40bd2080389d1`。目标须新的`/Game/NodeForgeTests/`直接子资产，名称和请求ID使用ASCII字母数字；同名拒绝。节点逻辑ID唯一，连接精确引用ID和实际引脚，数组顺序不定义拓扑。不要输出脚本、HLSL或未知节点/属性。

## 新增可组合表达式

类名前缀为`/Script/Engine.MaterialExpression`；以下列真实引脚与可配置属性摘要，完整默认值/枚举/限制以目录为准。输出索引用字符串`"0"`，不是显示名称`None`。

| 类名后缀 | 输入 | 允许属性 |
| --- | --- | --- |
| TextureObject | 无 | Texture、SamplerType |
| TextureObjectParameter | 继承Coordinates/Apply View MipBias实际不消费，禁止连接 | Texture、SamplerType、ParameterName |
| Rotator | Coordinate、Time | CenterX、CenterY、Speed、ConstCoordinate |
| Distance | A、B均必需 | 无 |
| Step | Y、X | ConstX、ConstY |
| SmoothStep | Min、Max、Value | ConstMin、ConstMax、ConstValue |

Texture必须显式指向实际已保存的Texture2D，限`/Engine/`或`/Game/NodeForgeTests/`，不支持虚拟纹理。SamplerType省略时由引擎推导并回读；提供时必须与资源一致。TextureObject输出是纹理对象，接TextureSample的TextureObject或函数的纹理输入，不能直接作为颜色接EmissiveColor。采样器本身也须有显式合法Texture属性。Rotator输入单数`Coordinate`，不能猜成`Coordinates`。

## 复用原生WorldAlignedTexture

必须依据[本机实际函数签名](../Reports/20261009UsableEditor/TextureFunctionExpansion2/WorldAlignedTexture.json)及[函数样例](../Tests/TextureFunctions/WorldTexture.json)设置MaterialFunction引用/文件指纹，不凭网页猜测引脚。

- 资产：`/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedTexture.WorldAlignedTexture`。
- 本机函数资产SHA256：`a0bd3a9d3762e4e7b3e70446f1bbd1cfbd6df37e25ec6e4f63d2cef5521f6c1a`；这不是递归嵌套依赖锁。
- 必接`TextureObject (T2d)`；可选`TextureSize (V3)`、`WorldPosition (V3)`等以该签名为准。XYZ Texture输出是`"2"`。
- 使用目录中的MaterialFunctionCall，通用创建/连接和函数绑定负责构图；没有“世界贴图”“水面”专用JSON开关。

## 可直接阅读的结构样例

- [世界对齐纹理](../Tests/TextureFunctions/WorldTexture.json)：纹理对象和三分量尺度经原生函数生成颜色。
- [可替换纹理参数](../Tests/TextureFunctions/WorldParameter.json)及[材质实例模板](../Tests/TextureFunctions/WorldInstance.json)：实例需绑定实际已保存父材质的SHA，不能直接使用模板占位指纹。
- [旋转纹理](../Tests/TextureFunctions/RotatingTexture.json)：UV→Rotator→采样；Time×SpinSpeed控制时间，Tint控制颜色。
- [柔边圆形遮罩](../Tests/TextureFunctions/RadialMask.json)及[参数变化](../Tests/TextureFunctions/RadialChanged.json)：UV距离经SmoothStep和OneMinus生成遮罩。
- [阶跃条纹](../Tests/TextureFunctions/StepBands.json)：UV分量/Frac/Step控制两色插值。

样例材质设置为Surface/Opaque/Unlit，以EmissiveColor直接观察运算输出；这组样例不宣称透明水面或受光材质已经验证。创建不同效果要组合目录节点、默认值和边，不能让导入器按资产名选择配方。

目录中的discovered/constructible/experimental不是整类所有配置都通过。当前每个样例的构图、编译、保存、独立重开、运行/画面证据由[功能记录](../Docs/MaterialTextureFunctions.md)维护。此规则由实际目录整理，不代表已经取得新一轮外部AI原始输出或完成真实UI导入。
