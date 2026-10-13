## 下载

**支持环境：Windows 64 位 · 官方 Unreal Engine 5.3.2**

| 文件 | 用途 | 大小 |
| --- | --- | --- |
| [NodeForge 插件](https://github.com/XRH66/NodeForge/releases/download/v0.1.0/NodeForge-0.1.0-UE5.3.2-Windows.zip) | 在 UE 中使用；无需 Python 或 MCP | 约 1.8 MB |
| [NodeForge AI 接入](https://github.com/XRH66/NodeForge/releases/download/v0.1.0/NodeForge-AI-0.1.0-Windows.zip) | 可选，包含连接服务、运行环境和安装工具 | 约 28 MB |

两个包均为 **v0.1.0**，请配套使用。

## 本版功能

- 从 JSON 创建材质和 Actor 蓝图，并检查节点、参数和连线。
- 读取资源信息，将支持的节点图导出为 JSON。
- 预览修改差异，创建新副本。
- 通过 MCP 向 AI 提供节点资料和插件工具。

当前生成目录为 `Content/NodeForgeTests`，已有资产通过新副本进行调整。本版暂不支持任意 UE 节点、蓝图多图或新增组件。其他 UE 版本和平台尚未适配。

<details>
<summary>附件说明</summary>

- `ReleaseManifest.json`：插件与 AI 的版本、适用环境和文件校验值，供安装工具读取。
- `Checksums.sha256`：检查下载文件是否完整。
- GitHub 自动生成的 `Source code`：源码归档，供开发者使用。安装插件请选择上面的插件 ZIP。

</details>

