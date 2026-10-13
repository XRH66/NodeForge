# 活动编辑器调用示例（Build118 / Editor API 1.0）

适用[实际契约](../../Contracts/EditorControlProtocol.md)，最终stage为[Build118](../../Reports/PluginControlStageTwo/EditorControl1/Build118/PluginStage/NodeForge/NodeForge.uplugin)。已准备空的专用[NFCExample118工程](../../ValidationProject/RegressionRuns/NFCExample118/NFCExample118.uproject)和[注册模板](EditorRegistration118.json)，没有启动它或生成资产。以下配方的相同CLI/方法/字段已有实际证据；本例用户目录尚未执行，不冒充新增运行结果。使用同一个专用editor顺序生成两图，既有目标/报告拒绝覆盖，重复配方要新独立工程和新注册根。

## 在第一个PowerShell终端启动专用工程

此段只启动该示例editor，保持这个终端；在第二个终端调用后端。使用官方本机5.3.2和准确stage，不替换根/RC1/DLL。启动正常成功后公共SessionPublic.json才会出现；无该文件就读注册根的日志/身份拒绝，不能猜旧session。

```powershell
$NfPython = 'C:\Users\13179\AppData\Local\Programs\Python\Python312\python.exe'
$NfEditor = 'X:\UnrealEngine\UE_5.3\Engine\Binaries\Win64\UnrealEditor.exe'
$NfProject = 'X:\NodeForge\ValidationProject\RegressionRuns\NFCExample118\NFCExample118.uproject'
$NfStage = 'X:\NodeForge\Reports\PluginControlStageTwo\EditorControl1\Build118\PluginStage\NodeForge'
$NfPublic = 'X:\NodeForge\Reports\PluginControlStageTwo\UserExamples\Editor118First'
if (Test-Path -LiteralPath $NfPublic) { throw '报告根已存在，请为重复运行准备新的独立工程和注册根。' }
$NfBinary = Join-Path $NfStage 'Binaries\Win64\UnrealEditor-NodeForgeEditor.dll'
if ((Get-FileHash -LiteralPath $NfBinary -Algorithm SHA256).Hash.ToLowerInvariant() -ne 'bb76d52d56a6aa14a9d4b834a74cb61723558f6eeb3b6bd2bba6eb59e1449e3a') { throw 'DLL与本例不匹配。' }
New-Item -ItemType Directory -Path $NfPublic -ErrorAction Stop | Out-Null
$NfRegistration = Join-Path $NfPublic 'Registration.json'
Copy-Item -LiteralPath 'X:\NodeForge\Examples\PluginControl\EditorRegistration118.json' -Destination $NfRegistration -ErrorAction Stop
& $NfEditor $NfProject '-NFControlEnable' "-NFControlRegistration=$NfRegistration" '-NoLiveCoding' '-d3d11' '-nop4'
```

## 在第二个终端读取状态并顺序生成

不传凭据值。客户端从当前public定位受限session文件，自己核OS身份并完成hello。session来自当前注册根，不使用交付包里停止后的历史identity。

