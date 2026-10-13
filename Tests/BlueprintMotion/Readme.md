# Actor 位移与向量测试

本目录包含 19 项构图或拒绝用例。这里记录输入与预期，实际结果由每次运行产生的报告决定。运行环境与通用规则见[开发测试说明](../TestPlan.md)。

## 检查内容

- 正例包括直接位置、MakeVector、向量成员读取与赋值、BreakVector→MakeVector、位置 Getter、文本转换和零向量。
- MotionShuffled 与 MotionMade 比较图语义和布局，节点数组顺序变化不应改变结果。
- 负例覆盖向量长度、分量类型、数值范围、未知函数或引脚、外部 self 和不兼容连接。

运行检查在新编辑器进程中进入 PIE，同时核对 Actor 的实际位置与归属正确的消息。打印一条坐标文字不能代替读取真实位置。

## 文件与运行

- [Cases.json](Cases.json)：构图与拒绝用例。
- [RuntimeCases.json](RuntimeCases.json)：运行预期。

构图用例由 [RunGraphRegression.py](../../Scripts/RunGraphRegression.py) 的 `--fixtures Tests/BlueprintMotion` 选择；提供实际节点目录、匹配插件和新的运行名称。包含目录或资源摘要占位符的模板不能直接粘贴生成，固定摘要的拒绝或兼容用例应保留原值。

运行检查使用 [RunBlueprintMotionChecks.py](../../Scripts/RunBlueprintMotionChecks.py)，消费本组构图结果和对应专用工程。先确认前置生成与保存结果完整，再检查实际变换和消息。
