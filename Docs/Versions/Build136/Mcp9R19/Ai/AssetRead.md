# 显式读取、原图导出与新副本

公开副本中的请求、目标、进程、会话及来源报告身份为虚构示例；历史产品结论保持，原始证明由维护者内部保留。实际调用须使用本次分配和返回的完整原 text/sourceRef，示例身份与零摘要不能作为来源证明。

本页对应 Build136 Repair6、桥 0.3.6、NodeForge-Build136-Mcp9R19。这是有限受控读取，不搜索或枚举资源，不支持任意UE图。发布状态和实际业务证据以本版Manifest及版本说明为准；工具存在、编译通过、保存通过和新PID Verify不是同一结论。

## 先选择任务

| 任务 | 正式工具与参数 | 取得什么 |
| --- | --- | --- |
| 普通材质全局参数 | nodeforge_resources_inspect，kind=MaterialParameters | /parameters：真实标量／向量／纹理默认值、纹理采样签名、parent与parentAssetSha256 |
| MaterialFunction签名 | nodeforge_resources_inspect，kind=MaterialFunction | /functionSignature及/nodeTemplate：输入名、GUID、原生类型、required、预览默认策略、输出索引；模板是单节点片段 |
| 普通材质完整图 | nodeforge_graphs_export，assetKind=Material | /exportedRequest专用读取：原完整Graph v2/v3与来源引用；新配方formatVersion=3 |
| 普通Actor蓝图完整图 | nodeforge_graphs_export，assetKind=Blueprint | 同上，单EventGraph／允许变量及默认骨架，必须能完整表达 |

source是精确Package.Asset对象路径。Material／MaterialFunction允许 /Engine/ 或 /Game/NodeForgeTests/；Blueprint只允许测试根。材质实例、函数实例、材质层、函数整图导出、搜索、多图、新父类／组件、循环／数组不在这些工具范围。支持节点数量不能当作所有源图可导出的承诺；未知节点或无法表达的有效属性会明确拒绝。

## 准备现场与身份

先读 [完整准备、注册和Verify](Preparation.md)，确认本版stage/DLL、复用工程及注册目录；当前工程 SelectedProject，路径 ReplaceWithInstallationRoot/ReplaceWithSelectedProject.uproject。MCP不启动／关闭UE、不分配批次、不抢锁。操作者按已有台账分配新requestId和完整assetPath，保留批次所有权；普通重试或换PID不新建工程。测试目标允许根下0／1／2级子目录，每段1–16 ASCII字母数字，叶名与request公开1–64，建议≤32。两个子目录可以有相同叶名，完整目标和项目request必须唯一。正式使用目录名NodeFroge保留，本版不猜生产mount。

源必须已保存、clean，Blueprint已有可用GeneratedClass及编译状态。当前进程只接受由本共享链在文件只读保护下完成的加载；已加载对象若没有这份来源证明，拒绝sourceMemoryUnproven。不会自动reload、保存或编译源。应在新专用编辑器中先执行正式读取，再手动打开源资产；遇到该拒绝保留原结果，正常结束自有会话后明确准备新会话，不换业务身份自动重发。sourceNotReady说明注册表尚不能确认，不能当sourceNotFound。路径／类型／redirect／未保存／dirty均有各自拒绝。

调用connect取得本会话handle；draft_get取得当前draftRevision字符串。所有四模式经同GT gate，必须新callId及准确expectedDraftRevision，即使纯查询也不能绕过busy／修订保护。查询不改变草稿／origin／revision／LastSuccess／生成快照／收据／导出基线；只是本次读取结果与报告更新。

下面全零handle和修订0只展示字段，不能照抄为活动身份；callId首次提交前保存，批次身份来自当前台账。测试材料路径是本机已有明确示例，不表示每个用户工程都有这些对象。

## 普通材质与函数读取

nodeforge_resources_inspect参数：

```json
{
  "handle": "00000000000000000000000000000000",
  "callId": "ExampleCalld7ab60de",
  "expectedDraftRevision": "0",
  "arguments": {
    "kind": "MaterialParameters",
    "source": "/Game/NodeForgeTests/Material/ExampleRun/Sample01.Sample01"
  }
}
```

