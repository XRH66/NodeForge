# 字符串变换与分支测试

本目录包含 17 项构图或拒绝用例。这里记录输入与预期，实际结果由每次运行产生的报告决定。运行环境与通用规则见[开发测试说明](../TestPlan.md)。

## 检查内容

| 用例 | 预期消息 |
| --- | --- |
| TransformCompose | `ABC/efg` |
| TransformComposeReordered | `QR:tuvw` |
| TransformUpperOnly | `MIXED/ABC` |
| TransformLowerOnly | `mixed/abc` |
| TransformContainsSensitive | `Contains Miss` |
| TransformContainsInsensitive | `Contains Match` |
| TransformLengthEnough | `Length Enough` |
| TransformLengthShort | `Length Short` |
| TransformZeroSlice | `/` |
| TransformLongSlice | `abc` |

这些用例组合 Left、Right、ToUpper、ToLower、Contains、Len 和 Branch，检查实际计算决定的唯一消息。默认位置与旋转为零，缩放为一。

负例覆盖不允许该函数的固定目录摘要、函数名和引脚大小写错误、小数 Count、未知属性及同名目标。TransformCollision 依赖先成功生成 TransformCompose。

## 文件与运行

- [Cases.json](Cases.json)：构图与拒绝用例。
- [RuntimeCases.json](RuntimeCases.json)：运行预期。

构图用例由 [RunGraphRegression.py](../../Scripts/RunGraphRegression.py) 的 `--fixtures Tests/BlueprintStringTransforms` 选择；提供实际节点目录、匹配插件和新的运行名称。包含目录或资源摘要占位符的模板不能直接粘贴生成，固定摘要的拒绝或兼容用例应保留原值。

运行检查使用 [RunBlueprintMotionChecks.py](../../Scripts/RunBlueprintMotionChecks.py)，消费本组构图结果和对应专用工程。先确认前置生成与保存结果完整，再检查实际变换和消息。
