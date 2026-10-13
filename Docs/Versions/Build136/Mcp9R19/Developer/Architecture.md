# 原理、调用链与状态

本包对应 NodeForge-Build136-Mcp9R19 / Build136 Repair6。新任务使用当前完整输入与新身份；具体字段见[统一资产设置](../Ai/AssetSettings.md)、[资源选用](../Ai/Resources.md)和[JSON规则](../Reference/GraphAuthoring.md)。准确验证范围集中在[发行说明](../Version/ReleaseNotes.md)。


核心采用受控Specs/允许配置和UE真实反射/Spawner实例。目录记录可发现、可构造、精确引脚/属性/类型；它不是全UE自动开放。输入由同解析器校验，显式新目标、节点/默认/连接和设置经原生构图、编译、回读、保存产生原报告。普通参数由反射/原生方法读取，动态函数/Switch/变量/对象上下文还有专门语义校验；不能靠允许表替代执行语义。

正式链：AI → 官方SDK stdio NodeForgeMcp → Bridge受控工具/版本/身份 → NodeForgeControl CLI → EditorTransport认证/有界管道 → Channel/Protocol → Operations定义 → Shared Service → Graph/Rules/Prototype → 原生报告。UI按钮也用同Service，CLI Commandlet执行另一正式host。MCP不构图、不新造传输、不逐节点注册handler。

状态分层：MCP协议请求ID只管传输；handle绑定桥与controlSession；RPC callId和expectedDraftRevision绑定一次操作/不可变稿；Graph requestId/assetPath绑定项目级生成身份；reportId绑定当前已登记原文件SHA。草稿每次编辑增加uint64修订，旧检查/预览/结果会失效。busy和GT gate防并发，缓存读与GT操作不同。业务先持久记录call再提交，unknown/pending防止再次写入；lease防第二控制者抢占，不是必须启动UE才能看文档。

草稿来源、成功输入快照、源资产及导出基线各自有文件/摘要关系。导出不修改源，预览核基线/源干净/字节一致；重排基于归档成功输入，不是当前稿或后改资产。目标create-only、request收据项目级唯一；保存不等于新PID Verify、运行/画面/打包。

本版显式读取通过NodeForgeResourceSource进行精确路径/原生类型/注册表状态/clean与编译前置检查；GT共享协调器在保存文件只读句柄保护下建立有限的受控加载证明。已载入但没有证明的对象拒绝sourceMemoryUnproven，文件或内存状态有冲突就停止，不能自动reload/resave。resources.inspect/graphs.export的API读取不改变草稿、来源、修订、成功输入或导出基线；UI调用同服务，再对捕获的草稿修订做成功后置提交。

导出原报告同时保存exportedRequest对象、准确exportedRequestText及其SHA。MCP登记原报告后，report_read的/exportedRequest返回完整text/value/sourceRef；其它子树仍有32KiB上限。draft.replace/loadFile成对传exportReportPath和exportReportSha256，服务核报告原字节、会话/DLL、源与payload，首次载入必须语义等同原导出。后续编辑/准备副本保留派生关系；无关替换清掉旧基线，恢复只恢复匹配来源。preflight/generate再次核源及原直接依赖，来源报告变更或结果未知停止依赖写入。Graph2正文不塞入未知字段。

Bridge保留原CLI/nativeSummary而非把它们统一成“成功”。报告只从本会话登记引用读取，pointer有界、不截断；凭据路径不可公开。版本/执行绑定变更要求成套批准身份；已载入0.3.2且Application ABI兼容时沿现有SDK连接自动载入独立应用代次，重取业务handle；SDK/解释器/连接核不兼容才需要服务级载入。批准同版资料刷新只换文档清单，均不重置调用/lease/unknown。详AiResultsAndRecovery和VersionCompatibilityAndMigration。


来源拒绝也必须绑定捕获输入的真实inputDigest：Prototype在来源校验之前输出摘要，Control与MCP才能保持原rejected而非缺身份的unknown。没有摘要仍拒绝推进，不能由Bridge补猜。CLI参数准入以required子集、已声明属性及dependentRequired为准；不能把required当成全部允许字段，否则合法的成对来源参数会在dispatch前被误拒绝。


## 当前来源编辑与准备提交

RetainOriginEditCore从当前draft取ExportOrigin，ReadExportOrigin(false)核派生稿类型/catalog，再FCapture核saved源与report SHA后Input.Edit；首次载入仍ReadExportOrigin(true)。显式retainExportOrigin:boolean与新来源字段互斥，各入口消费同Text schema。PrepareSnapshot复用原PrepareNewCopy，先核native身份、完整派生文件/SHA、仅requestId/assetPath改变，再Input.PrepareCopy；报告/文件已写与草稿提交分开，提交后sidecar失败仍保留draftApplied及新revision、状态unknown。PreviewSnapshot使用当前Origin并由Prototype通用核源链及GraphDiff比较，不改稿/revision/成功基线。两个UI按钮委托同Snapshot生产协调器，没有第二执行规则。CLI精确boolean与标准条件schema同步，桥只验证/投影原结果。完整配方见AiCopyPreview；Build131本期实际调用、UI、生成保存与新PIDVerify分别见有限C证据索引，不能将静态编译或资料发布替代这些层次。


## 同一节点定义与精确能力档案

NodeForgeNodeDefinitions 汇合原材质 Specs、蓝图 Functions 与特殊 K2/成员定义；两个原生图适配器读取统一定义，不共享一个虚构跨引擎生成器。限定反射发现是定义补充，原特殊定义优先；每个新普通条目经同一生产工厂/Spawner 的实例/端口/默认检查才 admitted。发现、原生构造、组合编译保存、独立重开、运行和画面按层记录。

旧摘要通过一个历史档案表保持原材质修订/蓝图修订和实例类型范围。未知/空外部摘要拒绝；新增普通节点仅新完整摘要可用。NativeCatalogProbe 是内部类型化上下文，在 Shape/BuildChecked/内部回读明确传递，不以空摘要提供公共旁路。GraphCatalog 初始化只在 GameThread 执行，定义发现与实例资格分开，完成后按本次加载 DLL 进程缓存准确 JSON/摘要，不在每次 Import 整库重建或空闲扫描。

目录、规则、MCP 及发布器消费原生事实。新普通节点页通过真实目录的原生用途、类型、端口和限制生成，节点身份包含 assetKind/class/binding/variableType/configuration，页ID不充当跨版身份。完整目录不能以查询子集或分页重新计算摘要。用户 UI 与 API 仍委托共享 Service；本批不增加逐节点工具、控制层、运行解释器或隐式 cast。


JSON治理：GraphCodec在输入边界解码v2/v3→GraphDocument，现有Material/BP/Instance/native布局直接消费描述；v1固定路径保持。AssetDefinitions维护闭合资产/类型/值/范围/presence与schema，NodeDefinitions继续普通反射/Spawner/资格及绑定准入。Apply与ReadActual分开；导出由真实对象形成描述，经原native重构后Encode2或3。Python桥只消费生成schema和wire身份，不复制业务规则。Difference/1仅保留兼容输出投影；原report/payload/input SHA从未用模型摘要代替。


BT02普通Actor事件与持续移动已完成有限产品验收；完整JSON、独立时间配方、正式MCP/Verify/来源链及兼容/回退见[BlueprintTick](../Ai/BlueprintTick.md)。
