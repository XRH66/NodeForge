# 当前节点能力与可操作范围（Build136 Repair6）

2026-10-12有限N已通过：[最终报告](../Reports/PluginControlStageTwo/NodeBulk1/N4/FinalReport.md)/[逐项验收](../Reports/PluginControlStageTwo/NodeBulk1/N4/FinalAcceptance.json)。旧新共用NodeForgeNodeDefinitions与原生适配，新增23材质/235普通函数；原139精确身份保持，不逐节点MCP。

当前[Build136/Mcp9R13目录](../Docs/Versions/Build136/Mcp9R13/Catalog.json)为71材质类/327蓝图签名8类，全部新增原生构造/签名资格通过；具体组合的生成/保存/Verify按独立证据。旧目录与NodeInventory保留原版本，不覆盖。

Build134新增ReceiveTick与BeginPlay共存，有限持续行为实证见BT4；当前资源/安装/画面/交付范围见[PC8报告](../Reports/PluginControlStageTwo/PersonalCandidate1/PC8/FinalReport.md)，不重做N整批。

## 数量与支持层级

| 对象 | 当前目录数量 | 准确含义 | 目录状态 |
| --- | --- | --- | --- |
| Material | 71条、71种MaterialExpression类 | 可发现并构造受控节点实例；属性、默认值和引脚仍受适配器限制 | 71条均discovered/constructible=true、supportState=experimental |
| Actor Blueprint | 327条、8种K2节点类 | 条目包含函数、变量类型与动态节点样本签名，不是327种节点类 | 326条均discovered/constructible=true、supportState=experimental |
| MaterialInstanceConstant | 独立资产能力，不计入图节点数 | 新建普通常量实例；依据真实父材质设置全局标量、向量和Texture2D覆盖 | 按实例报告核验，不把参数数量当节点数量 |

Blueprint的327条组成：CallFunction 299、Event 2、ExecutionSequence 7、IfThenElse 1、SwitchInteger 2、SwitchName 2、VariableGet 7、VariableSet 7。299项函数按精确owner/member区分；变量Get/Set覆盖bool/float/string/vector/int/name/rotator；Sequence样本为2–8输出；Switch两类分别记录有/无Default的样本，不代表只能使用样本里两个case。

**目录不维护完整逐节点测试结论。** Material的verified明确为notEstablishedByCatalog；Blueprint要求另查回归证据。不能说“139项全部未验证”或“139项全部通过”。当前资料包保留原MCP交付的[历史证据索引](../Docs/Versions/Build132/Mcp5R8/EvidenceIndex.json)和[节点阅读入口](../Docs/Versions/Build132/Mcp5R8/Readme.md)，按精确身份/组合/原版本/验证层级关联历史报告；不是397项全部参数/组合通过。本批23材质/235普通函数全数原生构造/签名及代表组合证据见N4，目录仍experimental。新增普通节点的MCP同步机制见[扩展规则](../Docs/PluginControl/NodeExtensionWorkflow.md)，原理/用途资料补强已有限完成，见[NF058最终报告](../Reports/PluginControlStageTwo/McpMaintenance1/E4/FinalReport.md)。历史Mcp4R2的139行均关联准确历史记录，Left/Right/ToLower/ToUpper已补Build58原输入/报告SHA；目录仍experimental，不把关联当全配置通过。原[勘误](../Reports/20261011ProjectStateAudit/DocumentationFindings.md)保留审计时点，修正发布依据见[C4复核](../Reports/PluginControlStageTwo/CopyPreview1/C4/FinalReport.md)。

当前原生报告SHA256为`f927284177028bab2fa57677ecb4e614497e0fca1964e688a484c6b60f5767ad`，目录摘要`3908fac046ddb215e53529a94afe53d3f61f86de0731af02e75738de32edc606`，UE5.3.2/NodeForge-UE532-Graph-2。子集携带完整摘要；重新取得实际加载版本目录，不硬编码未来摘要。旧48/91及摘要保留在原版本报告。

## 材质目前能做什么

既有48类表达式包括常量与参数、基本算术、插值和分量组合、UV/Time/Panner/Rotator、纹理采样和纹理对象、空间位置与法线、颜色选择、距离与阈值、受控MaterialFunctionCall。精确类名见NodeInventory及原目录，不能用中文近似名称匹配引脚。

| 目录家族 | 条目数 | 主要范围 |
| --- | --- | --- |
| ExistingMaterialFamilies | 32 | 常量、参数、算术、插值、分量、纹理/UV/时间、原生函数绑定等既有基础 |
| SpatialInputs | 6 | WorldPosition、ObjectPositionWS、CameraPositionWS、CameraVectorWS、PixelNormalWS、VertexNormalWS |
| ColorSelection | 4 | DotProduct、CrossProduct、Desaturation、If |
| DistanceAndThreshold | 3 | Distance、Step、SmoothStep |
| UvTransform | 1 | Rotator |
| TextureObjectComposition | 2 | TextureObject、TextureObjectParameter |
| OrdinarySurfaceMath | 23 | 新增固定端口数学表达式，准确 scalar/bool 属性与原生默认 |