```powershell
$NfPython = 'C:\Users\13179\AppData\Local\Programs\Python\Python312\python.exe'
$NfCli = 'X:\NodeForge\Scripts\NodeForgeControl.py'
$NfProject = 'X:\NodeForge\ValidationProject\RegressionRuns\NFCExample118\NFCExample118.uproject'
$NfStage = 'X:\NodeForge\Reports\PluginControlStageTwo\EditorControl1\Build118\PluginStage\NodeForge'
$NfPublic = 'X:\NodeForge\Reports\PluginControlStageTwo\UserExamples\Editor118First'
$NfPublicIdentity = Get-Content -LiteralPath (Join-Path $NfPublic 'SessionPublic.json') -Raw -Encoding utf8 | ConvertFrom-Json
$NfSessionFile = $NfPublicIdentity.sessionFile
$NfCommon = @('--session-file', $NfSessionFile, '--project', $NfProject, '--plugin-directory', $NfStage, '--timeout-seconds', '180')
$NfClientRoot = Join-Path $NfPublic 'Client'

function Invoke-NfEditor {
    param([string]$Label, [string]$Operation, [string]$ArgumentsFile, [string]$Revision)
    $NfArgs = @('editor-call') + $NfCommon + @('--run-directory', (Join-Path $NfClientRoot $Label), '--operation', $Operation, '--call-id', $Label)
    if ($ArgumentsFile) { $NfArgs += @('--arguments-file', $ArgumentsFile) }
    if ($Revision -ne '') { $NfArgs += @('--expected-revision', $Revision) }
    $NfText = & $NfPython $NfCli @NfArgs
    $NfCode = $LASTEXITCODE
    $NfValue = ($NfText -join [Environment]::NewLine) | ConvertFrom-Json
    if ($NfCode -ne 0) { throw "调用 $Label 返回 $NfCode。保留原run/call，只读查证后处理，不自动生成重试。" }
    return $NfValue
}
$NfState = Invoke-NfEditor -Label 'InitialState1' -Operation 'editor.getState' -Revision ''
$NfDraft = Invoke-NfEditor -Label 'InitialDraft1' -Operation 'draft.get' -Revision ''
$NfRevision = [string]$NfDraft.response.result.draftRevision
$NfGenerations = @()
$NfCases = @(
    @{ Name='Blueprint'; Arguments='X:\NodeForge\Examples\PluginControl\BlueprintLoad.json' },
    @{ Name='Material'; Arguments='X:\NodeForge\Examples\PluginControl\MaterialLoad.json' }
)
foreach ($NfCase in $NfCases) {
    $NfLoaded = Invoke-NfEditor -Label ($NfCase.Name+'Load1') -Operation 'draft.loadFile' -ArgumentsFile $NfCase.Arguments -Revision $NfRevision
    $NfRevision = [string]$NfLoaded.response.result.result.draft.draftRevision
    $NfPreflight = Invoke-NfEditor -Label ($NfCase.Name+'Preflight1') -Operation 'graphs.preflight' -Revision $NfRevision
    $NfGenerated = Invoke-NfEditor -Label ($NfCase.Name+'Generate1') -Operation 'graphs.generate' -Revision $NfRevision
    $NfReceipt = $NfGenerated.response.result.result
    $NfNative = Get-Content -LiteralPath $NfReceipt.nativeReport.path -Raw -Encoding utf8 | ConvertFrom-Json
    if ($NfNative.overallStatus -ne 'waitingPersistence') { throw '未取得预期保存终态，停止后续写入。' }
    $NfGenerations += [pscustomobject]@{ Name=$NfCase.Name; Input=$NfReceipt.inputSnapshot; Native=$NfReceipt.nativeReport.path }
}
```

UI可以显示同一稿/结果，但示例执行期间若你改稿，旧revision会正确拒绝，不能偷偷改成最新revision重发生成。保存后、停止通道前，可用下面的只读查询读取原Call。查询非零时同样先保留现场，不继续停止/生成步骤。

```powershell
& $NfPython $NfCli editor-status @NfCommon --call-id 'BlueprintGenerate1' --run-directory (Join-Path $NfClientRoot 'BlueprintOldCallQuery1')
if ($LASTEXITCODE -ne 0) { throw '旧call尚无确定终态，保留原run并只读查证。' }
$NfStopped = Invoke-NfEditor -Label 'Disable1' -Operation 'control.disable' -Revision ''
```

如发生5/pending/unknown，请保留发生时的callId，用当前仍有效session只读查询旧call，不能重新执行上面的foreach或换ID重跑。成功disable后旧session失效，改查已保存的客户端run与原生报告。

## 正常关闭该editor后独立Verify

control.disable只关通道，不退出UE。仅正常关闭这一个专用editor，确认第一个终端返回，再在原第二终端运行下段；客户端的工程独占守卫会拦住尚在运行的同工程editor，不放宽它。保留 $NfGenerations，或从两个Generate客户端原Summary恢复准确 inputSnapshot/nativeReport 引用。

```powershell
$NfCmdDescription = 'X:\NodeForge\Reports\PluginControlStageTwo\EditorControl1\S5\CommandletOperations.json'
foreach ($NfSaved in $NfGenerations) {
    & $NfPython $NfCli execute --operation graphs.verifySaved --input $NfSaved.Input --generation-report $NfSaved.Native --project $NfProject --plugin-directory $NfStage --description-file $NfCmdDescription --run-directory (Join-Path $NfClientRoot ($NfSaved.Name+'Verify1')) --timeout-seconds 180
    if ($LASTEXITCODE -ne 0) { throw '独立重开失败或尚无确定终态，保留原结果，不重Import。' }
}
```

预检/生成/Verify语法来自现有正式CLI和实际[程序双图](../../Reports/PluginControlStageTwo/EditorControl1/S3/ReviewBC.json)、[最终118旧CLI](../../Reports/PluginControlStageTwo/EditorControl1/S4/ReviewCompatibility120.json)；不同版本证据的准确源码连续性沿[影响审计](../../Reports/PluginControlStageTwo/EditorControl1/S4/StateImpactReview120.json)。本示例新工程没有运行/画面验收。草稿replace可用UTF-8对象 `{"text":"..."}` 的arguments-file，restore为无arguments调用且必须有当前revision；它们的实际正负例见S2C报告，不自动执行第二轮示例。
