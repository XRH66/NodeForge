# 材质设置与蓝图变量属性

本页说明 NodeForge v0.2.0 支持的材质设置和变量编辑属性，并给出完整 JSON。节点目录仍为 `experimental`，提交前需要预检。

## 材质设置

在 UE 材质 Details 中可以查看 Blend Mode、Shading Model、Two Sided、Opacity Mask Clip Value、Dither Opacity Mask、Dithered LOD Transition 和 Allow Negative Emissive Color。

v3 放在 `asset.settings`，v2 放在 `settings`。支持规则如下：

- domain 仅支持 Surface；blend 为 Opaque、Translucent 或 Masked；shading 为 Unlit 或 DefaultLit。
- Translucent 必须显式设置 `lighting=SurfaceForwardShading`。
- `opacityMaskClipValue` 是 0–1 的有限数值，Masked 必须显式填写。允许省略的输入采用原生默认值 0.3333。
- `ditherOpacityMask`、`ditheredLodTransition`、`allowNegativeEmissiveColor` 省略时为 false；允许负自发光只适用于 Unlit。
- 根输入能否连接由原生 `IsPropertyActive` 决定。Masked 使用 OpacityMask，Unlit 使用 EmissiveColor。

Opaque 中可以保存 clip/dither 值，但它们当前不起遮罩作用。切换模式后应重新预检连线，插件不会自动删除边。TAA 和 LOD 相关画面效果尚未验证。

## 蓝图变量属性

v3 的变量位于 `asset.members.variables`，type 使用 `{kind:"Value",name:类型名}`；v2 位于 `settings.variables`，type 是字符串。可用类型见 [蓝图说明](../Reference/Blueprint.md)。

`attributes` 整组可以省略；出现时必须恰好包含三个字段：

| 字段 | 类型与范围 |
| --- | --- |
| `instanceEditable` | boolean，是否允许在 Actor 实例上编辑 |
| `category` | 1–128 个 Unicode 码点，无首尾空白或控制字符；竖线分隔的各段不能为空 |
| `tooltip` | 0–1024 个 Unicode 码点，可含中文和换行，不允许 NUL；空串清除用户提示文本 |

显式 false、0、空串与省略字段不同。string 类型保留原生 MultiLine 元数据。任意 metadata、额外变量类型、组件、Spawn 或 SaveGame 等属性未开放。

在 UE 蓝图中选择变量即可查看 Instance Editable、Category 和 Tooltip。修改实例值不会自动改变类默认值；检查画面和最终保存结果是独立步骤。

## Masked 材质示例

以下图使用当前目录定义。生成前选择未占用的 requestId 与目标路径，检查实际目录和依赖。

### JSON v3

```json
{
  "format": "NodeForgeGraph",
  "version": 3,
  "requestId": "ExampleMaterial9",
  "catalogDigest": "be97bf57d226778102038c261c556b89b262197c78158fe16ca293de709d8847",
  "target": {
    "assetPath": "/Game/NodeForgeTests/Examples/Sample9"
  },
  "asset": {
    "type": "Material",
    "settings": {
      "domain": "Surface",
      "blend": "Masked",
      "shading": "Unlit",
      "twoSided": true,
      "opacityMaskClipValue": 0.5,
      "ditherOpacityMask": true,
      "ditheredLodTransition": true,
      "allowNegativeEmissiveColor": true
    }
  },
  "graphs": [
    {
      "id": "MaterialGraph",
      "kind": "Material",
      "nodes": [
        {
          "id": "Glow",
          "class": "/Script/Engine.MaterialExpressionConstant",
          "properties": {
            "R": 1
          }
        },
        {
          "id": "Mask",
          "class": "/Script/Engine.MaterialExpressionConstant",
          "properties": {
            "R": 0.6
          }
        }
      ],
      "edges": [
        {
          "from": "Glow",
          "output": "0",
          "to": "Material",
          "input": "EmissiveColor"
        },
        {
          "from": "Mask",
          "output": "0",
          "to": "Material",
          "input": "OpacityMask"
        }
      ]
    }
  ]
}
```

