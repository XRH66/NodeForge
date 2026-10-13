# NodeForgeGraph JSON 协议

本协议适用于 NodeForge v0.1.0、Windows 64 位和官方 UE 5.3.2。`NodeForgeGraph` 支持 v2 和 v3 两种 JSON 格式，使用相同的材质、蓝图和实例生成逻辑。新请求建议使用 v3；格式版本不等于产品版本。

完整机器定义见 [GraphRequestSchema.json](../Docs/Versions/v0.1.0/GraphRequestSchema.json)，操作示例见[JSON 编写说明](../Docs/Versions/v0.1.0/Reference/GraphAuthoring.md)和[完整 v3 示例](../Docs/Versions/v0.1.0/Ai/JsonArchitectureExamples.md)。

## 编码、编号和目标路径

一个文件描述一个新资产。输入需为无 BOM 的 UTF-8，长度 1–65,536 字节，嵌套深度最多 32。重复键、NUL、注释、尾逗号、未知字段和非有限数值均拒绝。JSON 数字和布尔值不能用字符串代替。

`requestId` 使用 1–64 个 ASCII 字母或数字，在工程中必须未使用。目标位于 `/Game/NodeForgeTests/`，可以增加最多两级子目录；每段使用 1–16 个 ASCII 字母或数字，资产名使用 1–64 个。禁止空格、下划线、点段、扩展名、对象后缀和反斜线。磁盘路径还需满足 UE 包名、实际 Content 目录、路径长度和非重解析点检查。

已有磁盘、内存或资源注册表目标，以及已使用的请求编号，都会被拒绝。不会覆盖或自动加后缀。需要修改现有图时，应导出并准备新副本，再明确执行生成。

`catalogDigest` 必须来自正在使用的插件节点目录。它是目录内容摘要，不是整个文件的 SHA256。不能通过替换摘要让某个输入获得目录未支持的能力。

## v3 根对象

根对象恰好包含 `format`、`version`、`requestId`、`catalogDigest`、`target`、`asset`、`graphs` 七个字段。`format` 为 `NodeForgeGraph`，`version` 为 `3`，`target` 恰为 `{ "assetPath": "..." }`。

| 资产 | `asset` 结构 | `graphs` |
| --- | --- | --- |
| 材质 | `type=Material`、`settings` | 一个 `id=MaterialGraph`、`kind=Material` 的图 |
| Actor 蓝图 | `type=Blueprint`、`parent:{class}`、`members:{variables}` | 一个 `id=EventGraph`、`kind=EventGraph` 的图 |
| 材质实例 | `type=MaterialInstance`、`parent:{path,assetSha256}`、`parameters:{scalar,vector,texture}` | 空数组 |

材质图包含 1–256 个节点、1–1,024 条边。蓝图包含 1–128 个节点、0–256 条边和 0–64 个变量。材质实例的三类覆盖参数合计最多 128 项。

## v2 根对象

v2 恰好使用 `format`、`version`、`requestId`、`assetType`、`assetPath`、`catalogDigest`、`nodes`、`edges`、`settings` 九个字段，`version=2`。资产路径、设置和节点放在根对象中；函数、事件与变量绑定写入节点的 `properties`。具体请求见[v2 完整示例](../Docs/Versions/v0.1.0/Reference/Examples.md)。

导出时可选 `formatVersion=2|3`，默认是 2。准备副本、差异预览和重排会保留输入格式。输入文件 SHA、导出内容 SHA 和报告 SHA 分别计算，不因解析成同一个图结构而互相替代。

## 节点与边

节点必需 `id`、`class`、`properties`，可选 `defaults`、`position`；v3 还允许需要绑定的节点使用 `binding`。

- `id` 是本图唯一的 1–64 位 ASCII 字母数字编号。材质保留 `Material` 作为资产输出，不能用作表达式编号。
- `class` 使用目录中精确的完整原生类路径；不接受显示名、短名或大小写别名。
- `properties` 始终是对象，只能包含该节点允许的属性。
- v3 的 Event/Function 使用 `kind/owner/member` 绑定，Member 使用 `kind/name`，MaterialFunction 使用 `kind/path/assetSha256`。这些节点的 `properties` 为空；不需要绑定的节点不能随意添加 `binding`。
- `defaults` 只指定未连接、可写、可见输入的默认值。同一个输入不能既连线又设置默认值；材质也不能通过 `properties` 和 `defaults` 重复设置同一原生属性。
- `position` 为两个整数。材质坐标绝对值最多 1,000,000，蓝图最多 100,000。省略时由自动布局安排；显式位置保留，重叠锚点会给出警告。

每条边恰有四个字符串字段：

```json
{"from":"SourceId","output":"0","to":"TargetId","input":"A"}
```

节点和引脚必须存在、方向正确，一个目标输入最多一条连接。拒绝自连、环路、自动类型转换和替换已有连接；执行输出不能扇出。节点数组顺序不决定执行顺序，Switch 等配置数组中的顺序则有实际含义。

材质输出使用规范数字字符串，例如 `"0"`；连接最终输出时 `to="Material"`。蓝图使用精确引脚名，例如 `then`、`ReturnValue`。两种规则不能混用。

## 材质

