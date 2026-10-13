param([Parameter(Mandatory=$true)][string]$ReportDirectory,[switch]$StageOnly,[string]$DiagnosticPathPrefix,[int]$MaxParallelActions=0)
$ErrorActionPreference = 'Stop'
$nfWorkspace = Split-Path -Parent $PSScriptRoot
$nfReportsRoot = [IO.Path]::GetFullPath((Join-Path $nfWorkspace 'Reports'))
$nfReportPath = [IO.Path]::GetFullPath((Join-Path $nfWorkspace $ReportDirectory))
if (-not $nfReportPath.StartsWith($nfReportsRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Report path must be inside workspace Reports.' }
if (Test-Path -LiteralPath $nfReportPath) { throw 'Use a new report directory; historical evidence is never overwritten.' }
New-Item -ItemType Directory -Path $nfReportPath | Out-Null
$nfEngine = 'X:\UnrealEngine\UE_5.3'
$nfProject = Join-Path $nfWorkspace 'DevProject\NodeForgeDev.uproject'
$nfArguments = @('NodeForgeDevEditor','Win64','Development',"-Project=$nfProject",'-WaitMutex','-NoHotReloadFromIDE','-CompilerVersion=14.34.31933','-ToolchainVersion=14.34.31933','-WindowsSdkVersion=10.0.22621.0',"-Log=$nfReportPath\Build.log")
if ($MaxParallelActions -lt 0) { throw 'MaxParallelActions must be nonnegative.' }
if ($MaxParallelActions -gt 0) { $nfArguments += "-MaxParallelActions=$MaxParallelActions" }
# Source filenames are diagnostic data only. Explicit staged release builds can
# trim a locally verified source prefix in both source and original PCH actions.
if ($DiagnosticPathPrefix) {
    if (-not $StageOnly) { throw 'Diagnostic-path release repair must be StageOnly.' }
    if ($DiagnosticPathPrefix -notmatch '^[A-Za-z]:\\$') { throw 'Use an explicit drive-root prefix verified for the current compiler and build roots.' }
    if (-not $nfWorkspace.StartsWith($DiagnosticPathPrefix,[StringComparison]::OrdinalIgnoreCase) -or -not $nfEngine.StartsWith($DiagnosticPathPrefix,[StringComparison]::OrdinalIgnoreCase)) { throw 'Both build source roots must match the verified diagnostic prefix.' }
    $nfArguments += @("-CompilerArguments=/d1trimfile:$DiagnosticPathPrefix",'-OverrideBuildEnvironment')
}
& "$nfEngine\Engine\Build\BatchFiles\Build.bat" @nfArguments *> (Join-Path $nfReportPath 'Console.txt')
$nfBuildExit = $LASTEXITCODE
if ($nfBuildExit -ne 0) { throw "UE build failed ($nfBuildExit). See preserved log." }
$nfBuildText = Get-Content -LiteralPath (Join-Path $nfReportPath 'Build.log') -Raw
if ($nfBuildText -notmatch '14\.34\.31948 toolchain' -and $nfBuildText -notmatch 'Target is up to date') { throw 'Expected actual MSVC 14.34.31948 was not observed; do not silently change the baseline.' }
# External plugin builds place output in DevProject. Sync our own generated module,
# then record both source and installed bytes; otherwise UE may load a stale plugin copy.
$nfSourceDir = Join-Path $nfWorkspace 'DevProject\Binaries\Win64'
$nfStageRoot = Join-Path $nfReportPath 'PluginStage\NodeForge'
$nfPluginDir = if ($StageOnly) { Join-Path $nfStageRoot 'Binaries\Win64' } else { Join-Path $nfWorkspace 'Plugins\NodeForge\Binaries\Win64' }
if ($StageOnly) {
    # Keep the user's live validation Editor on its unchanged installed binary.
    New-Item -ItemType Directory -Path $nfStageRoot | Out-Null
    foreach ($nfFile in @('NodeForge.uplugin','ThirdPartyNotices.md')) {
        Copy-Item -LiteralPath (Join-Path $nfWorkspace "Plugins\NodeForge\$nfFile") -Destination $nfStageRoot
    }
    foreach ($nfDirectory in @('Source','Licenses')) {
        Copy-Item -LiteralPath (Join-Path $nfWorkspace "Plugins\NodeForge\$nfDirectory") -Destination $nfStageRoot -Recurse
    }
}
New-Item -ItemType Directory -Path $nfPluginDir -Force | Out-Null
$nfDllName = 'UnrealEditor-NodeForgeEditor.dll'
$nfSource = Join-Path $nfSourceDir $nfDllName
$nfDestination = Join-Path $nfPluginDir $nfDllName
$nfOldHash = if (Test-Path -LiteralPath $nfDestination) { (Get-FileHash -LiteralPath $nfDestination -Algorithm SHA256).Hash } else { $null }
Copy-Item -LiteralPath $nfSource -Destination $nfDestination -Force
$nfManifest = Get-Content -LiteralPath (Join-Path $nfSourceDir 'UnrealEditor.modules') -Raw | ConvertFrom-Json
$nfPluginManifest = @{BuildId=$nfManifest.BuildId;Modules=@{NodeForgeEditor=$nfDllName}}
$nfPluginManifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $nfPluginDir 'UnrealEditor.modules') -Encoding utf8
$nfHash = (Get-FileHash -LiteralPath $nfSource -Algorithm SHA256).Hash
if ($nfHash -ne (Get-FileHash -LiteralPath $nfDestination -Algorithm SHA256).Hash) { throw 'Plugin binary copy hash mismatch.' }
@{buildExit=$nfBuildExit;arguments=$nfArguments;source=$nfSource;destination=$nfDestination;stageOnly=[bool]$StageOnly;previousSha256=$nfOldHash;sha256=$nfHash;buildId=$nfManifest.BuildId} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $nfReportPath 'BinaryEvidence.json') -Encoding utf8
Write-Output "Build passed and plugin bytes synchronized. Evidence: $nfReportPath"
