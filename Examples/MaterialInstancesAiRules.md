# 创建普通材质实例

本页说明 NodeForge v0.1.0 的 Graph JSON v2 写法。先读取[通用规则](GraphAiRules.md)和[当前节点目录](../Docs/Versions/v0.1.0/Catalog.json)，从实际目录取得 `catalogDigest`、节点类、属性和引脚。

创建材质实例前，先读取真实父材质的参数和文件 SHA256。父材质必须是已保存、没有未保存改动的普通 Surface 材质。

## Graph v2 格式

`assetType` 为 `MaterialInstance`，`nodes` 和 `edges` 都为空数组。`settings` 使用以下字段：

| 字段 | 内容 |
| --- | --- |
| `parent` | 父材质的完整对象路径 |
| `parentAssetSha256` | 父材质文件的 SHA256 |
| `scalarParameters` | 参数名到数字的映射 |
| `vectorParameters` | 参数名到四数 RGBA 数组的映射 |
| `textureParameters` | 参数名到真实 Texture2D 路径的映射 |

三个参数映射都需要提供；没有覆写时使用 `{}`。只填写要改变的参数，其他参数使用父材质的默认值。参数名称和类型必须来自实际读取结果。

数值需有限且位于 ±1,000,000 内，最多 128 项覆写。纹理必须是允许读取的普通非虚拟 Texture2D，并与父材质中相应采样器匹配。

## 不能改变什么

材质实例不会创建新的节点或连线，也不能在这里另写材质域、混合模式和着色模型。父级不能是另一个实例，不支持材质层、静态开关或任意父资源。

父文件变化后先重新读取，不能仅替换哈希绕过检查。本版没有材质实例专用 MCP 操作或实例整图导出。

完整格式见[材质实例参考](../Docs/Versions/v0.1.0/Reference/MaterialInstance.md)。[测试模板](../Tests/MaterialInstances/Cases.json) 包含需要填写的父文件哈希，不能原样导入。

本文链接的测试 JSON 用于阅读结构，可能含目录摘要或资源占位值，不能保证原样导入当前版本。请核对当前规则、填写真实资源，并使用新的请求编号和资产名。第一次使用推荐插件包附带的 `Examples/FirstMaterial.json`，见[入门教程](../Docs/GettingStarted.md)。
