> 当前接入身份：Build137 Repair5 / Ai1 / Mcp10R1；Windows x64官方UE5.3.2。正文功能说明继承Mcp9R19；Build136、原stage/PID/验收路径只表示历史证据。当前安装和连接统一见[AI安装](../User/AiInstallation.md)。不要复制历史本机路径或环境配置。

# 新业务与统一接口

先核业务是否已经有共享Service/正式Operation或只有UI包装。新增业务应从核心实现/共享Service开始，在NodeForgeControlOperations唯一注册输入、host、dispatch、revision条件和结果；同步CLI入口/描述、busy及失败/原调用状态。UI和API用同Service，避免完成UI后再造一套自动化逻辑。

AI开放须明确评审：只读还是写入、副作用/对象根/身份/恢复、返回schema和结果判据、帮助/例子/资料pageId。ToolMetadata是显式正向政策，defaultAiExposure=deny；管理操作session.hello/control.disable不投影。build_tool_contract对未知tool要求明确adapter，不能让所有新native operation自动开放，也不能提供任意operation转发工具。

当前发布器安全地消费已有adapter；若真的添加新正式tool，先实现其受控输入schema/adapter/原结果路径和必要验证，再更新元数据/基线，不修改历史Freeze.py来冒充发布。保留原CLI/native字段，未知/等待和保存分层，不把业务结果压成一个passed。

同步FeatureMap、AiApiReference、任务/错误/恢复、结果schema、用户手动/AI可用性、SourceMap、版本差异与兼容。按影响一次代表性正式入口验收；原业务未变不重跑整批。DLL/业务/工具/schema/SDK变化新身份且服务实际重连，协商通知按实际协议支持，不能默认整个应用重启。

本版新增resources.inspect和graphs.export，保留四种既有原生模式，UI/API复用共享读取协调器；读取结果经固定报告注册和完整payload读取到显式草稿来源消费。ToolMetadata中的两个正向开放项与原生Operations共同生成schema，不能自动开放管理操作。副本预览、重排API及其它对象仍未开放；本页说明维护方式，不授予后续新增业务权限。
