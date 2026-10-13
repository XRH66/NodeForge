# 材质空间输入测试

本目录包含 27 项构图或拒绝用例。这里记录输入与预期，实际结果由每次运行产生的报告决定。运行环境与通用规则见[开发测试说明](../TestPlan.md)。

## 检查内容

- WorldBands/Changed/Shuffled：世界位置、时间和颜色参数；乱序不改变结构和布局。
- WorldHeight：世界 Z 驱动颜色；ObjectLocal/Relative：世界位置减物体中心，并检查原点枚举。
- ViewFacing/CameraDistance：法线、视角与距离的组合。
- NormalMotion：顶点法线、时间波形与 WorldPositionOffset。
- 拒绝未知、数值或 MAX 枚举、错误字段和端口；PixelNormalWS 用于顶点或 Normal 输出应编译失败且不保存。

WorldBands 与 NormalMotion 的画面配置分别检查时间、参数和可见性。像素发生变化不等于对象可见，也不等于美术效果合格；位移场景还要检查对象是否被地面遮挡。

## 文件与运行

- [Cases.json](Cases.json)：构图与拒绝用例。
- [RuntimeCases.json](RuntimeCases.json)：运行预期。
- [VisualConfig.json](VisualConfig.json)：画面检查配置。
- [NormalVisualConfig.json](NormalVisualConfig.json)：位移画面检查配置。

构图用例由 [RunGraphRegression.py](../../Scripts/RunGraphRegression.py) 的 `--fixtures Tests/MaterialSpatial` 选择；提供实际节点目录、匹配插件和新的运行名称。包含目录或资源摘要占位符的模板不能直接粘贴生成，固定摘要的拒绝或兼容用例应保留原值。

运行检查使用 [RunBlueprintMotionChecks.py](../../Scripts/RunBlueprintMotionChecks.py)，消费本组构图结果和对应专用工程。先确认前置生成与保存结果完整，再检查实际变换和消息。
