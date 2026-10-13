# 固定最小回归

2026-10-08冻结原始夹具和当时完整两组证据。当前按[Workflow](../../Docs/Workflow.md#后续能力扩展优先级2026-10-09)及明确阶段授权选择受影响回归，集中验收再核完整范围，不要求每次局部补强机械重跑历史包；不能覆盖已存在目标/报告、删失败或把subset说成完整通过。该历史阶段和实际构建见[夜间记录](../../Reports/20261010NightAudit/Readme.md)。 当前项目状态见[ProjectStatus](../../ProjectStatus.md)与[第二阶段最终报告](../../Reports/PluginControlStageTwo/EditorControl1/S5/FinalReport.md)；本页原构建/夜间证据仍限定于各自时点，不因此继承新版全部验收。

下面保留2026-10-08旧安装调用模板。当前调用必须显式给RunPrototypeRegression的 `--plugin-directory <Reports内实际NodeForge stage>`、ValidatePrototype的 `--project <对应RegressionRuns非RC1工程>`；省略时分别沿用根插件与DevProject默认，不适用于本夜。新工程、目录和报告均须由主任务核实，不直接照抄旧名称执行。

1. `python Scripts/RunPrototypeRegression.py --run 新的字母数字名称`：原样读取[MinimalBlueprint](../Valid/MinimalBlueprint.json)、[MinimalMaterialReady](../Valid/MinimalMaterialReady.json)，创建独立RegressionRuns工程；各自生成、独立进程重开、固定关卡、实际PIE打印一次及最小Slate面板。所有七步骤需通过。入口、实际命令和摘要写入Reports/20261008UsablePrototype/该名称。
2. `python Scripts/ValidatePrototype.py --report-dir Reports/新的目录`：严格重放[11项清单](PrototypeCases.json)。要求全部拒绝符合预期，已有开发/验收测试资产字节不变；未知标识/引脚、错误类型/摘要/重复键/深度/未知字段、同名及请求字节变更均须保留。

固定夹具内容不能随实现改写。2026-10-08本轮最终[正例](../../Reports/20261008UsablePrototype/Regression5/Summary.json)和[负例](../../Reports/20261008UsablePrototype/Negative3/Summary.json)通过；两原始正例及11反例的SHA在本轮最终完整性报告复核。以后新能力需追加测试，不替换本组。

实际工具链以[Build8](../../Reports/20261008UsablePrototype/Build8/BinaryEvidence.json)及其Build.log为准：MSVC14.34.31948、安装目录14.34.31933、SDK10.0.22621.0。[构建脚本](../../Scripts/BuildPrototype.ps1)固定目录/SDK并核对实际日志。无C++纯蓝图Win64包采用引擎预编译UnrealGame，不把UAT命令参数误当另一轮MSVC编译证据。

面板烟测只检查真实Slate构造，不替代原生文件框/粘贴的[交互检查](../../Reports/20261008UsablePrototype/UiChecks.json)。视觉、无插件工程/包运行和外部AI数据另存对应报告；本组不能覆盖全Schema/故障注入/并发锁/通用节点。
