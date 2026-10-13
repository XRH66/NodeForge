# 连接前的准备与保存核验

NodeForge v0.2.0 支持 Windows 64 位和官方 UE 5.3.2。AI 接入包包含 Python 3.12.4、MCP SDK 2.3.0 和所需依赖，无需另装 Python。

## 安装并连接工程

1. 关闭使用插件的 UE 编辑器，下载匹配两包。
2. 运行 `InstallNodeForgeAi.ps1`，指定 `-EngineRoot` 与客户端配置，不需要工程参数。
3. 检查安装收据、工具和文档，必要时刷新客户端。安装器不会启动 UE。
4. 正常打开已启用插件的工程并完成首次确认。
5. 调用 `nodeforge_connect`；唯一候选自动连接，多候选显式选择。只对 `connected` 读取返回句柄。日常无需重装。

完整命令与参数见 [AI 安装指南](../User/AiInstallation.md)。安装 AI 需要下载、运行程序和写入客户端配置的权限。MCP 协议支持本身不会赋予这些权限。

已有插件时只补装 AI 部分。安装工具会检查插件版本、工程和引擎；主要输入为 `-EngineRoot`，高级工程级安装再传 `-Project`。

## 准备一次生成

选择未占用的 `requestId` 和完整目标路径。当前写入范围是 `/Game/NodeForgeTests/`；根下允许 0、1 或 2 级子目录，每段为 1–16 个 ASCII 字母或数字，资产叶名和请求 ID 为 1–64 个 ASCII 字母或数字。即使不同目录使用同一叶名，完整目标和请求 ID 仍必须唯一。

从 `nodeforge_draft_get` 读取当前修订。每次业务操作先保存一个新 `callId`，只在查询该操作时复用它。调用 ID 允许 1–64 个 ASCII 字母、数字或连字符。

保存实际提交的 JSON、调用返回、报告位置和 SHA。后续检查必须针对这次输入与生成结果，不能使用示例中的会话或报告 ID。

## 正常结束连接

正常关闭 UE 会结束本会话；面板也可停止本工程 AI。高级自动化可读取本会话原准备收据，使用原 CLI 的 `editor-commands` 与 `close-editor` 停止并正常关闭准确进程。正常重开后重新连接，不再运行安装器；不要复用旧句柄。

若状态仍是 `pending`、`unknown`，或无法确认会话已关闭，先保留安装记录并查询原调用，不要强制删除连接文件。

## 检查资产保存后能否重新打开

生成报告的 `waitingPersistence` 表示资产已保存，独立重开检查尚未执行。需要这个结论时，先正常结束生成时的编辑器并确认目标工程没有其他进程占用，再使用同一插件执行 `graphs.verifySaved`。

以下 PowerShell 命令在**实际安装的插件目录**运行，例如进入引擎的 `Engine/Plugins/Marketplace/NodeForge`。将三个输入变量换成这次实际文件位置：

```powershell
$projectFile = '工程的 .uproject 文件绝对路径'
$inputFile = '这次生成所用 JSON 文件的绝对路径'
$generationReport = '这次生成返回的原生报告绝对路径'
$pluginDirectory = (Get-Location).Path
$projectDirectory = Split-Path -Parent $projectFile
$verifyDirectory = Join-Path $projectDirectory ('Saved/NodeForge/Reports/Verify/' + [guid]::NewGuid().ToString('N'))

& .\Tools\NodeForgeMcp\Runtime\python.exe .\Scripts\NodeForgeControl.py execute `
  --operation graphs.verifySaved `
  --project $projectFile `
  --plugin-directory $pluginDirectory `
  --run-directory $verifyDirectory `
  --input $inputFile `
  --generation-report $generationReport `
  --timeout-seconds 240
```

输入文件须在插件允许的目录内；生成报告使用本次连接返回的原文件，位于该工程的 `Saved/NodeForge/Reports/` 内。`verifyDirectory` 必须是新目录。CLI 会启动独立进程，从同一插件读取操作定义，并核对原始输入、生成报告和已保存资产。不要重新生成或保存资产来代替检查。

检查返回中的原生状态、`validation.persistence`、回读结果、目标与文件 SHA。独立进程检查通过后，才可以说明资产保存后能够重新加载；游戏行为、材质画面和打包仍需在对应环境观察。
