# 原生纯计算：EqualEqual_StriStri

用途：原生纯计算：EqualEqual_StriStri。

前提：读取本页准确 owner/member 和 pins，使用本包完整 catalogDigest。输入、输出及 float/double 以原生签名为准，不能隐式转换；函数本身不推动执行流，结果需接入消费它的成员赋值、打印或其它计算。

限制：限两个受控原生库的已准入静态纯函数；不扩对象、数组、int64、FText、随机状态、隐藏上下文或 mutable ref。只给可写、未连接的可见输入设置准确类型默认值。原生用途：Test if the input strings are equal (A == B), ignoring case
@param A - The string to compare against
@param B - The string to compare
@returns True if the strings are equal, false otherwise；原生分类：Utilities|String。目录构造不等于所有组合运行通过。

精确身份（绑定本包；不能凭序号跨版匹配）：

```json
{
  "assetKind": "Blueprint",
  "class": "/Script/BlueprintGraph.K2Node_CallFunction",
  "binding": {
    "owner": "/Script/Engine.KismetStringLibrary",
    "member": "EqualEqual_StriStri"
  }
}
```

当前原目录属性/允许默认/引脚与类型：

```json
{
  "class": "/Script/BlueprintGraph.K2Node_CallFunction",
  "properties": {
    "owner": "/Script/Engine.KismetStringLibrary",
    "member": "EqualEqual_StriStri"
  },
  "discovered": true,
  "actionDatabaseMatch": true,
  "constructible": true,
  "family": "OrdinaryPureComputation",
  "minimumBlueprintRevision": 10,
  "nativeDescription": "Test if the input strings are equal (A == B), ignoring case\n@param A - The string to compare against\n@param B - The string to compare\n@returns True if the strings are equal, false otherwise",
  "nativeCategory": "Utilities|String",
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
      "name": "A",
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
      "name": "B",
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
      "category": "bool",
      "subcategory": "None",
      "subtype": "",
      "hidden": false,
      "defaultWritable": false,
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
    "owner": "/Script/Engine.KismetStringLibrary",
    "member": "EqualEqual_StriStri"
  }
}
```

这是准确单节点声明片段，使用原生默认；不是完整可提交图。完整配方见 AiNodeBulk。先按真实目录补必需连接，再用实际分配的 requestId/assetPath 和本版完整摘要预检。

来源：Plugins/NodeForge/Source/NodeForgeEditor/Private/NodeForgeNodeDefinitions.cpp。
历史记录保留原身份/配置/Build/层级，缺证据保持notEstablished：

```json
[]
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
