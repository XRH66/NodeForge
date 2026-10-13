# 结果状态、报告与有界恢复

三个不同身份：MCP协议请求、Editor controlSession/callId、Graph requestId/assetPath。不能互换。桥返回结构化结果并保留原CLI/response、workflow/native状态、原报告 SHA/bytes/reportId/证据目录，不合成业务succeeded。

生成/Import资产：validation/build/initialReadback/compile/finalReadback/publish/save均真实passed、独立persistence未执行时 waitingPersistence。新PID Verify核原保存后才可满足八门槛；runtime/visual/package另列。辅助 GraphPreflight overallStatus=succeeded 只代表预检，GraphRules看operationStatus、Catalog看本操作完成，不套资产八门槛。准备副本 requestOutcome=notBuilt，资产/名称未预留。

GT串行、busy拒绝；expectedDraftRevision准确uint64字符串。staleRevision时先读原草稿/状态，不能自动改修订重发。callId持久记录先于提交：同ID同参数只查calls.get；同ID不同操作/参数/修订拒绝。新报告/输入/原资产不覆盖。控制保留128调用、桥256报告，容量不足停止。

pending/unknown、截止/取消、残留或报表指纹冲突立即停止后续GT。保存原callId、原CLI run、PID/创建身份、input snapshot、原收据/资产与部分写入；有限调calls.get查询原调用。CLI仍运行时不强杀，保留LiveCli。晚到收据只收集同进程原结果，不重发。明确失败仍需residue复核；桥 requiresReview不能通过重新连会话绕过。

只读恢复不能证明没有部分写入；不同会话旧handle/reportId失效，不猜PID/自动重连重放。报告篡改/越界/reparse/凭据文件/版本不符拒绝；原CLI可用于操作者精确状态查询。未知不能换request/asset重新生成，停止说明证据缺口。

报告读需本会话已登记reportId和RFC6901 pointer。原报告≤1MiB，所选≤32768字节；超限resultTooLarge返回navigation.keys/navigation.length等导航且selectionReturned=false，不截断。按`/catalog/material/nodes/0`、`/checks`及普通Material/Blueprint Import实际的`/initialReadback`、`/finalReadback`、`/publishedReadback`逐次读。以原报告键名为准，不猜不存在的`/readback`；文档pageId≤128、每页≤65536字节，只manifest原SHA/bytes，不任意路径。

原 [EditorControlProtocol](../../../../../Contracts/EditorControlProtocol.md)、[ControlProtocol](../../../../../Contracts/ControlProtocol.md)、[ResultContractDraft](../../../../../Contracts/ResultContractDraft.md)是不同层契约。ResultContractDraft早期完整phase/取消/幂等原子锁等候选未全实现，不因这份手册宣称已实现；使用实际原生字段和本轮有界策略。

同版资料更新仅由固定本地配置的docsManifestSha256批准，新Manifest业务/运行绑定不变时自动刷新；返回docsManifestSha256可核当前版本。未批准的文件变化仍NFMCP_FILE_SHA；工程/会话/执行或业务定义变化必须成套批准；兼容更新在原SDK内自动接续，正常新会话重新connect取得handle，不重放旧call。刷新不清原unknown/pending、call、报告、handle或lease。原P4服务级载入是历史边界；本版兼容应用代码按批准配置可在同一SDK进程实际载入，新代次以serviceRuntime和当前接续回执为准，不把无UE边界检查写成已连通。
