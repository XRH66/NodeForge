# 按任务调用工具

先确认要创建的资产类型：颜色、纹理和 UV 使用 Material；事件、数值、字符串或 Actor 变换使用 Blueprint；调整已保存父材质的参数使用 MaterialInstance。材质实例的 MCP 生成和保存尚未验证。

数组、循环、多图、新父类和额外组件不在当前范围内。需要材质函数时，先按 [资源读取](AssetRead.md)取得实际签名和 SHA。

## 1. 读取当前状态

以下 JSON 展示参数结构。全零 handle/reportId 和修订 `0` 是占位值，必须替换为 connect、draft_get 和对应报告返回的值。callId 在首次提交前生成并保存，不能重复用于不同操作。

`operations_describe`、`editor_getState` 和 `draft_get` 使用：

```json
{
  "handle": "00000000000000000000000000000000",
  "arguments": {}
}
```

## 2. 查规则和目录

`nodeforge_rules_query` 示例：

```json
{
  "handle": "00000000000000000000000000000000",
  "callId": "AllocatedRulesCall",
  "expectedDraftRevision": "0",
  "arguments": {
    "assetKind": "Material",
    "query": "ScalarParameter VectorParameter"
  }
}
```

Blueprint 可以查询 `Concat PrintString`。query 最多 256 字符、8 个词，按大小写无关的 OR 子串筛选。`nodeforge_catalog_get` 使用相同的外层字段和 `arguments:{}`。

从结果中取得 `catalogDigest`，查阅 [节点说明](NodeBulk.md)。完整图结构见 [基础示例](../Reference/Examples.md)与 [JSON v3 示例](JsonArchitectureExamples.md)。使用未占用的 requestId 和目标路径，依赖资源的 SHA 必须来自当前检查。

## 3. 载入完整 JSON

将输入保存为插件允许目录内的 UTF-8 JSON 文件，再调用 `nodeforge_draft_loadFile`：

```json
{
  "handle": "00000000000000000000000000000000",
  "callId": "AllocatedLoadCall",
  "expectedDraftRevision": "0",
  "arguments": {
    "path": "填写已安装插件目录内的输入 JSON 绝对路径"
  }
}
```

也可以用 `nodeforge_draft_replace` 的 `arguments.text` 直接传入完整 JSON 字符串。text 不是文件名，也不能传对象代替字符串。

载入后重新 `draft_get` 取得修订。然后分别调用 `nodeforge_graphs_preflight` 和 `nodeforge_graphs_generate`：各用新 callId、当前修订和 `arguments:{}`，预检通过后才生成。

## 4. 查询结果

`nodeforge_calls_get` 使用生成时的原 callId：

```json
{
  "handle": "00000000000000000000000000000000",
  "arguments": {
    "callId": "ExampleCall9e184122"
  }
}
```

从返回 reports 取得真实 reportId，再调用 `nodeforge_report_read`：

```json
{
  "handle": "00000000000000000000000000000000",
  "reportId": "0000000000000000000000000000000000000000000000000000000000000000",
  "pointer": "/checks"
}
```

继续读取实际存在的 `/initialReadback`、`/finalReadback`、`/publishedReadback`，与输入比较。`waitingPersistence` 按 [保存核验](Preparation.md)处理；`pending` 或 `unknown` 按 [失败处理](ResultsAndRecovery.md)查询原调用，不重复生成。

已有资产可按 [导出与载入](AssetRead.md)复制，再按 [副本编辑](CopyPreview.md)修改。导出显式 `formatVersion:3` 返回 v3，省略时返回 v2。持续移动的蓝图示例见 [Actor Tick](BlueprintTick.md)。
