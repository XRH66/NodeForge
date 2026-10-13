# Sequence：给外部 AI 的规则

本页适用于Windows、官方UE5.3.2、Build41隔离插件及[实际目录](../Catalog/UE532BlueprintSequence.json)。摘要 `57b27415fee08ce08df3b734d9ea8fbd7a03678564f218007a0f770c3e478b7d`。真实验证状态与限制见[功能说明](../Docs/BlueprintSequence.md)及[逐配置清单](../Catalog/UE532BlueprintSequenceVerification.json)：2/3/8输出已做代表性构图/重开/实际运行，4..7为实验支持。根工程和RC1插件尚未升级。

继承[整数/名称成员规则](BlueprintMemberScalarsAiRules.md)及[Graph v2协议](../Contracts/GraphProtocol.md)，本页替代旧摘要并增加Sequence节点适配。输出仍为NodeForgeGraph version2九字段JSON，新requestId和`/Game/NodeForgeTests/`下新assetPath；不覆盖，不使用脚本或未知配置。

节点class=`/Script/BlueprintGraph.K2Node_ExecutionSequence`，properties必须且只能包含`outputCount`，为2..8的整数JSON数字。按class与完整properties选择实际目录对应项，不能只取该class的第一份两输出签名。输入为execute，输出按所选项实际列出的then_0、then_1等使用。执行引脚不写defaults；同一执行输出不得同时连接两个目标，分别使用多个Sequence输出。

Sequence按then_0、then_1等次序执行，前一路同步链路返回后进入下一路。未连输出保留并按UE原生语义略过，无隐式补链或并行执行。可以把某一路连接到另一个已支持的Sequence实现嵌套。nodes/edges数组可乱序，不代表执行顺序；真正改变行为要改变执行输出与目标的连接。循环、latent、延迟、宏、委托和未声明动态引脚仍不支持。

可复用样例：[2输出](../Tests/BlueprintSequence/SequenceTwo.json)、[3输出按序累积AB](../Tests/BlueprintSequence/SequenceThree.json)、[交换连线后覆盖为A](../Tests/BlueprintSequence/SequenceSwapped.json)、[8输出](../Tests/BlueprintSequence/SequenceEight.json)、[嵌套后ABC](../Tests/BlueprintSequence/SequenceNested.json)。这些是项目验证数据，不是外部AI原始输出。相同导入器根据数据生成不同结构，不按样例名字选择配方。

旧cd5de7be及更早摘要没有Sequence权限，不能悄悄换成新摘要；旧成员/根组件/材质能力仍按各自摘要保留。找不到所需数量或精确引脚时报告缺口，不能默认接第一个输出。读取/导出实际源图会核对引脚完整性与原生顺序，无法表达的源配置明确失败，不丢弃后报告成功。
