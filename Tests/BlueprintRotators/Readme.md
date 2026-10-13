# 旋转成员和默认值测试

本目录包含 22 项构图或拒绝用例。这里记录输入与预期，实际结果由每次运行产生的报告决定。运行环境与通用规则见[开发测试说明](../TestPlan.md)。

## 检查内容

输入中的 rotator 是恰含 `pitch/yaw/roll` 的对象，单位为度，分量必须是有限数字且绝对值不超过 1e12。`RuntimeCases.json` 中的 expectedRotation 则使用 `[Pitch,Yaw,Roll]` 数组。

- RotatorRead：Actor 旋转为 `[15,40,-25]`；RotatorChanged 为 `[-12.5,72.25,8.75]`。
- RotatorStore：写入后读取 `[-20,65,35]`；RotatorReordered 先应用初值，再写变量，最终保持 `[15,40,-25]`。
- RotatorDirect：通过未连输入默认值得到 `[22.5,-55,12.25]`。
- RotatorStoreDefault：设置成员后得到 `[5.5,-35.25,-10.75]`。
- RotatorShuffled：数组乱序不改变 Store 的结构、布局和结果。
- 拒绝数组或字符串冒充 rotator、缺轴、多轴、布尔或过大分量以及错误类型连接。

消息 `Rotator applied` 应恰好出现一次并归属于测试 Actor。通常位置保持零、缩放为一，具体以运行用例为准。

## 文件与运行

- [Cases.json](Cases.json)：构图与拒绝用例。
- [RuntimeCases.json](RuntimeCases.json)：运行预期。

构图用例由 [RunGraphRegression.py](../../Scripts/RunGraphRegression.py) 的 `--fixtures Tests/BlueprintRotators` 选择；提供实际节点目录、匹配插件和新的运行名称。包含目录或资源摘要占位符的模板不能直接粘贴生成，固定摘要的拒绝或兼容用例应保留原值。

运行检查使用 [RunBlueprintMotionChecks.py](../../Scripts/RunBlueprintMotionChecks.py)，消费本组构图结果和对应专用工程。先确认前置生成与保存结果完整，再检查实际变换和消息。
