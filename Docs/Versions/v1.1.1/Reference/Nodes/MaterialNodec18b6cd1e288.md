# 调用材质函数

支持状态：**实验性（experimental）**。请先预检，再生成并检查实际结果。

## 使用方法

将下面的节点示例放入材质图的 `nodes` 数组，并用 `edges` 连接到材质输出。输入名称和输出索引见本页的节点参数；每个节点都必须连接到有效的材质输出。

## 节点示例（JSON v3）

```json
{
  "id": "ExampleNode",
  "class": "/Script/Engine.MaterialExpressionMaterialFunctionCall",
  "properties": {},
  "binding": {
    "kind": "MaterialFunction",
    "path": "/Engine/Functions/Engine_MaterialFunctions01/Texturing/WorldAlignedTexture.WorldAlignedTexture",
    "assetSha256": "a0bd3a9d3762e4e7b3e70446f1bbd1cfbd6df37e25ec6e4f63d2cef5521f6c1a"
  }
}
```

这是单个节点片段。完整请求还需要目标路径、节点连线和资产设置，参见[JSON 编写说明](../GraphAuthoring.md)和[完整示例](../../Ai/JsonArchitectureExamples.md)。

调用前使用 `resources.inspect(kind=MaterialFunction)` 读取函数，取得当前文件摘要和实际输入、输出名称。示例函数路径和摘要需要按读取结果核对。下面空的输入、输出列表表示尚未绑定函数；不能据此认为函数没有引脚。当前不支持函数实例、材质层或函数输入默认值。

## 节点参数

以下 JSON 列出 UE 类名、允许属性、输入输出类型及默认值。`properties` 中的说明对象用于查阅；提交请求时，应填写上面节点示例那样的实际属性值。

```json
{
  "class": "/Script/Engine.MaterialExpressionMaterialFunctionCall",
  "discovered": true,
  "constructible": true,
  "family": "ExistingMaterialFamilies",
  "minimumMaterialRevision": 2,
  "supportState": "experimental",
  "verified": "notEstablishedByCatalog",
  "signaturePolicy": "dynamic: MaterialFunction inspection binds saved actual function and exports native names/indices/GUIDs; unbound inputs/outputs below are not usable signatures",
  "configuration": "exactly MaterialFunction canonical path and FunctionAssetSha256; no function instances/layers; no per-call defaults",
  "inputs": [],
  "outputs": [],
  "properties": {
    "FunctionAssetSha256": {
      "nativeType": "adapter-owned direct asset byte SHA256 guard",
      "required": true
    },
    "MaterialFunction": {
      "nativeType": "saved UMaterialFunction reference bound by SetMaterialFunction",
      "required": true
    }
  }
}
```

## 节点标识

```json
{
  "assetKind": "Material",
  "class": "/Script/Engine.MaterialExpressionMaterialFunctionCall"
}
```

返回[节点列表](../MaterialNodeIndex.md) · 阅读[材质使用范围](../Material.md)。
