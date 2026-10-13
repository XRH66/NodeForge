# 程序控制测试

本目录的运行器调用正式 CLI，并读取实际编辑器结果，检查接口、身份、草稿、超时和错误恢复。协议说明见[CLI](../../Contracts/ControlProtocol.md)与[编辑器控制](../../Contracts/EditorControlProtocol.md)，环境准备见[开发测试说明](../TestPlan.md)。

## 运行器

| 脚本 | 检查内容 |
| --- | --- |
| [RunCases.py](RunCases.py) | 按 Cases/RepeatCases 顺序预检、生成、在不同进程 Verify；pending 或失败后停止后续用例 |
| [RunBoundaries.py](RunBoundaries.py) | 参数、文件、身份与错误输入，使用独立负例副本 |
| [RunProcessChecks.py](RunProcessChecks.py) | 创建时间、超时、只读状态查询、错误进程身份和晚到结果 |
| [RunEditorChecks.py](RunEditorChecks.py) | 活动编辑器的状态、UI、协议、草稿、业务调用、资源查询及生命周期 |

InvalidJson 是故意无效的 JSON。expected 字段描述预期，不能把用例文件本身当作执行结果。失败批次中未执行的后续项目应保持 notRun。

## 参数与前提

前三个运行器提供 `--help`，使用显式 `--project`、`--plugin-directory`、`--report-directory`；涉及操作描述时还需要匹配 DLL 的 `--description-file`。RunCases 和 RunBoundaries 可以指定 manifest，RunCases 可指定等待时长。

RunEditorChecks 使用 `--phase`、`--project-name`、`--plugin-directory` 和 `--report-directory`；需要保护已有测试资产时，通过 `--asset-protection` 指定实际清单。不同 phase 的输入和编辑器前提不同，执行前先查看对应源码分支。

所有运行目录必须新建。脚本仍依赖开发工作区的路径规则和专用 UE 工程，不能直接把文档中的名称当作自己已有的工程。

## 判断结果

进程必须与指定工程、创建时间和加载 DLL 匹配；只收到 accepted 不算完成。unknown、断连或超时后查询原 callId，不重新 Generate。读取原报告、输入快照、实际保存文件和退出记录，再判断结果。

程序调用、真实面板操作、独立保存检查和实际运行分别记录。某个接口检查通过不代表其他入口也已执行。
