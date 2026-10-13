# BuildString_Int

支持状态：**实验性（experimental）**。请先预检，再生成并检查实际结果。

UE 函数说明：

> Converts a int->string, creating a new string in the form AppendTo+Prefix+InInt+Suffix
> @param AppendTo - An existing string to use as the start of the conversion string
> @param Prefix - A string to use as a prefix, after the AppendTo string
> @param InInt - The int value to convert
> @param Suffix - A suffix to append to the end of the conversion string
> @return A new string built from the passed parameters

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
    "owner": "/Script/Engine.KismetStringLibrary",
    "member": "BuildString_Int"
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
    "owner": "/Script/Engine.KismetStringLibrary",
    "member": "BuildString_Int"
  },
  "discovered": true,
  "actionDatabaseMatch": true,
  "constructible": true,
  "family": "OrdinaryPureComputation",
  "minimumBlueprintRevision": 10,
  "nativeDescription": "Converts a int->string, creating a new string in the form AppendTo+Prefix+InInt+Suffix\n@param AppendTo - An existing string to use as the start of the conversion string\n@param Prefix - A string to use as a prefix, after the AppendTo string\n@param InInt - The int value to convert\n@param Suffix - A suffix to append to the end of the conversion string\n@return A new string built from the passed parameters",
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
      "name": "AppendTo",
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
      "name": "Prefix",
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
      "name": "InInt",
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
      "name": "Suffix",
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
      "category": "string",
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
  "class": "/Script/BlueprintGraph.K2Node_CallFunction",
  "binding": {
    "owner": "/Script/Engine.KismetStringLibrary",
    "member": "BuildString_Int"
  }
}
```

返回[节点列表](../BlueprintNodeIndex.md) · 阅读[蓝图使用范围](../Blueprint.md)。
