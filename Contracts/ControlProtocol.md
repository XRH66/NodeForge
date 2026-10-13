# NodeForge Control API 1.0 本地契约

<a id="个人候选实施中的增量pc13尚未构建交付"></a>

## 个人候选有限交付增量（Build136 Repair6）

本期源码在唯一Operations表新增`resources.query`，Commandlet模式为`ResourceQuery`，配置由原CLI的`--input`提供。配置恰允许`kind`、`directory`及可选`nameContains/recursive/limit/after`；前两项必需，kind精确为Texture2D、MaterialFunction或Material。schema与默认/边界由ResourceSource生成并消费：目录最多256字符，名称包含过滤最多128字符且大小写无关，递归默认false，limit整数1–200且默认50，after精确对象路径最多512字符且默认空。仅显式安装绑定的读取子目录可查询，不接受/Game或/Engine根扫描。

查询只访文件和AssetRegistry的磁盘元数据，不加载对象、建立来源证明、改草稿或生成资产。遍历最多5000项、128目录、16层；扫描中、缺失登记元数据、IO/reparse或遍历上限均保留`complete=false/truncated=true`且不提供继续游标。完整遍历按精确objectPath排序，再由after分页；跨页目录变化可能重复/遗漏。`resourceQuery`保存有效filter、当前目录/DLL/安装绑定身份、扫描/完整/截断/游标、遍历计数及rows。行的discovered/policyAllowed与inspected=false、requiresInspect=true、graphExportable=unknown分开；列表不替代精确检查或图导出。

候选安装准入改用选定uproject的`Config/NodeForgeInstallation.json`及安装根的`InstallationManifest.json`，原CLI guard与原生消费同一绑定；不再凭开发目录或工程名推断权限。被查询的目录身份从清单SHA绑定的`Metadata/Catalog.json`只读核对，不为一次列表查询构建节点CDO。prepare保存原配置并核after SHA后回退，不清理其它插件或未决调用。实际字段、冻结条件和后续真实验证见[PC0冻结](../Reports/PluginControlStageTwo/PersonalCandidate1/PC0/FrozenContract.json)及[安装冻结](../Reports/PluginControlStageTwo/PersonalCandidate1/PC0/InstallationFreeze.json)。本段是本期源码契约；Build135 Repair1及以下已交付/冻结包不因此获得新能力，集中构建与PC3/PC5验收仍必需。

2026-10-12 JR-DOC-20261012-02正式有限交付Build133 Repair2/桥0.3.5：原MaterialGraphExport/BlueprintGraphExport的NFInput配置仍必需source/requestId/assetPath，仅新增可选整数formatVersion=2|3，遗漏默认2。现代Import/GraphPreflight/Verify共用v2/v3描述与原构造/保存/收据链；raw inputDigest保持。J3已通过，不改变Build132或冻结包能力；完整结构见[GraphProtocol](GraphProtocol.md)。pending/unknown及原调用查询规则保持。

本期J0→J4必需项及JV01–16已按原证据收口；两族新资产保存和新PID原CLI Verify、双格式完整导出/来源/编辑/预览、实例transient生产回读及当前MCP/schema/资料已验。候选待验措辞仅描述之前时点；当前正式资料为[Build133/Mcp6R4](../Docs/Versions/Build133/Mcp6R4/Readme.md)，原[逐项验收](../Reports/PluginControlStageTwo/JsonArchitecture1/J4/FinalAcceptance.json)绑定准确范围。Build132及旧包的能力不升级，根/RC1保持。

2026-10-11有限B候选Build129保持原Commandlet四项操作和独立graphs.verifySaved入口；新增四模式业务复用原editor-call通道，由EditorControlProtocol管理。128真实读取/导出及按钮已验，129只修正式Transport可选字段结构准入和成功材质导出的解释文字。原输入/生成收据的新PID Verify仍留B3，编译与mock Session不等于独立保存验证；不重跑原四项整批。根/RC1未升级。

2026-10-10兼容增补：本页四项Control API 1.0在最终Build118另一新工程实际兼容通过，见[最终兼容复核](../Reports/PluginControlStageTwo/EditorControl1/S4/ReviewCompatibility120.json)及[真实新描述](../Reports/PluginControlStageTwo/EditorControl1/S5/CommandletOperations.json)。活动editor独立契约见[EditorControlProtocol](EditorControlProtocol.md)。下方Build80指纹和收据是已冻结首阶段历史，不冒充118重新执行。

A增量Build122原四项定义/宿主保持，实际描述见[A CommandletOperations](../Reports/PluginControlStageTwo/RulesCatalog1/P3/CommandletOperations.json)；同NFCUiApiLab/同stage两个新PID的原graphs.verifySaved实测见[A最终报告](../Reports/PluginControlStageTwo/RulesCatalog1/P3/FinalReport.md)。两查询只加活动editor宿主；未变原预检/生成等按组件连续性和旧证据复用，不声称本期重跑四项整批。

2026-10-10实际实现：[正式CLI](../Scripts/NodeForgeControl.py)＋独立UE Commandlet。范围/验收依据[FirstStagePlan](../Docs/PluginControl/FirstStagePlan.md)，证据见[交付记录](../Reports/20261010PluginControl/Readme.md)。活动编辑器/RPC属于后续。

## 版本与操作

仅声明本机Windows、官方UE5.3.2-29314046和Build80 DLL，SHA256为`5e3157850f753edf1d8559b782ebfe87a94c17768f38f59fddbc5254b97b4e00`。Control API只接受1.0；未知方法/版本拒绝，旧NFMode保留原语义。

