# 材质、蓝图与实例的 JSON v3 示例

以下是 NodeForge v1.1.2 的完整输入示例。使用前读取当前目录，选择未占用的 requestId 和目标路径，并按 [调用流程](QuickStart.md)执行预检与生成。示例中的资源路径、会话和 SHA 占位值需要换成当前实际返回的值。

示例如果引用资源，先用 resources_inspect 检查已保存且未修改的对象、参数和函数签名。全零 assetSha256 是占位值，必须替换为这次检查返回的真实 SHA。如果资源或引脚不匹配，先调整任务输入，不要改写来源报告。具体步骤见 [资源查询与引用](Resources.md)。

v3 根对象包含七个字段：format、version、requestId、catalogDigest、target、asset、graphs。target 描述新资产位置，asset 描述设置、父类、变量或实例参数。Material 使用一个图，Actor Blueprint 使用一个 EventGraph，MaterialInstance 不含图。普通 properties 与 Event/Function/Member/MaterialFunction binding 分开。v2 与 v3 使用同一套图模型和生成逻辑。

材质示例使用 WorldAlignedTexture 函数，使用前请查询函数签名并准备所需的 TextureObject。实例示例需要你自己工程中已保存且未修改的普通父材质，并具有 Gain、Tint、SurfaceTexture 参数；请替换示例中的父材质路径，并将占位 SHA 替换为资源检查返回的实际值。

## Material

```json
{
  "format": "NodeForgeGraph",
  "version": 3,
  "requestId": "ExampleMaterial5",
  "catalogDigest": "be97bf57d226778102038c261c556b89b262197c78158fe16ca293de709d8847",
  "target": {
    "assetPath": "/Game/NodeForgeTests/Examples/Sample5"
  },
  "asset": {
    "type": "Material",
    "settings": {
      "domain": "Surface",
      "blend": "Opaque",
      "shading": "Unlit",
      "twoSided": false
    }
  },
  "graphs": [
    {
      "id": "MaterialGraph",
      "kind": "Material",
      "nodes": [
        {
          "id": "Texture",
          "class": "/Script/Engine.MaterialExpressionTextureObjectParameter",
          "properties": {
            "Texture": "/Engine/EngineResources/DefaultTexture.DefaultTexture",
            "ParameterName": "SurfaceTexture"
          }
        },
        {
          "id": "Size",
          "class": "/Script/Engine.MaterialExpressionConstant3Vector",
          "properties": {
            "Constant": [
              64,
              64,
              64,
              1
            ]
          }
        },
        {
          "id": "Function",
          "class": "/Script/Engine.MaterialExpressionMaterialFunctionCall",
          "properties": {},
          "binding": {
            "kind": "MaterialFunction",
            "path": "/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedTexture.WorldAlignedTexture",
            "assetSha256": "0000000000000000000000000000000000000000000000000000000000000000"
          }
        },
        {
          "id": "Tint",
          "class": "/Script/Engine.MaterialExpressionVectorParameter",
          "properties": {
            "ParameterName": "Tint",
            "DefaultValue": [
              0.25,
              0.5,
              1,
              1
            ]
          }
        },
        {
          "id": "Tinted",
          "class": "/Script/Engine.MaterialExpressionMultiply",
          "properties": {}
        },
        {
          "id": "Gain",
          "class": "/Script/Engine.MaterialExpressionScalarParameter",
          "properties": {
            "ParameterName": "Gain",
            "DefaultValue": 0.5
          }
        },
        {
          "id": "Scaled",
          "class": "/Script/Engine.MaterialExpressionMultiply",
          "properties": {}
        }
      ],
      "edges": [
        {
          "from": "Texture",
          "output": "0",
          "to": "Function",
          "input": "TextureObject (T2d)"
        },
        {
          "from": "Size",
          "output": "0",
          "to": "Function",
          "input": "TextureSize (V3)"
        },
        {
          "from": "Function",
          "output": "2",
          "to": "Tinted",
          "input": "A"
        },
        {
          "from": "Tint",
          "output": "0",
          "to": "Tinted",
          "input": "B"
        },
        {
          "from": "Tinted",
          "output": "0",
          "to": "Scaled",
          "input": "A"
        },
        {
          "from": "Gain",
          "output": "0",
          "to": "Scaled",
          "input": "B"
        },
        {
          "from": "Scaled",
          "output": "0",
          "to": "Material",
          "input": "EmissiveColor"
        }
      ]
    }
  ]
}
```

## Blueprint：七类Value成员

