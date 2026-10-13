# NodeForge 活动编辑器控制契约 1.0

<a id="个人候选实施中的增量pc13尚未构建交付"></a>

## 个人候选有限交付增量（Build136 Repair6）

唯一Operations表与原GT共享Service新增`resources.query`：参数/默认/边界见[Control契约](ControlProtocol.md#个人候选实施中的增量pc13尚未构建交付)，需新callId和准确expectedDraftRevision，resultKind为resourceQueryReceipt。原Channel路由到ExecuteQueryCall，不另建控制通道；原call查询/完整report_read及busy/修订/unknown规则保持。查询仅更新本次查询报告和提示，不修改草稿、来源或成功基线；complete=false如实提示未完成并建议缩小目录，不把它当完整空结果。

原`resources.inspect`仅在kind枚举中增加`Texture2D`，source仍为准确Package.Asset；复用ResourceSource受控保存文件加载/原SHA/干净状态证明。返回texture的真实类、尺寸、sRGB、compression、非虚拟状态、原生sampler及可直接用于原节点properties的Texture/SamplerType片段。列表查到或普通引用已加载不能升级为original来源证明；预加载且无原保存来源证明仍拒绝，不自动reload/resave。现有函数/材质参数和两族导出来源规则保持。Build135 Repair1历史17方法及原证据保持，当前Build136 Repair6/桥0.3.6/Mcp9R13已集中构建并通过有限正式投影、资源实际查询/检查、生成/保存/新PIDVerify及安装门槛；旧版本原范围保持。

2026-10-12 JR-DOC-20261012-02正式有限交付Build133 Repair2/桥0.3.5：原graphs.export四必需参数assetKind/source/requestId/assetPath及host/dispatch不变，增加可选整数formatVersion=2|3，遗漏2。UI新导出显式3；准备副本、预览和重排保持草稿wire版本。来源report/text/payload原SHA与当前DLL/clean证明先核，再消费共同描述；original保留数组顺序/presence/目标/request，retain继续原已支持编辑。busy/修订/unknown保护保持，不新增操作。J3已通过，Build132未升级。

本期J0→J4必需项及JV01–16已按原证据收口；两族新资产保存和新PID原CLI Verify、双格式完整导出/来源/编辑/预览、实例transient生产回读及当前MCP/schema/资料已验。候选待验措辞仅描述之前时点；当前正式资料为[Build133/Mcp6R4](../Docs/Versions/Build133/Mcp6R4/Readme.md)，原[逐项验收](../Reports/PluginControlStageTwo/JsonArchitecture1/J4/FinalAcceptance.json)绑定准确范围。Build132及旧包的能力不升级，根/RC1保持。

2026-10-12有限C正式交付Build131，DLL SHA `43489121068c00f330598f6636a36e364b6fab1423cdb2edf5527da808413c46`；17 editor方法，新增draft.prepareCopy/graphs.preview与draft.replace显式retain来源编辑。两族正式链、Material实际UI、各一副本保存/新PIDVerify通过；[C最终报告](../Reports/PluginControlStageTwo/CopyPreview1/C4/FinalReport.md)/[逐项](../Reports/PluginControlStageTwo/CopyPreview1/C4/FinalAcceptance.json)。原Build130有限B与118/122历史结论保持，根/RC1未升级。

2026-10-11有限B正式交付为Build130，DLL SHA `91114f1caa8f3edf840188e1718630d1a042d20df90f985a6444c641d36dad88`。resources.inspect/graphs.export已沿共享Service及唯一Operations表接入，正式describe为15方法；读取、两族完整导出、来源正式消费、两件新副本编译保存和新PID原CLI Verify已通过。原生/UI的未变分支按125/128源码连续性复用，不声称130全部重测。见[最终报告](../Reports/PluginControlStageTwo/AssetRead1/B4/FinalReport.md)/[逐项验收](../Reports/PluginControlStageTwo/AssetRead1/B4/FinalAcceptance.json)。Build122/118以下描述和完成范围保留其历史身份；根/RC1未升级。

适用 Windows / 官方 UE5.3.2。A增量候选为[Build122 stage](../Reports/PluginControlStageTwo/RulesCatalog1/Build122/PluginStage/NodeForge/NodeForge.uplugin)，DLL SHA256：`2c456028d1c4cbd186ea4be3382229016a6b94e7f4976c93e8b30e1babc54c3c`，新增rules.query/catalog.get。正式13方法与两个查询到真实资产/新PIDVerify证据见[A实施记录](../Reports/PluginControlStageTwo/RulesCatalog1/Readme.md)。既有S阶段仍冻结于[Build118](../Reports/PluginControlStageTwo/EditorControl1/Build118/PluginStage/NodeForge/NodeForge.uplugin)，原DLL及终态不改写；根工程与RC1未升级。原四项独立Commandlet仍由[ControlProtocol](ControlProtocol.md)管理，图和原生结果沿[GraphProtocol](GraphProtocol.md)、[ResultContractDraft](ResultContractDraft.md)。

本契约原十一方法取自[S5 EditorOperations](../Reports/PluginControlStageTwo/EditorControl1/S5/EditorOperations.json)，A新增以[Build122实际13方法描述](../Reports/PluginControlStageTwo/RulesCatalog1/P3/EditorOperations.json)及[Run2原始捕获](../Reports/PluginControlStageTwo/RulesCatalog1/Run2/DescribeClientResult.json)为准。原始118帮助/投影见[RuntimeExports](../Reports/PluginControlStageTwo/EditorControl1/S5/RuntimeExports.json)。操作定义/校验/describe/分派共同消费[唯一Operations表](../Plugins/NodeForge/Source/NodeForgeEditor/Private/NodeForgeControlOperations.cpp)，本文表不是另一份运行允许清单。implemented与实际验收分开；S阶段边界继续见[S5 FinalReport](../Reports/PluginControlStageTwo/EditorControl1/S5/FinalReport.md)，A按其独立报告与版本说明。

## 连接与启用

宿主为 `activeEditor`，`editorApiVersion="1.0"`、`profile="nodeforge.editor.v1"`，线上 `jsonrpc="2.0"`。插件默认不开放通道；专用 editor 启动需 `-NFControlEnable -NFControlRegistration=<绝对注册文件> -NoLiveCoding`。注册是恰好四个字符串字段的对象：`project/pluginDirectory/binarySha256/reportDirectory`。它显式绑定本项目登记的独立 .uproject、准确 Reports stage/DLL 与新公共报告目录；A后默认复用已登记工程，不要求每次新建。工程名用 NFC 前缀、ASCII字母数字、≤20字符；服务兼容原 ControlEditor 前缀但仍限≤20，历史长名证据不重命名。根/RC1及其它工程不作为本次调用目标。

注册文件与公共报告根须在 `X:/NodeForge/Reports/PluginControlStageTwo`；editor 是已经运行的专用 `UnrealEditor.exe`，不能以 Commandlet 冒充。已存在 SessionPublic.json 的报告根拒绝再次启用；新会话须新报告根/当前身份，资产与request/调用报告须新合法身份且不能覆盖已有目标，**不要求全新工程**。A实际在NFCUiApiLab复用Run1/Run2/两个Verify，证据见[A最终报告](../Reports/PluginControlStageTwo/RulesCatalog1/P3/FinalReport.md)；工程/短名/台账规则见[Environment](../Docs/Environment.md)/[AssetNaming](../Docs/AssetNaming.md)。[原示例](../Examples/PluginControl/EditorControl.md)的新工程配方保留原时点，当前复用配方见[RulesCatalog](../Examples/PluginControl/RulesCatalog.md)。CLI 不创建工程、不启 UE、不选第一个 PID。

服务只在 GT 读工程/模块并创建随机会话。`SessionPublic.json`是无秘密身份投影，包含准确 `pid/processStart/project/module/binarySha256/controlSessionId/pipe/sessionFile/reportRoot/engine/limits`。32字节随机凭据仅存在受保护DACL的 `Reports/PluginControlSessions/<session>/Session.json` 与认证消息中。凭据不写命令行、普通报告/日志/截图或交付清单，不复制该文件。捕获的旧 public identity 仅为历史证据，停止后不可用于重新连接。

CLI显式提供 `--session-file --project --plugin-directory --run-directory`；以安全句柄核descriptor路径/DACL及实际pipe服务端PID、进程创建ticks、命令行工程和加载模块/SHA，再握手互核。受限token拒绝是已测机制，不能说成另一个真实Windows用户测试。

## 实际方法

| 方法 | arguments（准确字段） | expectedDraftRevision | 分派 | 结果种类 |
| --- | --- | --- | --- | --- |
| operations.describe | {} | 不得提供 | readOnlyCache | capabilityDescription |
| session.hello | editorApiVersion:string, profile:string, controlSessionId:string, expectedProject:string, expectedProcessStart:string, expectedBinarySha256:string, credential:string, expectedPid:integer | 不得提供 | handshake | sessionIdentity |
| editor.getState | {} | 不得提供 | readOnlyCache | publishedState |
| draft.get | {} | 不得提供 | readOnlyCache | publishedDraft |
| draft.loadFile | path:string；可选成对exportReportPath:string/exportReportSha256:64lowerhex | 必需 | gameThread | draftReceipt |
| draft.replace | text:string；可选retainExportOrigin:boolean及成对exportReportPath:string/exportReportSha256:64lowerhex（true与来源字段互斥） | 必需 | gameThread | draftReceipt |
| draft.restore | {} | 必需 | gameThread | draftReceipt |
| draft.prepareCopy（C正式） | {} | 必需 | gameThread | copyPreparationReceipt |
| graphs.preview（C正式） | {} | 必需 | gameThread | graphPreviewReceipt |
| graphs.preflight | {} | 必需 | gameThread | preflightReceipt |
| graphs.generate | {} | 必需 | gameThread | generationReceipt |
| rules.query | assetKind:string（Material或Blueprint）, query:string（≤256字符、≤8词） | 必需 | gameThread | rulesReceipt |
| catalog.get | {} | 必需 | gameThread | catalogReceipt |
| resources.inspect | kind:MaterialParameters或MaterialFunction, source:准确Package.Asset | 必需 | gameThread | resourceReceipt |
| graphs.export | assetKind:Material或Blueprint, source, requestId, assetPath | 必需 | gameThread | exportReceipt |
| calls.get | callId:string | 不得提供 | readOnlyCache | callRecord |
| control.disable | {} | 不得提供 | management | stoppingRequested |

所有字段名/大小写精确，额外arguments/context字段拒绝。hello是专用预鉴权消息，由客户端自动执行，普通 `editor-call` 不接受它。`expectedPid`是非零uint32 JSON整数，其它hello字段是字符串；credential仅由客户端从受限文件读取，不手写或展示。

普通params为 `{"context":{...},"arguments":{...}}`。context恰好包含 `controlSessionId/editorApiVersion/profile`，GT方法再加 `expectedDraftRevision`。revision和Windows创建ticks都是规范十进制uint64字符串，不接受前导零/符号/溢出。RPC `id`是1–64个ASCII字母、数字或连字符；GT调用将此id登记为callId。只读查询的请求id与arguments.callId可以不同。`controlSessionId`、原生报告的sessionId和图requestId互不混用。

读取状态/草稿消费I/O缓存，不在传输线程读UObject；草稿和真实生产tab共用一个服务。整体相同稿不增加修订，恢复一步，无恢复点返回具体拒绝；本地临时空/超限稿允许编辑，外部读取超限稿只提供元数据并标不可读，不截断冒充全文。UI/API共享busy槽，程序变更不能伪造UI来源。关闭通道仍保留UI与当前服务状态。

`draft.loadFile`只载入，不预检/生成。绝对UTF-8输入限本项目 Examples/Tests/Reports，排除PluginControlSessions和reparse；同一安全句柄核身份/大小并读取。`draft.replace`只接受非空有界文本，来源为program。固定样例已核稿SHA与原文件SHA一致；其它编码不作未验证的保真承诺。

B增量来源字段必须同时提供，原导出报告限本项目公共Reports、排除私有会话/reparse；同安全读取核原字节SHA、当前会话/DLL/引擎/工程、源文件、Graph种类/目录及原payload。首次载入与原导出语义一致，格式可变；后续编辑/准备副本保留派生来源，无关替换清掉旧基线。恢复仅恢复匹配来源。预检和生成消费来源并再次核源/原直接依赖，报告或源冲突停止依赖写入，不保存dirty源或自动重载。

两项读取均为共享协调器，精确saved/clean类型和允许根，Blueprint须有可用GeneratedClass/编译状态；注册表Exists未知保留sourceNotReady，已加载无受控保存文件证明拒绝sourceMemoryUnproven。API不改变草稿/origin/revision/LastSuccess/LastImport/收据/LastExport，UI同共享实现再做有修订保护的成功后置提交。纯API的四模式不预留身份、不生成新资产；完整导出不能丢不支持节点或有效属性。原报告增加resourceRead及完整exportedRequestText/sha，模式仍为MaterialParameters/MaterialFunction/MaterialGraphExport/BlueprintGraphExport。

`graphs.preflight/generate`从唯一服务稿取精确快照，复用原GraphPreflight/Import。程序报告标 `inputMethod=editorProgram`，sidecar关联controlSessionId/callId/revision/native报告；GUI点击才有原 `draft/uiInput` 收据。预检不预留、不构图/保存；生成0及 `waitingPersistence`只证明构图/编译/保存，独立持久化继续用旧 `graphs.verifySaved`，没有同进程Verify方法。

## 副本准备与差异预览（C正式）

`draft.replace`省略retainExportOrigin或false仍为原严格Replace。初次附原报告载入必须与原导出语义一致；loadFile不继承旧来源。true仅编辑当前已经绑定的可信来源，不接受任一exportReportPath/exportReportSha256，原生和CLI消费唯一schema并校验精确boolean。编辑再次核当前report、类型/catalog和saved source SHA，保留原文、来源、准备关系及一步恢复点；同文本不增revision，改变内容后旧结果stale。无来源返回exportOriginRequired；失败原稿保持。

`draft.prepareCopy`仅开放现有Graph v2 Material/Blueprint，调用原PrepareNewCopy并由共享Service校验/提交。成功只改requestId/assetPath，保留全部图数据/目录/同父目录；返回copyPreparation原字段、draftApplied、previousDraftRevision/newDraftRevision，draft元数据及原报告/sidecar引用。完整准备稿由draft.get.text取得，copyPreparationReport关联原记录。准备不预约、不生成资产或收据；目标仍须生成前再次检查。草稿提交前全稿字节/SHA及独立身份核清；报告写出但未提交时原稿和修订保持，记录不删除。unknown可能已有提交，先查询原call与draft，不重发/改身份。

`graphs.preview`从当前ExportOrigin取已核基线，arguments为空，调用者不能传任意previousReport。沿原ResourceSource/FCapture核真实源和原报告，再用原两族导出与GraphDiff比较当前快照；原报告graphDifference经正式报告读取取得。API/UI调用相同Service协调器。预览不修改稿、revision、PreparationReport或lastSuccess/lastImport/lastExport，不构图、不保存、不预约；输入改变后旧预览stale。无来源为写快照前已知拒绝，原报告/源冲突停止依赖写入。差异只声明结构精确值与布局，不声明编译/运行/画面等价。

两项同GT/busy/revision，仍没有Commandlet新增操作。sidecar的draftRevision/snapshotDigest绑定调用输入，newDraftRevision/draftApplied绑定真实提交结果，不能把native Prepare报告成功当草稿提交成功。32KiB一般pointer、64KiB完整Graph、1MiB报告上限保持；超限返回导航，不能截断成完整。

## 规则和目录查询（A增量）

两项查询走GT和唯一业务槽，要求准确expectedDraftRevision；空稿/非法稿也可查询，成功不改草稿、revision或资产。UI两个按钮和程序接口共用Service::QuerySnapshot→原GraphRules/GraphCatalog核心。query按真实class/function owner/member/variable type做大小写无关OR子串检索；空query返回指定完整族，无匹配明确拒绝。结构schema负责精确字段/类型，assetKind、长度/词数语义由原TaskRules再核验；不开放额外参数、分页或其它查询宿主。

终态result仅给nativeReport路径/SHA/字节数/原生session、querySnapshot/queryDigest、catalogDigest及controlEvidencePath/controlEvidenceSha256，不把完整目录塞入RPC。报告读取≤1MiB。GraphRules成功必须是NodeForgeTaskRules1、operationStatus=passed、overallStatus=succeeded及taskRules v1；GraphCatalog成功是NodeForgeActualCatalog2、operationStatus=passed/catalog，原生不含overallStatus。各自投影核mode/project/PID/摘要及真实payload，不能套用生成报告的全部必填字段或伪填成功。

程序sidecar记录实际host/inputMethod=editorProgram、controlSessionId/callId、查询mode/快照及SHA、revision、project/PID/processStart、module/二进制SHA、报告引用和目录摘要。原生报告不合成inputMethod/uiInput；UI来源以实际按钮路径及本批真实UI证据记录，不能伪装程序为UI。查询成功和封闭拒绝均可查原call；原生rejected映射CLI2、failed映射4，缺失/损坏/身份不符映射unknown/CLI5，调用者停止后续依赖写入并查询原call。

查询重置当前展示/旧读取内容，保留lastSuccess、原生成输入快照/摘要与lastExport基线；失败不复用旧读取结果。目录摘要必须来自同环境的完整GraphCatalog，不能对子集重算供生成。发现/实验/已验证层级保持，查询不证明任意组合、动态签名、运行或画面。最小调用见[RulesCatalog例子](../Examples/PluginControl/RulesCatalog.md)。

## 帧、权限与边界

Windows本地字节pipe为4字节little-endian长度+UTF-8单个JSON对象；partial头/体累计，严格重复key/深度/长度/类型校验。不支持batch/notification、任意代码/NFMode/console、HTTP/MCP/公网和加载DLL热换。使用Overlapped、显式受保护ACL（当前用户及SYSTEM）、远程连接拒绝、server PID核对；取消后观察I/O完成才释放资源。原始安全/断连/生命周期证据沿 [S2复核](../Reports/PluginControlStageTwo/EditorControl1/S2/ReviewD.json)及 [S4A/B](../Reports/PluginControlStageTwo/EditorControl1/S4/ReviewAB.json)。

| 实际描述字段 | 值 |
| --- | --- |
| frameBytes | 524288 |
| graphBytes | 65536 |
| reportBytes | 1048576 |
| descriptorBytes | 65536 |
| jsonDepth | 32 |
| connections | 2 |
| businessSlots | 1 |
| retainedCalls | 128 |
| helloSeconds | 5 |
| frameSeconds | 10 |
| callSeconds | 180 |
| querySeconds | 5 |
| idleSeconds | 60 |
| stopSeconds | 10 |
| exitSeconds | 60 |

字节上限分别约束frame/草稿/报告/descriptor。hello/frame/query/idle/stop为各自绝对截止，partial进展不刷新；CLI `--timeout-seconds`默认180，是调用总截止，OS查询还有各自有界限制。截止不能强制中断GT同步编译/保存，也不等于操作没执行。128个已受理call保留到本session结束，无自动淘汰重用；耗尽登记容量须正常结束会话，不能自动重发。

## 受理、结果与恢复

只有新GT调用已写入不可覆盖Accepted记录、占唯一业务槽后才受理；相同callId/参数返回原记录，同ID不同参数callConflict。GT分派前重核session/stopping/revision/busy，I/O不阻塞GT、GT不等网络。缓存状态含idle/busy/stopping、current/stale与lastSuccess，当前失败不把旧成功当新结果。

call状态为accepted/queued/running及completed/rejected/failed/cancelled/unknown；只收到accepted不是成功。客户端自动只读查询原call直到终态或总截止。断连/截止不撤销业务，不能换ID重发；unknown先查旧call、报告、进程和资产残留。单个run独占新目录，CLI保留Identity/Description/Request/FirstResponse/Query/Response/Summary；输入/原生收据另引用路径与SHA，不改原报告。

CLI `editor-describe`取描述；`editor-call --operation ... --call-id ... [--expected-revision ...] [--arguments-file ...]`调用；`editor-status --call-id ...`只查旧call。arguments-file须在Examples/Tests/Reports、非会话凭据目录、无reparse且有界对象，只含实际schema声明的精确字段/类型；boolean须为JSON true/false，整数不接受boolean或小数。stdout是一份UTF-8摘要JSON，help为文本。

| CLI退出 | 实际含义 |
| --- | --- |
| 0 | 该操作取得约定结果；生成仍待独立Verify |
| 2 | 参数/方法/schema/revision或业务写前拒绝 |
| 3 | 身份/会话/stage/权限/环境阻塞 |
| 4 | 原生业务终态失败，保留具体诊断/残留 |
| 5 | pending/stopping/unknown或无法确认终态；停止后续写入，只读查证 |

`control.disable`先回复stoppingRequested。CLI关闭peer并在原总截止内核StopReceipt、实际pipe消失、descriptor删除、所有worker threadDone且pendingIo=false/cancelRequested=cancelCompleted，才能返回0/stopped；否则5，不能声称已停止。正常退出仅限已重新核实的专用editor，须等待原调用完成并保留记录，不强杀保存。最终模块/ticker/tab/菜单/driver清理分层证据见实际报告。没有公开的enable/restart/原位覆盖/cancel API。

## 交付范围与证据层

同PID两类程序图的原始闭环在105；最终118生产service/构图/编译/保存相关源码逐字节相同，工程短名启动门槛和受影响UI已118实测，最终118旧CLI也实跑。此为准确组件源码连续性，不能声称两个版本整个DLL相同。图/程序/UI/CLI/原格式恢复各层记录不同：119真实UI原剪贴板清理为保留外部变化；120是Python自有窗口的真实Win32恢复fixture，不能说成UE-owner正例。

S阶段有限11方法不等于完整AC037。A/B已分别开放规则/目录、资源读取/两族导出；C正式准备/预览已有限验收通过，重排及其它UI操作没有据此全部接口化；新节点族、覆盖旧资产、根/RC1升级、运行/材质画面/打包不因本阶段通过而宣布完成。许可和固定来源沿 [复用账本](../Reports/PluginControlStageTwo/EditorControl1/S0/ReuseLedger.md)及 [ThirdPartyNotices](../Plugins/NodeForge/ThirdPartyNotices.md)。
