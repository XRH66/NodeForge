# NodeForge MCP 契约与版本状态

2026-10-13有限候选Build136 Repair6/桥0.3.6/Mcp9R13：20工具、16业务、18 editor定义；resources.query新增三族有界元数据查询，resources.inspect支持Texture2D精确读取，其余JSON2/3/来源/准备/预览规则保持。原SDK2.3.0/ABI1与显式准备/连接边界不变。正式SDK实际发现/有限资源调用、当前Codex同服务资料与兼容重绑定通过；不能把SDK业务冒称本对话新query工具调用。准确字段见[当前ToolContract](../Docs/Versions/Build136/Mcp9R13/ToolContract.json)，证据见[PC8报告](../Reports/PluginControlStageTwo/PersonalCandidate1/PC8/FinalReport.md)。下文为各历史版本归属。

2026-10-12 JR-DOC-20261012-02正式有限交付Build133 Repair2/桥0.3.5：原19工具/官方SDK/ABI和开放政策保持，graphs.export仅增加可选formatVersion=2|3，遗漏2、新配方显式3。桥按本版native字段生成的schema及最小wire身份accessor核v2/v3，完整导出返回原text/value/payloadSha256/sourceRef；这些不是资产最终验证。本期J2/J3/J4验收已通过，旧冻结132/0.3.4原字节保持。原报告/DLL/源保存证明、unknown查询及同连接更新边界保持；不新增实例或逐节点MCP。

本期J0→J4必需项及JV01–16已按原证据收口；两族新资产保存和新PID原CLI Verify、双格式完整导出/来源/编辑/预览、实例transient生产回读及当前MCP/schema/资料已验。候选待验措辞仅描述之前时点；当前正式资料为[Build133/Mcp6R4](../Docs/Versions/Build133/Mcp6R4/Readme.md)，原[逐项验收](../Reports/PluginControlStageTwo/JsonArchitecture1/J4/FinalAcceptance.json)绑定准确范围。Build132及旧包的能力不升级，根/RC1保持。

2026-10-12有限C正式交付Build131/桥0.3.2/NodeForge-Build131-Mcp4，Mcp4R2冻结资料；19工具/15业务投影/17 editor定义。新增nodeforge_draft_prepareCopy、nodeforge_graphs_preview，draft_replace可选retainExportOrigin:boolean；两族真实SDK正式链及新PID原CLIVerify、当前Codex新帮助/资料读取通过。[最终报告](../Reports/PluginControlStageTwo/CopyPreview1/C4/FinalReport.md)/[逐项](../Reports/PluginControlStageTwo/CopyPreview1/C4/FinalAcceptance.json)；原Build130冻结包保持。

## C正式准备与预览

两个工具均需当前handle、新callId、expectedDraftRevision及arguments:{}，复用唯一Operations/CLI/共享Service。初次导出来源严格原样载入；编辑当前来源时draft_replace明确retainExportOrigin:true且不附任何新exportReport字段，精确boolean/互斥由原生/CLI/MCP同schema校验。Prepare返回原nativeSummary与draftApplied、前后revision、copyPreparation，完整派生稿从draft_get取得；桥验证原native/sidecar/当前稿与真实准备文件SHA/身份。准备不预约不生成，记录已写与稿已提交分开。未知结果保留真实draftApplied而不转success。

graphs_preview只取当前已核Origin；正式report_read读取graphDifference/子树，超限导航仍32KiB，完整Graph仍64KiB、报告1MiB。不开放previousReport/任意file/operation。预览不改稿/revision/原asset/receipt/成功基线；不能证明运行或视觉相同。本版报告namespace为CopyPreview1，新DLL/工具/资料/批准Config成套；原0.3.2/ABI兼容应用接续复用，活动调用/unknown未清不能升级或抢lease。正式业务与当前客户端资料按不同证据层记录，不把SDK业务冒称当前对话新工具调用。


