# NodeForge文档

将JSON构图描述转换为可编辑的UE材质和Actor蓝图。支持Windows x64、官方UE5.3.2；具体节点和配置以当前目录为准。

## 先选一种使用方式

- **[普通插件]({{page:UserPluginInstallation}})**：在UE面板里载入JSON、检查并生成。无需配置AI；复制文件夹安装正在规划，当前候选仍需一次工程绑定。
- **[AI使用]({{page:UserAiInstallation}})**：让AI发现节点、读取资源并提交任务。AI运行环境单独准备，自带环境的一次配置包尚未发布。

[安装方式对照]({{page:UserInstallation}})说明两者区别。安装完成后，从[第一次使用]({{page:UserGettingStarted}})开始。

## 使用时查什么

- [用户手册]({{page:UserReadme}})：安装、界面、手动操作与问题处理。
- [AI调用手册]({{page:AiReadme}})：合法输入、正式调用与结果恢复。
- [完整JSON例子]({{page:Examples}})：材质、蓝图及需要真实父依赖的实例配方。
- [功能与限制]({{page:FeatureMap}})：支持的资产、操作与范围。

## 版本和开发

当前插件{{buildId}}、桥{{bridgeVersion}}；本资料版次{{bundleId}}。{{materialCount}}材质类、{{blueprintCount}}蓝图签名/{{blueprintClassCount}}类，{{toolCount}}个MCP工具。节点目录仍experimental，具体已验范围见[发行说明]({{page:VersionReleaseNotes}})。

本次仅改善安装文档和阅读入口，未改变插件功能。版本身份、迁移及回退见[版本资料]({{page:VersionReadme}})；维护源码和扩展见[原理与开发]({{page:DeveloperReadme}})。

## 查节点和调用细节

[材质节点索引]({{page:MaterialNodeIndex}}) · [蓝图节点索引]({{page:BlueprintNodeIndex}})

AI先调用nodeforge_docs_index确认客户端实际载入的版次，再按pageId读取AiQuickStart、AiPreparation及任务所需页面。网页更新不会自行更新客户端连接；不要仅凭本页版号判断已载入同版MCP资料。
