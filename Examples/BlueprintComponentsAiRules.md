# 调用自身根组件

本页说明 NodeForge v0.1.0 的 Graph JSON v2 写法。先读取[通用规则](GraphAiRules.md)和[当前节点目录](../Docs/Versions/v0.1.0/Catalog.json)，从实际目录取得 `catalogDigest`、节点类、属性和引脚。

## 取得根组件

使用 `/Script/BlueprintGraph.K2Node_CallFunction`，设置 `owner=/Script/Engine.Actor`、`member=K2_GetRootComponent`。它的 `ReturnValue` 是当前蓝图实例的根 SceneComponent。

组件函数的 `owner` 为 `/Script/Engine.SceneComponent`。每个调用节点的 `self` 都必须直接连接上述 `ReturnValue`；同一个根组件输出可以连接多个调用。

## 读取和修改变换

| 函数 | 关键引脚 |
| --- | --- |
| `K2_GetComponentLocation` | 输出 `ReturnValue`，类型为 vector |
| `K2_GetComponentRotation` | 输出 `ReturnValue`，类型为 rotator |
| `K2_GetComponentScale` | 输出 `ReturnValue`，类型为 vector |
| `K2_SetWorldLocation` | 输入 `NewLocation` |
| `K2_SetWorldRotation` | 输入 `NewRotation` |
| `SetWorldScale3D` | 输入 `NewScale` |

带执行引脚的操作使用 `execute` 和 `then` 排定顺序。读取函数在值被使用时求值，节点在数组中的位置不决定执行先后。

不要给 `self` 填默认值、资源路径或任意对象，也不要用中转、类型转换或其他组件替代根组件直连。此功能不创建组件，不开放任意 Actor 或组件上下文。碰撞扫掠和物理效果需在自己的工程中检查。

结构示例：[位置、旋转和缩放](../Tests/BlueprintComponents/RootMotion.json)、[读取后增加偏移](../Tests/BlueprintComponents/RootOffset.json)。

本文链接的测试 JSON 用于阅读结构，可能含目录摘要或资源占位值，不能保证原样导入当前版本。请核对当前规则、填写真实资源，并使用新的请求编号和资产名。第一次使用推荐插件包附带的 `Examples/FirstMaterial.json`，见[入门教程](../Docs/GettingStarted.md)。