2026-10-11有限B正式交付为Build130、桥0.3.2、冻结NodeForge-Build130-Mcp3R4，17工具/13业务投影/15原生editor方法。正式读取/两族导出、各一件新副本编译保存与新PID Verify通过；未变Build125/128原生与UI证据按连续性复用，未宣称130重跑全部检查。原Codex同连接自动载入后续应用代码和最终资料已实际通过，SDK/解释器/Application ABI变化保留服务级载入边界。根/RC1未升级。当前证据见[最终报告](../Reports/PluginControlStageTwo/AssetRead1/B4/FinalReport.md)/[逐项验收](../Reports/PluginControlStageTwo/AssetRead1/B4/FinalAcceptance.json)；以下带原版号段落是历史契约背景。

## 0.3 显式资源读取与完整导出增量

`resources.inspect` 的 arguments 为 `kind=MaterialParameters|MaterialFunction, source=准确Package.Asset`；`graphs.export` 为 `assetKind=Material|Blueprint, source, requestId, assetPath`。两方法均需handle、新callId及准确expectedDraftRevision，经原GT gate/统一操作/共享Service执行。Material/MaterialFunction源限/Engine/或测试根；Blueprint源限测试根且saved/clean/compiled。当前共享链的受控保存文件加载证明不可缺；sourceMemoryUnproven不自动reload/resave，sourceNotReady不当作不存在。API只更新本次查询结果/报告，不改草稿、来源、修订、成功输入、收据或导出基线；UI成功后的载入由其捕获修订保护。

成功导出报告保存 `exportedRequest`、准确 `exportedRequestText`、`exportedRequestSha256` 与 `exportEvidence/resourceRead`。`report_read` 的 `/exportedRequest` 是唯一完整Graph特例：Graph正文最多65536 UTF-8字节，一次返回value、text、payloadSha256及sourceRef；其它pointer仍限32768字节，原报告仍限1MiB。sourceRef包含exportReportPath/exportReportSha256/source/sourceAssetSha256/assetKind/payloadSha256；报告SHA、源文件SHA和payload SHA不可互换。`draft.replace/loadFile`只增加成对可选exportReportPath/exportReportSha256；服务校验原报告、会话/核心、首次载入语义与来源，后续预检/生成再核源。Graph2不增加未知字段。完整任务/停止/Verify配方随新包AiAssetRead交付。

候选报告和执行根限AssetRead1本包namespace。已配置的McpDocs1/Client/Config.json仅作为一个准确启动入口兼容，不开放其相邻文件；内容必须成套绑定本版工程/stage/DLL/工具/资料及新namespace。业务身份更新不能借资料刷新绕过核验；桥0.3.1载入后可按下方条件在同一SDK进程安全接续，无活动调用才可成套激活。

2026-10-10 P0冻结及后续有限交付已完成，真实验收状态见[McpDocs1 P4最终报告](../Reports/PluginControlStageTwo/McpDocs1/P4/FinalReport.md)。NF057/D022/AC041/T025原交付保持：Build122、Editor1.0/profile nodeforge.editor.v1、桥0.1.0、bundle NodeForge-Build122-Mcp1。根/RC1未升级。运行时唯一机器定义是[全局ToolContract](../Tools/NodeForgeMcp/ToolContract.json)，业务schema直接投影[准确13方法快照](../Reports/PluginControlStageTwo/McpDocs1/P0/OperationsSnapshot.json)，不是另一张节点白名单。

2026-10-11 NF058/AC042/T026桥0.2.0、bundle NodeForge-Build122-Mcp2已成套激活并实际载入本Codex；完整资料已E4冻结且本Codex索引/首页实际一致，见McpMaintenance1/E4最终报告。候选定义见[Mcp2R1 ToolContract](../Docs/Versions/Build122/Mcp2R1/ToolContract.json)，实际进度见[ProjectStatus](../ProjectStatus.md)。下文原业务和保护契约继续适用；新增帮助/结果schema/发布规则须完成本期针对性验收，不能把旧P4通过算作新代码通过。

