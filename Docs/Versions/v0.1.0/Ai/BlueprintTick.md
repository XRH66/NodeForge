# 让 Actor 持续沿 X 轴移动

这个蓝图在 BeginPlay 时向日志输出一次标记，在 Tick 时按实际 DeltaSeconds 计算位移，以每秒 100 厘米沿世界 X 轴移动。

## 图的工作方式

`ReceiveBeginPlay → PrintString` 输出一次标记。`ReceiveTick` 的执行引脚连接到 `K2_SetActorLocation`。当前坐标加上 `(100,0,0) × DeltaSeconds`，结果连接到 `NewLocation`。`bSweep=false`、`bTeleport=false`。

BeginPlay 和 Tick 各最多一个，共用 Actor 的单一 EventGraph。默认 SceneRoot 和空 ConstructionScript 保持不变。其他事件、额外组件、多图和 Tick 设置不在这个示例的支持范围内。

## 使用步骤

1. 完成 [连接准备](Preparation.md)，读取当前目录和草稿修订。
2. 将下方 requestId 和目标路径换成未占用的值，确认 catalogDigest 与当前目录一致。
3. 载入完整 JSON，依次预检、生成和读取报告。参数见 [调用流程](QuickStart.md)。
4. 要检查重开结果，正常结束编辑器后执行 `graphs.verifySaved`。
5. 在 UE 中把生成的 Actor 放入场景并运行，观察位置随游戏时间变化。生成成功本身不代表已经运行了这一步。

## 完整 JSON v3

```json
{
  "format": "NodeForgeGraph",
  "version": 3,
  "requestId": "ExampleBlueprint8",
  "catalogDigest": "be97bf57d226778102038c261c556b89b262197c78158fe16ca293de709d8847",
  "target": {
    "assetPath": "/Game/NodeForgeTests/Examples/Sample8"
  },
  "asset": {
    "type": "Blueprint",
    "parent": {
      "class": "/Script/Engine.Actor"
    },
    "members": {
      "variables": []
    }
  },
  "graphs": [
    {
      "nodes": [
        {
          "id": "Begin",
          "class": "/Script/BlueprintGraph.K2Node_Event",
          "properties": {},
          "binding": {
            "kind": "Event",
            "owner": "/Script/Engine.Actor",
            "member": "ReceiveBeginPlay"
          }
        },
        {
          "id": "Print",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.KismetSystemLibrary",
            "member": "PrintString"
          },
          "defaults": {
            "InString": "NodeForge Tick ExampleIdentity",
            "bPrintToScreen": false,
            "bPrintToLog": true
          }
        },
        {
          "id": "Tick",
          "class": "/Script/BlueprintGraph.K2Node_Event",
          "properties": {},
          "binding": {
            "kind": "Event",
            "owner": "/Script/Engine.Actor",
            "member": "ReceiveTick"
          }
        },
        {
          "id": "GetLocation",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.Actor",
            "member": "K2_GetActorLocation"
          }
        },
        {
          "id": "ScaleVelocity",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.KismetMathLibrary",
            "member": "Multiply_VectorFloat"
          },
          "defaults": {
            "A": [
              100,
              0,
              0
            ]
          }
        },
        {
          "id": "AddLocation",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.KismetMathLibrary",
            "member": "Add_VectorVector"
          }
        },
        {
          "id": "SetLocation",
          "class": "/Script/BlueprintGraph.K2Node_CallFunction",
          "properties": {},
          "binding": {
            "kind": "Function",
            "owner": "/Script/Engine.Actor",
            "member": "K2_SetActorLocation"
          },
          "defaults": {
            "bSweep": false,
            "bTeleport": false
          }
        }
      ],
      "edges": [
        {
          "from": "Begin",
          "output": "then",
          "to": "Print",
          "input": "execute"
        },
        {
          "from": "Tick",
          "output": "then",
          "to": "SetLocation",
          "input": "execute"
        },
        {
          "from": "Tick",
          "output": "DeltaSeconds",
          "to": "ScaleVelocity",
          "input": "B"
        },
        {
          "from": "GetLocation",
          "output": "ReturnValue",
          "to": "AddLocation",
          "input": "A"
        },
        {
          "from": "ScaleVelocity",
          "output": "ReturnValue",
          "to": "AddLocation",
          "input": "B"
        },
        {
          "from": "AddLocation",
          "output": "ReturnValue",
          "to": "SetLocation",
          "input": "NewLocation"
        }
      ],
      "id": "EventGraph",
      "kind": "EventGraph"
    }
  ]
}
```

## 如何检查移动结果

比较同一 Actor 在至少三个游戏时刻的位置。每一段应满足 `ΔX = 100 × 有效游戏时间`，Y、Z、旋转和缩放保持不变，BeginPlay 标记只出现一次。暂停和卡顿时不要用墙钟时间代替实际 DeltaSeconds。

读取已保存蓝图可以使用 [图导出](AssetRead.md)，支持默认 v2 或显式 v3。复制时使用工具返回的完整文本和来源引用。这个示例的结论只适用于所示节点与连接，其他持续行为需要按实际图检查。
