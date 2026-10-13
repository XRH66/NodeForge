# 普通节点发现、构图与结果核对

本版 {{buildId}} / 桥 {{bridgeVersion}}，{{materialCount}} 材质类 / {{blueprintCount}} 蓝图签名。先 nodeforge_docs_index、Readme/AiQuickStart/FeatureMap，再读本页与具体节点页；操作者显式准备/注册专用编辑器的完整步骤在 [AiPreparation]({{page:AiPreparation}})。文档不授予新账号权限。

## 从任务找到准确节点

1. connect 后核真实 bundle/DLL/catalog 身份，重取 handle 和 draft revision。
2. 用原 nodeforge_rules_query 的 assetKind 与短 query 查询节点，例如 Math、Vector、String；最多8词、query≤256字符，按原 OR 子串语义，不枚举整个工程。
3. nodeforge_catalog_get 保留完整目录摘要；从原报告 pointer 取得需要的节点、properties/pins 或 inputs/outputs。大数组逐行读取原 pointer，子集没有自己的 import digest。
4. 同版族索引导航精确 identity 页；function 使用确切 owner/member，成员类型和动态配置单独核。节点页的例子是声明片段，完整两族配方见 [Examples]({{page:Examples}})，本次代表图的实际输入与结果从版本证据定位。

## 组合规则

Blueprint 仍为 Actor 单 EventGraph、节点≤128/边≤256。纯函数输入/输出角色与真实 float/double 必须按 pins；只能直接 MAKE 接线，不会插 cast。可写且未连接的可见输入用准确 bool/int32/name/string/number/Vector/Rotator 默认结构；不把名字变成对象，不开放数组/FText/int64/mutable ref/随机状态或隐藏上下文。纯函数自己不推动执行流，必须接入被消费的数据链。

Material 仍为普通 Surface，节点≤256/边≤1024，所有表达式到达有效输出；必需端口接线，输出用原数字字符串索引。新普通 Math 仅本页公开 scalar/bool 属性，原生默认可以省略，不认识的字段不能悄悄丢掉。纹理/材质函数仍使用原资源 SHA、签名和适配，不等同普通数学节点。两族输入均≤64KiB、深度≤32。

## 正式调用与完整结果

分配新批次完整身份后，用原 draft_loadFile 或 draft_replace 显式载入完整 JSON，重取修订，再用 graphs_preflight 和 graphs_generate，各自保存首次 callId，不换ID重发。waitingPersistence 只是生成/编译/保存的原状态，必须依 [Preparation]({{page:AiPreparation}}) 在新 PID 用原 CLI、原输入和生成收据 Verify；不重新 Import/resave。

正式 graphs_export 返回完整 exportedRequest 与来源证明，按 [AssetRead]({{page:AiAssetRead}}) 的 report_read /exportedRequest 取得全文并显式载入；来源报告/SHA成对传入。原资产不覆盖。本版新增节点也必须被同一导出/预检识别，未知配置整体拒绝。源、草稿和原报告冲突或unknown/pending，查询原 calls_get/report 后停依赖写入，不能换目标碰运气。运行/材质画面/打包以各自证据为准。

## 更新后如何继续

普通节点扩展不增加每节点 MCP handler。读取新准确索引/目录和节点页，现有工具参数不变；旧摘要保留原能力，不把新图伪装旧输入。SDK/ABI兼容时沿本版桥0.3.4同连接接续批准配置，handle/revision重取，原失败/unknown/lease历史保持。版本迁移和成套回退见 [Compatibility]({{page:VersionCompatibilityAndMigration}})。


本批两族完整真实输入、用途和实际分层结果见[批量节点样例]({{page:AiNodeBulkExamples}})。样例中的request/目标已经占用，不能直接重放；下一任务按准备配方分配新身份。
