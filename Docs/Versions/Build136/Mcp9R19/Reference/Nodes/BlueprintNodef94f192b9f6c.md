# 字符串转大写

用途：字符串转大写。

前提：按本页原签名写精确输入/输出与允许属性，连接输入不可同时设default；目录发现不代表任意配置实测。仅Actor单EventGraph。owner/member精确；变量Value占位换实际声明，Sequence/Switch按实际配置核端口。

限制：只支持原目录明确字段/类型/默认；隐藏或不可写引脚不得填写。运行/数值范围/画面按原具体组合证据，不能推导全验。

精确身份（绑定本包；不能凭序号跨版匹配）：

```json
{
  "assetKind": "Blueprint",
  "class": "/Script/BlueprintGraph.K2Node_CallFunction",
  "binding": {
    "owner": "/Script/Engine.KismetStringLibrary",
    "member": "ToUpper"
  }
}
```

当前原目录属性/允许默认/引脚与类型：

```json
{
  "class": "/Script/BlueprintGraph.K2Node_CallFunction",
  "properties": {
    "owner": "/Script/Engine.KismetStringLibrary",
    "member": "ToUpper"
  },
  "discovered": true,
  "actionDatabaseMatch": true,
  "constructible": true,
  "family": "StringTransforms",
  "minimumBlueprintRevision": 9,
  "requiresStringTransformsCatalog": true,
  "supportState": "experimental",
  "verified": "See regression evidence; discovery and instance construction do not imply compiled/runtime support",
  "variableType": "",
  "pins": [
    {
      "name": "self",
      "direction": "input",
      "category": "object",
      "subcategory": "None",
      "subtype": "/Script/Engine.KismetStringLibrary",
      "hidden": true,
      "defaultWritable": false,
      "defaultText": "",
      "defaultObject": "/Script/Engine.Default__KismetStringLibrary"
    },
    {
      "name": "SourceString",
      "direction": "input",
      "category": "string",
      "subcategory": "None",
      "subtype": "",
      "hidden": false,
      "defaultWritable": true,
      "defaultText": "",
      "defaultObject": ""
    },
    {
      "name": "ReturnValue",
      "direction": "output",
      "category": "string",
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
  "class": "/Script/BlueprintGraph.K2Node_CallFunction",
  "properties": {},
  "binding": {
    "kind": "Function",
    "owner": "/Script/Engine.KismetStringLibrary",
    "member": "ToUpper"
  }
}
```

这是该节点的声明片段，不是独立可生成完整图；按族规则补必需默认/输入/连接/输出。

来源：Plugins/NodeForge/Source/NodeForgeEditor/Private/NodeForgeNodeDefinitions.cpp。
历史记录保留原身份/配置/Build/层级，缺证据保持notEstablished：

```json
[
  {
    "verificationSource": "internalEvidenceRetained",
    "originalBuild": "Build58",
    "originalBinarySha256": "56d302e9bbef3f5cd528f896cb631a29cd5ad3d14e9b6b6fbd47b94548571aba",
    "originalCatalogDigest": "715528fbc5e20f55115457238f04eee9bc49354e602f9297f90f6b7c8ad1c6f3",
    "matchKind": "exactOwnerMemberAndOriginalInput",
    "record": {
      "logicalIds": [
        "UpperText"
      ],
      "checks": {
        "readback": "passedInListedGraphs",
        "compile": "passedInListedGraphs",
        "save": "passedInListedGraphs",
        "independentReopen": "passedInListedGraphs",
        "runtime": "passedExactMessageInListedBlueprint"
      },
      "caseNames": [
        "TransformCompose"
      ],
      "reports": [
        {
          "path": "internalEvidenceRetained",
          "sha256": "1b05396dd081088a55ded703dca414c7f560c70988dc654c8a6bd7a8b7a3b9d4",
          "bytes": 124052
        },
        {
          "path": "internalEvidenceRetained",
          "sha256": "69f45bdc2591379bed45a62ee4e20cd3ec22fe572746b88cab1f29ffe70d4aa7",
          "bytes": 50205
        },
        {
          "path": "internalEvidenceRetained",
          "sha256": "3d44e4253f7a6e5cb2b01dd2775e1354b2fae81f52fed366297ae54b94070937",
          "bytes": 34916
        },
        {
          "path": "internalEvidenceRetained",
          "sha256": "6cf37c1fa31834bc7ec188b254f5e4309f6d6a6f56161332bb4e9819af6fb695",
          "bytes": 47840
        },
        {
          "path": "internalEvidenceRetained",
          "sha256": "53c52ee50071e7f51244c9fc17081285140c4558e0fba1f00f3892cde23826e2",
          "bytes": 1947
        },
        {
          "path": "internalEvidenceRetained",
          "sha256": "f94f645b74cc131043a87b1abc1b366149e9552ab734448ec426d7c862cb9cce",
          "bytes": 12136
        },
        {
          "path": "internalEvidenceRetained",
          "sha256": "afd55303019e88fd108fa7b62377f4858fb580b46e51b9126495b60793d8266c",
          "bytes": 3159
        }
      ],
      "scope": "historical Build58 exact signatures/configuration only; current C runtime notRun"
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