## 0.2 帮助与扩展政策

`Tools/NodeForgeMcp/ToolMetadata.json`是可追踪的工具用途、前置、效果、成功判据、参数说明与guide pageId来源；发布器消费同版Operations和已有受控adapter，生成ToolContract。服务发现从该契约返回帮助和结果schema，initialize instructions给出准确文档阅读路线。原15工具名称和输入参数保持；结果schema描述原返回，不转换unknown/pending/native状态。

普通节点/目录扩展更新核心允许配置、Catalog/Rules/digest、精确NodeDocumentation及版本资料；仍通过原业务工具，不逐节点增加MCP handler。特殊语义先补真实核心实现与必要回读/验证。新业务从共享Service和唯一Operation定义开始，必须显式评审AI开放、受控adapter、帮助/输入/返回/恢复与资料；`defaultAiExposure=deny`，未知/管理操作不会自动投影。当前session.hello/control.disable仍不作为默认AI工具。

新包由发布输入绑定核心Build/DLL/引擎/适配器、Operations/Catalog、桥/SDK/源文件、知识源与页SHA；发布器默认业务验收notEstablished，不自动继承基础包passed。旧未变组件的实测须由实际影响Review明确复用。候选与冻结、保存与新PID Verify、运行/画面/打包分别记录。维护说明见[节点扩展](../Docs/PluginControl/NodeExtensionWorkflow.md)和[版本发布](../Docs/PluginControl/VersionDocumentationPlan.md)。

0.2候选的纯资料刷新允许同步authoringSources/htmlArtifacts，以及publicationInputs中固定路径pages/nodeDocumentation/controls/sourceNotice的SHA/bytes；只在操作者批准的新Manifest SHA下采用，并保持原handle/call/report/unknown/pending。工具元数据、Catalog/Operations及其它输入、执行代码/SDK绑定仍要求重连；HTML限定本包，作者源限定Docs，有界引用不能逃逸或夹带私有session。此适配本期SDK/当前Codex加载已实际验收，见McpMaintenance1/E3；旧0.1/P4证据仍保持其原版次。

0.2候选docs_index返回版本/Build/DLL/API/目录身份、能力数量、读者入口和全部指南导航的发现摘要；不会返回139节点细页或整包维护清单。完整绑定仍在本机Manifest，源码/许可/证据通过专用页导航。摘要最多32768 UTF-8字节；超限返回resultTooLarge、selectionReturned=false、limitBytes和NFMCP_DISCOVERY_INDEX_TOO_LARGE，不截断冒称完整。report_read自己的超限导航语义保持，普通业务原结果不变。

## 入口和配置

本机Windows x64/Python3.12；官方SDK mcp==2.3.0，固定上游2118f14f8a19bc158d8a1cf90af58d85d187f849及[轮子依赖锁](../Tools/NodeForgeMcp/Requirements.lock)/[许可与哈希](../Tools/NodeForgeMcp/DependencyManifest.json)。只开放stdio；SDK处理协议协商/封帧，stdout仅MCP，诊断到stderr。MCP会话与NodeForge控制会话/图请求不同，不能互换身份。已实际协议/发现结果及修订在P0 SDK/Codex报告中；真实业务调用须P3，不据网页推测客户端兼容。

本地操作者提供`--config`，不得由AI指定任意工程、PID、会话凭据或脚本。配置精确字段：`format=NodeForgeMcpConfig`、`version=1`、`bundleId`、`project`、`pluginDirectory`、`binarySha256`、`sessionPublic`、`reportRoot`、`bridgeRunRoot`、`toolContractSha256`、`docsManifestSha256`；字符串路径均绝对。仅既有登记NFCUiApiLab、当前批准Build130准确stage与同SHA；本期报告/桥运行根属于Reports/PluginControlStageTwo/AssetRead1批准namespace。McpDocs1/Client/Config.json是保留的固定启动配置入口，其当前内容已成套绑定本期身份，不把旧目录作为新业务允许根。每个新授权任务由操作者准备新run并绑定准确SessionPublic；B000003/Run4已结束，仅为历史证据。无连接时docs工具可读正式manifest，业务明确未连接。原Build122配置验收保持原版记录。桥不启动/停止UE、不做进程编排或独立Verify。

