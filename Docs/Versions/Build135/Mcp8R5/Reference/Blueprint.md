# Actor Blueprint：327 签名、8 类与执行边界

当前v3结构见[GraphAuthoring](GraphAuthoring.md)，三族完整v3输入见[AiJsonArchitectureExamples](../Ai/JsonArchitectureExamples.md)。下方settings/nodes/edges和绑定properties是仍使用的v2详细表示；v3在asset/graphs/binding中明确分区，解码到同一生产描述。


当前v3结构见[GraphAuthoring](GraphAuthoring.md)，三族完整v3输入见[AiJsonArchitectureExamples](../../../Build134/Mcp7R5/Ai/JsonArchitectureExamples.md)。下方settings/nodes/edges和绑定properties是仍使用的v2详细表示；v3在asset/graphs/binding中明确分区，解码到同一生产描述。


当前v3结构见[GraphAuthoring](GraphAuthoring.md)，三族完整v3输入见[AiJsonArchitectureExamples](../../../Build133/Mcp6R4/Ai/JsonArchitectureExamples.md)。下方settings/nodes/edges和绑定properties是仍使用的v2详细表示；v3在asset/graphs/binding中明确分区，解码到同一生产描述。


目录327签名/8类：299 CallFunction、2 Event、7 Sequence（outputCount2–8）、1 Branch、SwitchInteger/Name各2种 hasDefault探针、bool/float/string/vector/int/name/rotator 各Get/Set。状态experimental；变量探针 Value 必须换成输入中真实声明名，不把它当固定端口。

普通 Actor、一个 EventGraph；UE默认 SceneRoot/空 ConstructionScript 是脚手架，另列回读。最多128节点/256边/64成员；nodes/edges条目顺序不表示执行顺序；cases等配置数组保持语义顺序，original来源比较仍核原节点数组顺序。允许无事件的纯数据图，但运行另验。事件按本包目录：Actor.ReceiveBeginPlay、Actor.ReceiveTick，每个精确事件最多1个、可共存；函数 owner/member 必须在目录且精确绑定，不允许任意 UFunction。

settings 恰为 parentClass=/Script/Engine.Actor、variables 数组（可空），成员 name ASCII字母开头、字母数字、不能大小写折叠重复或继承碰撞。float 为原生 real/double；vector=[x,y,z]；rotator={pitch,yaw,roll} 三角度数且各有限abs≤1e12；int32 整数范围；name 非空且小于NAME_SIZE，用规范 None 表示空身份；string≤4096。编译后读取真实 CDO，不依赖清空的编辑器声明 DefaultValue。

defaults 仅未连接、可写、可见的精确原生输入；bool/real/string/vector/int/name/rotator按类型，不用字符串替代数字/数组替代Rotator，不任意对象/容器。连接仅原生 MAKE；禁止自动转换/替换/环、split/orphan pins。

自身 Actor 函数用隐式 self；SceneComponent 成员只有本图 Actor.K2_GetRootComponent.ReturnValue 直接连 component.self，拒绝外部Actor/组件、任意对象默认/隐式cast/reroute。碰撞/物理/网络或任意调度正确性不由构图成功证明。

Sequence明确 outputCount2–8，端口逐字 Then_0…；历史具体输出数配置的编译/运行不能推广到其它数。SwitchInteger限定startIndex/caseCount1–8/hasDefault、int32溢出保护；SwitchName有限唯一cases/hasDefault。原生name相等大小写无关，但端口显示身份精确，无Default时不匹配结束，不补出口。

不支持自定义父类、任意多图、宏/函数图、自定义事件、timeline/delegate/interface、额外组件、循环、数组/Map/Set/enum、任意结构体或未知函数。图规模上限不等于可生成任何复杂业务；复杂度受这些执行/对象/类型限制。

读 [327签名索引](BlueprintNodeIndex.md)和对应节点/历史组合证据。目录扩展沿相同 CallFunction/反射/Spawner/配置路径，MCP 工具本身无需按函数追加。


已有七类 Value 变量可显式提供 attributes 三字段：instanceEditable、category、tooltip。整组省略保留原 factory 行为，完整规则和原 MCP 配方见[统一资产设置](../Ai/AssetSettings.md)。