操作规则唯一来自[ControlOperations.cpp](../Plugins/NodeForge/Source/NodeForgeEditor/Private/NodeForgeControlOperations.cpp)的Operations表，describe和分派共用它。运行描述包含parameters/nativeArgument/required/resultKind/sideEffect、实现/验证状态及实际环境，CLI通用消费。图字段仍由[GraphProtocol](GraphProtocol.md)管理，节点目录只引用，原收据仍由[ResultContractDraft](ResultContractDraft.md)管理。

| 操作 | 业务参数 | 实际处理与完成含义 |
| --- | --- | --- |
| operations.describe | 无 | 当前DLL四项定义和实际工程/引擎/模块SHA；描述不是生成证明 |
| graphs.generateFromFile | inputPath | 同Execute Import；0/waitingPersistence仅生成/编译/保存，仍须独立Verify |
| graphs.preflightFromFile | inputPath | 同Execute GraphPreflight；v2输入/目录/碰撞检查，不预留/构图/保存 |
| graphs.verifySaved | inputPath、generationReport | 同Execute Verify；不同PID、同stage、原收据/输入/资产SHA，实际回读/编译，不Import或resave |

[实际运行描述](../Reports/20261010PluginControl/M4/Operations.json)保持捕获原字节，其内置verificationStatus保守为notRun；同DLL的外部[验收证据](../Reports/20261010PluginControl/M4/OperationEvidence.json)列实际passed。implemented不等于实测通过，不另维护节点目录或第二份方法规则。

## 客户端与边界

真实子命令是describe、execute --operation、status --run-directory。前两项要求显式project/plugin-directory/新run-directory。timeout-seconds默认240，须正有限数，案例180；它是每个UE child的截止，省略description-file会先有一次另行有限describe。OS查询另限10秒，整个命令不等同一个child的timeout。

input/generation-report是业务文件便捷名，param NAME=VALUE通用读取实际描述，未知/重复参数拒绝。description-file仅接受同实际DLL路径/SHA/API；失配返回3。省略它会实际启动只读describe到本run/Describe，不自动安装或猜环境。

工程必须是ValidationProject/RegressionRuns下现有非CandidateRC1 .uproject，只选择指定Reports PluginStage/NodeForge；不回退根工程/插件。启动前查该工程无其它UE进程，调用方按约定串行，不提供并发调度平台。CLI不创建工程、不连接当前面板。input限64KiB，generationReport及客户端读取native报告限1MiB，使用真实字节快照/SHA，原文件不改；图JSON仍由原插件严格解析。

每个run独占新目录，已有run返回2，提示status；同工程目标/request由原插件拒绝，不自动改身份或覆盖。原native报告仅引用path/SHA/退出和原checks，不重写。程序不拼shell/eval/console、不透传任意NFMode。

底层正式参数是-NFOperation、可选-NFControlApiVersion=1.0、-NFReport和定义中的NFInput/NFPrevious。新参数逐token解析，空/缺操作不走Smoke；未知/重复参数、模式混用、版本冲突有具体拒绝。缺报告/写不出时日志有NFCONTROL_RESULT机器JSON；执行/查询stdout是UTF-8单份JSON，help为文本，UE输出留run文件。

## 结果与退出

摘要包含apiVersion/operation/run、已知project/pluginDirectory/moduleSha256/pid、inputPath/inputSnapshot/inputDigest/requestId、executionState、nativeReport引用、validation/errors/warnings/residue/cliExitCode。写前拒绝不伪造PID/requestId，原sessionId/requestId继续复用；原UE退出和CLI退出分别保存。具体业务错误沿用原errorCode、nextAction和failedStage。

| CLI退出 | 含义 |
| --- | --- |
| 0 | 此操作满足原终态；generate仍待Verify，describe不是资产成功 |
| 2 | 参数/文件/操作/新run前提拒绝 |
| 3 | 工程/stage/描述绑定、报告路径或环境阻塞 |
| 4 | 插件业务/验证终态失败或拒绝，保留原错误及残留 |
| 5 | pending或终态unknown，停止后续写入，不自动重发 |

摘要executionState使用rejected/environmentBlocked/finished/pending/unknown；运行事实由Started记录，同步CLI在终态或截止才输出。batch采用passed/failed/notRun。validation引用原生成/编译/保存/持久化checks，固定预期独立对照由Tests/PluginControl负责，runtime/visual另标notRun，不让生成器提供答案。

真实执行后native报告缺失/损坏/超限是5/unknown，保留已知PID/残留未知，不改写成写前拒绝；该损坏/超大分支未另做写入故障注入。文件阻挡报告目录已实测，Windows ACL专项没有改变权限另测。

## status与恢复

run保存Request/Input、NativeStarted/LoadedModule/Exit、Native/Summary及日志。Started包括实际Windows创建ticks；status只读原终态/报告SHA或实际PID+创建时间+命令行工程+模块。PID复用/缺时间/权限不可读不能仅凭PID认旧任务，status不启动UE或改原Summary。

超时保留LiveProcess及活child，不强杀；batch停后例。晚到报告不等于取得退出值，缺原guard Exit仍unknown。本次另以只读句柄观测自然退出，不伪造原NativeExit。历史LiveProcess不代表进程一直活着，也不能沿旧PID猜归属。查询有界，先查旧run/OS/残留，不提供自动cancel/retry/清理或断电回滚。

实际用法见[例子](../Examples/PluginControl/Readme.md)，帮助、stage与逐项验收见[交付记录](../Reports/20261010PluginControl/Readme.md)。
