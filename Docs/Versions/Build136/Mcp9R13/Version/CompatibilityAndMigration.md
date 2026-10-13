# 本版兼容、更新与成套回退

当前 NodeForge-Build136-Mcp9R13 / Build136 Repair6 / 桥0.3.6 / SDK2.3.0 / ABI1。JSON2/3进入共同GraphDocument和原生核心；v1仅保留原固定有限语义。旧资产、原输入、调用/报告/收据和失败历史不自动迁移或改写。安装操作先看[UserInstallation](../User/Installation.md)，连接与正常结束见[AiPreparation](../Ai/Preparation.md)。

## 精确能力与旧输入

| 目录档案 | 可消费的有限能力 |
| --- | --- |
| 当前候选 | 原Surface设置/变量attributes、普通Actor BeginPlay/Tick、现代2/3，以及当前显式读取策略/有界查询/Texture2D检查；实际digest从同包Catalog或正式catalog_get取得 |
| Build135 | 原AS material7/blueprint12、现代2/3、Tick/Masked/attributes；未因新查询改变其原摘要 |
| Build134 | material6/blueprint11/max3，含有限Tick；不开放新增AS设置/attributes |
| Build133 | material6/blueprint10/max3，原2/3，无Tick或新增AS设置 |
| Build132 | material6/blueprint10/max2，原v2，无v3/Tick/新增AS设置 |

旧摘要只允许原档案范围，未知/空摘要拒绝。迁移先核本版实际目录、所需节点/字段/pin/配置与直接资源资格，重新形成明确新输入再预检；仅替换digest或变换结构不会升级旧来源证明。原来源初载使用完整text/sourceRef，保持数组/presence/值；retain显式编辑与来源成对初载互斥。不同会话handle/report属于原会话，不可拿新handle重放旧call。

## 同版资料与兼容应用刷新

操作者先查清原call/unknown，正常停止旧自有通道/UE；核成套批准的DLL、CLI、SDK依赖、ToolContract、Catalog/Operations、Manifest和固定客户端Config。固定客户端Config可按批准刷新；私有Preparation Config/Registration/Receipt/Commands按原SHA不可改写换版，新会话用原准备工具产生新记录。

已载入0.3.2或后续兼容Application ABI的服务，下一次docs_index/tools/list沿原stdio连接核批准的新资料或应用代次，不要求逐版手动开关。实际bundle、docsManifestSha256、configurationRefresh与serviceRuntime/loadedRuntimeFiles必须核对；ProtocolSeen是首次观察。工具变化再发现tools/list，新业务会话重新connect取得真实handle/revision。纯资料修订不切业务/DLL/SDK，不清未知记录。SDK/解释器/Application ABI不兼容需真实服务级载入，不能改SHA或版本字符串伪装成功。

busy/inflight/unknown/pending、存活旧编辑器或外部lease阻止完整切换。原handle仍只可查已登记原call/报告以收束现场；不能借重启、清锁、抢owner恢复写入。

## 安装绑定的可验证撤销

绑定前工程须正常关闭且独占。原PrepareNodeForgeInstallation bind返回本次原Receipt；撤销用原rollback配方核文件仍匹配原after SHA，恢复该原绑定/工程描述的before字节。其它用户后续改动必须保留。当前候选实际完成解压安装验证后按原receipt逐字回退到原开发绑定，未替换运行DLL、删除资产或恢复旧台账。

## 产品版本回退

从当前候选回到前已交付Build135 Repair1/Mcp8R5或其它已批准包，先保存当前私有配置和准确产品字节；核无pending/unknown、正常结束自有UE、工程独占，再成套选择未改旧插件stage、原CLI/桥/依赖/ToolContract/Manifest/资料及本次私有绑定。Rollback.json说明成套恢复范围与旧资料身份；安装Receipt的撤销和跨产品版本回退是两件事。本轮实际验的是安装撤销，不声称已在客户端运行回退至旧Build135。

保留新资产、输入、原调用/收据、失败和当前台账，不复制旧nextBatch/activeRun覆盖当前状态，不删除新包或改写冻结历史。开发源码回退须定点核自己的原字节/差异，不能批量还原无关修改。根Build2/RC1 Build29没有自动升级；实际身份看模块/SHA，不能只比Build名称。

[发行说明](ReleaseNotes.md) · [结果与恢复](../Ai/ResultsAndRecovery.md) · [开发发布](../Developer/VersionRelease.md)
