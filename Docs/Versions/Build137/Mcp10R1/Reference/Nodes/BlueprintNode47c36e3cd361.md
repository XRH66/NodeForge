# 按Then_0起顺序触发2个执行出口

用途：按Then_0起顺序触发2个执行出口。

前提：按本页原签名写精确输入/输出与允许属性，连接输入不可同时设default；目录发现不代表任意配置实测。仅Actor单EventGraph。owner/member精确；变量Value占位换实际声明，Sequence/Switch按实际配置核端口。

限制：只支持原目录明确字段/类型/默认；隐藏或不可写引脚不得填写。运行/数值范围/画面按原具体组合证据，不能推导全验。

精确身份（绑定本包；不能凭序号跨版匹配）：

```json
{
  "assetKind": "Blueprint",
  "class": "/Script/BlueprintGraph.K2Node_ExecutionSequence",
  "configuration": {
    "outputCount": 2
  }
}
```

当前原目录属性/允许默认/引脚与类型：

```json
{
  "class": "/Script/BlueprintGraph.K2Node_ExecutionSequence",
  "properties": {
    "outputCount": 2
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
    "outputCount": 2
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
        "outputCount": 2
      },
      "nativeDiscovered": true,
      "actionDatabaseMatch": true,
      "nativeConstructible": true,
      "supportState": "verified",
      "compileSaveIndependentReopen": [
        "internalEvidenceRetained",
        "internalEvidenceRetained"
      ],
      "runtimeSampleEvidence": [
        "internalEvidenceRetained",
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
  },
  {
    "level": "representativeGraphGenerationExportAndIndependentVerify",
    "build": "Build132",
    "logicalId": "Sequence",
    "configuration": {
      "class": "/Script/BlueprintGraph.K2Node_ExecutionSequence",
      "properties": {
        "outputCount": 2
      }
    },
    "reports": {
      "input": {
        "path": "internalEvidenceRetained",
        "sha256": "42b2c2427aacc35a8416b0381ca25e9c0422dc96ccf2dfbb2074945e6d26280f",
        "bytes": 13973
      },
      "expected": {
        "path": "internalEvidenceRetained",
        "sha256": "8b0ab30020b736e9dde79f7392ab68be8b670300c77eb8bc43139912c74acc73",
        "bytes": 16020
      },
      "generation": {
        "path": "internalEvidenceRetained",
        "sha256": "fb9f56dad92797e1f45e9727c7cf4fa61c133471d8749a380a7057d64126a456",
        "bytes": 213887
      },
      "generationReview": {
        "path": "internalEvidenceRetained",
        "sha256": "1b5f21da25419923ed4b612d51af2b1a94c9ce9c72e5cc410ace86397fc0fc8a",
        "bytes": 696
      },
      "verify": {
        "path": "internalEvidenceRetained",
        "sha256": "1e9faa43479867cd3f52b4408d621a09302a7b57478fe19de3f295becd5123b9",
        "bytes": 143662
      },
      "verifyReview": {
        "path": "internalEvidenceRetained",
        "sha256": "168dcc4d0cc11bf4b64d64140f28417f81d4174385e05fde7141d8ea0962ccd8",
        "bytes": 527
      },
      "exportReview": {
        "path": "internalEvidenceRetained",
        "sha256": "e596dd3cb7e931bb4f8d15468dd9efe441afe69fa892f9c3901043f5e77059e5",
        "bytes": 909
      },
      "payload": {
        "path": "internalEvidenceRetained",
        "sha256": "e1dfe0007bcdb394a1950685bd5defd2224816356fbfaacba52fb4e55c59bac8",
        "bytes": 13845
      },
      "sourceAsset": {
        "path": "internalEvidenceRetained",
        "sha256": "21442814b37d46733ecaad3aea7bc1187ed1c5dde1cd90c21e9388a50ec2332f"
      },
      "receipt": {
        "path": "internalEvidenceRetained",
        "sha256": "d99b7a2348d9ba11eba7657dab53fcaccd1bf8d5ab366b02d4d038c8008b39a5"
      }
    },
    "runtime": "notRun",
    "visual": "notRun",
    "package": "notRun",
    "allConfigurationsVerified": false
  }
]
```

返回[族索引](../BlueprintNodeIndex.md)、[族限制](../Blueprint.md)和[验证说明](../../Developer/TestingAndEvidence.md)。
