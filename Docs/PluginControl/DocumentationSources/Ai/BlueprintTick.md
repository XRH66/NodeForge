# 普通Actor事件与持续移动

本页对应 {{bundleId}} / {{buildId}}，目录摘要由当前正式目录生成。下面是完整当前配方，保留原有限样例的节点、属性和连接语义；它们不是历史导出原payload或已保存来源证明。当前schema、类名及事件/函数绑定已静态核对，目录仍experimental。新任务先确认授权与本次实际目录/目标占用，按[AiPreparation]({{page:AiPreparation}})使用新的requestId和完整目标，保存实际输入原字节后正式预检；示例身份不预留，也不能把换digest当旧能力升级。

带资源依赖的配方先精确inspect当前已保存、clean资源及实际文件SHA/函数GUID签名。全零assetSha256是明确未取得证明的占位值，结构合法但不可直接预检生成；必须用本次返回的真实值。若资源/参数/端口与配方不匹配就停止，不能修改来源报告或凭文档补造证明。完整材料引用正例见[AiResources]({{page:AiResources}})。
## 用户：一次标记与每帧位移

Actor.ReceiveBeginPlay→PrintString只打印一次可归属标记；Actor.ReceiveTick的then→K2_SetActorLocation.execute。K2_GetActorLocation与速度(100,0,0)×实际DeltaSeconds相加，将结果连入NewLocation；bSweep=false、bTeleport=false。BeginPlay/Tick各最多一个，共存于原单EventGraph；默认SceneRoot/空ConstructionScript保持。没有新增变量属性、组件、多图、其它事件或运行API。原BT只验持续行为；当前候选另有固定Masked画面和Win64包后运行，范围见发行说明。

## AI：正式生成、独立核验和只读往返

先docs_index/read确认同版Readme/AiQuickStart/FeatureMap及本页。显式准备/register当前本次已显式绑定的专用工程并connect，query Catalog取得实际目录。按AiPreparation固定本次新的一件Blueprint目标/request及新的call/report身份，保存完整JSON原字节。沿原draft.load显式version=3→draft.get→preflight→generate；waitingPersistence时保留原报告，正常退出自有生成窗，同stage、原JSON、原生成收据走原CLI新PID Verify，禁止Import/resave替代。pending/unknown先查原calls.get，不换ID重发。

新saved-load会话以显式源取得同一已保存资产默认2/显式formatVersion=3完整导出；显式原payload+原sourceProof加载3并预检。来源original/retain及input/report/payload SHA、DLL/session/catalog/保存状态/busy/revision保护保持，不再生成第二副本。新资产目标一律拒绝占用。

## 独立时间配方与开发消费

Graph JSON只描述图，不携带运行期待。独立配方ContinuousX冻结速度[100,0,0]、采样阈值[0,0.5,1]有效游戏秒、至少三时刻两个区间每区间至少两帧、每帧Delta≤0.25、位置容差0.001厘米/旋转缩放1e-6、最多4096帧、原停止55秒/硬限60秒。原观察器通过匹配OnWorldPreActorTick/OnWorldPostActorTick累计本Actor实际有效Delta，只读取Actor/World，不调用Tick/设位置/启用Tick；墙钟仅超时。原GraphMapCreate最多一张本批辅助地图，正常交互编辑器PIE核两区间ΔX=100×ΣΔt及Y/Z/旋转/缩放不变、BeginPlay一次。

GraphCodec→GraphDocument/FNodeBinding→BlueprintGraph共享精确事件定义；v3 binding、v2 properties.owner/member同源解码。准入、唯一性、Spawner、目录与回读消费同一Events；精确编译派生bCanEverTick仅允许已声明Tick引出的差异，其余Actor继承/Tick设置仍按NativeCDO，不忽略整组属性。133归档保留原2/3和Blueprint10，132仍原2；旧目录没有Tick能力，不新增JSON版本、逐事件MCP工具或第二控制层。未知事件/owner、重复两事件、非法端口、未支持Tick字段都拒绝。


## 完整v3持续X移动配方

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
            "InString": "NodeForge Tick NFA000007TickReq",
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

## 版本与有限证明

Build134有限BT已终审完成：BeginPlay归属标记一次、实际游戏Delta的三时刻/两个区间持续世界X100厘米/秒通过。时间期待独立于Graph JSON，观察器只读，不驱动Actor。原测试台账、配方与失败报告留维护库，普通安装任务不要求运行内部运动验证脚本。本候选仅只读复用原Tick Blueprint用于最小无生成器运行/包后归属，不重复速度验收。

132原v2无Tick；133保持原2/3但无Tick；旧摘要不得借新digest冒充支持。保存/Verify/运行分别记录，回退按[兼容]({{page:VersionCompatibilityAndMigration}})恢复成套字节，不删除新资产或旧收据。其它持续行为/组合未外推。
