param(
    [string]$Project,
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [string]$PluginDirectory,
    [string]$PluginZip,
    [string]$ClientConfig,
    [ValidateSet('mcp-json','codex-toml')][string]$ClientFormat='mcp-json',
    [string]$ServerName='nodeforge'
)
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
$taskPython=Join-Path $taskRoot 'Tools/NodeForgeMcp/Runtime/python.exe'
$taskArguments=@((Join-Path $PSScriptRoot 'NodeForgeAiInstall.py'),'--client-format',$ClientFormat,'--server-name',$ServerName)
if ($Project) { $taskArguments+=@('--project',$Project) }
if ($EngineRoot) { $taskArguments+=@('--engine-root',$EngineRoot) }
if ($PluginZip) { $taskArguments+=@('--plugin-zip',$PluginZip) }
if ($ClientConfig) { $taskArguments+=@('--client-config',$ClientConfig) }
if ($PluginDirectory) { $taskArguments+=@('--plugin-directory',$PluginDirectory) }
& $taskPython @taskArguments
exit $LASTEXITCODE
