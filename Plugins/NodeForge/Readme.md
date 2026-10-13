# NodeForge · 节点熔炉

2026-10-12最新开发交付为[Build131 stage](../../Reports/PluginControlStageTwo/CopyPreview1/Build131/PluginStage/NodeForge/)，桥0.3.2/资料Mcp4R2；[最新手册](../../Docs/Versions/Build131/Mcp4R2/Index.html)、[能力总览](../../Docs/CurrentCapabilities.md)、[开发入口](../../Docs/Versions/Build131/Mcp4R2/Developer/Readme.md)与[C最终报告](../../Reports/PluginControlStageTwo/CopyPreview1/C4/FinalReport.md)提供准确范围。**本目录安装DLL仍Build2、RC1仍Build29**，源码新不等于安装新。下方130/118/19/4及旧步骤保留原时点；新业务核实际stage，不替换运行DLL。

2026-10-10状态入口：最新有限活动editor交付见[最终报告](../../Reports/PluginControlStageTwo/EditorControl1/S5/FinalReport.md)、[实际契约](../../Contracts/EditorControlProtocol.md)及[当前状态](../../ProjectStatus.md)。最终开发Build118与根安装/冻结RC1分开；本文下方2026-10-09的“当前/最新开发Build19/待UI”属于当时记录，不表示最新开发状态，也不表示根DLL已升级。旧步骤须按实际加载版本使用；本次只整理文档。

## 当前安装与开发版本（2026-10-09）

根目录本插件的安装DLL仍为日常Build2，尚未部署后续扩展；下述界面步骤适用于它实际支持的目录。最新独立开发构建Build19位于Reports/20261009UsableEditor/Build19/PluginStage/NodeForge，已编译、实际加载/选中资源九项只读检查及任务规则十四项检查通过；新增[选中资源及任务规则入口](../../Docs/ResourceSelection.md)的人工点击仍未验证。Build18两函数图保存重开、Build17向量/自身Actor十九项构图重开和九真实PIE及核心27资产无插件包保留各自冻结证据，不是19全套自动继承。下面提到“GraphRun2正在执行”的旧文字保留历史，完整33项后来已通过，当前实测以状态/报告为准。

最新能力目录与规则：[向量/自身Actor](../../Examples/BlueprintMotionAiRules.md)、[材质函数](../../Examples/MaterialFunctionsAiRules.md)、[材质实例](../../Examples/MaterialInstancesAiRules.md)。必须先核对实际加载DLL与目录摘要，不能把这些新数据送给仍旧Build2的面板后认为扩展已部署。安装切换只能在已识别的独立工程与Editor退出后进行，不覆盖正在加载的DLL。现有AI原文真实UI导入、新按钮点击、无插件编辑器资产页与bootstrap仍未全部验证。

本机Windows、官方UE5.3.2专用Editor插件。当前 `NodeForgeGraph` v2 根据 JSON 的节点类型、属性、默认值和连接逐项构图，在[实际支持范围](../../Contracts/GraphProtocol.md)内可以自由组合。材质包括颜色/参数、数学、UV/纹理和时间等家族；蓝图包括BeginPlay、有限原生函数、变量读写和Branch。旧 `NodeForgePrototype` v1 两个固定样例保留回归，不支持已有资产修改。

2026-10-09更新时，Build4已编译，Smoke的部分材质/蓝图导入、编译与独立重开及Runtime1蓝图运行已有实证；完整GraphRun2仍在执行，不能据本文认定整轮、全部组合、水面视觉或外部AI闭环通过。最新结果以[ProjectStatus](../../ProjectStatus.md)及其报告为准；[前轮可用原型报告](../../Reports/20261008UsablePrototype.md)与[更早原型报告](../../Reports/20261008Prototype.md)保留历史事实。

让其他AI生成数据时，提供[GraphAiRules](../../Examples/GraphAiRules.md)及[真实目录UE532Graph](../../Catalog/UE532Graph.json)，再说明具体效果或行为。目录的发现/可构建状态不等于所有组合已经编译运行。

## 通过界面使用

