# 调用记录自动回收

NodeForge v1.1.2 会自动回收已完成调用，普通插件与 AI 接入包须使用同一版本。适用于 Windows 64 位和官方 UE 5.3.2。

## 默认行为

AI、CLI继续正常提交业务，不计次数，不调用清理。原生完成路径在终态记录落盘、共享业务门释放后检查驻留数，达到50条时调用已有calls.releaseCompleted核心，最多回收50条。若遇UI/排队/活动业务，保留记录并在原有编辑器空闲路径重新检查；不增加服务、线程或无限等待。

状态读取、草稿读取、原调用查询和清理自身不占128条业务名额。回收由原生控制会话负责，客户端断开重连或换对话不重置计数、不改变已提交业务。原调用/输入/报告保留，已归档原callId仍可查询，重复提交不重新执行。

## 状态与恢复

editor.getState.callCapacity仍返回retained/available/capacity/reclaimable，新增autoCleanup对象：enabled、threshold、batchSize、completedBatches、status、lastCallId、code。正常连续业务的空闲时点驻留量低于50；忙时可短暂延后，不能据单次快照误判回收失败。

仅回收已确认completed/code0或rejected、cancelled/code2。任何publishing、queued、running、pending、unknown、failed或requiresReview阻止整次回收。归档与磁盘终态不一致、写入失败时停止后续原生业务；自动维护原callId和未知收据保留，idle观察不换编号、不重试写入。按lastCallId查询calls.get，保留原报告，不强制释放或删除锁。

## 高级维护接口

nodeforge_calls_releaseCompleted仍保留，参数为当前handle、新callId和arguments.maxCount（1–128整数），不传expectedDraftRevision，没有force。正常批次无需调用它。满128仍可显式维护，但未解决调用、业务门、会话和归档摘要保护不变。此接口与自动路径调用同一原生ReleaseCompleted。

回收不清空草稿、不生成或绑定资产，不代替编译、保存、独立重开或画面检查。交接仍正常disconnect，见[连接交接](SessionHandoff.md)。
