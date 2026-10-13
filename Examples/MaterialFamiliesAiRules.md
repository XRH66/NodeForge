# 扩展材质目录的AI输入规则

2026-10-09，Windows、UE5.3.2。本规则配合[真实目录](../Catalog/UE532MaterialFamilies.json)，由[独立Build4](../Reports/20261009UsableEditor/Build4/BinaryEvidence.json)导出。当前根安装/已打开面板仍为Build2，不能用旧面板导入这些新节点；构建部署与测试状态见[当前实测](../Reports/20261009UsableEditor.md)。发现/可创建不是任意组合全部通过。

将本文、实际目录以及具体需求交给AI。输出一个UTF-8 JSON对象，保留[协议](../Contracts/GraphProtocol.md)的九根字段、格式NodeForgeGraph、version2、节点ID、受控属性、默认值、连接和资产保护规则。当前扩展目录digest为：

```text
9703b92dac063c1a473c335d505d78d917fc97608a78501dc0c1d1f2491264ca
```

目录是报告包装，读取根`catalogDigest`、`catalog.material.nodes`和`catalog.blueprint.nodes`。文件SHA与目录摘要不同。仅使用真实目录支持的节点、端口、属性；不能写脚本、HLSL、猜测资源、使用任意对象属性或复制整图配方。旧24类材质及15蓝图签名规则沿用[原AI规则](GraphAiRules.md)，下列七类为本构建增量，须使用本构建新摘要。

类路径前缀均为`/Script/Engine.MaterialExpression`：

| 类后缀 | properties | 实际输入名 | 输入默认写入 |
| --- | --- | --- | --- |
| TextureSampleParameter2D | Texture、ParameterName必须显式；可选SamplerType、ConstCoordinate | Coordinates、Apply View MipBias | Coordinates对应整数UV通道0–7 |
| Fresnel | Exponent、BaseReflectFraction | ExponentIn、BaseReflectFractionIn、Normal | 前两输入对应同名配置的原生默认字段 |
| Saturate、Floor、Frac | 空对象 | Input，必须连接 | 无 |
| DotProduct、CrossProduct | 空对象 | A、B，必须连接 | 无 |

Fresnel与普通数学输出索引是字符串`"0"`；TextureSampleParameter2D输出0–5分别为RGB、R、G、B、A、RGBA，其真实实例没有TextureObject输入，不要照抄普通TextureSample的完整引脚表。节点参数名必须显式、唯一，遵守FName大小写不敏感限制。

Texture必须引用调用者确认存在的非虚拟UTexture2D精确对象路径，当前只接受`/Engine/`或`/Game/NodeForgeTests/`；不下载、不导入贴图。省略SamplerType由引擎根据贴图推断并回读；显式值必须匹配。Normal采样需要确实为法线的资源与正确采样器，不能凭颜色/名称猜。参数节点即使有引擎默认贴图，输入也不能省略Texture。

旧digest只兼容原24类及未改变蓝图签名，不能搭配新增类；未知digest失败。原始AI响应和人工修订分别保存，不手工替换原输出的摘要再声称AI原样一次成功。

实际不同拓扑示例见[测试清单](../Tests/MaterialFamilies/Cases.json)：TextureParameter/Tint/Shuffle、FractionMath、VectorProducts、FacingColor/Defaults、WaterFacing。这些是输入数据，经同一个导入器处理；没有新增水面专用构建器。具体编译/保存/重开/视觉状态仍以实测报告为准。
