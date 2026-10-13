> 当前接入身份：Build137 Repair5 / Ai1 / Mcp10R1；Windows x64官方UE5.3.2。正文功能说明继承Mcp9R19；Build136、原stage/PID/验收路径只表示历史证据。当前安装和连接统一见[AI安装](../User/AiInstallation.md)。不要复制历史本机路径或环境配置。

# 编辑已有图、预览差异并生成新副本

公开副本中的请求、目标、进程、会话及来源报告身份为虚构示例；历史产品结论保持，原始证明由维护者内部保留。实际调用须使用本次分配和返回的完整原 text/sourceRef，示例身份与零摘要不能作为来源证明。

本页对应 Build136 Repair6、桥 0.3.6、NodeForge-Build136-Mcp9R19。任务是读取一个明确的已保存源图，在受控 JSON 中修改已有值，查看相对源图的差异，再生成独立的新资产。原资产保留。工具定义与当前加载身份以本版 Manifest 和 `operations.describe` 为准；真实读取、草稿提交、编译保存、独立 Verify 分别判断，实际验收记录见 [版本说明](../Version/ReleaseNotes.md)。

## 适用范围与阅读入口

本路线支持当前目录可完整表达的普通 Material，以及 Actor 父类的单 EventGraph Blueprint。它复用原导出、共享编辑服务、PrepareNewCopy、GraphDiff、预检与生成实现。未知节点、不可表达属性、多图、新父类、循环、数组会按原规则拒绝。材质实例、函数整图、源资产原地覆盖、合并、搜索枚举和生产路径不属于此路线。

- 首次接入先读 [AI 快速开始](QuickStart.md)、[功能地图](../Reference/FeatureMap.md)。
- 专用工程、显式注册、当前会话、批次、正常退出及原 CLI Verify 的准备配方见 [操作者准备](Preparation.md)。
- 原完整导出、报告读取和来源限制见 [显式资源读取](AssetRead.md)。
- 参数定义见 [正式 API 参考](ApiReference.md)；失败判读见 [结果与恢复](ResultsAndRecovery.md)。
- 手动按钮路线见 [用户新副本教程](../User/CopyPreview.md)。

普通 JSON 可以沿原规则准备副本；差异预览必须有当前草稿绑定的可信导出来源。一次工具调用完成不表示资产已经完成独立保存重开核验。

## 1. 核准现场，只分配一次

先调用 `nodeforge_docs_index`，再用 `nodeforge_docs_read` 读取 `Readme`、`AiQuickStart`、`FeatureMap` 和 `AiCopyPreview`。确认本版 bundle、Build、桥、DLL、目录摘要和契约身份；资料读取无需启动 UE。

业务现场由有本机操作权限的操作者按 AiPreparation 显式准备。复用工程为 SelectedProject，项目路径 ReplaceWithInstallationRoot/ReplaceWithSelectedProject.uproject，候选插件路径 ReplaceWithInstallationRoot/PluginStage/NodeForge。已有编辑器、未闭合 run 或资产增量未对账时不接管。MCP 不启动或强杀 UE，不注册编辑器、不分配实验批次，也不执行新 PID Verify。

当前专用编辑器发布真实注册信息后，调用 `nodeforge_connect` 取得本次 `handle`，再调用 `nodeforge_operations_describe`。确认 `draft.prepareCopy`、`graphs.preview` 和 `draft.replace.retainExportOrigin` 的当前正式定义。用 `nodeforge_draft_get` 的 `nativeSummary.response.result` 取得真实 `draftRevision` 字符串与当前稿；用 `nodeforge_editor_getState` 核空闲状态。不能照抄旧 handle、PID、修订、callId 或批次。

测试目标使用 `/Game/NodeForgeTests/Material/批次/叶名` 或 `/Game/NodeForgeTests/Blueprint/批次/叶名`。允许根下可有 0、1、2 级子目录，每段 1–16 个 ASCII 字母数字；资产叶名和 requestId 的公开限额为 1–64，短名建议不改变公开限额。普通重试、换 PID 或换用例不新建工程。正式使用目录名保留 `NodeFroge`，本版不推定生产 mount。

