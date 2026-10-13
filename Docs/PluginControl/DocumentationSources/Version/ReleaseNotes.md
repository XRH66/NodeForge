# 本期发行说明

## 当前候选与新增能力

本手册对应 {{bundleId}} / {{buildId}}，桥 {{bridgeVersion}} / SDK2.3.0 / ABI1。目录包含 {{materialCount}} 材质类、{{blueprintCount}} 蓝图精确签名；目录状态仍experimental。工具数以同包ToolContract为准。

新增Texture2D、MaterialFunction、Material有界元数据查询、Texture2D必要精确检查及显式安装/工程绑定。查询列表不加载对象、不建立来源证明；发现、允许读取、可引用、整图可导出分别表达。resources.inspect、GraphCodec共同描述及原生生产核心复用，未新增JSON版本、逐节点handler或第二控制层。

JSON2/3、Surface Masked/OpacityMask与四设置、七类Value成员属性、普通Actor BeginPlay/ReceiveTick、完整两族导出、来源编辑/准备/差异保持。导出遗漏formatVersion默认2，显式3返回完整v3；准备、预览、重排保持草稿格式。实例仍无专用MCP或实例图导出。

## 本候选实际验证范围

| 层次 | 证据和边界 |
| --- | --- |
| 正式资源与生产 | 三族查询/精确检查；一件引用纹理和函数、带空间遮罩的Material，正式生成/编译/save；同原JSON、原收据、Repair3同stage，在新PID原CLI独立Verify通过，无Import/resave |
| 来源与新副本 | 同保存资产完整默认2/显式3导出，原payload严格载入预检、retain设置编辑/准备/Settings-only差异通过；未再生成第二件 |
| Repair6字节 | 原生80文件与Repair4逐字一致；仅明确的候选构建参数去除编译器绝对源码路径前缀，原共享预编译头实际重建。新DLL的实际模块、原生目录和相同固定Masked GPU/独立PNG检查通过；正式生成/Verify仍准确引用Repair3，无生成器副本/Win64包不含该DLL，原证据保持 |
| Repair4字节 | 仅CandidateVisualCheck渲染宿主IsClient条件修复；其它79原生源与正式生产核心连续，原生成/Verify保持Repair3证明，不冒称Repair4重新生成 |
| 干净安装 | 独立解压、29个锁定离线依赖、新私有绑定/注册；实际从解压位置加载Repair4 DLL，正式catalog/query/inspect/load及必要面板/材质打开通过；原占用和未证明来源正确拒绝；停止通道、正常退出、原receipt逐字回退通过 |
| 实际画面 | 原一件Masked材质用GPU读取固定PNG，对事先冻结的遮罩内外采样通过；设置/保存与画面分层记录，不推广TAA/LOD |
| 无生成器 | 唯一隔离副本只复制原Material和旧Tick Blueprint，新Map仅存在副本；原资产SHA/实际依赖核对、无NodeForge模块的native游戏运行通过 |
| Win64交付 | 一套Development cook/package，目录及实际pak无生成器依赖；包后实际游戏进程、BeginPlay归属和固定遮罩像素通过。未测试bootstrap点击、第二台机器或其它平台 |

旧Tick100厘米/秒的持续速度依据Build134有限BT原三时刻/两个实际Delta区间，代码/资产未变；本候选包后观察只核归属和画面，未重新测速度。普通节点、旧双图、SDK安全整批按未变范围复用。

## 已完成基础与兼容

- Build135 Repair1：统一资产设置有限AS已终审完成；材质/蓝图各一件、独立Verify、两格式完整导出、来源/准备/预览和必要Details/实例编辑分层通过。
- Build134：有限BT已完成；BeginPlay/Tick唯一性与共存、持续X移动的实际Delta验收通过。
- Build133 Repair2：有限J已完成；现代2/3共用GraphDocument和原生核心、实例transient参数/父SHA检查通过。旧133继续允许原2/3，不支持Tick或新增设置。
- Build132：有限N已完成；71材质类/326蓝图签名的资格和代表图通过，旧132保持原v2，不开放v3/Tick/新增设置。
- Build130/131：有限B/C读取、完整导出、来源、新副本和差异的既有证据按原版本保持；旧S完整同PID双图属于Build105，相关源码连续性到118，118没有重新完成全测。

## 使用、恢复与交付状态

安装从[解压与安装]({{page:UserInstallation}})开始；AI从[准备]({{page:AiPreparation}})和[资源选用]({{page:AiResources}})进入。源应先正式受控读取，再手动打开；未证明来源不能自动重载或保存。pending/unknown先查原call与原报告，停止依赖写入，不换身份重发。

成套升级/回退见[兼容与迁移]({{page:VersionCompatibilityAndMigration}})，原输入、调用、收据、失败和冻结资料保留。根Build2与RC1 Build29未升级。Manifest.publicationStatus区分资料候选与最终交付；本次内容、MCP/Site同步及公开附件隐私终审完成前，不将中间ZIP称最终公开包。
