# 按整数case选择执行出口；本页是目录代表配置

用途：按整数case选择执行出口；本页是目录代表配置。

前提：按本页原签名写精确输入/输出与允许属性，连接输入不可同时设default；目录发现不代表任意配置实测。仅Actor单EventGraph。owner/member精确；变量Value占位换实际声明，Sequence/Switch按实际配置核端口。

限制：只支持原目录明确字段/类型/默认；隐藏或不可写引脚不得填写。运行/数值范围/画面按原具体组合证据，不能推导全验。

精确身份（绑定本包；不能凭序号跨版匹配）：

```json
{
  "assetKind": "Blueprint",
  "class": "/Script/BlueprintGraph.K2Node_SwitchInteger",
  "configuration": {
    "hasDefault": false,
    "startIndex": 0,
    "caseCount": 2
  }
}
```

当前原目录属性/允许默认/引脚与类型：

```json
{
  "class": "/Script/BlueprintGraph.K2Node_SwitchInteger",
  "properties": {
    "hasDefault": false,
    "startIndex": 0,
    "caseCount": 2
  },
  "discovered": true,
  "actionDatabaseMatch": true,
  "constructible": true,
  "family": "MultiwaySelection",
  "caseAdapter": {
    "kind": "switchInteger",
    "minimumCaseCount": 1,
    "maximumCaseCount": 8,
    "casePinNames": "native decimal startIndex+i in case order; startIndex+caseCount<=2147483647; no int32 increment overflow",
    "hasDefaultRequired": true,
    "signature": "Native sample input/default signatures; case output names/count specialize from properties and are fully checked against each actual constructed instance. Hidden comparison context remains native and cannot be connected/configured."
  },
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
      "name": "Selection",
      "direction": "input",
      "category": "int",
      "subcategory": "None",
      "subtype": "",
      "hidden": false,
      "defaultWritable": true,
      "defaultText": "0",
      "defaultObject": "",
      "defaultMinimum": -2147483648,
      "defaultMaximum": 2147483647,
      "defaultIntegral": true
    },
    {
      "name": "NotEqual_IntInt",
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
      "name": "0",
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
      "name": "1",
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
  "class": "/Script/BlueprintGraph.K2Node_SwitchInteger",
  "properties": {
    "hasDefault": false,
    "startIndex": 0,
    "caseCount": 2
  }
}
```

这是该节点的声明片段，不是独立可生成完整图；按族规则补必需默认/输入/连接/输出。

来源：Plugins/NodeForge/Source/NodeForgeEditor/Private/NodeForgeNodeDefinitions.cpp。
历史记录保留原身份/配置/Build/层级，缺证据保持notEstablished：

```json
[
  {
    "verificationSource": "Catalog/UE532BlueprintSwitchesVerification.json",
    "originalBuild": "Build52",
    "originalBinarySha256": "01fdbe3ab986fbd75caebdb82073ed24aab205e219e9fe8df00bc869e65affd7",
    "originalCatalogDigest": "1ae2791da31e336b3d147d1ed20e173ddfe51e721d255df040c843c743171317",
    "matchKind": "representativeClassConfiguration",
    "record": {
      "class": "/Script/BlueprintGraph.K2Node_SwitchInteger",
      "nativeDiscovered": true,
      "actionDatabaseMatch": true,
      "nativeConstructible": true,
      "supportState": "verified",
      "representativeCaseCounts": [
        1,
        2,
        3,
        8
      ],
      "experimentalOtherAllowedCounts": [
        4,
        5,
        6,
        7
      ],
      "allowedCaseCounts": [
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8
      ],
      "nativeHasDefaultConfigurations": [
        false,
        true
      ],
      "compileSaveIndependentReopen": [
        "internalEvidenceRetained",
        "internalEvidenceRetained",
        "internalEvidenceRetained",
        "internalEvidenceRetained",
        "internalEvidenceRetained",
        "internalEvidenceRetained",
        "internalEvidenceRetained",
        "internalEvidenceRetained",
        "internalEvidenceRetained",
        "internalEvidenceRetained",
        "internalEvidenceRetained",
        "internalEvidenceRetained"
      ],
      "nativeIndependentRuntime": [
        "internalEvidenceRetained",
        "internalEvidenceRetained",
        "internalEvidenceRetained",
        "internalEvidenceRetained",
        "internalEvidenceRetained",
        "internalEvidenceRetained",
        "internalEvidenceRetained",
        "internalEvidenceRetained",
        "internalEvidenceRetained"
      ],
      "exportCopyIndependentReopen": "internalEvidenceRetained",
      "abnormalSourceGuards": "internalEvidenceRetained",
      "scope": "Representative bounded configurations, exact requested case labels and pins, native execution; no universal UE node coverage",
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
