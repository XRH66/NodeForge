# 给外部 AI 的 NodeForgeGraph v2 规则

**2026-10-10 适用范围更正**：下文是 Build3 / `0d786af7…884f` 的历史规则和样例快照，类、属性、变量类型等限制适用于该快照，不能当作当前能力全集。当前生成先取得实际插件导出的 `GraphRules` / 目录，结合[协议](../Contracts/GraphProtocol.md)、[目录导航](../Catalog/Readme.md)及对应专题规则；后续明确扩展覆盖下文同项旧限制。使用当前目录中的规范标识与实际摘要，不能猜节点、端口或资源。目录发现/实例化不等于所有组合已验证。历史 JSON 与摘要原样保留，不把查询报告包装当成可导入请求。

2026-10-09。将本文与具体效果/行为要求一起提供给其它 AI，可另外附上[当前真实目录 UE532Graph](../Catalog/UE532Graph.json)，它是 Build3 [Catalog2 原始报告](../Reports/20261008GraphUpgrade/Catalog2.json)的准确副本。本文是可用输入规则快照，不是所有节点或组合的实测通过声明；24 个材质类、15 个蓝图签名表示实际发现/实例化范围，编译、保存、重开、运行及视觉要看本地报告。协议权威说明见 [GraphProtocol](../Contracts/GraphProtocol.md)，本节以下可直接交给生成数据的 AI。

本次目录新增 `reachabilityPolicy`，明确所有材质表达式必须可达有效输出，故摘要由 Catalog1 的历史值更新为下方 Catalog2 的实际值；节点/端口内容未变。旧原始报告保持不动。目录文件为报告包装，读取根 catalogDigest，读取 catalog.material.nodes / catalog.blueprint.nodes；不能将整个文件 SHA 当作目录摘要。

## 你的任务

你为 Windows、UE 5.3.2 上的 NodeForge Editor 插件生成结构化数据。根据调用者要求，在下列明确范围内组合节点、属性、输入默认值及连线。图结构由 JSON 决定，节点数量、顺序和拓扑可变化。

输出一个资产时，只输出一个完整 JSON 对象。要求材质和蓝图两个资产时，输出两个独立 JSON 代码块，分别标注 Material.json 与 Blueprint.json；不要把它们装进数组或其它外层字段，不要把说明文字写入 JSON。文件必须能保存为无 BOM 的 UTF-8，最大 65536 字节，不能含注释、重复键或尾逗号。

不能使用下表之外的类/属性/函数，也不能猜引脚、创建不存在的贴图、添加执行脚本或声称结果已验证。遇到不可表达需求，先明确缺少的能力；不要用简单灰色资产代替要求。复制示例时需按任务生成新的 requestId 与新资产名；不能覆盖本地已存在资产。调用者未提供现有名称清单时，你只能生成候选新名称，最终碰撞由导入器拒绝。

## 必填根结构

根必须恰有九字段，不能添加 title、description、success 等字段：

```json
{
  "format": "NodeForgeGraph",
  "version": 2,
  "requestId": "AiGraphNewRequest1",
  "assetType": "Material",
  "assetPath": "/Game/NodeForgeTests/NFAiGraphNewAsset1",
  "catalogDigest": "0d786af7d63e1ea209691493e9b336f50e61eaeb45ef34eeec12ff01b22a884f",
  "nodes": [],
  "edges": [],
  "settings": {}
}
```

这是结构说明，空节点/设置不能直接导入。assetType 只能 Material 或 Blueprint。requestId 和目标资产名都是 1–64 个 ASCII 字母数字；assetPath 固定前缀 `/Game/NodeForgeTests/` 后直接接名字，无扩展名、对象后缀、子文件夹、空格、中文或下划线。节点/参数/变量 ID 也遵循各自下面的命名限制，输出文本可以是中文。

摘要来自本机真实 `GraphCatalog`，对应 engine `5.3.2-29314046+++UE5+Release-5.3`、adapter `NodeForge-UE532-Graph-2`。只有提供了更新的实际目录，才替换摘要和遵循新目录；不能自己编摘要或拿 v1 的摘要代用。旧 `NodeForgePrototype` v1 有独立格式，本任务使用 v2，不能混写旧版顶层 owner/member/value。

## 节点与边共同语法

节点必填 `id,class,properties`，可选 `defaults,position`，其它字段禁止。

