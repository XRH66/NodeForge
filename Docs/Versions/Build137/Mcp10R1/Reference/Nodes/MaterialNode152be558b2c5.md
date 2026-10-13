# 材质数学：Logarithm2

用途：材质数学：Logarithm2。

前提：普通 Surface 材质；按本页原生 inputs 名和 outputs 数字索引接线。必需输入要连接，每个表达式必须到达活动材质输出，未知配置不能省略成另一语义。

限制：只可写本页 properties 中的标量/bool，未声明字段保留原生默认；参数范围与引脚 defaultProperty 按原生目录和协议，Period 必须大于零。不提供资源/GUID/动态端口适配；Shader 编译按实际组合，材质画面和运行另列，不能从构造推断。

精确身份（绑定本包；不能凭序号跨版匹配）：

```json
{
  "assetKind": "Material",
  "class": "/Script/Engine.MaterialExpressionLogarithm2"
}
```

当前原目录属性/允许默认/引脚与类型：

```json
{
  "class": "/Script/Engine.MaterialExpressionLogarithm2",
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
  "class": "/Script/Engine.MaterialExpressionLogarithm2",
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
  },
  {
    "level": "representativeGraphGenerationExportAndIndependentVerify",
    "build": "Build132",
    "logicalId": "Log",
    "configuration": {
      "class": "/Script/Engine.MaterialExpressionLogarithm2",
      "properties": {}
    },
    "reports": {
      "input": {
        "path": "internalEvidenceRetained",
        "sha256": "6eeada83777f87424aa0257ec88a397492e7801b67feed8060eae15899b185ff",
        "bytes": 4250
      },
      "expected": {
        "path": "internalEvidenceRetained",
        "sha256": "b93435e8444a7b684705204b92d55f6481023b68883c027e94b274bfdf60604a",
        "bytes": 5418
      },
      "generation": {
        "path": "internalEvidenceRetained",
        "sha256": "14f1290f9f23aa4c05740712db2864cb0b9787215bf975c0e72c2887da402a26",
        "bytes": 34987
      },
      "generationReview": {
        "path": "internalEvidenceRetained",
        "sha256": "abd4516941ba564de0c2bf875be84f9e2df3c51e3fafef3d083d75a0308e3c03",
        "bytes": 395
      },
      "verify": {
        "path": "internalEvidenceRetained",
        "sha256": "bb92be625a5894752272be8b4a80a3d546683671734660daa3e7045d5e924677",
        "bytes": 24966
      },
      "verifyReview": {
        "path": "internalEvidenceRetained",
        "sha256": "10b38b2cc8d960e961600cc754f9acf2e336db101804aaa8c5a444380c26ba20",
        "bytes": 524
      },
      "exportReview": {
        "path": "internalEvidenceRetained",
        "sha256": "d64a46b94be08934c61409bd8658badb30423d00baf9f81f07085ed2c33f5f8f",
        "bytes": 980
      },
      "payload": {
        "path": "internalEvidenceRetained",
        "sha256": "168912b411f62774316f8c55507ae457949c150af8f95adb59c9922b4e595b94",
        "bytes": 4505
      },
      "sourceAsset": {
        "path": "internalEvidenceRetained",
        "sha256": "90233253265152151f53bb36fe8337471641412b0900f38b4479fba871d54c79"
      },
      "receipt": {
        "path": "internalEvidenceRetained",
        "sha256": "88184906da9666a135e6d1579c6c17fadce4d46e6bc89b948ae9e1d4b1e643ab"
      }
    },
    "runtime": "notRun",
    "visual": "notRun",
    "package": "notRun",
    "allConfigurationsVerified": false
  }
]
```

返回[族索引](../MaterialNodeIndex.md)、[族限制](../Material.md)和[验证说明](../../Developer/TestingAndEvidence.md)。
