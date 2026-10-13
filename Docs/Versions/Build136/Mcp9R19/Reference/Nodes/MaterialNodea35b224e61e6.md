# 材质数学：Logarithm10

用途：材质数学：Logarithm10。

前提：普通 Surface 材质；按本页原生 inputs 名和 outputs 数字索引接线。必需输入要连接，每个表达式必须到达活动材质输出，未知配置不能省略成另一语义。

限制：只可写本页 properties 中的标量/bool，未声明字段保留原生默认；参数范围与引脚 defaultProperty 按原生目录和协议，Period 必须大于零。不提供资源/GUID/动态端口适配；Shader 编译按实际组合，材质画面和运行另列，不能从构造推断。

精确身份（绑定本包；不能凭序号跨版匹配）：

```json
{
  "assetKind": "Material",
  "class": "/Script/Engine.MaterialExpressionLogarithm10"
}
```

当前原目录属性/允许默认/引脚与类型：

```json
{
  "class": "/Script/Engine.MaterialExpressionLogarithm10",
  "discovered": true,
  "constructible": true,
  "family": "OrdinarySurfaceMath",
  "minimumMaterialRevision": 6,
  "supportState": "experimental",
  "verified": "notEstablishedByCatalog",
  "inputs": [
    {
      "name": "X",
      "nativeType": 15,
      "required": true
    }
  ],
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
  "properties": {}
}
```

当前v3声明片段（v2使用下方历史绑定表示）：

```json
{
  "id": "ExampleNode",
  "class": "/Script/Engine.MaterialExpressionLogarithm10",
  "properties": {}
}
```

这是准确单节点声明片段，使用原生默认；不是完整可提交图。完整配方见 AiNodeBulk。先按真实目录补必需连接，再用实际分配的 requestId/assetPath 和本版完整摘要预检。

来源：Plugins/NodeForge/Source/NodeForgeEditor/Private/NodeForgeNodeDefinitions.cpp。
历史记录保留原身份/配置/Build/层级，缺证据保持notEstablished：

```json
[]
```

本期资料引用的实际证据（无条目时 notEstablished；签名证据不能代替组合编译/保存/Verify）：

```json
[
  {
    "level": "nativeConstructionAndExactSignature",
    "build": "Build132",
    "report": {
      "path": "internalEvidenceRetained",
      "sha256": "f927284177028bab2fa57677ecb4e614497e0fca1964e688a484c6b60f5767ad",
      "bytes": 582956
    },
    "overallStatus": "passed",
    "meaning": "actual production spawner/factory/catalog qualification; not representative compile/save/Verify/runtime"
  }
]
```

返回[族索引](../MaterialNodeIndex.md)、[族限制](../Material.md)和[验证说明](../../Developer/TestingAndEvidence.md)。
