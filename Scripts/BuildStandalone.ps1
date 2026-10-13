param(
    [Parameter(Mandatory=$true)][string]$ReportDirectory,
    [string]$ProjectPath='ValidationProject/StandaloneChecks/NodeForgeStandalone/NodeForgeStandalone.uproject',
    [string]$ArchiveDirectory='Builds/UsablePrototypeWin64',
    [string[]]$Maps=@('/Game/NodeForgeTests/NFVisualGallery')
)
$ErrorActionPreference='Stop'
$nfWorkspace=Split-Path -Parent $PSScriptRoot
$nfReports=[IO.Path]::GetFullPath((Join-Path $nfWorkspace $ReportDirectory))
$nfReportRoot=[IO.Path]::GetFullPath((Join-Path $nfWorkspace 'Reports'))
if (-not $nfReports.StartsWith($nfReportRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Reports must stay in project Reports.' }
if (Test-Path -LiteralPath $nfReports) { throw 'Use a fresh evidence directory.' }
$nfProject=[IO.Path]::GetFullPath((Join-Path $nfWorkspace $ProjectPath))
$nfStandaloneRoot=[IO.Path]::GetFullPath((Join-Path $nfWorkspace 'ValidationProject/StandaloneChecks'))
if (-not $nfProject.StartsWith($nfStandaloneRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetExtension($nfProject) -ne '.uproject') { throw 'Use only a standalone validation copy.' }
$nfDescriptor=Get-Content -LiteralPath $nfProject -Raw | ConvertFrom-Json
if ($nfDescriptor.AdditionalPluginDirectories -or ($nfDescriptor.Plugins | Where-Object { $_.Name -eq 'NodeForge' -and $_.Enabled })) { throw 'Standalone copy must not find or enable NodeForge.' }
if (Test-Path -LiteralPath (Join-Path (Split-Path -Parent $nfProject) 'Plugins')) { throw 'Standalone copy must not contain plugin files.' }
$nfArchive=[IO.Path]::GetFullPath((Join-Path $nfWorkspace $ArchiveDirectory))
$nfBuildRoot=[IO.Path]::GetFullPath((Join-Path $nfWorkspace 'Builds'))
if (-not $nfArchive.StartsWith($nfBuildRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Archive must stay in project Builds.' }
if ($Maps.Count -eq 0) { throw 'Explicit test maps required.' }
foreach ($nfMap in $Maps) {
    if ($nfMap -notmatch '^/Game/NodeForgeTests/(?:[A-Za-z0-9]{1,64}/)*[A-Za-z0-9]{1,64}$') { throw 'Only exact isolated test maps are allowed.' }
    $nfMapFile=Join-Path (Split-Path -Parent $nfProject) ('Content/'+$nfMap.Substring(6)+'.umap')
    if (-not (Test-Path -LiteralPath $nfMapFile)) { throw "Cook map absent: $nfMap" }
}
if (Test-Path -LiteralPath $nfArchive) { throw 'Archive target exists; never overwrite a prior build.' }
New-Item -ItemType Directory -Path $nfReports | Out-Null
$nfArguments=@('BuildCookRun',"-project=$nfProject",'-noP4','-platform=Win64','-clientconfig=Development',
    '-build','-skipbuildeditor','-cook','-stage','-pak','-archive',"-archivedirectory=$nfArchive",('-map='+($Maps -join '+')),
    '-unattended','-utf8output','-ubtargs=-CompilerVersion=14.34.31933 -ToolchainVersion=14.34.31933 -WindowsSdkVersion=10.0.22621.0')
@{command='X:/UnrealEngine/UE_5.3/Engine/Build/BatchFiles/RunUAT.bat';arguments=$nfArguments;project=$nfProject;archive=$nfArchive;started=(Get-Date).ToUniversalTime().ToString('o')} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $nfReports 'Command.json') -Encoding utf8
& 'X:/UnrealEngine/UE_5.3/Engine/Build/BatchFiles/RunUAT.bat' @nfArguments *> (Join-Path $nfReports 'Console.txt')
$nfExit=$LASTEXITCODE
@{exitCode=$nfExit;finished=(Get-Date).ToUniversalTime().ToString('o');archive=$nfArchive;scope='Win64 Development packaging only; packaged execution is separate.'} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $nfReports 'Exit.json') -Encoding utf8
if ($nfExit -ne 0) { throw "Packaging failed ($nfExit); evidence preserved." }
$nfFiles=@(Get-ChildItem -LiteralPath $nfArchive -File -Recurse)
$nfGeneratorFiles=@($nfFiles | Where-Object { $_.Name -match 'NodeForgeEditor|NodeForge\.uplugin' })
if ($nfGeneratorFiles.Count -ne 0) { throw 'Unexpected generator files in archive.' }
@{result='passed';exitCode=$nfExit;generatorFiles=@();files=@($nfFiles | ForEach-Object { @{path=$_.FullName;length=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash} })} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $nfReports 'Summary.json') -Encoding utf8
Write-Output "Win64 packaging passed; generator files absent. Evidence: $nfReports"
