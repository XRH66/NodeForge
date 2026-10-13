# 创建材质实例

NodeForge 可以为已经保存的普通 Surface 材质创建 `MaterialInstanceConstant`，并覆盖父材质中的标量、向量和贴图参数。创建实例不会修改父材质。

## 操作步骤

1. 在 UE 中保存父材质，确保没有未保存的修改。
2. 用插件的资源检查功能，或 `resources.inspect(kind=MaterialParameters)`，读取父材质的实际参数、资源路径和文件 SHA256。
3. 按[实例示例](Examples.md)或[JSON v3 示例](../Ai/JsonArchitectureExamples.md)填写父材质信息。只填写父材质实际存在的参数，并保持参数类型一致。
4. 选择新的目标路径，先预检，再生成。生成后打开实例检查参数与效果。

## 输入格式

v2 请求使用 `assetType=MaterialInstance`，`nodes` 和 `edges` 均为空数组，`settings` 包含以下五项。v3 的对应结构见[JSON 编写说明](GraphAuthoring.md)。

| 字段 | 内容 |
| --- | --- |
| `parent` | 父材质的完整对象路径 |
| `parentAssetSha256` | 读取父材质时返回的实际文件摘要 |
| `scalarParameters` | 标量参数名及数值 |
| `vectorParameters` | 向量参数名及值 |
| `textureParameters` | 贴图参数名及资源路径 |

不需要覆盖的参数映射可设为 `{}`。父文件摘要用于检测父材质是否发生变化；不能填示例中的占位摘要。如果父材质修改并重新保存，需要重新读取后再生成。

## 当前限制

父资源必须是普通 Surface Material，不能是材质实例、材质层或函数实例。只支持全局标量、向量和普通非虚拟 Texture2D 参数；静态 Switch、材质层参数、未知参数名、类型不符或采样器不匹配会被拒绝。

材质实例使用通用的 `graphs.preflight` 和 `graphs.generate` 接口。规则查询仅支持材质和蓝图，没有实例专用查询工具或实例图导出。本版 AI 安装与连接检查不覆盖实例生成流程；使用时请检查实际生成和保存结果。