```json
{"id":"Scale","class":"/Script/Engine.MaterialExpressionMultiply","properties":{},"defaults":{"B":0.4},"position":[-300,0]}
```

id 在本图唯一，为 1–64 个 ASCII 字母数字。properties 没有值也必须写 `{}`。position 为可选 `[x,y]` 整数；为便于兼容，两领域都建议在 ±100000 内。不要根据 nodes 数组先后猜依赖。

每条边恰有字符串 `from,output,to,input`。例如材质 `{"from":"Color","output":"0","to":"Scale","input":"A"}`；蓝图 `{"from":"Begin","output":"then","to":"Print","input":"execute"}`。

材质 output 是数字索引的字符串；蓝图 output 是真实引脚名。输入输出必须精确区分大小写；同一目标输入只能接一条边，不能自连或成环。同一输入不能同时被连接和设置 defaults。所有声明节点都将实际创建，不能放无意义的“说明节点”。

## 材质可用目录

完整类路径是 `/Script/Engine.MaterialExpression` 加下表后缀。`*` 表示必需连接。没有列出的属性不可写；properties 省略键表示保留引擎原生默认。

| 类后缀 | properties | 输入 | defaults 可写键 |
| --- | --- | --- | --- |
| Constant | `R:number` | 无 | 无 |
| Constant2Vector | `R:number,G:number` | 无 | 无 |
| Constant3Vector、Constant4Vector | `Constant:[r,g,b,a]` | 无 | 无 |
| ScalarParameter | `ParameterName:string,DefaultValue:number` | 无 | 无 |
| VectorParameter | `ParameterName:string,DefaultValue:[r,g,b,a]` | 无 | 无 |
| Add、Subtract、Multiply、Divide | `ConstA:number,ConstB:number` | `A,B` | `A,B` |
| Power | `ConstExponent:number` | `Base*,Exponent` | `Exponent` |
| Sine、Cosine | `Period:number` | `Input*` | 无 |
| Abs、OneMinus | `{}` | `Input*` | 无 |
| Normalize | `{}` | `VectorInput*` | 无 |
| Clamp | `MinDefault:number,MaxDefault:number` | `Input*,Min,Max` | `Min,Max` |
| AppendVector | `{}` | `A*,B*` | 无 |
| ComponentMask | `R:bool,G:bool,B:bool,A:bool` | `Input*` | 无 |
| LinearInterpolate | `ConstA:number,ConstB:number,ConstAlpha:number` | `A,B,Alpha` | `A,B,Alpha` |
| TextureCoordinate | `CoordinateIndex:int,UTiling:number,VTiling:number,UnMirrorU:bool,UnMirrorV:bool` | 无 | 无 |
| Time | `bIgnorePause:bool,bOverride_Period:bool,Period:number` | 无 | 无 |
| Panner | `SpeedX:number,SpeedY:number,ConstCoordinate:int,bFractionalPart:bool` | `Coordinate,Time,Speed` | `Coordinate`：UV 通道编号 |
| TextureSample | `Texture:string,SamplerType:string,ConstCoordinate:int` | `Coordinates,TextureObject,Apply View MipBias` | `Coordinates`：UV 通道编号 |

材质约束：

- 节点 1–256，边 1–1024，至少一条连接到 `to:"Material"` 的有效材质输出。每个材质节点都必须沿输出边最终到达有效 Material 输出；孤立节点及整条无输出作用的支链会明确拒绝。不能把 `Material` 用作表达式 id。
- 数值有限且在 ±1000000 内；UV 通道 CoordinateIndex/ConstCoordinate 为 0–7 整数。显式 Period 必须正数。普通 Time 建议 properties `{}`，不要显式给 Period=0。
- 颜色是四个线性 RGBA 数字，即使 Constant3Vector 也写四数数组。参数名必须显式、1–64 ASCII 字母数字、同一材质不重名；参数 DefaultValue 类型严格匹配。
- ComponentMask 的通道字段是布尔，至少一个 true。不要使用 `channels:"R"` 等未实现字段。
- defaults 只接受表内未连接输入的数字；不能同时声明对应 Const 属性。例如 Multiply 用 defaults.B 后，properties 中不能再给 ConstB。
- 除向量/纹理外，各类输出使用 `"0"`。Constant2Vector 的 0/1/2 对应 RG/R/G；Constant3Vector 的 0/1/2/3 对应 RGB/R/G/B；Constant4Vector 的 0/1/2/3/4 对应 RGBA/R/G/B/A；VectorParameter 的 0/1/2/3/4 对应 RGB/R/G/B/A；TextureSample 的 0/1/2/3/4/5 对应 RGB/R/G/B/A/RGBA。必须写索引字符串，不能写 `"RGB"`。
- TextureSample 需要现有非虚拟 UTexture2D 精确对象路径，前缀只能 `/Engine/` 或 `/Game/NodeForgeTests/`。本机已核实存在的测试贴图为 `/Engine/EngineResources/DefaultTexture.DefaultTexture`；它只是引擎默认图，不是水面法线图。只有调用者确认存在时才引用其它资源。通常省略 SamplerType 让原生推断；显式枚举必须与贴图匹配。没有 TextureObject 创建器，不要编造这一类节点。
- Time/Panner/数学可以组合动态效果。必须避免除零、无效幂、向量维度不匹配；图经过语法检查仍需 UE 真正编译。

