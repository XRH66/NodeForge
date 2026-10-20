# AI 调用流程

本页描述安装后的常用调用顺序。尚未安装时，先完成 [安装与连接](../User/AiInstallation.md)。

## 1. 读取说明

调用 `nodeforge_docs_index`，检查当前版本；随后用 `nodeforge_docs_read` 读取 `Readme`、`FeatureMap` 和需要的节点说明。资料可以离线读取，无需编辑器连接。

## 2. 连接与读取草稿

正常打开已启用 NodeForge 的工程，调用 `nodeforge_connect({})`。唯一合格会话返回 `connected` 和 `handle`；零候选返回 `notConnected`，此时等工程就绪；多个候选返回 `selectionRequired` 与 `candidates`，显式调用 `nodeforge_connect({"project":"实际 .uproject 绝对路径"})`，或传候选中的 `controlSessionId`。只对 `connected` 使用返回的 `handle`；关闭重开后重新连接，旧句柄不会改指另一会话。后续调用使用这个返回值，不自行构造。依次调用 `nodeforge_operations_describe`、`nodeforge_editor_getState` 和 `nodeforge_draft_get`；没有额外参数时也传 `arguments:{}`。

从 `draft_get` 返回的 `nativeSummary.response.result` 读取 `draftRevision`。它是十进制字符串，用于确认接下来修改的是哪一版草稿。

## 3. 准备 JSON

先通过 `nodeforge_rules_query` 和 `nodeforge_catalog_get` 查询所需节点。参考 [完整示例](../Reference/Examples.md)编写图，使用未占用的 `requestId` 和目标路径，以及当前 `catalogDigest`。

用 `nodeforge_draft_replace` 载入完整 JSON 字符串，或用 `nodeforge_draft_loadFile` 载入允许目录内的文件。每次新操作先保存自己的 `callId`，并传入当前 `expectedDraftRevision`。加载后再次读取草稿，取得新修订。

## 4. 预检与生成

对当前草稿调用 `nodeforge_graphs_preflight`，检查返回报告中的错误。通过后再调用 `nodeforge_graphs_generate`；两个操作分别使用自己的 `callId`，`arguments` 均为 `{}`。

生成会新建资产，不覆盖已有目标。调用返回后保存结果，通过原 `callId` 查询状态，并读取登记报告。`finished` 表示调用结束；`waitingPersistence` 表示已保存但尚未独立检查重开结果。具体检查方法见 [准备与保存核验](Preparation.md)。

## 5. 遇到问题

`pending` 或 `unknown` 时停止后续写入，通过 `nodeforge_calls_get` 查询原调用。不要换一个调用 ID、请求 ID 或目标名称再次生成。`staleRevision` 时先读取当前草稿，确认修改来源；详见 [结果与恢复](ResultsAndRecovery.md)。

## 接下来可以做什么

- [读取已有资源并导出图](AssetRead.md)
- [修改图的副本并查看差异](CopyPreview.md)
- [使用 JSON v3 描述材质、蓝图和实例](JsonArchitectureExamples.md)
- [生成随时间移动的 Actor](BlueprintTick.md)

新输入推荐 v3。`graphs.export` 显式传入 `formatVersion:3` 时返回 v3，省略时返回 v2；副本准备、预览和重排保持当前草稿的格式。


## 1.1.2 清漆与折射契约

当前新增字段、根输入类型、切线空间和 CLI 进程查询见[完整契约](../Reference/LayeredMaterials.md)。新 Catalog/Schema 来自原生共享定义；旧输入需重新预检。

## 批量调用与容量

v1.1.2 默认自动回收已完成调用，正常使用无需计数或手动清理。pending/unknown 仍先查询原 callId，原报告保留；详见[调用回收](../Reference/CallCapacity.md)。

## 在对话之间交接

本对话使用完后，先确认原调用无pending/unknown，再调用nodeforge_disconnect({"handle":"本对话当前handle"})。只有released/leaseReleased/handleInvalidated都为true才算释放；下一对话显式connect取得自己的新handle。UE与AI服务保持运行，原调用/报告保留。没有force参数。完整规则见[连接交接](../Reference/SessionHandoff.md)。
