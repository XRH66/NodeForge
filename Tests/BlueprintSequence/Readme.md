# Sequence 测试

本目录包含 24 项构图或拒绝用例。这里记录输入与预期，实际结果由每次运行产生的报告决定。运行环境与通用规则见[开发测试说明](../TestPlan.md)。

## 检查内容

- SequenceTwo/Three：按输出顺序写入或追加文本，预期为 `A` 或 `AB`。
- SequenceChanged：改变文本得到 `XY`；SequenceSwapped：交换执行连接后得到 `A`。
- SequenceShuffled：节点与边乱序后仍为 `AB`，布局应一致。
- SequenceEight：检查 8 输出上限，预期 `1234567`。
- SequenceNested：嵌套顺序执行，预期 `ABC`。
- 拒绝错误数量、类型或缺失配置、未知输出、执行默认值、环路、执行输出扇出和同名目标。

执行次序由 Sequence 输出与连线决定，不由 JSON 节点数组顺序决定。

## 文件与运行

- [Cases.json](Cases.json)：构图与拒绝用例。
- [RuntimeCases.json](RuntimeCases.json)：运行预期。

构图用例由 [RunGraphRegression.py](../../Scripts/RunGraphRegression.py) 的 `--fixtures Tests/BlueprintSequence` 选择；提供实际节点目录、匹配插件和新的运行名称。包含目录或资源摘要占位符的模板不能直接粘贴生成，固定摘要的拒绝或兼容用例应保留原值。

运行检查使用 [RunBlueprintMotionChecks.py](../../Scripts/RunBlueprintMotionChecks.py)，消费本组构图结果和对应专用工程。先确认前置生成与保存结果完整，再检查实际变换和消息。
