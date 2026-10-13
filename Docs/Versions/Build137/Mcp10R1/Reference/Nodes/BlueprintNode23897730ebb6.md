# 双精度乘法

用途：双精度乘法。

前提：按本页原签名写精确输入/输出与允许属性，连接输入不可同时设default；目录发现不代表任意配置实测。仅Actor单EventGraph。owner/member精确；变量Value占位换实际声明，Sequence/Switch按实际配置核端口。

限制：只支持原目录明确字段/类型/默认；隐藏或不可写引脚不得填写。运行/数值范围/画面按原具体组合证据，不能推导全验。

精确身份（绑定本包；不能凭序号跨版匹配）：

```json
{
  "assetKind": "Blueprint",
  "class": "/Script/BlueprintGraph.K2Node_CallFunction",
  "binding": {
    "owner": "/Script/Engine.KismetMathLibrary",
    "member": "Multiply_DoubleDouble"
  }
}
```

当前原目录属性/允许默认/引脚与类型：

```json
{
  "class": "/Script/BlueprintGraph.K2Node_CallFunction",
  "properties": {
    "owner": "/Script/Engine.KismetMathLibrary",
    "member": "Multiply_DoubleDouble"
  },
  "discovered": true,
  "actionDatabaseMatch": false,
  "constructible": true,
  "family": "ExistingCore",
  "minimumBlueprintRevision": 0,
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
    "member": "Multiply_DoubleDouble"
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
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "catalogBinding": {
        "owner": "/Script/Engine.KismetMathLibrary",
        "member": "Multiply_DoubleDouble"
      },
      "variableType": "",
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
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "binding": {
        "owner": "/Script/Engine.KismetMathLibrary",
        "member": "Multiply_DoubleDouble"
      },
      "variableType": "",
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
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetMathLibrary",
        "member": "Multiply_DoubleDouble"
      },
      "family": "ExistingCore",
      "nativeDiscovered": true,
      "nativeConstructible": true,
      "supportState": "experimental",
      "compileSaveIndependentReopen": [],
      "runtimeSampleEvidence": [],
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
  }
]
```

返回[族索引](../BlueprintNodeIndex.md)、[族限制](../Blueprint.md)和[验证说明](../../Developer/TestingAndEvidence.md)。