`nodeforge_connect {}`调用原CLI editor-describe，原安全客户端内部读取DACL保护descriptor并hello。桥核当前public身份/报告根/运行描述/配置版本一致，返回随机32hex handle及无秘密身份；每个CLI调用仍核实际PID/创建时间/pipe/module/SHA，不把handle当安全认证。配置不含credential；不会返回Session.json内容。不同控制会话使旧handle和报告身份失效，不自动重连后重发。已连接handle重复connect仅只读核当前身份，未结写入时禁止切换会话。

2026-10-11启动锁有限修正：MCP启动、工具发现、docs_index/read不取得业务lease，可由多个客户端读取同一个固定版本包。connect先核公开身份，再非阻塞取得配置bridgeRunRoot中共享的Bridge.lock；同实例首次连接也串行。已存在的业务lease返回environmentBlocked/NFMCP_BUSY，不退出stdio、不重发、不自动清锁，资料工具仍可用。业务实例继续使用同一个Calls/CLI证据根；只读实例退出不删除其它实例lease。仍有pending CLI、执行中GT或connect时保留lease；已退出/未知旧所有者及未结记录必须按有证据的有限恢复处理，不能仅因进程消失自动接管。首次ProtocolSeen写入竞争保留先写记录，旧记录只代表其原观察时点。实施/未验状态见[有限修复计划](../Docs/PluginControl/McpStartupRepairPlan.md)。

## 原0.1／0.2的15工具与参数

11个业务工具名称按`nodeforge_`＋正式method中的`.`替为`_`生成，例如`nodeforge_draft_loadFile`、`nodeforge_graphs_generate`。所有业务参数含`handle`和`arguments`；arguments精确消费原argumentsSchema，即使无字段也提供`{}`。GT方法额外必需显式`callId`（1–64 ASCII字母数字或连字符）和规范uint64字符串`expectedDraftRevision`。不根据MCP请求id生成GT身份，不自动修订重试；只读RPC内部用短随机请求id，calls.get的arguments.callId仍查询原GT调用。

原13项中的`session.hello`仅CLI内部握手，`control.disable`仅操作者原入口管理，均不开放默认AI工具。唯一可调用业务集合是冻结机器定义中的11项，不提供任意method/console/eval/NFMode、OS命令、逐节点tool或第二套构图器。后续目录节点变化不改变tool数，新增业务方法需另行审定定义、版本和测试。

其它四工具：

| 工具 | 精确参数 | 含义 |
| --- | --- | --- |
| nodeforge_connect | {} | 固定本地配置连接/核身份，取得opaque handle |
| nodeforge_docs_index | {} | 返回同版Manifest条目、版本和能力入口 |
| nodeforge_docs_read | pageId:string（1–128） | 仅读取manifest指定的同SHA、有界非秘密资料 |
| nodeforge_report_read | handle、reportId:64hex、pointer:string（≤1024） | 只读本会话已登记原生报告；pointer为RFC6901 JSON Pointer，空字符串指根 |

读取报告不接受路径、任意SHA或任意文件名。reportId绑定本会话/原call/报告路径/SHA/字节，保留原生report，不改字段。每次读取核新安全句柄/最终路径/大小/完整SHA及归属；排除reparse、Session.json、PluginControlSessions、越界和指纹变化。完整原报告≤1MiB；所选JSON返回≤32768 UTF-8字节，超出时明确`resultTooLarge`及可导航键/数组长度，不能截断冒充全部。文档≤65536 UTF-8字节、仅manifest列举条目；manifest自身≤1MiB、SHA固定。报告标识最多256项，不淘汰后复用身份。

## 结果与恢复