### 等价 JSON v2

```json
{
  "format": "NodeForgeGraph",
  "version": 2,
  "requestId": "ExampleMaterial10",
  "catalogDigest": "be97bf57d226778102038c261c556b89b262197c78158fe16ca293de709d8847",
  "assetType": "Material",
  "assetPath": "/Game/NodeForgeTests/Examples/Sample10",
  "settings": {
    "domain": "Surface",
    "blend": "Masked",
    "shading": "Unlit",
    "twoSided": true,
    "opacityMaskClipValue": 0.5,
    "ditherOpacityMask": true,
    "ditheredLodTransition": true,
    "allowNegativeEmissiveColor": true
  },
  "nodes": [
    {
      "id": "Glow",
      "class": "/Script/Engine.MaterialExpressionConstant",
      "properties": {
        "R": 1
      }
    },
    {
      "id": "Mask",
      "class": "/Script/Engine.MaterialExpressionConstant",
      "properties": {
        "R": 0.6
      }
    }
  ],
  "edges": [
    {
      "from": "Glow",
      "output": "0",
      "to": "Material",
      "input": "EmissiveColor"
    },
    {
      "from": "Mask",
      "output": "0",
      "to": "Material",
      "input": "OpacityMask"
    }
  ]
}
```

## 变量属性示例

### JSON v3

```json
{
  "format": "NodeForgeGraph",
  "version": 3,
  "requestId": "ExampleBlueprint11",
  "catalogDigest": "be97bf57d226778102038c261c556b89b262197c78158fe16ca293de709d8847",
  "target": {
    "assetPath": "/Game/NodeForgeTests/Examples/Sample11"
  },
  "asset": {
    "type": "Blueprint",
    "parent": {
      "class": "/Script/Engine.Actor"
    },
    "members": {
      "variables": [
        {
          "name": "Enabled",
          "type": {
            "kind": "Value",
            "name": "bool"
          },
          "default": false,
          "attributes": {
            "instanceEditable": true,
            "category": "设置|基础",
            "tooltip": "是否启用"
          }
        },
        {
          "name": "Count",
          "type": {
            "kind": "Value",
            "name": "int"
          },
          "default": 0,
          "attributes": {
            "instanceEditable": false,
            "category": "内部",
            "tooltip": ""
          }
        },
        {
          "name": "Speed",
          "type": {
            "kind": "Value",
            "name": "float"
          },
          "default": 100,
          "attributes": {
            "instanceEditable": true,
            "category": "设置|运动",
            "tooltip": "速度，厘米每秒"
          }
        },
        {
          "name": "Label",
          "type": {
            "kind": "Value",
            "name": "string"
          },
          "default": "",
          "attributes": {
            "instanceEditable": true,
            "category": "设置|基础",
            "tooltip": "显示文本"
          }
        },
        {
          "name": "Tag",
          "type": {
            "kind": "Value",
            "name": "name"
          },
          "default": "Demo",
          "attributes": {
            "instanceEditable": true,
            "category": "设置|基础",
            "tooltip": "名称"
          }
        },
        {
          "name": "Offset",
          "type": {
            "kind": "Value",
            "name": "vector"
          },
          "default": [
            0,
            0,
            0
          ],
          "attributes": {
            "instanceEditable": true,
            "category": "设置|运动",
            "tooltip": "偏移"
          }
        },
        {
          "name": "Facing",
          "type": {
            "kind": "Value",
            "name": "rotator"
          },
          "default": {
            "pitch": 0,
            "yaw": 0,
            "roll": 0
          },
          "attributes": {
            "instanceEditable": false,
            "category": "内部",
            "tooltip": "初始角度"
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
          "id": "GetSpeed",
          "class": "/Script/BlueprintGraph.K2Node_VariableGet",
          "properties": {},
          "binding": {
            "kind": "Member",
            "name": "Speed"
          }
        }
      ],
      "edges": []
    }
  ]
}
```

