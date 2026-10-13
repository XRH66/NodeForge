# 编辑已有图并生成副本

这个流程读取已保存的材质或 Actor 蓝图，修改 JSON 草稿，查看差异，再生成独立的新资产。源资产保持不变。

支持范围是当前目录能完整表达的普通 Material，以及 Actor 父类的单 EventGraph Blueprint。材质实例、函数整图、多图、新父类、循环、数组和原地覆盖不在此路线内。先阅读 [连接准备](Preparation.md)与 [资源读取](AssetRead.md)。

## 1. 连接并选择源图

调用 `nodeforge_connect`，取得当前 handle；读取 `operations_describe`、`editor_getState` 和 `draft_get`。从 `nativeSummary.response.result` 取得当前 draftRevision。

确定资产类型、精确 source 对象路径，以及未占用的导出 requestId 和 assetPath。每次操作先保存自己的 callId，后续查询仍使用原 ID。以下片段是参数构造示例，变量使用这次实际返回值；每次草稿变化后都要更新 current_revision。

源必须已保存且未修改，Blueprint 需要可用 GeneratedClass 和编译状态。`sourceMemoryUnproven` 表示已加载对象缺少可确认的保存文件来源；`sourceNotReady` 表示当前无法确认对象。不要自动重载、保存或编译源来绕过错误。

## 2. 导出并取得完整文本

向 `nodeforge_graphs_export` 传入：

```python
arguments = {
    "handle": current_handle,
    "callId": export_call_id,
    "expectedDraftRevision": current_revision,
    "arguments": {
        "assetKind": kind,
        "source": source,
        "requestId": new_request_id,
        "assetPath": new_asset_path,
        "formatVersion": 3,
    },
}
```

导出不会改变草稿或预留目标。检查原生报告中的 `operationStatus=passed`、`overallStatus=succeeded`，然后从 reports 取实际 reportId，用 `nodeforge_report_read` 读取 `/exportedRequest`。将返回记为 P。

| 字段 | 用途 |
| --- | --- |
| `P.value` | 完整 Graph 对象 |
| `P.text` | 原生序列化的完整 UTF-8 文本 |
| `P.payloadSha256` | 文本字节的 SHA |
| `P.sourceRef.exportReportPath`、`exportReportSha256` | 原报告路径与 SHA，必须成对使用 |
| `P.sourceRef.source`、`sourceAssetSha256` | 源对象与源文件 SHA |

Graph 文本最多 65536 UTF-8 字节，报告最多 1 MiB。普通 pointer 结果最多 32768 字节。保留 P.text 原文，不由摘要或节点片段重建来源文本。

## 3. 先载入，再编辑

首次 `nodeforge_draft_replace` 载入完整来源：

```python
arguments = {
    "handle": current_handle,
    "callId": load_call_id,
    "expectedDraftRevision": current_revision,
    "arguments": {
        "text": P["text"],
        "exportReportPath": P["sourceRef"]["exportReportPath"],
        "exportReportSha256": P["sourceRef"]["exportReportSha256"],
    },
}
```

首次载入保持图的语义不变，不能提前修改参数或目标。也可以把文本保存为允许目录内的 JSON 文件，用 `draft_loadFile` 和同一对报告字段载入。

载入成功后 `draft_get` 应返回 `draftReadable=true`。读取完整 text、digest、draftRevision 和 exportOrigin，再根据用户要求编辑这份图。例如，修改 ScalarParameter 的 DefaultValue 时，保留其他节点、连接与设置；修改字符串函数的输入时，先检查该输入是否实际参与连接和计算。

编辑后的完整 JSON 字符串记为 edited_text，调用 `nodeforge_draft_replace`：

```python
arguments = {
    "handle": current_handle,
    "callId": edit_call_id,
    "expectedDraftRevision": current_revision,
    "arguments": {
        "text": edited_text,
        "retainExportOrigin": True,
    },
}
```

`retainExportOrigin` 必须是 boolean。true 不能同时传入新的 exportReportPath 或 exportReportSha256；它使用当前草稿已绑定的来源，并重新检查源和报告。省略或 false 不表示保留来源。来源不存在时会拒绝。

实际文本变化会增加修订号，使旧检查失效；相同文本不增加修订。成功后再次读取完整草稿，确认修改内容和来源关系。

