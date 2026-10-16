# 材质设置与输入

## 公共字段与原生语义

`NodeForgeGraph` v3 的设置位于 `asset.settings`；v2 同名设置位于根 `settings`。UI、CLI、MCP、预检、生成、Verify 和图导出共用 `NodeForgeAssetDefinitions`、codec 及 MaterialGraph。

| 字段 | 精确取值 | 条件与省略规则 |
| --- | --- | --- |
| `domain` | `Surface` | 原有必填字段 |
| `blend` | `Opaque`、`Masked`、`Translucent` | 原有规则保持 |
| `shading` | `Unlit`、`DefaultLit`、新增 `ClearCoat` | ClearCoat 设置真实 `MSM_ClearCoat`，限 Opaque/Masked；Masked 仍须给 clip |
| `lighting` | `SurfaceForwardShading` | Translucent 必填，对应 `TLM_SurfacePerPixelLighting` |
| `refractionMethod` | `None`、`IndexOfRefraction` | 新增可选字段；省略为 UE5.3 原生 `RM_None`。IOR 模式要求 Translucent/DefaultLit 与上述 lighting；设置 `RM_IndexOfRefraction` 及 UE 用于激活检查的派生 distortion 状态 |

原有 `twoSided`、clip、dither、发光设置及贴图采样语义保持。设置实际读取与导出会明确记录 `refractionMethod`，不会把 ClearCoat 标为 unsupported，也不会丢失 IOR 模式。

所有根边仍使用 `to: "Material"`，`output` 必须是规范数字字符串。新增输入如下：

| `input` | 输入类型 | UE 原生属性 | 激活与坐标要求 |
| --- | --- | --- | --- |
| `ClearCoat` | scalar | `MP_CustomData0` / ClearCoat | ClearCoat 着色模型 |
| `ClearCoatRoughness` | scalar | `MP_CustomData1` / ClearCoatRoughness | 与普通 Roughness 独立；可取清漆粗糙度贴图 R |
| `Anisotropy` | scalar | `MP_Anisotropy` | 原生允许的 DefaultLit/ClearCoat Surface 输入 |
| `Tangent` | float3 | `MP_Tangent` | **切线空间**；既有 `bTangentSpaceNormal=true`，保留 NormalDX 语义。世界空间向量不能直接接入 |
| `Refraction` | scalar IOR | `MP_Refraction` | Translucent + IOR 模式，仍须通过原生 `IsPropertyActive` |

新增 scalar 根拒绝 RGB/RGBA 输出，须显式选择单通道。Tangent 拒绝 scalar、float2、float4。新引脚在旧归档目录 revision 下拒绝；新请求须按新 Catalog 实际检查，不能仅替换旧图的摘要就视为兼容。

## 舞台参数与通道

- 103 个地板分区：清漆权重 `0.3499999940395355`；`CoatRoughness` 独立 Masks 贴图的 **R，output="1"** 接 ClearCoatRoughness。原 ORM 的 **G，output="2"** 继续接普通 Roughness；B，output="3" 继续接 Metallic。
- M04StageTop：权重 `1`，清漆粗糙度 `0.1599999964237213`；M05RedSide：权重 `1`，粗糙度 `0.2199999988079071`。
- M06Decor、M06PolishedGold、M07Housing、M07PortalBacking、M07PortalBright 的 Anisotropy 为 `0.15000000596046448`；M07Backing 为 `0.25`；M07Decor 为 `0.30000001192092896`。
- M11Glass：`refractionMethod="IndexOfRefraction"`，Refraction 参数 `1.5199999809265137`。Transmission=1/Alpha=1 是源 BSDF 语义，不能写成 UE Opacity=1。现有 Fresnel 到 Opacity 的图作为独立透明近似保留，需要实际透视与折射图验收，不能宣称与 Blender 完整 BSDF 等价。

完整逐材质数据仍以用户提供的 `native_requirements.json` 和 `recipes.json` 为权威；本修复不改这两份文件或124份基础图。

## 切线方向

UE5.3.2 的 `MaterialAttributeDefinitionMap.cpp` 对未连接 Tangent 的切线空间材质返回 `(1,0,0)`；`MaterialTemplate.ush::CalculateAnisotropyTangent` 用网格的 `TangentToWorld` 转成世界空间，并相对最终法线正交化。因此默认值沿**导入网格切线 X**。该切线由 UV0 U 正确生成/导入时，材质沿源 U；材质契约本身不能修复已丢失或反转的网格切线。

本任务无需引入世界空间 VertexTangentWS 节点来替代已经正确的默认值。需要显式 U 时，可使用既有 `MaterialExpressionConstant3Vector` 的 `[1,0,0,1]` 输出0接 Tangent；Tangent 根为 float3，Constant3Vector 的属性仍按既有四分量线性颜色编码。不要把 VertexTangentWS 的世界空间输出误当切线空间输入。

实际显式小样使用既有 `MaterialExpressionVectorParameter`，`DefaultValue=[1,0,0,1]`，RGB输出0接Tangent；保存和新PID只读Verify已通过。方向图在同一已保存材质的临时实例中把该参数改为`[0,1,0,1]`，只旋转切线、保持UV与贴图，未重存父材质。