以下配方使用当前变量，不提供可照抄的假会话身份：

| 变量 | 从哪里取得 |
| --- | --- |
| `current_handle` | 本次成功 connect 返回的 handle |
| `current_revision` | 该步提交前 draft.get 返回的字符串 draftRevision |
| `kind`、`source` | 本次明确选择的 Material 或 Blueprint 和精确 Package.Asset 路径 |
| `seed` | 本次首次分配的新 requestId、完整 assetPath；实验使用现有 run.allocation 中该族行 |
| 各步 callId | 首次提交前分配、保存的一组新 ID，ASCII 字母数字和连字符，1–64；原调用查询仍用原 ID |

下面 Python 片段只展示参数对象的构造；实际调用使用当前客户端提供的正式工具。`current_revision` 每步从真实返回更新，不能将同一个旧值沿整条流程复用。

## 2. 正式导出，并取得原完整正文

源必须是精确对象路径、已保存且 clean。Blueprint 还须已有可用 GeneratedClass 和编译状态。当前进程须有共享读取链的受控保存文件加载证明；没有证明的已载入对象会拒绝 `sourceMemoryUnproven`，插件不会自动重载、保存或编译源。新专用编辑器应先正式读取，再手动打开源资产。`sourceNotReady` 不等于不存在；应保留原结果并处理具体原因。

调用 `nodeforge_graphs_export`：

```python
export_params = {
    'handle': current_handle,
    'callId': export_call_id,
    'expectedDraftRevision': current_revision,
    'arguments': {
        'assetKind': kind,
        'source': source,
        'requestId': seed['requestId'],
        'assetPath': seed['assetPath'],
    },
}
```

导出不改变草稿、不预留目标、不写资产或请求收据。目标必须和源不同；文件、内存、注册表目标和项目 request 占用按原规则拒绝。成功后保留原完整返回，从本次 `reports` 取得该导出报告的真实 `reportId`。核原 native 调用与报告的 `operationStatus=passed`、`overallStatus=succeeded`，不能只看 `cliExitCode=0`。

调用 `nodeforge_report_read`，参数为当前 handle、上一步 reportId、`pointer='/exportedRequest'`。将它的完整返回记为 `P`。专用分支返回：

| 字段 | 用途 |
| --- | --- |
| `value` | 完整导出的 Graph v2 对象 |
| `text` | 原生序列化的完整文本；严格初载和原字节留存使用此值 |
| `payloadSha256` | 这份 text 的 UTF-8 字节 SHA |
| `sourceRef.exportReportPath`、`exportReportSha256` | 原报告路径与原报告字节 SHA，必须成对使用 |
| `sourceRef.source`、`sourceAssetSha256` | 真实源对象与源 uasset SHA |

完整 Graph 文本上限 65536 UTF-8 字节，原报告上限 1 MiB。普通 pointer 仍有 32768 字节上限；摘要和超限导航不能替代完整 Graph。这里不拼 sidecar，不从节点摘要重建正文。报告 SHA、源文件 SHA、payload 文本 SHA 和 Graph 语义摘要各有职责，不互换。

有本机文件权限时，将 `P.text` 按 UTF-8 原字节保存为本 run 的 `MaterialExportInput.json` 或 `BlueprintExportInput.json`，首次写入并拒绝覆盖；不添加 BOM 或尾部换行，不修改原导出报告。这份文件是来源留存，不是后续准备完成的生成输入。

## 3. 严格载入原图，再显式保留来源编辑

初次调用 `nodeforge_draft_replace`，把原导出文本和原报告引用交给正式来源校验：

