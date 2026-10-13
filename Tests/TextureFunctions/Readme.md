# 贴图对象与材质函数测试

本目录包含 19 项构图或拒绝用例。这里记录输入与预期，实际结果由每次运行产生的报告决定。运行环境与通用规则见[开发测试说明](../TestPlan.md)。

## 检查内容

- WorldTexture/Parameter/Shuffled：TextureObject 或 TextureObjectParameter 连接 WorldAlignedTexture 的实际 T2d 输入，读取 XYZ Texture 输出。
- WorldInstance：对实际保存父材质中的 SurfaceTexture 做 Texture2D 覆盖。
- RotatingTexture：TextureObject→TextureSample，UV 经 Rotator，Time 与 SpinSpeed 控制旋转。
- RadialMask/Changed：Distance、SmoothStep 和 OneMinus 组合柔边遮罩。
- StepBands：UV、Frac、Step 和 Lerp 组合条纹。
- TextureNumericFail：纹理对象误接数值颜色输出应在 compile 阶段失败且不保存。
- ObjectIgnoredUv：TextureObjectParameter 不消费的继承 UV 输入必须拒绝，不能默默丢弃连接。

运行器需要 `--function-signature` 指定同插件的实际函数检查结果。实例父 SHA 也从实际保存文件取得；画面检查与蓝图运行检查分别使用对应配置。

## 文件与运行

- [Cases.json](Cases.json)：构图与拒绝用例。
- [RuntimeCases.json](RuntimeCases.json)：运行预期。
- [VisualConfig.json](VisualConfig.json)：画面检查配置。

构图用例由 [RunGraphRegression.py](../../Scripts/RunGraphRegression.py) 的 `--fixtures Tests/TextureFunctions` 选择；提供实际节点目录、匹配插件和新的运行名称。包含目录或资源摘要占位符的模板不能直接粘贴生成，固定摘要的拒绝或兼容用例应保留原值。

运行检查使用 [RunBlueprintMotionChecks.py](../../Scripts/RunBlueprintMotionChecks.py)，消费本组构图结果和对应专用工程。先确认前置生成与保存结果完整，再检查实际变换和消息。
