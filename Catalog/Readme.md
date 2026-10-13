# 节点目录区域

## 当前目录入口（2026-10-13）

当前有限候选Build136 Repair6/桥0.3.6/Mcp9R13：[真实目录](../Docs/Versions/Build136/Mcp9R13/Catalog.json)、[节点状态](NodeSupportStatus.md)、[能力总览](../Docs/CurrentCapabilities.md)。71材质类/327蓝图签名/8类，原N新增资格与后续Tick准确范围保留；实验状态不升级为全部组合已验。资源与候选有限结果见[PC8报告](../Reports/PluginControlStageTwo/PersonalCandidate1/PC8/FinalReport.md)，根Build2/RC1不升级。

## 先前目录导航与历史快照


2026-10-10 A交付Build122最新能力入口：[NodeSupportStatus](NodeSupportStatus.md)核实际48材质类、91蓝图签名/8类、可操作范围及目录实验/分层证据区别；来源为[A正式目录原件](../Reports/PluginControlStageTwo/RulesCatalog1/Run2/Calls/CatalogNative.json)。[统计快照](../Reports/20261010NodeExtensionPlanning/NodeInventory.json)不覆盖本目录历史JSON；新增节点的同步/验证机制见[NodeExtensionWorkflow](../Docs/PluginControl/NodeExtensionWorkflow.md)。根/RC1未升级，按实际加载版本选择目录。

2026-10-10当前导航：开发阶段和版本以[ProjectStatus](../ProjectStatus.md)与[最终报告](../Reports/PluginControlStageTwo/EditorControl1/S5/FinalReport.md)为准；有限S阶段已完成，根/RC1未升级。本页目录继续按各自实际DLL/摘要/验证范围选用，不生成或改写目录JSON。下方74、58及更早节点快照只对应历史构建；58四函数后续R1验证见[R1最终报告](../Reports/20261010R1Final/Readme.md)，旧notRun不能覆盖其终态。

## 2026-10-10 夜间开发检查点导航

稳定74的[真实实例目录原件](../Reports/20261009UsableEditor/NightResources74/ActualCatalog.json)、[实际模块证据](../Reports/20261009UsableEditor/NightResources74/ActualCatalogLoadedModule.json)和[规则查询验证](../Reports/20261009UsableEditor/NightTaskRules74/Summary.json)属于该固定stage，摘要为`715528fbc5e20f55115457238f04eee9bc49354e602f9297f90f6b7c8ad1c6f3`。不另复制一套目录、不改历史快照。后续测试补强构建不自动获得74的真实UI验收，具体匹配状态见[夜间报告](../Reports/20261010NightAudit/Readme.md)。RC1继续作为冻结试用基线，根工程仍加载旧DLL；按实际加载版本选目录，下面“最新开发58”只表示当时的导航。

实例签名、可创建和具体图编译/保存重开/运行仍分开；本夜没有增加节点族或扩大所有组合的通过承诺。旧AI目录摘要只按[明确兼容规则](../Contracts/GraphProtocol.md)使用，不替换原AI归档，不把旧摘要兼容解释为新节点已在旧规则中定义。

## 2026-10-09 当前版本导航与更正

本页下方24材质/15蓝图属于早期Build2/3历史快照，不能作为所有版本的当前目录。稳定用户候选按[RC1清单](../Reports/20261009CandidateRC1/Readme.md)选择配套规则；最新开发58真实实例目录为[UE532BlueprintStringTransforms58](UE532BlueprintStringTransforms58.json)，四新函数仍只有签名/预检，实际构图和运行notRun，见[收尾](../Reports/20261009UsableEditor/PanelRefresh1/Readme.md)。必须以实际加载DLL选目录；下方每份文件保留自己的构建时点。本次只更正导航，不修改任何目录JSON或测试状态。

历史目录原始说明：

2026-10-09 当前数据驱动目录：[UE532Graph.json](UE532Graph.json)，是本机UE5.3.2的Catalog2原始导出报告副本；内部catalog含24个材质类和15个蓝图实例签名。目录只证明发现/可构建，按图的编译、保存、重开与运行须另查[升级日志](../WorkLogs/2026-10-08-数据驱动构图升级.md)及实际报告。[v2协议](../Contracts/GraphProtocol.md) / [AI规则](../Examples/GraphAiRules.md)使用同一真实摘要。旧UE532Minimal.json继续服务v1回归，研究fixture不是可导入目录。

本目录负责实际节点目录及支持证据，权威要求见 NF013–NF015。初始化阶段只建立规则；当前已有最小UE实例目录，见本页最新实测增补。

## 目录用途

| 位置 | 职责 |
| --- | --- |
| [Blueprints](Blueprints/) | 实际环境采集的蓝图相关节点、引脚与类型信息；反射函数与特殊节点分别记录采集方法和覆盖边界。 |
| [Materials](Materials/) | 实际材质表达式、输入输出、连接、通道和依赖信息，不能用蓝图语义替代。 |
| [Overrides](Overrides/) | 有依据的中文解释、补充说明或适配覆盖；记录来源、理由与验证，不篡改真实标识或伪造生成支持。 |

## 必须区分的状态

- 引擎中存在：由实际 UE 环境采集或对应真实源码证据确认。
- 转换器可生成：由具体生成能力版本与适配器支持范围确认，不能从“存在”推导。
- 已通过测试：有对应输入、环境、操作与报告，不能从“可生成”推导。

三者分别记录；未知保留未知，不用一个模糊“支持”字段吞并差异。具体字段格式属于之后的协议/目录设计，本表不构成已定稿 Schema。

## 每个条目与批次的记录要求

