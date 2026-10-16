# DynamicWeightedMovingAverage_FVector

支持状态：**实验性（experimental）**。请先预检，再生成并检查实际结果。

UE 函数说明：

> Calculates the new value in a weighted moving average series using the previous value and a weight range.
> The weight range is used to dynamically adjust based upon distance between the samples
> This allows you to smooth a value more aggressively for small noise and let large movements be smoothed less (or vice versa)
> 
> @param CurrentSample - The value to blend with the previous sample to get a new weighted value
> @param PreviousSample - The last value from the series
> @param MaxDistance - Distance to use as the blend between min weight or max weight
> @param MinWeight - The weight use when the distance is small
> @param MaxWeight - The weight use when the distance is large
> 
> @return the next value in the series

## 使用方法

将下面的节点示例放入 Actor 蓝图的 EventGraph。按本页给出的名称和类型连接引脚；已连接的输入不要再填写 `defaults`。隐藏引脚或 `defaultWritable=false` 的引脚不能设置默认值。

这是纯计算节点，没有执行引脚。请把结果连接到使用它的其他节点，例如变量赋值、打印或下一步计算。NodeForge 不会自动转换引脚类型。

## 节点示例（JSON v3）

```json
{
  "id": "ExampleNode",
  "class": "/Script/BlueprintGraph.K2Node_CallFunction",
  "properties": {},
  "binding": {
    "kind": "Function",
    "owner": "/Script/Engine.KismetMathLibrary",
    "member": "DynamicWeightedMovingAverage_FVector"
  }
}
```

这是单个节点片段。完整请求还需要目标路径、节点连线和资产设置，参见[JSON 编写说明](../GraphAuthoring.md)和[完整示例](../../Ai/JsonArchitectureExamples.md)。

## 节点参数

以下 JSON 列出 UE 类名、函数或变量的绑定，以及引脚类型和默认值。对于函数、事件和变量，目录中的 `properties` 包含 v2 的绑定信息；编写 v3 时，这些绑定使用上方示例中的 `binding` 字段。

```json
{
  "class": "/Script/BlueprintGraph.K2Node_CallFunction",
  "properties": {
    "owner": "/Script/Engine.KismetMathLibrary",
    "member": "DynamicWeightedMovingAverage_FVector"
  },
  "discovered": true,
  "actionDatabaseMatch": true,
  "constructible": true,
  "family": "OrdinaryPureComputation",
  "minimumBlueprintRevision": 10,
  "nativeDescription": "Calculates the new value in a weighted moving average series using the previous value and a weight range.\nThe weight range is used to dynamically adjust based upon distance between the samples\nThis allows you to smooth a value more aggressively for small noise and let large movements be smoothed less (or vice versa)\n\n@param CurrentSample - The value to blend with the previous sample to get a new weighted value\n@param PreviousSample - The last value from the series\n@param MaxDistance - Distance to use as the blend between min weight or max weight\n@param MinWeight - The weight use when the distance is small\n@param MaxWeight - The weight use when the distance is large\n\n@return the next value in the series",
  "nativeCategory": "Math|Smoothing",
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
      "name": "CurrentSample",
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
      "name": "PreviousSample",
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
      "name": "MaxDistance",
      "direction": "input",
      "category": "real",
      "subcategory": "float",
      "subtype": "",
      "hidden": false,
      "defaultWritable": true,
      "defaultText": "0.0",
      "defaultObject": ""
    },
    {
      "name": "MinWeight",
      "direction": "input",
      "category": "real",
      "subcategory": "float",
      "subtype": "",
      "hidden": false,
      "defaultWritable": true,
      "defaultText": "0.0",
      "defaultObject": ""
    },
    {
      "name": "MaxWeight",
      "direction": "input",
      "category": "real",
      "subcategory": "float",
      "subtype": "",
      "hidden": false,
      "defaultWritable": true,
      "defaultText": "0.0",
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

## 节点标识

```json
{
  "assetKind": "Blueprint",
  "class": "/Script/BlueprintGraph.K2Node_CallFunction",
  "binding": {
    "owner": "/Script/Engine.KismetMathLibrary",
    "member": "DynamicWeightedMovingAverage_FVector"
  }
}
```

返回[节点列表](../BlueprintNodeIndex.md) · 阅读[蓝图使用范围](../Blueprint.md)。
