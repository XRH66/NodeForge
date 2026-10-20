# NodeForge 材质编写规则（v1.1.2 · UE 5.3.2）

本文规定 NodeForge 生成普通 Surface 材质时使用的 JSON v3 格式和可用节点，供生成式 AI 按规则编写，也供人核对。按本文写出的 JSON 可直接载入 NodeForge 面板生成材质。**本文自包含**：不需要再查插件在线目录、不需要先运行插件查询，也不需要联网。

- 适用版本：NodeForge v1.1.2，Unreal Engine 5.3.2（Windows 64 位）。
- 适用资产：普通 Surface 材质（Material）。材质实例、材质函数本体、层级材质不在本文范围内。
- 目录摘要：`catalogDigest` 固定为 `5531e81860370f04bd0e79bd4363d766a483eee3e4e59944bee7c2c47b43bd00`。

## 1. 一次输出什么

1. 只输出**一个 JSON 对象**，不要数组、不要拆成多份、不要在 JSON 里写注释。
2. 需要解释时，把解释放在 JSON 之外，用普通文字写。
3. JSON 必须能直接被 `JSON.parse` 解析：双引号、无尾逗号、无重复键。
4. 输出前逐项走一遍第 13 节的自检清单。

## 2. 根对象

根对象必须且只能有这七个字段：

| 字段 | 固定值或规则 |
| --- | --- |
| `format` | 固定 `"NodeForgeGraph"` |
| `version` | 固定 `3` |
| `requestId` | 本次请求的唯一编号，`^[A-Za-z0-9]{1,64}$`；每次都换新值，不能与之前用过的相同 |
| `catalogDigest` | 固定 `"5531e81860370f04bd0e79bd4363d766a483eee3e4e59944bee7c2c47b43bd00"` |
| `target` | `{"assetPath": "<目标资产路径>"}` |
| `asset` | `{"type": "Material", "settings": {…}}`，见第 4 节 |
| `graphs` | 恰好一个元素的数组，见第 3 节 |

不要额外添加字段。v2 的九个根字段（`assetType`、`assetPath`、`nodes`、`edges`、`settings` 等）与本文件无关，混用会被拒绝。

## 3. 目标路径、图与规模

**目标路径** `target.assetPath`：

- 必须以 `/Game/NodeForgeTests/` 开头。
- 之后最多两级子目录，每级 1–16 个 ASCII 字母或数字。
- 最后是资产名，1–64 个 ASCII 字母或数字。
- 不能有空格、下划线、扩展名、反斜线、`.`、`..` 路径段。
- 不能用已经存在的资产路径。
- 例：`/Game/NodeForgeTests/Examples/MAiPreview`（示例值，每次按需换新路径）。

**图** `graphs` 恰好一个元素：

~~~json
{"id": "MaterialGraph", "kind": "Material", "nodes": [], "edges": []}
~~~

- `id` 固定 `"MaterialGraph"`，`kind` 固定 `"Material"`。
- `nodes` 至少 1 个、最多 256 个。
- `edges` 至少 1 条、最多 1024 条，并且至少一条要连到有效的材质输出。

**文件与嵌套**：UTF-8 无 BOM；1–65,536 字节；嵌套深度不超过 32；不能出现 NUL、重复键、注释、尾逗号或未知字段。节点 `id` 使用 1–64 个 ASCII 字母或数字，`Material` 是材质输出保留名，不能当节点 id。

## 4. 材质设置 `asset.settings`

必填四项：`domain`、`blend`、`shading`、`twoSided`。

| 字段 | 取值 | 说明 |
| --- | --- | --- |
| `domain` | `Surface` | 只能 Surface |
| `blend` | `Opaque`、`Translucent`、`Masked` | 决定是否能用 Opacity / OpacityMask |
| `shading` | `Unlit`、`DefaultLit`、`ClearCoat` | 着色模型 |
| `twoSided` | `true` / `false` | 双面 |
| `lighting` | `SurfaceForwardShading` | 仅 Translucent 必填 |
| `opacityMaskClipValue` | 0–1 的数字 | 仅 Masked 必填 |
| `refractionMethod` | `None`、`IndexOfRefraction` | 省略等于 None |
| `ditherOpacityMask`、`ditheredLodTransition` | 布尔 | 可选，省略为 false |
| `allowNegativeEmissiveColor` | 布尔 | 为 true 时 shading 必须是 Unlit |

条件约束（不满足会被拒绝）：

- `blend` 为 `Translucent` 时必须显式写 `"lighting": "SurfaceForwardShading"`。
- `blend` 为 `Masked` 时必须显式写 `opacityMaskClipValue`（0–1）。
- `shading` 为 `ClearCoat` 时，`blend` 只能是 `Opaque` 或 `Masked`。
- `refractionMethod` 为 `IndexOfRefraction` 时，`blend` 必须是 `Translucent`、`shading` 必须是 `DefaultLit`，并同时写 lighting。
- `allowNegativeEmissiveColor` 为 `true` 时，`shading` 必须是 `Unlit`。

三种常见组合：

~~~json
{"domain": "Surface", "blend": "Opaque", "shading": "DefaultLit", "twoSided": false}
{"domain": "Surface", "blend": "Translucent", "shading": "DefaultLit", "twoSided": false, "lighting": "SurfaceForwardShading"}
{"domain": "Surface", "blend": "Masked", "shading": "DefaultLit", "twoSided": false, "opacityMaskClipValue": 0.3333}
~~~

## 5. 节点

每个节点是：

~~~json
{"id": "Mix", "class": "/Script/Engine.MaterialExpressionMultiply", "properties": {}}
~~~

- `id`：1–64 个 ASCII 字母或数字，同一图内唯一。
- `class`：必须与附录 B 的 class **完全一致**（区分大小写）。附录 B 之外的类一律不支持。
- `properties`：必须是对象；只写附录 B 为该类列出的属性；值按附录 A 的 schema。
- 属性省略 = 采用 UE 默认值；不要用 `null` 占位。
- `position`：可选 `[x, y]`，两个整数，绝对值不超过 1,000,000。不写也可以，插件会自行排版。
- `ComponentMask` 必须至少选择一个通道（R/G/B/A 中至少一个为 true），不能全为 false。
- 节点数组的先后顺序**不决定**图的求值或执行顺序。

## 6. 连线与默认值

每条边是四个字符串：

