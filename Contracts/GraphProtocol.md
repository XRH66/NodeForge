# NodeForgeGraph v2/v3 数据协议

PC-DOC-20261012-01个人候选有限交付Build136 Repair6/桥0.3.6/Mcp9R13：现代v2/v3继续使用同一GraphDocument、codec及生产核心，不新增JSON字段或版本。Texture精确引用仍用原`properties.Texture`及可选原生SamplerType；函数仍用原binding及FunctionAssetSha256。当前候选目录的`resourceReadRevision=1`使资源读取/普通引用共用显式安装绑定的Game/Engine读取子目录；Blueprint源仍限测试根，创建目标仍限原/Game/NodeForgeTests子目录。归档Build135的原Material7/Blueprint12/instance/v3能力及原Engine/测试根读取政策保留，旧目录不会因新版读取配置获得新范围。ordinary引用与严格original保存来源证明分别处理，retain按原已核payload目录消费，不通过结构或路径转换升级旧证明。schema的对象路径模式只说明语法，精确类/非虚拟/保存/干净/真实mount/目录准入由共同原生消费者核验；当前有限交付为Build136 Repair6；下文Build135及更早结论保留原版本归属。

AS-DOC-20261012-01有限交付Build135 Repair1/Mcp8R5：Surface增加Masked/OpacityMask、opacityMaskClipValue（0..1；Masked显式；遗漏0.3333）、ditherOpacityMask/ditheredLodTransition/allowNegativeEmissiveColor（遗漏false，negative true仅Unlit）。inactive遮罩值如实储存，根连线依IsPropertyActive。七Value成员可选attributes整组，出现则instanceEditable/category/tooltip三项必需；category 1..128 Unicode码点、无首尾空白/控制且竖线段非空；tooltip 0..1024码点、无NUL、空串清除Tooltip而保留自动metadata。共同v2/v3保留presence，旧134/133/132目录不获新增语义。[实际分层证据](../Reports/PluginControlStageTwo/AssetSettings1/AS4/FinalReport.md)及当前目录才确定能力；画面/TAA/LOD未重测。

2026-10-12 JR-DOC-20261012-02正式有限交付Build133 Repair2/桥0.3.5：当前源码将现代v2/v3在输入边界解码为同一个GraphDocument，由原Material/Blueprint/MaterialInstance核心构造、回读、保存和Verify。J3真实验收已通过；Build132及旧冻结包仍只支持v2，不能据此接受v3或获得新能力。正式能力以受测当前目录为准。

本期J0→J4必需项及JV01–16已按原证据收口；两族新资产保存和新PID原CLI Verify、双格式完整导出/来源/编辑/预览、实例transient生产回读及当前MCP/schema/资料已验。候选待验措辞仅描述之前时点；当前正式资料为[Build133/Mcp6R4](../Docs/Versions/Build133/Mcp6R4/Readme.md)，原[逐项验收](../Reports/PluginControlStageTwo/JsonArchitecture1/J4/FinalAcceptance.json)绑定准确范围。Build132及旧包的能力不升级，根/RC1保持。

v3根精确七个必需字段：`format="NodeForgeGraph"`、`version=3`、`requestId`、`catalogDigest`、`target`、`asset`、`graphs`。`target`精确`{assetPath}`，仍create-only；原命名/占用/收据和保存保护保持。`asset`按类型闭合：

- Material：`{type:"Material",settings:{domain,blend,shading,twoSided,lighting?,opacityMaskClipValue?,ditherOpacityMask?,ditheredLodTransition?,allowNegativeEmissiveColor?}}`。Surface；当前135允许Opaque/Masked/Translucent，旧档案按原范围；Unlit/DefaultLit；twoSided必需bool。lighting仅SurfaceForwardShading，Translucent必需；新增设置条件/默认/presence见本页顶部及当前同源schema。
- Blueprint：`{type:"Blueprint",parent:{class:"/Script/Engine.Actor"},members:{variables:[{name,type:{kind:"Value",name},default,attributes?}]}}`。0–64成员，七Value bool/int/float/string/name/vector/rotator；int32，有限实数/向量/rotator分量±1e12，string至4096字符，name沿原canonical FName。vector三项；rotator精确pitch/yaw/roll。当前135 attributes条件见顶部；旧档案保持原成员语义；无组件/新父类。
- MaterialInstance：`{type:"MaterialInstance",parent:{path,assetSha256},parameters:{scalar,vector,texture}}`。三个映射必需、总数≤128；scalar/RGBA分量±1e6并按native float回读，vector是四项RGBA。父须实际保存干净普通Surface Material，直接文件SHA及真实参数/sampler继续核验。实例零图，无实例导出或新增MCP。

Material固定一图`{id:"MaterialGraph",kind:"Material",nodes,edges}`，1–256节点/1–1024边；Blueprint固定一个EventGraph（id/kind均EventGraph），1–128节点/0–256边。MI `graphs:[]`。不支持多图、容器或循环；Build134有限BeginPlay/ReceiveTick沿同一事件定义支持，见文末BT02及BT4证据。

节点必需id/class/properties；可选binding/defaults/position。Event、Function绑定精确kind/owner/member，Member绑定kind/name，MaterialFunction绑定kind/path/assetSha256；这些节点properties必须空，普通节点禁止binding。普通属性仍NodeDefinitions+反射/native适配，不设逐节点MCP处理。defaults映射的遗漏与空对象不同，须真实未连接可赋默认输入；材质默认经原DefaultProperty对应单一native属性，同属性在properties/defaults重复拒绝。position两整数，Material±1000000、Blueprint±100000，遗漏保持原布局。edge精确from/output/to/input，端口/动态签名及DAG仍原生核验。

