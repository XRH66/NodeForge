# 材质与蓝图的节点组合示例

以下是 NodeForge v1.1.1 的完整输入示例。使用前读取当前目录，选择未占用的 requestId 和目标路径，并按 [调用流程](QuickStart.md)执行预检与生成。示例中的资源路径、会话和 SHA 占位值需要换成当前实际返回的值。

示例如果引用资源，先用 resources_inspect 检查已保存且未修改的对象、参数和函数签名。全零 assetSha256 是占位值，必须替换为这次检查返回的真实 SHA。如果资源或引脚不匹配，先调整任务输入，不要改写来源报告。具体步骤见 [资源查询与引用](Resources.md)。

材质示例将标量、颜色与数学节点连接到 Unlit 输出。蓝图示例组合纯函数、变量、Branch/Switch、消息和自身根组件。准确引脚、默认值与属性以当前节点页和目录为准；这些示例不代表任意组合都经过运行检查。

## Material：标量、颜色与数学链

```json
{
  "format": "NodeForgeGraph",
  "version": 2,
  "requestId": "ExampleMaterial3",
  "assetType": "Material",
  "assetPath": "/Game/NodeForgeTests/Examples/Sample3",
  "catalogDigest": "be97bf57d226778102038c261c556b89b262197c78158fe16ca293de709d8847",
  "nodes": [
    {
      "id": "Scalar",
      "class": "/Script/Engine.MaterialExpressionConstant",
      "properties": {
        "R": 0.25
      }
    },
    {
      "id": "Color",
      "class": "/Script/Engine.MaterialExpressionConstant3Vector",
      "properties": {
        "Constant": [
          0.25,
          0.5,
          1,
          1
        ]
      }
    },
    {
      "id": "Sqrt",
      "class": "/Script/Engine.MaterialExpressionSquareRoot",
      "properties": {}
    },
    {
      "id": "Log",
      "class": "/Script/Engine.MaterialExpressionLogarithm2",
      "properties": {}
    },
    {
      "id": "Exp",
      "class": "/Script/Engine.MaterialExpressionExponential2",
      "properties": {}
    },
    {
      "id": "Round",
      "class": "/Script/Engine.MaterialExpressionRound",
      "properties": {}
    },
    {
      "id": "Trunc",
      "class": "/Script/Engine.MaterialExpressionTruncate",
      "properties": {}
    },
    {
      "id": "Tangent",
      "class": "/Script/Engine.MaterialExpressionTangent",
      "properties": {
        "Period": 1
      }
    },
    {
      "id": "Arc",
      "class": "/Script/Engine.MaterialExpressionArctangent",
      "properties": {}
    },
    {
      "id": "Inverse",
      "class": "/Script/Engine.MaterialExpressionInverseLinearInterpolate",
      "properties": {
        "ConstA": 0,
        "ConstB": 1,
        "ConstValue": 0.5,
        "bClampResult": true
      }
    },
    {
      "id": "Ceil",
      "class": "/Script/Engine.MaterialExpressionCeil",
      "properties": {}
    },
    {
      "id": "Mod",
      "class": "/Script/Engine.MaterialExpressionFmod",
      "properties": {}
    },
    {
      "id": "RgbHsv",
      "class": "/Script/Engine.MaterialExpressionRgbToHsv",
      "properties": {}
    },
    {
      "id": "HsvRgb",
      "class": "/Script/Engine.MaterialExpressionHsvToRgb",
      "properties": {}
    },
    {
      "id": "Mix",
      "class": "/Script/Engine.MaterialExpressionAdd",
      "properties": {}
    },
    {
      "id": "ScaleColor",
      "class": "/Script/Engine.MaterialExpressionMultiply",
      "properties": {}
    }
  ],
  "edges": [
    {
      "from": "Scalar",
      "output": "0",
      "to": "Sqrt",
      "input": "Input"
    },
    {
      "from": "Sqrt",
      "output": "0",
      "to": "Log",
      "input": "X"
    },
    {
      "from": "Log",
      "output": "0",
      "to": "Exp",
      "input": "Input"
    },
    {
      "from": "Exp",
      "output": "0",
      "to": "Round",
      "input": "Input"
    },
    {
      "from": "Round",
      "output": "0",
      "to": "Trunc",
      "input": "Input"
    },
    {
      "from": "Scalar",
      "output": "0",
      "to": "Tangent",
      "input": "Input"
    },
    {
      "from": "Tangent",
      "output": "0",
      "to": "Arc",
      "input": "Input"
    },
    {
      "from": "Inverse",
      "output": "0",
      "to": "Ceil",
      "input": "Input"
    },
    {
      "from": "Arc",
      "output": "0",
      "to": "Mod",
      "input": "A"
    },
    {
      "from": "Ceil",
      "output": "0",
      "to": "Mod",
      "input": "B"
    },
    {
      "from": "Color",
      "output": "0",
      "to": "RgbHsv",
      "input": "Input"
    },
    {
      "from": "RgbHsv",
      "output": "0",
      "to": "HsvRgb",
      "input": "Input"
    },
    {
      "from": "Trunc",
      "output": "0",
      "to": "Mix",
      "input": "A"
    },
    {
      "from": "Mod",
      "output": "0",
      "to": "Mix",
      "input": "B"
    },
    {
      "from": "HsvRgb",
      "output": "0",
      "to": "ScaleColor",
      "input": "A"
    },
    {
      "from": "Mix",
      "output": "0",
      "to": "ScaleColor",
      "input": "B"
    },
    {
      "from": "ScaleColor",
      "output": "0",
      "to": "Material",
      "input": "EmissiveColor"
    }
  ],
  "settings": {
    "domain": "Surface",
    "blend": "Opaque",
    "shading": "Unlit",
    "twoSided": false
  }
}
```