~~~json
{"from": "Tint", "output": "0", "to": "Mix", "input": "A"}
~~~

- `from`：源节点 id。
- `output`：源节点的输出索引，写数字字符串，例如 `"0"`。连线始终使用输出索引，**不使用显示名称**；附录 B 会同时列出各索引的显示名（例如 RGB、R、G、B、A）与通道 mask，那些信息用于核对类型和通道，不写进连线。
- `to`：目标节点 id，或材质输出保留名 `"Material"`。
- `input`：目标节点的输入名，或第 7 节的根输入名。

规则：

- 一个输入最多只能接一条边；同一输入不能既连线又写默认值。
- 不允许自连、环路、隐式类型转换、自动替换已有连线。
- 一个输出可以接到多个输入。
- 插件按原生端口与通道直连（ConnectExact），不会自动插入转换节点；但材质表达式自身支持的原生标量/向量运算（例如 RGB 乘 scalar）仍然可用，跨维度是否合法最终由 UE 编译判断。
- 所有节点都必须能通向一个有效的材质输出；孤立节点或没有出口的分支会被拒绝，不能靠删节点规避。

**默认值 `defaults`**（可选对象，键是输入名）：

- 只能给附录 B 中标注为“可省略”并且有唯一数字 `defaultProperty` 映射的输入写默认值；这类输入不连线时就读对应属性。
- 值按该 `defaultProperty` 在附录 A 中的 schema 填写，绝对值不超过 1,000,000。
- 同一输入不能同时连线和写 `defaults`；同一输入也不能同时写 `defaults` 和它映射的 `properties` 属性。例如 `Multiply` 的输入 `A` 对应属性 `ConstA`：写了 `defaults.A` 就不要再写 `ConstA`。显式写属性同时又把输入连上是允许的，不是拒绝条件；连线生效时该属性不再作为输入，不要依赖它表达数值。
- 需要给这类没有映射的输入提供值时，用真实节点连线，不能凭默认值替代；本身可省略的输入（例如 `TextureObjectParameter` 继承的采样输入）仍可保持未连接。
- 必连输入、`不可连接` 输入，以及 `MaterialFunctionCall` 都不接受 `defaults`。

## 7. 材质根输入（14 个）

`to` 写 `"Material"` 时，`input` 只能取下面 14 个名称之一。是否可用取决于材质设置中对应的原生属性是否激活；下表只列可用的配置条件，不推测额外状态。

| 根输入 | 类型 | 有效的配置条件 |
| --- | --- | --- |
| `BaseColor` | RGB | 支持的 DefaultLit / ClearCoat 光照配置；Unlit 不连接光照输入 |
| `EmissiveColor` | RGB | 自发光输出，Unlit 与光照材质都可以使用 |
| `Metallic` | scalar | 支持的 DefaultLit / ClearCoat 光照配置；Unlit 不连接 |
| `Roughness` | scalar | 支持的 DefaultLit / ClearCoat 光照配置；Unlit 不连接 |
| `Specular` | scalar | 支持的 DefaultLit / ClearCoat 光照配置；Unlit 不连接 |
| `Normal` | float3，切线空间 | 支持的 DefaultLit / ClearCoat 光照配置；不能接世界空间向量 |
| `Opacity` | scalar | 仅 `blend` 为 `Translucent` |
| `OpacityMask` | scalar | 仅 `blend` 为 `Masked`，并配合 `opacityMaskClipValue` |
| `WorldPositionOffset` | float3 | 在支持的材质配置下可用，最终按原生激活状态判断 |
| `ClearCoat` | scalar | 仅 `shading` 为 `ClearCoat` |
| `ClearCoatRoughness` | scalar | 仅 `shading` 为 `ClearCoat`，与普通 Roughness 相互独立 |
| `Anisotropy` | scalar | 需要有效的光照配置（DefaultLit 或 ClearCoat） |
| `Tangent` | float3，切线空间 | 需要有效的切线空间光照配置，不能接标量 / float2 / float4 |
| `Refraction` | scalar（IOR） | 仅 `blend` 为 `Translucent` + `shading` 为 `DefaultLit` + 写 `lighting` + `refractionMethod` 为 `IndexOfRefraction` |

补充约束：

- ClearCoat、ClearCoatRoughness、Anisotropy、Refraction 这四个较新的标量根有原生严格标量检查，必须显式选单通道输出，不能接 RGB / RGBA 整体输出。
- Metallic、Roughness、Specular、Opacity、OpacityMask 等其余标量根也建议选单通道输出；这是写法建议，不把它们写成与上面四个相同的原生拒绝条件。
- 根输入最终是否可连由 UE 原生属性激活状态（IsPropertyActive）校验；不确定时用最保守的配置，并保持其余根输入不连。
- 空间位置、法线类节点输出的是世界空间，插件不会自动做世界空间与切线空间转换；`PixelNormalWS` 不能接 Normal 或 WorldPositionOffset。

## 8. 属性值类型

正文只给要点，完整 schema 见**附录 A**（共 10 种，每种都带字段数）。

- 数值：默认在 [-1000000, 1000000] 内；标注为必须大于 0 的字段不能写 0 或负数。
- 布尔：写 `true` / `false`，不要写 `"true"`。
- 整数：只能在规定区间内取整数。
- 颜色/向量四数组：`[R, G, B, A]` 四个数字，例如 `[1, 0.5, 0.2, 1]`。
- 参数名：`^[A-Za-z0-9]{1,64}$`。同一材质里的参数节点必须使用互不相同的参数名。
- 贴图路径：见第 10 节的完整对象路径规则。
- 采样器类字段：只能取附录 A 中列出的枚举值。

## 9. 参数节点

`ScalarParameter`、`VectorParameter` 等参数节点没有输入，靠 `properties` 里的 `ParameterName` 与 `DefaultValue` 定义。

- `ScalarParameter` 只有输出 `"0"`（单通道）。
- `VectorParameter` 有五个输出：`"0"` = RGB，`"1"` = R，`"2"` = G，`"3"` = B，`"4"` = A。
- 所有参数节点都必须显式写 `ParameterName`，不能省略后依赖目录默认值；同一材质内按 FName 大小写不敏感唯一。
- 参数名不能是空名称；FName 的 `None` 及其大小写变体都视为空名称，原生拒绝。
- 需要多个参数时分别起名，不要复用同一个名字。

## 10. 贴图与材质函数

**贴图**：

