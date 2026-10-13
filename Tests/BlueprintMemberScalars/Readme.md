# 整数与名称变量测试

本目录包含 21 项构图或拒绝用例。这里记录输入与预期，实际结果由每次运行产生的报告决定。运行环境与通用规则见[开发测试说明](../TestPlan.md)。

## 检查内容

- MemberCount/Low/Shuffled：读取、修改并再次读取整数成员，检查分支、旋转和缩放。
- MemberState/None：读取和设置 name 成员，检查字符串转换及规范的 `None` 值。
- MemberLimits：保存并重新读取 int32 的两个边界值。
- 拒绝整数小数、越界、bool 混用、空或过长的 name、未声明变量、错误引脚、非法连接和同名目标。

成员默认值应从编译后的 CDO 读取；运行实例的当前值另行检查。

## 文件与运行

- [Cases.json](Cases.json)：构图与拒绝用例。
- [RuntimeCases.json](RuntimeCases.json)：运行预期。

构图用例由 [RunGraphRegression.py](../../Scripts/RunGraphRegression.py) 的 `--fixtures Tests/BlueprintMemberScalars` 选择；提供实际节点目录、匹配插件和新的运行名称。包含目录或资源摘要占位符的模板不能直接粘贴生成，固定摘要的拒绝或兼容用例应保留原值。

运行检查使用 [RunBlueprintMotionChecks.py](../../Scripts/RunBlueprintMotionChecks.py)，消费本组构图结果和对应专用工程。先确认前置生成与保存结果完整，再检查实际变换和消息。
