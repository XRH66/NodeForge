# 已知限制与适用范围

本包 NodeForge-Build136-Mcp9R13 / Build136 Repair6 / 桥 0.3.6。安装、固定Masked实际GPU、最小无生成器副本和Win64包后运行的有限范围已验证，详情见[发行说明](ReleaseNotes.md)。目录条目仍experimental；单个签名合格不等于所有组合、配置或运行均通过。

## 当前能力边界

生成仅在批准的`/Game/NodeForgeTests/`根下create-only。Material限Surface；Blueprint限普通Actor单EventGraph及已开放Value类型/对象上下文；不支持数组、循环、多图、新父类、任意函数/属性或额外组件。MaterialInstance须实际已保存父文件和真实参数/SHA，没有实例专用MCP或整图导出。

有界资源查询只支持Texture2D、MaterialFunction、Material。metadataUnavailable/complete=false/truncated等结果必须保留，不能把查询未列出当资产不存在。列表不加载对象；读取/引用/导出能力还需精确检查。资源必须saved/clean且具有当前正式受控加载证明；已手动加载无证明时拒绝sourceMemoryUnproven，不自动reload/resave。

运行、保存、独立Verify、GPU、cook和包后运行分别证明不同事实。当前有限图的遮罩通过不推广TAA/LOD、所有素材、全部节点组合、性能最大图、第二机/其它平台或实例MCP。旧Tick速度证据属于原有限BT，本候选未重复速度测试。

## 调用与恢复

MCP资料可离线读，业务要求本次专用编辑器/显式注册/独占控制。本机安装、启动、正常退出与新PID Verify由有CLI权限的操作者执行；MCP没有这些管理工具。关闭面板或桥不会替代通道StopReceipt和编辑器正常退出。

报告/调用大小和容量有界。pending/unknown、来源/身份冲突、目标占用或部分写入先查同一原call/报告，不清锁/强杀/抢owner或换request自动重发。新副本准备不预约，差异不证明编译、运行或画面；原地覆盖、merge和独立重排MCP未开放。

兼容应用与资料刷新需成套批准身份；SDK/解释器/Application ABI不兼容必须真实服务级载入，不能改SHA伪装。旧ProtocolSeen是首次观察，当前载入代次看serviceRuntime/configurationRefresh。详细步骤见[结果与恢复](../Ai/ResultsAndRecovery.md)和[兼容迁移](CompatibilityAndMigration.md)。

## 版本与资料状态

根Build2/RC1 Build29保持原安装。历史原报告留内部，公共资料只保留准确限定结论和干净模板。当前候选公开隐私/全文/附件与MCP/Site终审仍按实际Review完成，Manifest发布标记不能替代产品验收。