- 必须是用户工程里真实存在、已保存、非 virtual 的普通 `Texture2D`。
- 所有带 `Texture` 字段的贴图节点（包括 `TextureObject`、贴图参数节点）都必须显式提供真实贴图，不能依赖目录默认贴图。
- 写完整对象路径，格式如 `/Game/Textures/TWood.TWood`（对象名.对象名）；实际路径以用户已保存的资源为准，不能凭空编造。
- 资源还必须位于当前安装允许读取的路径范围内（`/Game/` 或 `/Engine/`）。
- 当前不支持导入新贴图、虚拟纹理、运行时合成。
- `SamplerType` 可以省略，由 UE 按贴图推断；显式写时必须是附录 A 里列出的枚举值，并与真实贴图设置一致。
- `TextureObjectParameter` 继承的 `Coordinates`、`Apply View MipBias` 等输入是原生不消费的输入，`connectible` 为 false，**不要连，也不要写默认值**。

**材质函数**（v3 写法）：

~~~json
{
  "id": "Tint",
  "class": "/Script/Engine.MaterialExpressionMaterialFunctionCall",
  "properties": {},
  "binding": {"kind": "MaterialFunction", "path": "/Game/Functions/MF_Tint.MF_Tint", "assetSha256": "<64 位十六进制>"}
}
~~~

- `properties` 必须是空对象 `{}`；函数信息只放在 `binding` 里。
- `path` 必须是真实存在、已保存、无未保存修改的普通材质函数资产路径；`assetSha256` 必须是该文件真实的 SHA256。
- 该函数的输入/输出引脚名与类型，必须按用户提供的实际函数签名填写。本文件不含任何具体函数的端口。
- 信息不足时，先向用户索取函数路径、SHA256 与实际端口签名，或者改用本文件里的普通节点组合。**不要编造 SHA 或引脚名。**

## 11. 限制与禁止

- 只允许附录 B 中的 71 类节点；禁止 Custom HLSL、Material Attributes、未列出的表达式、任意对象引用。
- 一个请求只生成一个材质、一张图；不支持层级材质、创建或导出材质函数。
- 不做隐式类型转换，不自动补节点，不自动替换连线，不支持循环。
- 节点类型、着色阶段和编译规则仍由 UE 决定：JSON 检查通过不等于材质编译通过。

## 12. 完整格式示例

下面是一份完整的 JSON 格式示例（4 个节点、4 条边）。示例中的 `requestId` 与 `target.assetPath` 是占位值，每次生成都要换成未使用的目标和未使用过的请求编号。

~~~json
{
  "format": "NodeForgeGraph",
  "version": 3,
  "requestId": "NFMAT20261020A1",
  "catalogDigest": "5531e81860370f04bd0e79bd4363d766a483eee3e4e59944bee7c2c47b43bd00",
  "target": {"assetPath": "/Game/NodeForgeTests/Examples/MAiPreview"},
  "asset": {
    "type": "Material",
    "settings": {"domain": "Surface", "blend": "Opaque", "shading": "DefaultLit", "twoSided": false}
  },
  "graphs": [
    {
      "id": "MaterialGraph",
      "kind": "Material",
      "nodes": [
        {"id": "Tint", "class": "/Script/Engine.MaterialExpressionVectorParameter", "properties": {"ParameterName": "Tint", "DefaultValue": [0.8, 0.35, 0.12, 1]}},
        {"id": "Level", "class": "/Script/Engine.MaterialExpressionConstant", "properties": {"R": 0.6}},
        {"id": "Mix", "class": "/Script/Engine.MaterialExpressionMultiply", "properties": {}},
        {"id": "Rough", "class": "/Script/Engine.MaterialExpressionScalarParameter", "properties": {"ParameterName": "Roughness", "DefaultValue": 0.35}}
      ],
      "edges": [
        {"from": "Tint", "output": "0", "to": "Mix", "input": "A"},
        {"from": "Level", "output": "0", "to": "Mix", "input": "B"},
        {"from": "Mix", "output": "0", "to": "Material", "input": "BaseColor"},
        {"from": "Rough", "output": "0", "to": "Material", "input": "Roughness"}
      ]
    }
  ]
}
~~~

## 13. 自检清单

- [ ] 根对象只有七个字段，`version` 是数字 `3`，`catalogDigest` 完全一致。
- [ ] `requestId` 与 `target.assetPath` 都是本次新值，路径符合 `/Game/NodeForgeTests/` 规则。
- [ ] `graphs` 只有一个，`id`/`kind` 是 `MaterialGraph`/`Material`。
- [ ] `asset.type` 是 `"Material"`，settings 四项必填齐全，条件字段按第 4 节补齐。
- [ ] 每个节点的 `class` 都能在附录 B 精确找到，`properties` 只含附录 B 列出的字段。
- [ ] 每个属性值都符合附录 A 的对应 schema。
- [ ] `output` 全部是数字字符串；`input` 与节点/根输入名精确一致。
- [ ] 没有重复连线、没有自连、没有环路、没有隐式类型转换。
- [ ] 所有节点都通向某个有效根输入，且根输入在设置下可用。
- [ ] `defaults` 只写在可省略且有 `defaultProperty` 的输入上，未与连线或对应属性重复。
- [ ] 参数名互不重复；贴图/函数路径真实存在，没有编造 SHA。
- [ ] 输出是单个 JSON 对象，可直接解析，无注释、无尾逗号、无重复键。

## 附录 A · 属性值 schema 图例

节点 properties 中每个字段的 valueSchema 只有下面 10 种。写值前先按本表核对，不能凭字段名或数字习惯猜测。