材质 settings 必填 domain/blend/shading/twoSided，仅可选 lighting：

```json
{"domain":"Surface","blend":"Opaque","shading":"DefaultLit","twoSided":false}
```

或需要半透明时：

```json
{"domain":"Surface","blend":"Translucent","shading":"DefaultLit","twoSided":true,"lighting":"SurfaceForwardShading"}
```

domain 只允许 Surface；blend 为 Opaque/Translucent；shading 为 DefaultLit/Unlit。Translucent 必须给上面的 lighting。资产输出允许 `BaseColor,EmissiveColor,Metallic,Normal,Opacity,Roughness,Specular,WorldPositionOffset`，且必须在所选模式下有效；Unlit 通常连 EmissiveColor，Opaque 不连 Opacity。不能增加 Refraction、Custom HLSL、材质函数、Material Attributes 或需要改变工程设置的能力。

## 蓝图可用目录

所有节点类的完整前缀是 `/Script/BlueprintGraph.`：

| 类 | properties | 可用常见引脚 |
| --- | --- | --- |
| K2Node_Event | `{"owner":"/Script/Engine.Actor","member":"ReceiveBeginPlay"}` | 输出 then，最多一个 BeginPlay |
| K2Node_CallFunction | 精确 owner/member，见下表 | 原生函数引脚 |
| K2Node_IfThenElse | `{}` | 输入 execute、Condition(bool)，输出 then、else |
| K2Node_VariableGet | `{"variable":"变量名"}` | 输出等于变量名 |
| K2Node_VariableSet | `{"variable":"变量名"}` | 输入 execute、变量名；输出 then、Output_Get |

可绑定函数：

| owner | member | 输入→输出 |
| --- | --- | --- |
| `/Script/Engine.KismetSystemLibrary` | PrintString | execute→then；InString(string)、bPrintToScreen(bool)、bPrintToLog(bool)、Duration(float) |
| `/Script/Engine.KismetStringLibrary` | Concat_StrStr | A(string)、B(string)→ReturnValue(string) |
| `/Script/Engine.KismetStringLibrary` | Conv_DoubleToString | InDouble(double)→ReturnValue(string) |
| `/Script/Engine.KismetMathLibrary` | Add_DoubleDouble | A(double)、B(double)→ReturnValue(double) |
| `/Script/Engine.KismetMathLibrary` | Multiply_DoubleDouble | A(double)、B(double)→ReturnValue(double) |
| `/Script/Engine.KismetMathLibrary` | Greater_DoubleDouble | A(double)、B(double)→ReturnValue(bool) |
| `/Script/Engine.KismetMathLibrary` | BooleanAND | A(bool)、B(bool)→ReturnValue(bool) |

只有 PrintString 有 execute/then，其它表列函数是纯数据节点。不要连接或配置隐藏 self/WorldContext 引脚。默认值只能放在节点 defaults 内，类型须匹配；例如 `{"InString":"输入决定这句话","bPrintToScreen":false,"bPrintToLog":true}`。TextColor、Key 等默认值尚未开放；不要把字符串伪装成 struct/name。double 与 float 不能假定可隐式转换。

蓝图 settings 必须恰有：

```json
{"parentClass":"/Script/Engine.Actor","variables":[{"name":"Message","type":"string","default":"原始值"},{"name":"Enabled","type":"bool","default":true},{"name":"Amount","type":"float","default":0.5}]}
```