| 对象 | 必需信息 |
| --- | --- |
| 节点条目 | 节点作用、真实类型与稳定标识、输入输出、类型、默认值、依赖、版本与上述状态；中文名称和解释单独用于阅读。 |
| 采集批次 | UE 精确版本、启用插件范围、采集时间、采集方法/来源与生成能力版本。 |
| 支持或覆盖变更 | 对应适配器/源码固定版本、修改理由、适用环境、输入与测试报告。 |

优先从实际 UE 环境采集，再补说明和示例。默认值或依赖不能采集时记录未知/不适用及原因，不猜测。函数反射不是全部蓝图节点：特殊节点和材质表达式各自研究与适配，不声称完全覆盖。

采集必须由获准任务触发，不在插件空闲时持续轮询或扫描。目录验证见 [T018](../Tests/TestPlan.md)，来源见 [SourceIndex.md](../References/SourceIndex.md)，环境见 [Environment.md](../Docs/Environment.md)。

## 2026-10-08 候选目录设计（尚无采集数据）
研究[04](../References/Reviews/20261008ResearchPack/04-节点目录与检索体系.txt)提出EngineFacts、NodeSignatures、AdapterCapabilities、ProjectOverlay、Knowledge、TaskBundles分层；沿用现有Blueprints/Materials/Overrides按类别存分片和索引，不新增虚拟节点数据库或每节点一文件。

快照候选字段包含catalogId、engineVersion/BuildId、启用插件及版本、projectTypesDigest、adapterVersion、日期、扫描范围和失败/遗漏数。条目保存精确owner/class/member或asset/graph身份、上下文、允许绑定/属性、完整真实pinKey与类型/方向/默认策略、隐藏/动态引脚及来源；中文解释/原tooltip不覆盖机器事实。

availability、construction、verification三维分别表示存在情况、适配能力和测试结果；每个passed必须链接真实输入/环境/报告。反射函数、特殊K2、材质表达式/函数分别处理，RGB/RGBA/A不能近似匹配。签名、插件或适配器变化使相关快照失效；模糊搜索只给候选，不用于导入匹配。

采集先最小双链路白名单：反射线索→受控节点实例化→最终真实签名。实例化可触发插件代码，不是只读预览；只能在获准范围/沙盒工程执行。保留WorldContext/Self等机器信息，不默认实例化任意第三方节点。TaskBundle闭合必要类型/资源/规则依赖，用户预览后本地导出；数目由实测决定，缓存只在任务/手动刷新触发。

以上是研究阶段候选，当时真实UE记录和支持适配器为零；[fixture样例](../References/Reviews/20261008ResearchPack/附录C-协议示例与反例.txt)不进入本目录。下一步见[准备清单](../Docs/ValidationPreparation.md)。

## 2026-10-08 原型实际采集

[UE532Minimal.json](UE532Minimal.json)保留Catalog2真实临时实例输出，包含BeginPlay/PrintString实际Pins及Constant/Multiply输入输出。其目录导出操作的succeeded不是资产生成成功；新版目录模式使用独立NodeForgeActualCatalog1报告类型。

这四类节点在本轮两条固定链路已可构造，并已通过对应结构/编译/保存/重开；蓝图另有PIE。默认值详见生成报告，独立状态/范围见[原型报告](../Reports/20261008Prototype.md)。此最小切片不是完整候选目录Schema，也没有全库扫描。

## 2026-10-09 材质族扩展快照

[UE532MaterialFamilies.json](UE532MaterialFamilies.json)是日常阶段Build4/Catalog4实际导出的精确副本：31材质类/15蓝图签名，digest9703b92dac063c1a473c335d505d78d917fc97608a78501dc0c1d1f2491264ca。新增纹理参数、Fresnel及五类数学节点；端口与默认来自真实原生实例。发现/可创建不代表全部组合编译运行，测试进度见[日常实测](../Reports/20261009UsableEditor.md)。

此构建在独立PluginStage测试，根安装/打开的面板仍Build2，对应[旧v2快照](UE532Graph.json)；没有静默替换旧目录或已获得的外部AI原文。新增节点规则见[扩展AI规则](../Examples/MaterialFamiliesAiRules.md)，选择目录前先确认实际加载DLL。

## 材质实例快照

[UE532MaterialInstances.json](UE532MaterialInstances.json)逐字节复制Build6/Catalog6实际导出，保留31材质类/15蓝图签名，新增materialInstance能力约束；digest1b12e94c89af0c2e18d3dbbf362222754473f341d3a353eff24ca6abc8809dc1。实例具体可覆盖的参数属于实际父资产，必须再用MaterialParameters只读提取名称/类型/默认值和父字节SHA；静态目录不伪造全局参数百科。新目录不能用于根Build2，旧31类目录保留，其摘要只兼容普通Material/Blueprint。实际实例结果见[当前报告](../Reports/20261009UsableEditor.md)，输入规则见[实例AI规则](../Examples/MaterialInstancesAiRules.md)。


函数切片的实际32材质类目录见[UE532MaterialFunctions](UE532MaterialFunctions.json)，动态函数必须先读实际绑定签名，详见[AI规则](../Examples/MaterialFunctionsAiRules.md)。[采样目录](UE532TextureSamplers.json)保留Build11原生采样校验时点，旧目录文件不覆盖。根安装仍Build2，不能认为上述源码目录已经部署到用户面板。

[UE532BlueprintMotion](UE532BlueprintMotion.json)为Build16实际GraphCatalog报告的逐字节副本，保留32材质类、扩展至22个蓝图实例签名（新增五原生函数及vector Get/Set）。这是构建/实例化目录，不代表全部22项运行通过；[AI规则](../Examples/BlueprintMotionAiRules.md)与当前实测记录分别管理输入及证据。所有旧目录快照保留，旧蓝图摘要仅兼容原15签名/标量类型。
