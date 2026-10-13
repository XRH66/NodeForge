# 原生多路选择数据回归

39项模板由Build49真实实例签名生成；随后在首次保存批次前补原生名称语义和精确引脚负例，最终以Build52重新采集目录及实际DLL运行。此前Build49/50守卫失败保留，不等于已运行这些保存模板。

Cases.json有21正例、18拒绝；RuntimeCases.json有14代表性原生执行期望。未连执行case保留，独立Sequence消息证明事件已执行，Actor位置证明分支选择，而不是把没打印消息当运行成功。

| 数据变化 | 模板及预期 |
| --- | --- |
| Selection/属性 | SwitchInt→SwitchChanged，X20→10 |
| 改连线 | SwitchTopology仍选1但连至第一目标，X10 |
| 数量与起点 | SwitchOne/八分支SwitchEight/负起点SwitchNegativeStart；另含MIN与最大安全末case保存重开 |
| 顺序 | SwitchShuffled与SwitchInt实际结构/自动布局一致 |
| 默认及未连接 | SwitchDefault X100；无Default的未匹配与有Default的未连命中保持X0 |
| 名称 | Idle/Active、canonical name成员、Selection=active原生匹配；引脚active替代Active拒绝 |
| 版本兼容 | 46数学、45材质、早期Branch图保持其旧摘要通过；旧46声明Switch拒绝 |
| 保护 | 类型/数量/溢出/名字重复和保留/未知引脚/隐藏上下文/连接同时默认/同名拒绝 |

实际构图/编译/保存/重开终态在[Switches1](../../Reports/20261008GraphV2/Switches1/Summary.json)，39项/60步通过。后续14图/28步实际运行、三图导出及五拒绝、原33/旧v1/11负例/规则全部终态，见[最终汇总](../../Reports/20261009UsableEditor/SwitchExpansion1/FinalSummary.json)。20行原生瞬态异常源守卫通过，历史失败原因及更正保留于[阶段日志](../../WorkLogs/2026-10-09-蓝图多路选择扩展.md)。[AI规则](../../Examples/BlueprintSwitchesAiRules.md)说明目录/端口与边界；新族UI/外部AI/打包未测。

这些模板不能直接粘贴导入：CATALOG_DIGEST_FROM_REAL_ENGINE须由实际加载插件导出的摘要替换；requestId和新目标应唯一。运行器另存绑定后的Inputs，不改变原模板；拒绝例不是使用说明。测试期间源码和DLL必须保持不变，不安装工具、修改生产或根/RC1。
