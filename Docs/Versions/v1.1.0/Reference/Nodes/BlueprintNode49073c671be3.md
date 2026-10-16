# 写入本Actor声明的rotator成员变量

支持状态：**实验性（experimental）**。请先预检，再生成并检查实际结果。

## 使用方法

将下面的节点示例放入 Actor 蓝图的 EventGraph。按本页给出的名称和类型连接引脚；已连接的输入不要再填写 `defaults`。隐藏引脚或 `defaultWritable=false` 的引脚不能设置默认值。

示例中的 `Value` 是变量名占位符。先在蓝图的 `asset.members.variables` 中声明变量，再把节点绑定的名称改为该变量名；类型必须一致。

## 节点示例（JSON v3）

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

这是单个节点片段。完整请求还需要目标路径、节点连线和资产设置，参见[JSON 编写说明](../GraphAuthoring.md)和[完整示例](../../Ai/JsonArchitectureExamples.md)。

## 节点参数

以下 JSON 列出 UE 类名、函数或变量的绑定，以及引脚类型和默认值。对于函数、事件和变量，目录中的 `properties` 包含 v2 的绑定信息；编写 v3 时，这些绑定使用上方示例中的 `binding` 字段。

```json
{
  "class": "/Script/BlueprintGraph.K2Node_VariableSet",
  "properties": {
    "variable": "Value"
  },
  "discovered": true,
  "actionDatabaseMatch": false,
  "constructible": true,
  "family": "RotatorMembers",
  "supportState": "experimental",
  "verified": "See regression evidence; discovery and instance construction do not imply compiled/runtime support",
  "variableType": "rotator",
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
      "category": "struct",
      "subcategory": "None",
      "subtype": "/Script/CoreUObject.Rotator",
      "hidden": false,
      "defaultWritable": true,
      "defaultText": "0, 0, 0",
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
      "category": "struct",
      "subcategory": "None",
      "subtype": "/Script/CoreUObject.Rotator",
      "hidden": false,
      "defaultWritable": false,
      "defaultText": "0, 0, 0",
      "defaultObject": ""
    }
  ]
}
```

## 节点标识

```json
{
  "assetKind": "Blueprint",
  "class": "/Script/BlueprintGraph.K2Node_VariableSet",
  "variableType": "rotator"
}
```

返回[节点列表](../BlueprintNodeIndex.md) · 阅读[蓝图使用范围](../Blueprint.md)。
