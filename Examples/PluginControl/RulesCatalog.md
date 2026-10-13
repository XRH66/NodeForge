# 规则与目录查询到新资产：方案A

本页适用A的Build122及可信本地调用者。正式查询、两处UI和两个子目录资产/同工程新PIDVerify已经由[Run2集中验收](../../Reports/PluginControlStageTwo/RulesCatalog1/Run2/P2Review.json)实测；本页手工配方仅静态对照CLI参数，未另起批次。实际版本/交付见[实施记录](../../Reports/PluginControlStageTwo/RulesCatalog1/Readme.md)，计划见[ThirdStagePlan](../../Docs/PluginControl/ThirdStagePlan.md)。原Build118不具备两个查询，根安装/RC1未升级。

## 1. 先确认本批身份

仅连接显式登记、实际运行的同一测试工程和A候选stage。调用者从本批无秘密SessionPublic.json取得sessionFile、project、module、binarySha256及PID/创建身份；正式CLI仍核受保护描述符、进程/模块/管道，不展示凭据。不要拿历史public identity连接新进程，也不要自动接管当前窗口。

设置已有的准确路径；`$NfSessionFile`是当前本批Session.json路径，不能复制其内容。`$NfRun`必须是本次新报告根，各调用使用独占子目录；不要复用失败目录或覆盖旧结果。

```powershell
$NfCli = 'X:\NodeForge\Scripts\NodeForgeControl.py'
$NfProject = 'X:\NodeForge\ValidationProject\RegressionRuns\NFCUiApiLab\NFCUiApiLab.uproject'
$NfStage = 'X:\NodeForge\Reports\PluginControlStageTwo\RulesCatalog1\Build122\PluginStage\NodeForge'
# 按当前本批SessionPublic.json设置 $NfSessionFile 和全新 $NfRun。
python $NfCli editor-describe --session-file $NfSessionFile --project $NfProject --plugin-directory $NfStage --run-directory "$NfRun\Describe"
python $NfCli editor-call --operation draft.get --call-id ReadDraft --session-file $NfSessionFile --project $NfProject --plugin-directory $NfStage --run-directory "$NfRun\Draft"
```

检查真实describe中有rules.query/catalog.get及其schema。取当前稿draftRevision字符串作为`$NfRevision`；后续每个GT操作使用当时的准确revision，查询成功本身不增加它。

## 2. 查询并读取受控报告

在本批Reports内保存MaterialQuery.json：`{"assetKind":"Material","query":"VectorParameter ScalarParameter"}`。BlueprintQuery.json：`{"assetKind":"Blueprint","query":"ReceiveBeginPlay Concat_StrStr PrintString"}`。字段精确；空query返回一整类，OR子串匹配，最多256字符/8个词；无匹配拒绝，不猜替代节点。

```powershell
python $NfCli editor-call --operation rules.query --call-id MaterialRules --arguments-file "$NfRun\MaterialQuery.json" --expected-revision $NfRevision --session-file $NfSessionFile --project $NfProject --plugin-directory $NfStage --run-directory "$NfRun\MaterialRules"
python $NfCli editor-call --operation rules.query --call-id BlueprintRules --arguments-file "$NfRun\BlueprintQuery.json" --expected-revision $NfRevision --session-file $NfSessionFile --project $NfProject --plugin-directory $NfStage --run-directory "$NfRun\BlueprintRules"
python $NfCli editor-call --operation catalog.get --call-id Catalog --expected-revision $NfRevision --session-file $NfSessionFile --project $NfProject --plugin-directory $NfStage --run-directory "$NfRun\Catalog"
python $NfCli editor-status --call-id MaterialRules --session-file $NfSessionFile --project $NfProject --plugin-directory $NfStage --run-directory "$NfRun\MaterialRulesStatus"
```

每一步先核CLI退出值和原call终态，再依赖下一步。读取终态result.nativeReport引用，核实际字节数/SHA、mode/project/PID与sidecar中的调用、查询快照摘要、模块、session/创建身份一致。完整报告不塞入RPC。GraphRules成功有overallStatus=succeeded及taskRules；GraphCatalog原生成功只有operationStatus=passed/catalog，不能要求或伪造overallStatus。生成时使用返回的**完整catalogDigest**，不对子集重算摘要。

## 3. 形成、生成并独立验证

