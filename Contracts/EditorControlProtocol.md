# 编辑器控制协议

本协议用于操作已经运行的 UE 编辑器。适用 NodeForge v1.0.0，宿主为 `activeEditor`，`editorApiVersion="1.0"`、`profile="nodeforge.editor.v1"`，消息使用 JSON-RPC 2.0。

建议通过[AI 安装工具](../Docs/AiInstallation.md)准备连接，或调用 [NodeForgeControl.py](../Scripts/NodeForgeControl.py)。独立命令行生成与保存检查见[CLI 协议](ControlProtocol.md)。

## 建立连接

仅普通插件不开放控制通道。AI 一次明确启用后，正常启动、安装绑定就绪时由同一原生入口创建本工程新注册和通道；面板连接/停止调用同一入口。工程状态在 `Saved/NodeForge`，发现索引在当前用户 `NodeForge`，凭据仍只在原受保护私有会话内。程序目录不保存运行记录。每次会话独立，不需要日常启动参数。

原高级准备器仍可生成显式注册并使用 `-NFControlEnable -NFControlRegistration=<注册文件绝对路径> -NoLiveCoding`，其身份和未知结果保障不变。日常 AI 通道启用时仅本编辑器会话禁用 Live Coding；重新启用或加载补丁会停止通道，不放宽 DLL 身份。

注册对象恰含四个字符串字段：`project/pluginDirectory/binarySha256/reportDirectory`。它与工程的安装绑定共同指定工程、插件、DLL 和新报告目录。已存在 `SessionPublic.json` 的报告目录不能再次启用；新会话需要新目录。

`SessionPublic.json` 包含无秘密的连接信息：`pid/processStart/project/module/binarySha256/controlSessionId/pipe/sessionFile/reportRoot/engine/limits`。客户端核对进程创建时间、工程和已加载模块，再完成握手。32 字节随机凭据仅保存在受保护的私有 Session 文件并用于认证，不应复制到命令行、普通报告或客户端配置中。

CLI 显式提供 `--session-file --project --plugin-directory --run-directory`，不会挑选第一个 UE 进程或自行创建工程。已结束会话的连接文件不能用于新进程。

## 可用方法

精确参数类型、范围和条件见 [Operations.json](../Docs/Versions/v1.0.0/Operations.json)。额外或大小写不符的字段会被拒绝。

| 方法 | arguments | expectedDraftRevision | 结果类型 |
| --- | --- | --- | --- |
| `operations.describe` | `{}` | 不得提供 | `capabilityDescription` |
| `session.hello` | `editorApiVersion`: string；`profile`: string；`controlSessionId`: string；`expectedProject`: string；`expectedProcessStart`: string；`expectedBinarySha256`: string；`credential`: string；`expectedPid`: integer | 不得提供 | `sessionIdentity` |
| `editor.getState` | `{}` | 不得提供 | `publishedState` |
| `draft.get` | `{}` | 不得提供 | `publishedDraft` |
| `draft.loadFile` | `path`: string；`exportReportPath`: string（可选）；`exportReportSha256`: string（可选） | 必需 | `draftReceipt` |
| `draft.replace` | `text`: string；`exportReportPath`: string（可选）；`exportReportSha256`: string（可选）；`retainExportOrigin`: boolean（可选） | 必需 | `draftReceipt` |
| `draft.restore` | `{}` | 必需 | `draftReceipt` |
| `draft.prepareCopy` | `{}` | 必需 | `copyPreparationReceipt` |
| `graphs.preview` | `{}` | 必需 | `graphPreviewReceipt` |
| `graphs.preflight` | `{}` | 必需 | `preflightReceipt` |
| `graphs.generate` | `{}` | 必需 | `generationReceipt` |
| `rules.query` | `assetKind`: Material/Blueprint；`query`: string | 必需 | `rulesReceipt` |
| `catalog.get` | `{}` | 必需 | `catalogReceipt` |
| `resources.inspect` | `kind`: MaterialParameters/MaterialFunction/Texture2D；`source`: string | 必需 | `resourceReceipt` |
| `resources.query` | `kind`: Texture2D/MaterialFunction/Material；`directory`: string；`nameContains`: string（可选）；`recursive`: boolean（可选）；`limit`: integer（可选）；`after`: string（可选） | 必需 | `resourceQueryReceipt` |
| `graphs.export` | `assetKind`: Material/Blueprint；`source`: string；`requestId`: string；`assetPath`: string；`formatVersion`: 2/3（可选） | 必需 | `exportReceipt` |
| `calls.get` | `callId`: string | 不得提供 | `callRecord` |
| `control.disable` | `{}` | 不得提供 | `stoppingRequested` |