工具返回MCP TextContent＋同值structuredContent对象，必含`bridgeVersion`、`status`；成功仍保留`cliExitCode/executionState/operation/callId/controlSessionId/response`以及原call/报告标识和CLI证据目录。GT结果按原语义保存，不包装成笼统资产成功。完整大目录留原报告，业务返回目录digest、数量和可读报告身份；通过pointer取得实际节点/引脚。描述和目录分别校验，无数据则不使用旧缓存。

CLI业务出口0、2、3、4、5的意义保持[Editor契约](EditorControlProtocol.md)；桥自身参数错误`rejected`，身份/版本/文件前提错误`environmentBlocked`。已提交后CLI截止、输出损坏、断连或报告不符为`unknown/pending`，保留原callId/CLI目录，不声称写前拒绝。不自动重发或换ID；关闭后续GT写入，只允`calls.get`、状态/草稿/报告读取核证。cancel/客户端断开不能撤销UE保存，服务器完成已开始CLI并记录LateResult；仍须查询原call明确终态。

GT使用唯一桥锁，并由原UE业务槽再守卫；busy/stale保留具体原错误，不刷新revision重试。所有CLI单次总截止180秒、外层仅增加有界回执收束时间；不创建无限Session，不强杀UE。写入前保存原调用/状态；同callId相同参数查询原记录，不再执行桥端第二次提交，同ID不同参数拒绝。终态未知不因MCP isError而当作已失败未写入。原128 call容量/2连接/1业务槽/64KiB草稿/32层JSON边界保持。

生成`waitingPersistence`只证明构图、编译和保存。操作者正常停通道/退出本批自有editor，确认工程独占后，使用原CLI `graphs.verifySaved`同stage新PID、原输入和原生成报告，各核一个新资产；不Import、不resave。运行、材质画面、打包分别notRun。故障/范围变化停止条件按NextStagePlan，不自动扩测或升级插件。

## 文档与扩展

2026-10-11 DocsRefresh1有限修正：每次资料读取从启动时固定的本地配置路径重读受控配置。只有docsManifestSha256变动且新Manifest的业务版本、工程/stage/DLL、API/schema/目录、原CLI及运行桥/依赖绑定完全不变时，允许在同一连接刷新资料快照；页仍限定当前版本目录、唯一pageId、64KiB及精确SHA。生成器绑定和资料页/索引/发布证据可变化，Catalog/Operations机器内容不能借资料刷新替换。读配置和Manifest后再次核原字节，发布竞争或未批准SHA仍拒绝，失败保留旧快照。

资料刷新只更新文档快照与已核docs指纹，保持handle、公开业务身份、修订、原call/report、unknown/pending及业务lease；不重连或重发、不启动CLI/UE。返回附docsManifestSha256便于客户端核当前资料；原0.1/P4的15工具、参数及输出schema未变。0.3.0及更早代码对业务绑定变化返回environmentBlocked/NFMCP_CONFIG_RELOAD_REQUIRED，需要载入新服务代码。桥0.3.1补充以下同进程接续；它不声称运行中的旧Python实现已经热替换，也不把重启整个Codex作为默认步骤。

## 桥0.3.1：已批准配置的安全接续

现有SDK服务在tools/list或tool调用入口读取原固定配置。仅资料SHA改变仍走原资料刷新；其它绑定改变时，只有零在途请求、连接/业务槽空闲、unknown/pending/待收束CLI为空、磁盘Calls全部终态且无需复核，才构造完整新Bridge。原已登记编辑器必须按PID和创建时间确认结束，私有descriptor必须已关闭；存在活编辑器、未知进程或未关闭会话则停止。不会启动UE、停止进程、抢占或删除其它owner的锁。

新Bridge重新核批准Config、ToolContract、Manifest、实际DLL、原CLI及同工程/API/profile。运行中的MCP/Bridge/Transport/进程守卫与SDK锁指纹必须仍与该进程最初载入的代码一致；Python实现或SDK更新返回NFMCP_RUNTIME_RESTART_REQUIRED，需要一次服务级载入。DLL/目录/操作/工具资料更新在这些条件满足时可接续，不要求每版手动开关。

