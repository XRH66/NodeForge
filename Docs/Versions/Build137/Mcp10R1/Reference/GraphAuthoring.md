> 当前接入身份：Build137 Repair5 / Ai1 / Mcp10R1；Windows x64官方UE5.3.2。正文功能说明继承Mcp9R19；Build136、原stage/PID/验收路径只表示历史证据。当前安装和连接统一见[AI安装](../User/AiInstallation.md)。不要复制历史本机路径或环境配置。

# 当前v3与v2兼容编写

公开副本中的请求、目标、进程、会话及来源报告身份为虚构示例；历史产品结论保持，原始证明由维护者内部保留。实际调用须使用本次分配和返回的完整原 text/sourceRef，示例身份与零摘要不能作为来源证明。

本包推荐NodeForgeGraph v3，七根字段、明确asset/graphs/binding/Value类型。旧v2解码同一模型；旧档案按原wire能力准入：134保持2/3及已验Tick，133保持2/3但无Tick，132保持2且无Tick；三者均不开放本期资产设置。graphs.export显式formatVersion=3；遗漏仍2。准备、预览、重排保持输入版本。raw input/report/payload SHA分别保留；来源original保留数组顺序/presence/目标，retain按原编辑规则。

以下结构和值事实由本包native AssetDefinitions生成；资源/动态pin/编译/保存/Verify仍需正式核心验收。完整当前schema见[GraphRequestSchema.json](../GraphRequestSchema.json)。