```python
load_params = {
    'handle': current_handle,
    'callId': load_call_id,
    'expectedDraftRevision': current_revision,
    'arguments': {
        'text': P['text'],
        'exportReportPath': P['sourceRef']['exportReportPath'],
        'exportReportSha256': P['sourceRef']['exportReportSha256'],
    },
}
```

首次文本只能有格式差异，Graph 语义必须与原导出相同。不能在这次调用里提前改值、改身份或把整个 sourceRef 放进 JSON。也可使用准入文件的 `draft.loadFile` 和同一对报告字段严格初载；普通 loadFile 不继承原有来源。

载入成功后调用 `nodeforge_draft_get`。要求 `draftReadable=true`，取得完整 text、digest、新修订和 exportOrigin，核来源确实绑定。然后只对这份完整对象作本次明确编辑；保留 assetType、catalogDigest、原节点身份和未授权修改的内容。

### 两个有限固定例子

以下为本库已有明确材料，源路径不是新生成目标；其它用户工程不保证存在这些对象。必须在当前 DLL 的正式导出结果里先核节点、字段和原值，吻合才应用编辑。历史报告仅解释材料，不代替本次来源证明。

| 材料 | 本次唯一值改动 | 必须保留 |
| --- | --- | --- |
| `/Game/NodeForgeTests/Material/历史批次/Sample01.Sample01` | Node1，MaterialExpressionScalarParameter，ParameterName=`SurfaceRoughness`；`properties.DefaultValue` 从 `0.30000001192092896` 改为 `0.6` | Node2 的 SurfaceColor 值；Node1→Roughness、Node2→BaseColor 两条边；全部位置与 settings |
| `/Game/NodeForgeTests/Blueprint/历史批次/Sample01.Sample01` | Node1，owner=`/Script/Engine.KismetStringLibrary`、member=`Concat_StrStr`；`defaults.A` 从 `Data-driven ` 改为 `DATA-driven `，尾部空格保留 | defaults.B=`function output`；PrintString、BeginPlay、全部连线、位置与 settings |

该蓝图的 PrintString.InString 由 Concat_StrStr.ReturnValue 连入。保持原连线时，编辑一个未使用的 InString 默认值不能作为打印字符串输入变化；本例明确修改参与拼接的 A。这里只说明图数据的输入变化，不声称游戏已经运行或输出已实测。

例如，在当前正式导出的 `P.text` 上应用一个固定编辑：

```python
import json
edited = json.loads(P['text'])
node = next(n for n in edited['nodes'] if n['id'] == 'Node1')
assert edited['assetType'] == kind
if kind == 'Material':
    assert node['class'] == '/Script/Engine.MaterialExpressionScalarParameter'
    assert node['properties']['ParameterName'] == 'SurfaceRoughness'
    assert node['properties']['DefaultValue'] == 0.30000001192092896
    node['properties']['DefaultValue'] = 0.6
elif kind == 'Blueprint':
    assert node['class'] == '/Script/BlueprintGraph.K2Node_CallFunction'
    assert node['properties']['owner'] == '/Script/Engine.KismetStringLibrary'
    assert node['properties']['member'] == 'Concat_StrStr'
    assert node['defaults']['A'] == 'Data-driven '
    node['defaults']['A'] = 'DATA-driven '
else:
    raise ValueError('This recipe only covers Material or Blueprint')
edited_text = json.dumps(edited, ensure_ascii=False, separators=(',', ':'))
```

其它资产的 nodeId 与字段从实际目录、规则和导出中查明，不能把 Node1 当跨资产固定身份。对完整对象的其它变化，必须先取得本次授权并声明独立预期。

再次调用 `nodeforge_draft_replace`，这次明确保留当前来源：

```python
edit_params = {
    'handle': current_handle,
    'callId': edit_call_id,
    'expectedDraftRevision': current_revision,
    'arguments': {'text': edited_text, 'retainExportOrigin': True},
}
```

