# 材质和蓝图的完整输入示例

以下是 NodeForge v0.1.0 的完整输入示例。使用前读取当前目录，选择未占用的 requestId 和目标路径，并按 [调用流程](QuickStart.md)执行预检与生成。示例中的资源路径、会话和 SHA 占位值需要换成当前实际返回的值。

示例如果引用资源，先用 resources_inspect 检查已保存且未修改的对象、参数和函数签名。全零 assetSha256 是占位值，必须替换为这次检查返回的真实 SHA。如果资源或引脚不匹配，先调整任务输入，不要改写来源报告。具体步骤见 [资源查询与引用](Resources.md)。

## 复制已有资产的调用顺序

1. connect 后用 draft_get 取得当前修订。
2. graphs_export 指定源、未占用的新请求和目标。
3. 查询原调用，读取报告的 /exportedRequest，将返回记为 P。
4. draft_replace 使用 P.text 及 P.sourceRef 中成对的报告路径和 SHA。
5. 读取新修订，预检通过后生成。
6. 检查生成报告；需要确认重开结果时，按 [保存核验](Preparation.md)处理。

P.text是原完整UTF-8文本，不能由P.value重新序列化后声称原payload。下方图只用于新建配方说明；从已有资产复制时使用本次原P.text，不使用这里的图或虚构sourceRef。返回handle、callId、修订和报告引用均来自本次真实结果；未知、部分写入或来源冲突先停止依赖写入并查询原结果。

## Material v2

```json
{
  "format": "NodeForgeGraph",
  "version": 2,
  "requestId": "ExampleMaterial1",
  "assetType": "Material",
  "assetPath": "/Game/NodeForgeTests/Examples/Sample1",
  "catalogDigest": "be97bf57d226778102038c261c556b89b262197c78158fe16ca293de709d8847",
  "nodes": [
    {
      "id": "Node1",
      "class": "/Script/Engine.MaterialExpressionScalarParameter",
      "properties": {
        "DefaultValue": 0.30000001192092896,
        "ParameterName": "SurfaceRoughness"
      },
      "position": [
        -408,
        352
      ]
    },
    {
      "id": "Node2",
      "class": "/Script/Engine.MaterialExpressionVectorParameter",
      "properties": {
        "DefaultValue": [
          0.05000000074505806,
          0.550000011920929,
          0.800000011920929,
          1
        ],
        "ParameterName": "SurfaceColor"
      },
      "position": [
        -408,
        0
      ]
    }
  ],
  "edges": [
    {
      "from": "Node2",
      "output": "0",
      "to": "Material",
      "input": "BaseColor"
    },
    {
      "from": "Node1",
      "output": "0",
      "to": "Material",
      "input": "Roughness"
    }
  ],
  "settings": {
    "domain": "Surface",
    "blend": "Opaque",
    "shading": "DefaultLit",
    "twoSided": false,
    "lighting": "SurfaceForwardShading"
  }
}
```

## Blueprint v2

```json
{
  "format": "NodeForgeGraph",
  "version": 2,
  "requestId": "ExampleBlueprint2",
  "assetType": "Blueprint",
  "assetPath": "/Game/NodeForgeTests/Examples/Sample2",
  "catalogDigest": "be97bf57d226778102038c261c556b89b262197c78158fe16ca293de709d8847",
  "nodes": [
    {
      "id": "Node1",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetStringLibrary",
        "member": "Concat_StrStr"
      },
      "defaults": {
        "A": "Data-driven ",
        "B": "function output"
      },
      "position": [
        0,
        256
      ]
    },
    {
      "id": "Node2",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetSystemLibrary",
        "member": "PrintString"
      },
      "defaults": {
        "bPrintToScreen": false,
        "bPrintToLog": true,
        "Duration": 2,
        "Key": "None"
      },
      "position": [
        536,
        48
      ]
    },
    {
      "id": "Node3",
      "class": "/Script/BlueprintGraph.K2Node_Event",
      "properties": {
        "owner": "/Script/Engine.Actor",
        "member": "ReceiveBeginPlay"
      },
      "position": [
        0,
        0
      ]
    }
  ],
  "edges": [
    {
      "from": "Node1",
      "output": "ReturnValue",
      "to": "Node2",
      "input": "InString"
    },
    {
      "from": "Node3",
      "output": "then",
      "to": "Node2",
      "input": "execute"
    }
  ],
  "settings": {
    "parentClass": "/Script/Engine.Actor",
    "variables": []
  }
}
```

## 检查结果

生成后检查报告中的编译、保存和回读结果；`waitingPersistence` 表示尚未独立检查重开结果，按 [保存核验](Preparation.md)继续。

[精确字段与来源](AssetRead.md) · [准备](Preparation.md) · [结果与恢复](ResultsAndRecovery.md)
