# 蓝图函数测试

本目录包含 19 项构图或拒绝用例。这里记录输入与预期，实际结果由每次运行产生的报告决定。运行环境与通用规则见[开发测试说明](../TestPlan.md)。

## 检查内容

- CountTrue/CountFalse/CountShuffled：整数相加、阈值分支、旋转与缩放，以及数组乱序后的等价结果。
- TextKnown/TextAbsent：名称转换、切片、包含判断、选择、长度和拼接，预期消息为 `NodeForge9` 或 `missing7`。
- VectorBasis：读取自身方向，组合叉积、点积、向量计算和显式数值转换。
- ScalarSelection：整数计算、比较、布尔选择、浮点计算和限幅。
- 拒绝分数或越界整数、错误名称、未知函数和引脚、双源输入、外部对象及同名资产。

运行期旋转数组按 Pitch/Yaw/Roll，位置和缩放按 X/Y/Z。

## 文件与运行

- [Cases.json](Cases.json)：构图与拒绝用例。
- [RuntimeCases.json](RuntimeCases.json)：运行预期。

构图用例由 [RunGraphRegression.py](../../Scripts/RunGraphRegression.py) 的 `--fixtures Tests/BlueprintFamilies` 选择；提供实际节点目录、匹配插件和新的运行名称。包含目录或资源摘要占位符的模板不能直接粘贴生成，固定摘要的拒绝或兼容用例应保留原值。

运行检查使用 [RunBlueprintMotionChecks.py](../../Scripts/RunBlueprintMotionChecks.py)，消费本组构图结果和对应专用工程。先确认前置生成与保存结果完整，再检查实际变换和消息。
