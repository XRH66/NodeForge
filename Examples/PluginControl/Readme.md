# 正式程序调用例子

A已交付Build122的两查询及工程复用流程见[RulesCatalog](RulesCatalog.md)/[最终报告](../../Reports/PluginControlStageTwo/RulesCatalog1/P3/FinalReport.md)。本页以下Build118/80配方保留对应版本，其“新空工程/重复正例新工程”是旧隔离策略；本期和以后常规测试默认复用登记工程、预分配批次短身份并累计保护，不按case/重试新建。旧配方没有新增查询，不应以它改写已登记工程或重跑A。

当前活动editor使用[Build118可复制例子](EditorControl.md)：短名专用空工程、显式注册、读取/载入/预检/生成/查旧call/disable及正常退出后的独立Verify。新例子仅静态检查，方法实际证据见[交付](../../Reports/PluginControlStageTwo/EditorControl1/S5/FinalReport.md)。下方Build80独立Commandlet配方继续保留。

适用本机UE5.3.2/已验证Build80，[契约](../../Contracts/ControlProtocol.md)及[真实证明](../../Reports/20261010PluginControl/Readme.md)。以下PowerShell参数都指向已有路径。ControlExamples80是本阶段新建空工程，仅执行过只读预检，尚无资产。UserRuns路径在交付时未创建；每次必须新run，重复先status，不覆盖旧结果。

```powershell
$NfPython = 'C:\Users\13179\AppData\Local\Programs\Python\Python312\python.exe'
$NfCli = 'X:\NodeForge\Scripts\NodeForgeControl.py'
$NfProject = 'X:\NodeForge\ValidationProject\RegressionRuns\ControlExamples80\NodeForgeControl.uproject'
$NfStage = 'X:\NodeForge\Reports\20261010PluginControl\Build80\PluginStage\NodeForge'
$NfDescription = 'X:\NodeForge\Reports\20261010PluginControl\M4\Operations.json'
$NfInput = 'X:\NodeForge\Examples\GraphV2Samples\BlueprintConcat.json'
& $NfPython $NfCli --help
& $NfPython $NfCli execute --help
& $NfPython $NfCli describe --project $NfProject --plugin-directory $NfStage --run-directory 'X:\NodeForge\Reports\PluginControlUserRuns\Describe1' --timeout-seconds 180
& $NfPython $NfCli execute --operation graphs.preflightFromFile --input $NfInput --project $NfProject --plugin-directory $NfStage --description-file $NfDescription --run-directory 'X:\NodeForge\Reports\PluginControlUserRuns\Blueprint1\Preflight' --timeout-seconds 180
```

先读退出值/机器结果，前一步终态满足约定才运行下一步；preflight只检查，不预留/生成。生成0且waitingPersistence仍须不同PID的Verify。

```powershell
& $NfPython $NfCli execute --operation graphs.generateFromFile --input $NfInput --project $NfProject --plugin-directory $NfStage --description-file $NfDescription --run-directory 'X:\NodeForge\Reports\PluginControlUserRuns\Blueprint1\Generate' --timeout-seconds 180
& $NfPython $NfCli execute --operation graphs.verifySaved --input $NfInput --generation-report 'X:\NodeForge\Reports\PluginControlUserRuns\Blueprint1\Generate\Native.json' --project $NfProject --plugin-directory $NfStage --description-file $NfDescription --run-directory 'X:\NodeForge\Reports\PluginControlUserRuns\Blueprint1\Verify' --timeout-seconds 180
& $NfPython $NfCli status --run-directory 'X:\NodeForge\Reports\PluginControlUserRuns\Blueprint1\Generate'
```

材质用原[StaticColor](../GraphV2Samples/StaticColor.json)和另一新run族，操作相同；[DynamicPulse](../GraphV2Samples/DynamicPulse.json)也有本次结构/编译/保存/新PIDVerify证据，动画/视觉未另验证。重复正例需新隔离工程，不自动换ID/名字或覆盖。

下面已有结果可以现在只读查询：

```powershell
& $NfPython $NfCli status --run-directory 'X:\NodeForge\Reports\20261010PluginControl\M2\Cases\BlueprintConcat\Generate'
& $NfPython $NfCli status --run-directory 'X:\NodeForge\Reports\20261010PluginControl\M2\Cases\StaticColor\Verify'
```

最终CLI/共享run_client/空工程的实际预检见[ExampleCheck](../../Reports/20261010PluginControl/M4/ExampleCheck.json)；同样生成/Verify语法已在[M2两例](../../Reports/20261010PluginControl/M2/Cases/Checks.json)及[M3重复/第三图](../../Reports/20261010PluginControl/M3/RepeatCases/Checks.json)实跑。UserRuns配方尚未执行，不冒充已有产物；每份run保留原输入SHA、PID/模块、收据与终态。
