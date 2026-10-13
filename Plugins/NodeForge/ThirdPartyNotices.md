# 代码来源

NodeForgePrototype.cpp 的蓝图精确事件/函数创建顺序、编译诊断遍历与单包保存返回处理，参考并改造 ZiggyMar/unreal-mcp 固定提交 d1a85396d9c62328521256005fe7c420725651cc 的 MCPCommandHandler.cpp：AddNodeCore、CompileMessagesToJson、HandleCompileBlueprint、SaveAssetPackage（仅原生保存小段）。

Copyright (c) 2026 Ziggy。许可全文见[ZiggyMarMIT.txt](Licenses/ZiggyMarMIT.txt)。

改造：移除MCP/socket/响应/SourceControl/既有资产加载；固定两个真实函数、严格连接/输入、仅自有候选、拒绝碰撞、分层失败/保存/独立重开报告。未移植上游批回滚或内存对象改名逻辑。

上述描述对应保留的 v1 实现。v2 新增实际提取改造如下；控制/校验/界面/测试继续使用本项目代码。

- NodeForgeMaterialGraph.cpp 的 ConnectExact 提取 ChiR24/Unreal_mcp 固定提交 591d3b0ea3c46a2a1926dbcafc225afecb6a75b3，MaterialAuthoring/Connections/McpAutomationBridge_MaterialAuthoringHandlersConnectNodes.cpp 的 HandleConnectNodes 输出索引检查和 Wire 原生连线核心。保留精确输出/原生掩码与连接，去掉别名、默认引脚回退、网络和宏依赖。版权及完整 MIT 见源码头和 [ChiR24MIT.txt](Licenses/ChiR24MIT.txt)，与归档原始 LICENSE 字节一致。
- NodeForgeBlueprintGraph.cpp 提取上述 ZiggyMar 固定提交的 MCPNodeCatalog.cpp/ShouldIncludeFunction 过滤、MCPCommandHandler.cpp/AddNodeCore 自身变量绑定和 SetFromFunction 小段；结合 UE 原生 ActionDatabase/NodeSpawner/K2 schema，删除不受支持的模糊匹配和远端功能。版权和 MIT 继续保留于源码与 ZiggyMarMIT.txt。

逐模块接入、自写缺口及本机适配依据位于NodeForge文件库根目录的`Docs/MaterialGraphReuse.md`和`Docs/BlueprintGraphReuse.md`，从根索引`1.md`进入。专题文档不随插件stage复制，故这里使用明确的文件库路径；本文件中的`Licenses/`链接仍相对本插件目录，可随插件保留。没有复制 GraphDeck、MCPMaterialGraph.cpp 的 Epic 标头文件、Kantan/MPL 或其依赖代码。本文件不重新许可 UE 引擎代码。

2026-10-09布局扩展：NodeForgeGraphLayout.cpp的Kahn最长路径分层骨架提取自PRQELT/Autonomix固定提交47a46ae8cc3f00230edceeb33f0d4f9d80cb185b，Source/AutonomixActions/Private/Blueprint/AutonomixBlueprintActions.cpp/FAutonomixBlueprintActions::AutoLayoutNodes。Copyright(c)2025 Autonomix，完整[MIT声明](Licenses/AutonomixMIT.txt)与源码头保留。去掉宿主日志/exec-only/固定间距/不稳定集合遍历，补数据依赖、稳定ID、节点尺寸、锚点与分支排序。调用UE原生材质布局及节点尺寸API，不复制其实现，不引入Autonomix其它模块。

2026-10-09材质实例：NodeForgeMaterialInstances.cpp 的 Factory/InitialParent 设置实际提取改造自上述 ZiggyMar 固定提交 MCPCommandHandler.cpp/HandleCreateMaterialInstance（约 L11765–11769）。保留原生工厂和父绑定，将上游 AssetTools.CreateAsset 改为在本项目暂存包调用 FactoryCreateNew，由既有流程负责同名保护、发布及保存；不引入 MCP、网络或上游既有资产修改。参数存在检查采用本机原生 GetAllScalar/Vector/TextureParameterInfo，类型化 setter 之后独立核对 override 数组与有效值。Copyright (c) 2026 Ziggy，MIT 全文继续保留。


## 原生函数绑定与纹理采样补充（2026-10-09）

NodeForgeMaterialFunctions.cpp/h为项目自写的受控引用/签名适配，直接调用本机UE5.3.2公共SetMaterialFunction及原生FunctionInputs/Outputs。继续实际使用既有固定ChiR24 MIT ConnectExact连接；实例采样直接调用引擎原生递归与VerifySamplerType，不复制引擎实现。现有ZiggyMar/ChiR24原始MIT文本在根和Build14 stage逐字节核对一致，证据Reports/20261009UsableEditor/FunctionsLicense1.json。Build14归档声明保留构建时字节，本段后来补充不改归档。GraphDeck及未知来源Epic文件仍未复制。

2026-10-09蓝图向量/自身Actor扩展：沿用上述ZiggyMar固定MIT过滤/自身变量绑定与原生ActionDatabase/NodeSpawner。向量采用UE公共TBaseStructure/FStructProperty、FDefaultValueHelper和K2 schema，函数执行仍由原生MakeVector/BreakVector/Conv_VectorToString/Actor位置函数完成；自写仅有受控JSON三坐标映射、精确类型/范围与旧目录子集校验、回读和实际位置测试，没有复制未知Epic或GraphDeck实现，也未增加第三方运行依赖。


