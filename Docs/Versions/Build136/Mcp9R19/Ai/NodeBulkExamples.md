# 普通节点的两族完整配方

本页对应 NodeForge-Build136-Mcp9R19 / Build136 Repair6，目录摘要由当前正式目录生成。下面是完整当前配方，保留原有限样例的节点、属性和连接语义；它们不是历史导出原payload或已保存来源证明。当前schema、类名及事件/函数绑定已静态核对，目录仍experimental。新任务先确认授权与本次实际目录/目标占用，按[AiPreparation](Preparation.md)使用新的requestId和完整目标，保存实际输入原字节后正式预检；示例身份不预留，也不能把换digest当旧能力升级。

带资源依赖的配方先精确inspect当前已保存、clean资源及实际文件SHA/函数GUID签名。全零assetSha256是明确未取得证明的占位值，结构合法但不可直接预检生成；必须用本次返回的真实值。若资源/参数/端口与配方不匹配就停止，不能修改来源报告或凭文档补造证明。完整材料引用正例见[AiResources](Resources.md)。

材质以标量/颜色/数学链连接到Unlit活动输出；蓝图把普通纯函数结果交给成员、Branch/Switch、消息或自身根组件消费。节点不因列出就算运行通过；准确pin/default/property以同包节点页和实际Catalog为准。

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

有限N的原两族配置在Build132完成生成/编译/save、完整导出/来源载入和新PIDVerify；未重跑全部节点或全部组合。这里按当前定义核对原结构形成新配方，旧实证仍属于原版本，不能继承其它图的画面/运行。要找节点先[rules/节点发现](NodeBulk.md)，结果按[恢复](ResultsAndRecovery.md)处理。
