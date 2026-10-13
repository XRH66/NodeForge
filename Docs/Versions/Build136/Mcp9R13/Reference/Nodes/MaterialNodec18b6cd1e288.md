# 调用实际已绑定材质函数

用途：调用实际已绑定材质函数。

前提：按本页原签名写精确输入/输出与允许属性，连接输入不可同时设default；目录发现不代表任意配置实测。所有节点须到有效Material输出；采样/函数需已存在准入资源及实际签名/SHA。

限制：只支持原目录明确字段/类型/默认；隐藏或不可写引脚不得填写。运行/数值范围/画面按原具体组合证据，不能推导全验。

精确身份（绑定本包；不能凭序号跨版匹配）：

```json
{
  "assetKind": "Material",
  "class": "/Script/Engine.MaterialExpressionMaterialFunctionCall"
}
```

当前原目录属性/允许默认/引脚与类型：

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

当前v3声明片段（v2使用下方历史绑定表示）：

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

本片段使用本期resources.inspect取得的WorldAlignedTexture直接保存文件SHA与原生绑定；仍是单节点，完整图见AiJsonArchitectureExamples。必须连接原生输入TextureObject (T2d)，可连接TextureSize (V3)，输出2为XYZ Texture；正式前置仍核实际函数当前签名与SHA。没有函数默认值适配或嵌套依赖递归SHA保护。

来源：Plugins/NodeForge/Source/NodeForgeEditor/Private/NodeForgeNodeDefinitions.cpp。
历史记录保留原身份/配置/Build/层级，缺证据保持notEstablished：

```json
[
  {
    "verificationSource": "Catalog/UE532BlueprintMotionVerification.json",
    "originalBuild": "notExplicitInVerification",
    "originalBinarySha256": "b5475c6feb8d6ac7f7011c29b65636aee7711f2a9919d80c5edeb7e5e8e8f7fe",
    "originalCatalogDigest": "a6488c77da856b78bd72ac32d5c480008f34cdb897524c2296289f088893b786",
    "matchKind": "nativeIdentity",
    "record": {
      "assetType": "Material",
      "class": "/Script/Engine.MaterialExpressionMaterialFunctionCall",
      "binding": null,
      "variableType": "",
      "discovered": true,
      "constructible": true,
      "compileSaveIndependentReopenVerified": false,
      "runtimeVerifiedInNamedCases": false,
      "compileEvidence": [],
      "runtimeEvidence": [],
      "scope": "Only named fixture bindings/properties/ports/topologies; not every pin, parameter or combination"
    }
  },
  {
    "verificationSource": "Catalog/UE532TextureFunctionsVerification.json",
    "originalBuild": "notExplicitInVerification",
    "originalBinarySha256": "33b90d71974ef5f9064edcc81915ff7fe1251656b8cf5fc38f8d34e386088afc",
    "originalCatalogDigest": "340e22d25dbc9c24271c5e05c89c5043d366f765a5e8b9fc4bc40bd2080389d1",
    "matchKind": "nativeIdentity",
    "record": {
      "class": "/Script/Engine.MaterialExpressionMaterialFunctionCall",
      "family": "ExistingMaterialFamilies",
      "nativeDiscovered": true,
      "nativeConstructible": true,
      "supportState": "verified",
      "compileSaveIndependentReopen": [
        "WorldParameter",
        "WorldShuffled",
        "WorldTexture"
      ],
      "visualSampleEvidence": [],
      "scope": "Representative sample evidence, not all configurations or per-node execution tracing",
      "realUi": "notRun",
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