2026-10-09普通蓝图函数族扩展：继续复用既有ZiggyMar固定MIT过滤、绑定与原生ActionDatabase/NodeSpawner/K2 schema；新增只是函数族配置、整型/名称默认值约束和原生PIE旋转/缩放读取。函数计算由UE原生Kismet/Actor执行，没有复制引擎实现、未知Epic或GraphDeck代码，没有新增第三方依赖。

2026-10-09纹理对象/函数组合扩展：NodeForgeMaterialGraph.cpp继续使用原生MaterialEditingLibrary、反射属性、既有固定ChiR24 MIT精确连接和NodeForgeMaterialFunctions受控绑定。六个表达式通过Spec配置接入，WorldAlignedTexture以安装引擎资产路径及文件哈希引用，不复制其实现。TextureObjectParameter不消费继承UV/Mip输入的适配规则来自本机原生接口/实现核对；仅新增小型拒绝策略，没有重写效果整图或引入外部依赖。

2026-10-09自身根组件适配：NodeForgeBlueprintGraph.cpp继续实际使用既有ZiggyMar MIT过滤/绑定与原生FunctionNodeSpawner、K2 schema。新增函数族配置和自身RootComponent来源约束；SceneComponent调用保持原生外部成员绑定，目录复用相同构图器创建上下文探针。引擎头文件/实现仅核对签名与原生行为，没有复制UE实现或来源/许可不明代码，没有额外对象加载器。

2026-10-09整数/名称成员扩展：复用同一变量创建/绑定路径、FBlueprintEditorUtils::AddMemberVariable、原生VariableGet/Set和FIntProperty/FNameProperty编译CDO读取。新增受控类型映射、int32精确范围/回读、原生名称默认值拼写约束及旧目录门槛；已归档实现不提供本项目确定性默认值/保存重开契约，因此只补这些边界，不另写变量节点实现或引入依赖。本机PropertyName.cpp仅核对行为，没有复制引擎实现。

2026-10-09顺序执行节点适配：原生UK2Node_ExecutionSequence通过现有UBlueprintNodeSpawner创建，使用IK2Node_AddPinInterface虚调用和GetThenPinGivenIndex扩展/核对输出，原有连接、布局和导出路径保持。仅补受控outputCount、完整引脚/执行顺序检查及逐配置原生目录探针；本机原生编译器按Pins顺序执行的源码仅用于核对，不复制引擎方法体、不重写执行器，不引入新第三方依赖。

2026-10-09材质空间输入扩展：WorldPosition、ObjectPositionWS、CameraPositionWS、CameraVectorWS、VertexNormalWS、PixelNormalWS通过现有Spec配置、UMaterialEditingLibrary和固定ChiR24 MIT精确连接接入；没有复制原生坐标/法线计算实现。通用属性映射补FEnumProperty并复用原枚举名校验，目录版本约束合并为等价的按版本配置，避免新族复用旧摘要。自写仅为协议约束/证据，不引入新第三方依赖或材质效果分支。

2026-10-09旋转成员与默认值：继续复用现有ZiggyMar固定MIT变量绑定/函数过滤、原生AddMemberVariable、VariableGet/Set、K2 schema和节点创建器。Rotator适配直接调用TBaseStructure<FRotator>、FStructProperty和FDefaultValueHelper::ParseRotator；本机DefaultValueHelper.cpp仅核实原生CSV顺序和成员文本解析，不复制实现。自写范围为严格pitch/yaw/roll数据映射、旧目录门槛、CDO/引脚回读与导出；已归档实现不提供本项目的版本化确定性默认值契约，无需新库或新节点执行实现。

2026-10-09材质条件选择/去色：Min、Max、If、Desaturation由既有Spec配置接入UMaterialEditingLibrary、原生反射白名单和ChiR24固定MIT精确连接。没有复制引擎表达式计算实现、GraphDeck或不明Epic素材；仅扩展类型/属性配置和目录版本门槛，不新增完整效果配方、依赖或执行器。本机MaterialExpression*.h与MaterialExpressions.cpp只核对API、参数与原生行为，定向复用记录见Reports/20261009UsableEditor/ColorSelectionExpansion1/ReuseReview.md。

2026-10-09蓝图数学与插值：Abs、FMin、FMax、Lerp、DegSin、DegCos、RLerp通过既有函数族配置及UE原生ActionDatabase/FunctionNodeSpawner/K2 schema接入。继续实际使用上述ZiggyMar固定MIT过滤、绑定与精确回读；计算/旋转插值由原生函数执行，没有复制引擎实现、添加函数专用构图分支或依赖。新增仅为配置族标志与旧目录权限门槛，本机KismetMathLibrary.h用于核对签名和类型。

2026-10-09整数/名称多路选择：NodeForgeBlueprintSwitches.cpp/h为项目自写的有限动态cases配置/回读适配，调用原生NodeSpawner Customize、SwitchInteger AddPinToSwitchNode及SwitchName PinNames引脚分配；不手造引脚或重写分支执行器。已归档ChiR24 NodeCatalog仅有类别名，没有cases适配，故不复制该别名表冒充复用。根构图器继续实际使用已有ZiggyMar固定MIT、原生K2连接/编译与通用保护/布局/保存/导出；引擎Switch实现只用于核实API、保留名、隐藏比较上下文及int32递增安全边界，没有复制方法体或增加依赖。
