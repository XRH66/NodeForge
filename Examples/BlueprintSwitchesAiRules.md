# 外部AI：整数和名称多路分支规则

仅用于实际加载Build52或同签名后续版本的NodeForge，Windows/UE5.3.2。必须先从插件导出规则确认catalogDigest；当前实测摘要为`1ae2791da31e336b3d147d1ed20e173ddfe51e721d255df040c843c743171317`。根安装2和冻结RC1不自动支持本批。输出只包含JSON，不携带代码、脚本或任意类反射指令。

顶层恰为format/version/requestId/assetType/assetPath/catalogDigest/nodes/edges/settings九字段：format=NodeForgeGraph，version=2，assetType=Blueprint；requestId用唯一ASCII字母数字标识；assetPath使用独立验证目录/Game/NodeForgeTests下尚不存在的新资产名称；settings为parentClass=/Script/Engine.Actor及variables数组。节点逻辑ID稳定唯一；nodes数组顺序无执行含义；edges以from/output/to/input逐项明确连接，不用数字索引代替真实蓝图引脚名。

## 新增原生节点

- `/Script/BlueprintGraph.K2Node_SwitchInteger`：properties恰为`{"startIndex":0,"caseCount":3,"hasDefault":true}`。输入execute(exec)、Selection(int)；输出"0"、"1"、"2"与Default。caseCount为整数1..8，startIndex在int32内，且startIndex+caseCount≤2147483647。可以负起点；不能用不连续标签数组。
- `/Script/BlueprintGraph.K2Node_SwitchName`：properties恰为`{"cases":["Idle","Active"],"hasDefault":true}`。输入execute(exec)、Selection(name)；输出Idle、Active与Default。case标识ASCII字母起头，其后字母数字，≤64字符，1..8个，以原生FName忽略大小写的身份排重，不能用None/Default/execute/Selection/NotEqual_NameName/NotEqual_IntInt等保留身份。

hasDefault=false时禁止连接Default。名称引脚按原拼写区分大小写，不能将Active写成active；Selection的name值由UE运行时忽略大小写比较。不连接Selection时可显式defaults：如`{"Selection":1}`或`{"Selection":"Active"}`；连接变量输出后必须删除该defaults字段。禁止设置隐藏比较函数上下文、隐藏引脚或其它未声明属性。

匹配一个未连的case时结束该条执行链，不会自动转Default。只有没有命中任何case时才走存在且已连线的Default。

## 可组合的已有能力与真实样例

BeginPlay：K2Node_Event，properties owner=/Script/Engine.Actor、member=ReceiveBeginPlay，输出then。Sequence：K2Node_ExecutionSequence，properties outputCount=2..8，输入execute，输出then_0..then_N。Actor.K2_SetActorLocation：K2Node_CallFunction，properties owner=/Script/Engine.Actor、member=K2_SetActorLocation；输入execute、NewLocation(vector)、bSweep(bool)、bTeleport(bool)，输出then等真实签名见目录。PrintString：owner=/Script/Engine.KismetSystemLibrary、member=PrintString，常用defaults InString、bPrintToScreen、bPrintToLog；其余端口以当前目录为准。

可声明int/name成员再用K2Node_VariableGet/Set，properties仅variable=成员名；成员实际输出以其声明名命名。类型/default约束沿用[整数/名称规则](BlueprintMemberScalarsAiRules.md)，不随意增加对象类型或转换。

[SwitchName](../Tests/BlueprintSwitches/SwitchName.json)示例由BeginPlay→Sequence分两条：第一条按Active/Idle选择Actor位置，第二条打印独立消息。[SwitchEight](../Tests/BlueprintSwitches/SwitchEight.json)用8个整数出口。[SwitchUnwiredMatch](../Tests/BlueprintSwitches/SwitchUnwiredMatch.json)保留未连的匹配case。都是相同导入器读取节点/属性/连线，没有专用菜单或状态机配方。示例模板摘要占位符须先替换，requestId及新目标必须唯一。

只生成声明支持的图。不存在的类/属性/引脚、不兼容连接、连接同时赋默认、同名资产都应失败；不减少节点或改成固定简单图来掩盖缺口。当前新族真实外部AI→UI闭环未测，不能把这些人工编写测试JSON称为AI一次成功。