`retainExportOrigin` 是 JSON boolean，只能为 true 或 false，不能写字符串 `"true"`、数字 0/1。true 禁止同时给任一 exportReportPath 或 exportReportSha256；它使用服务器当前已绑定的来源并重新核源和报告。省略或 false 仍走原严格 Replace：附来源字段只能原图初载，无关替换清掉旧基线。来源不存在会明确拒绝。

成功后再读 draft.get，核全文、digest 和 exportOrigin。真正文本变化使修订增加，旧预检和预览失效；完全相同文本不增加修订。草稿编辑成功不等于预检通过。

## 4. 正式准备副本，并读取实际新身份

用当前修订和新 prepare_call_id 调用 `nodeforge_draft_prepareCopy`，`arguments={}`。不得传自己猜测的目标或另一份源报告。它使用同一共享生产实现，只改变 requestId 和 assetPath，保留同父目录与全部图内容，并写准备记录；不预约目标、不生成资产或收据。

保存完整返回，将 `nativeSummary.response.result` 记为 `C`，其 `result` 记为 `R`。依次核：

1. 原调用已终态，native 业务成功且 mode=`PrepareNewCopy`；不是 pending 或 unknown。
2. `R.draftApplied` 必须为 true；`previousDraftRevision` 等于提交时修订，`newDraftRevision` 是本次提交后的修订。
3. `R.copyPreparation` 与原 native 报告相同，保留 `R.nativeReport` 的路径和 SHA。
4. `copyPreparation.sourceInputDigest` 等于准备时的编辑稿摘要；`sourceInputUnchanged=true`、`targetReserved=false`、`assetGenerated=false`。
5. 读取实际 `originalRequestId/originalAssetPath`、`newRequestId/newAssetPath`、`preparedInputPath/preparedInputDigest`，不是分配器 seed 的猜测派生值。

然后调用 `nodeforge_draft_get`，将该次完整返回的 `nativeSummary.response.result` 记为准备稿 `D`，不是摘要里的无 text 元数据。要求 draftReadable=true，修订等于 R.newDraftRevision，digest 等于 preparedInputDigest，copyPreparationReport 关联本次准备报告，exportOrigin 仍对应原源。解析 D.text 并核 requestId/assetPath 等于本次返回的新值；将这两个字段临时还原为编辑前身份后，完整 Graph 内容必须与编辑稿一致。

后续预览、预检、生成、保存核验和 Verify 全部使用这份准备后的完整稿及实际身份。命名由产品提供；requestId 采用 Copy 加 GUID 的真实值，36 字符符合公开 64 上限，不能按分配 seed 的建议 32 字符拒绝。不要手工缩短、更换目标或改写产品输出。

有本机权限时，将 D.text 的 UTF-8 原字节首次保存为本 run 的 `MaterialInput.json` 或 `BlueprintInput.json`，核文件 SHA 等于 D.digest。本库实验工程先完成下一小节的身份登记，再保存此文件；普通产品使用不要求实验台账。`out` 是 AiPreparation 中保留的原 run 目录，`kind` 是本次实际 Material 或 Blueprint。以下只保存已取得的正文，不启动生成：

```python
prepared_bytes = D['text'].encode('utf-8')
assert hashlib.sha256(prepared_bytes).hexdigest() == D['digest']
prepared_input_path = out / (kind + 'Input.json')
with prepared_input_path.open('xb') as stream:
    stream.write(prepared_bytes)
```

`hashlib` 沿用 AiPreparation 的导入。首次写入拒绝覆盖，不加 BOM 或尾部换行。先前 `MaterialExportInput.json` / `BlueprintExportInput.json`、编辑稿和产品 `preparedInputPath` 的原字节均保留；不得用准备稿覆盖 ExportInput。这个 Input 才是本次真实生成和新 PID 独立 Verify 的输入，文件 SHA 必须同时等于 D.digest 和 copyPreparation.preparedInputDigest。