生成支持Surface、Opaque/Translucent、Unlit/DefaultLit等有限设置；活动输出和设置必须匹配。所有声明表达式必须到达活动输出，不静默删除孤立节点；循环、错误类型/引脚或不允许属性拒绝。材质最多256节点/1024边。

MaterialFunctionCall不是“任何函数自动能用”：需要真实已保存函数的动态签名、GUID与直接资产SHA；Build130已有resources.inspect(kind=MaterialFunction)正式读取签名/模板/SHA；源须符合saved/clean与受控加载证明，不能自动保存或改源。TextureObjectParameter某些继承的采样输入不会被原生编译消费，不能仅凭出现引脚就承诺可连。PixelNormalWS的着色阶段/输出限制、世界与切线空间等仍由现有规则和原生编译核实。纹理限允许目录内的普通非虚拟Texture2D；自定义HLSL、任意表达式类、材质层/函数实例等不在当前承诺内。

## 普通蓝图目前能做什么

普通Actor父类、单EventGraph；可创建BeginPlay事件、函数节点、Branch、Sequence、Integer/Name Switch、成员变量Get/Set，并设置允许的真实属性、未连接输入默认值与显式连接。

原64项函数覆盖：PrintString、字符串拼接/转换/查找/截取/大小写；整数与标量计算、布尔逻辑、选择、Abs/Min/Max/Lerp/角度三角函数；Vector/Rotator构造、拆分、向量运算和旋转插值；自身Actor位置/旋转/缩放/方向；受限自身根SceneComponent位置/旋转/缩放读写。精确列表和真实引脚以原目录为准，不能按类似函数名猜参数。

新增235普通函数通过限定Math/String共同筛选，当前CallFunction共299条；名称/真实端口/类型/默认以本版精确目录为准，不能按Pure就开放任意函数。

成员类型只覆盖上表7种，float实际引脚可能是native real/double。连接采用准确名称/方向，K2 Schema必须允许直接MAKE；不插隐式转换，不替换已有边，不支持循环。最多128节点/256边。

SceneComponent self目前必须来自同图Actor.K2_GetRootComponent.ReturnValue的直接连接；不能用任意对象变量、对象默认值、cast/reroute或另一组件代替。数组/Map/Set、任意对象/枚举成员、组件增删、自定义父类、用户函数图、宏、Timeline、委托/接口、任意UFunction调用等不在本阶段承诺内。未来开放这些能力需要专门语义/生命周期适配，不能只把节点加入白名单。

## JSON复杂性与使用判据

图文件总上限65536字节、JSON深度32；这些限制可能先于节点/边数量触发。上限是校验边界，**没有证明128/256节点的大图任意组合均可生成或满足性能要求**。复杂性取决于节点家族、精确类型、执行流、对象上下文、资源依赖、动态引脚与当前支持的图结构。

现有组合可表达参数化材质计算、UV变化、空间/颜色/纹理组合，以及Actor事件驱动的计算、分支、多路选择、变量读写和受限变换。它不能据此承诺完整游戏逻辑、任意循环系统或任意材质效果。规划使用实际业务样例评价能力，不用节点总数替代可用性。

| 层级 | 什么证据可以证明 | 不代表什么 |
| --- | --- | --- |
| discovered / constructible | 原生目录中存在并能构造受控实例、取得签名 | 整张图编译、保存或运行 |
| generated / compiled / saved | 正式生成报告、UE回读与真实保存字节 | 独立重开、运行/画面 |
| verifiedSaved | 同环境、同stage、**新PID**的原Verify输入/收据/字节回读 | 实际行为、材质视觉、打包 |
| runtime / visual / package | 对应真实运行/画面/打包证据 | 其它版本、其它组合也全部通过 |

已有目录Verification文件按[目录入口](Readme.md)追溯，统一关联见上方已交付的EvidenceIndex；A限定证据见[最终报告](../Reports/PluginControlStageTwo/RulesCatalog1/P3/FinalReport.md)。索引只关联身份和版本可核实的报告；未知保持未知，不自动把experimental改成verified。

## 扩展结论

基础已统一：NodeForgeNodeDefinitions精确登记/共同筛选与两族原生适配器/Spawner、GraphCatalog、TaskRules、唯一Operations与共享Service，以及已交付的有限MCP/版本包可复用。普通节点扩展改核心适配/目录/说明/证据，继续通过同一业务工具；原P2已证明两个目录的数据适配，未来新节点仍需真实对应证据。参数化发布器、原理/已有内容与接手资料已按NF058有限交付；新节点仍需同步真实用途、限制、证据与版本资料，不能说现有MCP或历史索引尚未交付。

三类扩展如何同步见[NodeExtensionWorkflow](../Docs/PluginControl/NodeExtensionWorkflow.md)，资料发布规则见[VersionDocumentationPlan](../Docs/PluginControl/VersionDocumentationPlan.md)。MCP/NF058/B均已完成有限范围，旧计划只供追溯。本批新增普通节点及统一迁移已有限完成；未来类别按授权及受影响链扩展，未开放语义保持。