```json
{
  "fieldGroups": {
    "RootV2": [
      {
        "name": "format",
        "type": "string",
        "required": true
      },
      {
        "name": "version",
        "type": "integer",
        "required": true
      },
      {
        "name": "requestId",
        "type": "string",
        "required": true
      },
      {
        "name": "catalogDigest",
        "type": "string",
        "required": true
      },
      {
        "name": "assetType",
        "type": "string",
        "required": true
      },
      {
        "name": "assetPath",
        "type": "string",
        "required": true
      },
      {
        "name": "settings",
        "type": "object",
        "required": true
      },
      {
        "name": "nodes",
        "type": "array",
        "required": true
      },
      {
        "name": "edges",
        "type": "array",
        "required": true
      }
    ],
    "RootV3": [
      {
        "name": "format",
        "type": "string",
        "required": true
      },
      {
        "name": "version",
        "type": "integer",
        "required": true
      },
      {
        "name": "requestId",
        "type": "string",
        "required": true
      },
      {
        "name": "catalogDigest",
        "type": "string",
        "required": true
      },
      {
        "name": "target",
        "type": "object",
        "required": true
      },
      {
        "name": "asset",
        "type": "object",
        "required": true
      },
      {
        "name": "graphs",
        "type": "array",
        "required": true
      }
    ],
    "Target": [
      {
        "name": "assetPath",
        "type": "string",
        "required": true
      }
    ],
    "MaterialAsset": [
      {
        "name": "type",
        "type": "string",
        "required": true
      },
      {
        "name": "settings",
        "type": "object",
        "required": true
      }
    ],
    "BlueprintAsset": [
      {
        "name": "type",
        "type": "string",
        "required": true
      },
      {
        "name": "parent",
        "type": "object",
        "required": true
      },
      {
        "name": "members",
        "type": "object",
        "required": true
      }
    ],
    "InstanceAsset": [
      {
        "name": "type",
        "type": "string",
        "required": true
      },
      {
        "name": "parent",
        "type": "object",
        "required": true
      },
      {
        "name": "parameters",
        "type": "object",
        "required": true
      }
    ],
    "MemberAttributes": [
      {
        "name": "instanceEditable",
        "type": "boolean",
        "required": true
      },
      {
        "name": "category",
        "type": "string",
        "required": true
      },
      {
        "name": "tooltip",
        "type": "string",
        "required": true
      }
    ],
    "MaterialSettings": [
      {
        "name": "domain",
        "type": "string",
        "required": true,
        "enum": [
          "Surface"
        ]
      },
      {
        "name": "blend",
        "type": "string",
        "required": true,
        "enum": [
          "Opaque",
          "Translucent",
          "Masked"
        ]
      },
      {
        "name": "shading",
        "type": "string",
        "required": true,
        "enum": [
          "Unlit",
          "DefaultLit"
        ]
      },
      {
        "name": "lighting",
        "type": "string",
        "required": false,
        "enum": [
          "SurfaceForwardShading"
        ]
      },
      {
        "name": "twoSided",
        "type": "boolean",
        "required": true
      },
      {
        "name": "opacityMaskClipValue",
        "type": "number",
        "required": false
      },
      {
        "name": "ditherOpacityMask",
        "type": "boolean",
        "required": false
      },
      {
        "name": "ditheredLodTransition",
        "type": "boolean",
        "required": false
      },
      {
        "name": "allowNegativeEmissiveColor",
        "type": "boolean",
        "required": false
      }
    ],
    "BlueprintSettingsV2": [
      {
        "name": "parentClass",
        "type": "string",
        "required": true
      },
      {
        "name": "variables",
        "type": "array",
        "required": true
      }
    ],
    "BlueprintParent": [
      {
        "name": "class",
        "type": "string",
        "required": true
      }
    ],
    "Members": [
      {
        "name": "variables",
        "type": "array",
        "required": true
      }
    ],
    "VariableV2": [
      {
        "name": "name",
        "type": "string",
        "required": true
      },
      {
        "name": "type",
        "type": "string",
        "required": true
      },
      {
        "name": "default",
        "type": "value",
        "required": true
      },
      {
        "name": "attributes",
        "type": "object",
        "required": false
      }
    ],
    "VariableV3": [
      {
        "name": "name",
        "type": "string",
        "required": true
      },
      {
        "name": "type",
        "type": "object",
        "required": true
      },
      {
        "name": "default",
        "type": "value",
        "required": true
      },
      {
        "name": "attributes",
        "type": "object",
        "required": false
      }
    ],
    "ValueType": [
      {
        "name": "kind",
        "type": "string",
        "required": true
      },
      {
        "name": "name",
        "type": "string",
        "required": true
      }
    ],
    "InstanceSettingsV2": [
      {
        "name": "parent",
        "type": "string",
        "required": true
      },
      {
        "name": "parentAssetSha256",
        "type": "string",
        "required": true
      },
      {
        "name": "scalarParameters",
        "type": "object",
        "required": true
      },
      {
        "name": "vectorParameters",
        "type": "object",
        "required": true
      },
      {
        "name": "textureParameters",
        "type": "object",
        "required": true
      }
    ],
    "InstanceParent": [
      {
        "name": "path",
        "type": "string",
        "required": true
      },
      {
        "name": "assetSha256",
        "type": "string",
        "required": true
      }
    ],
    "Parameters": [
      {
        "name": "scalar",
        "type": "object",
        "required": true
      },
      {
        "name": "vector",
        "type": "object",
        "required": true
      },
      {
        "name": "texture",
        "type": "object",
        "required": true
      }
    ],
    "Graph": [
      {
        "name": "id",
        "type": "string",
        "required": true
      },
      {
        "name": "kind",
        "type": "string",
        "required": true
      },
      {
        "name": "nodes",
        "type": "array",
        "required": true
      },
      {
        "name": "edges",
        "type": "array",
        "required": true
      }
    ],
    "NodeV2": [
      {
        "name": "id",
        "type": "string",
        "required": true
      },
      {
        "name": "class",
        "type": "string",
        "required": true
      },
      {
        "name": "properties",
        "type": "object",
        "required": true
      },
      {
        "name": "defaults",
        "type": "object",
        "required": false
      },
      {
        "name": "position",
        "type": "array",
        "required": false
      }
    ],
    "NodeV3": [
      {
        "name": "id",
        "type": "string",
        "required": true
      },
      {
        "name": "class",
        "type": "string",
        "required": true
      },
      {
        "name": "properties",
        "type": "object",
        "required": true
      },
      {
        "name": "binding",
        "type": "object",
        "required": false
      },
      {
        "name": "defaults",
        "type": "object",
        "required": false
      },
      {
        "name": "position",
        "type": "array",
        "required": false
      }
    ],
    "Edge": [
      {
        "name": "from",
        "type": "string",
        "required": true
      },
      {
        "name": "output",
        "type": "string",
        "required": true
      },
      {
        "name": "to",
        "type": "string",
        "required": true
      },
      {
        "name": "input",
        "type": "string",
        "required": true
      }
    ],
    "FunctionBinding": [
      {
        "name": "kind",
        "type": "string",
        "required": true
      },
      {
        "name": "owner",
        "type": "string",
        "required": true
      },
      {
        "name": "member",
        "type": "string",
        "required": true
      }
    ],
    "MemberBinding": [
      {
        "name": "kind",
        "type": "string",
        "required": true
      },
      {
        "name": "name",
        "type": "string",
        "required": true
      }
    ],
    "MaterialFunctionBinding": [
      {
        "name": "kind",
        "type": "string",
        "required": true
      },
      {
        "name": "path",
        "type": "string",
        "required": true
      },
      {
        "name": "assetSha256",
        "type": "string",
        "required": true
      }
    ],
    "Rotator": [
      {
        "name": "pitch",
        "type": "number",
        "required": true
      },
      {
        "name": "yaw",
        "type": "number",
        "required": true
      },
      {
        "name": "roll",
        "type": "number",
        "required": true
      }
    ],
    "Sequence": [
      {
        "name": "outputCount",
        "type": "integer",
        "required": true
      }
    ],
    "SwitchInteger": [
      {
        "name": "startIndex",
        "type": "integer",
        "required": true
      },
      {
        "name": "caseCount",
        "type": "integer",
        "required": true
      },
      {
        "name": "hasDefault",
        "type": "boolean",
        "required": true
      }
    ],
    "SwitchName": [
      {
        "name": "cases",
        "type": "array",
        "required": true
      },
      {
        "name": "hasDefault",
        "type": "boolean",
        "required": true
      }
    ],
    "FunctionPropertiesV2": [
      {
        "name": "owner",
        "type": "string",
        "required": true
      },
      {
        "name": "member",
        "type": "string",
        "required": true
      }
    ],
    "MemberPropertiesV2": [
      {
        "name": "variable",
        "type": "string",
        "required": true
      }
    ],
    "MaterialFunctionPropertiesV2": [
      {
        "name": "MaterialFunction",
        "type": "string",
        "required": true
      },
      {
        "name": "FunctionAssetSha256",
        "type": "string",
        "required": true
      }
    ],
    "ExportConfig": [
      {
        "name": "source",
        "type": "string",
        "required": true
      },
      {
        "name": "requestId",
        "type": "string",
        "required": true
      },
      {
        "name": "assetPath",
        "type": "string",
        "required": true
      },
      {
        "name": "formatVersion",
        "type": "integer",
        "required": false
      }
    ]
  },
  "materialSettingsSchema": {
    "type": "object",
    "properties": {
      "domain": {
        "type": "string",
        "enum": [
          "Surface"
        ]
      },
      "blend": {
        "type": "string",
        "enum": [
          "Opaque",
          "Translucent",
          "Masked"
        ]
      },
      "shading": {
        "type": "string",
        "enum": [
          "Unlit",
          "DefaultLit"
        ]
      },
      "lighting": {
        "type": "string",
        "enum": [
          "SurfaceForwardShading"
        ]
      },
      "twoSided": {
        "type": "boolean"
      },
      "opacityMaskClipValue": {
        "type": "number",
        "minimum": 0,
        "maximum": 1,
        "default": 0.3333
      },
      "ditherOpacityMask": {
        "type": "boolean",
        "default": false
      },
      "ditheredLodTransition": {
        "type": "boolean",
        "default": false
      },
      "allowNegativeEmissiveColor": {
        "type": "boolean",
        "default": false
      }
    },
    "required": [
      "domain",
      "blend",
      "shading",
      "twoSided"
    ],
    "additionalProperties": false,
    "if": {
      "properties": {
        "blend": {
          "const": "Translucent"
        }
      }
    },
    "then": {
      "required": [
        "lighting"
      ]
    },
    "allOf": [
      {
        "if": {
          "properties": {
            "blend": {
              "const": "Masked"
            }
          }
        },
        "then": {
          "required": [
            "opacityMaskClipValue"
          ]
        }
      },
      {
        "if": {
          "properties": {
            "allowNegativeEmissiveColor": {
              "const": true
            }
          },
          "required": [
            "allowNegativeEmissiveColor"
          ]
        },
        "then": {
          "properties": {
            "shading": {
              "const": "Unlit"
            }
          }
        }
      }
    ],
    "description": "Surface only. Inactive clip/dither values are stored; no visual/TAA/LOD claim. Root connections must pass native IsPropertyActive."
  },
  "memberAttributesSchema": {
    "type": "object",
    "properties": {
      "instanceEditable": {
        "type": "boolean"
      },
      "category": {
        "type": "string",
        "minLength": 1,
        "maxLength": 128,
        "pattern": "^(?!\\s)(?!.*\\s$)(?!.*[\\u0000-\\u001f\\u007f-\\u009f])[^|]+(?:\\|[^|]+)*$"
      },
      "tooltip": {
        "type": "string",
        "maxLength": 1024,
        "pattern": "^[^\\u0000]*$"
      }
    },
    "required": [
      "instanceEditable",
      "category",
      "tooltip"
    ],
    "additionalProperties": false
  },
  "materialOptions": [
    {
      "name": "twoSided",
      "nativeProperty": "TwoSided",
      "required": true,
      "default": false
    },
    {
      "name": "opacityMaskClipValue",
      "nativeProperty": "OpacityMaskClipValue",
      "required": false,
      "default": 0.3333
    },
    {
      "name": "ditherOpacityMask",
      "nativeProperty": "DitherOpacityMask",
      "required": false,
      "default": false
    },
    {
      "name": "ditheredLodTransition",
      "nativeProperty": "DitheredLODTransition",
      "required": false,
      "default": false
    },
    {
      "name": "allowNegativeEmissiveColor",
      "nativeProperty": "bAllowNegativeEmissiveColor",
      "required": false,
      "default": false
    }
  ],
  "assetTypes": [
    {
      "type": "Material",
      "graphId": "MaterialGraph",
      "graphKind": "Material",
      "graphCount": 1,
      "minimumNodes": 1,
      "maximumNodes": 256,
      "minimumEdges": 1,
      "maximumEdges": 1024
    },
    {
      "type": "Blueprint",
      "graphId": "EventGraph",
      "graphKind": "EventGraph",
      "graphCount": 1,
      "minimumNodes": 1,
      "maximumNodes": 128,
      "minimumEdges": 0,
      "maximumEdges": 256
    },
    {
      "type": "MaterialInstance",
      "graphId": "",
      "graphKind": "",
      "graphCount": 0,
      "minimumNodes": 0,
      "maximumNodes": 0,
      "minimumEdges": 0,
      "maximumEdges": 0
    }
  ],
  "nodeBindings": [
    {
      "class": "/Script/BlueprintGraph.K2Node_Event",
      "kind": "Event"
    },
    {
      "class": "/Script/BlueprintGraph.K2Node_CallFunction",
      "kind": "Function"
    },
    {
      "class": "/Script/BlueprintGraph.K2Node_IfThenElse",
      "kind": "None"
    },
    {
      "class": "/Script/BlueprintGraph.K2Node_VariableGet",
      "kind": "Member"
    },
    {
      "class": "/Script/BlueprintGraph.K2Node_VariableSet",
      "kind": "Member"
    },
    {
      "class": "/Script/BlueprintGraph.K2Node_ExecutionSequence",
      "kind": "None"
    },
    {
      "class": "/Script/BlueprintGraph.K2Node_SwitchInteger",
      "kind": "None"
    },
    {
      "class": "/Script/BlueprintGraph.K2Node_SwitchName",
      "kind": "None"
    },
    {
      "class": "/Script/Engine.MaterialExpressionMaterialFunctionCall",
      "kind": "MaterialFunction"
    }
  ],
  "nodeConfigurationSchemas": {
    "/Script/BlueprintGraph.K2Node_Event": {
      "type": "object",
      "additionalProperties": false
    },
    "/Script/BlueprintGraph.K2Node_CallFunction": {
      "type": "object",
      "additionalProperties": false
    },
    "/Script/BlueprintGraph.K2Node_IfThenElse": {
      "type": "object",
      "additionalProperties": false
    },
    "/Script/BlueprintGraph.K2Node_VariableGet": {
      "type": "object",
      "additionalProperties": false
    },
    "/Script/BlueprintGraph.K2Node_VariableSet": {
      "type": "object",
      "additionalProperties": false
    },
    "/Script/BlueprintGraph.K2Node_ExecutionSequence": {
      "type": "object",
      "properties": {
        "outputCount": {
          "type": "integer",
          "minimum": 2,
          "maximum": 8
        }
      },
      "required": [
        "outputCount"
      ],
      "additionalProperties": false
    },
    "/Script/BlueprintGraph.K2Node_SwitchInteger": {
      "type": "object",
      "properties": {
        "startIndex": {
          "type": "integer",
          "minimum": -2147483648,
          "maximum": 2147483647
        },
        "caseCount": {
          "type": "integer",
          "minimum": 1,
          "maximum": 8
        },
        "hasDefault": {
          "type": "boolean"
        }
      },
      "required": [
        "startIndex",
        "caseCount",
        "hasDefault"
      ],
      "additionalProperties": false,
      "x-nativeConstraint": "startIndex+caseCount<=MAX_int32"
    },
    "/Script/BlueprintGraph.K2Node_SwitchName": {
      "type": "object",
      "properties": {
        "cases": {
          "type": "array",
          "minItems": 1,
          "maxItems": 8,
          "uniqueItems": true,
          "items": {
            "type": "string",
            "pattern": "^[A-Za-z][A-Za-z0-9]{0,63}$"
          }
        },
        "hasDefault": {
          "type": "boolean"
        }
      },
      "required": [
        "cases",
        "hasDefault"
      ],
      "additionalProperties": false,
      "x-nativeConstraint": "case-insensitive canonical FName uniqueness and reserved native pin names"
    }
  },
  "memberTypes": [
    {
      "name": "bool",
      "minimumBlueprintRevision": 0,
      "valueSchema": {
        "type": "boolean"
      }
    },
    {
      "name": "float",
      "minimumBlueprintRevision": 0,
      "valueSchema": {
        "type": "number",
        "minimum": -1000000000000,
        "maximum": 1000000000000
      }
    },
    {
      "name": "string",
      "minimumBlueprintRevision": 0,
      "valueSchema": {
        "type": "string",
        "maxLength": 4096
      }
    },
    {
      "name": "vector",
      "minimumBlueprintRevision": 1,
      "valueSchema": {
        "type": "array",
        "minItems": 3,
        "maxItems": 3,
        "items": {
          "type": "number",
          "minimum": -1000000000000,
          "maximum": 1000000000000
        }
      }
    },
    {
      "name": "int",
      "minimumBlueprintRevision": 4,
      "valueSchema": {
        "type": "integer",
        "minimum": -2147483648,
        "maximum": 2147483647
      }
    },
    {
      "name": "name",
      "minimumBlueprintRevision": 4,
      "valueSchema": {
        "type": "string",
        "maxLength": 1023,
        "minLength": 1,
        "if": {
          "pattern": "^[Nn][Oo][Nn][Ee]$"
        },
        "then": {
          "const": "None"
        },
        "x-nativeConstraint": "canonical FName display spelling checked against actual native storage"
      }
    },
    {
      "name": "rotator",
      "minimumBlueprintRevision": 6,
      "valueSchema": {
        "type": "object",
        "properties": {
          "pitch": {
            "type": "number",
            "minimum": -1000000000000,
            "maximum": 1000000000000
          },
          "yaw": {
            "type": "number",
            "minimum": -1000000000000,
            "maximum": 1000000000000
          },
          "roll": {
            "type": "number",
            "minimum": -1000000000000,
            "maximum": 1000000000000
          }
        },
        "required": [
          "pitch",
          "yaw",
          "roll"
        ],
        "additionalProperties": false
      }
    }
  ],
  "pinDefaultSchema": {
    "anyOf": [
      {
        "type": "boolean"
      },
      {
        "type": "number",
        "minimum": -1000000000000,
        "maximum": 1000000000000
      },
      {
        "type": "string",
        "maxLength": 4096
      },
      {
        "type": "array",
        "minItems": 3,
        "maxItems": 3,
        "items": {
          "type": "number",
          "minimum": -1000000000000,
          "maximum": 1000000000000
        }
      },
      {
        "type": "object",
        "properties": {
          "pitch": {
            "type": "number",
            "minimum": -1000000000000,
            "maximum": 1000000000000
          },
          "yaw": {
            "type": "number",
            "minimum": -1000000000000,
            "maximum": 1000000000000
          },
          "roll": {
            "type": "number",
            "minimum": -1000000000000,
            "maximum": 1000000000000
          }
        },
        "required": [
          "pitch",
          "yaw",
          "roll"
        ],
        "additionalProperties": false
      }
    ]
  },
  "instanceScalarSchema": {
    "type": "number",
    "minimum": -1000000,
    "maximum": 1000000
  },
  "instanceVectorSchema": {
    "type": "array",
    "minItems": 4,
    "maxItems": 4,
    "items": {
      "type": "number",
      "minimum": -1000000,
      "maximum": 1000000
    }
  },
  "acceptedGraphVersions": [
    2,
    3
  ],
  "materialPositionLimit": 1000000,
  "blueprintPositionLimit": 100000,
  "materialNumberLimit": 1000000,
  "memberNumberLimit": 1000000000000,
  "maxStringLength": 4096,
  "nameMaximumLength": 1023,
  "maxVariables": 64,
  "maxInstanceOverrides": 128,
  "maxInputUtf8Bytes": 65536,
  "maxDepth": 32
}
```