报告写出不等于草稿提交。draftApplied=false 时保留原稿、修订与已写记录，不进入预览或生成；unknown 时可能已提交，要查原 call 和当前稿，不删除文件或换身份重做。

### 实验工程台账与产品职责

产品的 Prepare 和 Generate 不依赖实验台账；正式生产实现负责真实新身份、目标冲突、来源和生成收据。`ReusableEditorProject` 是本库复用 专用验证工程 的实验保护助手，负责同 run、批次、原累计资产与收据保护，不能代替产品准备副本，也不能改写产品命名。

实验操作者在同一 run 下保存该族的准备身份映射，登记位置为 activeRun 的 preparedIdentities。原 allocated 行不删除、不改成另一份预分配：

| 映射信息 | 必须来自的真实依据 |
| --- | --- |
| case、batch、runDirectory、allocated requestId/assetPath | 当前原 run 与 IdentityAllocation |
| actual requestId/assetPath | 本次 copyPreparation 的 newRequestId/newAssetPath 与 draft.get 全文一致 |
| 原导出、编辑稿、准备输入的路径及各自 SHA | 各阶段保留的原字节与真实 digest，不能混用 |
| prepare callId、原报告路径/SHA、前后修订、draftApplied | 本次正式返回、原报告及新 draft.get |
| 源对象/SHA与 exportOrigin | 本次正式导出、绑定来源与当前源保护 |

在 AiPreparation 中仍持有原 `run` 对象的同一操作者 Python 进程里，正式 Prepare 成功、完整 draft.get 已核对、生成尚未提交时执行现有 `Tests/PluginControl/RunEditorChecks.py` 的方法：

```python
prepared_row = run.register_prepared_identity(kind, C, D)
```

这里 `C` 是本次 prepareCopy 的原 `nativeSummary.response.result` 调用记录，包含 operation、callId、state 和 result；`D` 是紧接其后的 draft.get 原 `nativeSummary.response.result`，包含完整 text。不能只传 copyPreparation、payload、草稿摘要或整个 MCP 外层返回，也不能另建 run 来登记已提交准备。该方法属于本机实验助手，不是 MCP 工具或产品 API。

登记前须满足原 activeRun 的 runDirectory/batch/allocated identities 与内存 run 一致、台账未被别的进程改变、串行租约属于此操作者 PID。该族尚未登记，实际目标与 request 未占用；原准备调用、Calls 下的 Input/Native/Control 证据、产品 preparedInput 文件和当前 SessionPublic/完整稿必须同一 run、会话、项目、DLL，修订和各份 SHA 一致。缺任一提交证据就停止，不重复分配批次。

成功后，台账 activeRun.state 为 `prepared`，preparedIdentities 追加该族映射；原 allocated seed、nextBatch、既有累计 assets/requests 和历史 runs 保持。返回的 `prepared_row` 含 allocated 与 actual、prepareCallId/controlSessionId、originalInput/preparedInput/prepareReport/controlEvidence 及其各自 SHA、previousDraftRevision/draftRevision、draftApplied 和 exportOrigin。行中的 originalInput 是 Prepare 提交前编辑稿的原 Calls 输入快照，不是 ExportInput；preparedInput 是产品准备文件。各文件字段使用相对 ReplaceWithInstallationRoot 的路径，batch/runDirectory 从该行所属 activeRun 查取。助手只登记映射，不额外写 Input/报告、不生成资产/收据，也不结束 run。登记成功后再按上方首次写入配方保存实际完整 Input。

若登记拒绝或写入结果不明，停止预检和生成。产品 Prepare 可能已经应用新稿；保存原 C/D、报告和现场，查询原 call、当前稿与原台账，先判断是否已经登记。不能把辅助登记失败当作产品未提交，不能删除准备文件、覆盖台账、换 seed/request/目标重发或调用 finish 掩盖未决结果。