支持 Surface、Opaque/Translucent/Masked、Unlit/DefaultLit。`twoSided` 必需；Translucent 必需 `lighting=SurfaceForwardShading`，Masked 必需 `opacityMaskClipValue`（0–1）。`ditherOpacityMask`、`ditheredLodTransition`、`allowNegativeEmissiveColor` 为可选布尔值，默认 false；负自发光仅允许 Unlit。未使用的遮罩设置可以保存，但连接只能使用 UE 当前启用的材质输出。

所有表达式必须连到有效输出；孤立节点或无输出分支拒绝。属性值遵守节点页的类型、范围和原生枚举拼写。省略使用原生默认，显式设置 Time 的 Period 必须大于零。参数名不能重复。`Desaturation` 当前要求显式连接 Input 和 Fraction。

贴图限已保存的普通非虚拟 Texture2D，路径须属于安装配置允许读取的目录，采样器类型必须匹配。TextureObjectParameter 不消费其继承的 UV/Mip 输入，连接这些输入会被拒绝。

材质函数须先通过 `resources.inspect` 取得实际路径、直接文件 SHA、输入输出和 GUID。函数未绑定时的空引脚列表不能用于构图。不支持函数调用默认值、函数实例、材质层、任意对象导入或嵌套依赖的递归 SHA 保护。

世界空间方向不会自动转换到切线空间。PixelNormalWS 不能接 Normal 或顶点位移输出；其他着色阶段限制由实际编译检查。当前不支持 Custom HLSL、Material Attributes 或未列入目录的表达式。详情见[材质参考](../Docs/Versions/v0.1.0/Reference/Material.md)。

## Actor 蓝图

父类固定 `/Script/Engine.Actor`，一个 EventGraph。BeginPlay 和 ReceiveTick 每个最多一个，可以共存。仅使用目录中列出的函数；UE 创建的 SceneRoot 和空 ConstructionScript 保留。

变量使用 `name/type/default`，v3 的 `type` 为 `{kind:"Value",name:"类型"}`。支持 bool、int32、float（real/double）、string、name、vector 和 rotator。变量名以 ASCII 字母开头，仅含字母数字，最长 64 字符，不能大小写折叠重复或与继承成员冲突。

字符串最长 4,096 字符；name 非空且短于 NAME_SIZE，空名称规范为 `None`。vector 为三个有限数字；rotator 恰含 `pitch/yaw/roll`，单位为度。实数、向量和旋转各分量绝对值最多 1e12。Rotator 默认值按各分量比较，不能把减去 360 度当作原默认值。

变量可选 `attributes`；出现时必须同时提供 `instanceEditable/category/tooltip`。category 为 1–128 个 Unicode 码点，无首尾空白或控制字符，`|` 分段不能为空；tooltip 最长 1,024 码点，不含 NUL，空串清除提示文字。

变量节点的 `Value` 是目录示例名，实际引脚名来自声明变量。对象上下文、隐藏引脚和不可写默认值不能自行配置。自身 Actor 使用隐式 self；SceneComponent 的 self 只能直接来自同图的 `K2_GetRootComponent.ReturnValue`。

Sequence 的 `outputCount` 为 2–8。SwitchInteger 恰含 `startIndex/caseCount/hasDefault`，caseCount 为 1–8 且 `startIndex+caseCount<=2147483647`。SwitchName 恰含 `cases/hasDefault`，cases 为 1–8 个字母起头、仅字母数字且最长 64 的名称，按 FName 去重并排除保留名。隐藏比较函数上下文不能配置。未连的匹配分支不会落入 Default；没有匹配且无 Default 时执行结束。

类名、成员名、类型、变量引用、逻辑 ID 和引脚区分大小写；string 值与 name 显示拼写也按原文保留。FName 的运行比较仍遵守 UE 自身规则。当前不支持自定义父类、多图、宏、Timeline、额外组件、循环、容器和任意结构体。详情见[蓝图参考](../Docs/Versions/v0.1.0/Reference/Blueprint.md)。

## 材质实例

父资源必须是已保存且无未保存修改的普通 Surface Material。用资源检查读取真实父参数和 SHA 后再填写请求；不能使用实例父、静态 Switch、材质层参数或全零摘要。

v2 的 settings 恰含 `parent/parentAssetSha256/scalarParameters/vectorParameters/textureParameters`，nodes/edges 为空。三个参数映射即使无覆盖也须提供。仅支持全局 scalar、四分量线性 RGBA vector 和普通 Texture2D；数值有限且绝对值最多 1,000,000，参数名、类型与采样器须匹配实际父材质。生成不会修改父文件，不支持实例图导出或重排。

## 来源与结果

读取范围由工程的 `Config/NodeForgeInstallation.json` 和插件 `InstallationManifest.json` 共同约束。目录中的对象路径格式只检查语法；实际存在、资源类型、保存状态、清洁状态及可读目录仍由插件检查。

导出报告绑定原报告、源文件和内容摘要。首次载入应与导出内容一致；后续显式保留来源的编辑和准备副本按[编辑器控制协议](EditorControlProtocol.md)处理。普通资源引用不自动取得导出来源证明。

逻辑 ID 保存在材质 Desc 或蓝图 NodeComment 的 `NodeForge:` 元数据中，这些保留字段不能被输入改写。插件会比较构建后、编译后、保存后和独立重开的实际结构。状态与失败处理见[结果说明](ResultContractDraft.md)；预检、保存和运行效果分别判断。
