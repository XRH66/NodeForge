# 向量与自身Actor：给外部AI的规则

目标Windows、官方UE5.3.2。与[GraphAiRules](GraphAiRules.md)的九字段v2结构共同使用；该历史规则的旧摘要不支持这里的扩展，本页与[真实新目录](../Catalog/UE532BlueprintMotion.json)为本切片权威补充。最新独立构建Build17，根安装仍Build2，尚未部署这些能力到用户面板。目录文件保留Build16真实报告，Build17实际导出已逐项确认签名/摘要完全一致，未覆盖旧报告。

目录摘要必须为`a6488c77da856b78bd72ac32d5c480008f34cdb897524c2296289f088893b786`，读取catalog.blueprint.nodes的真实实例签名，不能用文件SHA、fixture标识或显示中文替代引脚身份。发现/可构建不等于编译或运行验证；实测查[当前报告](../Reports/20261009UsableEditor.md)。

输出完整JSON对象：format=NodeForgeGraph、version=2、assetType=Blueprint，根恰有format/version/requestId/assetType/assetPath/catalogDigest/nodes/edges/settings。requestId与/Game/NodeForgeTests/下直接资产名只ASCII字母数字、1–64字符，使用新名，同名拒绝。每个node恰有id/class/properties，可另有defaults/position；每条edge恰有from/output/to/input。节点最多128、边256、声明变量64；UTF-8无BOM、无重复键/注释、最大65536字节。

settings恰为parentClass=/Script/Engine.Actor与variables数组。原bool/float/string继续支持；新增vector对应精确/Script/CoreUObject.Vector。向量default是三个有限JSON数字的数组[x,y,z]，每分量绝对值最多1e12，不能写引擎文本、任意结构体对象、Rotator或数组容器。向量成员采用K2Node_VariableGet/Set，properties.variable为声明名；真实引脚随变量名改变，目录Value只是占位规则。Set输出Output_Get、execute/then保留原名。

K2Node_CallFunction的properties恰为owner/member。本切片新增：

| owner | member | 关键实际引脚 |
| --- | --- | --- |
| /Script/Engine.KismetMathLibrary | MakeVector | X/Y/Z为double输入，ReturnValue为Vector输出 |
| /Script/Engine.KismetMathLibrary | BreakVector | InVec为Vector输入，X/Y/Z为double输出 |
| /Script/Engine.KismetStringLibrary | Conv_VectorToString | InVec为Vector输入，ReturnValue为string输出 |
| /Script/Engine.Actor | K2_GetActorLocation | 原生自身Actor，ReturnValue为Vector输出 |
| /Script/Engine.Actor | K2_SetActorLocation | execute/then，NewLocation向量，bSweep/bTeleport布尔，ReturnValue布尔输出、SweepHitResult结构体输出 |

Actor函数只使用原生隐式self：不要给self默认值或外部目标，不创建任意Actor对象。SweepHitResult未作为可配置默认值开放。不允许任意函数、latent/容器/宏/动态pin；本切片事件仍为唯一Actor.ReceiveBeginPlay。位置单位由UE原生接口定义，常规UE为厘米；此操作作用于新生成蓝图自己的运行实例。

defaults只写目录声明可写、未连接、非隐藏输入；向量pin用[x,y,z]，标量严格匹配原生类型。同一输入不可同时默认和连接，不存在的引脚立即拒绝，不自动连第一个pin、不插转换节点。执行边保证SetActorLocation之后才读取位置/打印；所有节点创建后按逻辑ID连接，nodes可乱序。

可参考[MotionDirect](../Tests/BlueprintMotion/MotionDirect.json)、[MotionMade](../Tests/BlueprintMotion/MotionMade.json)、[MotionVariable](../Tests/BlueprintMotion/MotionVariable.json)、[MotionWrite](../Tests/BlueprintMotion/MotionWrite.json)、[MotionReadback](../Tests/BlueprintMotion/MotionReadback.json)理解不同结构，但模板占位摘要不能直接导入。新构建已绑定摘要的实际输入在Reports/20261008GraphV2/BlueprintMotion2/Inputs；复制时生成新的requestId/assetPath，不覆盖既有目标。这些是项目自写回归，不能称外部AI一次生成成功；原始AI输出另存，人工修改另存。
