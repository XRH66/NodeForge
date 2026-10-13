# 结果、错误与重复请求

本页说明 NodeForge v0.1.0 的结果读取方式。文件名中的 Draft 是开发时使用的名称；实际调用请按这里的操作区别和返回字段处理。

## 先看操作，再看状态

同样的 `succeeded` 在不同操作中表示完成了该操作：预检成功没有创建资产，准备副本成功没有保存资产，独立保存检查成功也没有重新生成资产。

| 操作或状态 | 实际含义 |
| --- | --- |
| `GraphPreflight` 成功 | 输入与依赖通过检查；尚未构建、编译或保存 |
| `PrepareNewCopy` 成功、`requestOutcome=notBuilt` | 新请求与目标已准备；资产未生成，名称未预留 |
| Import 的 `waitingPersistence` | 构建、编译和保存已完成，尚待独立重开检查 |
| Verify 的 `succeeded` | 已保存资产在新进程中检查通过；`verificationOnly` 表明没有重新生成 |
| `rejected` | 某项输入或操作条件不满足，查看具体错误 |
| `failed` | 执行或检查失败，报告保留失败步骤与已产生的结果 |
| `cleanupRequired` | 失败后仍有需要处理的残留 |
| `pending/unknown` | 尚不能确定终态，不应重复提交写入 |

`phase=finished` 只说明流程结束。`requestOutcome=built` 表示构建曾产生对象，不保证编译或保存成功。运行、材质画面和打包效果也不会因为生成成功而自动视为正确。

## 图生成与保存检查

资产流程分别记录 `validation`、`build`、`initialReadback`、`compile`、`finalReadback`、`publish`、`save`、`persistence`，运行检查为独立的 `runtime` 项。

前七项通过、但尚未独立检查保存文件时，返回 `waitingPersistence`。八项均通过且没有待处理残留时，资产流程可返回 `succeeded`。某一步实际失败时保留该错误，不以其他步骤成功覆盖。

保存后的 Verify 使用原输入、生成报告、目录、插件和文件摘要，在不同编辑器进程中读取并检查资产。它不重新 Import 或重新保存。

## 重要字段

| 字段 | 用途 |
| --- | --- |
| `requestId` | 图生成请求编号 |
| `inputDigest` | 原始 UTF-8 输入字节的 SHA256；空白变化也会改变摘要 |
| `executionFingerprint` | 引擎、插件、目录与执行条件，用于检查请求和结果是否匹配 |
| `sessionId` | 原生处理会话；不同于 MCP 会话或 callId |
| `requestOutcome` | 本次请求如何处理，例如 `notBuilt/built/conflict/alreadyCompleted` |
| `checks` | 各步骤的状态、原因和结果引用 |
| `assets/residue` | 资产路径、磁盘写入状态及可能的部分结果 |
| `diagnostics/nextAction` | 具体错误、诊断和处理建议 |

不同操作的报告格式不同。读取实际存在的字段，不要把某类生成报告的必填项强加到规则查询、目录读取或图导出上。当前没有实现设计草案中的全部阶段、取消、幂等恢复或故障处理枚举。

## 同名资产和重复请求

Import、GraphPreflight 和 GraphPreview 使用 `conflict.kind` 区分：

| 冲突 | 错误码 |
| --- | --- |
| `targetExists` | `NFERR-TARGET-EXISTS` |
| `requestExists` | `NFERR-REQUEST-EXISTS` |
| `targetAndRequestExist` | `NFERR-TARGET-AND-REQUEST-EXIST` |

已有资产不会被覆盖，也不会被标成这次失败新建的残留。当前没有通用的“重复提交即幂等成功”入口；Verify 返回 alreadyCompleted 表示检查已有结果，不意味着可以重复 Import。

发生超时或未知结果时，先查询原调用和残留。没有查清写入结果前，不要更换 requestId 或目标路径绕过冲突重试。

## 草稿、输入快照和副本

载入 JSON 只替换草稿。草稿编辑增加修订号，并使旧预检或预览结果失效。执行前保存完整 UTF-8 输入快照，构建时检查摘要；不会在生成中偷偷重读已改变的原文件。

界面报告的 `uiInput` 包含来源、修订、`snapshotPath/snapshotDigest` 等信息。这些属于报告，不是 Graph 请求可额外填写的根字段。

准备副本只更改 `assetPath/requestId`，`copyPreparation` 记录原值、新值和派生文件摘要，并明确 `assetGenerated=false/targetReserved=false`。后续生成仍需检查冲突。重排使用上次成功输入，不能把当前无关草稿当作原资产来源。

## 中断与容量

输入上限为 65,536 字节，外部报告读取上限为 1,048,576 字节。超限返回错误，不截断内容后继续执行。报告写入使用新的独占路径，不覆盖已有结果。

取消等待或客户端断开不会撤销 UE 已执行的操作。保留原调用、输入、报告和资产路径，按[编辑器控制协议](EditorControlProtocol.md)或[MCP 协议](McpProtocol.md)查询。当前不承诺自动资产回滚、断电恢复或并发资产构建。