本期生成、Verify 和终态核验只采用 actual 行；seed 没有生成便不能假称出现了 seed 资产或收据。结束前须核旧保护集加本次实际新增集合及必要 Verify；既有 finish 只记录终态和累计文件，不替操作者判定业务成功。开发维护时参见 [架构与状态](../Developer/Architecture.md)。

## 5. 读取差异，和独立预期逐项比较

用准备后当前修订、新 preview_call_id 调用 `nodeforge_graphs_preview`，`arguments={}`。来源从当前 exportOrigin 取得，不能传任意 previousReport。它重新核源与原报告，写本次差异报告并更新展示，不改 text、digest、revision、准备关系、历史成功输入、源资产或收据，也不预约目标。

成功后从本次 reports 取真实 reportId，用 `nodeforge_report_read` 读取 `/graphDifference`。另核本次报告的 sourceEvidence、exportSourceValidation、exportOrigin 与 inputDigest/requestId/assetPath，确保比较的是这份准备稿和这份原源。`nativeSummary` 摘要不能代替原差异。

两固定例的独立预期来自原 JSON，不从 GraphDiff 输出反向编造：

| 差异项 | Material 固定例 | Blueprint 固定例 |
| --- | --- | --- |
| nodesChanged | 只有 id=Node1；changes 中只有 field=properties，before/after 里唯一变化为 DefaultValue 的上述值 | 只有 id=Node1；changes 中只有 field=defaults，before/after 里唯一变化为 A 的上述字符串 |
| nodesAdded、nodesRemoved | 空数组 | 空数组 |
| positionsChanged | 空数组 | 空数组 |
| edgesAdded、edgesRemoved | 空数组 | 空数组 |
| settingsChanged | 空数组 | 空数组 |
| requestStructureOrValuesChanged | true | true |
| layoutChanged | false | false |

`nodesChanged` 的原结构是 `[{id, changes:[{field,beforePresent,afterPresent,before,after}]}]`；不是扁平的 `{id,field}`。准备步骤仅改变 requestId/assetPath，这两个派生身份不计入图差异。还须独立核其它节点、属性、默认值、边、位置与 settings 原样，不能只数一个 changed 节点。

普通 pointer 超限时按返回导航读取 `/graphDifference/nodesChanged`、`/graphDifference/nodesChanged/0`、`/graphDifference/nodesChanged/0/changes/0/before` 等准确子树，直到本次所需差异取得完整值；原报告保持。无法完整读取所需值则停止，不能把截断内容当完整预览。当前固定例不需要增加一套分页 API。

差异符合预期只说明数据和布局的比较结果，不证明编译、保存、运行或材质画面。之后编辑任何内容都会使旧预览失效；重新对实际当前稿做受影响的预览与预检，不重跑旧整批测试。

## 6. 预检、生成并记录真实保存结果

调用 `nodeforge_graphs_preflight`，本次新 callId、当前准备稿修订、`arguments={}`。读原 GraphPreflight 结论及诊断；通过才显式调用 `nodeforge_graphs_generate`，另一个新 callId、同一未变修订、`arguments={}`。

准备和预览均未预留名字，生成会再次检查目标、request 和来源。不能根据先前 passed 跳过新占用冲突。新稿只有 actual 身份；原资产、输入、报告和失败历史保持。

保存完整生成返回，用 `nodeforge_calls_get` 查询原生成 call，并从其登记报告读取原生成结果。核 checks、initialReadback/finalReadback/publishedReadback、输入 digest、实际 requestId/assetPath、生成 PID、DLL/项目、fileSha256 和项目请求收据。`status=finished` 是 CLI 调用终态；原 Import 常见 `overallStatus=waitingPersistence`，表示已保存但独立持久化尚未核验。不能改写生成原报告为最终资产通过。

实验保护以原 LedgerBefore 的完整 assets/requests 加本次 actual 资产/收据为期望集合，逐字节核旧保护，不能把未知新增全部采纳。源 uasset SHA 也保持原值。完整保存核验配方见 AiPreparation，输入路径必须指本节已保存的准备稿，不是原 ExportInput。普通用户任务只核实际生成的对象；本库有限 C 默认两族各一个新资产，不为填表生成第三个对象。

