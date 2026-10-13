# 程序控制验收运行器

第二阶段实际入口为[RunEditorChecks](RunEditorChecks.py)，既有参数化phase及原CLI/独立比较器共用，证据见[最终报告](../../Reports/PluginControlStageTwo/EditorControl1/S5/FinalReport.md)、[ST01–ST18映射](../../Reports/PluginControlStageTwo/EditorControl1/S5/AcceptanceAudit.json)和[16入口影响](../../Reports/PluginControlStageTwo/EditorControl1/S4/StateImpactReview120.json)。阶段全验收与低影响变更分开：已有相关证据/源码未变则引用，不因编号更新整批重跑。断连/unknown只查原call，不再Generate；每批显式加入全部累计SavedAssetProtection地图。下方原M运行器说明保留首阶段范围。

范围与出口见[FirstStagePlan](../../Docs/PluginControl/FirstStagePlan.md)，反馈见[ControlProtocol](../../Contracts/ControlProtocol.md)，真实结果见[实施报告](../../Reports/20261010PluginControl/Readme.md)。脚本仅调用正式CLI和独立检查，不另造UE资产/业务。

- RunCases消费Cases/RepeatCases，顺序预检→生成→不同PIDVerify，复用readback_errors与validate_terminal_step，固定预期和真实保存字节。pending/失败停后例，原输入重复正例用新工程。
- RunBoundaries消费Boundary/Supplement/IdentityCases，共用RunCases.run_client。身份变化只在明确负例副本，不改原输入，不为正例换名。
- RunProcessChecks使用真实自有短child和readonly UE，核创建ticks、截止/observer异常不kill、只读status/错误身份/晚到结果；所有等待有限，原LiveProcess保留。
- InvalidJson是故意坏输入。预期pending batch的failed/notRun原样保留，ExpectedPendingCheck及事前SHA Review证明正确停批次，不删失败制造成功。

三个运行器均有--help，显式project/plugin-directory/report-directory；正负例另需同DLL的description-file。report目录必须新；RunCases/RunBoundaries可选manifest，RunCases可设timeout。真实命令数组存在每个ClientCommand.json，CLI例子见[使用说明](../../Examples/PluginControl/Readme.md)。

本轮没改共享解析/构图/编译/保存，按影响测旧Smoke、unknownPin/duplicateKey、查重和guard；完整旧11矩阵未重跑。GUI接线不变，M06使用原源码SHA及同Execute调用链静态证明，真实GUI/运行/视觉不继承API结果。
