# 自身根组件调用：给外部 AI 的规则

目标 Windows、官方 UE 5.3.2，使用 Build39 隔离插件及[真实目录](../Catalog/UE532BlueprintComponents.json)。当前摘要为 `7dea96da738f0f1b526e7782ec08230a4e0d6d14e815db2538638693f2309326`。冻结 RC1 与根运行插件尚未升级。目录发现/构建与实测分别记录，详[功能及证据](../Docs/BlueprintComponents.md)。

继承 [Graph v2 协议](../Contracts/GraphProtocol.md)和[普通函数族规则](BlueprintFunctionFamiliesAiRules.md)的格式、限制、精确引脚及资产保护；本文件替代其中目录摘要和“所有组件调用均不支持”的旧范围。输出一个完整 JSON 对象：format/version/requestId/assetType/assetPath/catalogDigest/nodes/edges/settings。format=`NodeForgeGraph`、version=2、assetType=`Blueprint`，目标仅 `/Game/NodeForgeTests/` 下新名称。禁止覆盖。

从目录的 `catalog.blueprint.nodes` 选择 class/properties.owner/member 和真实引脚。普通函数共用 `/Script/BlueprintGraph.K2Node_CallFunction`，不使用展示名或 fixture 标识。不执行 JSON 中的脚本，不隐式改线或自动类型转换。成员声明仍限 bool/float/string/vector；int/name 仅支持现有函数引脚，不能声明为成员变量。

本次新增的是受控的根组件来源及调用，使用普通 edges，不增加对象路径字段：

- 根来源：owner=`/Script/Engine.Actor`、member=`K2_GetRootComponent`，Actor self 隐式指向当前蓝图实例。其输出名为 `ReturnValue`，类型为 SceneComponent。`GetRootComponent` 不是本目录的函数名。
- 组件调用的 owner 为 `/Script/Engine.SceneComponent`，每个节点的 `self` 必须恰有一条来自同图上述 Root 节点 `ReturnValue` 的直连。该 Root 输出可接多个合法组件 self。不要填写 self 默认值、对象路径、任意对象引用或中转节点。
- 读取：`K2_GetComponentLocation`、`K2_GetComponentRotation`、`K2_GetComponentScale`，输出均为 `ReturnValue`。实际类型分别为 vector、rotator、vector。
- 写入：`K2_SetWorldLocation.NewLocation`、`K2_SetWorldRotation.NewRotation`、`SetWorldScale3D.NewScale`，执行入口/出口按实际目录的 `execute`/`then` 连接。缩放输入不是 `NewScale3D`。原始 rotator 数组默认值不支持，旋转由 MakeRotator.ReturnValue 连接。
- 运行验证样例使用 bSweep=false、bTeleport=true；碰撞扫掠/物理行为尚未验证。执行顺序由执行边确定；纯 getter 的取值时间由其消费节点确定，不由 nodes 数组位置决定。

实际样例：[根组件位置/旋转/缩放](../Tests/BlueprintComponents/RootMotion.json)、[只改参数](../Tests/BlueprintComponents/RootChanged.json)、[读取位置再加偏移的不同拓扑](../Tests/BlueprintComponents/RootOffset.json)。这些为本地项目测试输入，不是外部 AI 原始输出。使用时生成新的 requestId 与 assetPath，其他变化由节点/默认值/连接表达，无样例专用生成器。

不支持任意 Actor/组件来源、创建自定义组件、对象变量、cast/reroute 上下文、宏/委托/动态引脚。旧 699/340 摘要不授予新根组件能力，不能静默替换摘要。导出仍要求普通 Actor 蓝图及原生默认脚手架；自定义根组件属性不允许被悄悄忽略。缺能力时明确报告缺口，未执行的 UI、运行和交付步骤不写通过。