沿[原StaticColor](../GraphV2Samples/StaticColor.json)及[原BlueprintConcat](../GraphV2Samples/BlueprintConcat.json)的已支持节点/属性/默认值/连接/settings形成受控输入。查询用于核实际能力，原示例不改写。发现/实验/已验证层级继续保留，动态资源签名与任意组合不因查询成功而算验证通过。

首次提交前按登记工程台账分配新批次、项目级唯一requestId及未占用的完整assetPath，例如同一批次：`/Game/NodeForgeTests/Material/B000001/Sample01`与`/Game/NodeForgeTests/Blueprint/B000001/Sample01`。例名不代表可重复使用；查实际台账和磁盘后才提交，冲突不自动改名。测试叶名/requestId≤32，父目录段≤16且0/1/2级，公开旧64字符叶名仍兼容；工程名≤20，推荐≤12。正式使用目录名NodeFroge的完整路径后续落实，不新建UE挂载根。

现有流程：draft.loadFile→取新revision→graphs.preflight→graphs.generate→editor-status查原call。按返回的真实读回核节点、属性/defaults、边和settings，保留输入/原生成报告，生成0/waitingPersistence只说明构图编译保存。两例串行，共用工程/stage，不为每个调用新建工程。

将已核实、已分配身份的本次MaterialInput.json放在本批Reports内，MaterialLoadArguments.json只含其准确绝对路径，例如`{"path":"X:/NodeForge/Reports/PluginControlStageTwo/RulesCatalogNext/MaterialInput.json"}`；RulesCatalogNext仅为配方名，必须替换为本批准确目录、先保存本次输入。逐条执行，上一条的终态与原生结果通过后再运行下一条：

```powershell
python $NfCli editor-call --operation draft.loadFile --call-id MaterialLoad --arguments-file "$NfRun\MaterialLoadArguments.json" --expected-revision $NfRevision --session-file $NfSessionFile --project $NfProject --plugin-directory $NfStage --run-directory "$NfRun\MaterialLoad"
# 从load成功返回取得新的draftRevision，更新 $NfRevision。
python $NfCli editor-call --operation graphs.preflight --call-id MaterialPreflight --expected-revision $NfRevision --session-file $NfSessionFile --project $NfProject --plugin-directory $NfStage --run-directory "$NfRun\MaterialPreflight"
# 核原生preflight通过及身份仍准确，再生成。
python $NfCli editor-call --operation graphs.generate --call-id MaterialGenerate --expected-revision $NfRevision --session-file $NfSessionFile --project $NfProject --plugin-directory $NfStage --run-directory "$NfRun\MaterialGenerate"
python $NfCli editor-status --call-id MaterialGenerate --session-file $NfSessionFile --project $NfProject --plugin-directory $NfStage --run-directory "$NfRun\MaterialGenerateStatus"
```

保存返回的真实nativeReport路径为`$NfGenerationReport`，核报告SHA/输入digest与本次MaterialInput一致。Blueprint使用自己的input/arguments/callId/run路径，读取当前revision后执行同一流程，不能把Material的收据或revision直接套用。

正常停止通道并退出本批自有、无未处置dirty资产的编辑器，实际核同工程无占用后，使用原CLI `graphs.verifySaved`、原输入、原生成报告和同stage，在**新PID**读回原资产；不重Import，不复制工程绕独占守卫。语法沿[原CLI例子](Readme.md)。运行、材质画面、打包本期未新增验收。

```powershell
python $NfCli describe --project $NfProject --plugin-directory $NfStage --run-directory "$NfRun\CommandletDescribe"
python $NfCli execute --operation graphs.verifySaved --project $NfProject --plugin-directory $NfStage --run-directory "$NfRun\MaterialVerify" --input "$NfRun\MaterialInput.json" --generation-report $NfGenerationReport --description-file "$NfRun\CommandletDescribe\Native.json"
```

核新PID与原生成PID不同、project/stage相同、persistence与结构读回passed；Blueprint同样使用自己的原输入/收据。此手工片段为静态对照配方，正式实测仍引用Run2，不另起测试批次。

## 4. 拒绝和未知时

无匹配/参数拒绝保留诊断；不会保留旧查询内容冒充本次成功。旧revision先读取当前稿，确认本次意图后使用新call；不要在资产结果未知时以新身份重新生成。pending/unknown立即停止后续写入，仅editor-status查**原call**并读原报告/进程/部分写入；无法闭合就保全现场和停点。工程复用台账有活动/未知run或占用时不分配下一批；原资产和请求收据不清空，旧历史工程/缓存不自动删除。
