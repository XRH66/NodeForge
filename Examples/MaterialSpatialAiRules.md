# 材质空间位置与法线

本页说明 NodeForge v0.1.0 的 Graph JSON v2 写法。先读取[通用规则](GraphAiRules.md)和[当前节点目录](../Docs/Versions/v0.1.0/Catalog.json)，从实际目录取得 `catalogDigest`、节点类、属性和引脚。

类路径前缀为 `/Script/Engine.MaterialExpression`。

| 类后缀 | 用途与注意事项 |
| --- | --- |
| `WorldPosition` | 世界位置；`WorldPositionShaderOffset` 使用目录列出的枚举。输出 0/1/2 分别为 XYZ/XY/Z |
| `ObjectPositionWS` | 物体位置；`OriginType` 为目录允许的绝对或摄像机相对原点 |
| `CameraPositionWS` | 摄像机世界位置 |
| `CameraVectorWS` | 像素指向摄像机的方向，不能用于顶点 WorldPositionOffset |
| `VertexNormalWS` | 顶点的世界空间法线 |
| `PixelNormalWS` | 像素的世界空间法线，不能连接到 Normal 或顶点 WorldPositionOffset |

输出编号在 JSON 中写作字符串，例如 `"0"`。枚举也使用准确字符串，不能改成数字或中文别名。

做相减、点积或比较前，确认两个向量位于同一坐标空间和同一原点。世界空间向量不会自动转换为切线空间；绝对世界位置与摄像机相对位置也不能直接混算。

可以使用位置分量、Time、参数和数学节点组合条纹、相对中心颜色或视角变化效果。JSON 只描述节点与连线，插件没有专门的“空间效果”开关。

材质着色阶段可能进一步限制某些连接。输入检查后仍需编译并查看实际画面。

结构示例见[空间节点测试数据](../Tests/MaterialSpatial/Readme.md)。

本文链接的测试 JSON 用于阅读结构，可能含目录摘要或资源占位值，不能保证原样导入当前版本。请核对当前规则、填写真实资源，并使用新的请求编号和资产名。第一次使用推荐插件包附带的 `Examples/FirstMaterial.json`，见[入门教程](../Docs/GettingStarted.md)。