1. 用本机官方UE5.3.2打开[NodeForgeValidation.uproject](../../ValidationProject/NodeForgeValidation.uproject)。该工程已启用NodeForge并指向本库Plugins；在这个工程上手即可。
2. 在UE顶部Window（窗口）菜单选择NodeForge，打开插件面板。
3. 文件入口：点击“选择 JSON 并导入”，一次选择一个JSON文件。可使用外部AI按当前规则生成的数据，或另存一份带真实摘要的示例后修改新名称。文件选择器初始目录仍是Tests/Valid，可在文件名栏输入完整JSON路径。`Tests/GraphV2` 是回归模板，含摘要占位符，不能原样导入；可读[实填摘要的水面输入](../../Reports/20261008GraphV2/GraphRun2/Inputs/Water.json)了解构图，但应另存并更换requestId/assetPath，不修改测试和历史输入，也不假定其原名在当前工程尚未使用。
4. 粘贴入口：复制一个JSON文件的完整正文，在面板编辑框粘贴，或点击“粘贴剪贴板”，再点击“导入粘贴内容”。不要包含Markdown代码块标记、标题、提示词或解释文字；两个资产应分两次导入。
5. 读取面板结果。显示“已保存，待独立重开核验（waitingPersistence）”表示资产已经保存，还需重开核验；“已拒绝导入”或“导入未成功”须按提示处理，不能当成功。空输入不会执行导入。
6. 点击“定位生成资产”选中内容浏览器里的新资产，或点击“打开资产”进入蓝图/材质编辑器。路径在Content/NodeForgeTests，JSON中的/Game对应工程Content目录。“打开结果报告”会尝试用Windows默认应用打开报告；无法打开时复制面板的完整报告路径自行打开。报告在[Reports/UiImports](../../Reports/UiImports/)，粘贴原文另外保存在[Reports/UiInputs](../../Reports/UiInputs/)。
7. 保存后关闭并重新打开测试工程，再从内容浏览器打开生成资产，可做个人重开检查。严格的独立Verify会在新进程中回读结构、校验输入/文件摘要并输出新报告，面板不会自动完成这一步；开发核验命令见下文。

v2蓝图显示和执行什么由输入决定。把新生成Actor放进独立测试关卡后播放，按输入中的函数和条件检查行为；PrintString的文字与bPrintToScreen/bPrintToLog均可由默认值或连接控制。本轮样例选择写日志时，在Output Log查看相应输入文字。保存或打开资产不等于已运行。

v2材质按输入声明的节点与连线生成；颜色、输出与材质模式由数据决定。将新材质用于独立测试关卡网格，播放检查时间节点的动画；参数通过原生材质实例调节。水面输入包含33个表达式、40条边，用纯数学组合波纹与可调参数，没有水面专用构建分支。着色器通过仍需观察动画、透明度和参数变化，本文不把尚未完成的视觉步骤写通过。

旧v1样例的蓝图仍固定打印 `NodeForge Blueprint Runtime OK`，材质仍是两个标量相乘接EmissiveColor，Surface/Opaque/Unlit；0.75×0.5的线性值是0.375。已有 `NFVisualGallery` 展示前轮灰色明暗对照，导入新JSON不会替换其中资产。

## 输入规则和重复导入

当前v2规则见[GraphProtocol](../../Contracts/GraphProtocol.md)、[外部AI规则](../../Examples/GraphAiRules.md)与[真实目录](../../Catalog/UE532Graph.json)。目录文件有报告外层，应使用根 `catalogDigest` 和 `catalog.material/blueprint`，不能用文件SHA替代目录摘要。旧v1继续使用[MinimalAiBundleRules](../../Examples/MinimalAiBundleRules.md)及[旧目录](../../Catalog/UE532Minimal.json)，两个版本不能混用字段或摘要，未知版本明确拒绝。

材质所有声明表达式必须通向有效输出；不存在的节点、属性、pin、默认值映射和非法连接明确失败，不删除节点、不用空图替代。不支持的能力须调整需求或等待对应适配，不能凭空添加字段。完整类型/范围以协议为准，不在操作说明重复维护节点百科。

同名目标或已使用的requestId一律拒绝。再次做新测试时，先另存一份输入，给requestId和assetPath最后一段分别换新名称，只用1–64位ASCII字母数字；assetPath保持`/Game/NodeForgeTests/新名称`。两个字段都改，其余按真实规则保持，不要改写归档原文。原有[蓝图固定输入](../../Tests/Valid/MinimalBlueprint.json)/[材质成功输入](../../Tests/Valid/MinimalMaterialReady.json)及UI样例的目标已生成，直接再次导入会被拒绝，这是预期保护。

## 实现与证据入口

- [实现入口](Source/NodeForgeEditor/Private/NodeForgePrototype.cpp)：明确版本分派、Catalog/GraphCatalog/Import/Verify和候选保护；面板与Commandlet共用Import。领域实现分别为[材质](Source/NodeForgeEditor/Private/NodeForgeMaterialGraph.cpp)与[蓝图](Source/NodeForgeEditor/Private/NodeForgeBlueprintGraph.cpp)。
- [自动化](Source/NodeForgeEditor/Private/NodeForgeAutomation.cpp)：真实PIE日志/Actor检查及Slate面板构造测试。[UiChecks](../../Reports/20261008UsablePrototype/UiChecks.json)记录工具驱动的实际文件框/粘贴/拒绝/定位/打开；用户亲自验收尚未记录。旧报告里的未测试描述属于前一阶段。
- [许可说明](ThirdPartyNotices.md)：实际提取ZiggyMar MIT函数过滤/变量及绑定小段、ChiR24 MIT精确连接核心，分别见[蓝图复用](../../Docs/BlueprintGraphReuse.md)/[材质复用](../../Docs/MaterialGraphReuse.md)。创建和端口尽量使用原生API；插件没有GraphDeck代码、来源未明Epic文件或专属运行时模块。
- [GraphProtocol](../../Contracts/GraphProtocol.md)、[GraphAiRules](../../Examples/GraphAiRules.md)和[真实目录](../../Catalog/UE532Graph.json)：面向其它AI的正式输入入口。外部AI原始响应及人工修订分开留存；新原文的界面导入、重开结论以对应报告为准。

