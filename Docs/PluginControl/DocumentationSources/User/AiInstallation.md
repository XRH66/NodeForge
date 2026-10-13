# AI接入安装

让AI通过MCP查询节点、读取资源、编辑草稿并提交生成任务。AI接入是普通插件之外的可选步骤；只在UE面板操作的用户可以跳过本页。

## 一次配置包：尚未发布

计划中的AI包将自带Python和固定依赖。目标流程是解压包、打开配置入口、确定一个工程，再使用生成的客户端配置。用户不用安装Python、创建环境或敲pip命令。

当前没有可下载的一键安装程序。客户端能否自动登记还需分别适配；安装包本身也不会代替当前编辑器的连接确认。

## 当前AI候选：首次配置

本页使用 **Build136 Repair6完整候选包**，不是Build137普通插件包。AI自带环境的新包尚未发布，当前仍需Python3.12准备固定依赖。

### 1. 解压完整包并准备工程

解压Build136完整候选，进入其中的NodeFroge目录。应能看到InstallationManifest.json、PluginStage、Scripts、Metadata和Tools。正常关闭本次所选工程的UE，打开PowerShell，把下面三个占位项改成自己的真实路径：

```powershell
$installRoot = 'Build136完整候选包内NodeFroge目录的绝对路径'
$projectFile = '你的工程uproject文件的绝对路径'
$engineRoot = '已安装UE5.3.2的根目录绝对路径'
py -3.12 "$installRoot/Scripts/PrepareNodeForgeInstallation.py" --action bind --installation-root $installRoot --project $projectFile --engine-root $engineRoot
py -3.12 "$installRoot/Scripts/PrepareNodeForgeInstallation.py" --action check --installation-root $installRoot --project $projectFile --engine-root $engineRoot
```

两次结果应为passed，保留原bind收据。已绑定同一完整包与工程时跳过重复绑定；不要在已有Build137普通插件的工程中叠放第二份NodeForge。后续独立入口会用选择器取得真实路径，目前这份AI包仍按以上步骤准备。

### 2. 准备包内AI环境

当前包已提供固定依赖文件；需要已有64位Python3.12。首次安装在新的候选目录执行：

```powershell
py -3.12 -m venv "$installRoot/Tools/NodeForgeMcp/Environment"
$pythonExe = "$installRoot/Tools/NodeForgeMcp/Environment/Scripts/python.exe"
& $pythonExe -m pip install --no-index --find-links "$installRoot/Tools/NodeForgeMcp/Packages" --require-hashes -r "$installRoot/Tools/NodeForgeMcp/Requirements.lock"
```

这条命令只从包内安装固定依赖，不联网下载其它版本。已有环境不要覆盖重建；缺文件、版本不匹配或安装报错时先保留原结果。

### 3. 生成连接配置

```powershell
& $pythonExe "$installRoot/Scripts/PrepareNodeForgeInstallation.py" --action prepare-editor --installation-root $installRoot --project $projectFile --engine-root $engineRoot --python $pythonExe
```

**确认成功：** 结果为passed，并给出本次Commands.json和准备收据的位置。工具不会自动启动UE或改你的AI客户端设置。

### 4. 添加客户端并打开UE

从Commands.json读取commands.mcp，按[客户端填写说明]({{page:UserAiWorkflows}})逐项填入。启动命令、参数、工作目录都有实际生成值，不需要猜路径。

随后按commands.editor生成的命令打开所选工程。完整执行配方见[工程准备]({{page:AiPreparation}})；等待本次编辑器会话就绪后，AI才能连接业务。

### 5. 确认AI能读到插件

先让AI读取NodeForge版本索引和快速开始，确认本次资料与插件版本一致，再检查当前编辑器连接。能读到手册和能操作UE是两项确认。

可以这样告诉AI：

> 先读取NodeForge版本索引和AI快速开始，确认当前编辑器是否准备好。说明本次支持的操作和生成目标，不要直接生成资产。

连接完成后的任务例子见[使用AI]({{page:UserAiWorkflows}})。

## 常见问题

- **AI能读手册，但不能生成：** 检查当前编辑器是否按本次准备命令启动并完成连接。
- **找不到Python或依赖：** 当前候选还需要上面的环境准备；自带运行环境的新包尚未发布。
- **参数填哪里：** commands.mcp.args的每一项是一个参数，不要拼成一整行；见[填写示例]({{page:UserAiWorkflows}})。
- **调用等待或结果未知：** 让AI查询原调用和报告，先停后续写入；见[结果与恢复]({{page:AiResultsAndRecovery}})。
