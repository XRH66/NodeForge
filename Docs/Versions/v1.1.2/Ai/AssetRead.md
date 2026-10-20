# 读取资源、导出图与生成副本

本页适用于 NodeForge v1.1.2。给出精确对象路径后，可以读取材质参数、材质函数签名，或将支持的材质和 Actor 蓝图导出为完整 JSON。

## 1. 选择工具

| 任务 | 工具与参数 | 读取结果 |
| --- | --- | --- |
| 普通材质参数 | `nodeforge_resources_inspect`，kind=MaterialParameters | `/parameters`：参数默认值、纹理信息、parent 和 parentAssetSha256 |
| 材质函数签名 | `nodeforge_resources_inspect`，kind=MaterialFunction | `/functionSignature` 和 `/nodeTemplate`：真实输入、输出、GUID、类型和默认策略 |
| 普通材质完整图 | `nodeforge_graphs_export`，assetKind=Material | `/exportedRequest`：完整图和来源引用 |
| Actor 蓝图完整图 | `nodeforge_graphs_export`，assetKind=Blueprint | 单 EventGraph、允许变量及默认骨架组成的完整图 |

source 使用精确的 `Package.Asset` 路径。Material 和 MaterialFunction 允许绑定配置中的 `/Engine/` 或 `/Game/NodeForgeTests/` 范围，Blueprint 只允许配置的测试根。需要查找路径时使用 [资源查询](Resources.md)。

导出只支持当前目录能完整表达的图。材质实例、函数实例、材质层、函数整图、多图、新父类、额外组件、循环和数组不在此路线内；未知节点或属性会被拒绝。

## 2. 准备连接和源资产

按 [准备工作](Preparation.md)完成安装与连接。调用 `nodeforge_connect` 取得 handle，调用 `nodeforge_draft_get` 取得 draftRevision。读取操作也经过编辑器串行执行检查，需要新的 callId 和当前 expectedDraftRevision。

源资产必须已保存且未修改；Blueprint 还需要可用的 GeneratedClass 与编译状态。当前进程如果已加载对象，却无法确认其保存文件来源，会返回 `sourceMemoryUnproven`。插件不会自动重载、保存或编译源资产，因此建议在新连接的编辑器里先用工具读取，再手动打开资产。

`sourceNotReady` 表示注册表尚不能确认对象，不等于对象不存在。类型不符、重定向、未保存或 dirty 会分别报错，按实际错误处理。

下方全零 handle、修订 0 和示例 ID 只表示格式。请使用本次连接和草稿返回值，并替换为自己工程中的实际资源。

## 3. 读取材质或函数

材质参数示例：

```json
{
  "handle": "00000000000000000000000000000000",
  "callId": "ExampleCalld7ab60de",
  "expectedDraftRevision": "0",
  "arguments": {
    "kind": "MaterialParameters",
    "source": "/Game/NodeForgeTests/Material/ExampleRun/Sample01.Sample01"
  }
}
```

函数签名示例：

```json
{
  "handle": "00000000000000000000000000000000",
  "callId": "ExampleCall19459e52",
  "expectedDraftRevision": "0",
  "arguments": {
    "kind": "MaterialFunction",
    "source": "/Engine/Functions/Engine_MaterialFunctions02/SmoothStep.SmoothStep"
  }
}
```

先检查 status、cliExitCode 和 nativeSummary，再从 reports 取得 reportId。原生报告应有 `operationStatus=passed`、`overallStatus=succeeded`。材质读取 `/parameters`；函数读取 `/functionSignature` 和 `/nodeTemplate`；源信息位于 `/resourceRead`。

空参数集合可能是合法材质。函数 nodeTemplate 是单节点片段，不能直接当完整图载入草稿。输入的 name 用于连接，guid 标识真实引脚，required=true 的输入必须连接；可选输入沿结果给出的 usesNativePreviewDefault/nativePreviewValue 使用原生预览默认值。输出 index 是真实输出索引的字符串。

