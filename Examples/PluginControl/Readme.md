# 程序调用示例

程序调用适合需要自动化操作 NodeForge 的工具或 AI。普通使用从 [面板入门](../../Docs/GettingStarted.md) 开始；使用 MCP 时先按 [AI 安装指南](../../Docs/AiInstallation.md) 配置客户端。

## 选择接口

| 需求 | 接口 |
| --- | --- |
| 在已运行的 UE 中读状态、编辑输入和生成 | [编辑器控制示例](EditorControl.md) |
| 查询节点规则和完整目录 | [目录查询示例](RulesCatalog.md) |
| 通过 MCP 调用 | [AI 调用手册](../../Docs/Versions/v0.1.0/Ai/Readme.md) |
| 检查已保存资产 | [结果与恢复](../../Docs/Versions/v0.1.0/Ai/ResultsAndRecovery.md) |

CLI 为 `Scripts/NodeForgeControl.py`。AI 包的 Python 位于安装插件目录的 `Tools/NodeForgeMcp/Runtime/python.exe`；参数可以从 CLI 的 `--help`、`editor-call --help` 和 `execute --help` 查看。

## 先准备当前工程

用安装包中的 `InstallNodeForgeAi.ps1` 指定真实 `.uproject`，需要启动和连接时添加 `-Connect`。安装结果会提供配置、记录及后续命令的位置。

后续调用使用本次准备结果中的项目、插件目录和会话文件。不要复制另一个工程的配置或把示例中的注册记录当成正在运行的会话。

## 调用结果

每个修改操作使用新的调用编号，并保留输入、输出和报告。依赖草稿的操作应提交当前 `draftRevision`；如果用户在界面中编辑了输入，先重新读取并确认内容，再继续。

`waitingPersistence` 表示资产已保存。需要检查重启后的内容时，用原输入和原生成报告执行保存检查，不重新生成。遇到 `pending` 或 `unknown`，先查询同一个调用编号。

## 本目录 JSON 文件

`BlueprintLoad.json`、`MaterialLoad.json` 和 `EditorRegistration118.json` 是开发时保留的参数结构示例，可能包含只在维护者环境有效的路径、版本或目录摘要。它们不是 v0.1.0 的即用配置，不要复制后直接启动。

新配置由安装工具创建；新输入使用当前目录规则。仓库没有附带这些示例对应的 UE 工程。

[CLI 协议](../../Contracts/ControlProtocol.md) · [编辑器协议](../../Contracts/EditorControlProtocol.md)
