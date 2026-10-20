# 查找并使用节点

NodeForge v1.1.2 的目录包含 71 个材质类和 327 个蓝图签名。目录仍标记为 `experimental`，使用时应按具体节点和组合检查支持范围。

## 查找步骤

1. 调用 `nodeforge_docs_index`，阅读 [功能说明](../Reference/FeatureMap.md)。
2. 连接后用 `nodeforge_rules_query` 按材质或蓝图查询任务需要的规则。
3. 用 `nodeforge_catalog_get` 获取目录和 `catalogDigest`。完整报告可通过 `nodeforge_report_read` 分段读取。
4. 从 [材质索引](../Reference/MaterialNodeIndex.md)或 [蓝图索引](../Reference/BlueprintNodeIndex.md)进入具体节点页。
5. 按页面中的准确 class、owner/member、引脚、类型和默认值编写图。

规则查询的 `query` 是最多 256 字符、最多 8 个词的大小写无关 OR 子串筛选，不会自动生成图。不要从中文名称猜引脚，也不要把 float 与 double、Value 与对象引用混用。

## 从节点片段组成完整图

单个节点页给出的是声明片段。完整输入还需要请求 ID、目录摘要、目标、资产设置、节点和连线，参考 [完整示例](../Reference/Examples.md)与 [更多节点示例](NodeBulkExamples.md)。

材质函数先读取实际资源签名和 SHA；Sequence、Switch、成员变量与组件上下文需要满足各自限制。未知属性、动态类型或未支持组合会被拒绝。

## 提交和检查

按 [调用流程](QuickStart.md)载入草稿，重新读取修订，再预检和生成。每次操作使用独立 `callId`。结果不明确时查询原调用，不换目标再次生成。

已有图的导出与新副本使用相同目录，详见 [资源读取与导出](AssetRead.md)。目录变化后重新读取节点定义；只替换 `catalogDigest` 不能让不兼容的图变得可用。
