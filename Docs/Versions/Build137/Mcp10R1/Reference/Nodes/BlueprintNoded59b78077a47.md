# 设置本Actor根组件世界旋转

用途：设置本Actor根组件世界旋转。

前提：按本页原签名写精确输入/输出与允许属性，连接输入不可同时设default；目录发现不代表任意配置实测。仅Actor单EventGraph。owner/member精确；变量Value占位换实际声明，Sequence/Switch按实际配置核端口。self必须由本图K2_GetRootComponent.ReturnValue直接连入。

限制：只支持原目录明确字段/类型/默认；隐藏或不可写引脚不得填写。运行/数值范围/画面按原具体组合证据，不能推导全验。

精确身份（绑定本包；不能凭序号跨版匹配）：

```json
{
  "assetKind": "Blueprint",
  "class": "/Script/BlueprintGraph.K2Node_CallFunction",
  "binding": {
    "owner": "/Script/Engine.SceneComponent",
    "member": "K2_SetWorldRotation"
  }
}
```

当前原目录属性/允许默认/引脚与类型：

```json
{
  "class": "/Script/BlueprintGraph.K2Node_CallFunction",
  "properties": {
    "owner": "/Script/Engine.SceneComponent",
    "member": "K2_SetWorldRotation"
  },
  "discovered": true,
  "actionDatabaseMatch": true,
  "constructible": true,
  "family": "SelfRootComponent",
  "minimumBlueprintRevision": 3,
  "supportState": "experimental",
  "verified": "See regression evidence; discovery and instance construction do not imply compiled/runtime support",
  "variableType": "",
  "functionContext": "External-member SceneComponent call: self requires exactly one direct same-graph Actor.K2_GetRootComponent.ReturnValue; exact native SceneComponent object type; no object/string/text defaults or other sources",
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
      "name": "self",
      "direction": "input",
      "category": "object",
      "subcategory": "None",
      "subtype": "/Script/Engine.SceneComponent",
      "hidden": false,
      "defaultWritable": false,
      "defaultText": "",
      "defaultObject": ""
    },
    {
      "name": "NewRotation",
      "direction": "input",
      "category": "struct",
      "subcategory": "None",
      "subtype": "/Script/CoreUObject.Rotator",
      "hidden": false,
      "defaultWritable": true,
      "defaultText": "0, 0, 0",
      "defaultObject": ""
    },
    {
      "name": "bSweep",
      "direction": "input",
      "category": "bool",
      "subcategory": "None",
      "subtype": "",
      "hidden": false,
      "defaultWritable": true,
      "defaultText": "false",
      "defaultObject": ""
    },
    {
      "name": "SweepHitResult",
      "direction": "output",
      "category": "struct",
      "subcategory": "None",
      "subtype": "/Script/Engine.HitResult",
      "hidden": false,
      "defaultWritable": false,
      "defaultText": "",
      "defaultObject": ""
    },
    {
      "name": "bTeleport",
      "direction": "input",
      "category": "bool",
      "subcategory": "None",
      "subtype": "",
      "hidden": false,
      "defaultWritable": true,
      "defaultText": "false",
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
    "owner": "/Script/Engine.SceneComponent",
    "member": "K2_SetWorldRotation"
  }
}
```

这是该节点的声明片段，不是独立可生成完整图；按族规则补必需默认/输入/连接/输出。

来源：Plugins/NodeForge/Source/NodeForgeEditor/Private/NodeForgeNodeDefinitions.cpp。
历史记录保留原身份/配置/Build/层级，缺证据保持notEstablished：

```json
[
  {
    "verificationSource": "Catalog/UE532BlueprintComponentsVerification.json",
    "originalBuild": "Build39",
    "originalBinarySha256": "aafb078b09f2e69bcc701a00ce89f89574462d43b354d85bb9465f0d39bb5392",
    "originalCatalogDigest": "7dea96da738f0f1b526e7782ec08230a4e0d6d14e815db2538638693f2309326",
    "matchKind": "nativeIdentity",
    "record": {
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.SceneComponent",
        "member": "K2_SetWorldRotation"
      },
      "family": "SelfRootComponent",
      "nativeDiscovered": true,
      "actionDatabaseMatch": true,
      "nativeConstructible": true,
      "supportState": "verified",
      "compileSaveIndependentReopen": [
        "internalEvidenceRetained",
        "internalEvidenceRetained",
        "internalEvidenceRetained"
      ],
      "runtimeSampleEvidence": [
        "internalEvidenceRetained",
        "internalEvidenceRetained",
        "internalEvidenceRetained"
      ],
      "scope": "Representative own default root component configurations, bSweep=false and bTeleport=true where present; no per-node execution trace or physics/collision guarantee",
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
