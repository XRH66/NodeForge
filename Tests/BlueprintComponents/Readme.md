# 自身根组件测试

本目录包含 16 项构图或拒绝用例。这里记录输入与预期，实际结果由每次运行产生的报告决定。运行环境与通用规则见[开发测试说明](../TestPlan.md)。

## 检查内容

- RootMotion、RootChanged：设置根组件的位置、旋转和缩放，再读取实际 Actor 变换；默认值变化应改变运行结果。
- RootShuffled：反转节点和连线数组后，图结构、布局和运行结果应保持一致。
- RootOffset：先设置位置，再读取并增加偏移，最终位置应为 `[15,20,32]`。
- 拒绝缺失 self、对象默认值、错误对象来源、多源 self、错误引脚、未知函数和同名目标。

组件 self 必须直接来自同图的 `Actor.K2_GetRootComponent.ReturnValue`。这些用例使用 `bSweep=false`，不覆盖碰撞扫掠、外部 Actor 或自定义组件。

## 文件与运行

- [Cases.json](Cases.json)：构图与拒绝用例。
- [RuntimeCases.json](RuntimeCases.json)：运行预期。

构图用例由 [RunGraphRegression.py](../../Scripts/RunGraphRegression.py) 的 `--fixtures Tests/BlueprintComponents` 选择；提供实际节点目录、匹配插件和新的运行名称。包含目录或资源摘要占位符的模板不能直接粘贴生成，固定摘要的拒绝或兼容用例应保留原值。

运行检查使用 [RunBlueprintMotionChecks.py](../../Scripts/RunBlueprintMotionChecks.py)，消费本组构图结果和对应专用工程。先确认前置生成与保存结果完整，再检查实际变换和消息。
