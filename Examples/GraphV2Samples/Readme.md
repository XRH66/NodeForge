# 可导入 v2 数据样例

2026-10-09。本目录八份 JSON 均逐字节复制自 [GraphRun2 Inputs](../../Reports/20261008GraphV2/GraphRun2/Inputs/)，已包含真实目录摘要，无占位符。它们分别在该隔离测试工程完成实际构建回读、编译、保存及新 UE 进程独立重开；整个 [GraphRun2](../../Reports/20261008GraphV2/GraphRun2/Summary.json) 共 33 例通过，其中 20 个成功生成正例、13 个明确拒绝负例。具体节点条目覆盖见 [UE532GraphVerification](../../Catalog/UE532GraphVerification.json)，未测目录条目仍标 notVerified。

这些是已测输入的可复用副本。文件本身不等于可以覆盖旧资产，也不代表每个节点组合都通过；运行、视觉和动画证据由主代理最终报告另行关联，本文不代填其状态。

## 使用与新名称

1. 使用匹配 [UE532Graph](../../Catalog/UE532Graph.json) 的 NodeForge v2 插件，在独立验证工程的 NodeForge 面板选择文件或粘贴 JSON。目录摘要为 `0d786af7d63e1ea209691493e9b336f50e61eaeb45ef34eeec12ff01b22a884f`。
2. 导入前核对该工程没有同名目标、也没有用过对应 requestId。GraphRun2 自己的工程已经存在这些目标，原样重导会按 create-only 规则拒绝；其它测试工程也不能假定名称空闲。
3. 要生成自己的新资产，将样例另存为新输入，修改 requestId 与 assetPath 的最后名称，例如 requestId=`MyWaterTrial2`、assetPath=`/Game/NodeForgeTests/NFMyWaterTrial2`。两者使用 1–64 ASCII 字母数字，目标仍直接位于 NodeForgeTests；不要覆盖本目录准确副本。
4. 新名称输入是派生数据，其字节 SHA 与下表不同，应另存导入报告。编辑属性、节点或连线后也必须重新验证，不能沿用旧 receipt 的通过结论。
5. Import 显示 waitingPersistence 代表当前会话已保存；独立重开仍需单独核验。同名拒绝时按报告处理，不删除或改写原资产来绕过保护。

协议见 [GraphProtocol](../../Contracts/GraphProtocol.md)，给其它 AI 的规则见 [GraphAiRules](../GraphAiRules.md)。当前文件是在本机 UE 5.3.2 生成时使用的数据；它们不是第三方 AI 的原始输出，不得用于冒充外部 AI 一次成功闭环。

## 准确来源及哈希

