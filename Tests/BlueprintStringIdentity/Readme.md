# 字符串与名称逐字完整性样例

本目录是字符串完整性修复阶段的新数据模板。`Cases.json`供`RunGraphRegression.py`使用，`RuntimeCases.json`沿用`RunBlueprintMotionChecks.py`接口。`expected`和消息均是验收期望；原始准备时只完成JSON与真实目录端口静态检查，没有启动UE、构图、编译、保存、重开或运行，所有实际验收为`notRun`。

2026-10-10状态整理：上述notRun限定于准备时点；后续Build55构图/重开、逐字运行与受影响回归证据见[功能终态](../../Docs/BlueprintStringIntegrity.md)和[逐项验证](../../Catalog/UE532BlueprintStringIdentityVerification.json)。原模板与断言保持，不把文档整理作为新增测试。

## 来源与摘要

真实端口来源：[Build52原生目录](../../Reports/20261009UsableEditor/SwitchExpansion1/Capture52/ActualCatalog.json)，engine为`5.3.2-29314046+++UE5+Release-5.3`，catalogDigest为`1ae2791da31e336b3d147d1ed20e173ddfe51e721d255df040c843c743171317`，文件SHA256为`2d97f32e341761e627b01f78d334c7ce85b0bf953e0686ec7330d7691ed313ad`。PrintString的`execute/InString/bPrintToScreen/bPrintToLog`、Concat_StrStr的`A/B/ReturnValue`、Conv_NameToString的`InName/ReturnValue`和对应string/name成员读取签名均来自该实际目录，不按显示名猜端口。

当前修复构建模板位于本目录各JSON，摘要使用`CATALOG_DIGEST_FROM_REAL_ENGINE`；运行器用root最终选定的新构建原生目录替换。两个兼容例明确`preserveCatalogDigest:true`：StringSwitch52Compat固定上述Build52摘要；StringMaterial45Compat固定Build45摘要`96c9459d583fd0c5156842882d5ba68a259db91e5b3db8d495f77be79cfee8c7`，不随新目录替换。

兼容结构来源：[SwitchInt.json](../BlueprintSwitches/SwitchInt.json)与[SwitchMaterialCompat.json](../BlueprintSwitches/SwitchMaterialCompat.json)。只在新模板中改变请求ID、目标名及明确的兼容摘要；原文件不修改。准备器：[PrepareFixtures.py](../../Reports/20261009UsableEditor/StringIdentityExpansion1/PrepareFixtures.py)，只使用标准库及既有运行器的纯目录检查函数，默认拒绝任何已有输出，`--check-only`仅静态检查。

## 期望结果与拒绝边界

- StringPrintMixed与StringPrintLower仅改变新图中的消息为`AbC`/`abc`，要求保存重开及实际消息分别保留大小写。StringMember读取string成员Message的`AbC`默认值。
- StringNameNone读取name成员Mode的规范`None`，经Conv_NameToString输出`None`。StringCompose使用同一个Concat_StrStr，把`AbC`和`/abc`组合为`AbC/abc`。
- StringSwitch52Compat保留原生整数Switch结构，期望`Switch checked`及位置`[20,0,0]`；StringMaterial45Compat保留条件选色材质与旧摘要，只登记构图/保存重开期望，本目录没有材质画面通过声明。
- StringWrongOwner/Member/Class/Parent/Type各仅改变对应身份的大小写。StringWrongVariable用纯getter引用`message`，而声明为`Message`，不让另一条错误端口掩盖变量身份校验。
- StringNameLowerNone/UpperNone分别使用`none`/`NONE`，期望拒绝；StringWrongPin把原生默认值端口`InString`写为`inString`。StringCollision使用新requestId指向StringPrintMixed的同名目标，必须先运行StringPrintMixed并拒绝覆盖。

本目录不含对已有UE资产的篡改指令；root单独负责CPP修复、原生篡改守卫和最终调度。尚未执行的UI、外部AI、交付及运行不能因模板或静态预检存在而记为通过。

## 2026-10-09 实测终态追加

上方notRun是准备时点，模板expected仍是期望而非运行回执。实际Build53在none/NONE两个负例失败，失败报告/部分资产保留；Build54修复通过后又发现消息验证器比较风险，最终同Build55完成17例构图保存独立重开、14行实际篡改/恢复、六图逐字PIE与错消息拒绝、三图导出往返及受影响回归。[最终汇总](../../Reports/20261009UsableEditor/StringIdentityExpansion1/FinalSummary.json)和[逐项验证](../../Catalog/UE532BlueprintStringIdentityVerification.json)是实际证据；模板原文不改写为执行报告。本版本真实UI/外部AI/新包仍notRun，旧摘要代表保留原能力边界。
