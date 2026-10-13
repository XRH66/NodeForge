# 按整数case选择执行出口；本页是目录代表配置

支持状态：**实验性（experimental）**。请先预检，再生成并检查实际结果。

## 使用方法

将下面的节点示例放入 Actor 蓝图的 EventGraph。按本页给出的名称和类型连接引脚；已连接的输入不要再填写 `defaults`。隐藏引脚或 `defaultWritable=false` 的引脚不能设置默认值。

`startIndex` 是起始整数，`caseCount` 支持 1–8 个连续分支，`hasDefault` 决定是否保留默认分支。修改配置后，请查询对应的引脚名称再连线。

## 节点示例（JSON v3）

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

这是单个节点片段。完整请求还需要目标路径、节点连线和资产设置，参见[JSON 编写说明](../GraphAuthoring.md)和[完整示例](../../Ai/JsonArchitectureExamples.md)。

## 节点参数

以下 JSON 列出 UE 类名、函数或变量的绑定，以及引脚类型和默认值。对于函数、事件和变量，目录中的 `properties` 包含 v2 的绑定信息；编写 v3 时，这些绑定使用上方示例中的 `binding` 字段。

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

## 节点标识

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

返回[节点列表](../BlueprintNodeIndex.md) · 阅读[蓝图使用范围](../Blueprint.md)。
