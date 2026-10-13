# AI 使用指南

安装完成后，AI 可以通过 MCP 查询 NodeForge 规则、读取资源、编辑 JSON 草稿，以及生成新的蓝图或材质。本手册适用于 NodeForge v0.2.0。

第一次连接请先读 [安装与连接](../User/AiInstallation.md)，然后按 [调用流程](QuickStart.md)完成一次简单操作。

| 任务 | 文档 | MCP 文档 ID |
| --- | --- | --- |
| 连接工程、准备输入与检查保存结果 | [准备工作](Preparation.md) | `AiPreparation` |
| 从零生成材质或蓝图 | [调用示例](TaskRecipes.md) | `AiTaskRecipes` |
| 查询节点、函数和参数 | [查找节点](NodeBulk.md) | `AiNodeBulk` |
| 查找贴图、函数和材质 | [资源查询](Resources.md) | `AiResources` |
| 读取参数、导出已有图 | [资源读取与导出](AssetRead.md) | `AiAssetRead` |
| 修改副本、预览差异 | [编辑副本](CopyPreview.md) | `AiCopyPreview` |
| 设置材质和蓝图变量属性 | [资产设置](AssetSettings.md) | `AiAssetSettings` |
| 查询参数和返回值 | [API 参考](ApiReference.md) | `AiApiReference` |
| 处理失败或不确定结果 | [结果与恢复](ResultsAndRecovery.md) | `AiResultsAndRecovery` |

通过工具阅读时，先调用 `nodeforge_docs_index`，再将索引返回的 `pageId` 传给 `nodeforge_docs_read`。文档读取不需要启动 UE；操作工程需要先连接编辑器。

支持范围见 [功能说明](../Reference/FeatureMap.md)、[材质节点](../Reference/MaterialNodeIndex.md)和 [蓝图节点](../Reference/BlueprintNodeIndex.md)。请使用目录中的准确类型、引脚和参数，不要按显示名称猜测。
