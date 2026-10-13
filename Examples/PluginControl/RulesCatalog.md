# 查询节点规则后生成资产

查询可以帮助工具或 AI 获得当前插件接受的节点、属性和引脚。先按 [编辑器控制示例](EditorControl.md) 准备连接，并读取当前 `draftRevision`。

## 1. 查询任务需要的节点

`rules.query` 的参数示例：

```json
{"assetKind":"Material","query":"VectorParameter ScalarParameter"}
```

蓝图示例：

```json
{"assetKind":"Blueprint","query":"ReceiveBeginPlay Concat_StrStr PrintString"}
```

空查询返回该类全部规则。多个词用空格分隔，匹配任意一个词即可；查询最多 256 个字符、8 个词。查询结果是规则说明，不是可直接生成的资产 JSON。

通过 CLI 调用时，把参数保存为文件，传给 `editor-call --operation rules.query --arguments-file <文件>`，同时填写当前会话参数、调用编号和草稿修订号。

## 2. 读取完整目录

调用 `catalog.get`，读取结果中的原生报告。生成输入使用返回的完整 `catalogDigest`，不能对查询子集重新计算摘要。

规则查询的原生报告包含 `taskRules`；目录报告包含 `catalog`。报告格式和成功字段应按实际操作读取，不能假定所有操作都有同一个状态字段。

从目录确定以下内容：

- 材质节点类、允许属性、输入名称与输出索引。
- 蓝图节点类、函数的 `owner/member`、默认值类型与实际引脚。
- 资产设置、资源读取条件和写入位置限制。

## 3. 形成新输入

使用当前规则编写完整 JSON。涉及材质函数、纹理或父材质时，先读取真实资源信息。

新资产放在 `/Game/NodeForgeTests/` 下。使用副本准备工具或明确分配尚未使用的资产名称与请求编号，避免与已有结果冲突。

[Graph v2 示例](../GraphV2Samples/Readme.md) 可用于理解结构；部分文件的摘要来自开发输入，需要按当前规则重新核对，不能原样当成当前版本用例。

## 4. 检查和生成

顺序为 `draft.loadFile → graphs.preflight → graphs.generate`。载入后使用新的草稿修订号；每一步先读取实际结果，再继续。

生成后保留原输入、调用编号和报告。`waitingPersistence` 表示保存完成，独立重开检查按 [结果说明](../../Docs/Versions/v0.1.0/Ai/ResultsAndRecovery.md) 执行。

查询没有匹配项时，先调整关键词或说明缺少的能力。遇到未知生成结果，先查询原调用，不清空结果目录或更换编号反复生成。
