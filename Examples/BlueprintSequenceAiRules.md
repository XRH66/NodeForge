# 使用 Sequence 安排执行顺序

本页说明 NodeForge v0.1.0 的 Graph JSON v2 写法。先读取[通用规则](GraphAiRules.md)和[当前节点目录](../Docs/Versions/v0.1.0/Catalog.json)，从实际目录取得 `catalogDigest`、节点类、属性和引脚。

节点类为 `/Script/BlueprintGraph.K2Node_ExecutionSequence`，`properties` 中填写整数 `outputCount`，范围为 2–8。

```json
{"id":"Steps","class":"/Script/BlueprintGraph.K2Node_ExecutionSequence","properties":{"outputCount":3}}
```

输入为 `execute`，三个输出分别为 `then_0`、`then_1` 和 `then_2`。需要其他数量时，从当前目录选择对应配置，不能沿用两输出节点的引脚表。

Sequence 按输出编号依次执行：前一路同步调用返回后，再进入下一路。它不是并行执行，也不会为没有连接的出口补上操作。

一个执行输出不能同时连接两个目标。需要多个步骤时，使用不同的 Sequence 输出；也可以把一路连接到另一个支持的 Sequence。循环、延迟和未列出的动态引脚仍不支持。

结构示例：[两个输出](../Tests/BlueprintSequence/SequenceTwo.json)、[三个输出](../Tests/BlueprintSequence/SequenceThree.json)、[嵌套 Sequence](../Tests/BlueprintSequence/SequenceNested.json)。

本文链接的测试 JSON 用于阅读结构，可能含目录摘要或资源占位值，不能保证原样导入当前版本。请核对当前规则、填写真实资源，并使用新的请求编号和资产名。第一次使用推荐插件包附带的 `Examples/FirstMaterial.json`，见[入门教程](../Docs/GettingStarted.md)。
