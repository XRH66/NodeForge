# 给外部AI的条件选色与去色规则

仅适用于UE5.3.2、Build45或明确兼容版本；根Build2/冻结RC1不具备此能力。使用[GraphProtocol](../Contracts/GraphProtocol.md)的NodeForgeGraph version2九字段；catalogDigest必须为`96c9459d583fd0c5156842882d5ba68a259db91e5b3db8d495f77be79cfee8c7`。完整签名见[真实目录](../Catalog/UE532MaterialColorSelection.json)，不能用本文件猜测其它节点。

| class完整路径 | 输入名 | 属性 |
| --- | --- | --- |
| /Script/Engine.MaterialExpressionMin | A、B；均允许未连接时defaults中的数字 | ConstA、ConstB |
| /Script/Engine.MaterialExpressionMax | A、B；同上 | ConstA、ConstB |
| /Script/Engine.MaterialExpressionIf | A必连；B可连或default；AGreaterThanB、ALessThanB必连；AEqualsB可选 | ConstB、EqualsThreshold |
| /Script/Engine.MaterialExpressionDesaturation | Input、Fraction在本适配中均必连 | LuminanceFactors，RGBA四有限数字数组 |

输出使用真实数字字符串`"0"`，不是DisplayName None。defaults仅用于目录声明的未连数值输入，不给已连输入同时设default；属性与同一default不能重复赋值。建议Fraction在0..1和EqualsThreshold非负；不要求导入器自动钳位或归一权重。所有节点须到达已启用材质输出，未用分支的数据节点不能孤立放置。

测试设置为Surface/Opaque/Unlit，输出接Material.EmissiveColor。它用于清晰观察数学颜色，不自动替换成光照水面或更改项目设置。已有材质设置的其它合法组合按真实目录使用，不由本文件扩大支持范围。

已绑定完整样例：[条件选色](../Reports/20261008GraphV2/ColorSelection1/Inputs/ColorSelect.json)、[去色](../Reports/20261008GraphV2/ColorSelection1/Inputs/ColorDesaturate.json)、[时间和参数组合](../Reports/20261008GraphV2/ColorSelection1/Inputs/ColorPulse.json)。输入全部节点、属性与边逐项决定实际图；更换合法拓扑无须重编译。已有测试目标存在，再试须更换requestId和assetPath，不覆盖原资产。

这些是项目自写验证数据，不能称为外部AI原始输出；新族真实UI闭环未测。实际分层支持和限制见[功能说明](../Docs/MaterialColorSelection.md)。