## 4. 准备新副本

用新 callId、当前修订与 `arguments:{}` 调用 `nodeforge_draft_prepareCopy`。工具生成新的 requestId 和 assetPath，保留图内容，并提交新草稿；它会写准备记录，但不预留名称或生成资产。

将返回的 `nativeSummary.response.result` 记为 C，其中 `result` 记为 R，检查：

1. 调用已经结束，原生业务成功，mode 为 `PrepareNewCopy`。
2. `R.draftApplied=true`，previousDraftRevision 与提交时一致，newDraftRevision 是提交后的修订。
3. `R.copyPreparation` 与 nativeReport 一致，sourceInputDigest 对应编辑稿。
4. `sourceInputUnchanged=true`、`targetReserved=false`、`assetGenerated=false`。
5. 读取实际 newRequestId/newAssetPath、preparedInputPath/preparedInputDigest。

随后调用 `draft_get`。完整草稿 digest 应等于 preparedInputDigest，修订应等于 newDraftRevision，并保留 exportOrigin。除 requestId 和 assetPath 外，内容应与准备前一致。

工具可能生成以 Copy 加 GUID 组成的请求 ID，它符合 64 字符上限，不要手工缩短或替换。后续所有操作使用这个实际新目标。

要保留生成输入，把这次 draft_get 的完整 text 按原 UTF-8 字节保存到允许目录内的新文件，不加 BOM 或尾部换行，并核对 SHA 与 digest 一致。不要覆盖此前导出的原文。

准备报告存在但 draftApplied=false 时不能继续。unknown 时草稿可能已经提交，应查询原调用和当前草稿，不要再准备一次。

## 5. 预览差异

用新 callId、当前修订与 `arguments:{}` 调用 `nodeforge_graphs_preview`。来源使用当前 exportOrigin，不传其他 previousReport。

从报告读取 `/graphDifference`，检查 sourceEvidence、exportSourceValidation、exportOrigin、inputDigest 和目标。差异字段包括 nodesChanged、nodesAdded、nodesRemoved、positionsChanged、edgesAdded、edgesRemoved 与 settingsChanged。

nodesChanged 的结构为：

```text
[{id, changes:[{field,beforePresent,afterPresent,before,after}]}]
```

若只改一个节点默认值，应仅出现对应字段的变化，其他节点、连线、位置和设置保持不变。准备时更换的 requestId/assetPath 不计入图差异。

超出单次读取大小时，按导航读取 `/graphDifference/nodesChanged` 或更小子树。预览符合预期只说明图数据的差异正确，仍需预检、编译和保存。再次编辑后重新预览当前草稿。

## 6. 预检、生成与保存核验

先调用 `nodeforge_graphs_preflight`，通过后再调用 `nodeforge_graphs_generate`。各自使用新 callId、当前修订和 `arguments:{}`。生成时会重新检查来源和目标占用。

查询原生成调用，读取 checks、initialReadback/finalReadback/publishedReadback、inputDigest、实际 requestId/assetPath、fileSha256 与请求收据。`finished` 表示调用结束，`waitingPersistence` 表示保存后尚未独立检查重开。

需要重开结论时，按 [保存核验命令](Preparation.md)使用同一插件、**准备后的输入文件**和这次生成报告执行 `graphs.verifySaved`。不要使用最初导出的输入，也不要重新生成或保存来代替检查。运行、材质画面和打包另行检查。

## 遇到问题时

| 现象 | 处理 |
| --- | --- |
| pending 或 unknown | 保留当前草稿和结果，用原 callId 查询，不换请求或目标重发 |
| busy 或 staleRevision | 读取状态和草稿，先确认其他操作或修改 |
| 来源、报告、版本或输入冲突 | 核对本次实际文件与来源，不改写报告或校验值 |
| draftApplied=false | 保留原稿和准备记录，不进入预览或生成 |
| 目标占用或部分写入 | 查看已有资产、收据与原报告，不自动删除或覆盖 |
| 需要恢复草稿 | 显式调用 draft_restore，再读取完整草稿；恢复草稿不会撤销磁盘资产 |

更多状态解释见 [结果与恢复](ResultsAndRecovery.md)。