## Blueprint：事件、成员与普通计算

```json
{
  "format": "NodeForgeGraph",
  "version": 2,
  "requestId": "ExampleBlueprint4",
  "assetType": "Blueprint",
  "assetPath": "/Game/NodeForgeTests/Examples/Sample4",
  "catalogDigest": "be97bf57d226778102038c261c556b89b262197c78158fe16ca293de709d8847",
  "nodes": [
    {
      "id": "Begin",
      "class": "/Script/BlueprintGraph.K2Node_Event",
      "properties": {
        "owner": "/Script/Engine.Actor",
        "member": "ReceiveBeginPlay"
      }
    },
    {
      "id": "Sequence",
      "class": "/Script/BlueprintGraph.K2Node_ExecutionSequence",
      "properties": {
        "outputCount": 2
      }
    },
    {
      "id": "Branch",
      "class": "/Script/BlueprintGraph.K2Node_IfThenElse",
      "properties": {}
    },
    {
      "id": "SwitchInt",
      "class": "/Script/BlueprintGraph.K2Node_SwitchInteger",
      "properties": {
        "startIndex": 0,
        "caseCount": 2,
        "hasDefault": true
      }
    },
    {
      "id": "SwitchName",
      "class": "/Script/BlueprintGraph.K2Node_SwitchName",
      "properties": {
        "cases": [
          "CaseA",
          "CaseB"
        ],
        "hasDefault": true
      }
    },
    {
      "id": "GetFlag",
      "class": "/Script/BlueprintGraph.K2Node_VariableGet",
      "properties": {
        "variable": "Flag"
      }
    },
    {
      "id": "SetFlag",
      "class": "/Script/BlueprintGraph.K2Node_VariableSet",
      "properties": {
        "variable": "Flag"
      }
    },
    {
      "id": "GetCount",
      "class": "/Script/BlueprintGraph.K2Node_VariableGet",
      "properties": {
        "variable": "Count"
      }
    },
    {
      "id": "SetCount",
      "class": "/Script/BlueprintGraph.K2Node_VariableSet",
      "properties": {
        "variable": "Count"
      }
    },
    {
      "id": "GetValue",
      "class": "/Script/BlueprintGraph.K2Node_VariableGet",
      "properties": {
        "variable": "Value"
      }
    },
    {
      "id": "SetValue",
      "class": "/Script/BlueprintGraph.K2Node_VariableSet",
      "properties": {
        "variable": "Value"
      }
    },
    {
      "id": "GetLabel",
      "class": "/Script/BlueprintGraph.K2Node_VariableGet",
      "properties": {
        "variable": "Label"
      }
    },
    {
      "id": "SetLabel",
      "class": "/Script/BlueprintGraph.K2Node_VariableSet",
      "properties": {
        "variable": "Label"
      }
    },
    {
      "id": "GetText",
      "class": "/Script/BlueprintGraph.K2Node_VariableGet",
      "properties": {
        "variable": "Text"
      }
    },
    {
      "id": "SetText",
      "class": "/Script/BlueprintGraph.K2Node_VariableSet",
      "properties": {
        "variable": "Text"
      }
    },
    {
      "id": "GetPosition",
      "class": "/Script/BlueprintGraph.K2Node_VariableGet",
      "properties": {
        "variable": "Position"
      }
    },
    {
      "id": "SetPosition",
      "class": "/Script/BlueprintGraph.K2Node_VariableSet",
      "properties": {
        "variable": "Position"
      }
    },
    {
      "id": "GetOrientation",
      "class": "/Script/BlueprintGraph.K2Node_VariableGet",
      "properties": {
        "variable": "Orientation"
      }
    },
    {
      "id": "SetOrientation",
      "class": "/Script/BlueprintGraph.K2Node_VariableSet",
      "properties": {
        "variable": "Orientation"
      }
    },
    {
      "id": "Greater",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetMathLibrary",
        "member": "Greater_IntInt"
      },
      "defaults": {
        "B": 0
      }
    },
    {
      "id": "IntAbs",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetMathLibrary",
        "member": "Abs_Int"
      }
    },
    {
      "id": "IntClamp",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetMathLibrary",
        "member": "Clamp"
      },
      "defaults": {
        "Min": 0,
        "Max": 4
      }
    },
    {
      "id": "ScalarSin",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetMathLibrary",
        "member": "Sin"
      }
    },
    {
      "id": "NameEqual",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetMathLibrary",
        "member": "EqualEqual_NameName"
      },
      "defaults": {
        "B": "CaseA"
      }
    },
    {
      "id": "BoolXor",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetMathLibrary",
        "member": "BooleanXOR"
      }
    },
    {
      "id": "Reverse",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetStringLibrary",
        "member": "Reverse"
      }
    },
    {
      "id": "ToName",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetStringLibrary",
        "member": "Conv_StringToName"
      }
    },
    {
      "id": "Negate",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetMathLibrary",
        "member": "NegateVector"
      }
    },
    {
      "id": "Scale",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetMathLibrary",
        "member": "Multiply_VectorFloat"
      },
      "defaults": {
        "B": 0.5
      }
    },
    {
      "id": "Compose",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetMathLibrary",
        "member": "ComposeRotators"
      },
      "defaults": {
        "B": {
          "pitch": 0,
          "yaw": 45,
          "roll": 0
        }
      }
    },
    {
      "id": "YawPitch",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetMathLibrary",
        "member": "GetYawPitchFromVector"
      }
    },
    {
      "id": "Blend",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetMathLibrary",
        "member": "RLerp"
      },
      "defaults": {
        "B": {
          "pitch": 0,
          "yaw": 90,
          "roll": 0
        },
        "bShortestPath": false
      }
    },
    {
      "id": "BlendDefault",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetMathLibrary",
        "member": "RLerp"
      },
      "defaults": {
        "B": {
          "pitch": 0,
          "yaw": 90,
          "roll": 0
        },
        "Alpha": 0.25,
        "bShortestPath": true
      }
    },
    {
      "id": "Root",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.Actor",
        "member": "K2_GetRootComponent"
      }
    },
    {
      "id": "MoveRoot",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.SceneComponent",
        "member": "K2_SetWorldLocation"
      },
      "defaults": {
        "bSweep": false,
        "bTeleport": true
      }
    },
    {
      "id": "Print",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetSystemLibrary",
        "member": "PrintString"
      },
      "defaults": {
        "bPrintToScreen": false,
        "bPrintToLog": true
      }
    },
    {
      "id": "PrintTail",
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "properties": {
        "owner": "/Script/Engine.KismetSystemLibrary",
        "member": "PrintString"
      },
      "defaults": {
        "bPrintToScreen": false,
        "bPrintToLog": true
      }
    }
  ],
  "edges": [
    {
      "from": "GetCount",
      "output": "Count",
      "to": "Greater",
      "input": "A"
    },
    {
      "from": "GetCount",
      "output": "Count",
      "to": "IntAbs",
      "input": "A"
    },
    {
      "from": "IntAbs",
      "output": "ReturnValue",
      "to": "IntClamp",
      "input": "Value"
    },
    {
      "from": "IntClamp",
      "output": "ReturnValue",
      "to": "SetCount",
      "input": "Count"
    },
    {
      "from": "GetValue",
      "output": "Value",
      "to": "ScalarSin",
      "input": "A"
    },
    {
      "from": "ScalarSin",
      "output": "ReturnValue",
      "to": "SetValue",
      "input": "Value"
    },
    {
      "from": "GetLabel",
      "output": "Label",
      "to": "NameEqual",
      "input": "A"
    },
    {
      "from": "GetFlag",
      "output": "Flag",
      "to": "BoolXor",
      "input": "A"
    },
    {
      "from": "NameEqual",
      "output": "ReturnValue",
      "to": "BoolXor",
      "input": "B"
    },
    {
      "from": "BoolXor",
      "output": "ReturnValue",
      "to": "SetFlag",
      "input": "Flag"
    },
    {
      "from": "GetText",
      "output": "Text",
      "to": "Reverse",
      "input": "SourceString"
    },
    {
      "from": "Reverse",
      "output": "ReturnValue",
      "to": "SetText",
      "input": "Text"
    },
    {
      "from": "Reverse",
      "output": "ReturnValue",
      "to": "ToName",
      "input": "InString"
    },
    {
      "from": "ToName",
      "output": "ReturnValue",
      "to": "SetLabel",
      "input": "Label"
    },
    {
      "from": "GetPosition",
      "output": "Position",
      "to": "Negate",
      "input": "A"
    },
    {
      "from": "Negate",
      "output": "ReturnValue",
      "to": "Scale",
      "input": "A"
    },
    {
      "from": "Scale",
      "output": "ReturnValue",
      "to": "SetPosition",
      "input": "Position"
    },
    {
      "from": "GetOrientation",
      "output": "Orientation",
      "to": "Compose",
      "input": "A"
    },
    {
      "from": "GetPosition",
      "output": "Position",
      "to": "YawPitch",
      "input": "InVec"
    },
    {
      "from": "Compose",
      "output": "ReturnValue",
      "to": "Blend",
      "input": "A"
    },
    {
      "from": "YawPitch",
      "output": "Yaw",
      "to": "Blend",
      "input": "Alpha"
    },
    {
      "from": "Blend",
      "output": "ReturnValue",
      "to": "BlendDefault",
      "input": "A"
    },
    {
      "from": "BlendDefault",
      "output": "ReturnValue",
      "to": "SetOrientation",
      "input": "Orientation"
    },
    {
      "from": "Root",
      "output": "ReturnValue",
      "to": "MoveRoot",
      "input": "self"
    },
    {
      "from": "Scale",
      "output": "ReturnValue",
      "to": "MoveRoot",
      "input": "NewLocation"
    },
    {
      "from": "Reverse",
      "output": "ReturnValue",
      "to": "Print",
      "input": "InString"
    },
    {
      "from": "GetText",
      "output": "Text",
      "to": "PrintTail",
      "input": "InString"
    },
    {
      "from": "Begin",
      "output": "then",
      "to": "Sequence",
      "input": "execute"
    },
    {
      "from": "Sequence",
      "output": "then_0",
      "to": "Branch",
      "input": "execute"
    },
    {
      "from": "Greater",
      "output": "ReturnValue",
      "to": "Branch",
      "input": "Condition"
    },
    {
      "from": "Branch",
      "output": "then",
      "to": "SetFlag",
      "input": "execute"
    },
    {
      "from": "SetFlag",
      "output": "then",
      "to": "SetCount",
      "input": "execute"
    },
    {
      "from": "SetCount",
      "output": "then",
      "to": "SetValue",
      "input": "execute"
    },
    {
      "from": "SetValue",
      "output": "then",
      "to": "SetLabel",
      "input": "execute"
    },
    {
      "from": "SetLabel",
      "output": "then",
      "to": "SetText",
      "input": "execute"
    },
    {
      "from": "SetText",
      "output": "then",
      "to": "SetPosition",
      "input": "execute"
    },
    {
      "from": "SetPosition",
      "output": "then",
      "to": "SetOrientation",
      "input": "execute"
    },
    {
      "from": "SetOrientation",
      "output": "then",
      "to": "MoveRoot",
      "input": "execute"
    },
    {
      "from": "MoveRoot",
      "output": "then",
      "to": "SwitchInt",
      "input": "execute"
    },
    {
      "from": "SwitchInt",
      "output": "0",
      "to": "SwitchName",
      "input": "execute"
    },
    {
      "from": "SwitchName",
      "output": "CaseA",
      "to": "Print",
      "input": "execute"
    },
    {
      "from": "GetCount",
      "output": "Count",
      "to": "SwitchInt",
      "input": "Selection"
    },
    {
      "from": "GetLabel",
      "output": "Label",
      "to": "SwitchName",
      "input": "Selection"
    },
    {
      "from": "Sequence",
      "output": "then_1",
      "to": "PrintTail",
      "input": "execute"
    }
  ],
  "settings": {
    "parentClass": "/Script/Engine.Actor",
    "variables": [
      {
        "name": "Flag",
        "type": "bool",
        "default": true
      },
      {
        "name": "Count",
        "type": "int",
        "default": 1
      },
      {
        "name": "Value",
        "type": "float",
        "default": 0.5
      },
      {
        "name": "Label",
        "type": "name",
        "default": "CaseA"
      },
      {
        "name": "Text",
        "type": "string",
        "default": "AbC"
      },
      {
        "name": "Position",
        "type": "vector",
        "default": [
          1,
          2,
          3
        ]
      },
      {
        "name": "Orientation",
        "type": "rotator",
        "default": {
          "pitch": 0,
          "yaw": 45,
          "roll": 0
        }
      }
    ]
  }
}
```

示例展示节点如何组成完整图。节点目录不承诺任意组合都可用，提交前仍需预检。查找具体节点见 [节点说明](NodeBulk.md)，失败时见 [结果与恢复](ResultsAndRecovery.md)。