## 既有v2详细规则与兼容例子

# JSON 构图、路径与身份

NodeForgeGraph v2 根必须恰有 format、version、requestId、assetType、assetPath、catalogDigest、nodes、edges、settings 九字段。format="NodeForgeGraph"、version=2；catalogDigest 来自当前实际目录内容摘要，不能用目录文件 SHA 替代。v1 旧固定样例保持兼容子集，不隐式迁移。

完整无 BOM UTF-8 1–65536 字节、深度≤32；拒绝重复键、NUL、注释、尾逗号、未知字段。节点必需 id/class/properties，可有 defaults/position。节点 ID 1–64 ASCII 字母数字，Material 是材质输出保留身份；显式 position 为两整数，abs≤100000。数组顺序不决定执行/构图，不能靠改排序修复连线。

每条边恰为 from/output/to/input 字符串。端口/类/属性/绑定大小写规范、目标输入单连接、拒绝环、自连、隐式转换或替线；默认值与同输入连接互斥。各对象真实签名见目录与节点页，不能猜输出别名。布局在实际构图后统一处理，保留显式锚点；报告 size estimate 不证明所有屏幕无重叠。

目标仅 `/Game/NodeForgeTests/` 下0/1/2级子目录，各段1–16 ASCII字母数字，叶名1–64；无空格/下划线/后缀/点段/扩展名/反斜线。磁盘路径核 ProjectContent/reparse/UE包名与总长≤min(260,平台上限)。测试工具分配叶名/request建议≤32，工程≤20建议≤12，不收紧原公开64上限。

