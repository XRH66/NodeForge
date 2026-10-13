# Actor／向量图的独立交付测试

**2026-10-10 夜间阶段历史入口**：本页Build19九图/交付是固定历史证据，当时独立构建与实际范围见[夜间审计记录](../../Reports/20261010NightAudit/Readme.md)。样例与原始失败保留；按[Workflow](../../Docs/Workflow.md#后续能力扩展优先级2026-10-09)选受影响回归，不把旧game/package或三小数位置结论继承给新构建，也不自动重跑旧包。 当前项目状态见[ProjectStatus](../../ProjectStatus.md)与[第二阶段最终报告](../../Reports/PluginControlStageTwo/EditorControl1/S5/FinalReport.md)；本页原构建/夜间证据仍限定于各自时点，不因此继承新版全部验收。

沿用[BlueprintMotion](../BlueprintMotion/Readme.md)的九种输入，只在这些项目自写JSON样例增加原生自身 `GetActorLocation → Conv_VectorToString → PrintString` 读回，并为新请求/资产命名。原样例和历史日志不修改，插件没有新增节点配方或重新编译。来源哈希与差异见[FixtureProvenance](FixtureProvenance.json)。这些不是外部AI原文。

九种输入分别覆盖直接/改变向量、MakeVector、乱序、成员Get、成员Set、BreakVector→MakeVector、原读回、零向量。位置消息来自真实Getter连接，不写常量坐标字符串；执行连接保证Move之后Print。`Cases.json`另保留外部self默认与string→vector错误两项拒绝，用于当前交付阶段。同名保护仍由通用导入器处理。

JSON中目录摘要占位符必须由实际同DLL目录绑定；模板不能直接导入。当前[BlueprintDelivery1](../../Reports/20261008GraphV2/BlueprintDelivery1/Summary.json)是Build19九正例构图/编译/保存/独立重开及两拒绝全通过的证据，保护资产/源码/DLL成立，乱序语义和布局一致；不冒充完整33项或原19项全部回归。

下一使用原生PIE实际位置断言（1e-6）和归属消息核验；全部通过后生成新的显式交付清单，校验图中的Getter读回链、生成收据和副本字节。无插件-game和Development实际包通过准确Actor消息及原生坐标打印观察位置，`FVector.ToString`为三位小数，只能声明该文本精度；不冒充包内1e-6直接读取或资产页人工检查。所有阶段状态以[主报告](../../Reports/20261009UsableEditor.md)为准。

2026-10-09终态补充：上述下一阶段现已在Build19完成，九PIE、18资产副本、九game、Development打包、8430项Pak审计及九实际包运行全passed。详细收据见[交付说明](../../Docs/StandaloneDelivery.md)，保留上段作为此前使用步骤，不重复跑已完成批次。
