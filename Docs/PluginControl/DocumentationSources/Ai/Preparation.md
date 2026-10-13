# 操作者准备、唯一身份、注册与 Verify

本页使用真实 PrepareNodeForgeInstallation 与原 NodeForgeControl，不依赖维护者盘符或历史批次。MCP没有安装、启动UE、分配测试台账、退出编辑器或Verify工具；有本机文件/CLI权限的操作者或AI可在明确授权内执行，只有MCP权限的客户端需操作者先完成这些一次性前置。公开包不包含私有工程配置、会话、调用、报告或旧收据。

## 1. 准确绑定与专用注册

按[UserInstallation]({{page:UserInstallation}})设置 installRoot、projectFile、engineRoot、pythonExe，关闭所选工程后执行 bind/check。私有 Config/NodeForgeInstallation.json 精确绑定本机根、工程、唯一插件、实际DLL、引擎和读取目录。InstallationManifest是产品相对路径与字节身份，不是所有工程许可。

```powershell
& $pythonExe "$installRoot/Scripts/PrepareNodeForgeInstallation.py" --action prepare-editor --installation-root $installRoot --project $projectFile --engine-root $engineRoot --python $pythonExe
```

保存返回的PreparationReceipt、Commands路径。 本次私有Preparation Config、Registration、Receipt与Commands按原SHA保留，不改写换版；要准备新会话，由原工具产生新的明确准备记录。可刷新的是已有MCP服务的固定客户端Config，并须核批准的ToolContract/Manifest和本次私有准备身份，不能拿私有Preparation Config充当可编辑客户端配置。本次生成 Registration四字段和MCP十一字段，不复制旧会话身份，也不启动UE或改Codex设置。Commands.mcp提供真实启动 command、逐行 args、cwd；已有客户端保持固定批准Config路径，仅在原owner/call/UE收束后成套核准更新该本机文件。初次安装由操作者把准确字段登记到已有NodeForge服务，不创建第二个相同服务。

操作者显式执行 Commands.editor。先确认本次 SessionPublic.json 的准确PID/创建时间/工程/DLL/实际module和注册目录，再connect获取handle；不能凭“UE窗口出现”判断版本已载入。凭据Session.json由原CLI认证读取，不交给MCP参数或公开页。

## 2. 一次身份与完整输入

普通新任务使用新的 requestId 和 /Game/NodeForgeTests/ 下完整目标；不能沿用本文示例占位、已有目标或历史request。本机授权执行者可产生一个新随机ASCII标识、把本次完整输入/身份保存到本次私有目录，且先核原规则、目标和请求收据；没有预留名称承诺。工程维护验收使用现有台账分配/owner机制，在同一原操作者进程保持到finish；这属于维护者内部流程，不要求普通安装者导入历史测试脚本、工程或台账。

载入草稿前保留完整 UTF-8 原字节、输入SHA及首次调用参数；原输入不覆盖。输入是完整JSON文本，不是源报告片段。载入后draft_get取全文和真实修订；preflight/generate分别保存一个新的callId，各自成功才推进。占用、旧请求、源不干净或未知结果时，不自动换身份/工程重发。

导出使用本次新request/目标，以及默认2或显式formatVersion=3；从原报告 /exportedRequest 完整取得 text/sourceRef。原text作为ExportInput只读保留，严格载入时exportReportPath/exportReportSha256成对传入。明确编辑新副本时retainExportOrigin=true，正式Prepare后取draft_get完整稿，保存为本次实际Input，再预检/生成；不能用旧ExportInput或仅含seed的稿作为最终Verify输入。详[AiAssetRead]({{page:AiAssetRead}})、[AiCopyPreview]({{page:AiCopyPreview}})。

## 3. 原结果、正常结束与新PID Verify

生成后保存原返回，calls_get查同一原callId，report_read取同一原报告。核nativeReport SHA、inputDigest、request/目标、当前工程/DLL/来源文件和原生 checks/readback。native overallStatus=waitingPersistence只证明编译并保存；完整保存文件及本次请求收据须真实存在，未完成独立Verify不可写最终持久化通过。

从 Commands.afterStartup 的真实 command/args/cwd 执行 editor-commands，可读到当前原CLI describe、control.disable和查询同一原stop callId的命令；它只返回命令，不发送业务。stopCallId在初次准备已唯一落盘，开始stop后不能另换ID。由原正式CLI执行control.disable；pending/unknown/stopping先查询同一原call/报告。

