# 写入本Actor声明的float成员变量

用途：写入本Actor声明的float成员变量。

前提：按本页原签名写精确输入/输出与允许属性，连接输入不可同时设default；目录发现不代表任意配置实测。仅Actor单EventGraph。owner/member精确；变量Value占位换实际声明，Sequence/Switch按实际配置核端口。

限制：只支持原目录明确字段/类型/默认；隐藏或不可写引脚不得填写。运行/数值范围/画面按原具体组合证据，不能推导全验。

精确身份（绑定本包；不能凭序号跨版匹配）：

```json
{
  "assetKind": "Blueprint",
  "class": "/Script/BlueprintGraph.K2Node_VariableSet",
  "variableType": "float"
}
```

当前原目录属性/允许默认/引脚与类型：

```json
{
  "class": "/Script/BlueprintGraph.K2Node_VariableSet",
  "properties": {
    "variable": "Value"
  },
  "discovered": true,
  "actionDatabaseMatch": false,
  "constructible": true,
  "family": "ExistingCore",
  "supportState": "experimental",
  "verified": "See regression evidence; discovery and instance construction do not imply compiled/runtime support",
  "variableType": "float",
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
      "name": "Value",
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
      "name": "self",
      "direction": "input",
      "category": "object",
      "subcategory": "None",
      "subtype": "{selfBlueprint}",
      "hidden": true,
      "defaultWritable": false,
      "defaultText": "",
      "defaultObject": ""
    },
    {
      "name": "Output_Get",
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
  "class": "/Script/BlueprintGraph.K2Node_VariableSet",
  "properties": {},
  "binding": {
    "kind": "Member",
    "name": "Value"
  }
}
```

这是该节点的声明片段，不是独立可生成完整图；按族规则补必需默认/输入/连接/输出。

来源：Plugins/NodeForge/Source/NodeForgeEditor/Private/NodeForgeNodeDefinitions.cpp。
历史记录保留原身份/配置/Build/层级，缺证据保持notEstablished：

```json
[
  {
    "verificationSource": "Catalog/UE532GraphVerification.json",
    "originalBuild": "notExplicitInVerification",
    "originalBinarySha256": "517d7694c57b01ace5e7a89c9e1541a5e1b13cb0da06f29c97875ff7f5f514e2",
    "originalCatalogDigest": "0d786af7d63e1ea209691493e9b336f50e61eaeb45ef34eeec12ff01b22a884f",
    "matchKind": "nativeIdentity",
    "record": {
      "class": "/Script/BlueprintGraph.K2Node_VariableSet",
      "catalogBinding": {
        "variable": "Value"
      },
      "variableType": "float",
      "discovered": true,
      "constructible": true,
      "verification": "notVerified",
      "checks": {
        "readback": "notVerified",
        "compile": "notVerified",
        "save": "notVerified",
        "independentReopen": "notVerified"
      },
      "caseNames": [],
      "uses": [],
      "runtime": "notAssessedHere",
      "assetType": "Blueprint"
    }
  },
  {
    "verificationSource": "Catalog/UE532BlueprintMotionVerification.json",
    "originalBuild": "notExplicitInVerification",
    "originalBinarySha256": "b5475c6feb8d6ac7f7011c29b65636aee7711f2a9919d80c5edeb7e5e8e8f7fe",
    "originalCatalogDigest": "a6488c77da856b78bd72ac32d5c480008f34cdb897524c2296289f088893b786",
    "matchKind": "nativeIdentity",
    "record": {
      "assetType": "Blueprint",
      "class": "/Script/BlueprintGraph.K2Node_VariableSet",
      "binding": {
        "variable": "Value"
      },
      "variableType": "float",
      "discovered": true,
      "constructible": true,
      "compileSaveIndependentReopenVerified": false,
      "runtimeVerifiedInNamedCases": false,
      "compileEvidence": [],
      "runtimeEvidence": [],
      "scope": "Only named fixture bindings/properties/ports/topologies; not every pin, parameter or combination"
    }
  },
  {
    "verificationSource": "Catalog/UE532BlueprintFamiliesVerification.json",
    "originalBuild": "notExplicitInVerification",
    "originalBinarySha256": "799902583894dc830322bc87f9ad239e276794fd92709f97ddb104ce3453e8f0",
    "originalCatalogDigest": "6993743a0513489069423790f678c2cccd6bccc29435cd0dd98e4156248aa069",
    "matchKind": "nativeIdentity",
    "record": {
      "class": "/Script/BlueprintGraph.K2Node_VariableSet",
      "properties": {
        "variable": "Value"
      },
      "family": "ExistingCore",
      "nativeDiscovered": true,
      "nativeConstructible": true,
      "supportState": "experimental",
      "compileSaveIndependentReopen": [],
      "runtimeSampleEvidence": [],
      "scope": "Representative sample evidence; no per-node execution trace or complete configuration guarantee",
      "realUi": "notRun",
      "noPluginDelivery": "notRun",
      "variableType": "float"
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
    "logicalId": "SetValue",
    "configuration": {
      "class": "/Script/BlueprintGraph.K2Node_VariableSet",
      "properties": {
        "variable": "Value"
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
