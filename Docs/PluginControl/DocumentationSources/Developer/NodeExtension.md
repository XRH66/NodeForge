# 节点扩展与 MCP 同步

普通节点：先确认需求可用现有Material Specs或Blueprint函数/事件配置表达；查UE5.3官方文档/论坛和本机头文件，核函数所有者、类/属性/引脚、默认与许可。修改核心Specs/过滤配置，走原反射/Spawner，不为节点写MCP handler。更新正式目录、TaskRules/revision/digest；补精确identity的中文用途、前提、引脚/默认/限制、合法组合示例和原版本证据。操作参数/结果不变时MCP工具数不变。用必要代表正/负例核准入、默认/连接、编译保存/Verify；有运行语义才单独运行，保留未验。

特殊语义：例如新动态函数签名、成员类型、Sequence/Switch输出、SceneComponent上下文。先核解析/构图/回读/编译/导出/预览各受影响面，再实现真实语义和精确配置，不只放开类名。动态GUID/签名、变量声明与占位引脚、对象生命周期和执行出口要一致。对应目录/规则/JSON/schema/例子/配置证据同步；某个代表配置通过不推广到所有配置。需要多图、数组、循环、新父类等时先扩大需求评审，本期不实施。

发布同步：准确coreBuild/DLL/engine/adapter、Catalog、Operations、ToolMetadata、精确NodeDocumentation、EvidenceIndex、文档源及ReleaseInput一起进入原发布器。新增identity缺用途/前提/限制/例子/来源时阻止发布；节点允许范围仍只在核心。Node页身份用精确class/owner/member/variableType/动态配置，页ID由身份hash生成，版本索引再绑定；不使用Node001跨版匹配。

改变Catalog或DLL后旧缓存失效，AI重读目录与具体节点。旧JSON按其原子集和版本限制，不能只改digest宣称迁移成功。输入/业务未变可复用旧可靠证据，新的节点实现必须有按影响实际检查；数据快照适配只证明发现/资料变化。

第三类新业务入口见DeveloperOperationExtension。停止条件：真实签名/上下文不明、需放宽源保护或任意函数、返回/回读不一致、unknown或部分写入未解、超出已授权范围。


## 统一登记后的普通批量扩展

NodeForgeNodeDefinitions.h/.cpp 是节点事实来源：原有48材质定义/64函数绑定、8特殊K2类、7成员类型和准确历史能力档案集中维护。BlueprintGraph/MaterialGraph 是原生适配器，校验、生成、回读、导出都消费同一登记；不再在原 Functions/Specs 与新表各加一遍。限定库中的普通函数以反射共同筛选，材质以 Engine 原生 Math 类及准确属性规则筛选。新增普通身份先 pendingNative，同一工厂/Spawner 实例核原生端口/类型/默认后才准入。Pure 或 CPF_Edit 本身不够。

新类型/动态上下文先修改该类别共同适配，不能扩大普通规则掩盖特殊语义。旧纹理/函数绑定、Actor/root 来源、Sequence/Switch 和成员类型仍使用原必要适配。未知类型、随机状态、Latent、CustomThunk、容器、mutable ref、GUID/资源等本期不自动开放。

原生目录完成后发布器 ordinary_node_knowledge 从已准入普通族的精确签名、类型、原生 Tooltip/Category 生成逐项知识；不会借文档函数允许一个核心未准入节点。旧特殊/既有人工用途仍按精确身份维护，缺必需知识拒绝发布。新增普通节点不需 MCP handler，MCP 沿原 catalog_get/rules_query 发现，同版节点页和差异自动生成。人工可继续补中文业务解释，但不是第二张支持白名单。

新 DLL 先在同工程候选窗取得真实完整目录，检查旧身份/摘要能力和容量，生成有意义两族代表图；全部新增节点构造/签名用临时对象，不逐节点落盘。原 JSON、结果、收据与新PID Verify 使用原正式链。普通规则或类型/构造发生变化才重验对应路径；工具 schema 未变复用原 SDK 证据。维护者从本版 SourceMap、CompatibilityAndMigration 和 NodeBulk 指引恢复，不靠旧聊天。


新增普通字段时，在AssetDefinitions对应字段/type/value规则登记，再闭合原族native Apply、ReadActual和Export；codec、native目录schema及帮助从同一事实生成。事件/函数/成员/材质函数绑定来自NodeDefinitions，不放回普通properties。特殊原生行为仍沿原适配，不能设逐节点MCP handler；组件、多图、Tick等必须另有范围授权。