通道StopReceipt确认descriptorDeleted、transportHandlesReleased、全部worker完成且无pendingIO后，可执行Commands.normalCloseAfterChannelStop。工具核准确PID/创建时间/当前模块和唯一所选工程窗口，只发送一次正常WM_CLOSE；不点面板、不强杀、不清锁。保存提示或超时仍pending，查原进程，不再次关闭或开始依赖写入。准确关闭后用check或原assert_exclusive确认工程独占。

同stage、原完整Input、原生成报告，用原NodeForgeControl的新进程核验，不再次Import/resave：

```powershell
& $pythonExe "$installRoot/Scripts/NodeForgeControl.py" execute --operation graphs.verifySaved --project $projectFile --plugin-directory '替换为私有绑定中的准确pluginDirectory' --run-directory '替换为本次新Verify目录的绝对路径' --input '替换为本次实际原Input.json绝对路径' --generation-report '替换为本次原生成报告绝对路径' --timeout-seconds 240
```

这些报告目录/Input/报告值取本次原结果，不能凭猜测找历史Reports。CLI未传description-file时从当前DLL读取真实describe；使用描述快照时必须同实际DLL路径/SHA/API。核CLI退出、原native checks.persistence/readback、输入/目标/文件SHA、实际Verify PID与原generation PID不同；运行、画面、打包和包后运行各有自己的证据。

## 4. 完成、恢复与回退

内部台账run只有各自必要项通过且工程无占用才由原owner正常finish；不因接口finished、无错误或额度收尾提前清activeRun。普通安装的绑定撤销按UserInstallation，用原bind Receipt和相同after SHA；外部改动必须保留。原输入、调用、收据、失败、报告及新结果都不改写；桥close只释放自己的空闲lease，不手动删锁。

## 已准备新版后的连接接续

桥0.3.1及以后不要求每次DLL/目录资料升级都手动开关MCP：操作者先查清原call、正常结束旧编辑器，再成套批准原固定Config与新契约/Manifest。下一次docs_index或tools/list由原SDK进程重新核新绑定；返回准确新版本后，用nodeforge_connect取得新handle，再读取当前草稿修订执行授权动作。旧handle、旧报告不能充当新会话证据。此过程不自动开UE、不生成或重发业务，不清外部锁。

如果返回NFMCP_REBIND_BUSY、UNRESOLVED、EDITOR_ACTIVE或SESSION_NOT_CLOSED，接续条件未满足，保留原call/report/PID查询；不能换request重发。configurationRefresh.toolsChanged=true时先取得正式tools/list，按新Schema调用；现代协议没有订阅通知支持时仍可显式发现，不假称客户端缓存已刷新。仅同版文档纠错保持原handle，通常无需重新connect。已实际载入0.3.2及兼容Application ABI的服务可自动载入成套发布的新应用代码；configurationRefresh.applicationCodeReloaded及loadedRuntimeFiles记录执行的代次。旧0.3.1及更早进程首次安装此修复仍需一次真实服务载入；SDK/解释器或Application ABI变更分别保留NFMCP_SDK_RESTART_REQUIRED/NFMCP_APPLICATION_INCOMPATIBLE，不能靠改SHA伪装已载入。详情见[版本发布]({{page:DeveloperVersionRelease}})。

配置提前换版时，原handle仍可查询已登记的原calls.get与report_read，不提交新业务；返回configurationRefreshPending=true表示仍属旧绑定。原Python/SDK/CLI/public身份与报告SHA都须一致，不能猜新报告ID。先收束原unknown和晚到结果，再正常结束旧编辑器，接续新版后显式connect。服务退出不会删除未决记录的owner租约；不能通过重启把原调用当未提交。

## 生成后读取与独立Verify的顺序

显式读取/导出本身不分配生成批次、不预约目标。源须saved/clean，并在当前编辑器由正式读取入口首次受控载入。生成窗里的新资产即使已保存，也没有这种读取证明；sourceMemoryUnproven是已知拒绝，不是unknown，不能清锁或自动reload来规避。

需要导出刚生成的图时，先正常停止并关闭本次自有生成窗，按上面的原输入/收据做新PID Verify；然后仍用同一工程和stage，以新的准确reportDirectory/Registration准备专用读取窗。不要预先手动打开源资产，让正式graphs.export首次受控加载它。核新的SessionPublic、固定Config与handle/revision后，按AiAssetRead取得完整text/sourceRef、显式载入和预检；仅验证导出可用时无需生成第三副本。其它用户窗口不自动关闭，不沿用历史PID或分配。

重开同一版本编辑器时，原Operations快照的取证PID不约束新会话；本版桥仍完整核当前describe.identity和SessionPublic，以及静态接口/schema/限额。旧call/report属于原会话，保留作证据，不能拿新handle重放它们。