函数读取只替换kind和source，外层身份仍须真实新调用：

```json
{
  "handle": "00000000000000000000000000000000",
  "callId": "ExampleCall19459e52",
  "expectedDraftRevision": "0",
  "arguments": {
    "kind": "MaterialFunction",
    "source": "/Engine/Functions/Engine_MaterialFunctions02/SmoothStep.SmoothStep"
  }
}
```

检查原status／cliExitCode／nativeSummary.response.result状态及reports。成功须原报告operationStatus=passed、overallStatus=succeeded，不能只看进程退出0。用原reports[0].reportId调用nodeforge_report_read，材质pointer=/parameters；函数先/functionSignature，再/nodeTemplate。源身份在/resourceRead，函数/templateScope明确单节点限制。空参数集合可能是合法材质；原类型／读取保护仍须成功。

函数properties.MaterialFunction和FunctionAssetSha256绑定直接保存文件；inputs的name是连接用原生显示名，guid标真实引脚身份，nativeType是原生类型值。required=true必须连接；required=false时未连接沿usesNativePreviewDefault/nativePreviewValue的原策略，不存在逐调用默认适配器。outputs.index是字符串形式的真实输出索引，name／guid来自定义。nodeTemplate是原v2兼容片段，不能直接draft.replace。新v3节点将其中MaterialFunction/FunctionAssetSha256转为binding:{kind:"MaterialFunction",path,assetSha256}，properties:{}；其它id/class/连接保持。本版GraphAuthoring及节点页给v3片段，原签名／源SHA不变。SHA只锁直接资源文件，不承诺递归锁定其所有函数／纹理依赖。

## 两族导出：先确定新身份

nodeforge_graphs_export参数；示例身份必须替换为当前分配结果，不能占用历史目标：

```json
{
  "handle": "00000000000000000000000000000000",
  "callId": "ExampleCall0d95f29f",
  "expectedDraftRevision": "0",
  "arguments": {
    "assetKind": "Material",
    "formatVersion": 3,
    "source": "/Game/NodeForgeTests/Material/ExampleRun/Sample01.Sample01",
    "requestId": "AllocatedMaterialRequest",
    "assetPath": "/Game/NodeForgeTests/Material/AllocatedBatch/Sample01"
  }
}
```

Blueprint只改assetKind=Blueprint、明确Blueprint源、该族分配的新requestId／assetPath，以及本次callId。两族复用原Exporter／重构回读准入；源与目标不同，文件／内存／注册表目标以及项目Requests占用均拒绝。导出不预留目标，不写资产／收据。新身份在首次导出前确定，本有限原图副本配方不在导出后改身份或设计另一张图。

成功后一次调用nodeforge_report_read：

```json
{
  "handle": "00000000000000000000000000000000",
  "reportId": "0000000000000000000000000000000000000000000000000000000000000000",
  "pointer": "/exportedRequest"
}
```

必须用原业务返回的实际reportId。此专用分支返回value（完整Graph对象）、text（原生原完整序列化文本）、payloadSha256和sourceRef；text最多65536 UTF-8字节，原报告最多1MiB。普通pointer仍最多32768字节，超限返回导航而非截断。这里不需要拼sidecar、从多个节点重组输入，也不能用摘要替代text。

sourceRef.exportReportPath／exportReportSha256指向原报告字节；source／sourceAssetSha256标源文件；assetKind与payloadSha256标本次导出。原报告sha256、源uasset SHA、payload文本SHA、Graph语义摘要和草稿输入SHA是不同身份，不互换。保留原text、报告引用及完整原调用结果，原报告不编辑。

## 显式载入并生成

将专用结果P的text原值送入nodeforge_draft_replace。参数构造配方（是赋值示例，实际由当前客户端调用工具）：

```python
arguments = {
    'text': P['text'],
    'exportReportPath': P['sourceRef']['exportReportPath'],
    'exportReportSha256': P['sourceRef']['exportReportSha256'],
}
params = {'handle': current_handle, 'arguments': arguments,
          'callId': allocated_load_call, 'expectedDraftRevision': current_revision}
```