v2九字段与既有档案语义保留：settings→asset，assetPath→target，nodes/edges→固定graphs，绑定由旧properties解入共同描述。v1 NodeForgePrototype仍原固定有限路径，不获得现代字段。v3只接受真正声明v3的本期新目录；旧132摘要即使内容相同也拒v3，不能替换摘要绕准入。

原graphs.export及CLI导出配置新增可选整数formatVersion，限2/3，遗漏默认2；新配方显式3。ReadActual形成描述后同一native重构，再编码所选wire。准备副本/预览/重排保持草稿版本。Difference/1原分组保持；格式变化不会抹去raw input/report/payload SHA或草稿修订。来源先核原报告字节/当前DLL/保存SHA/clean，再用共同描述比较：original保留数组顺序、presence、请求/目标和值；retain允许原有编辑，不升级旧来源证明。

字段/schema事实由NodeForgeAssetDefinitions和native目录生成；native资源/端口/保存资格不能由schema通过替代。准确字段和迁移调用点见[本期Design](../Docs/PluginControl/JsonArchitectureDesign.md)，验收证据见[本期目录](../Reports/PluginControlStageTwo/JsonArchitecture1/)。下方既有v2详细规则及历史交付说明保留其原版本范围；其中旧Opaque/Translucent、成员恰三字段及平铺目标不是当前135能力上限，现代v2新增字段由顶部同源codec/schema定义。

2026-10-10 A增量：Build122保持v2与根字段，目标路径扩为测试根`/Game/NodeForgeTests/`下0/1/2级受限子目录，段长1–16、叶名公开上限64 ASCII字母数字；核实际Content文件总长/UE路径及reparse，旧平铺仍接受。同完整路径/项目级requestId仍拒绝碰撞，不自动换名；已有副本保留父目录。正式目录名NodeFroge的完整路径仍后续。准确限制见[AssetNaming](../Docs/AssetNaming.md)，两个子目录资产与新PIDVerify见[A实施记录](../Reports/PluginControlStageTwo/RulesCatalog1/Readme.md)；旧根安装/RC1不自动获得此增量。

2026-10-10同名使用逻辑增补：协议v2及九根字段不变，普通Import仍严格执行输入名称并拒绝已存在目标/请求。面板新增明确的“准备新副本”，沿用同一解析器，只生成新的`assetPath`/`requestId`并显示变化，不生成或预留资产；再明确生成时重新核验冲突。JSON文件名与UE资产名称分开，原AI文件保留。见[命名规则](../Docs/AssetNaming.md)与[本轮实际验证](../Reports/20261010NamingRepair1/Readme.md)，旧构建不自动获得此入口。

2026-10-10 夜间一致性补充：本文保留各版本快照，当前输入须配实际加载 DLL 导出的目录或 `GraphRules`，夜间状态与证据见[夜间报告](../Reports/20261010NightAudit/Readme.md)。类、属性、资源路径、参数名、引脚及协议字段使用目录中的规范拼写，不能以大小写别名替代；查询文本仍可大小写无关。后续专题扩展覆盖同项旧限制，不表示发现节点的所有组合均已验证。协议仍为 v2，未迁移历史输入或修改其摘要。 当前项目状态见[ProjectStatus](../ProjectStatus.md)与[第二阶段最终报告](../Reports/PluginControlStageTwo/EditorControl1/S5/FinalReport.md)；本页原构建/夜间证据仍限定于各自时点，不因此继承新版全部验收。

