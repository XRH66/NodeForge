# 创建 Actor 蓝图

NodeForge 可以创建以 `/Script/Engine.Actor` 为父类的蓝图，并在一个 EventGraph 中添加事件、函数、变量和连线。先从[蓝图节点列表](BlueprintNodeIndex.md)查找节点，再参考[完整示例](Examples.md)或[JSON v3 示例](../Ai/JsonArchitectureExamples.md)编写请求。

## 支持的节点

目录包含 327 条节点配置：299 个函数、2 个事件、7 种 Sequence 输出数量、Branch、整数或名称 Switch，以及 7 种变量类型的读取和赋值节点。目录状态为实验性，请先预检再生成。

- **事件：** `ReceiveBeginPlay` 和 `ReceiveTick` 可以共存，每个事件最多一个。
- **顺序执行：** Sequence 支持 2–8 个输出，引脚名称为 `Then_0`、`Then_1` 等。
- **整数分支：** SwitchInteger 使用 `startIndex`、`caseCount`、`hasDefault`；支持 1–8 个连续分支，不能超出 int32 范围。
- **名称分支：** SwitchName 使用 `cases` 和 `hasDefault`。分支名称不能只在大小写上不同；不保留 Default 时，未匹配的执行会结束。
- **函数：** 使用目录中列出的 `owner`、`member` 和引脚名称，不能填入任意 UFunction。

节点在 JSON 数组中的顺序不会决定执行顺序，执行顺序由连线决定。Switch 的 `cases` 等配置数组有顺序含义，请保持所需顺序。

## 声明变量

支持 `bool`、`float`、`string`、`vector`、`int`、`name` 和 `rotator`。读取或设置变量前，需要在资产设置中声明它。节点示例中的 `Value` 是占位变量名，应替换为自己的变量名。

| 类型 | 默认值格式 |
| --- | --- |
| `bool` | JSON 布尔值 |
| `float` | JSON 数字，在 UE 中使用 real/double |
| `int` | int32 范围内的整数 |
| `string` | 最多 4,096 个字符 |
| `name` | 非空名称，最长 1,023 个字符；空名称使用 `None` |
| `vector` | `[x, y, z]` |
| `rotator` | `{"pitch": 0, "yaw": 0, "roll": 0}`，单位为度 |

变量名以 ASCII 字母开头，其余为字母或数字；名称不能只在大小写上不同，也不能与继承成员冲突。Rotator 的各分量必须是有限数值，绝对值不超过 1e12。

可以设置 `instanceEditable`、`category`、`tooltip`，在 UE Details 面板中展示和编辑变量。填写 `attributes` 时需要同时提供这三个字段，详见[资产设置](../Ai/AssetSettings.md)。

## 连接和默认值

默认值只能填入未连接、可写、可见的输入引脚，值的 JSON 类型应与引脚一致。不要把数字写成字符串，或用数组代替 Rotator 对象。

NodeForge 不会自动插入类型转换、替换连线或修复环路。当前不支持拆分引脚和失效引脚。

自身 Actor 函数使用隐式 `self`。SceneComponent 操作仅支持本 Actor 的根组件：把 `K2_GetRootComponent.ReturnValue` 直接连接到组件节点的 `self`。不能使用外部 Actor、其他组件、任意对象默认值、隐式转换或 Reroute 来绕过这一要求。

## 当前限制

每个蓝图最多 128 个节点、256 条边和 64 个变量。支持没有事件的纯数据图；要产生运行行为，仍需连接事件与实际使用计算结果的节点。UE 自动创建的 SceneRoot 和空 ConstructionScript 会保留。

暂不支持自定义父类、多图、宏或函数图、自定义事件、Timeline、Delegate、Interface、额外组件、循环、数组、Map、Set、Enum、任意结构体和未列出的函数。碰撞、物理、联网行为以及 Tick 的实际效果，需要在目标场景中检查。
