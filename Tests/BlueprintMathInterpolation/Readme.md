# 数学与插值测试

本目录包含 23 项构图或拒绝用例。这里记录输入与预期，实际结果由每次运行产生的报告决定。运行环境与通用规则见[开发测试说明](../TestPlan.md)。

## 检查内容

- MathRange：Abs→Min→Max→Lerp，预期 Actor X=11，消息为 `math=11.0`。
- MathChanged/Topology/Reduced：改变参数或连接后，X 分别为 16、6.75、14；MathShuffled 应与 MathRange 等价。
- MathDegrees：角度制正余弦参与位置计算，检查度与弧度没有混用。
- MathRotation/Shortest/Linear/Member：检查旋转插值、最短路径选项和成员输入；170 到 -150 度的两种插值预期分别为 -170 和 10 度。
- 拒绝默认值类型错误、引脚大小写错误、未知字段、非法连接、连接与默认值并存及同名目标。

构图后由运行器读取真实 Actor 位置、旋转和消息，不能只看编译结果。

## 文件与运行

- [Cases.json](Cases.json)：构图与拒绝用例。
- [RuntimeCases.json](RuntimeCases.json)：运行预期。

构图用例由 [RunGraphRegression.py](../../Scripts/RunGraphRegression.py) 的 `--fixtures Tests/BlueprintMathInterpolation` 选择；提供实际节点目录、匹配插件和新的运行名称。包含目录或资源摘要占位符的模板不能直接粘贴生成，固定摘要的拒绝或兼容用例应保留原值。

运行检查使用 [RunBlueprintMotionChecks.py](../../Scripts/RunBlueprintMotionChecks.py)，消费本组构图结果和对应专用工程。先确认前置生成与保存结果完整，再检查实际变换和消息。
