# 下载 NodeForge

**v1.1.2 · 官方 UE 5.3.2 · Windows 64 位。** 普通插件独立使用；AI 接入包是可选补充，没有第二份核心 DLL。

## 插件与 AI 接入包

| 使用方式 | 下载 | 下一步 |
| --- | --- | --- |
| UE 面板操作 | [NodeForge-1.1.2-UE5.3.2-Windows.zip](https://github.com/XRH66/NodeForge/releases/download/v1.1.2/NodeForge-1.1.2-UE5.3.2-Windows.zip) | [复制到引擎目录](PluginInstallation.md) |
| AI 操作插件 | [NodeForge-AI-1.1.2-Windows.zip](https://github.com/XRH66/NodeForge/releases/download/v1.1.2/NodeForge-AI-1.1.2-Windows.zip) | [一次接入](AiInstallation.md) |

先保存并关闭使用插件的编辑器。普通用户将 ZIP 中的 NodeForge 文件夹复制到引擎 `Engine/Plugins/Marketplace`，然后在工程启用。AI 包解压到临时目录，按指南指定引擎与客户端；环境补装到同一插件位置。

准确大小和 SHA256 以同页 [ReleaseManifest.json](https://github.com/XRH66/NodeForge/releases/download/v1.1.2/ReleaseManifest.json) 与 [Checksums.sha256](https://github.com/XRH66/NodeForge/releases/download/v1.1.2/Checksums.sha256) 为准。GitHub 的 Source code 附件是源码，不能代替插件 ZIP。

仓库和发行附件已公开，无需登录。下载失败时可从 [Release 附件列表](https://github.com/XRH66/NodeForge/releases/tag/v1.1.2) 下载；官网与安装包均可公开访问。

## 编写规则文档

想让 AI 写蓝图或材质的 JSON 时，按目标下载对应的一份 Markdown，连同需求一起发给 AI。规则文档是纯文本，不需要安装，也不包含插件本体；单独一份就足够，不必再读其他文档。

| 想生成 | 规则文档 | 直接下载 Markdown |
| --- | --- | --- |
| 材质 | [材质编写规则](../Reference/MaterialRules.md) | [MaterialRules.md](https://github.com/XRH66/NodeForge/raw/main/Docs/Versions/v1.1.2/Reference/MaterialRules.md) |
| Actor 蓝图 | [蓝图编写规则](../Reference/BlueprintRules.md) | [BlueprintRules.md](https://github.com/XRH66/NodeForge/raw/main/Docs/Versions/v1.1.2/Reference/BlueprintRules.md) |

规则文档与插件安装包分开：它只说明 JSON 怎么写、能使用哪些节点，不包含可执行文件。在 GitHub 文件页面也可以点右上角的 **Download raw file** 下载同一份内容。

需要 AI 自己查询规则并提交任务时，再安装上面的 AI 接入包；只让 AI 写 JSON 时不需要它。