同完整包路径或项目级 requestId 已用即拒绝。不覆盖、不静默加后缀；两个不同目录 Sample01 可共存。新批次由现有 ControlRuns 台账独占、串行首次分配并累计保护；失败后不换批次重试。准备副本仅 UI 明确操作，不预留，后续生成再核冲突。

正式组织目录名 NodeFroge 是用户指定拼写，不代表 UE mount；生产完整路径以后落实。现有 NodeForge 工作区/插件/模块不改名，不能向任意生产目录生成。

公共原契约 [GraphProtocol](../../../../../Contracts/GraphProtocol.md)保留历史快照；本页已按 Build122 的子目录规则整理，不能按旧正文“平铺/24节点”解释新版。对象限制仍以当前真实目录与核心为准。


三族当前完整v3输入与已使用身份/实际父SHA说明见[当前完整JSON例子；历史原件内部保留](../Ai/JsonArchitectureExamples.md)。

三族当前完整v3输入与已使用身份/实际父SHA说明见[当前完整JSON例子；历史原件内部保留](../Ai/JsonArchitectureExamples.md)。

三族当前完整v3输入与已使用身份/实际父SHA说明见[当前完整JSON例子；历史原件内部保留](../Ai/JsonArchitectureExamples.md)。

三族当前完整v3输入与已使用身份/实际父SHA说明见[完整例子](../Ai/JsonArchitectureExamples.md)。