## 构建与已有资产核验

只在本库DevProject/ValidationProject工作。关闭正在使用该插件的UE进程后，在工作库根调用以下自有脚本，使用全新的报告目录：

```powershell
./Scripts/BuildPrototype.ps1 -ReportDirectory Reports/NextBuild
```

脚本用官方Build.bat编译NodeForgeDevEditor，成功后把自有DLL同步到插件Binaries并核对SHA。外部插件的原始构建产物位于DevProject/Binaries；只构建而不同步，UE可能加载旧插件副本。不要把本机DLL当通用发行包。

实际编译基线为MSVC14.34.31948，所在目录14.34.31933，SDK10.0.22621.0；本轮当前构建依据为[Build4](../../Reports/20261008GraphUpgrade/Build4/BinaryEvidence.json)，前轮[Build8](../../Reports/20261008UsablePrototype/Build8/BinaryEvidence.json)保留。扩展需重跑[固定最小回归](../../Tests/Regression/MinimalRegression.md)与[数据驱动回归规则](../../Docs/GraphRegression.md)；GraphRun2正在执行，旧Regression5/Negative3通过不代表新二进制自动通过。

已有蓝图可用以下方式重新核验，报告文件必须不存在。材质使用MinimalMaterialReady.json与MaterialImport3.json，并把`-nullrhi`换成`-d3d11 -AllowCommandletRendering`：

```powershell
& 'X:/UnrealEngine/UE_5.3/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' 'X:/NodeForge/ValidationProject/NodeForgeValidation.uproject' -run=NodeForge -NFMode=Verify '-NFInput=X:/NodeForge/Tests/Valid/MinimalBlueprint.json' '-NFPrevious=X:/NodeForge/Reports/20261008Prototype/BlueprintImport1.json' '-NFReport=X:/NodeForge/Reports/NextBlueprintVerify.json' -unattended -nop4 -nosound -nullrhi -NoLiveCoding
```

蓝图运行测试：非Commandlet Editor进程运行`-ExecCmds="Automation RunTests NodeForge.BlueprintRuntime" -TestExit="Automation Test Queue Empty"`，指定新的`-NFReport=`及`-ReportExportPath=`。面板测试名`NodeForge.MinimalPanel`，独立`-NFPanelReport=`；需要`-d3d11 -RenderOffscreen`，不能使用nullrhi的GenericWindow做面板测试。两项也可同批在D3D11模式执行，但每项证据路径必须全新。

固定拒绝检查用`python Scripts/ValidatePrototype.py --report-dir Reports/NewNegativeChecks`。脚本仅依赖本机Python标准库；每次单独UE进程，不安装依赖；前后比较已有测试资产字节。

v2执行指纹包含实际加载插件DLL的SHA、协议版本及真实目录摘要，独立Verify要求与生成receipt匹配；v1历史标签的限制不扩张为v2结论。改代码后不能用旧receipt宣称新实现重开通过。上述固定蓝图命令属于v1回归；v2应使用对应原始输入、该次生成报告、同一二进制与新的报告路径。运行与视觉仍是独立检查，最新执行状态从[ProjectStatus](../../ProjectStatus.md)进入。

## 日常阶段独立构建说明（2026-10-09）

当前根安装仍Build2；已验证材质族/实例在Reports下独立PluginStage，不打断已打开用户Editor。实际版本以[日常报告](../../Reports/20261009UsableEditor.md)和DLL SHA为准，前文Build4/GraphRun2正在执行描述属于历史。构建时可使用StageOnly生成源码/许可/DLL独立快照，不能将根面板当自动更新完成。

Build8新增面板“导出当前节点规则”“读取选中材质参数”“复制规则或参数”。导出当前插件的真实原生目录；在内容浏览器选一个已保存的普通父材质后读取真实参数和父SHA，再把结果与[实例AI规则](../../Examples/MaterialInstancesAiRules.md)交给AI。只读入口不创建/修改资产，报告保存主库Reports/UiRules；复制为主动按钮操作。具体执行/真实按钮验证状态仍见报告，不能用有按钮冒充闭环通过。