输入限额仍是完整无 BOM UTF-8 的 1–65536 字节；外部结果报告的有界读取上限为 1MiB，二者职责不同。面板“载入 JSON 文件”只载入草稿，另点“生成新资产”才执行；生成/检查/预览使用不可变输入快照。`uiInput` 来源、修订与快照摘要属于结果报告，不是本协议可新增的根字段。详[结果契约增补](ResultContractDraft.md#2026-10-10-夜间审计实际收据与输入边界)。

2026-10-09版本导航更正：下文“当前24材质/15蓝图”和Catalog2摘要指早期Build2/3基线，后续增量有各自真实摘要与准入范围，见[目录导航](../Catalog/Readme.md)、[当前状态](../ProjectStatus.md)。冻结RC1与开发58不得混用规则；四新函数的实验状态见[58收尾](../Reports/20261009UsableEditor/PanelRefresh1/Readme.md)。本轮[兼容表/任务规则调整](../Docs/DevelopmentAdjustmentPlan.md)只是提案，**没有修改version2、解析器或既有输入规则**。

2026-10-09数学与插值增量：协议version2不变，Build46真实摘要aec5b1e30e736a0a16b069b7567817d23a8e9ca9a583aa951ee538e6e73c24c7中的requiresMathCatalog函数接入普通CallFunction路径，旧Build44/45及更早摘要不获得这七函数。Abs/FMin/FMax/Lerp/DegSin/DegCos为double，度制三角输入明确为度；RLerp为Rotator A/B、float Alpha、bool bShortestPath。默认、连接、保存和精确引脚规则不变，实际能力/验证范围见[专题](../Docs/BlueprintMathInterpolation.md)，不能以目录代替运行。

P5差异预览不改变本协议version2：编辑后的完整请求仍经过原v2校验，再与同DLL当前实际源图比较；报告NodeForgeGraphDifference v1是只读结果，不是导入请求。生成仍通过原Import单独执行，源资产不修改；接口、来源规则和实测限制见[GraphPreview](../Docs/GraphPreview.md)。

日常阶段增量：独立Build4增加七类材质节点，见[扩展AI规则](../Examples/MaterialFamiliesAiRules.md)及[真实扩展目录](../Catalog/UE532MaterialFamilies.json)；Build6新增MaterialInstance及[对应真实目录](../Catalog/UE532MaterialInstances.json)，实例要求见下文。协议仍version2，目录摘要明确区分能力；归档24/31类摘要只兼容对应Material/Blueprint，不可用于新实例。下文原24类表保留其快照；根安装Build2尚未切换扩展构建，实际部署/实测以[日常报告](../Reports/20261009UsableEditor.md)为准。

更新：2026-10-09，Windows、本机官方 UE 5.3.2。本文是本轮已实现解析器与实际目录的输入约束；验证结果以逐资产报告为准。v2 由输入逐节点、逐属性、逐连接决定图结构，在允许范围内无需匹配完整图模板。

## 版本及事实依据

依据共同入口 [NodeForgePrototype.cpp](../Plugins/NodeForge/Source/NodeForgeEditor/Private/NodeForgePrototype.cpp)、[材质适配](../Plugins/NodeForge/Source/NodeForgeEditor/Private/NodeForgeMaterialGraph.cpp)、[蓝图适配](../Plugins/NodeForge/Source/NodeForgeEditor/Private/NodeForgeBlueprintGraph.cpp)及本机[当前真实目录 UE532Graph](../Catalog/UE532Graph.json)。当前目录逐字节复制自 Build3 导出的 [Catalog2 原始报告](../Reports/20261008GraphUpgrade/Catalog2.json)，engine 为 `5.3.2-29314046+++UE5+Release-5.3`、adapter 为 `NodeForge-UE532-Graph-2`，实际摘要为：

```text
0d786af7d63e1ea209691493e9b336f50e61eaeb45ef34eeec12ff01b22a884f
```

这是具体目录快照摘要，不是通用常数。目录文件保留完整报告包装：摘要位于根 `catalogDigest`，材质节点位于 `catalog.material.nodes`，蓝图签名位于 `catalog.blueprint.nodes`；不是将裸 catalog 内容误当根输入，也不能把文件字节 SHA 当作 catalogDigest。后续能力目录改变时必须重新导出并提供匹配的摘要，不静默把旧输入改为新版本。

2026-10-09 更新原因：Build3 新增材质通用输出可达性校验，Catalog2 的 `catalog.material.reachabilityPolicy` 明确所有表达式都必须可达有效输出，因此摘要与 [Catalog1 历史导出](../Reports/20261008GraphUpgrade/Catalog1.json)不同。两份目录的节点/属性/端口内容未变；旧导出及其测试输入保持原样，本页后续生成规则使用当前摘要。

| 输入 | 处理规则 |
| --- | --- |
| `format:"NodeForgePrototype", version:1` | 保留旧固定图解析、旧目录和旧回归；见[旧规则](../Examples/MinimalAiBundleRules.md)。不自动迁移。 |
| `format:"NodeForgeGraph", version:2` | 本文的节点/属性/默认值/连线构图。使用 GraphCatalog 模式的 v2 目录。 |
| 其它组合 | 明确拒绝，不猜测格式或使用固定样例替代。 |

当前目录记录 24 个材质类、15 个蓝图签名，均有真实实例采集。`discovered` 表示发现，`constructible` 表示能创建采集实例；这两个字段及目录数量均不代表所有组合已编译或运行。蓝图 15 项来自 1 个事件、7 个函数、1 个 Branch 和 3 种变量类型各自的 Get/Set，并非 15 种独立节点类。实测另见[回归规则](../Docs/GraphRegression.md)及具体报告，不在本文宣称正在执行的测试已通过。

## 文件、路径与根对象

一个 UTF-8 文件描述一个新资产。文件 1–65536 字节，不允许 BOM、NUL、重复 JSON 键、注释、尾逗号；最大嵌套 32 层。所有数值使用 JSON number，布尔使用 true/false，不以字符串代替。根对象必须恰有下面九个字段：

| 字段 | 类型与约束 |
| --- | --- |
| `format` | 字符串 `NodeForgeGraph` |
| `version` | 数字 `2` |
| `requestId` | 1–64 位 ASCII 字母或数字；在当前工程未使用的新 ID |
| `assetType` | `Material`、`Blueprint` 或已支持该能力构建的 `MaterialInstance` |
| `assetPath` | `/Game/NodeForgeTests/` 后直接接 1–64 位 ASCII 字母数字新资产名；没有扩展名、对象后缀、子目录、空格或下划线 |
| `catalogDigest` | 当前真实 GraphCatalog 的 SHA-256 摘要字符串 |
| `nodes` | 节点对象数组；数组顺序不决定图形依赖 |
| `edges` | 连接对象数组；所有节点先创建，再解析连接 |
| `settings` | 当前资产种类对应的精确设置对象 |

已存在的磁盘/内存/注册表目标或已使用 requestId 拒绝覆盖。失败可能留下会话候选或请求收据，查看报告后使用新的请求与新目标，不假定重复提交等于幂等覆盖。当前工程守卫仅允许本库独立开发、验证及 RegressionRuns 区域，不适用于正式工程。

## 材质实例设置（Build6增量）

`assetType:"MaterialInstance"`新建普通UE `MaterialInstanceConstant`，复用同一预检、暂存、同名保护、保存及Verify流程。实例不创建新图，`nodes:[]`、`edges:[]`；九根字段仍完全相同。真实实例目录digest为 `1b12e94c89af0c2e18d3dbbf362222754473f341d3a353eff24ca6abc8809dc1`，具体构建与验证状态见当前报告。

`settings`恰有五字段：`parent`精确对象路径、`parentAssetSha256`父资产已保存文件SHA、`scalarParameters`数字映射、`vectorParameters`四数线性RGBA映射、`textureParameters`现有非虚拟普通UTexture2D路径映射。三映射即使无override也写 `{}`，总计最多128项。数值有限±1000000。只接受真实父材质Global参数的大小写规范名称和正确类型，拒绝未知、重名/跨类型、无效资源、字段及过期父SHA。

父须干净、已保存普通Surface `Material`，仅允许 `/Engine/` 或 `/Game/NodeForgeTests/`；本切片不支持实例父级、层参数、静态开关、继承设置改写。实例继承父材质模式/节点/静态shader，省略override原生继承默认值。先用只读 `MaterialParameters` 传入 `{"parent":"实际对象路径"}`，取得真实参数/默认值/父SHA，再让AI生成实例；不能猜参数或填全零SHA。父材质先在本工程存在，实例不是自动生成父图的配方。

两个已归档digest（24类 `0d786a…`、31类 `9703b9…`）只接受对应普通Material/Blueprint子集，原文不改；实例必须匹配包含该能力的当前目录。实例无图，不允许RelayoutCopy。真实override数组、有效值、父字节及shader继承/独立重开分别验证，原生setter bool不代表成功。详情见[实例AI规则](../Examples/MaterialInstancesAiRules.md)。

## 共同节点及连接结构

每个节点恰有必填 `id`、`class`、`properties`，可增加 `defaults`、`position`，不能加其它字段。

- `id` 是本图唯一 1–64 位 ASCII 字母数字逻辑 ID；材质图保留 `Material` 作为资产输出终点，不能作表达式 ID。
- `class` 是目录中大小写精确的完整原生类路径，不接受中文显示名、短名或猜测别名。
- `properties` 必须是对象，即使没有配置也写 `{}`；仅接受该实际节点目录允许的属性或绑定字段。
- `defaults` 是可选对象，键为未连接输入端口的精确名字。只能使用目录明确支持的默认值映射。一个输入不能同时有显式默认值与连接；材质不能同时用 `defaults` 与 `properties` 给同一原生默认字段赋值。
- `position` 可选 `[x,y]`，两个整数。材质绝对值不超过 1000000，蓝图不超过 100000；只决定编辑器布局。2026-10-09布局扩展中，显式位置保留，省略位置按真实图依赖和尺寸估计自动排列，回读记录全部坐标；不能据数组位置猜连线。两显式锚点相互重叠时保留并告警。详见[布局规则与验证范围](../Docs/GraphLayout.md)，新代码通过情况以本轮证据为准，不影响节点签名/原协议版本。

每条边必须恰有四个字符串字段：

```json
{"from":"SourceId","output":"0","to":"TargetId","input":"A"}
```

源和目标 ID 必须存在，端口方向正确。同一目标输入最多一条连接；拒绝自连或环。材质连接资产输出时 `to:"Material"`。蓝图 `output` 使用实际 pin 名，例如 `then`、`ReturnValue`，不能使用材质数字索引规则。执行不会用默认端口替换指定但不存在的端口，也不会插入一套固定拓扑。

## 材质节点、属性及默认值

类路径均为 `/Script/Engine.MaterialExpression` 加下表后缀。下表来自当前真实目录的实际端口/属性；其细节、真实原生默认值、输出掩码以目录为准。`*` 表示必须接边，不能靠未实现的默认值配置替代。

| 后缀 | 可写 properties | 实际输入名 | defaults 可用键 |
| --- | --- | --- | --- |
| Constant | `R` | 无 | 无 |
| Constant2Vector | `R,G` | 无 | 无 |
| Constant3Vector、Constant4Vector | `Constant` | 无 | 无 |
| ScalarParameter | `ParameterName,DefaultValue` | 无 | 无 |
| VectorParameter | `ParameterName,DefaultValue` | 无 | 无 |
| Add、Subtract、Multiply、Divide | `ConstA,ConstB` | `A,B` | `A,B` |
| Power | `ConstExponent` | `Base*`,`Exponent` | `Exponent` |
| Sine、Cosine | `Period` | `Input*` | 无 |
| Abs、OneMinus | 空对象 | `Input*` | 无 |
| Normalize | 空对象 | `VectorInput*` | 无 |
| Clamp | `MinDefault,MaxDefault` | `Input*`,`Min,Max` | `Min,Max` |
| AppendVector | 空对象 | `A*,B*` | 无 |
| ComponentMask | `R,G,B,A`，布尔，至少选一项 | `Input*` | 无 |
| LinearInterpolate | `ConstA,ConstB,ConstAlpha` | `A,B,Alpha` | `A,B,Alpha` |
| TextureCoordinate | `CoordinateIndex,UTiling,VTiling,UnMirrorU,UnMirrorV` | 无 | 无 |
| Time | `bIgnorePause,bOverride_Period,Period` | 无 | 无 |
| Panner | `SpeedX,SpeedY,ConstCoordinate,bFractionalPart` | `Coordinate,Time,Speed` | `Coordinate`，值是原生 UV 通道编号 |
| TextureSample | `Texture,SamplerType,ConstCoordinate` | `Coordinates,TextureObject,Apply View MipBias` | `Coordinates`，值是原生 UV 通道编号 |

规则：

- 普通数值/颜色分量有限且在 ±1000000 内；`CoordinateIndex`、`ConstCoordinate` 是 0–7 整数。显式 `Period` 必须大于 0；Time 省略 Period 时保留引擎原生默认，不要把目录的 0 默认值机械复制成显式配置。
- `Constant3Vector.Constant`、`Constant4Vector.Constant`、`VectorParameter.DefaultValue` 必须是 **四数线性 RGBA 数组**；不是对象或三数 RGB 数组。ScalarParameter 的 DefaultValue 是数字。
- 每个参数必须显式给 `ParameterName`，为 1–64 位 ASCII 字母数字；同材质内不能重名（引擎 FName 不区分大小写）。现版不支持用同名参数隐式共享。
- 省略允许的普通属性保留该类真实原生默认；未知属性直接失败。`defaults:{"B":0.4}` 只适用于目录有 `defaultProperty` 的对应输入，例如 Multiply.B→ConstB。
- Panner 省略 Time/Speed 边时使用其原生行为及属性；目录存在输入不等于有对应 `defaults` 写入支持。
- TextureSample 必须用 `Texture` 引用现有非虚拟 UTexture2D 的精确对象路径，仅 `/Engine/` 或 `/Game/NodeForgeTests/`。不会导入/下载资源。省略 SamplerType 时用引擎推断并回读；显式枚举字符串须与真实贴图匹配。不要生成不存在的贴图或 TextureObject 节点：该家族尚不在白名单。

材质 `output` 使用规范十进制索引字符串，例如 `"0"`，不能写数字 0、`"RGB"`、`"00"`。实际常用输出：普通数学/Time/UV/ScalarParameter 只有 0；Constant2 的 0/1/2 为 RG/R/G；Constant3 的 0/1/2/3 为 RGB/R/G/B；Constant4 的 0/1/2/3/4 为 RGBA/R/G/B/A；VectorParameter 的 0/1/2/3/4 为 RGB/R/G/B/A；TextureSample 的 0/1/2/3/4/5 为 RGB/R/G/B/A/RGBA。通道含义来自真实输出掩码，不按显示名猜测。

## 材质设置与输出

`settings` 必填 `domain,blend,shading,twoSided`，仅可附加 `lighting`：

```json
{"domain":"Surface","blend":"Translucent","shading":"DefaultLit","twoSided":true,"lighting":"SurfaceForwardShading"}
```

domain 仅 Surface；blend 为 Opaque 或 Translucent；shading 为 Unlit 或 DefaultLit；twoSided 为布尔。Translucent 必须显式 `lighting:"SurfaceForwardShading"`，Opaque 可省略；其它枚举不支持。允许的资产输出是 `BaseColor,EmissiveColor,Metallic,Normal,Opacity,Roughness,Specular,WorldPositionOffset`，但必须在当前材质模式下原生 `IsPropertyActive` 为真。例如 Unlit 使用 EmissiveColor，Opaque 不连接 Opacity。

节点 1–256、边 1–1024，至少一条有效资产输出边。**每个声明的材质节点都必须沿输出连接最终到达至少一个有效 Material 输出**；孤立节点、整条未接资产输出的支链均拒绝，不静默删除或忽略。原因是原生着色器编译可能裁剪没有输出作用的表达式，不能用主输出编译成功证明孤立非法支链也正确。此限制是通用可达性校验，不限定完整拓扑或某种材质效果。

数值维度、采样器及表达式编译仍须实际材质编译判定；目录预检不能保证任意数学表达式都有有效输出。未开放 Custom HLSL、任意材质函数、Material Attributes、折射输出或未知表达式。水面应是上述节点的普通 JSON 组合，现有样例不依赖外部贴图/材质函数；不支持的效果不能换成灰色图冒充完成。

## 蓝图节点与绑定

类路径均为 `/Script/BlueprintGraph.` 加下列类名：

| 类/绑定 | properties | 常用精确引脚 |
| --- | --- | --- |
| K2Node_Event | `{"owner":"/Script/Engine.Actor","member":"ReceiveBeginPlay"}` | 输出 `then`；最多一个 BeginPlay，不强制存在 |
| K2Node_CallFunction | `{"owner":"精确库路径","member":"精确函数名"}` | 见下表实际函数签名 |
| K2Node_IfThenElse | `{}` | 输入 `execute,Condition`；输出 `then,else` |
| K2Node_VariableGet | `{"variable":"声明变量名"}` | 输出 pin 等于变量名 |
| K2Node_VariableSet | `{"variable":"声明变量名"}` | 输入 `execute` 和变量名；输出 `then,Output_Get` |

| owner 完整路径 | member | 数据输入→输出 |
| --- | --- | --- |
| `/Script/Engine.KismetSystemLibrary` | `PrintString` | 执行输入 `execute`→`then`；常用数据输入 `InString:string,bPrintToScreen:bool,bPrintToLog:bool,Duration:float` |
| `/Script/Engine.KismetStringLibrary` | `Concat_StrStr` | `A:string,B:string`→`ReturnValue:string` |
| `/Script/Engine.KismetStringLibrary` | `Conv_DoubleToString` | `InDouble:double`→`ReturnValue:string` |
| `/Script/Engine.KismetMathLibrary` | `Add_DoubleDouble` | `A:double,B:double`→`ReturnValue:double` |
| `/Script/Engine.KismetMathLibrary` | `Multiply_DoubleDouble` | `A:double,B:double`→`ReturnValue:double` |
| `/Script/Engine.KismetMathLibrary` | `Greater_DoubleDouble` | `A:double,B:double`→`ReturnValue:bool` |
| `/Script/Engine.KismetMathLibrary` | `BooleanAND` | `A:bool,B:bool`→`ReturnValue:bool` |

除 PrintString 外，上述函数为纯数据节点，没有 execute/then。只能使用实际目录允许的节点/绑定，不能只凭发现了某 UFunction 就创建。世界上下文等隐藏引脚由原生节点处理；输入不能配置隐藏引脚。PrintString 的 TextColor/Key 在目录中存在，但当前没有其 struct/name 默认值适配，不能放进 defaults。

蓝图 settings 必须恰有 `parentClass` 和 `variables`：

```json
{"parentClass":"/Script/Engine.Actor","variables":[{"name":"Message","type":"string","default":"Hello"},{"name":"Enabled","type":"bool","default":true}]}
```

variables 必须是数组，可空；每项恰有 name/type/default。变量名 ASCII 字母开头，后续只字母数字，总长 1–64；拒绝大小写折叠重名和与 Actor 继承属性/函数冲突。类型为 bool/float/string；float 对应本机 UE 的 real/double，默认值须为同类型 JSON 值。变量只能引用本图声明成员。

目录的变量示例名 `Value` 是动态 pin 规则：声明变量 Message 时，Get 输出是 `Message`，Set 输入是 `Message`；**不要写 Value**。`execute`、`then`、`Output_Get` 保持原名。变量类型决定原生 pin 类型，不靠引脚显示文字推断。

蓝图节点 1–128、边 0–256、变量 0–64。字符串默认最多 4096 字符；数字默认有限、绝对值最多 1e12。defaults 只能设置未连接、可写、非隐藏的布尔/数字/字符串输入；拒绝对象、结构体、容器和不可写引用默认。K2 Schema 必须返回 MAKE；不能要求隐式类型转换、自动插入转换节点或替换已有连接。连接 double→float 若 Schema 不允许直接 MAKE 也会拒绝，应使用真实兼容引脚。

不支持循环、宏、自定义事件、动态多输入、数组/Map/Set 或已有外部资产成员。创建候选时仅清除候选原生默认事件，再按 JSON 创建事件图；原生 ConstructionScript 和 SceneRoot 是保留且单独回读的脚手架，不计为额外 JSON 节点。

## 回读、失败与版本证据

逻辑 ID 分别保存在材质表达式 Desc 和蓝图节点 NodeComment 的 `NodeForge:` 元数据中；输入不可改写这些保留属性。回读从实际实例取得 ID、类型、属性/绑定、默认值、边和设置，初次、编译后、发布后及独立重开均比较；材质记录表达式数量与输出掩码，蓝图另记脚手架及实际 pin。

结果继续沿用 [ResultContractDraft](ResultContractDraft.md) 的分层 checks。Import 的 `waitingPersistence` 仅表示当前会话已保存，独立进程 Verify 核对输入/目录/执行指纹/文件 SHA/实际结构及重编译后才可 `succeeded`。运行与视觉是额外验证，不随保存/重开自动通过。输入拒绝必须明确错误且不改已有资产；构建/编译/保存失败保留阶段和残留，不能报告整体成功。

## 实际复用入口

材质创建/目录使用 UE 原生实例与反射；连接核心已提取 ChiR24 固定 MIT 文件的输出检查/ConnectExpression 小段。蓝图使用原生 Action Database/Spawner/K2 Schema，并提取 ZiggyMar MIT 的函数过滤、变量配置及必要函数绑定小段。精确协议、候选保护和独立回读是上游实现不满足本项目约束的必要新增控制层。详细文件/函数/保留与修改见[材质复用记录](../Docs/MaterialGraphReuse.md)、[蓝图复用记录](../Docs/BlueprintGraphReuse.md)和[总声明](../Plugins/NodeForge/ThirdPartyNotices.md)。GraphDeck 和来源待明 Epic 材质文件不复制。

外部 AI 的可直接提供规则见 [GraphAiRules](../Examples/GraphAiRules.md)。回归模板中的摘要占位符不能直接导入；应提供当前真实目录或本轮已注入摘要的输入，原始 AI 输出与人工修订分别留存。


## 2026-10-09 原生函数调用与采样修订

实例纹理override现在需要通过每个同名原生采样器VerifySamplerType；无签名、跨颜色/法线类型等在创建前拒绝。Build11实际目录digest d1bcfbf…；旧1b12实例数据须重新取得当前目录摘要，不再直接接受；旧材质/蓝图31类子集保持版本兼容，不静默改写输入。

函数切片目录digest `7d11f08760383d7b4704eab618fb098447f2558d9b456955139fbd818b7851e2`。MaterialFunctionCall节点properties恰为MaterialFunction/FunctionAssetSha256，由原生SetMaterialFunction生成动态输入/输出，连接仍按实际输入名和输出索引。先用只读MaterialFunction操作取真实签名；普通已保存干净函数限定Engine/Test资源。函数调用defaults暂未适配，用显式节点传值；原生可选preview默认值有回读，必需输入未连拒绝。独立回读比较函数引用、GUID、引脚顺序/类型及边，无额外JSON节点插入。

旧0d786目录仅原24类；9703/1b12/d1bc归档材质目录仅原31类，新函数使用其中任一摘要都明确拒绝。d1bc实例已具有相同采样校验，允许其兼容；更旧实例不继承该语义。直接函数SHA不锁定嵌套依赖。[实际目录](../Catalog/UE532MaterialFunctions.json)/[AI规则](../Examples/MaterialFunctionsAiRules.md)/[组合回归](../Tests/MaterialFunctions/Readme.md)分别维护当前能力和验证边界。

## 2026-10-09 自身Actor及向量修订

本切片仍用v2九字段协议，目录digest更新为`a6488c77da856b78bd72ac32d5c480008f34cdb897524c2296289f088893b786`，实际[目录](../Catalog/UE532BlueprintMotion.json)/[AI规则](../Examples/BlueprintMotionAiRules.md)/[19项输入](../Tests/BlueprintMotion/Readme.md)。蓝图新增vector成员和精确Vector pin默认，JSON表示恰为[x,y,z]三个有限数字、分量绝对值至多1e12；不接受任意结构体文本/对象/容器。原标量规则保留，vector是对上文历史标量边界的明确扩展。

新增MakeVector/BreakVector/Conv_VectorToString，以及Actor K2_GetActorLocation/K2_SetActorLocation；实际类型/引脚来自本机实例。两个Actor调用限定自身新蓝图的implicit self，不能绑定外部Actor。引擎SceneRoot/ConstructionScript脚手架规则保持，未新增隐藏JSON节点或固定位置配方。编译/保存/独立重开和实际运行分别报告，位置测试读取真实Actor而非仅打印字符串。

旧0d786/9703/1b12/d1bc/7d11蓝图只兼容原函数与bool/float/string；用旧摘要声称任一新函数、vector变量/默认值均拒绝。7d11 Material/MaterialInstance保持同函数/采样语义，可兼容新目录，不改原始输入；其它材质旧目录子集限制继续按上一节执行。root旧Build2不会因文档更新获得新能力，需加载同版本stage；实际验证见主报告。


## 2026-10-09 普通蓝图函数族增量

协议仍v2九字段，新增族继续使用相同K2Node_CallFunction及精确owner/member，具体引脚必须读取新构建的真实目录；不按效果/样例名称生成。整型int32输入defaults只接受有限、范围[-2147483648,2147483647]内的整数；name输入defaults为短于NAME_SIZE的字符串，界限由目录提取。旋转通过原生MakeRotator/BreakRotator流转，不允许直接rotator数组默认值。成员变量类型仍bool/float/string/vector，当前不支持int/name/rotator成员变量或外部对象/组件目标。

新族需要新目录digest；此前a6488c77…作为BlueprintMotionCatalogDigest保留原Material/Blueprint/MaterialInstance能力，仅旧函数/默认值路径，不能声称获得新增族或int/name默认值。不静默改写原数据。目录支持状态为discovered或experimental，逐函数verified由对应构图/编译/独立重开/运行清单证明，不从目录实例化直接推导。Build36代表性构图/重开/运行已通过，真实699目录及逐签名边界由[函数族说明](../Docs/BlueprintFunctionFamilies.md)维护；此前“尚未编译”是更新前时点。

## 2026-10-09 纹理组合与根组件上下文扩展

Build38实际340e22d2…0389d1目录增加TextureObject/TextureObjectParameter、Rotator、Distance、Step、SmoothStep，继承相同节点/属性/边协议。上文初始目录的“没有TextureObject”只对应当时24类快照；新族具体引脚/资源/适配限制见[纹理AI规则](../Examples/TextureFunctionsAiRules.md)。TextureObjectParameter继承的UV/Mip引脚不会参与原生编译，目录标connectible=false并明确拒绝；纹理对象不得直接作为数值颜色。已完成样例验证及未验证边界见[功能记录](../Docs/MaterialTextureFunctions.md)。

Build39当前7dea96da…09326目录增加Actor.K2_GetRootComponent及受控SceneComponent世界变换调用。组件self使用原生外部成员绑定，只接受同图自身Root getter的ReturnValue直接对象边；对象默认、变量/literal/cast/reroute来源及外部Actor不在范围。这明确扩展前一节“无组件目标”的旧边界，不开放任意对象。组件族需要该能力目录；699和340原数据保留旧能力，不能静默获得组件支持。真实引脚、实例状态与构图/重开/运行结论分开见[组件能力](../Docs/BlueprintComponents.md)。输入结构仍为version2，不新增隐藏节点、脚本或按效果名称执行的字段。

## 2026-10-09 整数与名称成员扩展

Build40实际cd5de7be…0bb9e1目录新增int/name成员声明，沿用v2九字段和VariableGet/Set。它替代前阶段“int/name仅函数引脚”的范围；7dea及更早摘要不允许新成员类型，旧范围保持兼容。int为原生int32范围内整数，编译CDO精确回读；name声明默认值需非空、短于NAME_SIZE且保留原生FName显示拼写，空身份写None。普通name输入引脚沿用各自签名，不把声明规则混为运行时字符串规则。不同类型及默认值不会自动降级。

真实目录、完整限制和同构图器的计数/名称状态/边界样例实测见[成员能力](../Docs/BlueprintMemberScalars.md)和[AI规则](../Examples/BlueprintMemberScalarsAiRules.md)。CDO声明初值与运行实例当前值分别记录；导出不把运行值冒充资产初值。枚举、容器、对象、int64、rotator成员仍待适配。

## 2026-10-09 原生Sequence配置扩展

Build41实际57b27415…478b7d目录新增`/Script/BlueprintGraph.K2Node_ExecutionSequence`；properties恰有整数outputCount，范围2..8。按class及完整properties选目录项，使用该项实际execute和then_0..then_N引脚；不取同类第一份签名代替指定配置。执行引脚不接受defaults，同一执行输出不得扇出。JSON数组顺序不决定执行次序，原生输出顺序决定同步步骤先后；未连输出保留，可嵌套Sequence，仍拒绝循环/latent/宏/委托。

构建使用原生Spawner及AddPin接口，不自造执行节点；回读和导出同时核对原生引脚数量、名称、类型、方向、默认及Pins数组顺序，异常明确失败。旧cd5de7be及更早目录不获得Sequence能力；既有成员/根组件等输入在原兼容边界内保持，不能静默改写摘要。仍为version2九字段。[能力与实测](../Docs/BlueprintSequence.md)、[逐配置状态](../Catalog/UE532BlueprintSequenceVerification.json)、[AI规则](../Examples/BlueprintSequenceAiRules.md)分别给出限制及证据；发现/可构建不自动等于实际运行通过。

## 2026-10-09 材质空间输入及原生枚举

Build42真实588f5726…e4e2801目录增加六类空间输入，仍用同一v2九字段。受控属性现在支持FEnumProperty和原FByteProperty，统一要求精确原生枚举名，拒绝数字、未知名/MAX；不允许任意反射写入。WorldPosition输出索引0/1/2对应实际XYZ/XY/Z掩码，ObjectPositionWS的OriginType为Absolute或CameraRelative。完整签名和空间/着色器阶段限制见[AI规则](../Examples/MaterialSpatialAiRules.md)与[逐类验证](../Catalog/UE532MaterialSpatialVerification.json)。世界方向不会自动变成切线空间Normal。

材质目录范围合并为minimumMaterialRevision配置：原始0、扩展表达式1、函数2、纹理组合3、空间输入4。映射保留每个归档摘要原有权限，Build41摘要已归档且可使用原Sequence/成员/材质能力，但不能生成新空间族；输入不被静默换摘要。这是能力版本校验整理，不改变整体数据格式。NativeCompile失败继续按原契约记录failed阶段、cleanupRequired残留及不保存证据。

独立重开和像素变化不能替代可见性验收：初轮法线位移参数后表面被地面遮住，追加检查失败；修正仅修改JSON和新目标，同一插件验证可见升降及参数颜色通过。历史失败与具体边界见[空间输入报告](../Docs/MaterialSpatialInputs.md)，不将修正测试数据称为外部AI原文。

## 2026-10-09 Rotator成员和原生输入默认

Build43真实目录`272ec558cb541bb1d69b4d998c4aced90cbcbe32ec42f3926a8005618eb52950`继续采用v2结构。成员type为rotator，声明default及未连Rotator输入defaults均用恰含pitch/yaw/roll的对象，三者为有限JSON数字、度数、abs<=1e12；拒绝数组、字符串、布尔值、额外字段及缺轴。引擎CSV由适配层生成，用户无需提交原生结构体字符串。

这替代早期“旋转只能由MakeRotator连线、无旋转成员/对象默认”的能力边界。Build42及更早摘要保留其原能力，新成员/默认须使用新目录；不静默升级摘要。编译CDO、引脚默认、导出与独立重开按三轴分量回读比较，不按四元数等价掩盖默认值改写。运行Actor欧拉角可能等价变化，另作真实变换验证。完整能力和限制见[旋转成员](../Docs/BlueprintRotators.md)。

后续Build44精度更正保持同一目录摘要：Rotator默认值逐分量比较容差为min(1e-3,max(1e-6,abs(target)*1e-15))。它修正大角度下旧相对容差过宽的问题，不把减360视为默认值相等；六种原生CDO/引脚篡改与恢复守卫通过，详细历史与边界见功能说明。该更正没有扩展任意结构体或改变运行Actor的原生欧拉角行为。

## 2026-10-09 材质条件选择和去色

Build45真实96c9459d…fee8c7目录增加minimumMaterialRevision5的Min、Max、If、Desaturation，仍为v2九字段和同一逐项构图机制。Build44摘要272ec558…eb52950归档后保持revision4，只允许原节点；旧图能兼容，新类不能借旧摘要进入。配置、端口和默认值均从实际实例读取，版本不被静默替换。

Min/Max的A/B有原生ConstA/ConstB默认映射；If使用真实A/B/AGreaterThanB/AEqualsB/ALessThanB，未连B可设defaults，不接受端口别名。Desaturation当前要求显式Input/Fraction，与原生允许省略Fraction的能力有明确适配限制；LuminanceFactors为受控FLinearColor，不复制引擎源码。完整规则、实测范围及未测项见[功能](../Docs/MaterialColorSelection.md)与[AI规则](../Examples/MaterialColorSelectionAiRules.md)。

## 2026-10-09 原生多路选择与精确引脚

Build52目录1ae2791d…71317仍采用v2九字段，增加原生SwitchInteger和SwitchName的受控动态配置。整数properties恰为startIndex/caseCount/hasDefault，caseCount整数1..8，startIndex+caseCount≤2147483647；名称properties恰为cases/hasDefault，名称ASCII字母数字、字母起头、≤64字符、1..8个，以FName排重及拒绝保留身份。Default仅由hasDefault决定，执行出口按原生顺序完整保留。

引脚标识区分大小写，Selection中name运行值仍按UE原生FName语义比较。隐藏NotEqual_*上下文由原生创建且禁止输入配置/连接。匹配未连case不走Default；无匹配且无Default结束该执行链。不补默认节点/拓扑。构造和回读均用原生实例，详[能力](../Docs/BlueprintSwitches.md)。

Build46精确摘要aec5b1e3…6c24c7归档为此前数学/插值范围，不能借该摘要生成Switch；45及更早边界保持。共享ExactPin修正为显式CaseSensitive，不用不存在的指定引脚的其它拼写代替；这是输入标识完整性修正。普通string/name默认值及其它绑定比较的历史宽松点仍待定向审计，不把该局部修正宣传为全协议逐字保护已经通过。

## 2026-10-09 蓝图文字与身份精确比较

后续Build55保持v2结构/原生节点签名，以af8a9537…bc15aa真实目录公布精确规则。class/parentClass/owner/member/type/成员引用/逻辑ID和引脚逐字匹配；string默认、name显示文本、变量FriendlyName/metadata值及未表达引脚默认的导出比较显式CaseSensitive。name成员空身份规范为`None`，即使引擎保留none/NONE显示拼写也明确拒绝。原生FName运行比较/排重、bool文本、数字容差和Windows路径归属保持既有语义。

52摘要1ae2791d…71317固定归档；规范旧52蓝图和旧45材质代表保存重开已通过，非法大小写不形成永久兼容义务。该更正替代上段“尚待字符串审计”的历史时点；具体范围与未测部分见[能力](../Docs/BlueprintStringIntegrity.md)和[本轮日志](../WorkLogs/2026-10-09-蓝图字符串完整性修复.md)，不把它扩大为任意嵌套UE字段的完整序列化。


## BT02有限交付：普通Actor事件边界

Build134新增Actor.ReceiveTick，与ReceiveBeginPlay同消费NodeDefinitions精确事件定义，各最多一个且可共存。v3绑定kind=Event、owner=/Script/Engine.Actor、member=ReceiveTick；v2原properties.owner/member由现有codec映射。一个EventGraph、原普通函数与MAKE连线规则保持。Tick实际DeltaSeconds（real/float）端口、与原Multiply_VectorFloat.B（real/double）的MAKE连线无隐藏转换节点、精确编译派生状态及有限X轴持续运行资格已通过BT3原生证据；旧Build133的2/3仍无Tick，Build132保持原2范围。现代版本与资产能力同消费档案，不升级旧输入摘要或来源。最终交付身份与运行范围见BlueprintTick1报告。
