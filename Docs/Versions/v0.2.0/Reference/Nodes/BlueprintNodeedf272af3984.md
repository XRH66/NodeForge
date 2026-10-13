# 按Name case选择执行出口；本页是目录代表配置

支持状态：**实验性（experimental）**。请先预检，再生成并检查实际结果。

## 使用方法

将下面的节点示例放入 Actor 蓝图的 EventGraph。按本页给出的名称和类型连接引脚；已连接的输入不要再填写 `defaults`。隐藏引脚或 `defaultWritable=false` 的引脚不能设置默认值。

`cases` 列出 1–8 个名称分支，名称不能只在大小写上不同。`hasDefault=false` 时，未匹配的执行不会继续到默认分支。

## 节点示例（JSON v3）

```json
{
  "id": "ExampleNode",
  "class": "/Script/BlueprintGraph.K2Node_SwitchName",
  "properties": {
    "hasDefault": false,
    "cases": [
      "CaseA",
      "CaseB"
    ]
  }
}
```

这是单个节点片段。完整请求还需要目标路径、节点连线和资产设置，参见[JSON 编写说明](../GraphAuthoring.md)和[完整示例](../../Ai/JsonArchitectureExamples.md)。

## 节点参数

以下 JSON 列出 UE 类名、函数或变量的绑定，以及引脚类型和默认值。对于函数、事件和变量，目录中的 `properties` 包含 v2 的绑定信息；编写 v3 时，这些绑定使用上方示例中的 `binding` 字段。

```json
{
  "class": "/Script/BlueprintGraph.K2Node_SwitchName",
  "properties": {
    "hasDefault": false,
    "cases": [
      "CaseA",
      "CaseB"
    ]
  },
  "discovered": true,
  "actionDatabaseMatch": true,
  "constructible": true,
  "family": "MultiwaySelection",
  "caseAdapter": {
    "kind": "switchName",
    "minimumCaseCount": 1,
    "maximumCaseCount": 8,
    "casePinNames": "exact cases[i] display spelling; ASCII letter then letters/digits, length<=64, unique by native case-insensitive FName; None/Default/execute/Selection/comparison function names reserved",
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
      "category": "name",
      "subcategory": "None",
      "subtype": "",
      "hidden": false,
      "defaultWritable": true,
      "defaultText": "None",
      "defaultObject": "",
      "defaultMaximumLength": 1023
    },
    {
      "name": "NotEqual_NameName",
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
      "name": "CaseA",
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
      "name": "CaseB",
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

## 节点标识

```json
{
  "assetKind": "Blueprint",
  "class": "/Script/BlueprintGraph.K2Node_SwitchName",
  "configuration": {
    "hasDefault": false,
    "cases": [
      "CaseA",
      "CaseB"
    ]
  }
}
```

返回[节点列表](../BlueprintNodeIndex.md) · 阅读[蓝图使用范围](../Blueprint.md)。
