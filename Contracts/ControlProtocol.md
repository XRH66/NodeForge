# 命令行控制 API

CLI 位于 [NodeForgeControl.py](../Scripts/NodeForgeControl.py)。它在明确指定的工程和插件上运行预检、生成、独立保存检查，或查询已有调用记录。安装方法见[安装指南](../Docs/Installation.md)，图输入见[图形 JSON 协议](GraphProtocol.md)。

本协议的 Control API 版本为 `1.0`。支持 Windows 64 位和官方 UE 5.3.2，未知操作和 API 版本会被拒绝。操作定义由 [NodeForgeControlOperations.cpp](../Plugins/NodeForge/Source/NodeForgeEditor/Private/NodeForgeControlOperations.cpp)的同一张表提供，`describe` 返回实际加载模块支持的定义。

## 独立命令行操作

| 操作 | 参数 | 结果 |
| --- | --- | --- |
| `operations.describe` | 无业务参数 | 操作定义、工程、引擎和模块身份 |
| `graphs.preflightFromFile` | `inputPath` | 检查输入、目录和目标冲突；不创建资产 |
| `graphs.generateFromFile` | `inputPath` | 构建、编译和保存新资产；独立重开检查另行进行 |
| `graphs.verifySaved` | `inputPath`、`generationReport` | 在不同编辑器进程中核对已保存的资产；不重新生成或保存 |

活动编辑器使用 `editor-describe`、`editor-call` 和 `editor-status`，可用方法见[编辑器控制协议](EditorControlProtocol.md)。

## 调用参数

独立模式使用 `describe`、`execute --operation`、`status --run-directory`。执行时显式提供 `--project`、`--plugin-directory` 和新的 `--run-directory`；运行目录已存在时会拒绝执行，后续应查询原目录的状态。

`--input`、`--generation-report` 是相应业务文件参数，`--param NAME=VALUE` 按操作描述传递其他参数。未知或重复参数拒绝。可选 `--description-file` 必须匹配当前 DLL 路径、摘要和 API；省略时先运行一次只读 describe。

工程使用已建立的 `Config/NodeForgeInstallation.json`，插件使用自己的 `InstallationManifest.json`。客户端核对工程、引擎、插件路径与实际 DLL，而不是根据目录名称推断权限。独立 Commandlet 执行前，目标工程不能被其他 UE 进程使用。CLI 不创建工程或安装插件。

图输入最多 64 KiB，生成报告与原生结果最多 1 MiB。输入读取后保存字节快照和 SHA，后续处理不覆盖原文件。每个调用使用新的报告目录，报告不能覆盖。

底层参数包括 `-NFOperation`、可选 `-NFControlApiVersion=1.0`、`-NFReport` 及操作定义中的 `NFInput/NFPrevious`。不提供任意 Shell、console 或 NFMode 转发。执行与查询的标准输出是一份 UTF-8 JSON，帮助输出为文本；UE 日志保存在运行目录中。

`--timeout-seconds` 默认 240，必须是正有限数。它限制每次 UE 子进程等待时间；额外 describe 和操作系统查询各有自己的超时，不等于整条命令只等待一次该时长。

## 资源查询

活动编辑器的 `resources.query` 使用 `kind`、`directory`，可选 `nameContains/recursive/limit/after`。kind 仅支持 Texture2D、MaterialFunction 和 Material；目录最长 256 字符，名称过滤最长 128 字符且不区分大小写，recursive 默认 false，limit 为 1–200 且默认 50，after 为最长 512 字符的精确对象路径，默认空。

只能查询安装绑定允许的子目录，不能扫描 `/Game` 或 `/Engine` 根目录。查询读取磁盘元数据，不加载对象、修改草稿或生成资产。遍历最多 5,000 项、128 个目录、16 层。扫描中、元数据缺失、IO 问题、重解析点或超出上限时，返回 `complete=false/truncated=true`，不提供继续游标。完整结果按对象路径排序后分页；两次查询之间目录变化可能导致重复或遗漏。

查询行中的 `discovered`、`policyAllowed`、`inspected=false`、`requiresInspect=true`、`graphExportable=unknown` 分别说明不同条件。列表找到资源后，引用或导出前仍需精确检查。

## 结果和退出码

摘要包含 API、操作、运行目录、工程和模块信息、输入快照与摘要、`executionState`、原生报告引用、错误、警告、可能的残留及 `cliExitCode`。没有启动进程或解析出请求时，不会虚构 PID 或 requestId。

| CLI 退出码 | 含义 |
| --- | --- |
| 0 | 本操作取得约定结果；生成仍可能为 `waitingPersistence` |
| 2 | 参数、文件、方法或新运行目录等前置条件不满足 |
| 3 | 工程、插件、描述、报告路径或环境不匹配 |
| 4 | 插件业务或检查失败，具体原因在原报告中 |
| 5 | 仍在处理或结果未知，停止后续写入并查询原调用 |

`executionState` 使用 `rejected/environmentBlocked/finished/pending/unknown`。`finished` 表示 CLI 已结束，仍需按具体操作解释原生结果。原生报告缺失、损坏或超限可能发生在实际写入之后，因此返回 unknown，而不是把它当作写入前拒绝。

## 状态查询与中断

运行目录保存输入、启动记录、模块信息、退出记录、原生结果、摘要和日志。`status` 只读取已有结果与进程状态，不启动 UE，也不修改原摘要。

进程身份同时检查 PID、Windows 创建时间、命令行工程和实际模块；不能只凭 PID 判断旧任务仍在运行。超时会保留进程与 `LiveProcess` 记录，不强制结束 UE。原调用未明确结束前，不要重发生成。

晚到报告不等于已经取得退出记录。存在未知状态时，先查询原调用并检查报告、进程及部分写入。当前不提供自动取消、自动重试、清理资产或断电回滚。使用步骤见[程序调用示例](../Examples/PluginControl/Readme.md)。
