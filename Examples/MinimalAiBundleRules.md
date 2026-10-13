# 两样例实际规则

2026-10-10版本导航：本页保留`NodeForgePrototype/version:1`的固定样例协议，供旧版本兼容与回归；“当前窄实现”等文字属于2026-10-08时点，不是整个产品永久边界。新自由构图使用[Graph v2协议](../Contracts/GraphProtocol.md)、[实际Graph规则](GraphAiRules.md)及与所加载DLL匹配的真实目录。不要将v1固定拓扑约束套到v2，也不要把新字段添加进旧版本。

2026-10-08，UE5.3.2。本页是当前窄实现的输入规则，不是全节点协议。读取[真实目录](MinimalAiBundle/RealCatalog.json)、[导出来源](MinimalAiBundle/Manifest.json)及两个模板。规则由本机真实实例目录和当前ParseInput复核，不使用研究fixture标识。

每文件为一个JSON对象，仅有九字段：format、version、requestId、assetType、assetPath、catalogDigest、nodes、edges、settings。format=`NodeForgePrototype`、version=1、catalogDigest=`b0e510f2affbf6dd59e60bd60fc42c728cfcbf90ef35cfab82f09f2b2f1817d6`。UTF-8、最多64KiB、最多32层、无重复键/未知字段。requestId为1–64位ASCII字母数字且本工程未用；assetPath为`/Game/NodeForgeTests/`下单一ASCII字母数字新名称，不设子目录、不用下划线；长度1–64。同名内存/磁盘/资产目录目标及用过requestId拒绝。必须一个文件一个资产，不能输出数组或将两图混为一个对象。

蓝图完全沿用[BlueprintTemplate](MinimalAiBundle/BlueprintTemplate.json)：nodes顺序Begin、Print，类型分别为`/Script/BlueprintGraph.K2Node_Event`、`/Script/BlueprintGraph.K2Node_CallFunction`；owner分别`/Script/Engine.Actor`、`/Script/Engine.KismetSystemLibrary`，member分别ReceiveBeginPlay、PrintString。唯一edge为Begin.then→Print.execute。settings必须InString=`NodeForge Blueprint Runtime OK`、bPrintToScreen=false、bPrintToLog=true。只允许改变requestId和assetPath，不支持任意文本、新事件/函数/变量。

材质沿用[MaterialTemplate](MinimalAiBundle/MaterialTemplate.json)：nodes顺序First、Second、Multiply，前两项类型`/Script/Engine.MaterialExpressionConstant`、第三项`/Script/Engine.MaterialExpressionMultiply`；前两value为有限JSON number，范围0..4，禁止字符串/NaN/Infinity。三个edge按顺序为First.output `0`→Multiply.A；Second.output `0`→Multiply.B；Multiply.output `0`→Material.EmissiveColor。`0`是实际无名表达式输出索引，不能猜成RGB。settings严格Surface/Opaque/Unlit。除新requestId/assetPath外可改变两value；本轮外部AI任务指定0.75和0.5，预期线性Emissive=0.375。

id、节点/边顺序及所有其他键/值必须保持模板；不支持动态目录/自由节点布局、贴图/函数/MaterialInstance、旧资产修改。目录constructible不代表支持所有目录引脚，当前只接上述精确边。

导入显示waitingPersistence表示已保存、尚需独立重开。failed/rejected/cleanupRequired不能按成功处理。同一工程重试须先看报告/残留/目标，不能仅换requestId覆盖。输出样例模板目标已存在，禁止直接当新生成输入再次导入。
