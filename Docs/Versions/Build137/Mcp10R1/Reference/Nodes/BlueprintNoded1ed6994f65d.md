# 普通Actor逐帧事件，实际DeltaSeconds用于速度积分

用途：普通Actor逐帧事件，实际DeltaSeconds用于速度积分。

前提：普通Actor单EventGraph，当前目录支持ReceiveTick；每个精确事件最多一个，可与BeginPlay共存。DeltaSeconds真实原生端口及MAKE资格由BT3核定。

限制：仅本期X轴100厘米/秒样例验收，不推广其它持续行为/帧率/组件/父类。旧133/132摘要不支持Tick。

精确身份（绑定本包；不能凭序号跨版匹配）：

```json
{
  "assetKind": "Blueprint",
  "class": "/Script/BlueprintGraph.K2Node_Event",
  "binding": {
    "owner": "/Script/Engine.Actor",
    "member": "ReceiveTick"
  }
}
```

当前原目录属性/允许默认/引脚与类型：

```json
{
  "class": "/Script/BlueprintGraph.K2Node_Event",
  "properties": {
    "owner": "/Script/Engine.Actor",
    "member": "ReceiveTick"
  },
  "discovered": true,
  "actionDatabaseMatch": true,
  "constructible": true,
  "family": "ExistingCore",
  "minimumBlueprintRevision": 11,
  "supportState": "experimental",
  "verified": "See regression evidence; discovery and instance construction do not imply compiled/runtime support",
  "variableType": "",
  "pins": [
    {
      "name": "OutputDelegate",
      "direction": "output",
      "category": "delegate",
      "subcategory": "None",
      "subtype": "",
      "hidden": false,
      "defaultWritable": false,
      "defaultText": "",
      "defaultObject": ""
    },
    {
      "name": "then",
      "direction": "output",
      "category": "exec",
      "subcategory": "None",
      "subtype": "",
      "hidden": false,
      "defaultWritable": false,
      "defaultText": "",
      "defaultObject": ""
    },
    {
      "name": "DeltaSeconds",
      "direction": "output",
      "category": "real",
      "subcategory": "float",
      "subtype": "",
      "hidden": false,
      "defaultWritable": false,
      "defaultText": "0.0",
      "defaultObject": ""
    }
  ]
}
```

当前v3声明片段（v2使用下方历史绑定表示）：

```json
{
  "id": "Tick",
  "class": "/Script/BlueprintGraph.K2Node_Event",
  "properties": {},
  "binding": {
    "kind": "Event",
    "owner": "/Script/Engine.Actor",
    "member": "ReceiveTick"
  }
}
```

v3 typed Event绑定片段；完整两事件配方见AiBlueprintTick，原生资格与运行结果由同版报告追溯。

来源：Plugins/NodeForge/Source/NodeForgeEditor/Private/NodeForgeNodeDefinitions.cpp。
历史记录保留原身份/配置/Build/层级，缺证据保持notEstablished：

```json
[]
```

本期资料引用的实际证据（无条目时 notEstablished；签名证据不能代替组合编译/保存/Verify）：

```json
[
  {
    "release": "Build134/BT02",
    "configuration": "HistoricalRun/BPTickMove only, two Actor events and X=100*DeltaSeconds",
    "catalogSupportUnchanged": "experimental",
    "allConfigurationsVerified": false,
    "layers": {
      "productionPreflight": true,
      "nativeBuildAndReadback": true,
      "compileAndSave": true,
      "independentSavedVerify": true,
      "fullExport2And3": true,
      "originalExport3SourcePreflight": true,
      "finitePassivePIE": true
    },
    "evidence": [
      {
        "path": "internalEvidenceRetained",
        "sha256": "d592d0ce526cd400045096d06f1e70b88443f237110e92f830a671cc7a3463e0",
        "bytes": 3862
      },
      {
        "path": "internalEvidenceRetained",
        "sha256": "86fb19a18118c712674deea0a590eac4f2a32e21ab2c71a6c796ecc8c673ff78",
        "bytes": 48161
      },
      {
        "path": "internalEvidenceRetained",
        "sha256": "6fa6f08ca55fd901b8bc6acbd7f21a599b656c6723ab99571ec6ffd58c4c3a05",
        "bytes": 33829
      },
      {
        "path": "internalEvidenceRetained",
        "sha256": "c91eaee1a1bf0d46ebd14c96f08bb438b65a9f17804c77375d5c6a86ae70a806",
        "bytes": 628
      },
      {
        "path": "internalEvidenceRetained",
        "sha256": "5fbb93387e453ea94726abda76955b187148e148f76b983f220d7d5ecaead12d",
        "bytes": 443
      },
      {
        "path": "internalEvidenceRetained",
        "sha256": "2f727edbc838a04eaac48762508120fd2cc31fec66c2b42993bc1b0b9d99fc25",
        "bytes": 1100
      },
      {
        "path": "internalEvidenceRetained",
        "sha256": "e4b98bfe8f15889bd0edc649c9ea60d6418ed5ec47a5aadc05605590d6e25720",
        "bytes": 13630
      },
      {
        "path": "internalEvidenceRetained",
        "sha256": "405f9e46c2c48f168fd8a2f970bc5a522f391c67cd4634da38d00f7ee379ffef",
        "bytes": 4595
      }
    ],
    "limitations": [
      "No all-configurations qualification",
      "No other continuous behavior, visual or package retest"
    ]
  }
]
```

返回[族索引](../BlueprintNodeIndex.md)、[族限制](../Blueprint.md)和[验证说明](../../Developer/TestingAndEvidence.md)。