## 发现、导出与验证

`rules.query(assetKind="Material", query="ClearCoat Refraction Anisotropy Tangent")` 可命中实际资产字段。仅字段命中时返回设置和根规则，`matchedNodeCount=0`，不猜测表达式类。Catalog 的 `materialInputs` 与 `layeredInputs` 给出精确输入和空间语义；Schema 来自同一资产字段定义。

可运行小样、每份原输入、报告和图片索引位于[材质支持说明](LayeredAcceptance.md)。生成/编译/保存、独立只读 Verify、导出回读和外观分开记录，实测状态不从本文推断。

## Windows 进程查询

原共享 `engine_processes()` 的 PowerShell/CIM 查询已在本机复现固定10秒超时；外层 UE 超时参数不影响它。新路径使用 Toolhelp32 完整枚举 UnrealEditor 候选，再用 Windows x64 只读进程信息读取实际可执行文件和 UTF-16 命令行，经 `CommandLineToArgvW` 解析准确参数，按规范 `.uproject` 路径比较。支持明确绝对位置参数和 `-project=`；相对项目参数、缺失命令行、访问失败或冲突项目参数均拒绝，不按本客户端工作目录猜测。

整个查询由一个只读 Python 工作进程执行，4秒总预算；超时只结束查询助手，不结束 UE。失败保留阶段、PID、WinError/NTSTATUS、耗时和 `NFCLI_PROCESS_QUERY`；不返回空列表冒充空闲、不重试。`assert_exclusive`、工程/引擎/插件/DLL绑定与原 pending/unknown 保护保持。CLI 与 editor transport/MCP 仍调用同一函数。

一手接口依据：[Toolhelp32](https://learn.microsoft.com/en-us/windows/win32/toolhelp/taking-a-snapshot-and-viewing-processes)、[NtQueryInformationProcess](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntqueryinformationprocess)、[RTL_USER_PROCESS_PARAMETERS](https://learn.microsoft.com/en-us/windows/win32/api/winternl/ns-winternl-rtl_user_process_parameters)。winternl 结构与具体 Windows 架构相关；平台、SDK布局或进程权限变化时须重核，信息不全仍失败关闭。


## 可运行红漆小样

以下JSON已通过本候选原CLI生成和独立Verify；已有同名资产会明确拒绝。新工程先绑定准确安装；其余四份带贴图小样在插件的`Examples/LayeredMaterials`，依赖已保存且路径/采样器匹配的贴图。不要重放pending/unknown。

```json
{
  "format": "NodeForgeGraph",
  "version": 3,
  "requestId": "ExampleRedCoatV001",
  "catalogDigest": "5531e81860370f04bd0e79bd4363d766a483eee3e4e59944bee7c2c47b43bd00",
  "target": {
    "assetPath": "/Game/NodeForgeTests/Examples/Mat/MRedCoat"
  },
  "asset": {
    "type": "Material",
    "settings": {
      "domain": "Surface",
      "blend": "Opaque",
      "shading": "ClearCoat",
      "twoSided": false
    }
  },
  "graphs": [
    {
      "id": "MaterialGraph",
      "kind": "Material",
      "nodes": [
        {
          "id": "Red",
          "class": "/Script/Engine.MaterialExpressionVectorParameter",
          "properties": {
            "ParameterName": "Red",
            "DefaultValue": [
              0.35,
              0.008,
              0.006,
              1
            ]
          }
        },
        {
          "id": "BaseRoughness",
          "class": "/Script/Engine.MaterialExpressionScalarParameter",
          "properties": {
            "ParameterName": "BaseRoughness",
            "DefaultValue": 0.34
          }
        },
        {
          "id": "Specular",
          "class": "/Script/Engine.MaterialExpressionScalarParameter",
          "properties": {
            "ParameterName": "Specular",
            "DefaultValue": 0.5
          }
        },
        {
          "id": "ClearCoatWeight",
          "class": "/Script/Engine.MaterialExpressionScalarParameter",
          "properties": {
            "ParameterName": "ClearCoatWeight",
            "DefaultValue": 1.0
          }
        },
        {
          "id": "ClearCoatRoughness",
          "class": "/Script/Engine.MaterialExpressionScalarParameter",
          "properties": {
            "ParameterName": "ClearCoatRoughness",
            "DefaultValue": 0.1599999964237213
          }
        }
      ],
      "edges": [
        {
          "from": "Red",
          "output": "0",
          "to": "Material",
          "input": "BaseColor"
        },
        {
          "from": "BaseRoughness",
          "output": "0",
          "to": "Material",
          "input": "Roughness"
        },
        {
          "from": "Specular",
          "output": "0",
          "to": "Material",
          "input": "Specular"
        },
        {
          "from": "ClearCoatWeight",
          "output": "0",
          "to": "Material",
          "input": "ClearCoat"
        },
        {
          "from": "ClearCoatRoughness",
          "output": "0",
          "to": "Material",
          "input": "ClearCoatRoughness"
        }
      ]
    }
  ]
}
```
