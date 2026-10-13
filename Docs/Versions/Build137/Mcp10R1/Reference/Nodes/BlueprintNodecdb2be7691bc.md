# 按Then_0起顺序触发8个执行出口

用途：按Then_0起顺序触发8个执行出口。

前提：按本页原签名写精确输入/输出与允许属性，连接输入不可同时设default；目录发现不代表任意配置实测。仅Actor单EventGraph。owner/member精确；变量Value占位换实际声明，Sequence/Switch按实际配置核端口。

限制：只支持原目录明确字段/类型/默认；隐藏或不可写引脚不得填写。运行/数值范围/画面按原具体组合证据，不能推导全验。

精确身份（绑定本包；不能凭序号跨版匹配）：

```json
{
  "assetKind": "Blueprint",
  "class": "/Script/BlueprintGraph.K2Node_ExecutionSequence",
  "configuration": {
    "outputCount": 8
  }
}
```

当前原目录属性/允许默认/引脚与类型：

```json
{
  "class": "/Script/BlueprintGraph.K2Node_ExecutionSequence",
  "properties": {
    "outputCount": 8
  },
  "discovered": true,
  "actionDatabaseMatch": true,
  "constructible": true,
  "family": "ExecutionSequence",
  "supportState": "experimental",
  "verified": "See regression evidence; discovery and instance construction do not imply compiled/runtime support",
  "variableType": "",
  "pins": [
    {
      "name": "execute",
      "direction": "input",
      "category": "exec",
      "subcategory": "None",
      "subtype": "",
      "hidden": false,
      "defaultWritable": false,
      "defaultText": "",
      "defaultObject": ""
    },
    {
      "name": "then_0",
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
      "name": "then_1",
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
      "name": "then_2",
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
      "name": "then_3",
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
      "name": "then_4",
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
      "name": "then_5",
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
      "name": "then_6",
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
      "name": "then_7",
      "direction": "output",
      "category": "exec",
      "subcategory": "None",
      "subtype": "",
      "hidden": false,
      "defaultWritable": false,
      "defaultText": "",
      "defaultObject": ""
    }
  ]
}
```

当前v3声明片段（v2使用下方历史绑定表示）：

```json
{
  "id": "ExampleNode",
  "class": "/Script/BlueprintGraph.K2Node_ExecutionSequence",
  "properties": {
    "outputCount": 8
  }
}
```

这是该节点的声明片段，不是独立可生成完整图；按族规则补必需默认/输入/连接/输出。

来源：Plugins/NodeForge/Source/NodeForgeEditor/Private/NodeForgeNodeDefinitions.cpp。
历史记录保留原身份/配置/Build/层级，缺证据保持notEstablished：

```json
[
  {
    "verificationSource": "Catalog/UE532BlueprintSequenceVerification.json",
    "originalBuild": "Build41",
    "originalBinarySha256": "b404718da17a0c2f9da1af5d0bf4f3c45506182cdd394b8533cf76e294a843cd",
    "originalCatalogDigest": "57b27415fee08ce08df3b734d9ea8fbd7a03678564f218007a0f770c3e478b7d",
    "matchKind": "nativeIdentity",
    "record": {
      "class": "/Script/BlueprintGraph.K2Node_ExecutionSequence",
      "properties": {
        "outputCount": 8
      },
      "nativeDiscovered": true,
      "actionDatabaseMatch": true,
      "nativeConstructible": true,
      "supportState": "verified",
      "compileSaveIndependentReopen": [
        "internalEvidenceRetained"
      ],
      "runtimeSampleEvidence": [
        "internalEvidenceRetained"
      ],
      "exportCopyIndependentReopen": [
        "internalEvidenceRetained"
      ],
      "scope": "Representative exact native output configuration; 4..7 have instance probes and rule export only, not saved graph or runtime verification",
      "realUi": "notRun",
      "noPluginDelivery": "notRun"
    }
  }
]
```

本期资料引用的实际证据（无条目时 notEstablished；签名证据不能代替组合编译/保存/Verify）：

```json
[
  {
    "level": "nativeConstructionAndExactSignature",
    "build": "Build132",
    "report": {
      "path": "internalEvidenceRetained",
      "sha256": "f927284177028bab2fa57677ecb4e614497e0fca1964e688a484c6b60f5767ad",
      "bytes": 582956
    },
    "overallStatus": "passed",
    "meaning": "actual production spawner/factory/catalog qualification; not representative compile/save/Verify/runtime"
  }
]
```

返回[族索引](../BlueprintNodeIndex.md)、[族限制](../Blueprint.md)和[验证说明](../../Developer/TestingAndEvidence.md)。
