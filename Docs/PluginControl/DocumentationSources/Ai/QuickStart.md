# 从发现到结果

1. docs_index{} → docs_read{pageId:"Readme"}、AiPreparation、FeatureMap、GraphAuthoring。资料不需UE。准确版次、DLL、目录摘要、开放工具必须一致。
2. 操作者完成注册与身份分配后，connect{}取handle。operations_describe、editor_getState、draft_get均传handle和arguments:{}，从真实返回读取draftRevision，不猜旧PID/handle。
3. catalog_get和rules_query走GT，均需新callId和当前字符串expectedDraftRevision。从reports登记报告ID，report_read按/catalog/material/nodes/0或/catalog/blueprint/nodes/0读精确节点；目录摘要是catalogDigest，不是文件SHA。图类/绑定/引脚精确，查询词大小写无关OR子串。
4. 按AiPreparation取得本次新目标/request（维护验收才分配内部批次），保存完整原输入。draft_replace传arguments.text=完整JSON，或draft_loadFile传准入文件绝对路径。只改草稿，不生成，随后读取新修订。
5. 对同一修订graphs_preflight，读原报告。通过后明确graphs_generate，另用新callId。busy/staleRevision时先停、读原状态；不自动改修订重试。
6. 从原返回读nativeSummary/CLI/call、reports、checks和readback。finished不是资产最终通过；waitingPersistence说明编译保存成功但独立Verify未做。calls_get查原生成ID，report_read取得对应原子树。
7. 需要资产持久化结论时，操作者正常结束自有UE，确认独占，新PID同stage原CLI Verify，原输入/生成报告不重新Import或保存。运行/画面/打包另列。

已有资源读取、函数签名、两族原图导出及带来源的新副本，直接读[显式读取配方]({{page:AiAssetRead}})。resources.inspect/graphs.export也需GT的callId和准确修订，查询本身不载入草稿。完整原导出用report_read pointer=/exportedRequest一次取得text/value/sourceRef，再显式draft.replace或loadFile传成对来源字段。详细完整参数和其它最小序列见AiTaskRecipes/AiApiReference；任何unknown/pending/部分写入先查原call/CLI/报告，不换ID/目标重发。


当前两族完整配方及原来源载入步骤见[AiAssetReadExamples]({{page:AiAssetReadExamples}})。这些是按当前schema和目录核对的新任务例子，原有限B实证身份和证明留内部历史。要复制现有资产，先指定本次新身份再导出，使用本次返回的原text/sourceRef；文档新配方不能冒充原导出。


需要修改已有受控材质/Actor蓝图的新副本时，按[AiCopyPreview]({{page:AiCopyPreview}})：严格载入原导出后用retainExportOrigin:true编辑，draft.prepareCopy提交真实新身份，draft.get取完整稿，graphs.preview读取原graphDifference，再明确预检/生成及新PID Verify。省略retain不继承旧来源，不手动修改历史payload来绕过保护。


本版新输入推荐v3七字段；从GraphAuthoring的native字段事实及本版完整例子编写。graphs.export显式formatVersion=3，遗漏默认2；prepare/preview/relayout保持草稿版本。Build132及旧目录不接受v3，不通过替换摘要绕能力准入。


本版三族完整v3例子见[AiJsonArchitectureExamples]({{page:AiJsonArchitectureExamples}})，原v2兼容细则见GraphAuthoring；示例身份不预留，提交前核本次占用；实例全零父SHA是未取得证明的占位值，必须用本次实际父检查结果。


BT02普通Actor事件与持续移动已完成有限产品验收；完整JSON、独立时间配方、正式MCP/Verify/来源链及兼容/回退见[BlueprintTick]({{page:AiBlueprintTick}})。
