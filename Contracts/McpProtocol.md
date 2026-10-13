# MCP 接口

NodeForge 的 AI 接入通过本地 stdio MCP 服务调用插件。v0.1.0 的 AI 接入包包含固定 Python 环境、MCP SDK 和连接工具；安装步骤见[连接 AI](../Docs/AiInstallation.md)。

服务提供 20 个工具，其中 16 个对应编辑器业务操作，另外 4 个用于连接、文档和报告读取。精确参数与返回结构见[ToolContract.json](../Releases/0.1.0/ToolContract.json)，逐个工具的说明见[API 参考](../Docs/Versions/v0.1.0/Ai/ApiReference.md)。

## 工具与调用顺序

1. 使用 `nodeforge_docs_index` 和 `nodeforge_docs_read` 阅读本版本说明。
2. 调用 `nodeforge_connect {}`，取得当前连接的 `handle`。
3. 读取 `nodeforge_editor_getState` 和 `nodeforge_draft_get`。
4. 查询规则或资源，载入完整 JSON，使用返回的最新草稿修订进行预检。
5. 确认目标后生成，通过 `nodeforge_calls_get` 查询原调用，用 `nodeforge_report_read` 查看报告。

| 工具组 | 工具 |
| --- | --- |
| 描述与状态 | `nodeforge_operations_describe`、`nodeforge_editor_getState`、`nodeforge_draft_get` |
| 草稿 | `nodeforge_draft_loadFile`、`nodeforge_draft_replace`、`nodeforge_draft_restore`、`nodeforge_draft_prepareCopy` |
| 图操作 | `nodeforge_graphs_preview`、`nodeforge_graphs_preflight`、`nodeforge_graphs_generate`、`nodeforge_graphs_export` |
| 规则与目录 | `nodeforge_rules_query`、`nodeforge_catalog_get` |
| 资源 | `nodeforge_resources_query`、`nodeforge_resources_inspect` |
| 调用记录 | `nodeforge_calls_get` |
| 连接和资料 | `nodeforge_connect`、`nodeforge_docs_index`、`nodeforge_docs_read`、`nodeforge_report_read` |

业务工具接收 `handle` 和 `arguments`，无参数时也须提供 `{}`。游戏线程方法还需要新 `callId`（1–64 个 ASCII 字母、数字或连字符）和规范 uint64 字符串 `expectedDraftRevision`。MCP 请求编号、插件 callId、图 requestId 各有用途，不能互换。

工具不接受任意 method、操作系统命令、console 或代码执行。握手 `session.hello` 由客户端内部处理，`control.disable` 由安装与管理工具使用，不是默认 AI 工具。新增节点仍通过同一套图操作工具调用。

## 服务与配置

stdio 标准输出只传输 MCP 消息，诊断写入标准错误。SDK 负责协议协商和消息封装。Python、SDK 与依赖的版本和 SHA 随 AI 包固定，见[依赖锁](../Tools/NodeForgeMcp/Requirements.lock)及[依赖清单](../Tools/NodeForgeMcp/DependencyManifest.json)。

安装工具生成 `NodeForgeMcpConfig` version 1，包含 `bundleId/project/pluginDirectory/binarySha256/sessionPublic/reportRoot/bridgeRunRoot/toolContractSha256/docsManifestSha256` 等精确绑定，路径均为绝对路径。配置不包含会话凭据。AI 业务参数不能任意指定工程、PID、私有凭据文件或脚本。

连接服务本身不启动或停止 UE，也不执行独立 Verify；这些由安装及命令行工具完成。没有连接时仍可读文档，业务工具明确返回未连接。

connect 通过安全客户端核对公开会话、PID、创建时间、管道、模块、SHA 和操作定义，返回随机 handle 与无秘密身份。handle 只是连接引用，不代替这些身份检查。不同会话会使旧 handle 和报告编号失效，服务不会自动重连并重放业务。

多个客户端可以读取同一份文档和工具列表，但业务连接受排他锁约束。已有业务所有者时返回 `NFMCP_BUSY`，文档工具仍可用。不能删除锁文件来绕过正在运行或尚未查清结果的调用。

## 读取资源和导出图

resources_query 查询 Texture2D、MaterialFunction、Material 的元数据；resources_inspect 读取实际 Texture2D、材质参数或材质函数。查询结果不自动证明资源能够引用或导出，精确资源检查仍有类型、保存状态、无未保存修改、允许目录和文件摘要等要求。