`session.hello` 由客户端内部握手使用，普通 editor-call 不接受它。`expectedPid` 为非零 uint32 整数，其他握手身份字段为字符串，credential 由安全客户端读取。

v1.0.0 握手可选 `clientId/clientName/clientVersion`，三项必须同时提供或同时省略。clientId 为32位小写十六进制字符串，clientName 为1–80字符，clientVersion 为0–80字符；普通 CLI 可省略。通过认证的客户端信息进入 `editor.getState.clients` 的 `id/name/version`，不包含凭据或新业务权限。面板的 Codex 定向断开与整个服务停止分开，详情见 [MCP 客户端状态](McpProtocol.md#原生面板中的客户端状态)。

普通参数由 `context` 和 `arguments` 两个对象组成。context 包含 `controlSessionId/editorApiVersion/profile`；需要修订号的方法再包含 `expectedDraftRevision`。修订号和 Windows 创建时间均为规范十进制 uint64 字符串，不能带前导零、符号或溢出。

RPC id 使用 1–64 个 ASCII 字母、数字或连字符。游戏线程方法将它登记为 callId。查询调用记录时，RPC id 和 `arguments.callId` 可以不同；`controlSessionId`、原生 sessionId 和图 requestId 也各有用途。

## 草稿操作

状态与草稿读取使用缓存，草稿与插件面板共用同一服务。相同全文不会增加修订；恢复只支持一步。超限草稿只返回元数据，不会截断并冒充全文。UI 和 API 共用一个业务槽。

`draft.loadFile` 只载入，路径必须位于安装配置允许的输入目录，不能使用私有会话文件或重解析点。文件按安全句柄核对并读取，最多 64 KiB。`draft.replace` 接受非空、有界的全文，来源记为 program。

修改草稿后，先读取新修订，再进行预检或生成。预检不预留目标、不构图或保存。生成使用当前草稿的字节快照，成功保存后仍可能返回 waitingPersistence；独立保存检查使用 CLI 的 `graphs.verifySaved`，没有同进程 Verify 方法。

## 导出来源与新副本

`graphs.export` 读取普通材质或 Actor 蓝图，生成完整请求和来源报告。source 必须是允许读取范围内已保存、无未保存修改的准确 `Package.Asset` 路径；蓝图还需要有效的编译结果。完整导出不能丢掉不支持的节点或属性。

首次载入导出内容时，`exportReportPath/exportReportSha256` 必须成对提供。服务核对报告字节、当前工程与插件、源文件、图类型、目录及导出内容。首次载入内容须与导出语义一致；后续预检和生成会再次检查源文件与直接依赖。

需要编辑已绑定来源的草稿时，使用 `draft.replace(retainExportOrigin=true)`，并且不再提供 exportReport 字段。没有可信来源时返回 `exportOriginRequired`；失败保留原稿。普通替换会清除不相关的旧来源。

`draft.prepareCopy` 支持 v2/v3 的材质与蓝图，成功只更改请求编号和目标路径，保留父目录和图内容。完整新稿从 draft.get 读取。返回的 `draftApplied`、前后修订号和 copyPreparation 说明是否真正提交到草稿；报告写出不代表草稿提交完成。准备不生成资产或预留名称。

`graphs.preview` 使用当前已绑定的来源，比较结构、属性值和布局。它不接受任意 previousReport，不修改草稿或资产，也不证明编译和运行效果相同。输入变化后旧预览失效。

## 规则、目录与资源

`rules.query` 在真实类名、函数 owner/member 和变量类型中做不区分大小写的 OR 子串搜索。query 最多 256 字符、8 个词，空 query 返回所选类型的规则。无匹配返回明确结果，不借用上一次查询数据。`catalog.get` 返回完整目录报告；生成请求应使用完整目录的 catalogDigest，不能对查询子集重算摘要。

`resources.query` 的种类、分页和遍历上限见[CLI 协议](ControlProtocol.md#资源查询)。查询不改变草稿、来源或成功输入。`complete=false` 表示查询未完整结束，应缩小目录后处理，不能把它当作完整空结果。

`resources.inspect` 读取材质参数、材质函数或 Texture2D。返回实际资源类型、保存状态、文件摘要以及相应参数、引脚或采样信息。列表中的发现状态不等于检查成功；已加载但没有可信保存来源的对象返回 `sourceMemoryUnproven`，状态未就绪时返回 `sourceNotReady`，不会自动重载或保存。

查询结果通过报告路径、SHA、字节数和目录摘要返回。规则报告使用其实际 operationStatus；目录报告按自身格式处理，不添加不存在的生成成功字段。

## 传输与限额

Windows 本地字节管道以 4 字节小端长度开头，后接一个 UTF-8 JSON 对象。解析检查重复键、深度、长度和类型。不支持 batch、notification、公网连接、任意 console/代码执行或 DLL 热替换。管道只允许当前用户和 SYSTEM，拒绝远程连接并核对服务端 PID。

| 限额 | 值 |
| --- | --- |
| frameBytes | 524288 |
| graphBytes | 65536 |
| reportBytes | 1048576 |
| descriptorBytes | 65536 |
| jsonDepth | 32 |
| connections | 4 |
| businessSlots | 1 |
| retainedCalls | 128 |
| helloSeconds / frameSeconds | 5 / 10 |
| callSeconds / querySeconds | 180 / 5 |
| idleSeconds / stopSeconds / exitSeconds | 60 / 10 / 60 |

各超时独立计时，部分数据到达不会无限延后截止。CLI 活动调用总等待默认 180 秒，但截止不会强制中断 UE 正在执行的同步编译或保存。调用记录保留到会话结束，达到 128 条后不会自动淘汰或复用编号。

## 状态、失败和关闭

调用只有在写入 Accepted 记录并占用业务槽后才被受理。相同 callId 和参数返回原记录，同一编号改参数返回 callConflict。执行前再次检查会话、关闭状态、busy 和草稿修订。

调用状态包括 `accepted/queued/running/completed/rejected/failed/cancelled/unknown`。accepted 仅表示已受理。CLI 只读轮询原调用直至终态或截止；断连与超时不撤销操作。结果未知时，先查原 callId、报告和可能的部分写入，不换编号重发。

CLI 使用 `editor-describe`、`editor-call --operation ... --call-id ...` 和 `editor-status --call-id ...`。需要修订号时提供 `--expected-revision`，参数对象通过 `--arguments-file` 指定。文件须位于允许的目录，JSON boolean 不能替代 integer。

退出码 0 表示该操作取得约定结果；2 为参数、修订或写入前拒绝，3 为身份/权限/环境阻塞，4 为业务失败，5 为尚未结束或结果未知。生成返回 0 仍需要按生成报告处理 waitingPersistence。

`control.disable` 先返回 stoppingRequested。只有 StopReceipt、管道消失、私有描述文件删除、所有工作线程完成、pendingIo=false 且取消操作完成后，客户端才确认 stopped。应先结束原调用、停止通道，再正常关闭对应编辑器。当前不提供公开 enable/restart、自动重试、强制取消或覆盖资产接口。
