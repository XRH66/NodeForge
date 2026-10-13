> 当前接入身份：Build137 Repair5 / Ai1 / Mcp10R1；Windows x64官方UE5.3.2。正文功能说明继承Mcp9R19；Build136、原stage/PID/验收路径只表示历史证据。当前安装和连接统一见[AI安装](../User/AiInstallation.md)。不要复制历史本机路径或环境配置。

# 开发、原理与无记忆接手

软件目标是把受控构图数据可靠变成可编辑资产，并通过统一正式接口查询状态/结果。当前核心Build136 Repair6/MCP资料NodeForge-Build136-Mcp9R19；阅读不授予开发、发布或资产写入权限。

## 先恢复项目状态

交付包先核InstallationManifest、ReleaseManifest和同版Manifest，从本页、架构与源码地图理解受测产品；原生源码在PluginStage/NodeForge/Source，正式工具在Scripts。安装包不包含维护库的根五文件、测试台账或内部原报告。进入完整维护库续开发时，才按1.md→AGENTS.md→ProjectContext.md→ProjectStatus.md→Handoff.md及当前有限计划/Review恢复现场；维护库原生源路径为Plugins/NodeForge/Source。

新账号需要重新核对实际 Goal、额度和工具权限。文档提供知识，不迁移权限，也不自动恢复旧任务。

## 理解已有实现

- **[原理与架构](Architecture.md)**
  理解调用链、草稿、状态、源资产保护和核心取舍。
- **[源码地图](SourceMap.md)**
  定位实际模块、关键入口及输入输出关系。

## 扩展与维护

- **[节点与特殊语义](NodeExtension.md)**
  区分普通节点配置、动态签名与需要核心支持的语义。
- **[业务扩展](OperationExtension.md)**
  同步共享逻辑、统一操作、正式入口和显式 AI 开放政策。
- **[验证与证据](TestingAndEvidence.md)**
  按影响选择必要验证，复用可靠旧证据。
- **[版本发布](VersionRelease.md)**
  更新帮助、schema、精确节点资料、差异和成套身份。
- **[恢复与交接](RecoveryAndHandoff.md)**
  保存当前事实、未验边界和下一条具体授权动作。

先定点读取需要改动的模块，不重扫全部历史，不自动续跑旧 Goal。
