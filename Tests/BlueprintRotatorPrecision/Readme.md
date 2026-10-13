# 旋转回读精度修正的影响回归

Build44复用[原始输入](../BlueprintRotators/Readme.md)，没有重写预期或删除失败断言。Cases选七个新能力正例，以及错误类型连接和两种旧摘要越界负例；RuntimeCases选六个代表性旋转、写回、拓扑及数组乱序图。完整22例/11图的Build43历史证据保留。

运行器分别为Scripts/RunGraphRegression.py与Scripts/RunBlueprintMotionChecks.py。本目录模板不作为可直接UI导入数据；实际绑定输入保存在Reports/20261008GraphV2/Rotators2/Inputs。

原生NodeForge.RotatorDefaultGuards另验证大角度CDO及未连引脚被减360后拒绝、恢复后通过，不保存故障对象。最新分层终态统一见[功能说明](../../Docs/BlueprintRotators.md)，UI、外部AI和打包不由本子集声明通过。
