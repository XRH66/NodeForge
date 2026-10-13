> 当前接入身份：Build137 Repair5 / Ai1 / Mcp10R1；Windows x64官方UE5.3.2。正文功能说明继承Mcp9R19；Build136、原stage/PID/验收路径只表示历史证据。当前安装和连接统一见[AI安装](../User/AiInstallation.md)。不要复制历史本机路径或环境配置。

# 最小完整JSON配方与结果恢复

完整当前v2材质/Actor蓝图及依赖明确的实例结构如下。图语义沿用原固定样例，并核当前schema；示例身份不预留，实际提交前按[AiPreparation](../Ai/Preparation.md)核新目标/request与真实目录。维护验收台账不是普通安装前置。

## Material v2

```json
{
  "format": "NodeForgeGraph",
  "version": 2,
  "requestId": "ExampleMaterial",
  "assetType": "Material",
  "assetPath": "/Game/NodeForgeTests/Examples/StaticColor",
  "catalogDigest": "be97bf57d226778102038c261c556b89b262197c78158fe16ca293de709d8847",
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

## Blueprint v2

```json
{
  "format": "NodeForgeGraph",
  "version": 2,
  "requestId": "ExampleBlueprint",
  "assetType": "Blueprint",
  "assetPath": "/Game/NodeForgeTests/Examples/BlueprintConcat",
  "catalogDigest": "be97bf57d226778102038c261c556b89b262197c78158fe16ca293de709d8847",
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

## MaterialInstance：填写实际依赖后才可执行

Parent路径和全零parentAssetSha256是明确未取得依赖证明的占位值，不是有效父引用。先精确读取本次已保存、clean普通父材质真实参数/文件SHA；空覆盖映射只说明不覆写参数，不证明任何参数覆盖验收。实例MCP未做本期实测。

```json
{
  "format": "NodeForgeGraph",
  "version": 2,
  "requestId": "ExampleInstance",
  "assetType": "MaterialInstance",
  "assetPath": "/Game/NodeForgeTests/Examples/Instance",
  "catalogDigest": "be97bf57d226778102038c261c556b89b262197c78158fe16ca293de709d8847",
  "nodes": [],
  "edges": [],
  "settings": {
    "parent": "/Game/NodeForgeTests/Examples/Parent.Parent",
    "parentAssetSha256": "0000000000000000000000000000000000000000000000000000000000000000",
    "scalarParameters": {},
    "vectorParameters": {},
    "textureParameters": {}
  }
}
```


完整v3见[AiJsonArchitectureExamples](../Ai/JsonArchitectureExamples.md)，纹理/函数引用见[AiResources](../Ai/Resources.md)。connect后先draft_get；rules_query/catalog_get使用本次handle、新callId、真实expectedDraftRevision。draft_replace/loadFile载入完整输入后再读修订，各新call预检和生成；calls_get查询原callId，report_read按原reportId取checks/readback。unknown停止依赖写入，waitingPersistence保留原输入/收据、正常退出自有UE、同stage新PID原CLI Verify。详细恢复见[AiResultsAndRecovery](../Ai/ResultsAndRecovery.md)。
