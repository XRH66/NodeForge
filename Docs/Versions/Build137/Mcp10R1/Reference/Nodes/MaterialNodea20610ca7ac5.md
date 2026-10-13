# 按比例去饱和

用途：按比例去饱和。

前提：按本页原签名写精确输入/输出与允许属性，连接输入不可同时设default；目录发现不代表任意配置实测。所有节点须到有效Material输出；采样/函数需已存在准入资源及实际签名/SHA。

限制：只支持原目录明确字段/类型/默认；隐藏或不可写引脚不得填写。运行/数值范围/画面按原具体组合证据，不能推导全验。

精确身份（绑定本包；不能凭序号跨版匹配）：

```json
{
  "assetKind": "Material",
  "class": "/Script/Engine.MaterialExpressionDesaturation"
}
```

当前原目录属性/允许默认/引脚与类型：

```json
{
  "class": "/Script/Engine.MaterialExpressionDesaturation",
  "discovered": true,
  "constructible": true,
  "family": "ColorSelection",
  "minimumMaterialRevision": 5,
  "supportState": "experimental",
  "verified": "notEstablishedByCatalog",
  "inputs": [
    {
      "name": "Input",
      "nativeType": 15,
      "required": true
    },
    {
      "name": "Fraction",
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
  "properties": {
    "LuminanceFactors": {
      "nativeType": "FLinearColor",
      "default": [
        0.30000001192092896,
        0.5899999737739563,
        0.10999999940395355,
        0
      ],
      "valueSchema": {
        "type": "array",
        "minItems": 4,
        "maxItems": 4,
        "items": {
          "type": "number",
          "minimum": -1000000,
          "maximum": 1000000
        }
      }
    }
  }
}
```

当前v3声明片段（v2使用下方历史绑定表示）：

```json
{
  "id": "ExampleNode",
  "class": "/Script/Engine.MaterialExpressionDesaturation",
  "properties": {
    "LuminanceFactors": [
      0.30000001192092896,
      0.5899999737739563,
      0.10999999940395355,
      0
    ]
  }
}
```

这是单个节点的属性声明片段，不是可独立提交的完整图；还须满足引脚、连线、可达性和族限制。properties内是实际值，前面的原目录则是属性说明，不能互换。省略项沿原生默认；Time的原生Period=0不可显式写入，需覆盖周期时给正数。多个参数节点须各用唯一ParameterName。

来源：Plugins/NodeForge/Source/NodeForgeEditor/Private/NodeForgeNodeDefinitions.cpp。
历史记录保留原身份/配置/Build/层级，缺证据保持notEstablished：

```json
[
  {
    "verificationSource": "Catalog/UE532MaterialColorSelectionVerification.json",
    "originalBuild": "Build45",
    "originalBinarySha256": "b30fd86a88b05c2ea90ba3db5eb63ca6070d1186ac430b9640bf345443a43fd5",
    "originalCatalogDigest": "96c9459d583fd0c5156842882d5ba68a259db91e5b3db8d495f77be79cfee8c7",
    "matchKind": "nativeIdentity",
    "record": {
      "class": "/Script/Engine.MaterialExpressionDesaturation",
      "minimumMaterialRevision": 5,
      "nativeDiscovered": true,
      "nativeConstructible": true,
      "supportState": "verified",
      "compileSaveIndependentReopen": [
        "internalEvidenceRetained",
        "internalEvidenceRetained",
        "internalEvidenceRetained"
      ],
      "representativeVisual": "internalEvidenceRetained",
      "exportCopyIndependentReopen": [
        "internalEvidenceRetained"
      ],
      "scope": "Representative explicit numeric/color configurations; actual native signatures, properties and connections, no universal UE node coverage",
      "realUi": "notRun",
      "externalAiOriginal": "notRun",
      "noPluginDelivery": "notRun"
    }
  }
]
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
