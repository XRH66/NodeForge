# 材质条件选择与去色测试

本目录包含 19 项构图或拒绝用例。这里记录输入与预期，实际结果由每次运行产生的报告决定。运行环境与通用规则见[开发测试说明](../TestPlan.md)。

## 检查内容

- ColorLimits/Changed/Defaults：Max、Min 的属性和未连输入默认值。
- ColorSelect/Changed/Topology/Shuffled：条件分支、冷热颜色互换、连线变化和数组乱序。
- ColorDesaturate/Changed：去色参数与加权配置。
- ColorPulse：Time/Sine→Max/Min→If→Desaturation 的组合。
- 拒绝非法字段、输入与连接以及同名目标；固定目录摘要用例检查相应能力范围。

当前 Desaturation 必须显式连接 Fraction。编译和保存检查不包含材质实际显示效果。

## 文件与运行

- [Cases.json](Cases.json)：构图与拒绝用例。
- [RuntimeCases.json](RuntimeCases.json)：运行预期。

构图用例由 [RunGraphRegression.py](../../Scripts/RunGraphRegression.py) 的 `--fixtures Tests/MaterialColorSelection` 选择；提供实际节点目录、匹配插件和新的运行名称。包含目录或资源摘要占位符的模板不能直接粘贴生成，固定摘要的拒绝或兼容用例应保留原值。

运行检查使用 [RunBlueprintMotionChecks.py](../../Scripts/RunBlueprintMotionChecks.py)，消费本组构图结果和对应专用工程。先确认前置生成与保存结果完整，再检查实际变换和消息。
