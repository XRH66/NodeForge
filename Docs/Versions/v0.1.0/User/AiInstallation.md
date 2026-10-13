# 连接 AI

AI 接入让支持 MCP 的客户端调用 NodeForge，例如查询节点规则、读取工程资源和生成蓝图或材质。MCP 是 AI 客户端连接外部工具的一种方式。

本指南适用于 **NodeForge v0.1.0、Windows 64 位、官方 UE 5.3.2**。AI 接入包已包含 Python 和连接服务，无需单独安装运行环境。

## 1. 准备好工程和客户端

保存并关闭准备安装的 UE 工程。记下工程的 `.uproject` 文件位置，以及你使用的 AI 客户端。

用于安装的 AI 需要能下载文件、运行本机程序和写入客户端配置。仅支持 MCP、但没有本机操作权限的聊天窗口，无法自行完成安装。

## 2. 把安装任务交给 AI

将下面这段话复制给有本机操作权限的 AI，把工程路径和客户端名称换成自己的：

```text
请帮我安装 NodeForge v0.1.0，并连接到我使用的 AI 客户端。

安装指南：https://github.com/XRH66/NodeForge/blob/main/Docs/AiInstallation.md
下载页面：https://github.com/XRH66/NodeForge/releases/tag/v0.1.0
工程路径：填写我的 .uproject 文件绝对路径
AI 客户端：填写客户端名称

请先阅读安装指南，再使用接入包中的 InstallNodeForgeAi.ps1 完成安装、
配置和连接检查。保留客户端的其他配置，完成后告诉我连接是否成功。
```

已经安装普通插件的用户只需补装 AI 部分。尚未安装时，安装工具会下载同版插件。

如果 AI 无法直接下载，可以先下载 [插件 ZIP](https://github.com/XRH66/NodeForge/releases/download/v0.1.0/NodeForge-0.1.0-UE5.3.2-Windows.zip) 和 [AI 接入 ZIP](https://github.com/XRH66/NodeForge/releases/download/v0.1.0/NodeForge-AI-0.1.0-Windows.zip)，再把这两个文件的本机位置告诉它。

GitHub 仓库目前为私有：下载前需要登录有仓库访问权的账号。看到 404 时，先确认账号权限，再从 [v0.1.0 Release](https://github.com/XRH66/NodeForge/releases/tag/v0.1.0) 查看附件。

## 3. 确认连接

安装工具会启动 UE 并检查连接。返回 `connected` 表示它已经连接到指定工程，能够读取插件状态。

客户端可能需要刷新工具列表或重启后才会加载新配置。加载后，可以先让 AI **读取 NodeForge 的使用说明并查询当前工程状态**，确认能调用工具，再开始生成资产。

## 安装工具的用法

以下内容供执行安装的 AI 或需要手动配置的用户阅读。

1. 从同一 Release 读取 `ReleaseManifest.json`，核对版本、平台和下载文件的 SHA256。
2. 将 AI ZIP 解压到工程插件目录之外，例如单独的下载文件夹。
3. 确认目标工程已关闭，并找到客户端实际使用的 MCP 配置文件。
4. 在解压目录运行：

```powershell
& .\Scripts\InstallNodeForgeAi.ps1 `
  -Project '工程的 .uproject 文件绝对路径' `
  -ClientConfig '客户端配置文件绝对路径' `
  -ClientFormat 'codex-toml' `
  -Connect
```

| 参数 | 用途 |
| --- | --- |
| `-Project` | 指定 `.uproject` 文件；也可指定仅包含一个 `.uproject` 的工程目录 |
| `-ClientConfig` | 客户端实际读取的配置文件 |
| `-ClientFormat` | Codex 使用 `codex-toml`；使用 `mcpServers` JSON 配置的客户端选择 `mcp-json` |
| `-Connect` | 安装后启动 UE，检查连接与工程状态 |
| `-PluginZip` | 使用已经下载到本机的同版插件 ZIP |
| `-EngineRoot` | 工具无法自动识别引擎时，指定 UE 5.3.2 安装目录 |
| `-ServerName` | 默认是 `nodeforge`；客户端已有不同的同名配置时，可指定另一个名称 |

工具保留客户端的其他配置。结果中包含工程位置、安装目录、客户端设置和安装记录的位置。请使用安装记录提供的启动与停止命令管理这次连接；关闭编辑器后，再次运行安装入口会准备新的连接。

配置写入成功后，仍需确认客户端已加载它。当前支持上表中的两种配置格式；其他格式需要客户端提供相应配置方法。

## 安装失败时

- **工程正在运行：** 保存并关闭目标工程后再安装。
- **插件版本不匹配：** 核对是否使用本页同一版本的两个下载包。
- **客户端已有同名配置：** 确认原配置用途；需要保留时，使用另一个 `-ServerName`。
- **下载需要登录：** 可使用本地 ZIP；自动下载私有附件时，工具也支持进程环境变量 `NODEFORGE_GITHUB_TOKEN`。
- **安装中途失败：** 先查看结果中的 `receipt` 安装记录，确认已执行的步骤和错误原因，再处理。不要直接删除工程。
