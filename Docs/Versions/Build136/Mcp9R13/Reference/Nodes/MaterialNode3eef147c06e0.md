# 三通道常量

用途：三通道常量。

前提：按本页原签名写精确输入/输出与允许属性，连接输入不可同时设default；目录发现不代表任意配置实测。所有节点须到有效Material输出；采样/函数需已存在准入资源及实际签名/SHA。

限制：只支持原目录明确字段/类型/默认；隐藏或不可写引脚不得填写。运行/数值范围/画面按原具体组合证据，不能推导全验。

精确身份（绑定本包；不能凭序号跨版匹配）：

```json
{
  "assetKind": "Material",
  "class": "/Script/Engine.MaterialExpressionConstant3Vector"
}
```

当前原目录属性/允许默认/引脚与类型：

```json
{
  "class": "/Script/Engine.MaterialExpressionConstant3Vector",
  "discovered": true,
  "constructible": true,
  "family": "ExistingMaterialFamilies",
  "minimumMaterialRevision": 0,
  "supportState": "experimental",
  "verified": "notEstablishedByCatalog",
  "inputs": [],
  "outputs": [
    {
      "index": "0",
      "name": "None",
      "nativeType": 4,
      "mask": 1,
      "maskR": 1,
      "maskG": 1,
      "maskB": 1,
      "maskA": 0
    },
    {
      "index": "1",
      "name": "None",
      "nativeType": 4,
      "mask": 1,
      "maskR": 1,
      "maskG": 0,
      "maskB": 0,
      "maskA": 0
    },
    {
      "index": "2",
      "name": "None",
      "nativeType": 4,
      "mask": 1,
      "maskR": 0,
      "maskG": 1,
      "maskB": 0,
      "maskA": 0
    },
    {
      "index": "3",
      "name": "None",
      "nativeType": 4,
      "mask": 1,
      "maskR": 0,
      "maskG": 0,
      "maskB": 1,
      "maskA": 0
    }
  ],
  "properties": {
    "Constant": {
      "nativeType": "FLinearColor",
      "default": [
        0,
        0,
        0,
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
  "class": "/Script/Engine.MaterialExpressionConstant3Vector",
  "properties": {
    "Constant": [
      0,
      0,
      0,
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
    "verificationSource": "Catalog/UE532GraphVerification.json",
    "originalBuild": "notExplicitInVerification",
    "originalBinarySha256": "517d7694c57b01ace5e7a89c9e1541a5e1b13cb0da06f29c97875ff7f5f514e2",
    "originalCatalogDigest": "0d786af7d63e1ea209691493e9b336f50e61eaeb45ef34eeec12ff01b22a884f",
    "matchKind": "nativeIdentity",
    "record": {
      "class": "/Script/Engine.MaterialExpressionConstant3Vector",
      "discovered": true,
      "constructible": true,
      "verification": "verifiedInListedGraphs",
      "checks": {
        "readback": "passedInListedGraphs",
        "compile": "passedInListedGraphs",
        "save": "passedInListedGraphs",
        "independentReopen": "passedInListedGraphs"
      },
      "caseNames": [
        "DynamicChanged",
        "DynamicPulse",
        "Interpolation",
        "MaterialDefaults"
      ],
      "uses": [
        {
          "case": "MaterialDefaults",
          "logicalId": "Color",
          "requestedProperties": [
            "Constant"
          ],
          "requestedDefaults": []
        },
        {
          "case": "Interpolation",
          "logicalId": "Blue",
          "requestedProperties": [
            "Constant"
          ],
          "requestedDefaults": []
        },
        {
          "case": "Interpolation",
          "logicalId": "Red",
          "requestedProperties": [
            "Constant"
          ],
          "requestedDefaults": []
        },
        {
          "case": "DynamicPulse",
          "logicalId": "GlowColor",
          "requestedProperties": [
            "Constant"
          ],
          "requestedDefaults": []
        },
        {
          "case": "DynamicChanged",
          "logicalId": "GlowColor",
          "requestedProperties": [
            "Constant"
          ],
          "requestedDefaults": []
        }
      ],
      "runtime": "notAssessedHere",
      "visual": "notAssessedHere",
      "assetType": "Material"
    }
  },
  {
    "verificationSource": "Catalog/UE532BlueprintMotionVerification.json",
    "originalBuild": "notExplicitInVerification",
    "originalBinarySha256": "b5475c6feb8d6ac7f7011c29b65636aee7711f2a9919d80c5edeb7e5e8e8f7fe",
    "originalCatalogDigest": "a6488c77da856b78bd72ac32d5c480008f34cdb897524c2296289f088893b786",
    "matchKind": "nativeIdentity",
    "record": {
      "assetType": "Material",
      "class": "/Script/Engine.MaterialExpressionConstant3Vector",
      "binding": null,
      "variableType": "",
      "discovered": true,
      "constructible": true,
      "compileSaveIndependentReopenVerified": true,
      "runtimeVerifiedInNamedCases": false,
      "compileEvidence": [
        {
          "signature": [
            "Material",
            "/Script/Engine.MaterialExpressionConstant3Vector",
            "",
            "",
            ""
          ],
          "case": "MaterialDefaults",
          "pinsConfiguredOrConnected": [
            "0"
          ],
          "evidence": "internalEvidenceRetained"
        },
        {
          "signature": [
            "Material",
            "/Script/Engine.MaterialExpressionConstant3Vector",
            "",
            "",
            ""
          ],
          "case": "Interpolation",
          "pinsConfiguredOrConnected": [
            "0"
          ],
          "evidence": "internalEvidenceRetained"
        },
        {
          "signature": [
            "Material",
            "/Script/Engine.MaterialExpressionConstant3Vector",
            "",
            "",
            ""
          ],
          "case": "Interpolation",
          "pinsConfiguredOrConnected": [
            "0"
          ],
          "evidence": "internalEvidenceRetained"
        },
        {
          "signature": [
            "Material",
            "/Script/Engine.MaterialExpressionConstant3Vector",
            "",
            "",
            ""
          ],
          "case": "DynamicPulse",
          "pinsConfiguredOrConnected": [
            "0"
          ],
          "evidence": "internalEvidenceRetained"
        },
        {
          "signature": [
            "Material",
            "/Script/Engine.MaterialExpressionConstant3Vector",
            "",
            "",
            ""
          ],
          "case": "DynamicChanged",
          "pinsConfiguredOrConnected": [
            "0"
          ],
          "evidence": "internalEvidenceRetained"
        }
      ],
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
      "class": "/Script/Engine.MaterialExpressionConstant3Vector",
      "family": "ExistingMaterialFamilies",
      "nativeDiscovered": true,
      "nativeConstructible": true,
      "supportState": "verified",
      "compileSaveIndependentReopen": [
        "StepBands",
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
  },
  {
    "level": "representativeGraphGenerationExportAndIndependentVerify",
    "build": "Build132",
    "logicalId": "Color",
    "configuration": {
      "class": "/Script/Engine.MaterialExpressionConstant3Vector",
      "properties": {
        "Constant": [
          0.25,
          0.5,
          1,
          1
        ]
      }
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
