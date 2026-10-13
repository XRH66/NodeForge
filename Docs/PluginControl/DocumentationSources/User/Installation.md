# 安装：先选你的使用方式

只在UE里载入JSON、检查输入和生成资产，选择普通插件。需要让AI查询节点、读取资源或提交任务，再配置AI接入。

| 我想怎样用 | 从哪里开始 | 当前试用方式 |
| --- | --- | --- |
| 在UE面板里手动操作 | [普通插件安装]({{page:UserPluginInstallation}}) | Build137 Repair5普通候选：复制文件夹、启用插件、首次确认当前工程；不用安装Python |
| 让AI操作插件 | [AI接入安装]({{page:UserAiInstallation}}) | 现有Build136 Repair6完整包：准备MCP运行环境和客户端连接 |

## 两种交付的进度

普通候选已支持工程Plugins安装与UE内首次确认，实际路径由当前工程和引擎自动取得。请复制完整Build137普通包；旧Build136的PluginStage单独复制仍不适用。

AI自带运行环境和一次配置包尚未发布。现有AI方式继续使用Build136完整候选与原准备流程，普通包不包含MCP SDK环境。

本次增加普通安装候选说明。原完整产品/MCP受测基线仍Build136 Repair6，新普通安装验证范围见候选包说明；不据此扩大节点、运行或打包结论。

## 安装后做什么

1. 按[开始使用]({{page:UserGettingStarted}})完成第一次输入检查。
2. 准备JSON时参考[完整示例]({{page:Examples}})，生成到新的目标位置。
3. 使用AI的任务步骤见[AI使用说明]({{page:UserAiWorkflows}})。