提交前重读配置/契约/Manifest防发布竞争，保留admission与committed两份接续记录。正常释放的仅是本进程自己的空闲lease；旧call/report/原字节不删不改。旧handle失效，新Bridge不携带旧连接或报告缓存，下一步由客户端调用nodeforge_connect取得新编辑器身份；不会自动重发原业务。返回configurationRefresh说明是否需新连接与tools/list。工具定义改变时，旧协议按SDK已协商能力发送tools/list_changed；2026-07-28及后续协议不冒发旧通知，本服务不宣称subscriptions/listen支持，客户端通过正式tools/list取得新定义。节点扩展通常不改变业务工具，仍需新目录/资料和完整发布身份。

NFMCP_REBIND_BUSY／UNRESOLVED／EDITOR_ACTIVE／SESSION_NOT_CLOSED说明接续尚未获准；旧现场保留，不把它当未提交后换ID重发。批准新配置前应完成原调用查询和正常关窗。若配置提前换版，保留的原handle可仅查询本桥已登记的原calls.get或report_read：仍核最初载入Python/SDK、原public身份、原报告SHA/范围及原CLI；不接受新调用或未知报告，不换用新工具契约。返回configurationRefreshPending=true，查询可收束原unknown，待旧会话正常结束后再接续。服务退出也保留未决/磁盘submitted记录的owner租约，不能靠重启消除未决状态。失败不回退到未批准旧版本执行业务；未知结果、waitingPersistence及独立Verify口径完全不变。

原冻结文档bundle位于`Docs/Versions/Build122/Mcp1`；本期候选见页首版本状态。机器Manifest包含bundle/桥/API/引擎/适配器/DLL/完整目录digest及每页path/SHA/bytes，只有实际发布Review通过才能冻结。业务资料说明现有UI/后台和未开放API的边界，历史验证带原Build/层级，未知保持未知。节点页由同版真实GraphCatalog生成，业务schema由实际Operations生成；两历史目录适配仅证明桥的数据读取/缓存失效，不证明未来节点UE运行。


## 桥0.3.2：成套应用代码自动载入

此节适用于实际已载入0.3.2和兼容Application ABI1接续核的服务。沿上述空闲、未知结果、原编辑器准确终态、会话关闭、原lease和发布竞争保护，在执行新代码前核批准Config/契约/Manifest/原依赖锁与四份源码SHA，并在独立模块代次执行NodeForgeMcp、Bridge、EditorTransport、RunProcessGuard。下一工具调用使用新应用的函数与Bridge；同一SDK stdio连接及PID保留。旧代次不原地修改、不自动重放、不启动UE。configurationRefresh增加applicationCodeReloaded/loadedRuntimeFiles；接续记录包含oldRuntimeFiles及新摘要。没有提供任意文件执行或客户端重启工具。

输出schema的bridgeVersion改为版本字符串格式；准确版次仍严格绑定批准契约与Manifest，未放松参数/业务状态/原生结果。旧const在真实官方SDK验证中导致缓存拒绝新版返回，失败与修复保留于ServiceReloadRepair1/Checks1及Checks2。initialize.serverInfo.version描述启动时服务接续核，当前业务应用版本及包身份从docs_index/bridgeVersion和代码载入证据读取，不把旧握手版本当当前应用指纹。

SDK/解释器或接续核Application ABI变更不能在原SDK连接内伪装升级，分别保留NFMCP_SDK_RESTART_REQUIRED/NFMCP_APPLICATION_INCOMPATIBLE。旧0.3.1及更早服务没有本代码载入路径，首次安装修复仍需一次实际服务载入。本机SDK与原Codex实际载入分别验证；原Codex同PID119012的下一次调用实际执行新应用代码，见[实际客户端复核](../Reports/PluginControlStageTwo/AssetRead1/B3/ServiceReloadRepair1/ClientRuntimeObservation1/ActualClientReview.json)。已有业务分派/返回投影/原call查询方法未变；源码SHA变化但没有成套批准发布时拒绝执行。

