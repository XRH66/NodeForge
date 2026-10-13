# 向量与 Actor 位置

本页说明 NodeForge v0.1.0 的 Graph JSON v2 写法。先读取[通用规则](GraphAiRules.md)和[当前节点目录](../Docs/Versions/v0.1.0/Catalog.json)，从实际目录取得 `catalogDigest`、节点类、属性和引脚。

## 向量值

向量默认值使用三个有限数字的数组 `[x,y,z]`。声明成员时设置 `type=vector`。需要从不同数值组成向量时，使用目录中的 `MakeVector`。

| owner | member | 关键引脚 |
| --- | --- | --- |
| `/Script/Engine.KismetMathLibrary` | `MakeVector` | `X`、`Y`、`Z` → `ReturnValue` |
| `/Script/Engine.KismetMathLibrary` | `BreakVector` | `InVec` → `X`、`Y`、`Z` |
| `/Script/Engine.KismetStringLibrary` | `Conv_VectorToString` | `InVec` → `ReturnValue` |
| `/Script/Engine.Actor` | `K2_GetActorLocation` | 输出 `ReturnValue` |
| `/Script/Engine.Actor` | `K2_SetActorLocation` | 输入 `NewLocation`、`bSweep`、`bTeleport`；执行引脚为 `execute`、`then` |

## 组织执行顺序

Actor 函数操作当前生成蓝图自身的实例，不接受任意外部 Actor。UE 常规位置单位为厘米。

要“先移动再打印位置”，把设置位置操作的 `then` 连到后续打印的 `execute`，并把实际读取值接入打印内容。仅把读取节点排在数组后面不能保证顺序。

需要持续移动时，读取[Tick 说明](../Docs/Versions/v0.1.0/Ai/BlueprintTick.md)，使用实际 DeltaSeconds 计算位移；不要把每帧位移误当成每秒速度。

结构示例：[直接向量](../Tests/BlueprintMotion/MotionDirect.json)、[向量变量](../Tests/BlueprintMotion/MotionVariable.json)、[移动后读取](../Tests/BlueprintMotion/MotionReadback.json)。

本文链接的测试 JSON 用于阅读结构，可能含目录摘要或资源占位值，不能保证原样导入当前版本。请核对当前规则、填写真实资源，并使用新的请求编号和资产名。第一次使用推荐插件包附带的 `Examples/FirstMaterial.json`，见[入门教程](../Docs/GettingStarted.md)。
