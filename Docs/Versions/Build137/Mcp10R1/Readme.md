> 当前接入身份：Build137 Repair5 / Ai1 / Mcp10R1；Windows x64官方UE5.3.2。正文功能说明继承Mcp9R19；Build136、原stage/PID/验收路径只表示历史证据。当前安装和连接统一见[AI安装](User/AiInstallation.md)。不要复制历史本机路径或环境配置。

# NodeForge文档

将JSON构图描述转换为可编辑的UE材质和Actor蓝图。支持Windows x64、官方UE5.3.2；具体节点和配置以当前目录为准。

## 先选一种使用方式

- **[普通插件](User/PluginInstallation.md)**：在UE面板里载入JSON、检查并生成。无需配置AI；复制文件夹安装正在规划，当前候选仍需一次工程绑定。
- **[AI使用](User/AiInstallation.md)**：让AI发现节点、读取资源并提交任务。AI运行环境单独准备，自带环境的一次配置包尚未发布。

[安装方式对照](User/Installation.md)说明两者区别。安装完成后，从[第一次使用](User/GettingStarted.md)开始。

## 使用时查什么

- [用户手册](User/Readme.md)：安装、界面、手动操作与问题处理。
- [AI调用手册](Ai/Readme.md)：合法输入、正式调用与结果恢复。
- [完整JSON例子](Reference/Examples.md)：材质、蓝图及需要真实父依赖的实例配方。
- [功能与限制](Reference/FeatureMap.md)：支持的资产、操作与范围。

## 版本和开发

当前插件Build136 Repair6、桥0.3.6；本资料版次NodeForge-Build136-Mcp9R19。71材质类、327蓝图签名/8类，20个MCP工具。节点目录仍experimental，具体已验范围见[发行说明](Version/ReleaseNotes.md)。

本次仅改善安装文档和阅读入口，未改变插件功能。版本身份、迁移及回退见[版本资料](Version/Readme.md)；维护源码和扩展见[原理与开发](Developer/Readme.md)。

## 查节点和调用细节

[材质节点索引](Reference/MaterialNodeIndex.md) · [蓝图节点索引](Reference/BlueprintNodeIndex.md)

AI先调用nodeforge_docs_index确认客户端实际载入的版次，再按pageId读取AiQuickStart、AiPreparation及任务所需页面。网页更新不会自行更新客户端连接；不要仅凭本页版号判断已载入同版MCP资料。
