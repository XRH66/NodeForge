# 编写蓝图函数调用

本页说明 NodeForge v0.1.0 的 Graph JSON v2 写法。先读取[通用规则](GraphAiRules.md)和[当前节点目录](../Docs/Versions/v0.1.0/Catalog.json)，从实际目录取得 `catalogDigest`、节点类、属性和引脚。

## 节点声明

函数节点使用 `/Script/BlueprintGraph.K2Node_CallFunction`。`properties` 使用目录列出的 `owner` 和 `member`，不要使用编辑器显示名或中文名替代。

本版包含数学、比较、字符串、名称、向量和旋转函数，以及当前 Actor 的部分操作。函数的输入、输出和类型从目录逐项查询；不能根据某个函数是纯函数，就假定它已被支持。

## 默认值与连接

- 默认值写在 `defaults` 中，只用于目录允许写入、未连接的输入。
- 同一输入不能同时填写默认值和连接一条边。
- `int` 使用 int32 范围内的 JSON 整数；`bool` 使用 `true` 或 `false`；不要用字符串模拟数字。
- `vector` 使用 `[x,y,z]`；`rotator` 使用 `{"pitch":值,"yaw":值,"roll":值}`。
- `float` 和 `double` 引脚需要精确匹配，必要时使用目录允许的显式转换节点。

Actor 实例函数使用当前生成蓝图自身的 `self`，不能填入其他 Actor。根组件函数另见[组件调用](BlueprintComponentsAiRules.md)。

## 执行顺序

有副作用的操作通过 `execute`、`then` 和分支输出连接。节点在 `nodes` 数组中的排列不决定执行顺序；纯函数在结果被使用时求值。

结构示例：[计数与分支](../Tests/BlueprintFamilies/CountTrue.json)、[文本处理](../Tests/BlueprintFamilies/TextKnown.json)、[向量运算](../Tests/BlueprintFamilies/VectorBasis.json)。

本文链接的测试 JSON 用于阅读结构，可能含目录摘要或资源占位值，不能保证原样导入当前版本。请核对当前规则、填写真实资源，并使用新的请求编号和资产名。第一次使用推荐插件包附带的 `Examples/FirstMaterial.json`，见[入门教程](../Docs/GettingStarted.md)。