### 等价 JSON v2

```json
{
  "format": "NodeForgeGraph",
  "version": 2,
  "requestId": "ExampleBlueprint12",
  "catalogDigest": "be97bf57d226778102038c261c556b89b262197c78158fe16ca293de709d8847",
  "assetType": "Blueprint",
  "assetPath": "/Game/NodeForgeTests/Examples/Sample12",
  "settings": {
    "parentClass": "/Script/Engine.Actor",
    "variables": [
      {
        "name": "Enabled",
        "type": "bool",
        "default": false,
        "attributes": {
          "instanceEditable": true,
          "category": "设置|基础",
          "tooltip": "是否启用"
        }
      },
      {
        "name": "Count",
        "type": "int",
        "default": 0,
        "attributes": {
          "instanceEditable": false,
          "category": "内部",
          "tooltip": ""
        }
      },
      {
        "name": "Speed",
        "type": "float",
        "default": 100,
        "attributes": {
          "instanceEditable": true,
          "category": "设置|运动",
          "tooltip": "速度，厘米每秒"
        }
      },
      {
        "name": "Label",
        "type": "string",
        "default": "",
        "attributes": {
          "instanceEditable": true,
          "category": "设置|基础",
          "tooltip": "显示文本"
        }
      },
      {
        "name": "Tag",
        "type": "name",
        "default": "Demo",
        "attributes": {
          "instanceEditable": true,
          "category": "设置|基础",
          "tooltip": "名称"
        }
      },
      {
        "name": "Offset",
        "type": "vector",
        "default": [
          0,
          0,
          0
        ],
        "attributes": {
          "instanceEditable": true,
          "category": "设置|运动",
          "tooltip": "偏移"
        }
      },
      {
        "name": "Facing",
        "type": "rotator",
        "default": {
          "pitch": 0,
          "yaw": 0,
          "roll": 0
        },
        "attributes": {
          "instanceEditable": false,
          "category": "内部",
          "tooltip": "初始角度"
        }
      }
    ]
  },
  "nodes": [
    {
      "id": "GetSpeed",
      "class": "/Script/BlueprintGraph.K2Node_VariableGet",
      "properties": {
        "variable": "Speed"
      }
    }
  ],
  "edges": []
}
```

## 提交与检查

按 [调用流程](QuickStart.md)连接、载入完整 JSON、读取新修订、预检，再生成。保留每次调用的 ID 和报告。`waitingPersistence` 按 [保存核验](Preparation.md)检查，`pending` 或 `unknown` 先查询原调用，不换目标补发。

导出已有材质或蓝图时，指定准确 source 和未占用的新 requestId/assetPath。省略 formatVersion 返回 v2，显式传 3 返回 v3。用 `/exportedRequest` 取得完整 text 和 sourceRef，再成对传入 exportReportPath/exportReportSha256 载入来源。

编辑已载入图时设置 `retainExportOrigin:true`，不要同时传新的来源报告字段。修改 twoSided 或变量 category 后，重新读取修订，再准备副本和预览。差异报告 `/graphDifference` 的 settingsChanged 显示相关变化。准备只提交新草稿和记录，不预留目标或生成资产，详见 [副本编辑](CopyPreview.md)。

## 开发实现

`GraphDocument` 保留可选字段及其出现状态，`AssetDefinitions` 定义普通属性、原生名称、默认值和 Schema。`GraphCodec` 的 v2/v3 共用这些定义。材质的 Blend/Shading/lighting 与 `IsPropertyActive` 使用专门校验。

蓝图通过 `SetBlueprintOnlyEditableFlag(!instanceEditable)`、`SetBlueprintVariableCategory(...,bDontRecompile=true)` 和 Tooltip Set/Remove 应用设置，最后统一编译并读取 NewVariables、FProperty 和 CDO。`GraphDiff` 比较共同模型中的 Settings。修改这些逻辑时需要同步生成、回读、导出和 [字段说明](../Reference/GraphAuthoring.md)。
