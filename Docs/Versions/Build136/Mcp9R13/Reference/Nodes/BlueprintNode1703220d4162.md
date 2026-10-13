# 双精度最大值

用途：双精度最大值。

前提：按本页原签名写精确输入/输出与允许属性，连接输入不可同时设default；目录发现不代表任意配置实测。仅Actor单EventGraph。owner/member精确；变量Value占位换实际声明，Sequence/Switch按实际配置核端口。

限制：只支持原目录明确字段/类型/默认；隐藏或不可写引脚不得填写。运行/数值范围/画面按原具体组合证据，不能推导全验。

精确身份（绑定本包；不能凭序号跨版匹配）：

```json
{
  "assetKind": "Blueprint",
  "class": "/Script/BlueprintGraph.K2Node_CallFunction",
  "binding": {
    "owner": "/Script/Engine.KismetMathLibrary",
    "member": "FMax"
  }
}
```

当前原目录属性/允许默认/引脚与类型：

```json
{
  "class": "/Script/BlueprintGraph.K2Node_CallFunction",
  "properties": {
    "owner": "/Script/Engine.KismetMathLibrary",
    "member": "FMax"
  },
  "discovered": true,
  "actionDatabaseMatch": false,
  "constructible": true,
  "family": "ScalarMathInterpolation",
  "minimumBlueprintRevision": 7,
  "requiresMathCatalog": true,
  "supportState": "experimental",
  "verified": "See regression evidence; discovery and instance construction do not imply compiled/runtime support",
  "variableType": "",
  "pins": [
    {
      "name": "self",
      "direction": "input",
      "category": "object",
      "subcategory": "None",
      "subtype": "/Script/Engine.KismetMathLibrary",
      "hidden": true,
      "defaultWritable": false,
      "defaultText": "",
      "defaultObject": "/Script/Engine.Default__KismetMathLibrary"
    },
    {
      "name": "A",
      "direction": "input",
      "category": "real",
      "subcategory": "double",
      "subtype": "",
      "hidden": false,
      "defaultWritable": true,
      "defaultText": "0.0",
      "defaultObject": ""
    },
    {
      "name": "B",
      "direction": "input",
      "category": "real",
      "subcategory": "double",
      "subtype": "",
      "hidden": false,
      "defaultWritable": true,
      "defaultText": "0.0",
      "defaultObject": ""
    },
    {
      "name": "ReturnValue",
      "direction": "output",
      "category": "real",
      "subcategory": "double",
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
  "id": "ExampleNode",
  "class": "/Script/BlueprintGraph.K2Node_CallFunction",
  "properties": {},
  "binding": {
    "kind": "Function",
    "owner": "/Script/Engine.KismetMathLibrary",
    "member": "FMax"
  }
}
```

这是该节点的声明片段，不是独立可生成完整图；按族规则补必需默认/输入/连接/输出。

来源：Plugins/NodeForge/Source/NodeForgeEditor/Private/NodeForgeNodeDefinitions.cpp。
历史记录保留原身份/配置/Build/层级，缺证据保持notEstablished：

```json
[
  {
    "verificationSource": "Catalog/UE532BlueprintMathInterpolationVerification.json",
    "originalBuild": "Build46",
    "originalBinarySha256": "4d0b3ac98ab1a9b9ecff5e3eb5fae7aaece62ac8138699c0084a2243a6b88910",
    "originalCatalogDigest": "aec5b1e30e736a0a16b069b7567817d23a8e9ca9a583aa951ee538e6e73c24c7",
    "matchKind": "nativeIdentity",
    "record": {
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetMathLibrary",
        "member": "FMax"
      },
      "supportState": "verified",
      "nativeDiscovered": true,
      "nativeConstructible": true,
      "actionDatabaseMatch": false,
      "compileSaveIndependentReopen": [
        "internalEvidenceRetained",
        "internalEvidenceRetained",
        "internalEvidenceRetained"
      ],
      "runtimeEvidence": [
        "internalEvidenceRetained",
        "internalEvidenceRetained",
        "internalEvidenceRetained"
      ],
      "export": "internalEvidenceRetained",
      "scope": "Representative finite numeric inputs, exact native ports, Actor transforms and attributed messages; not all numeric domains",
      "realUi": "notRun",
      "externalAiOriginal": "notRun",
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
