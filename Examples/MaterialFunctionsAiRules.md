# 调用材质函数

本页说明 NodeForge v0.1.0 的 Graph JSON v2 写法。先读取[通用规则](GraphAiRules.md)和[当前节点目录](../Docs/Versions/v0.1.0/Catalog.json)，从实际目录取得 `catalogDigest`、节点类、属性和引脚。

## 先读取函数

材质函数的输入输出取决于具体资源。先在面板中选择已保存的普通 MaterialFunction，点击 **读取选中材质 / 函数**；AI 可以使用对应的资源读取工具。

读取结果包含真实路径、文件 SHA256、输入输出和节点模板。不能直接使用未绑定函数节点的空引脚表，也不能从另一个函数复制 GUID 或输入名。

## 填写调用节点

类为 `/Script/Engine.MaterialExpressionMaterialFunctionCall`，`properties` 包含：

- `MaterialFunction`：读取结果中的完整对象路径。
- `FunctionAssetSha256`：该函数文件的实际 SHA256。

连接的 `input` 使用真实输入名，`output` 使用十进制索引字符串。GUID 用于内部检查，不代替 JSON 的引脚名称。

函数节点不接受自定义 defaults。需要传值时，创建支持的常量或参数节点并连接；必填输入必须连接，可选输入未连接时使用函数定义的预览默认值。

## 限制

只支持允许读取目录中、已保存且没有未保存改动的普通 MaterialFunction，不支持函数实例、材质层或混合函数。

文件哈希只对应直接引用的函数，不会递归锁定它引用的其他函数和纹理。函数文件发生变化后，应重新读取参数和哈希。

读取返回的 `nodeTemplate` 是单个节点片段，不是可直接生成的完整图。需要为它分配图内唯一的 id，并补齐完整请求、其他节点与连线。

结构示例见[材质函数测试数据](../Tests/MaterialFunctions/)，资源读取步骤见[资源说明](../Docs/Versions/v0.1.0/Ai/Resources.md)。

本文链接的测试 JSON 用于阅读结构，可能含目录摘要或资源占位值，不能保证原样导入当前版本。请核对当前规则、填写真实资源，并使用新的请求编号和资产名。第一次使用推荐插件包附带的 `Examples/FirstMaterial.json`，见[入门教程](../Docs/GettingStarted.md)。
