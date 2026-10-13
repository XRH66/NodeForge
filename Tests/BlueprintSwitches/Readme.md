# 整数与名称 Switch 测试

本目录包含 39 项构图或拒绝用例。这里记录输入与预期，实际结果由每次运行产生的报告决定。运行环境与通用规则见[开发测试说明](../TestPlan.md)。

## 检查内容

- 改变 Selection、分支配置或连线，检查实际 Actor 位置由正确分支决定。
- 覆盖一个、八个分支、负起点和 int32 安全边界。
- 检查 Default、未匹配且无 Default、命中但未连线等情况；未连的匹配分支不能落入 Default。
- 检查 FName 的运行匹配与引脚标识的大小写规则，两者不能混淆。
- 拒绝数量或类型错误、整数溢出、名称重复或保留名、未知引脚、隐藏上下文、默认值与连线并存及同名目标。

未连分支用例还使用独立 Sequence 消息证明事件确实执行过，不能把没有打印消息当作通过。

## 文件与运行

- [Cases.json](Cases.json)：构图与拒绝用例。
- [RuntimeCases.json](RuntimeCases.json)：运行预期。

构图用例由 [RunGraphRegression.py](../../Scripts/RunGraphRegression.py) 的 `--fixtures Tests/BlueprintSwitches` 选择；提供实际节点目录、匹配插件和新的运行名称。包含目录或资源摘要占位符的模板不能直接粘贴生成，固定摘要的拒绝或兼容用例应保留原值。

运行检查使用 [RunBlueprintMotionChecks.py](../../Scripts/RunBlueprintMotionChecks.py)，消费本组构图结果和对应专用工程。先确认前置生成与保存结果完整，再检查实际变换和消息。
