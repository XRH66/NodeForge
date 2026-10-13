# 向量加法

用途：向量加法。

前提：按本页原签名写精确输入/输出与允许属性，连接输入不可同时设default；目录发现不代表任意配置实测。仅Actor单EventGraph。owner/member精确；变量Value占位换实际声明，Sequence/Switch按实际配置核端口。

限制：只支持原目录明确字段/类型/默认；隐藏或不可写引脚不得填写。运行/数值范围/画面按原具体组合证据，不能推导全验。

精确身份（绑定本包；不能凭序号跨版匹配）：

```json
{
  "assetKind": "Blueprint",
  "class": "/Script/BlueprintGraph.K2Node_CallFunction",
  "binding": {
    "owner": "/Script/Engine.KismetMathLibrary",
    "member": "Add_VectorVector"
  }
}
```

当前原目录属性/允许默认/引脚与类型：

```json
{
  "class": "/Script/BlueprintGraph.K2Node_CallFunction",
  "properties": {
    "owner": "/Script/Engine.KismetMathLibrary",
    "member": "Add_VectorVector"
  },
  "discovered": true,
  "actionDatabaseMatch": false,
  "constructible": true,
  "family": "VectorAndRotation",
  "minimumBlueprintRevision": 2,
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
      "category": "struct",
      "subcategory": "None",
      "subtype": "/Script/CoreUObject.Vector",
      "hidden": false,
      "defaultWritable": true,
      "defaultText": "0, 0, 0",
      "defaultObject": ""
    },
    {
      "name": "B",
      "direction": "input",
      "category": "struct",
      "subcategory": "None",
      "subtype": "/Script/CoreUObject.Vector",
      "hidden": false,
      "defaultWritable": true,
      "defaultText": "0, 0, 0",
      "defaultObject": ""
    },
    {
      "name": "ReturnValue",
      "direction": "output",
      "category": "struct",
      "subcategory": "None",
      "subtype": "/Script/CoreUObject.Vector",
      "hidden": false,
      "defaultWritable": false,
      "defaultText": "0, 0, 0",
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
    "member": "Add_VectorVector"
  }
}
```

这是该节点的声明片段，不是独立可生成完整图；按族规则补必需默认/输入/连接/输出。

来源：Plugins/NodeForge/Source/NodeForgeEditor/Private/NodeForgeNodeDefinitions.cpp。
历史记录保留原身份/配置/Build/层级，缺证据保持notEstablished：

```json
[
  {
    "verificationSource": "Catalog/UE532BlueprintFamiliesVerification.json",
    "originalBuild": "notExplicitInVerification",
    "originalBinarySha256": "799902583894dc830322bc87f9ad239e276794fd92709f97ddb104ce3453e8f0",
    "originalCatalogDigest": "6993743a0513489069423790f678c2cccd6bccc29435cd0dd98e4156248aa069",
    "matchKind": "nativeIdentity",
    "record": {
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetMathLibrary",
        "member": "Add_VectorVector"
      },
      "family": "VectorAndRotation",
      "nativeDiscovered": true,
      "nativeConstructible": true,
      "supportState": "verified",
      "compileSaveIndependentReopen": [
        "VectorBasis"
      ],
      "runtimeSampleEvidence": [
        "VectorBasis"
      ],
      "scope": "Representative sample evidence; no per-node execution trace or complete configuration guarantee",
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
  },
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
