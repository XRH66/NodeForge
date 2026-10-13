# NodeForge 版本资料

本包 NodeForge-Build136-Mcp9R13 / Build136 Repair6 / 桥 0.3.6，官方MCP SDK2.3.0、ABI1、UE 5.3.2-29314046+++UE5+Release-5.3。当前 71 材质类、327 蓝图签名/8 类、20 MCP工具，其中 16 为业务投影。目录仍 experimental；准确取证层与交付状态看 Manifest.publicationStatus，不把本页生成或编译当真实业务通过。

本次增加 Texture2D、MaterialFunction、Material 的有界登记元数据查询、Texture2D精确检查，以及显式安装根/选定工程绑定。查询不加载对象，不将列表升级成来源或整图可导出证明；检查、引用、JSON2/3生产核心、来源、准备和差异沿原实现。首次安装看[安装](User/Installation.md)，AI完整配方见[资源选用](Ai/Resources.md)和[准备](Ai/Preparation.md)。

前期有限JSON、批量节点、普通Actor Tick、统一资产设置、资源读取/导出和新副本/预览保持；已有精确原证据按影响复用，不默认重跑全节点、双图或SDK整批。旧S完整同PID双图实测发生在Build105，源码连续性关联118，不能声称118重新全测。根Build2和RC1 Build29保持。保存、独立Verify、运行、画面、打包及包后运行分别判定；本包新增链路的实际证据以最终版本说明为准。

## 选择阅读入口

- **[使用手册](User/Readme.md)**
  插件介绍、开始步骤、界面按钮、手动操作和使用 AI。离线首页为 User/Index.html。
- **[AI 调用手册](Ai/Readme.md)**
  能力发现、准备条件、合法请求、正式调用和结果恢复。
- **[原理与开发](Developer/Readme.md)**
  项目目标、架构、源码地图、已有内容、扩展发布及无记忆接手。
- **[版本资料](Version/Readme.md)**
  准确身份、发行说明、兼容、迁移、回退和已知限制。

## 公共参考

- [功能与限制](Reference/FeatureMap.md)
- [材质节点索引](Reference/MaterialNodeIndex.md)
- [蓝图节点索引](Reference/BlueprintNodeIndex.md)

## MCP 阅读方法

MCP读本包：nodeforge_docs_index返回准确版次与非节点页；nodeforge_docs_read传本页pageId=Readme。所有本文链接的必需资料同时有pageId，路径是文件读者导航，不要求通用MCP读取任意文件。先取得索引，再按pageId读AiQuickStart、AiPreparation、AiTaskRecipes、AiAssetRead、AiResultsAndRecovery、DeveloperArchitecture、DeveloperSourceMap。节点族索引列精确身份页，不能跨版按序号认节点。

## 当前交付与边界

本包核心为 Build136 Repair6。既有Build132有限N统一原节点和新增普通节点的定义、能力档案与原生适配链：两限定函数库共同筛选、Surface Math 表达式共同筛选；MCP 沿原目录/规则工具发现，逐节点资料从准确原生目录同步，无逐节点 handler。两族目录保持 experimental；原生创建、组合编译保存、独立 Verify 和运行/画面/打包分别看[发行说明](Version/ReleaseNotes.md)。维护路线见 [NodeBulk](Ai/NodeBulk.md) 和 [扩展原理](Developer/NodeExtension.md)。

原 Build122/M/S/A、MCP P0–P4、Build130 有限B、Build131 有限C保留各自完成证据。resources.inspect/graphs.export、来源绑定及draft.prepareCopy/graphs.preview/retain来源编辑是既有能力，本期不新增这些业务。C完整新副本和新PIDVerify来自CopyPreview1/C3/Run1，B完整读取/导出来自AssetRead1，不能自动成为新增节点的实测。完整配方仍由 AiAssetReadExamples/AiCopyPreview 导航。S完整同PID双图属于105，核心连续性118，118未重新全测。

根Build2和RC1 Build29未升级。本候选已分层验证一件Masked实际画面、唯一最小无生成器副本和Win64包后运行；未验边界仍包括TAA/LOD、其它组合、第二机/其它平台和实例MCP，详细范围见发行说明。新包从维护源生成，不原位覆盖旧frozen包。桥兼容代码/成套配置的接续机制保留，客户端真正载入结论须来自本期实际调用，发布器不建立运行证据。


BT02普通Actor事件与持续移动已完成有限产品验收；完整JSON、独立时间配方、正式MCP/Verify/来源链及兼容/回退见[BlueprintTick](Ai/BlueprintTick.md)。
