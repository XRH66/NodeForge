# 字符串和名称完整性测试

本目录包含 17 项构图或拒绝用例。这里记录输入与预期，实际结果由每次运行产生的报告决定。运行环境与通用规则见[开发测试说明](../TestPlan.md)。

## 检查内容

- StringPrintMixed/Lower：分别保留 `AbC` 与 `abc`；StringMember 读取成员中的 `AbC`。
- StringNameNone：name 成员转换后必须为规范的 `None`。
- StringCompose：拼接结果为 `AbC/abc`。
- WrongOwner/Member/Class/Parent/Type/Variable：只改变对应标识的大小写，检查是否拒绝。
- NameLowerNone/UpperNone：拒绝 `none` 与 `NONE`；WrongPin 检查 `InString` 不能写成 `inString`。
- StringCollision：先生成 StringPrintMixed，再用新请求指向相同资产路径，必须拒绝覆盖。

消息比较区分大小写，不能通过大小写折叠或模糊匹配取得通过结果。

## 文件与运行

- [Cases.json](Cases.json)：构图与拒绝用例。
- [RuntimeCases.json](RuntimeCases.json)：运行预期。

构图用例由 [RunGraphRegression.py](../../Scripts/RunGraphRegression.py) 的 `--fixtures Tests/BlueprintStringIdentity` 选择；提供实际节点目录、匹配插件和新的运行名称。包含目录或资源摘要占位符的模板不能直接粘贴生成，固定摘要的拒绝或兼容用例应保留原值。

运行检查使用 [RunBlueprintMotionChecks.py](../../Scripts/RunBlueprintMotionChecks.py)，消费本组构图结果和对应专用工程。先确认前置生成与保存结果完整，再检查实际变换和消息。
