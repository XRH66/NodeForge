# 给外部AI的蓝图数学与插值规则

用于UE5.3.2开发Build46及明确兼容后续构建；根Build2、RC1不能使用本增量。输出JSON对象，format=NodeForgeGraph、version=2；全部根字段为format/version/requestId/assetType/assetPath/catalogDigest/nodes/edges/settings。不输出执行代码或额外说明字段。

- assetType为Blueprint；assetPath必须是`/Game/NodeForgeTests/`下直接接1–64位ASCII字母数字的新资产名，不允许子目录、空格、下划线或对象后缀，详[路径契约](../Contracts/GraphProtocol.md)；不覆盖。requestId使用新ASCII字母数字标识。catalogDigest为`aec5b1e30e736a0a16b069b7567817d23a8e9ca9a583aa951ee538e6e73c24c7`。
- settings为`{"parentClass":"/Script/Engine.Actor","variables":[]}`；需要成员时按[协议](../Contracts/GraphProtocol.md)声明类型/default。节点必须有唯一id、class、properties，可有defaults/position。
- 新函数class均为`/Script/BlueprintGraph.K2Node_CallFunction`；properties恰含owner=`/Script/Engine.KismetMathLibrary`与member。member使用Abs、FMin、FMax、Lerp、DegSin、DegCos、RLerp，不能用显示标题。
- Abs/DegSin/DegCos输入A；FMin/FMax输入A、B；Lerp输入A、B、Alpha。上述输入与ReturnValue均double。DegSin/DegCos明确为度，不能当弧度Sin/Cos。
- RLerp输入A/B为`{"pitch":数字,"yaw":数字,"roll":数字}`，角度单位为度；Alpha原生float，bShortestPath为JSON布尔，ReturnValue为Rotator。对象恰含三键，每分量有限、abs≤1e12；测试Alpha建议0..1，插件不隐式夹值。
- 每条edges恰为from/output/to/input，引用节点id及真实引脚；输出用ReturnValue，不猜测Result/Value。连接与defaults不可同时作用于一个输入，不添加隐式转换。
- 其它函数、BeginPlay、Actor位置/旋转及成员从[真实目录](../Catalog/UE532BlueprintMathInterpolation.json)挑选。ReceiveBeginPlay输出then；Actor使用implicit self；K2_SetActorLocation输入NewLocation，K2_SetActorRotation输入NewRotation。
- position可省略，显式值为[x,y]整数；nodes数组顺序无执行语义。不要输出未知属性/引脚、任意对象或未适配的动态配置；已适配Sequence/Switch须遵守各自专题及实际目录，不从本页推断所有混合图已验证。

[样例](../Tests/BlueprintMathInterpolation/Readme.md)的运行器绑定Inputs可用于导入。这份规则与数据由项目编写，不是外部AI一次成功证据；AI原始输出和人工修订须分别保存。
