# 结果状态与失败处理

一次工具调用结束、资产保存、重开成功是不同的结果。先看调用状态，再读对应的原生报告。

| 状态或字段 | 含义与下一步 |
| --- | --- |
| `finished` | CLI 调用已结束，继续检查 `cliExitCode`、`nativeSummary` 和报告 |
| `passed` / `succeeded` | 当前操作通过；预检通过不等于已生成资产 |
| `waitingPersistence` | 已完成生成和保存，尚未独立检查重开结果 |
| `notBuilt` | 尚未生成资产，例如刚完成副本准备 |
| `busy` | 编辑器正在执行其他操作，先查询状态 |
| `staleRevision` | 提交的草稿修订已过期，先读取当前草稿并确认变化 |
| `pending` / `unknown` | 调用尚未完成或结果不确定，停止新写入并查询原调用 |
| `requiresReview` | 需要检查现有结果，重新连接不会解除问题 |

生成报告包含 `validation/build/initialReadback/compile/finalReadback/publish/save` 等检查。需要确认保存后重开时，按 [准备与保存核验](Preparation.md)执行 `graphs.verifySaved`。运行效果、材质画面和打包分别检查。

## 查询原调用

保存首次提交的 `callId` 和参数。调用 `nodeforge_calls_get` 查询它，晚到结果仍归属于这次调用。不要用新的 ID、请求或资产名重复提交，也不要把新的草稿修订填进旧请求自动重试。

发生超时、取消、部分写入或报告指纹冲突时，保留输入、报告、资产、收据和进程信息。只读查询本身不能证明没有写入。只有确认原操作未发送或未执行，并处理原因后，才重新提交。

编辑器保留最多 128 次调用，连接服务保留最多 256 份报告；容量不足会停止接受新操作。不要通过清除锁、重连或删除文件来绕过未完成调用。

## 读取报告

从业务返回的 `reports` 取得当前会话的 `reportId`，再向 `nodeforge_report_read` 传入 RFC6901 `pointer`。

- 空字符串选择整个报告。
- `/checks` 查看检查结果。
- `/initialReadback`、`/finalReadback`、`/publishedReadback` 查看实际存在的各次回读。
- `/exportedRequest` 一次取得完整导出文本和来源引用。

报告文件最多 1 MiB，普通选择结果最多 32768 字节。超限会返回 `resultTooLarge`、`selectionReturned=false` 和导航信息；继续读取更小子树，不把摘要当完整内容。`/exportedRequest` 专用分支允许最多 65536 UTF-8 字节的完整 Graph。

报告、源资产、导出文本和输入各有自己的 SHA，不能互换。会话结束后旧 `handle` 和 `reportId` 不用于新连接。

## 来源或版本错误

`sourceNotReady` 表示当前无法确认资源，不等于资源不存在。`sourceMemoryUnproven` 表示已加载对象缺少可验证的保存文件来源，插件不会自动重载或保存它。源文件、原报告或草稿来源冲突时停止依赖操作，按 [资源读取](AssetRead.md)和 [副本编辑](CopyPreview.md)检查。

`NFMCP_FILE_SHA` 表示配置绑定的文件与实际字节不符。使用同一版本的完整包和安装入口修复，不手动改校验值。文档刷新不会清除 `unknown`、`pending` 或现有报告；服务重启也不会自动重放业务。

## 批量调用与容量

v1.1.2 默认自动回收已完成调用，正常使用无需计数或手动清理。pending/unknown 仍先查询原 callId，原报告保留；详见[调用回收](../Reference/CallCapacity.md)。
