# 旋转值与旋转成员

本页说明 NodeForge v0.1.0 的 Graph JSON v2 写法。先读取[通用规则](GraphAiRules.md)和[当前节点目录](../Docs/Versions/v0.1.0/Catalog.json)，从实际目录取得 `catalogDigest`、节点类、属性和引脚。

旋转值使用含 `pitch`、`yaw`、`roll` 的 JSON 对象，角度单位为度。每个值必须是有限数字，不能用数组、字符串或布尔值替代。

```json
{"name":"Facing","type":"rotator","default":{"pitch":15,"yaw":40,"roll":-25}}
```

变量读取和写入使用 `K2Node_VariableGet`、`K2Node_VariableSet`，`properties.variable` 设置为实际声明名。上例对应的变量数据引脚名是 `Facing`。

`Actor.K2_SetActorRotation` 的 `NewRotation` 可以连接旋转变量或 `MakeRotator.ReturnValue`。未连接时，也可按当前目录规则填写相同三键对象的默认值；同一个输入不能同时连接和赋默认值。

写入变量后再应用旋转，与先应用旧值再写入变量，行为不同。请通过执行边明确顺序。

只支持目录列出的 Actor 和自身根组件调用，不会因此开放任意组件、结构体或对象访问。

结构示例见[旋转测试数据](../Tests/BlueprintRotators/Readme.md)。

本文链接的测试 JSON 用于阅读结构，可能含目录摘要或资源占位值，不能保证原样导入当前版本。请核对当前规则、填写真实资源，并使用新的请求编号和资产名。第一次使用推荐插件包附带的 `Examples/FirstMaterial.json`，见[入门教程](../Docs/GettingStarted.md)。
