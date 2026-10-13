# 来源与第三方许可

NodeForge 使用 Unreal Engine 的公开接口创建、连接、编译和保存节点。UE 引擎需要由用户自行安装；插件下载包不包含完整引擎。

部分节点创建、连接和布局代码参考并改编自以下项目，原作者的版权声明和 MIT 许可随插件保留：

| 项目 | 使用范围 | 许可 |
| --- | --- | --- |
| ZiggyMar/unreal-mcp | 蓝图节点创建、编译诊断、变量绑定、保存与材质实例创建的部分实现 | [MIT 许可](https://github.com/XRH66/NodeForge/blob/v0.1.0/Plugins/NodeForge/Licenses/ZiggyMarMIT.txt) |
| ChiR24/Unreal_mcp | 材质节点连接的部分实现 | [MIT 许可](https://github.com/XRH66/NodeForge/blob/v0.1.0/Plugins/NodeForge/Licenses/ChiR24MIT.txt) |
| PRQELT/Autonomix | 节点布局的分层算法 | [MIT 许可](https://github.com/XRH66/NodeForge/blob/v0.1.0/Plugins/NodeForge/Licenses/AutonomixMIT.txt) |

更详细的文件来源说明见[插件第三方声明](https://github.com/XRH66/NodeForge/blob/v0.1.0/Plugins/NodeForge/ThirdPartyNotices.md)。AI 接入包还包含 Python、MCP SDK 及运行依赖，相关声明见[AI 依赖说明](../../../../Tools/NodeForgeMcp/ThirdPartyNotices.md)和下载包中的许可文件。离线查看原生代码许可时，打开已安装插件的 `Licenses` 目录。

第三方许可分别适用于相应组件，不改变 Unreal Engine 本身的许可要求。仓库尚未为整个 NodeForge 项目选择统一开源许可证。