v2 的 MaterialFunction/FunctionAssetSha256 放在 properties；v3 转为 `binding:{kind:"MaterialFunction",path,assetSha256}`，properties 为空。文件 SHA 只绑定直接资源文件，不递归锁定其所有依赖。

## 4. 导出完整图

先确定未占用的新 requestId 和 assetPath，再调用 `nodeforge_graphs_export`：

```json
{
  "handle": "00000000000000000000000000000000",
  "callId": "ExampleCall0d95f29f",
  "expectedDraftRevision": "0",
  "arguments": {
    "assetKind": "Material",
    "formatVersion": 3,
    "source": "/Game/NodeForgeTests/Material/ExampleRun/Sample01.Sample01",
    "requestId": "AllocatedMaterialRequest",
    "assetPath": "/Game/NodeForgeTests/Material/AllocatedBatch/Sample01"
  }
}
```

导出 Blueprint 时，将 assetKind 改为 Blueprint，并填写该蓝图的精确 source。源与目标必须不同；文件、内存、注册表目标和已有请求占用都会被拒绝。导出不预留名称，也不写资产或请求收据。

显式 `formatVersion:3` 返回 v3；省略时返回 v2。成功后用该次报告的真实 reportId 读取完整导出：

```json
{
  "handle": "00000000000000000000000000000000",
  "reportId": "0000000000000000000000000000000000000000000000000000000000000000",
  "pointer": "/exportedRequest"
}
```

将返回记为 `P`。它包含完整对象 value、原生序列化文本 text、payloadSha256 和 sourceRef。text 最多 65536 个 UTF-8 字节；完整报告最多 1 MiB。普通 pointer 仍限制 32768 字节，超限时按导航读取更小子树。

sourceRef.exportReportPath/exportReportSha256 指向原报告及其 SHA，source/sourceAssetSha256 标识源资产。报告 SHA、源文件 SHA、导出文本 SHA 与图语义摘要不能互换。保留 P.text 原文和原报告，不根据摘要重新拼出图。

## 5. 载入来源并生成副本

以下 Python 片段展示如何构造 `nodeforge_draft_replace` 参数。实际调用使用客户端的 MCP 工具，handle、修订和 callId 取当前值：

```python
arguments = {
    'text': P['text'],
    'exportReportPath': P['sourceRef']['exportReportPath'],
    'exportReportSha256': P['sourceRef']['exportReportSha256'],
}
params = {'handle': current_handle, 'arguments': arguments,
          'callId': allocated_load_call, 'expectedDraftRevision': current_revision}
```

也可以把 P.text 保存到允许目录内的 JSON 文件，再用 `draft_loadFile` 的 path 和同一对来源字段载入。两个报告字段必须同时提供，不能把整个 sourceRef 放入 Graph 或 arguments。

首次载入会核对报告、插件、工程、源文件和 payload。图的值、字段出现状态、数组顺序、request 与 target 必须与导出一致；允许无语义差异的格式调整或当前目录支持的 v2/v3 表示转换。修改已有图时，先完整载入，再按 [副本编辑](CopyPreview.md)显式保留来源编辑。

载入后重新 `draft_get`，确认 text、digest、exportOrigin 和新修订。预检通过后再生成，二者分别使用新的 callId。预检和生成会重新检查源文件 SHA 与保存状态。

## 6. 检查结果

检查生成报告的 checks、initialReadback/finalReadback/publishedReadback、inputDigest、requestId/assetPath、fileSha256 和请求收据。`waitingPersistence` 按 [保存核验](Preparation.md)执行独立重开检查。

源或报告发生冲突、出现部分写入，或返回 pending/unknown 时，保留结果并查询原调用。不要更换目标重发、删除部分文件或修改来源报告。详细状态处理见 [结果与恢复](ResultsAndRecovery.md)。

完整输入示例见 [材质与蓝图示例](AssetReadExamples.md)。界面使用方法见 [手动操作](../User/ManualWorkflows.md)。
