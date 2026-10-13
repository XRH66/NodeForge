# 大数值默认值测试

本目录包含 7 项构图或拒绝用例。这里记录输入与预期，实际结果由每次运行产生的报告决定。运行环境与通用规则见[开发测试说明](../TestPlan.md)。

## 检查内容

本组基于 GraphV2 和 MathRotation 的结构，检查大数值在默认值文本、编译和保存重开中的一致性。

LargeFloatPin 检查 K2 引脚文本；函数原生 float 仍可能量化。LargeMaterialFloat 检查 HDR 标量 999999.9 以 float32 保存为 999999.875 的实际结果。

这些数值用于测试存储精度，不是推荐的材质效果参数。该组不包含新节点类型，也不判断画面、UI 或打包。

## 文件与运行

- [Cases.json](Cases.json)：构图与拒绝用例。

构图用例由 [RunGraphRegression.py](../../Scripts/RunGraphRegression.py) 的 `--fixtures Tests/NightAuditCore` 选择；提供实际节点目录、匹配插件和新的运行名称。包含目录或资源摘要占位符的模板不能直接粘贴生成，固定摘要的拒绝或兼容用例应保留原值。
