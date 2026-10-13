# 整数与名称多路分支

本页说明 NodeForge v0.1.0 的 Graph JSON v2 写法。先读取[通用规则](GraphAiRules.md)和[当前节点目录](../Docs/Versions/v0.1.0/Catalog.json)，从实际目录取得 `catalogDigest`、节点类、属性和引脚。

## 整数 Switch

节点类为 `/Script/BlueprintGraph.K2Node_SwitchInteger`。配置示例：

```json
{"startIndex":0,"caseCount":3,"hasDefault":true}
```

输入为 `execute` 和整数 `Selection`；该配置的输出为 `0`、`1`、`2`、`Default`。`caseCount` 为 1–8，起点和终点必须满足目录中的 int32 范围检查。整数分支连续排列，不支持任意标签数组。

## 名称 Switch

节点类为 `/Script/BlueprintGraph.K2Node_SwitchName`。配置示例：

```json
{"cases":["Idle","Active"],"hasDefault":true}
```

输入为 `execute` 和名称 `Selection`；输出使用准确拼写的 `Idle`、`Active`、`Default`。分支数为 1–8，标识以 ASCII 字母开头，后接字母数字，最长 64 字符。不能使用目录中的保留名称，也不能添加只在大小写上不同的重复分支。

## 连接规则

`hasDefault=false` 时不能连接 `Default`。未连接的 `Selection` 可以填写默认值，连接变量后必须删除同一输入的默认值。

命中一个没有连线的分支时，执行链结束，不会自动转到 Default。只有没有命中任何分支时，才会执行存在且已连接的 Default。

分支出口名称需要精确拼写，运行时 name 值的比较则遵循 UE 的 FName 语义。不要设置隐藏的比较函数上下文或隐藏引脚。

结构示例：[名称分支](../Tests/BlueprintSwitches/SwitchName.json)、[八个整数分支](../Tests/BlueprintSwitches/SwitchEight.json)、[命中未连分支](../Tests/BlueprintSwitches/SwitchUnwiredMatch.json)。

本文链接的测试 JSON 用于阅读结构，可能含目录摘要或资源占位值，不能保证原样导入当前版本。请核对当前规则、填写真实资源，并使用新的请求编号和资产名。第一次使用推荐插件包附带的 `Examples/FirstMaterial.json`，见[入门教程](../Docs/GettingStarted.md)。
