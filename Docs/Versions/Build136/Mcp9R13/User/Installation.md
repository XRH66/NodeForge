# 解压、绑定工程与第一次打开

本页对应 NodeForge-Build136-Mcp9R13 / Build136 Repair6 / 桥 0.3.6。需要 Windows x64、已安装的官方 UE5.3.2，以及 Python3.12。包不包含引擎、已有测试工程、个人配置或 Python 环境；页面里的变量表示安装后由你选择的一次性本机路径。

## 1. 放置候选包与准备 Python

解压到一个独立目录，确认 InstallationManifest.json、PluginStage/NodeForge、Scripts、Tools/NodeForgeMcp/Packages、Docs 和 Metadata 齐全。不要把正在运行的插件 DLL 换掉。根安装或其它候选不会因解压自动升级。

在 PowerShell 中设置三个变量：

```powershell
$installRoot = '替换为解压目录的绝对路径'
$projectFile = '替换为选定工程uproject的绝对路径'
$engineRoot = '替换为已安装UE5.3.2根目录的绝对路径'
```

先确认 Python 是3.12且64位。首次安装只在新的安装目录创建环境；已有环境不能靠覆盖来修复：

```powershell
python -m venv "$installRoot/Tools/NodeForgeMcp/Environment"
$pythonExe = "$installRoot/Tools/NodeForgeMcp/Environment/Scripts/python.exe"
& $pythonExe -m pip install --no-index --find-links "$installRoot/Tools/NodeForgeMcp/Packages" --require-hashes -r "$installRoot/Tools/NodeForgeMcp/Requirements.lock"
```

依赖取自包内固定 wheel 和锁定 SHA，不下载其它版本。若缺 Python、UE 或 wheel，先补对应前置，准备工具不会自动安装或启动引擎。官方 MCP SDK 为2.3.0，应用 ABI 为1。

## 2. 显式绑定一个工程

正常结束该工程自己的 UE。准备工具会检查占用、实际 DLL/模块/引擎身份和重复插件，不会结束其它进程。执行一次：

```powershell
& $pythonExe "$installRoot/Scripts/PrepareNodeForgeInstallation.py" --action bind --installation-root $installRoot --project $projectFile --engine-root $engineRoot
& $pythonExe "$installRoot/Scripts/PrepareNodeForgeInstallation.py" --action check --installation-root $installRoot --project $projectFile --engine-root $engineRoot
```

bind只修改所选工程的唯一插件引用和 Config/NodeForgeInstallation.json，保留无关设置，并返回原字节/修改 SHA 的私有 Receipt.json。该绑定让读取目录与创建目标分开：默认读取测试内容、EngineResources和有限材质函数目录；其它工程内容需要 bind 时显式添加允许的 --read-root 子目录。不能选择整个 /Game 或 /Engine 根，不能扩展生成写根。

不要把生成后的本机绑定、收据或工程发到公共包。撤销本次安装使用原 bind 的 receipt：

```powershell
& $pythonExe "$installRoot/Scripts/PrepareNodeForgeInstallation.py" --action rollback --installation-root $installRoot --project $projectFile --engine-root $engineRoot --receipt '替换为本次原Receipt.json绝对路径'
```

只有文件仍等于本次 after SHA 才撤销；用户后续改动、部分写入、占用或冲突必须保留并查原报告。

## 3. 打开界面或准备 AI

只使用界面时，打开已绑定工程，在 Window（窗口）菜单选择 NodeForge。载入完整 JSON → 检查输入 → 生成新资产；结果、定位资产和打开资产见[开始使用](GettingStarted.md)。JSON2/3共用原生产核心，原资产不覆盖。

需要 AI 时执行：

```powershell
& $pythonExe "$installRoot/Scripts/PrepareNodeForgeInstallation.py" --action prepare-editor --installation-root $installRoot --project $projectFile --engine-root $engineRoot --python $pythonExe
```

它生成本次专用 Registration.json、Config.json、Commands.json 和 PreparationReceipt.json；不会启动 UE，也不会改客户端设置。读取返回 commands.mcp 填入客户端，名称沿已有 NodeForge 登记：启动命令是 command；args每一项各占一行；工作目录是 cwd。不要把它们拼成一个带引号的字符串。

操作者按 commands.editor 的独立命令/参数显式启动自有 UE；真正发布本次 SessionPublic.json 后，AI才调用 connect。完整权限、调用、正常结束和独立 Verify 见[AiPreparation](../Ai/Preparation.md)。MCP自身没有启动 UE、分配批次、清锁或退出编辑器工具。

## 4. 第一次任务

新建材质或普通 Actor 蓝图可使用[完整合法例子](../Reference/Examples.md)，先取得当前目录摘要并使用新的 requestId 和 /Game/NodeForgeTests/ 下完整目标。资源选用按[资源配方](../Ai/Resources.md)先查询，再精确检查，再引用；列表不等于可生成或可导出。

生成返回 waitingPersistence 时，仅表示编译并保存。要求独立持久化结论时，在原输入/原生成报告/同插件版本下正常结束本次 UE，再由新进程执行原 CLI Verify；不要再次导入或保存。画面与打包是其它结果，不能由编译代替。
