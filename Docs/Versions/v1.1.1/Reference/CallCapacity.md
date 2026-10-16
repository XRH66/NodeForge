# 已完成调用的内存回收

NodeForge v1.1.1 / AI 桥 0.4.4。普通插件和 AI 接入包必须配套升级；仅更新磁盘文件不会改变运行中编辑器加载的版本。

UE 的业务调用表最多保存 128 条驻留记录。载入草稿、预检、规则查询分别计一次；已完成记录需要主动回收。断开连接只释放租约，删除磁盘报告不会释放调用名额。

## 调用方法

```json
{"handle":"当前客户端实际取得的句柄","callId":"BatchCleanup-01","arguments":{"maxCount":50}}
```

工具名为 `nodeforge_calls_releaseCompleted`，原生操作名为 `calls.releaseCompleted`。`maxCount` 是 1–128 的整数；不传 `expectedDraftRevision`，不支持 `force`。CLI 使用原 `NodeForgeControl.py editor-call`，提供实际工程、插件、会话、新运行目录及 `--operation calls.releaseCompleted --call-id BatchCleanup-01 --arguments-file <参数文件>`；参数文件内容为 `{"maxCount":50}`。

该维护操作不占业务名额，满 128 条时仍能执行。每累计 50 次业务调用，等待原调用终态后回收，检查结果再继续。一份材质的载入和预检占两次，通常 25 份就达到 50 次；恢复草稿也计一次。状态读取、草稿读取、原调用查询和回收自身不占这些名额。本版提供主动回收接口，未增加自动回收策略。

## 成功与保护

确认返回 `status=finished`、`cliExitCode=0`，再读 `nativeSummary.response.result.result` 中的 `released`、`retained`、`available`、`capacity`、`archivedCallIds`、`recordsPreserved`。不足 maxCount 时按实际可回收数返回。`editor.getState.callCapacity` 同时提供 `retained/available/capacity/reclaimable`，用于核对当前真实计数。

仅回收已确认 completed/code0 或 rejected、cancelled/code2 的记录。存在 queued、running、publishing、pending、unknown 或未收尾失败时整次拒绝；共享业务门也排除正在执行的 UI/CLI 业务。未退出的 CLI 和未解决的 MCP 原调用会阻止回收。遇 pending/unknown，只通过 `nodeforge_calls_get` 查询原 callId，包括回收本身的编号，不换 ID 重发。

原 Accepted、Finished、输入和报告继续保留。回收前检查会话、参数指纹和磁盘终态，写入归档摘要引用后释放内存记录。原 callId 仍可查询；同编号同参数返回原结果，不重新执行，不同参数拒绝 callConflict。归档损坏或缺失明确失败，不把旧编号当新业务。原调用读取限于其控制会话；重开编辑器后通过原会话的磁盘证据保留历史，不能冒用新会话查询旧编号。

回收不生成资产、不清空草稿，也不替代编译、保存、独立重开或画面检查。连接交接见[正常释放连接](SessionHandoff.md)，未知结果处理见[结果与恢复](../Ai/ResultsAndRecovery.md)。
