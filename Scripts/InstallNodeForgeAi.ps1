param(
    [Parameter(Mandatory=$true)][string]$Project,
    [string]$EngineRoot,
    [string]$PluginZip,
    [string]$ClientConfig,
    [ValidateSet('mcp-json','codex-toml')][string]$ClientFormat='mcp-json',
    [string]$ServerName='nodeforge',
    [switch]$Connect
)
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
$taskPython=Join-Path $taskRoot 'Tools/NodeForgeMcp/Runtime/python.exe'
$taskArguments=@((Join-Path $PSScriptRoot 'NodeForgeAiInstall.py'),'--project',$Project,'--client-format',$ClientFormat,'--server-name',$ServerName)
if ($EngineRoot) { $taskArguments+=@('--engine-root',$EngineRoot) }
if ($PluginZip) { $taskArguments+=@('--plugin-zip',$PluginZip) }
if ($ClientConfig) { $taskArguments+=@('--client-config',$ClientConfig) }
if ($Connect) { $taskArguments+='--connect' }
& $taskPython @taskArguments
exit $LASTEXITCODE