```json
{
  "format": "NodeForgeGraph",
  "version": 3,
  "requestId": "ExampleBlueprint6",
  "catalogDigest": "be97bf57d226778102038c261c556b89b262197c78158fe16ca293de709d8847",
  "target": {
    "assetPath": "/Game/NodeForgeTests/Examples/Sample6"
  },
  "asset": {
    "type": "Blueprint",
    "parent": {
      "class": "/Script/Engine.Actor"
    },
    "members": {
      "variables": [
        {
          "name": "Flag",
          "type": {
            "kind": "Value",
            "name": "bool"
          },
          "default": true
        },
        {
          "name": "Count",
          "type": {
            "kind": "Value",
            "name": "int"
          },
          "default": 1
        },
        {
          "name": "Value",
          "type": {
            "kind": "Value",
            "name": "float"
          },
          "default": 0.5
        },
        {
          "name": "Label",
          "type": {
            "kind": "Value",
            "name": "name"
          },
          "default": "CaseA"
        },
        {
          "name": "Text",
          "type": {
            "kind": "Value",
            "name": "string"
          },
          "default": "AbC"
        },
        {
          "name": "Position",
          "type": {
            "kind": "Value",
            "name": "vector"
          },
          "default": [
            1,
            2,
            3
          ]
        },
        {
          "name": "Orientation",
          "type": {
            "kind": "Value",
            "name": "rotator"
          },
          "default": {
            "pitch": 0,
            "yaw": 45,
            "roll": 0
          }
        }
      ]
    }
  },
  "graphs": [
    {
      "id": "EventGraph",
      "kind": "EventGraph",
      "nodes": [
        {
          "id": "Begin",
          "class": "/Script/BlueprintGraph.K2Node_Event",
          "properties": {},
          "binding": {
            "kind": "Event",
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
          "properties": {},
          "binding": {
            "kind": "Member",
            "name": "Flag"
          }
        },
        {
          "id": "SetFlag",
          "class": "/Script/BlueprintGraph.K2Node_VariableSet",
          "properties": {},
          "binding": {
            "kind": "Member",
            "name": "Flag"
          }
        },
        {
          "id": "GetCount",
          "class": "/Script/BlueprintGraph.K2Node_VariableGet",
          "properties": {},
          "binding": {
            "kind": "Member",
            "name": "Count"
          }
        },
        {
          "id": "SetCount",
          "class": "/Script/BlueprintGraph.K2Node_VariableSet",
          "properties": {},
          "binding": {
            "kind": "Member",
            "name": "Count"
          }
        },
        {
          "id": "GetValue",
          "class": "/Script/BlueprintGraph.K2Node_VariableGet",
          "properties": {},
          "binding": {
            "kind": "Member",
            "name": "Value"
          }
        },
        {
          "id": "SetValue",
          "class": "/Script/BlueprintGraph.K2Node_VariableSet",
          "properties": {},
          "binding": {
            "kind": "Member",
            "name": "Value"
          }
        },
        {
          "id": "GetLabel",
          "class": "/Script/BlueprintGraph.K2Node_VariableGet",
          "properties": {},
          "binding": {
            "kind": "Member",
            "name": "Label"
          }
        },
        {
          "id": "SetLabel",
          "class": "/Script/BlueprintGraph.K2Node_VariableSet",
          "properties": {},
          "binding": {
            "kind": "Member",
            "name": "Label"
          }
        },
        {
          "id": "GetText",
          "class": "/Script/BlueprintGraph.K2Node_VariableGet",
          "properties": {},
          "binding": {
            "kind": "Member",
            "name": "Text"
          }
        },
        {
          "id": "SetText",
          "class": "/Script/BlueprintGraph.K2Node_VariableSet",
          "properties": {},
          "binding": {
            "kind": "Member",
            "name": "Text"
          }
        },
        {
          "id": "GetPosition",
          "class": "/Script/BlueprintGraph.K2Node_VariableGet",
          "properties": {},
          "binding": {
            "kind": "Member",
            "name": "Position"
          }
        },
        {
          "id": "SetPosition",
          "class": "/Script/BlueprintGraph.K2Node_VariableSet",
          "properties": {},
          "binding": {
            "kind": "Member",
            "name": "Position"
          }
        },
        {
          "id": "GetOrientation",
          "class": "/Script/BlueprintGraph.K2Node_VariableGet",
          "properties": {},
          "binding": {
            "kind": "Member",
            "name": "Orientation"
          }
        },
        {
          "id": "SetOrientation",
          "class": "/Script/BlueprintGraph.K2Node_VariableSet",
          "properties": {},
          "binding": {
            "kind": "Member",
            "name": "Orientation"
          }
        },
        {
          "id": "Greater",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "defaults": {
            "B": 0
          },
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.KismetMathLibrary",
            "member": "Greater_IntInt"
          }
        },
        {
          "id": "IntAbs",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.KismetMathLibrary",
            "member": "Abs_Int"
          }
        },
        {
          "id": "IntClamp",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "defaults": {
            "Min": 0,
            "Max": 4
          },
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.KismetMathLibrary",
            "member": "Clamp"
          }
        },
        {
          "id": "ScalarSin",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.KismetMathLibrary",
            "member": "Sin"
          }
        },
        {
          "id": "NameEqual",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "defaults": {
            "B": "CaseA"
          },
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.KismetMathLibrary",
            "member": "EqualEqual_NameName"
          }
        },
        {
          "id": "BoolXor",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.KismetMathLibrary",
            "member": "BooleanXOR"
          }
        },
        {
          "id": "Reverse",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.KismetStringLibrary",
            "member": "Reverse"
          }
        },
        {
          "id": "ToName",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.KismetStringLibrary",
            "member": "Conv_StringToName"
          }
        },
        {
          "id": "Negate",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.KismetMathLibrary",
            "member": "NegateVector"
          }
        },
        {
          "id": "Scale",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "defaults": {
            "B": 0.5
          },
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.KismetMathLibrary",
            "member": "Multiply_VectorFloat"
          }
        },
        {
          "id": "Compose",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "defaults": {
            "B": {
              "pitch": 0,
              "yaw": 45,
              "roll": 0
            }
          },
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.KismetMathLibrary",
            "member": "ComposeRotators"
          }
        },
        {
          "id": "YawPitch",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.KismetMathLibrary",
            "member": "GetYawPitchFromVector"
          }
        },
        {
          "id": "Blend",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "defaults": {
            "B": {
              "pitch": 0,
              "yaw": 90,
              "roll": 0
            },
            "bShortestPath": false
          },
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.KismetMathLibrary",
            "member": "RLerp"
          }
        },
        {
          "id": "BlendDefault",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "defaults": {
            "B": {
              "pitch": 0,
              "yaw": 90,
              "roll": 0
            },
            "Alpha": 0.25,
            "bShortestPath": true
          },
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.KismetMathLibrary",
            "member": "RLerp"
          }
        },
        {
          "id": "Root",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.Actor",
            "member": "K2_GetRootComponent"
          }
        },
        {
          "id": "MoveRoot",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "defaults": {
            "bSweep": false,
            "bTeleport": true
          },
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.SceneComponent",
            "member": "K2_SetWorldLocation"
          }
        },
        {
          "id": "Print",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "defaults": {
            "bPrintToScreen": false,
            "bPrintToLog": true
          },
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.KismetSystemLibrary",
            "member": "PrintString"
          }
        },
        {
          "id": "PrintTail",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "defaults": {
            "bPrintToScreen": false,
            "bPrintToLog": true
          },
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.KismetSystemLibrary",
            "member": "PrintString"
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
      ]
    }
  ]
}
```