变量数组可空。每项必须恰有 name/type/default；类型仅 bool/float/string，float 在本机是 real/double。name 以 ASCII 字母开头，其余只字母数字，长度 1–64，不与 Actor 继承成员冲突，也不与其它变量大小写折叠重名。节点 variable 必须大小写精确引用这里的声明。

目录变量样本叫 Value，但它不是固定引脚：声明 Message 时 Getter 输出为 `Message`，Setter 输入为 `Message`，Setter 输出仍为 `Output_Get`。声明 Amount 时相应 pin 就是 `Amount`。其类型来自变量声明。

蓝图 1–128 节点、0–256 边、0–64 变量。默认字符串最多 4096 字符，默认数字有限且在 ±1e12 内；布尔必须 true/false。defaults 只支持未连接的可写标量输入。连接不能替换已有边、自动插入转换节点或形成环。循环、宏、自定义事件、数组/Map/Set、任意函数以及生产工程资产修改不在当前范围。引擎原生 ConstructionScript/SceneRoot 由导入器保留，JSON 不用也不能伪造脚手架节点。

## 两个完整语法示例

以下示例用于说明精确结构；它们未在本文被宣称导入或运行通过。实际生成时根据任务改变合法节点组合，并使用新名称。材质示例为有色自发光、默认值控制亮度：

```json
{
  "format":"NodeForgeGraph",
  "version":2,
  "requestId":"AiSyntaxMaterialExample1",
  "assetType":"Material",
  "assetPath":"/Game/NodeForgeTests/NFAiSyntaxMaterialExample1",
  "catalogDigest":"0d786af7d63e1ea209691493e9b336f50e61eaeb45ef34eeec12ff01b22a884f",
  "nodes":[
    {"id":"Tint","class":"/Script/Engine.MaterialExpressionVectorParameter","properties":{"ParameterName":"TintColor","DefaultValue":[0.1,0.5,0.8,1]}},
    {"id":"Brightness","class":"/Script/Engine.MaterialExpressionMultiply","properties":{},"defaults":{"B":0.7}}
  ],
  "edges":[
    {"from":"Tint","output":"0","to":"Brightness","input":"A"},
    {"from":"Brightness","output":"0","to":"Material","input":"EmissiveColor"}
  ],
  "settings":{"domain":"Surface","blend":"Opaque","shading":"Unlit","twoSided":false}
}
```

蓝图示例为原生字符串函数输出到 PrintString：

```json
{
  "format":"NodeForgeGraph",
  "version":2,
  "requestId":"AiSyntaxBlueprintExample1",
  "assetType":"Blueprint",
  "assetPath":"/Game/NodeForgeTests/NFAiSyntaxBlueprintExample1",
  "catalogDigest":"0d786af7d63e1ea209691493e9b336f50e61eaeb45ef34eeec12ff01b22a884f",
  "nodes":[
    {"id":"Begin","class":"/Script/BlueprintGraph.K2Node_Event","properties":{"owner":"/Script/Engine.Actor","member":"ReceiveBeginPlay"}},
    {"id":"Join","class":"/Script/BlueprintGraph.K2Node_CallFunction","properties":{"owner":"/Script/Engine.KismetStringLibrary","member":"Concat_StrStr"},"defaults":{"A":"NodeForge ","B":"reads graph data"}},
    {"id":"Print","class":"/Script/BlueprintGraph.K2Node_CallFunction","properties":{"owner":"/Script/Engine.KismetSystemLibrary","member":"PrintString"},"defaults":{"bPrintToScreen":false,"bPrintToLog":true}}
  ],
  "edges":[
    {"from":"Begin","output":"then","to":"Print","input":"execute"},
    {"from":"Join","output":"ReturnValue","to":"Print","input":"InString"}
  ],
  "settings":{"parentClass":"/Script/Engine.Actor","variables":[]}
}
```

## 输出前检查

检查九根字段、真实摘要、所有 ID/引用、精确 class/owner/member、允许 properties、默认类型及同输入冲突、输出索引/引脚方向、模式下有效材质输出和无环。不要在缺能力时跳过节点或偷偷换效果。输出完成只表示提供数据；由用户实际导入、编译、保存和独立重开后才能判断生成成功，动画和蓝图行为需另行运行。

执行者保留外部 AI 原始响应、原始 JSON、来源和导入报告。人工修订另存并关联原文；不能将人工修订成功写成 AI 原样一次生成成功。
