# 材质实例贴图类型测试

本目录包含 7 项构图或拒绝用例。这里记录输入与预期，实际结果由每次运行产生的报告决定。运行环境与通用规则见[开发测试说明](../TestPlan.md)。

## 检查内容

用例包含颜色父材质、法线父材质、同类型贴图替换、显式法线默认值，以及颜色与法线之间的双向错误替换。

模板父摘要为全零，需要先读取实际保存父材质再绑定。跨类型覆盖应在 validation 阶段拒绝，diskState 应为 notWritten；保存或重开成功不能代替采样类型检查。

资源类型与 sampler 以 MaterialParameters 返回的 textureSamplerSignatures 为准，不能根据贴图名称猜测。

## 文件与运行

- [Cases.json](Cases.json)：构图与拒绝用例。

构图用例由 [RunGraphRegression.py](../../Scripts/RunGraphRegression.py) 的 `--fixtures Tests/TextureOverrides` 选择；提供实际节点目录、匹配插件和新的运行名称。包含目录或资源摘要占位符的模板不能直接粘贴生成，固定摘要的拒绝或兼容用例应保留原值。
