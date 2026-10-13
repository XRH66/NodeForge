# 三族完整v3配方

本页对应 {{bundleId}} / {{buildId}}，目录摘要由当前正式目录生成。下面是完整当前配方，保留原有限样例的节点、属性和连接语义；它们不是历史导出原payload或已保存来源证明。当前schema、类名及事件/函数绑定已静态核对，目录仍experimental。新任务先确认授权与本次实际目录/目标占用，按[AiPreparation]({{page:AiPreparation}})使用新的requestId和完整目标，保存实际输入原字节后正式预检；示例身份不预留，也不能把换digest当旧能力升级。

带资源依赖的配方先精确inspect当前已保存、clean资源及实际文件SHA/函数GUID签名。全零assetSha256是明确未取得证明的占位值，结构合法但不可直接预检生成；必须用本次返回的真实值。若资源/参数/端口与配方不匹配就停止，不能修改来源报告或凭文档补造证明。完整材料引用正例见[AiResources]({{page:AiResources}})。

v3根精确七字段format、version、requestId、catalogDigest、target、asset、graphs。target描述新建目标；asset描述Material设置、Blueprint父类和成员、MaterialInstance父引用和参数；graphs是材质一图、Actor一个EventGraph或实例零图。普通properties与Event/Function/Member/MaterialFunction binding分离。v2解码同一GraphDocument/原生核心，不增加第二执行器。

材质例含动态WorldAlignedTexture函数，必须先核实际函数签名及所需TextureObject；实例例的Parent必须是本次已保存、clean、具备Gain/Tint/SurfaceTexture真实参数的普通父材质，不能假设文档中存在这个文件。实例依赖占位SHA必须由实际父读取替换；这不是“零SHA有效”的例子。

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

旧有限J在Build133 Repair2完成两族生成/保存/Verify与实例transient参数、父SHA检查；实例没有保存。来源original必须用原完整text/sourceRef，retain明确编辑再准备；转换格式不升级旧来源证明。现行导出默认2/显式3，准备/预览/重排保持格式。详细规则见[GraphAuthoring]({{page:GraphAuthoring}})、[设置]({{page:AiAssetSettings}})和[来源/副本]({{page:AiCopyPreview}})。
