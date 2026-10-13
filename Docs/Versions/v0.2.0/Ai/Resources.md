# 查找资源并在图中使用

先查询资源位置，再检查实际类型和参数，最后把检查结果用于图。NodeForge v0.2.0 支持查询 Texture2D、MaterialFunction 和 Material；查询结果本身不会加载对象或修改草稿。

## 1. 连接并准备参数

完成 [连接准备](Preparation.md)，读取 [功能说明](../Reference/FeatureMap.md)和当前目录。用 `connect` 返回的 handle 与 `draft_get` 返回的修订替换下方占位值，每次新操作使用单独的 callId。

示例中的全零 handle、修订 0、全零 SHA 和 Allocated 前缀用于展示格式，不是可直接提交的会话或资源证明。

## 2. 查询指定目录

调用 `nodeforge_resources_query`：

```json
{
  "handle": "00000000000000000000000000000000",
  "callId": "AllocatedTextureQuery",
  "expectedDraftRevision": "0",
  "arguments": {
    "kind": "Texture2D",
    "directory": "/Engine/EngineResources",
    "nameContains": "Gray",
    "recursive": false,
    "limit": 50,
    "after": ""
  }
}
```

| 参数 | 规则 |
| --- | --- |
| `kind` | Texture2D、MaterialFunction 或 Material |
| `directory` | 明确允许的子目录，不能为空，也不能扫描根目录 |
| `nameContains` | 最多 128 字符，默认空字符串 |
| `recursive` | 默认 false |
| `limit` | 1–200 的整数，默认 50 |
| `after` | 下一页游标，最多 512 字符，默认空字符串 |

查询最多遍历 16 层目录、128 个目录和 5000 个条目。MaterialFunction 可使用已允许的 `/Engine/Functions/Engine_MaterialFunctions02` 子目录，Material 使用工程绑定允许的内容子目录。

从返回 reports 取得 reportId，再读取 `/resourceQuery`。结果包含过滤条件、路径、类、名称、注册表状态、complete/truncated、next 和 rows。`discovered` 或 `policyAllowed` 不等于检查通过；`requireInspect=true` 表示还需精确检查。

complete=false、仍在扫描、达到上限或缺少元数据时，先缩小目录或处理登记状态。只有列表完整且有 next 时，才把 next 原样传入 after，其他条件保持不变。目录可能在分页期间变化，可按 objectPath 去重。

结果超出读取大小限制时，按 navigation 读取 `/resourceQuery/rows/0` 等小片段。

## 3. 检查纹理

取得精确 objectPath 后，调用 `nodeforge_resources_inspect`：

```json
{
  "handle": "00000000000000000000000000000000",
  "callId": "AllocatedTextureInspect",
  "expectedDraftRevision": "0",
  "arguments": {
    "kind": "Texture2D",
    "source": "/Engine/EngineResources/Gray.Gray"
  }
}
```

查看 `/resourceRead` 和 `/texture`：资源必须已保存、未修改，类型为非虚拟 Texture2D。结果包含文件 SHA、尺寸、sRGB、压缩和原生采样器。

`/texture/properties` 中的 Texture 与 SamplerType 可用于纹理节点 properties，无需添加 TextureSha 字段。正式预检和生成会再次检查资源。已加载但无法确认保存文件来源的对象会返回 `sourceMemoryUnproven`，插件不会自动重载或保存它。

UE 5.3.2 可能预加载 WhiteSquareTexture，使其触发上述检查。下例使用 Gray；仍应检查实际对象，并采用这次返回的 SamplerType。

## 4. 检查材质函数或父材质

材质函数使用相同工具：

```json
{
  "handle": "00000000000000000000000000000000",
  "callId": "AllocatedFunctionInspect",
  "expectedDraftRevision": "0",
  "arguments": {
    "kind": "MaterialFunction",
    "source": "/Engine/Functions/Engine_MaterialFunctions02/SmoothStep.SmoothStep"
  }
}
```

`/functionSignature` 和 `/nodeTemplate` 给出实际输入、输出、默认策略和直接文件 SHA。v2 使用 properties.MaterialFunction/FunctionAssetSha256；v3 使用 `binding:{kind:"MaterialFunction",path,assetSha256}`，properties 为空。

不要猜函数引脚，也不要复用不匹配的 SHA。函数图创建、整图导出和逐调用默认值适配不在当前范围内。

