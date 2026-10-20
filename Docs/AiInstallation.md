# 连接 AI

AI 接入让支持 MCP 的客户端查询 NodeForge 规则、读取资源和生成资产。适用于 **NodeForge v1.1.2、官方 UE 5.3.2、Windows 64 位**。接入包自带 Python 和固定依赖，无需另装运行环境。

**配置一次，之后正常打开工程即可使用。** 不需要每次运行安装工具、填写启动参数或重新下载环境。

本文用于让 AI 直接操作插件。只让 AI 写 JSON 时不需要接入包：先下载两份规则中的对应一份（[材质编写规则](Versions/v1.1.2/Reference/MaterialRules.md)、[蓝图编写规则](Versions/v1.1.2/Reference/BlueprintRules.md)），按[让 AI 写出自己的 JSON](Versions/v1.1.2/User/ManualWorkflows.md#让-ai-写出自己的-json) 的步骤做即可。

## 1. 下载接入包

从 [v1.1.2 Release](https://github.com/XRH66/NodeForge/releases/tag/v1.1.2) 下载 **NodeForge-AI-1.1.2-Windows.zip**，解压到临时文件夹。已装同版普通插件可以补装；尚未安装时，工具可取得同版普通包并安装到引擎目录。

仓库和发行附件已公开，可直接下载，无需 GitHub 账号。下载失败时先检查网络和所选版本。也可下载同页的普通插件 ZIP，安装时用 `-PluginZip` 指定本地文件。

## 2. 配置引擎和客户端

关闭使用这套插件的 UE 编辑器，再把以下任务交给具有本机操作权限的 AI。只需提供**引擎目录和客户端名称**，无需预先指定工程：

```text
请安装 NodeForge v1.1.2 并接入我的 AI 客户端。
指南：https://github.com/XRH66/NodeForge/blob/v1.1.2/Docs/AiInstallation.md
下载：https://github.com/XRH66/NodeForge/releases/tag/v1.1.2
引擎：填写官方 UE 5.3.2 安装目录
客户端：填写客户端名称
请使用包内安装工具和运行环境，找到客户端实际读取的配置文件，
保留其他配置，检查工具与文档是否可读取，并告诉我结果和安装收据位置。
```

需要手动执行时，在解压目录运行：

```powershell
& .\Scripts\InstallNodeForgeAi.ps1 `
  -EngineRoot '官方 UE 5.3.2 安装目录' `
  -ClientConfig '客户端实际配置文件绝对路径' `
  -ClientFormat 'codex-toml'
```

Codex TOML 使用 `codex-toml`；采用 `mcpServers` JSON 的客户端使用 `mcp-json`。工具将配套文件放入同一插件位置，客户端指向那里稳定的启动入口；临时解压目录不作为日常启动位置。

安装完成不自动打开 UE。没有工程运行时，能读取工具和文档就是正常结果。客户端可能需要刷新工具列表或重启才能载入新配置；安装工具无法代替客户端自身刷新。

## 3. 正常打开工程

在 UE 中启用 NodeForge，首次从 **窗口 → NodeForge** 确认工程信息。已接入 AI 的安装会为当前工程建立独立会话；普通插件单独安装不会开放 AI 通道。

先让 AI **读取 NodeForge 使用说明，再连接并查询当前工程状态**：

| 当前情况 | 结果 |
| --- | --- |
| 一个合格工程正在运行 | 自动连接，返回实际工程与会话身份 |
| 多个合格工程正在运行 | 返回候选；明确选择工程后连接 |
| 没有合格工程 | 返回 `notConnected`，仍可读取文档；等待正常打开工程 |
| 工程关闭后重新打开 | 重新连接取得新句柄；旧句柄失效，不会转到另一个工程 |

日常无需再运行安装工具，也不需要 `NFControl` 或 `NoLiveCoding` 启动参数。齿轮 → MCP → **停止本工程服务** 只停止当前工程通道；需要时可在齿轮 → MCP 点 **启用服务**。正常关闭编辑器会结束本会话。连接期间模块身份必须稳定，重新启用 Live Coding 或加载补丁会停止通道。

## 高级选项与恢复

| 参数 | 用途 |
| --- | --- |
| `-EngineRoot` | 官方 UE 5.3.2 安装目录，必需 |
| `-ClientConfig` | 客户端实际配置文件 |
| `-ClientFormat` | `codex-toml` 或 `mcp-json` |
| `-PluginZip` | 已下载的同版普通 ZIP |
| `-PluginDirectory` | 指定已有合法安装，例如引擎直接 `Plugins/NodeForge` |
| `-Project` | 工程级安装的高级输入；指定 `.uproject` 或唯一工程目录 |
| `-ServerName` | 默认 `nodeforge`；需要并存时可使用不同名称 |

安装引擎目录可能需要写入权限；日常 UE 和 MCP 不要求管理员。引擎插件与工程插件不能同时存在同名副本，工具不会擅自删除旧副本。

同版重复安装会保持核心和客户端配置。可识别旧版 NodeForge 条目可迁移，未知同名条目会拒绝覆盖。旧版升级步骤见[兼容与迁移](Versions/v1.1.2/Version/CompatibilityAndMigration.md)。

失败时先读结果中的 `receipt`，确认已完成步骤和当前文件。部分写入可用包内 Python 调用 `Scripts/NodeForgeAiInstall.py --resume-receipt <原收据>` 并提供原安装参数；已完成的同版 AI 文件更新使用 `--upgrade-receipt <原完成收据>`。这些入口检查实际字节，不用于覆盖不同核心或未知文件。不要删除锁、改目标盲重试或清除历史收据。

高级用法：访问受限的镜像或遇到 GitHub API 限流时，可使用进程环境变量 `NODEFORGE_GITHUB_TOKEN`；不要把令牌写入指南、客户端参数或公开报告。

## 交给另一个 AI 对话

当前 AI 确认没有未完成调用后，使用 `nodeforge_disconnect` 释放自己的连接。下一对话重新连接，取得新句柄；无需为交接关闭 UE。详见[连接交接](Versions/v1.1.2/Reference/SessionHandoff.md)。

批量 AI 任务请按[调用回收说明](Versions/v1.1.2/Reference/CallCapacity.md)了解自动回收与异常恢复，正常使用无需计数或手动清理。
