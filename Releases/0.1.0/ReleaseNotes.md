# NodeForge v0.1.0

NodeForge 的第一个版本。通过 JSON 生成可编辑的 UE 材质和 Actor 蓝图，也可以连接 AI 客户端来操作插件。

**支持环境：Windows 64 位 · 官方 Unreal Engine 5.3.2**

## 下载

| 文件 | 用途 | 大小 |
| --- | --- | --- |
| [NodeForge 插件](https://github.com/XRH66/NodeForge/releases/download/v0.1.0/NodeForge-0.1.0-UE5.3.2-Windows.zip) | 在 UE 中使用；无需 Python 或 MCP | 约 1.8 MB |
| [NodeForge AI 接入](https://github.com/XRH66/NodeForge/releases/download/v0.1.0/NodeForge-AI-0.1.0-Windows.zip) | 可选，包含连接服务、运行环境和安装工具 | 约 28 MB |

两个包均为 **v0.1.0**，请配套使用。已有普通插件时可以补装 AI 接入；首次通过 AI 安装时，工具会同时下载插件。

## 安装和开始使用

1. 解压插件 ZIP，把 `NodeForge` 文件夹放入工程的 `Plugins` 目录。
2. 打开工程并启用插件，从 **窗口 → NodeForge** 打开面板，确认工程位置。
3. 跟随 [入门教程](https://github.com/XRH66/NodeForge/blob/main/Docs/GettingStarted.md)，使用包内示例生成第一个材质。

[完整安装步骤](https://github.com/XRH66/NodeForge/blob/main/Docs/Installation.md) · [让 AI 安装并连接](https://github.com/XRH66/NodeForge/blob/main/Docs/AiInstallation.md)

[更新日志](https://github.com/XRH66/NodeForge/blob/v0.1.0/CHANGELOG.md) · [用户手册](https://github.com/XRH66/NodeForge/blob/v0.1.0/Docs/Versions/v0.1.0/User/Readme.md)

## 本版功能

- 从 JSON 创建材质和 Actor 蓝图，并检查节点、参数和连线。
- 读取资源信息，将支持的节点图导出为 JSON。
- 预览修改差异，创建新副本。
- 通过 MCP 向 AI 提供节点资料和插件工具。

当前生成目录为 `Content/NodeForgeTests`，已有资产通过新副本进行调整。首版暂不支持任意 UE 节点、蓝图多图或新增组件。其他 UE 版本和平台尚未适配。

<details>
<summary>附件说明</summary>

- `ReleaseManifest.json`：插件与 AI 的版本、适用环境和文件校验值，供安装工具读取。
- `Checksums.sha256`：检查下载文件是否完整。
- GitHub 自动生成的 `Source code`：源码归档，供开发者使用。安装插件请选择上面的插件 ZIP。

</details>

遇到问题请 [提交 Issue](https://github.com/XRH66/NodeForge/issues)，附上 UE 版本、操作步骤和错误提示。