~~~json
{
  "S1": {
    "type": "number",
    "minimum": -1000000,
    "maximum": 1000000
  },
  "S2": {
    "type": "boolean"
  },
  "S3": {
    "type": "integer",
    "minimum": 0,
    "maximum": 7
  },
  "S4": {
    "type": "array",
    "minItems": 4,
    "maxItems": 4,
    "items": {
      "type": "number",
      "minimum": -1000000,
      "maximum": 1000000
    }
  },
  "S5": {
    "type": "number",
    "exclusiveMinimum": 0,
    "maximum": 1000000
  },
  "S6": {
    "type": "string",
    "pattern": "^[A-Za-z0-9]{1,64}$"
  },
  "S7": {
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
  "S8": {
    "type": "string",
    "pattern": "^(?:/Engine/|/Game/)(?!.*\\.\\.).*\\..+$",
    "x-nativeConstraint": "actual canonical existing nonvirtual Texture2D; sampler checked natively"
  },
  "S9": {
    "type": "string",
    "enum": [
      "Absolute",
      "CameraRelative"
    ]
  },
  "S10": {
    "type": "string",
    "enum": [
      "WPT_Default",
      "WPT_ExcludeAllShaderOffsets",
      "WPT_CameraRelative",
      "WPT_CameraRelativeNoOffsets"
    ]
  }
}
~~~

| 代号 | 含义 | 字段数 |
| --- | --- | --- |
| S1 | number，范围 [-1000000, 1000000] | 41 |
| S2 | boolean | 10 |
| S3 | integer，范围 [0, 7] | 5 |
| S4 | array，4 个 number，对应 RGBA 四通道 | 4 |
| S5 | number，必须大于 0 | 4 |
| S6 | string，必须匹配正则 ^[A-Za-z0-9]{1,64}$ | 4 |
| S7 | string，只能取枚举中的 10 个值之一 | 4 |
| S8 | string，必须匹配正则 ^(?:/Engine/|/Game/)(?!.*\.\.).*\..+$ | 4 |
| S9 | string，只能取枚举中的 2 个值之一 | 1 |
| S10 | string，只能取枚举中的 4 个值之一 | 1 |

## 附录 B · 材质节点目录（71 条）

按类名排序。class 必须原样写入 nodes[].class；输出一律按索引数字字符串（例如 `"0"`）引用，不能用输出名或输出序号。nativeType 是 UE 原生类型位掩码，maskR/G/B/A 是通道位，两者都不等于“通道数”，接线时按引脚签名与通道 mask 对齐，不做隐式转换。

| 节点 | class |
| --- | --- |
| MaterialExpressionAbs | /Script/Engine.MaterialExpressionAbs |
| MaterialExpressionAdd | /Script/Engine.MaterialExpressionAdd |
| MaterialExpressionAppendVector | /Script/Engine.MaterialExpressionAppendVector |
| MaterialExpressionArccosine | /Script/Engine.MaterialExpressionArccosine |
| MaterialExpressionArccosineFast | /Script/Engine.MaterialExpressionArccosineFast |
| MaterialExpressionArcsine | /Script/Engine.MaterialExpressionArcsine |
| MaterialExpressionArcsineFast | /Script/Engine.MaterialExpressionArcsineFast |
| MaterialExpressionArctangent | /Script/Engine.MaterialExpressionArctangent |
| MaterialExpressionArctangent2 | /Script/Engine.MaterialExpressionArctangent2 |
| MaterialExpressionArctangent2Fast | /Script/Engine.MaterialExpressionArctangent2Fast |
| MaterialExpressionArctangentFast | /Script/Engine.MaterialExpressionArctangentFast |
| MaterialExpressionCameraPositionWS | /Script/Engine.MaterialExpressionCameraPositionWS |
| MaterialExpressionCameraVectorWS | /Script/Engine.MaterialExpressionCameraVectorWS |
| MaterialExpressionCeil | /Script/Engine.MaterialExpressionCeil |
| MaterialExpressionClamp | /Script/Engine.MaterialExpressionClamp |
| MaterialExpressionComponentMask | /Script/Engine.MaterialExpressionComponentMask |
| MaterialExpressionConstant | /Script/Engine.MaterialExpressionConstant |
| MaterialExpressionConstant2Vector | /Script/Engine.MaterialExpressionConstant2Vector |
| MaterialExpressionConstant3Vector | /Script/Engine.MaterialExpressionConstant3Vector |
| MaterialExpressionConstant4Vector | /Script/Engine.MaterialExpressionConstant4Vector |
| MaterialExpressionCosine | /Script/Engine.MaterialExpressionCosine |
| MaterialExpressionCrossProduct | /Script/Engine.MaterialExpressionCrossProduct |
| MaterialExpressionDesaturation | /Script/Engine.MaterialExpressionDesaturation |
| MaterialExpressionDistance | /Script/Engine.MaterialExpressionDistance |
| MaterialExpressionDivide | /Script/Engine.MaterialExpressionDivide |
| MaterialExpressionDotProduct | /Script/Engine.MaterialExpressionDotProduct |
| MaterialExpressionExponential | /Script/Engine.MaterialExpressionExponential |
| MaterialExpressionExponential2 | /Script/Engine.MaterialExpressionExponential2 |
| MaterialExpressionFloor | /Script/Engine.MaterialExpressionFloor |
| MaterialExpressionFmod | /Script/Engine.MaterialExpressionFmod |
| MaterialExpressionFrac | /Script/Engine.MaterialExpressionFrac |
| MaterialExpressionFresnel | /Script/Engine.MaterialExpressionFresnel |
| MaterialExpressionHsvToRgb | /Script/Engine.MaterialExpressionHsvToRgb |
| MaterialExpressionIf | /Script/Engine.MaterialExpressionIf |
| MaterialExpressionInverseLinearInterpolate | /Script/Engine.MaterialExpressionInverseLinearInterpolate |
| MaterialExpressionLinearInterpolate | /Script/Engine.MaterialExpressionLinearInterpolate |
| MaterialExpressionLogarithm | /Script/Engine.MaterialExpressionLogarithm |
| MaterialExpressionLogarithm10 | /Script/Engine.MaterialExpressionLogarithm10 |
| MaterialExpressionLogarithm2 | /Script/Engine.MaterialExpressionLogarithm2 |
| MaterialExpressionMaterialFunctionCall | /Script/Engine.MaterialExpressionMaterialFunctionCall |
| MaterialExpressionMax | /Script/Engine.MaterialExpressionMax |
| MaterialExpressionMin | /Script/Engine.MaterialExpressionMin |
| MaterialExpressionMultiply | /Script/Engine.MaterialExpressionMultiply |
| MaterialExpressionNormalize | /Script/Engine.MaterialExpressionNormalize |
| MaterialExpressionObjectPositionWS | /Script/Engine.MaterialExpressionObjectPositionWS |
| MaterialExpressionOneMinus | /Script/Engine.MaterialExpressionOneMinus |
| MaterialExpressionPanner | /Script/Engine.MaterialExpressionPanner |
| MaterialExpressionPixelNormalWS | /Script/Engine.MaterialExpressionPixelNormalWS |
| MaterialExpressionPower | /Script/Engine.MaterialExpressionPower |
| MaterialExpressionRgbToHsv | /Script/Engine.MaterialExpressionRgbToHsv |
| MaterialExpressionRotator | /Script/Engine.MaterialExpressionRotator |
| MaterialExpressionRound | /Script/Engine.MaterialExpressionRound |
| MaterialExpressionSaturate | /Script/Engine.MaterialExpressionSaturate |
| MaterialExpressionScalarParameter | /Script/Engine.MaterialExpressionScalarParameter |
| MaterialExpressionSign | /Script/Engine.MaterialExpressionSign |
| MaterialExpressionSine | /Script/Engine.MaterialExpressionSine |
| MaterialExpressionSmoothStep | /Script/Engine.MaterialExpressionSmoothStep |
| MaterialExpressionSquareRoot | /Script/Engine.MaterialExpressionSquareRoot |
| MaterialExpressionStep | /Script/Engine.MaterialExpressionStep |
| MaterialExpressionSubtract | /Script/Engine.MaterialExpressionSubtract |
| MaterialExpressionTangent | /Script/Engine.MaterialExpressionTangent |
| MaterialExpressionTextureCoordinate | /Script/Engine.MaterialExpressionTextureCoordinate |
| MaterialExpressionTextureObject | /Script/Engine.MaterialExpressionTextureObject |
| MaterialExpressionTextureObjectParameter | /Script/Engine.MaterialExpressionTextureObjectParameter |
| MaterialExpressionTextureSample | /Script/Engine.MaterialExpressionTextureSample |
| MaterialExpressionTextureSampleParameter2D | /Script/Engine.MaterialExpressionTextureSampleParameter2D |
| MaterialExpressionTime | /Script/Engine.MaterialExpressionTime |
| MaterialExpressionTruncate | /Script/Engine.MaterialExpressionTruncate |
| MaterialExpressionVectorParameter | /Script/Engine.MaterialExpressionVectorParameter |
| MaterialExpressionVertexNormalWS | /Script/Engine.MaterialExpressionVertexNormalWS |
| MaterialExpressionWorldPosition | /Script/Engine.MaterialExpressionWorldPosition |

#### MaterialExpressionAbs

- 类 `/Script/Engine.MaterialExpressionAbs`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionAdd

- 类 `/Script/Engine.MaterialExpressionAdd`
- 输入 `A`，nativeType 15，可省略，省略时读属性 `ConstA`（默认 0）
- 输入 `B`，nativeType 15，可省略，省略时读属性 `ConstB`（默认 1）
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `ConstA`，schema S1，原生类型 float，默认 0
- 属性 `ConstB`，schema S1，原生类型 float，默认 1

#### MaterialExpressionAppendVector

- 类 `/Script/Engine.MaterialExpressionAppendVector`
- 输入 `A`，nativeType 15，必连
- 输入 `B`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionArccosine

- 类 `/Script/Engine.MaterialExpressionArccosine`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionArccosineFast

- 类 `/Script/Engine.MaterialExpressionArccosineFast`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionArcsine

- 类 `/Script/Engine.MaterialExpressionArcsine`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionArcsineFast

- 类 `/Script/Engine.MaterialExpressionArcsineFast`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionArctangent

- 类 `/Script/Engine.MaterialExpressionArctangent`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionArctangent2

- 类 `/Script/Engine.MaterialExpressionArctangent2`
- 输入 `Y`，nativeType 15，必连
- 输入 `X`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionArctangent2Fast

- 类 `/Script/Engine.MaterialExpressionArctangent2Fast`
- 输入 `Y`，nativeType 15，必连
- 输入 `X`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionArctangentFast

- 类 `/Script/Engine.MaterialExpressionArctangentFast`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionCameraPositionWS

- 类 `/Script/Engine.MaterialExpressionCameraPositionWS`
- 输入：无
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionCameraVectorWS

- 类 `/Script/Engine.MaterialExpressionCameraVectorWS`
- 输入：无
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionCeil

- 类 `/Script/Engine.MaterialExpressionCeil`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionClamp

- 类 `/Script/Engine.MaterialExpressionClamp`
- 输入 `Input`，nativeType 15，必连
- 输入 `Min`，nativeType 15，可省略，省略时读属性 `MinDefault`（默认 0）
- 输入 `Max`，nativeType 15，可省略，省略时读属性 `MaxDefault`（默认 1）
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `MaxDefault`，schema S1，原生类型 float，默认 1
- 属性 `MinDefault`，schema S1，原生类型 float，默认 0

#### MaterialExpressionComponentMask

- 类 `/Script/Engine.MaterialExpressionComponentMask`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `A`，schema S2，原生类型 uint8，默认 false
- 属性 `B`，schema S2，原生类型 uint8，默认 false
- 属性 `G`，schema S2，原生类型 uint8，默认 true
- 属性 `R`，schema S2，原生类型 uint8，默认 true

#### MaterialExpressionConstant

- 类 `/Script/Engine.MaterialExpressionConstant`
- 输入：无
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `R`，schema S1，原生类型 float，默认 0

#### MaterialExpressionConstant2Vector

- 类 `/Script/Engine.MaterialExpressionConstant2Vector`
- 输入：无
- 输出 `0`，nativeType 2，通道 RG（maskR=1, maskG=1, maskB=0, maskA=0）
- 输出 `1`，nativeType 2，通道 R（maskR=1, maskG=0, maskB=0, maskA=0）
- 输出 `2`，nativeType 2，通道 G（maskR=0, maskG=1, maskB=0, maskA=0）
- 属性 `G`，schema S1，原生类型 float，默认 0
- 属性 `R`，schema S1，原生类型 float，默认 0

#### MaterialExpressionConstant3Vector

- 类 `/Script/Engine.MaterialExpressionConstant3Vector`
- 输入：无
- 输出 `0`，nativeType 4，通道 RGB（maskR=1, maskG=1, maskB=1, maskA=0）
- 输出 `1`，nativeType 4，通道 R（maskR=1, maskG=0, maskB=0, maskA=0）
- 输出 `2`，nativeType 4，通道 G（maskR=0, maskG=1, maskB=0, maskA=0）
- 输出 `3`，nativeType 4，通道 B（maskR=0, maskG=0, maskB=1, maskA=0）
- 属性 `Constant`，schema S4，原生类型 FLinearColor，默认 [0, 0, 0, 0]

#### MaterialExpressionConstant4Vector

- 类 `/Script/Engine.MaterialExpressionConstant4Vector`
- 输入：无
- 输出 `0`，nativeType 8，通道 RGBA（maskR=1, maskG=1, maskB=1, maskA=1）
- 输出 `1`，nativeType 8，通道 R（maskR=1, maskG=0, maskB=0, maskA=0）
- 输出 `2`，nativeType 8，通道 G（maskR=0, maskG=1, maskB=0, maskA=0）
- 输出 `3`，nativeType 8，通道 B（maskR=0, maskG=0, maskB=1, maskA=0）
- 输出 `4`，nativeType 8，通道 A（maskR=0, maskG=0, maskB=0, maskA=1）
- 属性 `Constant`，schema S4，原生类型 FLinearColor，默认 [0, 0, 0, 0]

#### MaterialExpressionCosine

- 类 `/Script/Engine.MaterialExpressionCosine`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `Period`，schema S5，原生类型 float，默认 1

#### MaterialExpressionCrossProduct

- 类 `/Script/Engine.MaterialExpressionCrossProduct`
- 输入 `A`，nativeType 15，必连
- 输入 `B`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionDesaturation

- 类 `/Script/Engine.MaterialExpressionDesaturation`
- 输入 `Input`，nativeType 15，必连
- 输入 `Fraction`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `LuminanceFactors`，schema S4，原生类型 FLinearColor，默认 [0.30000001192092896, 0.5899999737739563, 0.10999999940395355, 0]

#### MaterialExpressionDistance

- 类 `/Script/Engine.MaterialExpressionDistance`
- 输入 `A`，nativeType 15，必连
- 输入 `B`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionDivide

- 类 `/Script/Engine.MaterialExpressionDivide`
- 输入 `A`，nativeType 15，可省略，省略时读属性 `ConstA`（默认 1）
- 输入 `B`，nativeType 15，可省略，省略时读属性 `ConstB`（默认 2）
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `ConstA`，schema S1，原生类型 float，默认 1
- 属性 `ConstB`，schema S1，原生类型 float，默认 2

#### MaterialExpressionDotProduct

- 类 `/Script/Engine.MaterialExpressionDotProduct`
- 输入 `A`，nativeType 15，必连
- 输入 `B`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionExponential

- 类 `/Script/Engine.MaterialExpressionExponential`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionExponential2

- 类 `/Script/Engine.MaterialExpressionExponential2`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionFloor

- 类 `/Script/Engine.MaterialExpressionFloor`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionFmod

- 类 `/Script/Engine.MaterialExpressionFmod`
- 输入 `A`，nativeType 15，必连
- 输入 `B`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionFrac

- 类 `/Script/Engine.MaterialExpressionFrac`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionFresnel

- 类 `/Script/Engine.MaterialExpressionFresnel`
- 输入 `ExponentIn`，nativeType 15，可省略，省略时读属性 `Exponent`（默认 5）
- 输入 `BaseReflectFractionIn`，nativeType 15，可省略，省略时读属性 `BaseReflectFraction`（默认 0.03999999910593033）
- 输入 `Normal`，nativeType 15，可省略
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `BaseReflectFraction`，schema S1，原生类型 float，默认 0.03999999910593033
- 属性 `Exponent`，schema S1，原生类型 float，默认 5

#### MaterialExpressionHsvToRgb

- 类 `/Script/Engine.MaterialExpressionHsvToRgb`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionIf

- 类 `/Script/Engine.MaterialExpressionIf`
- 输入 `A`，nativeType 15，必连
- 输入 `B`，nativeType 15，可省略，省略时读属性 `ConstB`（默认 0）
- 输入 `AGreaterThanB`，nativeType 1024，必连
- 输入 `AEqualsB`，nativeType 1024，可省略
- 输入 `ALessThanB`，nativeType 1024，必连
- 输出 `0`，nativeType 1024，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `ConstB`，schema S1，原生类型 float，默认 0
- 属性 `EqualsThreshold`，schema S1，原生类型 float，默认 9.999999747378752e-06

#### MaterialExpressionInverseLinearInterpolate

- 类 `/Script/Engine.MaterialExpressionInverseLinearInterpolate`
- 输入 `A`，nativeType 15，可省略，省略时读属性 `ConstA`（默认 0）
- 输入 `B`，nativeType 15，可省略，省略时读属性 `ConstB`（默认 1）
- 输入 `Value`，nativeType 15，可省略，省略时读属性 `ConstValue`（默认 0）
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `bClampResult`，schema S2，原生类型 bool，默认 false
- 属性 `ConstA`，schema S1，原生类型 float，默认 0
- 属性 `ConstB`，schema S1，原生类型 float，默认 1
- 属性 `ConstValue`，schema S1，原生类型 float，默认 0

#### MaterialExpressionLinearInterpolate

- 类 `/Script/Engine.MaterialExpressionLinearInterpolate`
- 输入 `A`，nativeType 15，可省略，省略时读属性 `ConstA`（默认 0）
- 输入 `B`，nativeType 15，可省略，省略时读属性 `ConstB`（默认 1）
- 输入 `Alpha`，nativeType 15，可省略，省略时读属性 `ConstAlpha`（默认 0.5）
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `ConstA`，schema S1，原生类型 float，默认 0
- 属性 `ConstAlpha`，schema S1，原生类型 float，默认 0.5
- 属性 `ConstB`，schema S1，原生类型 float，默认 1

#### MaterialExpressionLogarithm

- 类 `/Script/Engine.MaterialExpressionLogarithm`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionLogarithm10

- 类 `/Script/Engine.MaterialExpressionLogarithm10`
- 输入 `X`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionLogarithm2

- 类 `/Script/Engine.MaterialExpressionLogarithm2`
- 输入 `X`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionMaterialFunctionCall

- 类 `/Script/Engine.MaterialExpressionMaterialFunctionCall`
- 输入：无
- 输出：无
- properties 固定写 `{}`；函数由 binding 指定，见正文“材质函数”一节

#### MaterialExpressionMax

- 类 `/Script/Engine.MaterialExpressionMax`
- 输入 `A`，nativeType 15，可省略，省略时读属性 `ConstA`（默认 0）
- 输入 `B`，nativeType 15，可省略，省略时读属性 `ConstB`（默认 1）
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `ConstA`，schema S1，原生类型 float，默认 0
- 属性 `ConstB`，schema S1，原生类型 float，默认 1

#### MaterialExpressionMin

- 类 `/Script/Engine.MaterialExpressionMin`
- 输入 `A`，nativeType 15，可省略，省略时读属性 `ConstA`（默认 0）
- 输入 `B`，nativeType 15，可省略，省略时读属性 `ConstB`（默认 1）
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `ConstA`，schema S1，原生类型 float，默认 0
- 属性 `ConstB`，schema S1，原生类型 float，默认 1

#### MaterialExpressionMultiply

- 类 `/Script/Engine.MaterialExpressionMultiply`
- 输入 `A`，nativeType 15，可省略，省略时读属性 `ConstA`（默认 0）
- 输入 `B`，nativeType 15，可省略，省略时读属性 `ConstB`（默认 1）
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `ConstA`，schema S1，原生类型 float，默认 0
- 属性 `ConstB`，schema S1，原生类型 float，默认 1

#### MaterialExpressionNormalize

- 类 `/Script/Engine.MaterialExpressionNormalize`
- 输入 `VectorInput`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionObjectPositionWS

- 类 `/Script/Engine.MaterialExpressionObjectPositionWS`
- 输入：无
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `OriginType`，schema S9，原生类型 EPositionOrigin，默认 "Absolute"

#### MaterialExpressionOneMinus

- 类 `/Script/Engine.MaterialExpressionOneMinus`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionPanner

- 类 `/Script/Engine.MaterialExpressionPanner`
- 输入 `Coordinate`，nativeType 15，可省略，省略时读属性 `ConstCoordinate`（默认 0）
- 输入 `Time`，nativeType 15，可省略
- 输入 `Speed`，nativeType 15，可省略
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `bFractionalPart`，schema S2，原生类型 bool，默认 false
- 属性 `ConstCoordinate`，schema S3，原生类型 uint32，默认 0
- 属性 `SpeedX`，schema S1，原生类型 float，默认 0
- 属性 `SpeedY`，schema S1，原生类型 float，默认 0

#### MaterialExpressionPixelNormalWS

- 类 `/Script/Engine.MaterialExpressionPixelNormalWS`
- 输入：无
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionPower

- 类 `/Script/Engine.MaterialExpressionPower`
- 输入 `Base`，nativeType 15，必连
- 输入 `Exponent`，nativeType 15，可省略，省略时读属性 `ConstExponent`（默认 2）
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `ConstExponent`，schema S1，原生类型 float，默认 2

#### MaterialExpressionRgbToHsv

- 类 `/Script/Engine.MaterialExpressionRgbToHsv`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionRotator

- 类 `/Script/Engine.MaterialExpressionRotator`
- 输入 `Coordinate`，nativeType 15，可省略，省略时读属性 `ConstCoordinate`（默认 0）
- 输入 `Time`，nativeType 15，可省略
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `CenterX`，schema S1，原生类型 float，默认 0.5
- 属性 `CenterY`，schema S1，原生类型 float，默认 0.5
- 属性 `ConstCoordinate`，schema S3，原生类型 uint32，默认 0
- 属性 `Speed`，schema S1，原生类型 float，默认 0.25

#### MaterialExpressionRound

- 类 `/Script/Engine.MaterialExpressionRound`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionSaturate

- 类 `/Script/Engine.MaterialExpressionSaturate`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionScalarParameter

- 类 `/Script/Engine.MaterialExpressionScalarParameter`
- 输入：无
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `DefaultValue`，schema S1，原生类型 float，默认 0
- 属性 `ParameterName`，schema S6，原生类型 FName，默认 "Param"

#### MaterialExpressionSign

- 类 `/Script/Engine.MaterialExpressionSign`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionSine

- 类 `/Script/Engine.MaterialExpressionSine`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `Period`，schema S5，原生类型 float，默认 1

#### MaterialExpressionSmoothStep

- 类 `/Script/Engine.MaterialExpressionSmoothStep`
- 输入 `Min`，nativeType 15，可省略，省略时读属性 `ConstMin`（默认 0）
- 输入 `Max`，nativeType 15，可省略，省略时读属性 `ConstMax`（默认 1）
- 输入 `Value`，nativeType 15，可省略，省略时读属性 `ConstValue`（默认 0）
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `ConstMax`，schema S1，原生类型 float，默认 1
- 属性 `ConstMin`，schema S1，原生类型 float，默认 0
- 属性 `ConstValue`，schema S1，原生类型 float，默认 0

#### MaterialExpressionSquareRoot

- 类 `/Script/Engine.MaterialExpressionSquareRoot`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionStep

- 类 `/Script/Engine.MaterialExpressionStep`
- 输入 `Y`，nativeType 15，可省略，省略时读属性 `ConstY`（默认 0）
- 输入 `X`，nativeType 15，可省略，省略时读属性 `ConstX`（默认 1）
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `ConstX`，schema S1，原生类型 float，默认 1
- 属性 `ConstY`，schema S1，原生类型 float，默认 0

#### MaterialExpressionSubtract

- 类 `/Script/Engine.MaterialExpressionSubtract`
- 输入 `A`，nativeType 15，可省略，省略时读属性 `ConstA`（默认 1）
- 输入 `B`，nativeType 15，可省略，省略时读属性 `ConstB`（默认 1）
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `ConstA`，schema S1，原生类型 float，默认 1
- 属性 `ConstB`，schema S1，原生类型 float，默认 1

#### MaterialExpressionTangent

- 类 `/Script/Engine.MaterialExpressionTangent`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `Period`，schema S5，原生类型 float，默认 1

#### MaterialExpressionTextureCoordinate

- 类 `/Script/Engine.MaterialExpressionTextureCoordinate`
- 输入：无
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `CoordinateIndex`，schema S3，原生类型 int32，默认 0
- 属性 `UnMirrorU`，schema S2，原生类型 uint8，默认 false
- 属性 `UnMirrorV`，schema S2，原生类型 uint8，默认 false
- 属性 `UTiling`，schema S1，原生类型 float，默认 1
- 属性 `VTiling`，schema S1，原生类型 float，默认 1

#### MaterialExpressionTextureObject

- 类 `/Script/Engine.MaterialExpressionTextureObject`
- 输入：无
- 输出 `0`，nativeType 16，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `SamplerType`，schema S7，原生类型 TEnumAsByte<EMaterialSamplerType>，默认 "SAMPLERTYPE_Color"
- 属性 `Texture`，schema S8，原生类型 TObjectPtr<UTexture>，默认 "/Engine/EngineResources/DefaultTexture.DefaultTexture"

#### MaterialExpressionTextureObjectParameter

- 类 `/Script/Engine.MaterialExpressionTextureObjectParameter`
- 输入 `Coordinates`，nativeType 15，可省略，不可连接（inherited native sample input is not consumed by TextureObjectParameter::Compile; connections rejected）
- 输入 `Apply View MipBias`，nativeType 512，可省略，不可连接（inherited native sample input is not consumed by TextureObjectParameter::Compile; connections rejected）
- 输出 `0`，nativeType 12784，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `ParameterName`，schema S6，原生类型 FName，默认 "Param"
- 属性 `SamplerType`，schema S7，原生类型 TEnumAsByte<EMaterialSamplerType>，默认 "SAMPLERTYPE_Color"
- 属性 `Texture`，schema S8，原生类型 TObjectPtr<UTexture>，默认 "/Engine/EngineResources/DefaultTexture.DefaultTexture"

#### MaterialExpressionTextureSample

- 类 `/Script/Engine.MaterialExpressionTextureSample`
- 输入 `Coordinates`，nativeType 15，可省略，省略时读属性 `ConstCoordinate`（默认 0）
- 输入 `TextureObject`，nativeType 12784，可省略
- 输入 `Apply View MipBias`，nativeType 512，可省略
- 输出 `0`，nativeType 4，通道 RGB（maskR=1, maskG=1, maskB=1, maskA=0）
- 输出 `1`，nativeType 15，通道 R（maskR=1, maskG=0, maskB=0, maskA=0）
- 输出 `2`，nativeType 15，通道 G（maskR=0, maskG=1, maskB=0, maskA=0）
- 输出 `3`，nativeType 15，通道 B（maskR=0, maskG=0, maskB=1, maskA=0）
- 输出 `4`，nativeType 15，通道 A（maskR=0, maskG=0, maskB=0, maskA=1）
- 输出 `5`，nativeType 8，通道 RGBA（maskR=1, maskG=1, maskB=1, maskA=1）
- 属性 `ConstCoordinate`，schema S3，原生类型 uint8，默认 0
- 属性 `SamplerType`，schema S7，原生类型 TEnumAsByte<EMaterialSamplerType>，默认 "SAMPLERTYPE_Color"
- 属性 `Texture`，schema S8，原生类型 TObjectPtr<UTexture>，默认 null

#### MaterialExpressionTextureSampleParameter2D

- 类 `/Script/Engine.MaterialExpressionTextureSampleParameter2D`
- 输入 `Coordinates`，nativeType 15，可省略，省略时读属性 `ConstCoordinate`（默认 0）
- 输入 `Apply View MipBias`，nativeType 512，可省略
- 输出 `0`，nativeType 4，通道 RGB（maskR=1, maskG=1, maskB=1, maskA=0）
- 输出 `1`，nativeType 15，通道 R（maskR=1, maskG=0, maskB=0, maskA=0）
- 输出 `2`，nativeType 15，通道 G（maskR=0, maskG=1, maskB=0, maskA=0）
- 输出 `3`，nativeType 15，通道 B（maskR=0, maskG=0, maskB=1, maskA=0）
- 输出 `4`，nativeType 15，通道 A（maskR=0, maskG=0, maskB=0, maskA=1）
- 输出 `5`，nativeType 8，通道 RGBA（maskR=1, maskG=1, maskB=1, maskA=1）
- 属性 `ConstCoordinate`，schema S3，原生类型 uint8，默认 0
- 属性 `ParameterName`，schema S6，原生类型 FName，默认 "Param"
- 属性 `SamplerType`，schema S7，原生类型 TEnumAsByte<EMaterialSamplerType>，默认 "SAMPLERTYPE_Color"
- 属性 `Texture`，schema S8，原生类型 TObjectPtr<UTexture>，默认 "/Engine/EngineResources/DefaultTexture.DefaultTexture"

#### MaterialExpressionTime

- 类 `/Script/Engine.MaterialExpressionTime`
- 输入：无
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性 `bIgnorePause`，schema S2，原生类型 uint8，默认 false
- 属性 `bOverride_Period`，schema S2，原生类型 uint8，默认 false
- 属性 `Period`，schema S5，原生类型 float，默认 0

#### MaterialExpressionTruncate

- 类 `/Script/Engine.MaterialExpressionTruncate`
- 输入 `Input`，nativeType 15，必连
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionVectorParameter

- 类 `/Script/Engine.MaterialExpressionVectorParameter`
- 输入：无
- 输出 `0`，nativeType 4，通道 RGB（maskR=1, maskG=1, maskB=1, maskA=0）
- 输出 `1`，nativeType 15，通道 R（maskR=1, maskG=0, maskB=0, maskA=0）
- 输出 `2`，nativeType 15，通道 G（maskR=0, maskG=1, maskB=0, maskA=0）
- 输出 `3`，nativeType 15，通道 B（maskR=0, maskG=0, maskB=1, maskA=0）
- 输出 `4`，nativeType 15，通道 A（maskR=0, maskG=0, maskB=0, maskA=1）
- 属性 `DefaultValue`，schema S4，原生类型 FLinearColor，默认 [0, 0, 0, 0]
- 属性 `ParameterName`，schema S6，原生类型 FName，默认 "Param"

#### MaterialExpressionVertexNormalWS

- 类 `/Script/Engine.MaterialExpressionVertexNormalWS`
- 输入：无
- 输出 `0`，nativeType 15，通道 无（maskR=0, maskG=0, maskB=0, maskA=0）
- 属性：无，properties 写 `{}`

#### MaterialExpressionWorldPosition

- 类 `/Script/Engine.MaterialExpressionWorldPosition`
- 输入：无
- 输出 `0`，nativeType 4，通道 RGB（maskR=1, maskG=1, maskB=1, maskA=0）
- 输出 `1`，nativeType 2，通道 RG（maskR=1, maskG=1, maskB=0, maskA=0）
- 输出 `2`，nativeType 15，通道 B（maskR=0, maskG=0, maskB=1, maskA=0）
- 属性 `WorldPositionShaderOffset`，schema S10，原生类型 TEnumAsByte<EWorldPositionIncludedOffsets>，默认 "WPT_Default"

