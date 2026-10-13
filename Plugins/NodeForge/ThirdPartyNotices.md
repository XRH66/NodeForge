# 第三方代码与许可

NodeForge 使用以下 MIT 项目的部分代码。对应许可全文保存在插件的 `Licenses` 目录，源码文件也保留了相关声明。

## ZiggyMar / unreal-mcp

来源：[unreal-mcp](https://github.com/ZiggyMar/unreal-mcp)，固定提交 `d1a85396d9c62328521256005fe7c420725651cc`。Copyright (c) 2026 Ziggy。

许可：[ZiggyMarMIT.txt](Licenses/ZiggyMarMIT.txt)。

使用范围：

- `MCPCommandHandler.cpp` 的 `AddNodeCore`、`CompileMessagesToJson`、`HandleCompileBlueprint` 与 `SaveAssetPackage` 中的节点创建、编译诊断及保存处理，用于 `NodeForgePrototype.cpp`。
- `MCPNodeCatalog.cpp` 的 `ShouldIncludeFunction` 过滤，以及 `AddNodeCore` 自身变量绑定、`SetFromFunction` 片段，用于 `NodeForgeBlueprintGraph.cpp`。
- `HandleCreateMaterialInstance` 的工厂创建与父材质设置片段，用于 `NodeForgeMaterialInstances.cpp`。

这些代码按 NodeForge 的 JSON 输入、同名资产保护与结果报告进行适配。材质实例先在暂存包中创建，再由 NodeForge 保存。上游网络服务、SourceControl、批回滚和内存对象改名功能没有包含在内。

## ChiR24 / Unreal_mcp

来源：[Unreal_mcp](https://github.com/ChiR24/Unreal_mcp)，固定提交 `591d3b0ea3c46a2a1926dbcafc225afecb6a75b3`。

完整版权与许可：[ChiR24MIT.txt](Licenses/ChiR24MIT.txt)。

`NodeForgeMaterialGraph.cpp` 的 `ConnectExact` 使用了 `MaterialAuthoring/Connections/McpAutomationBridge_MaterialAuthoringHandlersConnectNodes.cpp` 中 `HandleConnectNodes` 的输出索引检查与 `Wire` 连线核心。适配后保留精确输出与原生连线，移除了别名、默认引脚回退、网络和宏依赖。

## PRQELT / Autonomix

来源：[Autonomix](https://github.com/PRQELT/Autonomix)，固定提交 `47a46ae8cc3f00230edceeb33f0d4f9d80cb185b`。Copyright (c) 2025 Autonomix。

许可：[AutonomixMIT.txt](Licenses/AutonomixMIT.txt)。

`NodeForgeGraphLayout.cpp` 的 Kahn 最长路径分层算法基于 `Source/AutonomixActions/Private/Blueprint/AutonomixBlueprintActions.cpp` 的 `FAutonomixBlueprintActions::AutoLayoutNodes`。NodeForge 增加了数据依赖、稳定排序、节点尺寸、锚点与分支排序处理，未包含该项目的其他模块。

## Unreal Engine

材质表达式、蓝图节点、反射、编译和资产保存通过 Unreal Engine 原生 API 实现。NodeForge 的协议检查与适配代码不改变引擎本身的许可；引擎代码及资源仍受其原有许可约束。
