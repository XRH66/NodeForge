# 统一资产设置：Masked材质与蓝图变量编辑属性

本包 NodeForge-Build136-Mcp9R13 / Build136 Repair6 / 桥0.3.6；目录仍experimental。AS-DOC-20261012-01只扩已有Surface材质和七类Value变量。受测身份和完成层次查Version发行说明及本期报告；原AS有限闭环已通过AS4；正式两件资产、独立Verify、来源/预览和Details分别留证，新任务按AiPreparation取得本次新身份。

## 用户：在哪里查看

材质原生Details的Material区域可看Blend Mode=Masked、Shading Model=Unlit、Two Sided、Opacity Mask Clip Value、Dither Opacity Mask、Dithered LOD Transition和Allow Negative Emissive Color。OpacityMask根输入是遮罩输出，EmissiveColor用于Unlit；原AS只验设置、编译与保存；当前候选另验一件固定空间Masked画面，范围见发行说明，TAA/LOD仍未重测。Opaque中的clip/dither潜在值可储存，当前无遮罩作用；切换模式须重新预检连线，不自动删边。

蓝图选择变量后看Instance Editable、Category、Tooltip。实例可编辑与Private/BlueprintReadOnly不同；Speed实例可以改125而类默认保持100，Count禁止实例编辑。实例编辑不保存地图，不新增组件、Spawn、复制或SaveGame属性。查看原生Details和字段真实读取是两种证据，最终保存核验另走新PID Verify。

## AI：字段与条件

v3材质放asset.settings，v2放settings；原domain/blend/shading/twoSided/lighting位置不改。仅Surface；blend为Opaque/Translucent/Masked，shading为Unlit/DefaultLit。Translucent必须显式lighting=SurfaceForwardShading。opacityMaskClipValue有限0..1，Masked必须显式；旧输入省略沿原生0.3333。ditherOpacityMask、ditheredLodTransition、allowNegativeEmissiveColor省略false，negative=true只允许Unlit。连接活动性由原生IsPropertyActive判定。

v3变量在asset.members.variables，type={kind:"Value",name:七类原类型}；v2在settings.variables，type是原字符串。可省略attributes整组；出现必须恰好instanceEditable(bool)、category(string)、tooltip(string)。category为1..128 Unicode码点，无首尾空白/控制，各竖线段非空；tooltip为0..1024码点，中文/换行允许、NUL拒绝，空串清除用户Tooltip。false/0/空串与省略有区别，保留原生string自动MultiLine元数据。任意metadata、变量新类型和管理属性未开放。

## 完整输入与正式调用

下面两份完整v3及对应v2采用当前目录定义和示例新身份，完整字段已静态核对；提交仍须按AiPreparation检查本次权限、实际目录及身份占用。历史134/133目录支持原2/3，132仅原2；均拒绝本期新设置/attributes，不能只替换摘要绕过能力门槛。不要把已占用实证身份再生成。

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

对应完整v2：

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

对应完整v2：

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

先nodeforge_docs_index/read读Readme、AiQuickStart、FeatureMap、本页和[AiPreparation](Preparation.md)。以准确同stage/DLL绑定本次专用工程，按AiPreparation准备新身份、完整输入和私有记录；普通安装不需要导入维护者测试台账。启动/注册专用UE后connect，读operations.describe/editor.getState/draft.get，Catalog/rules核实际digest和能力。每次callId新且提交前保存，expectedDraftRevision采用当前draft.get。

draft.replace arguments={text:完整JSON}，draft.get核修订，graphs.preflight和graphs.generate arguments={}。原reports/call/status保存；waitingPersistence保留原CLI/native结果，pending/unknown先calls.get，不换request/目标补发。原AS正式保存材质/蓝图各一件；生成、编译/save与新PID持久化分开判断。

正常退出本次自有UE并核独占，再原NodeForgeControl.py execute --operation graphs.verifySaved --project 当前工程 --plugin-directory 当前stage --run-directory 本次尚不存在核验目录 --input 原正式输入 --generation-report 原生成报告 --timeout-seconds 240；完整准备/核验命令见[AiPreparation](Preparation.md)。新PID、原JSON/收据/同stage，不Import/resave；维护验收的原run由原owner在最终核验与源保护通过后finish；普通任务保留自己的原调用记录。

## 完整导出、来源和Settings预览

同保存对象每族graphs.export arguments={assetKind:"Material"或"Blueprint",source:"实际Package.Asset完整路径",requestId:"新未占用导出种子",assetPath:"新未占用且不同源目标"}；遗漏formatVersion默认2，另一个新call显式3。返回原reports选择真实reportId，report.read pointer='/exportedRequest'得到P；P.text为完整原payload，payloadSha256与sourceRef中exportReportPath/exportReportSha256/sourceAssetSha256各有职责，不拼sidecar。

draft.replace arguments={text:P.text,exportReportPath:P.sourceRef.exportReportPath,exportReportSha256:P.sourceRef.exportReportSha256}严格初载后draft.get/预检。后续完整编辑arguments={text:editedText,retainExportOrigin:true}，不得同时传来源报告对；original初载保持省略/显式、数组顺序和值，不升级旧DLL来源证明。修改材质twoSided或Speed.category后重新取revision，draft.prepareCopy/graphs.preview arguments={}，从nativeSummary.response.result读真实prepared身份/copyPreparationReport/exportOrigin，再读原报告'/graphDifference'的Settings旧新值和baseline。

prepare会写准备记录并提交新身份草稿，但不预约目标、不生成资产或收据；不宣称完全无副作用。仅准备/预览，不generate第三件；原资产SHA、报告、原输入及收据保持。[AiCopyPreview](CopyPreview.md)定义准备登记及busy/revision/unknown恢复，任何身份冲突停依赖写入。

## 开发与迁移

GraphDocument保留可选材质值和成员attributes presence；AssetDefinitions统一普通字段、真实native属性名/default/schema/字符串规则。GraphCodec2/3同消费，MaterialGraph沿同表应用/读取普通值，Blend/Shading/lighting和IsPropertyActive保持专项。BlueprintGraph使用SetBlueprintOnlyEditableFlag(!instanceEditable)、SetBlueprintVariableCategory(...,bDontRecompile=true)、指定Tooltip Set/Remove，最终一次编译，核NewVariables/FProperty/CDO。GraphDiff仍消费共同描述的Settings兼容投影，无新分组或控制层；原AS19工具，当前工具数以同包ToolContract为准，SDK2.3.0/ABI1保持。

134精确归档material6/blueprint11/max3，133保留6/10/max3，132保留6/10/max2。根Build2、RC1 Build29和旧134冻结不升级；回退需成套stage/config/ToolContract/资料，先停未知调用，不回滚用户资产。旧B/C/J/N/BT未变证据复用；原AS未PIE/Tick/视觉/打包/材质实例MCP；本候选另有有限固定Masked画面及无生成器Win64包后运行，细节见发行说明；TAA/LOD与实例MCP未重测。
