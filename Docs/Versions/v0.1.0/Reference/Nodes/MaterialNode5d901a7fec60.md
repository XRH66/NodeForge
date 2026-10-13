# 对象世界空间位置

支持状态：**实验性（experimental）**。请先预检，再生成并检查实际结果。

## 使用方法

将下面的节点示例放入材质图的 `nodes` 数组，并用 `edges` 连接到材质输出。输入名称和输出索引见本页的节点参数；每个节点都必须连接到有效的材质输出。

## 节点示例（JSON v3）

```json
{
  "id": "ExampleNode",
  "class": "/Script/Engine.MaterialExpressionObjectPositionWS",
  "properties": {
    "OriginType": "Absolute"
  }
}
```

这是单个节点片段。完整请求还需要目标路径、节点连线和资产设置，参见[JSON 编写说明](../GraphAuthoring.md)和[完整示例](../../Ai/JsonArchitectureExamples.md)。

## 节点参数

以下 JSON 列出 UE 类名、允许属性、输入输出类型及默认值。`properties` 中的说明对象用于查阅；提交请求时，应填写上面节点示例那样的实际属性值。

```json
{
  "class": "/Script/Engine.MaterialExpressionObjectPositionWS",
  "discovered": true,
  "constructible": true,
  "family": "SpatialInputs",
  "minimumMaterialRevision": 4,
  "supportState": "experimental",
  "verified": "notEstablishedByCatalog",
  "inputs": [],
  "outputs": [
    {
      "index": "0",
      "name": "None",
      "nativeType": 15,
      "mask": 0,
      "maskR": 0,
      "maskG": 0,
      "maskB": 0,
      "maskA": 0
    }
  ],
  "properties": {
    "OriginType": {
      "nativeType": "EPositionOrigin",
      "default": "Absolute",
      "valueSchema": {
        "type": "string",
        "enum": [
          "Absolute",
          "CameraRelative"
        ]
      },
      "enumNames": [
        "Absolute",
        "CameraRelative"
      ]
    }
  }
}
```

## 节点标识

```json
{
  "assetKind": "Material",
  "class": "/Script/Engine.MaterialExpressionObjectPositionWS"
}
```

返回[节点列表](../MaterialNodeIndex.md) · 阅读[材质使用范围](../Material.md)。