graphs_export 支持 Material 和 Blueprint，需 `assetKind/source/requestId/assetPath`，可选 `formatVersion=2|3`，默认 2。导出不修改原资产或草稿。

成功报告包含 `exportedRequest`、`exportedRequestText`、`exportedRequestSha256` 和来源信息。读取 `/exportedRequest` 可取得完整 value、原始 text、payloadSha256 和 sourceRef。sourceRef 中的报告 SHA、源资产 SHA、内容 SHA 各不相同，不能互换。

初次载入导出内容时，draft_loadFile 或 draft_replace 必须成对提供 `exportReportPath/exportReportSha256`，内容须与导出语义一致。继续编辑当前已绑定来源时使用 `retainExportOrigin=true`，且不附新的来源字段。预检与生成会再次检查源文件。

prepareCopy 仅修改请求和目标，返回 draftApplied 与新修订，完整内容从 draft_get 读取。preview 只比较当前来源和草稿的结构、数值与布局，不创建资产，也不证明运行效果相同。

## 文档和报告容量

| 工具 | 参数与限制 |
| --- | --- |
| docs_read | `pageId` 为 1–128 字符，只能读取清单中的页面；单页最多 65,536 UTF-8 字节 |
| docs_index | 返回版本、能力和阅读入口；最多 32,768 UTF-8 字节 |
| report_read | `handle/reportId/pointer`；reportId 为已登记的 64 位十六进制标识，pointer 最长 1,024 字符 |

report_read 使用 RFC6901 JSON Pointer，空字符串选择根对象。工具只读取本会话已登记且路径、大小、SHA 均匹配的报告；不能传任意文件路径或读取私有 Session。

原报告最多 1 MiB，普通选择结果最多 32 KiB。超限返回 `resultTooLarge`、`selectionReturned=false` 及导航信息，不截断内容。`/exportedRequest` 是完整图的专用入口，最多 64 KiB。报告标识最多登记 256 项，不自动淘汰后复用。

## 状态与中断恢复

返回同时包含 TextContent 和等值 structuredContent，必含 `bridgeVersion/status`，业务返回保留 `cliExitCode/executionState/operation/callId/controlSessionId/response` 及原报告引用。不要把 MCP 成功响应统一理解为资产生成成功。

参数错误返回 rejected，身份、文件或版本不匹配返回 environmentBlocked。已经提交后出现超时、断连、损坏输出或报告不符时，可能返回 pending/unknown；这不表示没有发生写入。

结果未知时，停止新的游戏线程写入，只查询原 callId、状态、草稿和报告。不要更换编号重发，也不要自动把修订号改为最新值重试。客户端取消等待不会撤销 UE 保存；服务会收集已经开始的 CLI 结果，之后仍需查清原调用。

生成的 waitingPersistence 表示构建、编译和保存已完成，独立保存检查未完成。正常关闭对应控制通道和编辑器后，再用同一插件、原输入与生成报告执行 CLI `graphs.verifySaved`。运行画面和打包效果另行检查。

## 资料更新与服务更新

每次读取资料时，服务从固定配置路径检查文档清单。仅文档 SHA 变化，且工程、插件、API、目录、工具和运行依赖均保持匹配时，可以刷新资料而保留当前连接、调用记录和未决状态。未经配置批准的文件变化会被拒绝。

核心或配置变化时，需要没有在途请求、未收束 CLI 或不明结果，并确认旧编辑器与会话正常结束，再绑定新配置。旧 handle 失效，客户端随后重新 connect；服务不会自动重放调用。

兼容的应用代码更新可在相同 SDK 进程内加载，但须核对配置、契约、清单和所有相关源码 SHA。仅修复连接服务、且核心/公开会话/CLI/协议/限额均相同的情况，可保留原 handle 与调用记录，让新代码查询原调用；未知结果未查清前仍不能开始新写入。

Python、SDK 或应用 ABI 不兼容时需要重新加载服务。`NFMCP_RUNTIME_RESTART_REQUIRED`、`NFMCP_SDK_RESTART_REQUIRED`、`NFMCP_APPLICATION_INCOMPATIBLE` 会指出相应条件；不能仅修改版本文字宣称运行进程已经升级。

旧业务进程退出后的锁恢复，需要检查原 PID 确已消失、当前会话有效，并逐个查询磁盘记录中的原调用终态。活动所有者、未知身份、未决调用或文件竞争都会停止恢复。原报告和调用文件保留，不通过清锁掩盖未完成操作。