## 7. 新 PID 使用原 CLI 独立 Verify

需要最终保存重开结论时，由操作者正常关闭本次自有 UE，确认准确生成进程已结束且工程独占；不关闭无关窗口、不强杀。然后使用同 stage、同一准备稿原字节和该对象原生成报告，通过正式 CLI 启动独立 Verify：

```powershell
& 'ReplaceWithInstallationRoot\Tools\NodeForgeMcp\Environment\Scripts\python.exe' 'ReplaceWithInstallationRoot\Scripts\NodeForgeControl.py' execute --operation graphs.verifySaved --project 'ReplaceWithInstallationRoot/ReplaceWithSelectedProject.uproject' --plugin-directory 'ReplaceWithInstallationRoot/PluginStage/NodeForge' --run-directory 'ReplaceWithNewVerifyRunDirectory' --input 'ReplaceWithCurrentPreparedInputPath' --generation-report 'ReplaceWithOriginalGenerationReportPath' --timeout-seconds 240
```

三个 ReplaceWith 路径必须换成操作者本次真实绝对路径；run-directory 是本次尚不存在的 Verify 输出目录。Blueprint 和 Material 各用自己的输入/报告。CLI 没有 verify 子命令。不提供 description-file 时，原 CLI 会从当前 DLL 取得正式定义并绑定 SHA；不能给新 DLL 使用历史 Build 的描述。

检查 Verify 原 CLI/native 状态、persistence、回读、inputDigest、actual target、文件 SHA 和旧保护集。Verify PID 必须不同于原 generationPID，stage/DLL 必须相同。不重新 Import、resave、分配业务身份或创建新工程。只有必要 Verify 与全部原调用终态和保护核验通过，才能按 AiPreparation 收口原 run。游戏运行、材质画面和打包分别保留原未验边界，不自动追加。

## 8. 停止与有限恢复

| 现象 | 下一步 |
| --- | --- |
| pending 或 unknown | 停依赖写入，保留当前稿/身份/原返回；查原 nodeforge_calls_get、CLI、报告与准确进程。不换 call/request/目标重发 |
| busy 或 staleRevision | 先读 editor.getState、draft.get 和原 call，查谁改变了稿。不能把新修订填入旧写入后自动重试 |
| 来源、报告、DLL、工程、输入 digest 或身份冲突 | 停止，保留原字节，核本次成套版本与实际 source。不能手拼新引用或把旧报告重新标成功 |
| prepare 报告存在但 draftApplied=false | 原稿及修订保持；记录保留，不把 preparedInput 文件存在当提交成功 |
| prepare unknown 且新稿已出现 | 保留已提交事实，核原 call/draft/report；不能删除记录或另准备一次掩盖结果 |
| target 占用或生成已有部分写入 | 查询原资产/收据/报告，保持原目标；不自动删资产、缩名字、换 request 或覆盖原报告 |
| 需要恢复准备前草稿 | 明确调用 draft.restore，准确修订、新 callId，读真实恢复全文与来源；这是一步草稿恢复，不撤销磁盘资产，旧检查失效 |

只有确切证据证明原业务未发送/未 dispatch，且原因已经解决，才允许按现有契约一次有限恢复；仅查不到 call 不够。源/草稿/基线关系、权限或结果仍不清就停下说明。不能自动清锁、kill、保存 dirty 源、扩展业务范围或无限追加测试。

交付记录分别写正式读取/导出、来源载入与编辑、准备提交/预览、生成/编译/保存、新 PID Verify，以及实际未执行的运行/画面/打包。阅读文档、静态检查或候选编译不填成业务实测。无旧聊天的接手 AI 可从本版索引与上述正式结果恢复，不以文档代替本次权限与 Goal。