普通父材质用 `kind=MaterialParameters` 读取参数和父文件 SHA。导出完整材质或蓝图使用 `graphs.export`，详见 [资源读取与导出](AssetRead.md)。

## 5. 完整材质示例

这个材质把世界 X 坐标的 SmoothStep 结果连接到 OpacityMask，将 Gray 纹理乘颜色后连接到 EmissiveColor，采用 Surface/Masked/Unlit。

使用前检查函数 SHA、引脚与纹理，替换请求、目标和资源占位值。目录摘要必须来自当前目录，预检仍会检查节点是否支持。

### JSON v3

```json
{
  "format": "NodeForgeGraph",
  "version": 3,
  "requestId": "ReplaceWithNewRequest",
  "catalogDigest": "be97bf57d226778102038c261c556b89b262197c78158fe16ca293de709d8847",
  "target": {
    "assetPath": "/Game/NodeForgeTests/ExampleRun/MResource"
  },
  "asset": {
    "type": "Material",
    "settings": {
      "domain": "Surface",
      "blend": "Masked",
      "shading": "Unlit",
      "twoSided": true,
      "opacityMaskClipValue": 0.5,
      "ditheredLodTransition": false,
      "ditherOpacityMask": false,
      "allowNegativeEmissiveColor": false
    }
  },
  "graphs": [
    {
      "id": "MaterialGraph",
      "kind": "Material",
      "nodes": [
        {
          "id": "Position",
          "class": "/Script/Engine.MaterialExpressionWorldPosition",
          "properties": {
            "WorldPositionShaderOffset": "WPT_Default"
          }
        },
        {
          "id": "X",
          "class": "/Script/Engine.MaterialExpressionComponentMask",
          "properties": {
            "R": true,
            "G": false,
            "B": false,
            "A": false
          }
        },
        {
          "id": "Min",
          "class": "/Script/Engine.MaterialExpressionConstant",
          "properties": {
            "R": -50
          }
        },
        {
          "id": "Max",
          "class": "/Script/Engine.MaterialExpressionConstant",
          "properties": {
            "R": 50
          }
        },
        {
          "id": "Smooth",
          "class": "/Script/Engine.MaterialExpressionMaterialFunctionCall",
          "binding": {
            "kind": "MaterialFunction",
            "path": "/Engine/Functions/Engine_MaterialFunctions02/SmoothStep.SmoothStep",
            "assetSha256": "6738a8b58e1f9b0448ac3b2c50f8ee621fb1870a0c2693542d7ea06ec83d6aa5"
          },
          "properties": {}
        },
        {
          "id": "UV",
          "class": "/Script/Engine.MaterialExpressionTextureCoordinate",
          "properties": {
            "UTiling": 1,
            "VTiling": 1
          }
        },
        {
          "id": "Image",
          "class": "/Script/Engine.MaterialExpressionTextureSampleParameter2D",
          "properties": {
            "Texture": "/Engine/EngineResources/Gray.Gray",
            "ParameterName": "SurfaceTexture",
            "SamplerType": "SAMPLERTYPE_Color"
          }
        },
        {
          "id": "Tint",
          "class": "/Script/Engine.MaterialExpressionConstant3Vector",
          "properties": {
            "Constant": [
              0.8,
              0.1,
              0.05,
              1
            ]
          }
        },
        {
          "id": "Color",
          "class": "/Script/Engine.MaterialExpressionMultiply",
          "properties": {}
        }
      ],
      "edges": [
        {
          "from": "Position",
          "output": "0",
          "to": "X",
          "input": "Input"
        },
        {
          "from": "X",
          "output": "0",
          "to": "Smooth",
          "input": "Alpha (S)"
        },
        {
          "from": "Min",
          "output": "0",
          "to": "Smooth",
          "input": "Min (S)"
        },
        {
          "from": "Max",
          "output": "0",
          "to": "Smooth",
          "input": "Max (S)"
        },
        {
          "from": "Smooth",
          "output": "0",
          "to": "Material",
          "input": "OpacityMask"
        },
        {
          "from": "UV",
          "output": "0",
          "to": "Image",
          "input": "Coordinates"
        },
        {
          "from": "Image",
          "output": "0",
          "to": "Color",
          "input": "A"
        },
        {
          "from": "Tint",
          "output": "0",
          "to": "Color",
          "input": "B"
        },
        {
          "from": "Color",
          "output": "0",
          "to": "Material",
          "input": "EmissiveColor"
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
  "requestId": "ReplaceWithNewRequest",
  "assetType": "Material",
  "assetPath": "/Game/NodeForgeTests/ExampleRun/MResource",
  "catalogDigest": "be97bf57d226778102038c261c556b89b262197c78158fe16ca293de709d8847",
  "settings": {
    "domain": "Surface",
    "blend": "Masked",
    "shading": "Unlit",
    "twoSided": true,
    "opacityMaskClipValue": 0.5,
    "ditheredLodTransition": false,
    "ditherOpacityMask": false,
    "allowNegativeEmissiveColor": false
  },
  "nodes": [
    {
      "id": "Position",
      "class": "/Script/Engine.MaterialExpressionWorldPosition",
      "properties": {
        "WorldPositionShaderOffset": "WPT_Default"
      }
    },
    {
      "id": "X",
      "class": "/Script/Engine.MaterialExpressionComponentMask",
      "properties": {
        "R": true,
        "G": false,
        "B": false,
        "A": false
      }
    },
    {
      "id": "Min",
      "class": "/Script/Engine.MaterialExpressionConstant",
      "properties": {
        "R": -50
      }
    },
    {
      "id": "Max",
      "class": "/Script/Engine.MaterialExpressionConstant",
      "properties": {
        "R": 50
      }
    },
    {
      "id": "Smooth",
      "class": "/Script/Engine.MaterialExpressionMaterialFunctionCall",
      "properties": {
        "MaterialFunction": "/Engine/Functions/Engine_MaterialFunctions02/SmoothStep.SmoothStep",
        "FunctionAssetSha256": "6738a8b58e1f9b0448ac3b2c50f8ee621fb1870a0c2693542d7ea06ec83d6aa5"
      }
    },
    {
      "id": "UV",
      "class": "/Script/Engine.MaterialExpressionTextureCoordinate",
      "properties": {
        "UTiling": 1,
        "VTiling": 1
      }
    },
    {
      "id": "Image",
      "class": "/Script/Engine.MaterialExpressionTextureSampleParameter2D",
      "properties": {
        "Texture": "/Engine/EngineResources/Gray.Gray",
        "ParameterName": "SurfaceTexture",
        "SamplerType": "SAMPLERTYPE_Color"
      }
    },
    {
      "id": "Tint",
      "class": "/Script/Engine.MaterialExpressionConstant3Vector",
      "properties": {
        "Constant": [
          0.8,
          0.1,
          0.05,
          1
        ]
      }
    },
    {
      "id": "Color",
      "class": "/Script/Engine.MaterialExpressionMultiply",
      "properties": {}
    }
  ],
  "edges": [
    {
      "from": "Position",
      "output": "0",
      "to": "X",
      "input": "Input"
    },
    {
      "from": "X",
      "output": "0",
      "to": "Smooth",
      "input": "Alpha (S)"
    },
    {
      "from": "Min",
      "output": "0",
      "to": "Smooth",
      "input": "Min (S)"
    },
    {
      "from": "Max",
      "output": "0",
      "to": "Smooth",
      "input": "Max (S)"
    },
    {
      "from": "Smooth",
      "output": "0",
      "to": "Material",
      "input": "OpacityMask"
    },
    {
      "from": "UV",
      "output": "0",
      "to": "Image",
      "input": "Coordinates"
    },
    {
      "from": "Image",
      "output": "0",
      "to": "Color",
      "input": "A"
    },
    {
      "from": "Tint",
      "output": "0",
      "to": "Color",
      "input": "B"
    },
    {
      "from": "Color",
      "output": "0",
      "to": "Material",
      "input": "EmissiveColor"
    }
  ]
}
```

## 6. 生成与检查结果

保存完整 UTF-8 JSON，通过 `draft_loadFile` 或 `draft_replace` 载入，再读取新的草稿修订。依次调用 `graphs_preflight` 和 `graphs_generate`，每一步使用自己的 callId，通过后再继续。

使用原 callId 查询状态并读取生成报告，检查 inputDigest、目标、资源来源、编译、保存和回读。`waitingPersistence` 按 [保存核验](Preparation.md)处理；只读查询没有生成资产时无需执行这一步。

`pending`、`unknown`、目标占用或来源冲突时，先查原调用并保留结果，不换目标补发。`sourceNotReady` 不等于资源不存在，详见 [结果与恢复](ResultsAndRecovery.md)。
