# 创建材质

NodeForge 可以创建普通 Surface 材质。先查询[材质节点列表](MaterialNodeIndex.md)，选择需要的节点，再按[JSON 编写说明](GraphAuthoring.md)填写节点、连线和材质设置。第一次使用可以从[完整示例](Examples.md)中的颜色材质开始。

## 材质设置

| 设置 | 支持的值或条件 |
| --- | --- |
| `domain` | `Surface` |
| `blend` | `Opaque`、`Translucent`、`Masked` |
| `shading` | `Unlit`、`DefaultLit`、`ClearCoat` |
| `twoSided` | `true` 或 `false` |
| `refractionMethod` | `None`（省略默认）或 `IndexOfRefraction`（Translucent/DefaultLit） |
| `lighting` | Translucent 材质必须显式填写 `SurfaceForwardShading` |
| `opacityMaskClipValue` | Masked 材质必须显式填写，范围为 0–1 |
| `ditherOpacityMask`、`ditheredLodTransition` | 布尔值；实际显示效果需在 UE 中查看 |
| `allowNegativeEmissiveColor` | 设为 `true` 时，着色模型必须为 Unlit |

材质输出支持 BaseColor、EmissiveColor、Metallic、Normal、Opacity、OpacityMask、Roughness、Specular、WorldPositionOffset、ClearCoat、ClearCoatRoughness、Anisotropy、Tangent 和 Refraction，但只能连接当前设置下启用的输入。例如，Unlit 通常连接 EmissiveColor，Opaque 不能连接 Opacity。完整字段用法见[资产设置](../Ai/AssetSettings.md)。

## 节点与连线

1. 从节点列表找到需要的节点，使用页面中的类名和允许属性。
2. 使用节点页列出的输入名称；输出用数字字符串，例如 `"0"`，不能改成 `RGB`、`"00"` 或数字 `0`。
3. 把所有表达式连接到有效的材质输出。没有用途的孤立节点或分支会被拒绝。
4. 先预检，再生成。在 UE 中查看编译结果与材质效果。

属性省略时使用目录记录的 UE 默认值。`defaults` 只支持能够唯一对应到数值输入属性的项；已经连线的输入不能同时设置默认值。同一材质中的参数节点必须使用不同的参数名。

材质最多包含 256 个节点和 1,024 条边，至少需要一个节点和一条输出连线。这是输入大小限制；复杂图仍受节点类型、着色阶段和编译规则约束。

## 引用贴图和材质函数

贴图必须是 `/Engine/` 或工程允许读取目录中已经保存的普通 Texture2D。当前不支持导入新贴图或使用虚拟纹理。省略采样器设置时由 UE 推断；显式设置时必须与贴图匹配。

使用 `TextureObjectParameter` 时不要连接其继承的 UV 或 Mip 输入，这个节点不会使用它们。

调用材质函数前，先用插件的资源检查功能或 `resources.inspect(kind=MaterialFunction)` 读取函数的实际引脚和文件摘要，再按结果填写路径、摘要和连线。未绑定函数的节点目录没有可直接使用的引脚列表。当前支持已经保存且无未保存修改的普通材质函数，不支持函数实例、材质层、函数输入默认值、创建函数或导出函数图。

## 注意事项

空间位置与法线节点使用页面中指定的坐标空间，不会自动在世界空间和切线空间之间转换。`PixelNormalWS` 不能用于 Normal 或 WorldPositionOffset 输出；其他着色阶段限制由 UE 编译器检查。

本版本列出的 71 类材质节点均为实验性支持。Custom HLSL、Material Attributes、未列出的表达式以及任意对象引用暂不支持。生成成功后，仍应在目标场景中检查贴图、光照和动画效果。


## 1.1.2 清漆与折射契约

当前新增字段、根输入类型、切线空间和 CLI 进程查询见[完整契约](LayeredMaterials.md)。新 Catalog/Schema 来自原生共享定义；旧输入需重新预检。
