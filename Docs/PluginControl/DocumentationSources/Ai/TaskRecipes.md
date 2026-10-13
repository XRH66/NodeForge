# 任务选择和准确调用

先问目标对象：颜色/贴图/UV等用普通Material；事件/数值/字符串/Actor自身变换用受限Blueprint；只调整已保存父材质的参数用MaterialInstance。材质函数先用resources.inspect(kind=MaterialFunction)取得真实签名/SHA和单节点模板，完整步骤见[AiAssetRead]({{page:AiAssetRead}})；组件只允许本Actor RootComponent直接上下文；无所需资源/语义就停。数组、循环、多图、新父类、额外组件不支持，见族页。目录改变时旧缓存作废，按精确身份读新签名，不能只替换digest让旧图自动变合法。

两份完整合法结构在Examples。它们沿用原StaticColor/BlueprintConcat；占位身份不得提交，先核本版正式目录的格式能力及例子所需节点/字段，再按[AiPreparation]({{page:AiPreparation}})取得本次新requestId和target.assetPath（v2仍assetPath）；维护验收才使用内部台账，使用本版准确catalogDigest。历史样例不通过只换摘要升级能力。这是可用基础例子，不要求AI重新设计图。

下面是MCP参数JSON；全零handle/reportId和修订0只展示合法字段格式，不是活动会话；实际调用用connect/draft_get/原报告的真实值，callId在首次提交前保存：

```json
{"handle":"00000000000000000000000000000000","arguments":{}}
```

用于operations_describe/editor_getState/draft_get。规则查询：

```json
{"handle":"00000000000000000000000000000000","callId":"AllocatedRulesCall","expectedDraftRevision":"0","arguments":{"assetKind":"Material","query":"ScalarParameter VectorParameter"}}
```

catalog_get同样外层字段，arguments:{}。Blueprint可查Concat PrintString；query≤256字符、最多8个空白词，OR子串；不是图配方。

```json
{"handle":"00000000000000000000000000000000","callId":"AllocatedLoadCall","expectedDraftRevision":"0","arguments":{"path":"{{workspaceRoot}}/{{sessionDirectory}}/MaterialInput.json"}}
```

这是操作者准备的新文件绝对路径，不代表本包已创建该文件。加载/替换后重新draft_get取得修订。graphs_preflight/generate各arguments:{}、各自新callId/同真实修订。直接replace的text是完整JSON序列化字符串，不是文件名；不能把对象直接当text。

```json
{"handle":"00000000000000000000000000000000","arguments":{"callId":"OriginalGenerateCall"}}
```

上式用于calls_get，不能换新生成ID。report_read示例：

```json
{"handle":"00000000000000000000000000000000","reportId":"0000000000000000000000000000000000000000000000000000000000000000","pointer":"/checks"}
```

再读/initialReadback、/finalReadback、/publishedReadback，与原输入和独立预期逐项比较。示例字段格式合法但身份不代表当前现场，不能照抄提交；工具schema中的实际正则和完整参数见AiApiReference。若unknown或报告损坏，按AiResultsAndRecovery保留原call/资产/收据，停后续GT。


## 普通批量节点

本版数量是 {{materialCount}} 材质类 / {{blueprintCount}} 蓝图精确签名，数量与有效配置分别记录。先读 [批量普通节点配方]({{page:AiNodeBulk}})；通过已有 rules_query 查询任务所需类型，catalog_get 的完整报告保留全量摘要。资料节点页中原生输入/输出、float/double、允许属性和默认可直接用于 JSON；不能从显示名称猜 pin。旧摘要不能通过仅换 digest 自动迁移新节点。


新配方显式选择graphs.export arguments.formatVersion=3；兼容调用遗漏该参数仍返回v2。两布局进入同一个原生生产核心，完整text和sourceRef来自原导出，不由客户端另建执行器。


本版三族完整v3例子见[AiJsonArchitectureExamples]({{page:AiJsonArchitectureExamples}})，原v2兼容细则见GraphAuthoring；历史实证身份不得重复生成；当前配方使用示例新身份，提交前仍须核本次占用与依赖，实例父SHA必须来自本次实际检查。


BT02普通Actor事件与持续移动已完成有限产品验收；完整JSON、独立时间配方、正式MCP/Verify/来源链及兼容/回退见[BlueprintTick]({{page:AiBlueprintTick}})。
