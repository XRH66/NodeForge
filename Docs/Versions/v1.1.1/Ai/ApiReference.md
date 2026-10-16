# API 参考

本参考列出 NodeForge v1.1.1 的 MCP 工具、参数和返回 Schema。先按 [调用流程](QuickStart.md)连接，再按任务查阅：

| 文档 | 工具 |
| --- | --- |
| [状态与草稿](AiApiTools1.md) | 操作定义、工程状态、草稿读取、文件载入与文本替换 |
| [副本与生成](AiApiTools2.md) | 草稿恢复、副本准备、差异预览、预检与生成 |
| [规则与连接](AiApiTools3.md) | 规则、目录、调用查询、连接与文档索引 |
| [文档、报告与资源](AiApiTools4.md) | 文档读取、报告读取、资源检查、图导出与资源查询 |

`handle` 标识当前连接，`callId` 标识一次操作，`requestId/assetPath` 标识生成请求与目标，`reportId` 标识当前会话登记的报告。它们不能互换。

业务返回保留 `cliExitCode`、`nativeSummary`、`reports` 和 `unresolvedCalls`，错误码位于 `errors.code`。结果状态的处理方法见 [结果与恢复](ResultsAndRecovery.md)。

`report_read` 的 `/exportedRequest` 返回完整 `text/value/payloadSha256/sourceRef`，Graph 文本最多 65536 UTF-8 字节。其他子树通常最多 32768 字节。将导出载入草稿时，只把 `sourceRef` 中的 `exportReportPath` 和 `exportReportSha256` 传入对应参数，不要把整个 `sourceRef` 塞进 Graph，详见 [导出与载入](AssetRead.md)。

## 客户端身份与只读在线状态

真实 SDK 初始化 client_info 由原 transport 在 session.hello 中提供成组可选 clientId/clientName/clientVersion；三项同给或同省略。clientId 为32位小写十六进制，名称1–80字符，版本0–80字符；普通 CLI 可省略。认证后的 editor.getState.clients 返回 id/name/version，不含凭据。Codex presence 不占业务槽；4个有界管道连接与1个业务槽分别限制。选择不是连接，定向断开只关闭本工程匹配的客户端连接，查询原调用/报告仍保留。Claude/DeepSeek 仅框架、配置和未检测状态。

## 批量调用与容量

v1.1.1 新增 `nodeforge_calls_releaseCompleted`，每 50 次业务调用主动回收已确认终态的记录，再核对可用名额继续。pending/unknown 先查原 callId，原报告保留；详见[调用回收](../Reference/CallCapacity.md)。
