# API 参考

本参考列出 NodeForge v0.2.0 的 MCP 工具、参数和返回 Schema。先按 [调用流程](QuickStart.md)连接，再按任务查阅：

| 文档 | 工具 |
| --- | --- |
| [状态与草稿](AiApiTools1.md) | 操作定义、工程状态、草稿读取、文件载入与文本替换 |
| [副本与生成](AiApiTools2.md) | 草稿恢复、副本准备、差异预览、预检与生成 |
| [规则与连接](AiApiTools3.md) | 规则、目录、调用查询、连接与文档索引 |
| [文档、报告与资源](AiApiTools4.md) | 文档读取、报告读取、资源检查、图导出与资源查询 |

`handle` 标识当前连接，`callId` 标识一次操作，`requestId/assetPath` 标识生成请求与目标，`reportId` 标识当前会话登记的报告。它们不能互换。

业务返回保留 `cliExitCode`、`nativeSummary`、`reports` 和 `unresolvedCalls`，错误码位于 `errors.code`。结果状态的处理方法见 [结果与恢复](ResultsAndRecovery.md)。

`report_read` 的 `/exportedRequest` 返回完整 `text/value/payloadSha256/sourceRef`，Graph 文本最多 65536 UTF-8 字节。其他子树通常最多 32768 字节。将导出载入草稿时，只把 `sourceRef` 中的 `exportReportPath` 和 `exportReportSha256` 传入对应参数，不要把整个 `sourceRef` 塞进 Graph，详见 [导出与载入](AssetRead.md)。