| 样例 | 类型 | 原 requestId | 原目标 | SHA256 | 本地证据 |
| --- | --- | --- | --- | --- | --- |
| [StaticColor.json](StaticColor.json) | Material | `GraphStaticColor` | `/Game/NodeForgeTests/NFGraphStaticColor` | `b2ad54115ccba1f53a50c34034eb4add504d78a06c6f22b7873eae234507883a` | [原始输入](../../Reports/20261008GraphV2/GraphRun2/Inputs/StaticColor.json)、[Import](../../Reports/20261008GraphV2/GraphRun2/StaticColorImport.json)、[Reopen](../../Reports/20261008GraphV2/GraphRun2/StaticColorReopen.json) |
| [DynamicPulse.json](DynamicPulse.json) | Material | `GraphDynamicPulse` | `/Game/NodeForgeTests/NFGraphDynamicPulse` | `be7f8cdba44dac3eeefa47af45859b436a7c127aa357e696f7373f3f5ddb6bfd` | [原始输入](../../Reports/20261008GraphV2/GraphRun2/Inputs/DynamicPulse.json)、[Import](../../Reports/20261008GraphV2/GraphRun2/DynamicPulseImport.json)、[Reopen](../../Reports/20261008GraphV2/GraphRun2/DynamicPulseReopen.json) |
| [Water.json](Water.json) | Material | `GraphWater` | `/Game/NodeForgeTests/NFGraphWater` | `bc925a1b1ec1ad5a3907bef2bc734919a552ef11d05a7e134637808e6338cbf7` | [原始输入](../../Reports/20261008GraphV2/GraphRun2/Inputs/Water.json)、[Import](../../Reports/20261008GraphV2/GraphRun2/WaterImport.json)、[Reopen](../../Reports/20261008GraphV2/GraphRun2/WaterReopen.json) |
| [WaterChanged.json](WaterChanged.json) | Material | `GraphWaterChanged` | `/Game/NodeForgeTests/NFGraphWaterChanged` | `cf71efc48e253524afa096a41196b2435128296da4448d584d2b31a2f2af53a4` | [原始输入](../../Reports/20261008GraphV2/GraphRun2/Inputs/WaterChanged.json)、[Import](../../Reports/20261008GraphV2/GraphRun2/WaterChangedImport.json)、[Reopen](../../Reports/20261008GraphV2/GraphRun2/WaterChangedReopen.json) |
| [BlueprintTextChanged.json](BlueprintTextChanged.json) | Blueprint | `GraphBlueprintTextChanged` | `/Game/NodeForgeTests/NFGraphBlueprintTextChanged` | `121899c8a4c84b2659bcee5e47050bb7f8f10c92638417351f0c6bdfd682f0c0` | [原始输入](../../Reports/20261008GraphV2/GraphRun2/Inputs/BlueprintTextChanged.json)、[Import](../../Reports/20261008GraphV2/GraphRun2/BlueprintTextChangedImport.json)、[Reopen](../../Reports/20261008GraphV2/GraphRun2/BlueprintTextChangedReopen.json) |
| [BlueprintBranch.json](BlueprintBranch.json) | Blueprint | `GraphBlueprintBranch` | `/Game/NodeForgeTests/NFGraphBlueprintBranch` | `f7fd53ef7aa2f1cc93a7e6d94bdd17230599de5e468194353a8b894928c24e4b` | [原始输入](../../Reports/20261008GraphV2/GraphRun2/Inputs/BlueprintBranch.json)、[Import](../../Reports/20261008GraphV2/GraphRun2/BlueprintBranchImport.json)、[Reopen](../../Reports/20261008GraphV2/GraphRun2/BlueprintBranchReopen.json) |
| [BlueprintBranchFalse.json](BlueprintBranchFalse.json) | Blueprint | `GraphBlueprintBranchFalse` | `/Game/NodeForgeTests/NFGraphBlueprintBranchFalse` | `22a17f0f3563642782c669a0922188bbb5a167c2c76b61a868af173f68bf719b` | [原始输入](../../Reports/20261008GraphV2/GraphRun2/Inputs/BlueprintBranchFalse.json)、[Import](../../Reports/20261008GraphV2/GraphRun2/BlueprintBranchFalseImport.json)、[Reopen](../../Reports/20261008GraphV2/GraphRun2/BlueprintBranchFalseReopen.json) |
| [BlueprintConcat.json](BlueprintConcat.json) | Blueprint | `GraphBlueprintConcat` | `/Game/NodeForgeTests/NFGraphBlueprintConcat` | `2c14c2820ce59a5c7df8ec67bfac4743974865861f2e6f693f900d621db655cf` | [原始输入](../../Reports/20261008GraphV2/GraphRun2/Inputs/BlueprintConcat.json)、[Import](../../Reports/20261008GraphV2/GraphRun2/BlueprintConcatImport.json)、[Reopen](../../Reports/20261008GraphV2/GraphRun2/BlueprintConcatReopen.json) |

材质包括有色静态、Time 动态、33 节点/40 边的纯数学半透明水面及参数变化；水面不依赖外部贴图或材质函数。蓝图包括输入自定义文本、变量读写与真假分支、字符串函数数据流。实际属性、默认值和连线以各 JSON 为准。

本目录复制过程只创建新的数据文件和说明，没有启动 UE、修改测试资产、插件代码、DLL、运行脚本、冻结夹具或历史报告。
