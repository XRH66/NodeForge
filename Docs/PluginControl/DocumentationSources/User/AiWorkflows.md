# 让 AI 通过 MCP 使用

首次配置见[AI接入安装]({{page:UserAiInstallation}})。只用UE面板时选择[普通插件安装]({{page:UserPluginInstallation}})，不用添加MCP。

## 1. 准备插件与工程

先由操作者确认当前版本，并准备专用工程和活动会话。MCP 不会自动启动 UE 或抢占控制。

没有启动 UE 时，AI 可以读取手册；草稿、生成和当前编辑器状态等业务需要已准备的活动会话。

## 2. 在客户端添加 MCP

先按[AI接入安装]({{page:UserAiInstallation}})生成本次 Commands.json。使用返回的 commands.mcp 字段填写客户端；下面的路径是格式占位，不能直接提交。按顺序逐项填写，路径只复制内容本身。

**名称：** nodefroge

**类型：** STDIO

**启动命令：**

`ReplaceWithInstallationRoot/Tools/NodeForgeMcp/Environment/Scripts/python.exe`

**参数 1：**

`ReplaceWithInstallationRoot/Scripts/NodeForgeMcp.py`

**参数 2：**

`--config`

**参数 3：**

`ReplaceWithCurrentPreparationDirectory/Config.json`

**工作目录：** `ReplaceWithInstallationRoot`

**环境变量：** 留空

**环境变量传递：** 留空

**启动超时：** 30 秒

**工具超时：** 200 秒

三个参数各占一个输入框，不合并成一格，也不粘贴 Config 文件内容。若界面没有超时字段，需按客户端实际配置文件设置。

保存并启用后，让 AI 读取版本索引确认连接。当前候选是否已发布和重连，仍以项目状态为准；开关颜色不能证明版本已经加载。

其它通用客户端使用相同的 STDIO 命令与参数，需支持实际协商的 tools 和 structuredContent。已载入桥0.3.2后，维护者成套批准兼容的插件应用代码、DLL和资料更新，下一次调用可自动接续，无需逐版开关。AI必须实际读索引确认；活动UE或未决调用会暂时阻止换版。SDK、解释器或连接核心不兼容升级才需要服务级重新载入。普通用户不用理解内部租约来使用插件。

## 3. 给 AI 下达任务

可以直接使用下面的说法：

> 先读取 NodeForge 版本索引和 AI 手册，说明当前支持的材质节点与限制。核对准备条件后，用分配的新目标生成 StaticColor 样例，保留完整原结果，并说明保存后还需要什么独立核验。

请求新任务前，让 AI 先说明是否受支持、需要哪些人工准备。会话注册、批次分配、JSON 参数和 Verify 命令由 AI 查专用手册。

## 4. 判断 AI 的结果

AI 应报告准确版次、新目标、草稿检查、原保存结果，以及尚未验证的层级。查询成功或 waitingPersistence 都不能被说成资产全部完成；结果未知时应停写并查询原调用。

资源读取、普通材质和 Actor 蓝图的受控完整图导出已有正式 MCP 操作。可以这样说明任务：

> 读取我提供的准确源资产路径，说明参数或支持的图结构；使用本批次分配的新身份导出完整 JSON，显式绑定原导出来源后载入、预检并生成一个新副本。保留原资产和完整结果，说明独立保存核验是否完成。

AI 按[显式读取与新副本配方]({{page:AiAssetRead}})核前置和原结果。操作者显式准备当前专用编辑器及新身份；已获任务授权的助手可沿准备工具完成操作。MCP不会自行启动引擎或猜源路径。副本准备与差异预览分别使用正式 draft.prepareCopy 和 graphs.preview；重排仍没有独立 MCP 操作。

## AI 专用资料

- [AI 调用手册]({{page:AiReadme}})
- [工程、会话和身份准备]({{page:AiPreparation}})
- [任务与调用例子]({{page:AiTaskRecipes}})
- [结果与恢复]({{page:AiResultsAndRecovery}})
