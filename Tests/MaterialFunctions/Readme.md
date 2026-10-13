# 材质函数调用测试

本目录包含 16 项构图或拒绝用例。这里记录输入与预期，实际结果由每次运行产生的报告决定。运行环境与通用规则见[开发测试说明](../TestPlan.md)。

## 检查内容

- 正例涵盖原生可选输入、显式三输入、数组乱序、时间函数、Overlay、拓扑交换、双函数串联和速度参数。
- 负例涵盖不存在的输入输出、过期或缺失文件摘要、错误资源类型、额外属性和必需输入未连接。

运行前通过实际插件读取函数，给 RunGraphRegression 提供 `--function-signature`。签名报告须匹配所选 DLL，并包含实际 GUID、引脚顺序、默认策略及资源摘要；不能使用未绑定函数的空引脚列表。

画面检查使用 VisualConfig，按固定场景比较时间和参数变化。编译通过不替代这一步。

## 文件与运行

- [Cases.json](Cases.json)：构图与拒绝用例。
- [VisualConfig.json](VisualConfig.json)：画面检查配置。

构图用例由 [RunGraphRegression.py](../../Scripts/RunGraphRegression.py) 的 `--fixtures Tests/MaterialFunctions` 选择；提供实际节点目录、匹配插件和新的运行名称。包含目录或资源摘要占位符的模板不能直接粘贴生成，固定摘要的拒绝或兼容用例应保留原值。
