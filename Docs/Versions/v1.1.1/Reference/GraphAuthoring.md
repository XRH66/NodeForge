# 编写图形 JSON

NodeForge 使用 `NodeForgeGraph` JSON 描述目标资产、节点和连线。v1.1.1 支持格式 v2 和 v3；这里的数字是 JSON 格式版本。新内容建议使用 v3。

## 从示例开始

1. 选择[材质、蓝图或材质实例的完整 v3 示例](../Ai/JsonArchitectureExamples.md)。
2. 将 `requestId` 改为新的请求编号，将目标 `assetPath` 改为尚未存在的路径。
3. 查询当前节点目录，使用返回的 `catalogDigest`。这个值是目录内容摘要，不是目录文件的 SHA256。
4. 修改节点、参数和连线，先运行预检；错误修正后再生成。

## v3 的结构

根对象必须包含以下七个字段：

| 字段 | 用途 |
| --- | --- |
| `format` | 固定为 `NodeForgeGraph` |
| `version` | 固定为 `3` |
| `requestId` | 本次生成请求的唯一编号 |
| `catalogDigest` | 当前插件返回的节点目录摘要 |
| `target` | 目标资产路径，使用 `assetPath` 字段 |
| `asset` | 资产类型、材质设置或蓝图成员等信息 |
| `graphs` | 图、节点和连线；材质实例使用空数组 |

节点使用 `id`、`class`、`properties`，按需增加 `binding`、`defaults` 和 `position`。`binding` 指定函数、事件、变量或材质函数的绑定。具体字段见各[材质节点](MaterialNodeIndex.md)和[蓝图节点](BlueprintNodeIndex.md)页面。

v2 使用九个根字段：`format`、`version`、`requestId`、`assetType`、`assetPath`、`catalogDigest`、`nodes`、`edges`、`settings`。对应的完整请求见[v2 示例](Examples.md)。导出图时显式设置 `formatVersion=3` 可得到 v3，省略该选项时导出 v2；准备副本、预览和重排会保留输入格式。

## 连线与位置

每条边包含 `from`、`output`、`to`、`input` 四个字符串字段。它们分别指定源节点、源输出、目标节点和目标输入；名称和大小写必须与节点目录一致。

一个输入最多连接一条边，不能同时设置默认值。自连、环路、自动类型转换和替换现有连线不受支持。节点数组的顺序不会决定图的执行顺序。

可选的 `position` 是两个整数；材质坐标的绝对值不超过 1,000,000，蓝图不超过 100,000。NodeForge 会保留指定位置，并为其他节点安排布局。

## 目标路径与大小限制

目标路径必须位于 `/Game/NodeForgeTests/`，可增加最多两级子目录。每个子目录名使用 1–16 个 ASCII 字母或数字，资产名使用 1–64 个。不能带空格、下划线、扩展名、反斜线或 `.`、`..` 路径段。

已有资产路径或已使用的 `requestId` 会被拒绝。需要修改已有内容时，先导出并准备一个新副本，再生成到新路径。

JSON 文件需为无 BOM 的 UTF-8，大小为 1–65,536 字节，嵌套深度不超过 32。重复键、NUL、注释、尾逗号和未知字段均会被拒绝。节点 ID 使用 1–64 个 ASCII 字母或数字；`Material` 是材质输出的保留名称。

完整结构定义见 [GraphRequestSchema.json](../GraphRequestSchema.json)。资产、资源和引脚还会在预检时检查，JSON 格式正确后仍需处理这些检查结果。

## 字段定义

以下定义列出字段的类型、范围和默认值。

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
          "DefaultLit",
          "ClearCoat"
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
        "name": "refractionMethod",
        "type": "string",
        "required": false,
        "enum": [
          "None",
          "IndexOfRefraction"
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
          "DefaultLit",
          "ClearCoat"
        ]
      },
      "lighting": {
        "type": "string",
        "enum": [
          "SurfaceForwardShading"
        ]
      },
      "refractionMethod": {
        "type": "string",
        "enum": [
          "None",
          "IndexOfRefraction"
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
      },
      {
        "if": {
          "properties": {
            "refractionMethod": {
              "const": "IndexOfRefraction"
            }
          },
          "required": [
            "refractionMethod"
          ]
        },
        "then": {
          "properties": {
            "blend": {
              "const": "Translucent"
            },
            "shading": {
              "const": "DefaultLit"
            }
          }
        }
      },
      {
        "if": {
          "properties": {
            "shading": {
              "const": "ClearCoat"
            }
          }
        },
        "then": {
          "properties": {
            "blend": {
              "enum": [
                "Opaque",
                "Masked"
              ]
            }
          }
        }
      }
    ],
    "description": "Surface only. ClearCoat requires Opaque/Masked; IndexOfRefraction requires Translucent DefaultLit and lighting. Omitted refractionMethod is None. Inactive clip/dither values are stored; no visual/TAA/LOD claim. Root connections must pass native IsPropertyActive."
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


## 1.1.1 清漆与折射契约

当前新增字段、根输入类型、切线空间和 CLI 进程查询见[完整契约](LayeredMaterials.md)。新 Catalog/Schema 来自原生共享定义；旧输入需重新预检。
