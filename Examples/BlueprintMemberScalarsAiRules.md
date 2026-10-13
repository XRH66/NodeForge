# 整数和名称成员变量

本页说明 NodeForge v0.1.0 的 Graph JSON v2 写法。先读取[通用规则](GraphAiRules.md)和[当前节点目录](../Docs/Versions/v0.1.0/Catalog.json)，从实际目录取得 `catalogDigest`、节点类、属性和引脚。

在 `settings.variables` 中声明成员变量，再使用变量节点读取或写入。完整字段和可选属性见[蓝图说明](../Docs/Versions/v0.1.0/Reference/Blueprint.md)。

## 整数成员

`type` 为 `int`，默认值为 -2147483648 至 2147483647 的 JSON 整数。字符串、小数、布尔值和 int64 不属于这个类型。

```json
{"name":"Count","type":"int","default":0}
```

## 名称成员

`type` 为 `name`，默认值为非空字符串，UTF-16 长度最多 1023。空名称使用 `None`，不要写空字符串或用 `none`、`NONE` 替代。

```json
{"name":"State","type":"name","default":"Idle"}
```

## 读取与写入

节点使用 `/Script/BlueprintGraph.K2Node_VariableGet` 或 `K2Node_VariableSet`，`properties.variable` 必须与声明名逐字一致。目录中的样本引脚 `Value` 要替换为实际成员名，例如 `Count`。

Setter 的执行引脚为 `execute`、`then`，可读取的输出为 `Output_Get`。先接写入执行流，再在后续节点中读取值；数组位置不决定执行顺序。

名称需要参与字符串运算时使用支持的 `Conv_NameToString`。整数需要其他数值类型时使用目录中的显式转换函数。

结构示例：[整数计数](../Tests/BlueprintMemberScalars/MemberCount.json)、[名称状态](../Tests/BlueprintMemberScalars/MemberState.json)、[None 名称](../Tests/BlueprintMemberScalars/MemberNone.json)。

本文链接的测试 JSON 用于阅读结构，可能含目录摘要或资源占位值，不能保证原样导入当前版本。请核对当前规则、填写真实资源，并使用新的请求编号和资产名。第一次使用推荐插件包附带的 `Examples/FirstMaterial.json`，见[入门教程](../Docs/GettingStarted.md)。
