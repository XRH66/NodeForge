# 纹理与原生函数组合回归

以[Build38实际目录](../../Reports/20261009UsableEditor/TextureFunctionExpansion2/ActualCatalog.json)及[WorldAlignedTexture实际签名](../../Reports/20261009UsableEditor/TextureFunctionExpansion2/WorldAlignedTexture.json)为准，不使用研究fixture接口。

- WorldTexture / WorldParameter / WorldShuffled：TextureObject或TextureObjectParameter连接原生WorldAlignedTexture的真实T2d输入，输出XYZ世界对齐纹理；反转数组后图/布局仍一致。
- WorldInstance：对已保存WorldParameter父材质的SurfaceTexture作实际Texture2D覆盖，父SHA从本轮真实收据绑定，源模板不写成伪历史哈希。
- RotatingTexture：TextureObject→TextureSample，UV经Rotator，Time乘SpinSpeed决定旋转，Tint决定着色；与世界对齐图结构不同。
- RadialMask / RadialChanged：UV与Center的Distance经SmoothStep/OneMinus成柔边遮罩，输入改变圆心、范围和颜色。
- StepBands：UV分量、Frac、Step、两颜色Lerp组成条纹。
- BlueprintFamilyLegacy：以此前699目录数据在新构建生成原整数阈值旋转/缩放图，防止材质扩展破坏旧蓝图能力。

九项验证拒绝与一项编译失败预期分别记录。TextureNumericFail的目标是确认原生texture对象误连EmissiveColor后在compile阶段失败且不保存，不把它称为成功材质。ObjectIgnoredUv确认实际存在却不被UE编译消费的继承引脚被适配器拒绝，而非丢掉连接。

RunGraphRegression.py使用显式--function-signature绑定同DLL实际函数签名；RuntimeCases.json只验受影响蓝图；VisualConfig.json检查旋转纹理真实时间及参数画面。当前终态见[功能文档](../../Docs/MaterialTextureFunctions.md)与[项目状态](../../ProjectStatus.md)。全部使用同一通用构图器，不修改插件即可换数据。