只把这两个报告引用字段放进arguments，不能把整个sourceRef并入Graph或arguments。也可先将P.text原字节保存到准入JSON文件，再用draft.loadFile的path及同一对exportReportPath／exportReportSha256载入；二者必须成对，SHA为64位小写hex。来源载入验证原报告／DLL／工程／源／payload，首次文本允许同目录支持的v2/v3结构表示差异，但共同描述的值、presence、原数组顺序、request和target必须等于原导出；报告/text/payload原SHA及当前DLL先核，不升级旧来源证明。

载入后draft_get核text／digest／exportOrigin和新revision。然后graphs_preflight arguments={}，准确当前revision／新callId；通过才对同一稿graphs_generate arguments={}，另一个新callId。正式核心在预检／生成重新消费原来源报告，确认源对象／原文件SHA／clean；只读文件保护覆盖执行，已有直接依赖校验保持。普通JSON仍可不附这对引用，按原Graph规则工作；同来源编辑／准备副本保留derived关系，无关替换清掉旧基线，恢复只恢复匹配稿。MCP不自动载入查询结果。

检查生成原报告的checks、initialReadback／finalReadback／publishedReadback、inputDigest、requestId／assetPath、fileSha256与项目收据。默认生成保存成功仍是waitingPersistence。源或原报告在写入后发生冲突，结果unknown且保留实际资产／收据／失败证据，不能重发或删掉部分写入。

## 同一正式CLI的调用方式

参数JSON文件内容就是对应工具的arguments对象，不是整个MCP外层。SessionPrivatePath来自操作者本次注册（私有，不复制到公开文档）；RunDirectory须首次准备的新目录。下列占位必须替换本次真实值，CLI并不提供editor-register或verify子命令：

```powershell
& 'ReplaceWithInstallationRoot\Tools\NodeForgeMcp\Environment\Scripts\python.exe' 'ReplaceWithInstallationRoot\Scripts\NodeForgeControl.py' editor-call --session-file 'ReplaceWithCurrentPrivateSessionPath' --project 'ReplaceWithInstallationRoot/ReplaceWithSelectedProject.uproject' --plugin-directory 'ReplaceWithInstallationRoot/PluginStage/NodeForge' --run-directory 'ReplaceWithNewCliRunDirectory' --call-id 'AllocatedInspectCall' --operation resources.inspect --expected-revision 'ReplaceWithCurrentRevision' --arguments-file 'ReplaceWithArgumentsJsonPath' --timeout-seconds 180
```

材质参数／函数签名只换该arguments文件里的kind/source；graphs.export同editor-call，operation与四项必需arguments及可选formatVersion对应上表。注册按AiPreparation的真实Registration.json及启动参数，不从本页另建一套入口。

## 独立Verify与停止

生成窗结束后正常关闭本次自有UE、确认工程独占；同stage用原正式CLI execute --operation graphs.verifySaved，--input指生成实际载入的原输入，--generation-report指原成功保存报告，--run-directory为新Verify目录。新PID必须与生成PID不同，不再Import／resave。准确完整命令和台账登记见AiPreparation；两族各自引用自己的输入／报告，不拿另一族或历史Build122描述替代当前DLL。运行、材质画面、打包本有限阶段不默认验证。

任何pending／unknown／busy／staleRevision／源、报告或目标冲突，先停止依赖写入，查原nodeforge_calls_get、原CLI及报告，保留当前输入和身份。只有证据证明未发送／未dispatch并解决原因，才按既有契约作一次有限恢复；仅查不到call不够。不能换request／目标／call来掩盖重复提交，不自动清锁、kill、保存dirty源或扩工程。UI只读导出成功会在有归属／修订保护下将原payload与基线载入；API查询没有这个后置副作用。UI按钮完整操作见 [手动任务](../User/ManualWorkflows.md)。


## 本版实际闭环和完整输入

[完整两族样例](AssetReadExamples.md)保存本次合法原Graph、来源引用、输入摘要、生成与新PIDVerify。Material参数和函数签名正式CLI正例属于Build128，未变实现经准确连续性复用；两族导出、来源不符的明确拒绝、显式载入和两副本保存/Verify属于Build130。生成报告本身仍保留waitingPersistence，独立Verify有各自原报告，不改写旧报告为成功。未知、busy、未提交和明确业务拒绝按上述真实状态处理。
