# 调用正在运行的编辑器

本页介绍 Editor API 1.0 的操作顺序。执行前先按 [AI 安装指南](../../Docs/AiInstallation.md) 准备工程，并使用安装工具的 `-Connect` 启动和连接。

## 准备参数

从本次安装结果及会话信息取得：

- 当前 `.uproject` 的完整路径。
- 当前插件目录。
- 本次运行的会话文件位置。
- 用于保存客户端结果的新目录。

CLI 会核对工程、模块和进程信息。会话文件含连接信息，传递路径即可，不要把文件内容贴到聊天或写入示例文档。

CLI 命令的一般形式如下，尖括号表示必须换成自己准备结果中的值：

```text
<Python> <NodeForgeControl.py> editor-call --operation <操作名> --call-id <新编号> --session-file <会话文件> --project <工程文件> --plugin-directory <插件目录> --run-directory <新结果目录>
```

需要参数时增加 `--arguments-file <参数JSON文件>`；依赖草稿时增加 `--expected-revision <当前修订号>`。可用参数以该版本 CLI 的 `editor-call --help` 为准。

## 读取并生成

1. 调用 `editor.getState` 确认工程状态。
2. 调用 `draft.get`，读取当前草稿及 `draftRevision`。
3. 将完整图 JSON 保存到允许访问的位置，用 `draft.loadFile` 载入；参数文件包含 `{"path":"输入文件的完整路径"}`。
4. 从载入结果取得新的 `draftRevision`。
5. 使用该修订号调用 `graphs.preflight`，读取检查报告。
6. 检查通过后调用 `graphs.generate`，保存返回的输入快照与原生报告位置。

每一步先检查退出值和具体操作结果，再执行下一步。读取查询报告不等于草稿已经载入，输入检查通过也不等于资产已经保存。

界面中编辑草稿会改变修订号。遇到修订不匹配时，重新读取当前输入并确认要做的修改，不要直接用新编号重发未知的生成操作。

## 查询已提交的操作

```text
<Python> <NodeForgeControl.py> editor-status --call-id <原调用编号> --session-file <会话文件> --project <工程文件> --plugin-directory <插件目录> --run-directory <新的查询结果目录>
```

`pending` 或 `unknown` 时先查原调用，保留已有文件和报告。不要反复更换资产名称或请求编号来重试。

## 结束连接与检查保存

使用安装记录提供的停止命令调用 `control.disable`，确认连接服务已停止，再正常关闭这次启动的编辑器。这个操作不等于关闭其他 UE 工程。

需要检查保存结果时，在新的编辑器进程中使用 `graphs.verifySaved`，提供原输入和原生成报告。完整参数与结果说明见 [准备和检查](../../Docs/Versions/v0.1.0/Ai/Preparation.md)。

保存检查会读取已生成资产，不重复生成或保存。材质画面、蓝图运行和打包仍需要各自的实际检查。
