# 原生条件选择和去色数据

Build45的[实际矩阵](../../Reports/20261008GraphV2/ColorSelection1/Summary.json)已通过19例31步。数据由PrepareFixtures.py在实际目录采集后准备，Class、属性和端口均先核对真实实例，不是效果专用构建器。

- ColorLimits / Changed / Defaults：Max与Min组合、属性变化、未连输入默认，结构不同。
- ColorSelect / Changed / Topology / Shuffled：参数改变、交换冷热分支、节点和边数组倒序；乱序实际布局与语义核对。
- ColorDesaturate / Changed：参数去色和加权配置变化。
- ColorPulse：Time/Sine→Max/Min→If→Desaturation，阈值、颜色与Fraction由参数控制。
- ColorOld44Compat / ColorRotatorCompat：保留Build44真实摘要的旧材质/旋转蓝图。
- 七负例保留原始输入和真实拒绝收据，包含同名资产保护，不删除失败断言。

模板CATALOG_DIGEST_FROM_REAL_ENGINE不能直接UI导入；正式绑定数据在Reports/20261008GraphV2/ColorSelection1/Inputs。新目标与requestId必须唯一。旧兼容与越界例的真实摘要不被运行器替换。

图生成/保存重开通过不表示实际画面、UI或无插件包通过。分层终态统一见[功能说明](../../Docs/MaterialColorSelection.md)与本轮日志；当前Fraction省略明确拒绝。
