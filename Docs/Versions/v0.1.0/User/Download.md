# 下载 NodeForge

NodeForge v0.1.0 适用于 **Windows 64 位、官方 UE 5.3.2**。先安装插件；想让 AI 直接操作时，再补装 AI 接入包。两个包使用同一套插件功能。

## 选择安装包

### 普通插件

在 UE 面板中使用，包含插件、源码和首次材质示例。普通使用无需 Python 或 MCP。

[下载插件](https://github.com/XRH66/NodeForge/releases/download/v0.1.0/NodeForge-0.1.0-UE5.3.2-Windows.zip) · [安装指南](PluginInstallation.md)

`NodeForge-0.1.0-UE5.3.2-Windows.zip` · 1.68 MiB（1,760,515 字节）

SHA256：

```text
b41e36cca73635dbf0b10e3060d61e80f7df5d499fed9ea5003329338fa24267
```

### AI 接入包（可选）

自带 Python 3.12.4 和连接服务，可以在已安装插件上补装。未安装插件时，安装工具会取得同版插件。

[下载 AI 接入包（可选）](https://github.com/XRH66/NodeForge/releases/download/v0.1.0/NodeForge-AI-0.1.0-Windows.zip) · [安装指南](AiInstallation.md)

`NodeForge-AI-0.1.0-Windows.zip` · 26.29 MiB（27,569,711 字节）

SHA256：

```text
5d635f68bde6762c2e54329b9c8921a7d7b21159e9dcc5f1d168aa64c8d890fc
```

## 下载权限与备用入口

GitHub 仓库目前为私有：下载前需要登录有仓库访问权的账号。看到 404 时，先确认账号权限，再从 [v0.1.0 Release](https://github.com/XRH66/NodeForge/releases/tag/v0.1.0) 查看附件。

网站可见不代表 GitHub 附件可匿名下载。执行安装的 AI 也需要能取得附件；无法自动下载时，先用有权限的账号下载两个 ZIP，再提供本机位置。不要把账号令牌粘贴到网页或聊天中。

[查看 GitHub Release](https://github.com/XRH66/NodeForge/releases/tag/v0.1.0) · [版本清单](https://github.com/XRH66/NodeForge/releases/download/v0.1.0/ReleaseManifest.json) · [校验文件](https://github.com/XRH66/NodeForge/releases/download/v0.1.0/Checksums.sha256)

GitHub 自动生成的 `Source code` 是源码压缩包，不能替代插件或 AI 接入 ZIP。

## 安装后的下一步

1. [安装并启用插件](PluginInstallation.md)，从窗口菜单打开 NodeForge。
2. [生成第一个材质](GettingStarted.md)，使用附带示例检查、生成并打开结果。
3. 可选：[连接 AI](AiInstallation.md)，让客户端加载配置，再读取文档与当前工程状态。

完整支持范围见 [已知限制](../Version/KnownIssues.md)。
