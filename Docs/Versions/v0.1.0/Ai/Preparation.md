# 连接前的准备与保存核验

NodeForge v0.1.0 支持 Windows 64 位和官方 UE 5.3.2。AI 接入包包含 Python 3.12.4、MCP SDK 2.3.0 和所需依赖，无需另装 Python。

## 安装并连接工程

1. 保存并关闭目标 UE 工程。
2. 下载同一版本的插件与 AI 接入包，或让安装工具下载匹配插件。
3. 运行 `InstallNodeForgeAi.ps1`，指定 `.uproject`、客户端配置文件和格式，并加上 `-Connect`。
4. 检查安装结果和 `receipt` 安装记录。`connected` 表示工具已连接指定工程并读取状态。
5. 在客户端刷新工具列表，调用 `nodeforge_docs_index` 和 `nodeforge_connect`，取得这次连接的 `handle`。

完整命令与参数见 [AI 安装指南](../User/AiInstallation.md)。安装 AI 需要下载、运行程序和写入客户端配置的权限。MCP 协议支持本身不会赋予这些权限。

已有插件时只补装 AI 部分。安装工具会检查插件版本、工程和引擎；无法自动确定引擎时，用 `-EngineRoot` 指定安装目录。

## 准备一次生成

选择未占用的 `requestId` 和完整目标路径。当前写入范围是 `/Game/NodeForgeTests/`；根下允许 0、1 或 2 级子目录，每段为 1–16 个 ASCII 字母或数字，资产叶名和请求 ID 为 1–64 个 ASCII 字母或数字。即使不同目录使用同一叶名，完整目标和请求 ID 仍必须唯一。

从 `nodeforge_draft_get` 读取当前修订。每次业务操作先保存一个新 `callId`，只在查询该操作时复用它。调用 ID 允许 1–64 个 ASCII 字母、数字或连字符。

保存实际提交的 JSON、调用返回、报告位置和 SHA。后续检查必须针对这次输入与生成结果，不能使用示例中的会话或报告 ID。

## 正常结束连接

安装记录包含启动与停止命令。使用其中的 `editor-commands` 生成停止命令，确认控制通道已停止，再用 `close-editor` 正常关闭本次编辑器。下次运行安装入口会准备新连接；不要复用已经结束的进程或会话文件。

若状态仍是 `pending`、`unknown`，或无法确认会话已关闭，先保留安装记录并查询原调用，不要强制删除连接文件。

## 检查资产保存后能否重新打开

生成报告的 `waitingPersistence` 表示资产已保存，独立重开检查尚未执行。需要这个结论时，先正常结束生成时的编辑器并确认目标工程没有其他进程占用，再使用同一插件执行 `graphs.verifySaved`。

以下 PowerShell 命令在**已安装的工程插件目录**运行，例如先进入工程的 `Plugins/NodeForge`。将三个输入变量换成这次实际文件位置：

```powershell
$projectFile = '工程的 .uproject 文件绝对路径'
$inputFile = '这次生成所用 JSON 文件的绝对路径'
$generationReport = '这次生成返回的原生报告绝对路径'
$pluginDirectory = (Get-Location).Path
$verifyDirectory = Join-Path $pluginDirectory ('Reports/Verify/' + [guid]::NewGuid().ToString('N'))

& .\Tools\NodeForgeMcp\Runtime\python.exe .\Scripts\NodeForgeControl.py execute `
  --operation graphs.verifySaved `
  --project $projectFile `
  --plugin-directory $pluginDirectory `
  --run-directory $verifyDirectory `
  --input $inputFile `
  --generation-report $generationReport `
  --timeout-seconds 240
```

输入文件须在插件允许的目录内；生成报告使用本次连接返回的原文件，位于插件的 `Reports/` 内。`verifyDirectory` 必须是新目录。CLI 会启动独立进程，从同一插件读取操作定义，并核对原始输入、生成报告和已保存资产。不要重新生成或保存资产来代替检查。

检查返回中的原生状态、`validation.persistence`、回读结果、目标与文件 SHA。独立进程检查通过后，才可以说明资产保存后能够重新加载；游戏行为、材质画面和打包仍需在对应环境观察。
