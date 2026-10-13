# 三方向水波材质示例测试

[WaterWaves.json](WaterWaves.json)是一个 52 节点、61 条边的 NodeForgeGraph 输入。它用 DotProduct、Cosine、标量参数、法线和 Fresnel 组合三组不同方向、频率与速度的波纹，不依赖贴图或材质函数。

材质使用浅蓝色半透明 Surface、DefaultLit 和 SurfaceForwardShading。它没有折射、泡沫、几何浪高或流体物理模拟，是普通数学节点组合。

## 可以调整的参数

Scale、Speed、WaveStrength、Roughness、Opacity、Specular 控制尺度、速度、强度和表面属性；WaterColor 与 EdgeColor 控制颜色。

默认 WaveStrength=0.12 的波纹较弱。[较强参数示例](../WaterWavesVisible/WaterWavesVisible.json)调整了参数、请求编号和目标路径，节点拓扑保持一致。

## 检查方法

按[开发测试说明](../TestPlan.md)准备专用工程、实际目录与匹配插件。生成后检查编译、保存和独立重开，再在固定场景中比较时间与参数变化。

像素差非零只能说明图像发生变化。还应实际查看波纹是否清晰、颜色是否符合预期；这个示例不设定普遍适用的美术质量标准。
