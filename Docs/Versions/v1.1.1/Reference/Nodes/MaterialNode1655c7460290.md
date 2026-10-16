# 命名二维贴图采样参数

支持状态：**实验性（experimental）**。请先预检，再生成并检查实际结果。

## 使用方法

将下面的节点示例放入材质图的 `nodes` 数组，并用 `edges` 连接到材质输出。输入名称和输出索引见本页的节点参数；每个节点都必须连接到有效的材质输出。

## 节点示例（JSON v3）

```json
{
  "id": "ExampleNode",
  "class": "/Script/Engine.MaterialExpressionTextureSampleParameter2D",
  "properties": {
    "ConstCoordinate": 0,
    "ParameterName": "ExampleParameter",
    "SamplerType": "SAMPLERTYPE_Color",
    "Texture": "/Engine/EngineResources/DefaultTexture.DefaultTexture"
  }
}
```

这是单个节点片段。完整请求还需要目标路径、节点连线和资产设置，参见[JSON 编写说明](../GraphAuthoring.md)和[完整示例](../../Ai/JsonArchitectureExamples.md)。

同一材质中的参数节点应使用不同的 `ParameterName`。

贴图必须是工程允许读取范围内已经保存的普通 Texture2D，不能使用虚拟纹理。填写真实的资源路径，并确认采样器类型与贴图一致；资源查询方法见[资源读取](../../Ai/Resources.md)。

## 节点参数

以下 JSON 列出 UE 类名、允许属性、输入输出类型及默认值。`properties` 中的说明对象用于查阅；提交请求时，应填写上面节点示例那样的实际属性值。

```json
{
  "class": "/Script/Engine.MaterialExpressionTextureSampleParameter2D",
  "discovered": true,
  "constructible": true,
  "family": "ExistingMaterialFamilies",
  "minimumMaterialRevision": 1,
  "supportState": "experimental",
  "verified": "notEstablishedByCatalog",
  "inputs": [
    {
      "name": "Coordinates",
      "nativeType": 15,
      "required": false,
      "defaultProperty": "ConstCoordinate",
      "defaultValue": 0
    },
    {
      "name": "Apply View MipBias",
      "nativeType": 512,
      "required": false
    }
  ],
  "outputs": [
    {
      "index": "0",
      "name": "RGB",
      "nativeType": 4,
      "mask": 1,
      "maskR": 1,
      "maskG": 1,
      "maskB": 1,
      "maskA": 0
    },
    {
      "index": "1",
      "name": "R",
      "nativeType": 15,
      "mask": 1,
      "maskR": 1,
      "maskG": 0,
      "maskB": 0,
      "maskA": 0
    },
    {
      "index": "2",
      "name": "G",
      "nativeType": 15,
      "mask": 1,
      "maskR": 0,
      "maskG": 1,
      "maskB": 0,
      "maskA": 0
    },
    {
      "index": "3",
      "name": "B",
      "nativeType": 15,
      "mask": 1,
      "maskR": 0,
      "maskG": 0,
      "maskB": 1,
      "maskA": 0
    },
    {
      "index": "4",
      "name": "A",
      "nativeType": 15,
      "mask": 1,
      "maskR": 0,
      "maskG": 0,
      "maskB": 0,
      "maskA": 1
    },
    {
      "index": "5",
      "name": "RGBA",
      "nativeType": 8,
      "mask": 1,
      "maskR": 1,
      "maskG": 1,
      "maskB": 1,
      "maskA": 1
    }
  ],
  "properties": {
    "ConstCoordinate": {
      "nativeType": "uint8",
      "default": 0,
      "valueSchema": {
        "type": "integer",
        "minimum": 0,
        "maximum": 7
      }
    },
    "ParameterName": {
      "nativeType": "FName",
      "default": "Param",
      "valueSchema": {
        "type": "string",
        "pattern": "^[A-Za-z0-9]{1,64}$"
      }
    },
    "SamplerType": {
      "nativeType": "TEnumAsByte<EMaterialSamplerType>",
      "default": "SAMPLERTYPE_Color",
      "valueSchema": {
        "type": "string",
        "enum": [
          "SAMPLERTYPE_Color",
          "SAMPLERTYPE_Grayscale",
          "SAMPLERTYPE_Alpha",
          "SAMPLERTYPE_Normal",
          "SAMPLERTYPE_Masks",
          "SAMPLERTYPE_DistanceFieldFont",
          "SAMPLERTYPE_LinearColor",
          "SAMPLERTYPE_LinearGrayscale",
          "SAMPLERTYPE_Data",
          "SAMPLERTYPE_External"
        ]
      },
      "enumNames": [
        "SAMPLERTYPE_Color",
        "SAMPLERTYPE_Grayscale",
        "SAMPLERTYPE_Alpha",
        "SAMPLERTYPE_Normal",
        "SAMPLERTYPE_Masks",
        "SAMPLERTYPE_DistanceFieldFont",
        "SAMPLERTYPE_LinearColor",
        "SAMPLERTYPE_LinearGrayscale",
        "SAMPLERTYPE_Data",
        "SAMPLERTYPE_External"
      ]
    },
    "Texture": {
      "nativeType": "TObjectPtr<UTexture>",
      "default": "/Engine/EngineResources/DefaultTexture.DefaultTexture",
      "valueSchema": {
        "type": "string",
        "pattern": "^(?:/Engine/|/Game/)(?!.*\\.\\.).*\\..+$",
        "x-nativeConstraint": "actual canonical existing nonvirtual Texture2D; sampler checked natively"
      }
    }
  }
}
```

## 节点标识

```json
{
  "assetKind": "Material",
  "class": "/Script/Engine.MaterialExpressionTextureSampleParameter2D"
}
```

返回[节点列表](../MaterialNodeIndex.md) · 阅读[材质使用范围](../Material.md)。