## 桥0.3.3候选：原调用恢复与兼容代码接续

2026-10-12，此节是 NodeBulk1/N3 的修复候选，实际当前 Codex 载入仍须单独核验，不能继承上节0.3.2的客户端通过结论。

早期 `graphs.preflight` 解析拒绝可能没有核心 `inputDigest`。只有正式原结果为 rejected/2、validation failed、未进入 build/compile/save、无写入或保存尝试，且原输入快照、sidecar、报告 SHA/身份完整匹配时，允许正确保留该拒绝。其它缺摘要、结果冲突、写入残留仍为 unknown；不能把拒绝说成资产成功。

零在途请求、无尚未收束 CLI 时，兼容的桥代码修复先按批准 Config/ToolContract/Manifest/源码和 SDK 锁载入新应用。只有工程/stage/DLL、公开会话、CLI/Transport/进程守卫、目录/图协议、Operations、工具 schema 和限额都相同，才将原 handle、调用、报告缓存、unknown 集合和同一所有权交给新代次。原锁字节保持；新代码通过原 `calls.get` 查清结果，unknown 未收束之前仍禁止新 GT 写入。不会自动重发业务，不会更换原资产/request/call。接续日志区分 admitted/committed，返回 sameNativeOwner/newConnectionRequired=false；不兼容的核心变化仍遵循原正常关窗后新连接规则。

已退出旧所有者的恢复使用原 `Bridge.lock` 的排他文件句柄。仅当准确原 pid 为 absent、同一公开会话有效、原 Calls 的每项均通过正式 CLI `calls.get` 确认原 call/session/operation/revision 和真实终态、完整原结果绑定通过，才记录旧锁完整字节/SHA与查询证据，并在原锁文件中交接新所有者。原锁文件不删除、原 Calls/native/input/sidecar 不覆盖；查询不会生成或重发资产。活跃/unknown 所有者、未决调用、身份冲突或文件竞争一律停止。连接后原调用状态仍由正式 `calls.get` 收束，不因交接就清空 unknown。

旧0.3.2接续核在调用新代码工厂之前检查 unknown，因此它无法通过本候选代码自行修正这一启动循环；本候选不声称已经替换该正在运行的旧实现。当前环境的宿主重载入口与实际 Goal 状态见[修复证据](../Reports/PluginControlStageTwo/NodeBulk1/N3/ServiceRecovery1/Readme.md)。不得以清锁、强杀宿主、修改原报告或改写 Goal 完成状态替代实际载入。

## 桥0.3.4：操作定义与当前会话分离

原 editor.describe 报告保留取证时的 identity/PID/管道和报告目录。它们是原会话证据，不是未来编辑器必须沿用的操作定义。桥在验证原快照SHA后，只从操作定义比较中移除 identity；新发布 Operations.json 同步只保留静态定义，原报告字节保持。host、API/profile、全部操作/schema和限额仍严格相等，当前 describe.identity 仍必须完整等于当前 SessionPublic。因此正常新PID可以连接同一接口，错误当前身份、操作或限额仍拒绝。不改变业务调用、来源、unknown/pending或锁规则。针对性及原Codex真实新PID连接证据见 NodeBulk1/N3/OperationSnapshotRepair1。


## BT02成套同步（2026-10-12）

Build134/桥0.3.5/SDK2.3.0/ABI1仍19工具、15业务、17 editor方法；ReceiveTick通过原目录/预检/生成/导出自然可用，无新工具或参数。当前SDK服务112700实际接续新DLL/目录及新saved-load会话，完整导出2/3与来源载入预检通过；准确最终包/Manifest及当前回读见BlueprintTick1/BT4，不凭版本名判断身份。原pending/unknown/busy/修订/来源保护保持。
