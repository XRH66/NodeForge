# 旋转默认值精度测试

本目录包含 10 项构图或拒绝用例。这里记录输入与预期，实际结果由每次运行产生的报告决定。运行环境与通用规则见[开发测试说明](../TestPlan.md)。

## 检查内容

本组复用[旋转测试](../BlueprintRotators/Readme.md)的输入，选择成员读取、写入、拓扑变化、数组乱序及错误类型连接等相关用例。

默认值按 Pitch/Yaw/Roll 的各分量比较，不能因为四元数等价就接受默认值被减去 360 度。原生 `NodeForge.RotatorDefaultGuards` 还检查大角度 CDO 与未连引脚的修改、拒绝和恢复。

运行时 Actor 欧拉角可能发生等价变化，运行结果与资产默认值应分别检查。

## 文件与运行

- [Cases.json](Cases.json)：构图与拒绝用例。
- [RuntimeCases.json](RuntimeCases.json)：运行预期。

构图用例由 [RunGraphRegression.py](../../Scripts/RunGraphRegression.py) 的 `--fixtures Tests/BlueprintRotatorPrecision` 选择；提供实际节点目录、匹配插件和新的运行名称。包含目录或资源摘要占位符的模板不能直接粘贴生成，固定摘要的拒绝或兼容用例应保留原值。

运行检查使用 [RunBlueprintMotionChecks.py](../../Scripts/RunBlueprintMotionChecks.py)，消费本组构图结果和对应专用工程。先确认前置生成与保存结果完整，再检查实际变换和消息。
