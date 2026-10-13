# 材质节点组合测试

本目录包含 15 项构图或拒绝用例。这里记录输入与预期，实际结果由每次运行产生的报告决定。运行环境与通用规则见[开发测试说明](../TestPlan.md)。

## 检查内容

- TextureParameter、Tint、Shuffle：纹理参数、增减节点、连线及参数变化，数组乱序不改变结果。
- FractionMath：Frac、Floor、Saturate；VectorProducts：CrossProduct、DotProduct。
- FacingColor、FacingDefaults：Fresnel 及其允许的输入默认值。
- WaterFacing：在普通材质图中增加视角颜色插值。
- 拒绝缺少 Texture 或 ParameterName、未允许的属性、错误输入和不匹配目录摘要。

所有输入通过通用构图器处理，不要求插件存在水面或特定效果的专用入口。

## 文件与运行

- [Cases.json](Cases.json)：构图与拒绝用例。

构图用例由 [RunGraphRegression.py](../../Scripts/RunGraphRegression.py) 的 `--fixtures Tests/MaterialFamilies` 选择；提供实际节点目录、匹配插件和新的运行名称。包含目录或资源摘要占位符的模板不能直接粘贴生成，固定摘要的拒绝或兼容用例应保留原值。
