# 两族完整输入与来源载入

本页对应 {{bundleId}} / {{buildId}}，目录摘要由当前正式目录生成。下面是完整当前配方，保留原有限样例的节点、属性和连接语义；它们不是历史导出原payload或已保存来源证明。当前schema、类名及事件/函数绑定已静态核对，目录仍experimental。新任务先确认授权与本次实际目录/目标占用，按[AiPreparation]({{page:AiPreparation}})使用新的requestId和完整目标，保存实际输入原字节后正式预检；示例身份不预留，也不能把换digest当旧能力升级。

带资源依赖的配方先精确inspect当前已保存、clean资源及实际文件SHA/函数GUID签名。全零assetSha256是明确未取得证明的占位值，结构合法但不可直接预检生成；必须用本次返回的真实值。若资源/参数/端口与配方不匹配就停止，不能修改来源报告或凭文档补造证明。完整材料引用正例见[AiResources]({{page:AiResources}})。

## 导出原来源的调用顺序

connect → draft_get取真实修订 → graphs_export（assetKind/source/新requestId/新assetPath，默认2或显式formatVersion=3）→ calls_get查原call → report_read原reportId的/exportedRequest得到P → draft_replace使用P.text及P.sourceRef.exportReportPath/exportReportSha256 → draft_get核新修订 → graphs_preflight → 明确生成新副本才graphs_generate → 原calls_get/report_read → 正常结束自有UE → 同stage/原实际Input/原生成报告、新PID原CLI Verify。

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

## 结果与历史依据

有限B在Build130完成两族完整导出/来源载入、各一新副本保存与新PIDVerify；原报告、输入SHA、PID和已占用身份留内部历史。本页新配方经当前静态规则核对，不把它当一次新的产品生成证明。保存后的waitingPersistence还须独立Verify；运行、画面、打包分别看本候选发行说明。

[精确字段与来源]({{page:AiAssetRead}}) · [准备]({{page:AiPreparation}}) · [结果与恢复]({{page:AiResultsAndRecovery}})
