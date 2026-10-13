# 普通插件安装指南

当前 **Build137 Repair5** 支持Windows x64、官方UE5.3.2，普通使用无需Python或MCP。

1. 从[同版Release](https://github.com/XRH66/NodeForge/releases/tag/build137-repair5)下载 `NodeForgeBuild137Repair5PluginCandidate.zip`，按Checksums核对。
2. 正常关闭目标UE工程，将ZIP中的整个NodeForge文件夹复制到工程Plugins目录；没有Plugins则创建。
3. 打开工程并启用NodeForge；若UE提示重开，按提示正常重开。
4. 从Window菜单打开NodeForge，核对首次页面识别的工程、引擎和插件，点击“确认当前工程并开始使用”。
5. 进入原功能页；同一工程重开免重复确认。

目标为 `<工程>/Plugins/NodeForge/NodeForge.uplugin`，旁边包含Binaries中的准确DLL和 `.modules`。GitHub源码ZIP不含编译文件。

`Examples/FirstMaterial.json` 可作参考；实际生成使用新的requestId和目标资产名，当前写根为 `/Game/NodeForgeTests` 子目录，原资产受保护。完整功能资料见[Mcp9R19用户指南](Versions/Build136/Mcp9R19/User/Readme.md)，安装章节区分137普通和136历史AI候选。

已有其它版本时先核原绑定/回退说明，不叠放第二个插件，不删除原资产绕过冲突；已运行编辑器不能借此切换引擎。匹配AI补装包尚未提供，见[AI状态](AiInstallation.md)。

当前仓库私有，下载与指南需要访问权限；公开安装入口尚未开放。
