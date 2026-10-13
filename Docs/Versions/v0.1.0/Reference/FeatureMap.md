# 功能一览

NodeForge 根据 JSON 创建可继续编辑的 UE 材质和 Actor 蓝图，也能读取已有资源、导出图结构并准备新副本。v0.1.0 支持 Windows 64 位和官方 UE 5.3.2。

第一次使用，先完成[插件安装](../User/PluginInstallation.md)，再跟随[入门教程](../User/GettingStarted.md)生成一个材质。希望由 AI 操作时，继续[连接 AI](../User/AiInstallation.md)。

## 可以做什么

| 任务 | 在插件中操作 | AI 使用的接口 | 说明 |
| --- | --- | --- | --- |
| 查询节点和规则 | 规则查询、节点目录 | `rules.query`、`catalog.get` | 查看允许的节点、属性和引脚 |
| 准备生成内容 | 从文件载入或粘贴 JSON | `draft.loadFile`、`draft.replace`、`draft.get` | 载入草稿后，可以继续修改；不会立即创建资产 |
| 检查输入 | 检查 | `graphs.preflight` | 检查结构、节点、连线和依赖，修改报错后再生成 |
| 创建材质或蓝图 | 生成新资产 | `graphs.generate` | 创建新资产，不覆盖已有资产 |
| 创建材质实例 | 载入实例 JSON 后生成 | `graphs.preflight`、`graphs.generate` | 需要先读取父材质的参数和文件摘要，详见[材质实例](MaterialInstance.md) |
| 查找资源 | 通过 AI 查询 | `resources.query` | 查询允许目录中的贴图、材质和材质函数 |
| 读取资源内容 | 检查一个选中资源 | `resources.inspect` | 读取材质参数或函数引脚，供后续引用 |
| 导出已有图 | 导出选中图到草稿 | `graphs.export` | 支持普通材质和 Actor 蓝图；原资产保持不变 |
| 准备修改副本 | 准备新副本、预览差异 | `draft.prepareCopy`、`graphs.preview` | 设置新的目标和请求编号，生成前查看变化 |
| 布局节点 | 生成时自动布局、重排新副本 | 生成流程中的自动布局 | 保留显式指定的节点位置；重排使用上次成功输入 |
| 查看结果 | 打开资产、查看报告 | `calls.get`、`report_read` | 查看任务进度、编译错误、保存结果和资产路径 |
| 阅读说明 | 文档 | `docs_index`、`docs_read` | AI 可读取本版本的使用方法与接口资料 |

表中的操作名称用于说明插件能力；完整 MCP 工具名和参数见[API 参考](../Ai/ApiReference.md)。

## 使用范围

- **材质：** Surface 材质，支持 Opaque、Translucent、Masked，以及 Unlit、DefaultLit。详见[材质](Material.md)。
- **蓝图：** 普通 Actor 的单个 EventGraph，支持 BeginPlay、Tick、条件分支、顺序执行、变量及目录中的函数。详见[蓝图](Blueprint.md)。
- **节点目录：** 包含 71 类材质节点和 327 条蓝图配置。节点仍标为实验性，使用时需要预检，并检查生成结果。
- **资源保护：** 创建目标必须位于 `/Game/NodeForgeTests/` 允许的子目录中；同名资产或重复请求编号会被拒绝。
- **结果检查：** 预检、编译、保存、运行效果是不同步骤。生成后请打开资产检查；需要确认磁盘保存结果时，使用[保存检查与错误处理](ResultsAndRecovery.md)中的方法。

当前不支持任意 UE 节点、自定义蓝图父类、多图、宏、Timeline、额外组件、任意资源导入或覆盖现有资产。具体输入限制以对应节点页为准。
