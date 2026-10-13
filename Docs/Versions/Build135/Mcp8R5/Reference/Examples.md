# 最小 JSON 样例与查询、恢复

下面是原固定样例完整语义模板。requestId/assetPath只作占位，不允许直接复用；首次提交由 ControlRuns 台账分配新批次/两身份，catalogDigest来自当前formal query。原fixture SHA记录见 P0 FixtureManifest。输入原样保留，衍生仅改三字段，不能为绕失败改属性或连接。

## Material

```json
{
  "format": "NodeForgeGraph",
  "version": 2,
  "requestId": "ReplaceWithAllocatedRequest",
  "assetType": "Material",
  "assetPath": "/Game/NodeForgeTests/Material/B000000/ReplaceWithAllocatedName",
  "catalogDigest": "ff94bd6ad0895356478184a5a503fe829e8f8fe0d85007fdd3de8ca2c1390c10",
  "nodes": [
    {
      "id": "Color",
      "class": "/Script/Engine.MaterialExpressionVectorParameter",
      "properties": {
        "ParameterName": "SurfaceColor",
        "DefaultValue": [
          0.05,
          0.55,
          0.8,
          1
        ]
      }
    },
    {
      "id": "Rough",
      "class": "/Script/Engine.MaterialExpressionScalarParameter",
      "properties": {
        "ParameterName": "SurfaceRoughness",
        "DefaultValue": 0.3
      }
    }
  ],
  "edges": [
    {
      "from": "Color",
      "output": "0",
      "to": "Material",
      "input": "BaseColor"
    },
    {
      "from": "Rough",
      "output": "0",
      "to": "Material",
      "input": "Roughness"
    }
  ],
  "settings": {
    "domain": "Surface",
    "blend": "Opaque",
    "shading": "DefaultLit",
    "twoSided": false
  }
}
```

## Blueprint

```json
{
  "format": "NodeForgeGraph",
  "version": 2,
  "requestId": "ReplaceWithAllocatedRequest",
  "assetType": "Blueprint",
  "assetPath": "/Game/NodeForgeTests/Blueprint/B000000/ReplaceWithAllocatedName",
  "catalogDigest": "ff94bd6ad0895356478184a5a503fe829e8f8fe0d85007fdd3de8ca2c1390c10",
  "nodes": [
    {
      "id": "Begin",
      "class": "/Script/BlueprintGraph.K2Node_Event",
      "properties": {
        "owner": "/Script/Engine.Actor",
        "member": "ReceiveBeginPlay"
      }
    },
    {
      "id": "Join",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetStringLibrary",
        "member": "Concat_StrStr"
      },
      "defaults": {
        "A": "Data-driven ",
        "B": "function output"
      }
    },
    {
      "id": "Print",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetSystemLibrary",
        "member": "PrintString"
      },
      "defaults": {
        "bPrintToScreen": false,
        "bPrintToLog": true
      }
    }
  ],
  "edges": [
    {
      "from": "Begin",
      "output": "then",
      "to": "Print",
      "input": "execute"
    },
    {
      "from": "Join",
      "output": "ReturnValue",
      "to": "Print",
      "input": "InString"
    }
  ],
  "settings": {
    "parentClass": "/Script/Engine.Actor",
    "variables": []
  }
}
```

## MaterialInstance 模板（依赖未填，不是可直接生成样例）

先操作者实际读取已保存干净父 Material 的参数及直接文件 SHA。真实父/参数名称不可虚构；准备如下结构但替换真实数据后才可预检。此例本期MCP实例实测notRun：

```json
{"format":"NodeForgeGraph","version":2,"requestId":"ReplaceWithAllocatedRequest","assetType":"MaterialInstance","assetPath":"/Game/NodeForgeTests/Instance/B000000/ReplaceWithAllocatedName","catalogDigest":"ReplaceWithCurrentCatalogDigest","nodes":[],"edges":[],"settings":{"parent":"ReplaceWithInspectedParentObjectPath","parentAssetSha256":"ReplaceWithActualFileSha256","scalarParameters":{},"vectorParameters":{},"textureParameters":{}}}
```

## 查询与恢复参数

本版真实例子是同工程B000002批次的StaticColor和BlueprintConcat固定语义。[实际输入/结果](../../../../../Reports/PluginControlStageTwo/McpDocs1/P3/Run1/Review.json)、[原Codex请求与响应](../../../../../Reports/PluginControlStageTwo/McpDocs1/P3/Run1/Codex)和[McpAcceptance](../../../Build122/Mcp1/McpAcceptance.json)可阅读，所有其中的身份均已消费；后续使用先分配新批次/完整资产路径/requestId/callId，不能把历史调用复制后重新生成。

本批调用顺序：connect→operations_describe/editor_getState→catalog_get→两族rules_query→按reportId读精确节点→Material loadFile/preflight/generate/calls_get/report_read→Blueprint相同步骤。载入取得的新revision分别是1和2；生成报告状态waitingPersistence由两个新PID的原CLI graphs.verifySaved提升为独立持久化通过。下一次revision须实查，不能硬写本例数字。

`nodeforge_rules_query` arguments={"assetKind":"Material","query":"ScalarParameter VectorParameter"}；Blueprint可query="Concat PrintString"。外层必须同时提供 handle、新 callId 和当前字符串 expectedDraftRevision；catalog_get 同样需要这些GT字段。空query是全族，结果报告可能需pointer读取。查询不改变草稿，query大小写规则与图精确绑定不同。

未知生成只调用 `nodeforge_calls_get` arguments={"callId":"原生成callId"}，不换ID重发。报告拿到reportId再 `nodeforge_report_read` pointer="/checks" 或真实readback子树。必须将原状态/收据与输入对照，不能把查询成功当资产通过。
