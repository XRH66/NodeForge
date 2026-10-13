# 数据协议区域

## 当前正式契约（2026-10-13）

Build136 Repair6/Mcp9R13：[Graph v2/v3](GraphProtocol.md)、[CLI](ControlProtocol.md)、[Editor18方法](EditorControlProtocol.md)、[MCP20工具/16业务](McpProtocol.md)、[准确同源定义](../Docs/Versions/Build136/Mcp9R13/ToolContract.json)。三族资源query与Texture2D精确inspect沿共享核心/原入口，原来源/prepare/preview及保护保持；有限交付证据见[PC8](../Reports/PluginControlStageTwo/PersonalCandidate1/PC8/FinalReport.md)。根/RC1及旧协议原范围保持；下面版本导航均为历史。

## 历史协议研究与各阶段导航

下文“没有真实样例/协议未定稿/活动编辑器后续”只表示初始化研究或原阶段时点；不能否定上方已交付的正式受控协议，也不能把更广候选Schema当当前可执行格式。


2026-10-10 A增量Build122：活动editor正式13方法，新增规则/目录查询见[EditorControlProtocol](EditorControlProtocol.md)；原Commandlet四项保持。测试根受限子目录沿原v2图协议，有限实际结果见[A最终报告](../Reports/PluginControlStageTwo/RulesCatalog1/P3/FinalReport.md)，根/RC1未升级。下方早期拟稿文字按各自历史时点解释。

2026-10-10实际活动editor有限11方法见[EditorControlProtocol](EditorControlProtocol.md)，原四项Commandlet继续[ControlProtocol](ControlProtocol.md)。固定118交付/原始版本证据见[第二阶段报告](../Reports/PluginControlStageTwo/EditorControl1/S5/FinalReport.md)，完整AC037仍后续。下方首阶段及规划说明保留历史。

2026-10-10正式程序入口：[Control API 1.0本地契约](ControlProtocol.md)，四项Commandlet/CLI业务及环境/参数/结果/状态。图字段继续GraphProtocol，原收据继续ResultContractDraft；实际范围见[程序控制交付](../Reports/20261010PluginControl/Readme.md)，活动编辑器/RPC后续。

2026-10-10 当前导航：各版本目录与专题通过[Catalog 索引](../Catalog/Readme.md)选择；旧[AI 规则](../Examples/GraphAiRules.md)是 Build3 快照。面板草稿来源、预检与生成成功的区别、64KiB 输入 / 1MiB 报告边界见[结果契约夜间增补](ResultContractDraft.md#2026-10-10-夜间审计实际收据与输入边界)。审查证据见[契约报告](../Reports/20261010NightAudit/ContractReview1.md)，本导航不替代运行验收。

当前可用数据驱动协议为 [NodeForgeGraph v2](GraphProtocol.md)，配套[真实目录](../Catalog/UE532Graph.json)和[外部AI规则](../Examples/GraphAiRules.md)。v1固定样例继续按原格式独立兼容；下方未定稿说明属于更广的研究协议，不限制本轮已经授权的自由组合范围。实际验证状态见[升级日志](../WorkLogs/2026-10-08-数据驱动构图升级.md)，不能从文档存在推导验收通过。

本目录管理数据格式、Schema、协议版本兼容说明、错误码和迁移规则。权威需求为 NF016，候选技术选择见 [D006/D010](../Docs/Decisions.md)。协议未定稿、没有可导入的UE真实样例；已收到研究Schema草案，统一由[CoreCreateDraft](CoreCreateDraft.md)链接原文及候选修订。

## 后续应明确的内容

- 文件格式与版本标识、必要字段、类型、编码和约束，分别覆盖蓝图与材质真实语义。
- 引擎版本、启用插件、节点目录与生成能力的兼容关系；未知、不支持及已弃用输入的处理。
- 节点、引脚、连接、默认值与资源依赖的稳定解析标识，不用中文说明代替真实标识。
- 错误码、错误定位、编译诊断、重复请求与部分写入报告的关系，当前候选统一由[ResultContractDraft1](ResultContractDraft.md)维护。
- 协议升级、兼容与迁移规则；重复导入与同名冲突的获准策略。

上述职责由候选[CoreCreateDraft](CoreCreateDraft.md)逐步收敛，尚非已发布输入契约；字段与能力需在最小蓝图与最小材质真实验证后冻结，关联 [Requirements.md](../Docs/Requirements.md)、[Catalog](../Catalog/Readme.md)和[验收](../Docs/Acceptance.md)。

协议定稿后才建立对应材料：用户/AI 示例放 [Examples](../Examples/)，合法、错误和回归输入放 [Tests](../Tests/TestPlan.md)，都标明协议与适用版本并关联报告。研究阶段这些材料尚未生成；当前有独立命名的NodeForgePrototype v1固定输入切片，不能误认完整Schema定稿。

本轮真实输入/结果说明见[原型报告](../Reports/20261008Prototype.md)；[结果契约](ResultContractDraft.md)末尾记录实际已实现范围和仍未冻结部分。