## MaterialInstance：依赖填写后才可执行

```json
{
  "format": "NodeForgeGraph",
  "version": 3,
  "requestId": "ExampleMaterialInstance7",
  "catalogDigest": "be97bf57d226778102038c261c556b89b262197c78158fe16ca293de709d8847",
  "target": {
    "assetPath": "/Game/NodeForgeTests/Examples/Sample7"
  },
  "asset": {
    "type": "MaterialInstance",
    "parent": {
      "path": "/Game/NodeForgeTests/Examples/Parent.Parent",
      "assetSha256": "0000000000000000000000000000000000000000000000000000000000000000"
    },
    "parameters": {
      "scalar": {
        "Gain": 0
      },
      "vector": {
        "Tint": [
          0.125,
          0.25,
          0.5,
          1
        ]
      },
      "texture": {
        "SurfaceTexture": "/Engine/EngineResources/DefaultTexture.DefaultTexture"
      }
    }
  },
  "graphs": []
}
```

材质实例示例中的父材质和 SHA 必须来自当前检查。实例的 MCP 生成和保存未验证。导出省略 formatVersion 时返回 v2，显式传入 3 时返回 v3；准备、预览和重排保持当前格式。详见 [JSON 格式](../Reference/GraphAuthoring.md)和 [副本编辑](CopyPreview.md)。
