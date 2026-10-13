# 普通材质实例回归

**2026-10-10 夜间阶段历史入口**：以下19项是本目录历史矩阵和绑定规则，当时实际构建/父资源/通过范围见[夜间审计记录](../../Reports/20261010NightAudit/Readme.md)。原模板及过期父SHA反例保持；当前同DLL原生参数与实际保存父字节须重新核对，不能从旧目录或旧结果推断新构建全部通过。执行按[Workflow](../../Docs/Workflow.md#后续能力扩展优先级2026-10-09)选择受影响范围。 当前项目状态见[ProjectStatus](../../ProjectStatus.md)与[第二阶段最终报告](../../Reports/PluginControlStageTwo/EditorControl1/S5/FinalReport.md)；本页原构建/夜间证据仍限定于各自时点，不因此继承新版全部验收。

使用[真实目录](../../Catalog/UE532MaterialInstances.json)、[协议](../../Contracts/GraphProtocol.md)和[AI规则](../../Examples/MaterialInstancesAiRules.md)。本目录19项：两个父材质种子+五个实例正例（两组水面参数、全继承、显式原生默认、纹理参数覆盖），12个拒绝例（名称/类型/值范围/颜色维数、父SHA/父不存在/实例父、节点非空、未知设置、纹理不存在、同名）。

原始实例JSON包含全零父SHA占位，是测试模板。现有RunGraphRegression.py首先生成并独立重开父材质，再执行原生只读MaterialParameters，写独立InputTemplates、实际Inputs与InputBinding。只绑定实测父SHA；故意过期例保留零值。模板、源SHA和执行输入SHA分别留证，不改原始模板冒充可直接导入文件。

矩阵不编译插件。统一代理选定同一冻结构建串行执行；检查实例真实nativeClass、完整override键/值/有效值、父字节、继承shader、保存及新进程重开。每个拒绝案例前后整个独立Content资产集合/字节必须相同。父参数名称从原生报告核对，不使用fixture节点百科。

实际运行状态见[日常报告](../../Reports/20261009UsableEditor.md)，不把有清单当作测试通过。用户面板、最终美术质量、无插件打包均单独测试，不由本矩阵代替。
