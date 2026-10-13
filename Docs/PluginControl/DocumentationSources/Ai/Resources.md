# 发现资源、精确检查与引用生成

适用 {{bundleId}} / {{buildId}} / 桥 {{bridgeVersion}}。本期只增加一个 resources.query 业务，列表种类为 Texture2D、MaterialFunction、Material。JSON2/3和原生成、来源、准备/预览流程保持；没有逐资源/逐节点 MCP 工具。目录与本期新行为是否已取得真实证据，以本包 publicationStatus 和发行说明为准。

## 1. 前置与身份

先 docs_index，再读 Readme、AiPreparation、FeatureMap、GraphAuthoring。确认本包 DLL/目录/工具/版本一致；操作者已有明确绑定及当前专用编辑器注册，再 connect 取 handle，并 draft_get 取真实字符串修订。每个 GT 调用首次发送前保存自己的 callId 与参数；下方全零 handle、修订0和 Allocated 前缀是虚构格式，不能直接提交。

## 2. 有界列表

调用 nodeforge_resources_query：

```json
{"handle":"00000000000000000000000000000000","callId":"AllocatedTextureQuery","expectedDraftRevision":"0","arguments":{"kind":"Texture2D","directory":"/Engine/EngineResources","nameContains":"Gray","recursive":false,"limit":50,"after":""}}
```

kind精确为上面三个值；directory是一个明确允许子目录，不能为空或根级扫描；nameContains最多128字符，省略为空；recursive省略false；limit整数1–200，省略50；after省略空字符串，最多512字符。遍历受目录深度16、目录数128和访问条目5000限制，不能利用大 limit 扫全库。MaterialFunction可使用已允许的 /Engine/Functions/Engine_MaterialFunctions02 子目录，Material选择绑定工程允许的内容子目录。

从原返回资源查询报告的 /resourceQuery 读取完整过滤、路径/类/名称、政策版本、registry扫描状态、complete/truncated、next和条目。列表只使用磁盘登记元数据，不加载对象、不创建 Origins、草稿或资产。discovered/policyAllowed不等于 inspected；requireInspect=true，graphExport保持未知。complete=false、扫描中、访问上限或缺元数据时不把空列表当完整结果，不继续分页；先缩小目录或查登记状态。只有真正完整且有 next 时，把它原样放入 after，保留其它过滤。没有跨页稳定快照承诺，目录变化时按精确 objectPath 去重。

返回中的 reports 提供真实 reportId；按该 reportId 调用 nodeforge_report_read。子树超32768字节时，读取返回 navigation，再选 /resourceQuery/rows/0 等更小 pointer；禁止拿摘要拼完整来源证明。

## 3. 精确检查与字段映射

列表行给出准确 objectPath 后再调用 nodeforge_resources_inspect：

```json
{"handle":"00000000000000000000000000000000","callId":"AllocatedTextureInspect","expectedDraftRevision":"0","arguments":{"kind":"Texture2D","source":"/Engine/EngineResources/Gray.Gray"}}
```

检查原报告 /resourceRead 和 /texture：已保存、干净、准确 Texture2D、nonvirtual、源文件 SHA、尺寸、sRGB、压缩和原生采样器。/texture/properties 的 Texture、SamplerType 可直接用于原纹理节点 properties；没有新 TextureSha 字段。实际资格在正式预检/构造时重核，不缓存一次通过的对象来放宽来源。无保存加载证明的预加载源仍 sourceMemoryUnproven，不自动 reload/resave。UE 5.3.2 启动配置预加载 WhiteSquareTexture，该资源可能因此被正确拒绝；此配方使用实际受控读取通过的 Gray，仍须以当前检查返回的原生 SamplerType 为准。

函数使用同一工具，kind=MaterialFunction：

```json
{"handle":"00000000000000000000000000000000","callId":"AllocatedFunctionInspect","expectedDraftRevision":"0","arguments":{"kind":"MaterialFunction","source":"/Engine/Functions/Engine_MaterialFunctions02/SmoothStep.SmoothStep"}}
```

原报告 /functionSignature 和 /nodeTemplate 给准确输入、输出、optional/defaultPolicy、函数直接文件 SHA 与声明片段。v2使用原 properties.MaterialFunction / FunctionAssetSha256；v3将它们分别映射到 binding.kind=MaterialFunction、path、assetSha256，properties为空。不能猜 Alpha/Min/Max 引脚、用旧 SHA 或递归依赖安全结论代替当前检查。函数本体创建、函数图导出和逐调用默认适配未开放。

Material列表不提供整图可导出资格。普通父材质参数使用 inspect(kind=MaterialParameters)并检查父 SHA；两族原图完整导出使用原 graphs.export，详[AiAssetRead]({{page:AiAssetRead}})。读取和列表均不自动载入草稿。

## 4. 两种完整表示

下列同一材质配方把世界 X 的 SmoothStep 结果接 OpacityMask，Gray 纹理乘颜色接 EmissiveColor，Surface/Masked/Unlit。它们是完整结构模板，不是已经分配的提交：request/目标替换为本次真实新身份，摘要取真实候选目录；函数 SHA/引脚及纹理资格须当前正式 inspect 再核。不能只换目录摘要绕过能力门槛。

v3：

```json
{{resourceMaterialV3}}
```

v2保留同一语义：

```json
{{resourceMaterialV2}}
```

## 5. 原正式生成、结果与恢复

保存本次完整 JSON UTF-8 原字节；通过 draft_loadFile(arguments.path)或 draft_replace(arguments.text)显式载入，随后 draft_get 取得新修订。对这份准确修订分别以新 callId 调用 graphs_preflight(arguments={})、读原报告，再 graphs_generate(arguments={})。每一步成功才推进，不将查询或检查报告当生成输入。

生成后 calls_get 查询原 callId，report_read读取同一原生成报告。核 inputDigest、目标、来源文件与原生检查、编译/保存/readback；waitingPersistence仍待原 CLI 新 PID Verify，操作者配方见[AiPreparation]({{page:AiPreparation}})。没有新资产的只读查询/检查不需要新 PID Verify。

pending/unknown、源或报告冲突、目标占用、部分写入时停依赖写入，查原 call/原报告/准确进程；不换 ID/目标重发、不清锁、不强杀。sourceNotReady不等于不存在，sourceMemoryUnproven是已知拒绝；查源状态而不擅自修改原资源。完整返回和边界见[AiApiReference]({{page:AiApiReference}})、[结果恢复]({{page:AiResultsAndRecovery}})。
