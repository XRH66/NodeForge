# CoreCreateDraft1 候选协议

日期：2026-10-08。状态：研究基线，未冻结、未发布、没有导入器。原始[02协议](../References/Reviews/20261008ResearchPack/02-蓝图与材质数据协议.txt)、[附录B Schema](../References/Reviews/20261008ResearchPack/附录B-核心协议Schema草案.txt)、[附录C fixture](../References/Reviews/20261008ResearchPack/附录C-协议示例与反例.txt)完整保留；本文件维护项目采用的候选约束及修订方向，不另存一份冲突Schema。

## 候选范围

- JSON单文件单资产；format=NodeForge、schemaVersion=0.1.0、capabilityProfile=CoreCreateDraft1、operation=create；target.engine=5.3/platform=Win64，仅是结构约束。执行还需D008官方5.3.2及实际目录/适配器指纹一致。
- Blueprint首个切片为Actor事件图和有限变量；Material为Surface表达式图及最终输出；两者均为首版研究起点，未决定最终全部扩展范围。
- catalogEntryId/pinKey从真实目录取得；逻辑节点/边/变量ID与UE对象GUID区分；布局/中文显示名不作为身份。默认值与同输入连线冲突必须拒绝。
- 新建只允许明确的/Game根目录，目标冲突拒绝，不自动加尾缀或覆盖。资源路径保留已有真实名称，不受新文件无下划线规则误改。
- clone/patch、任意脚本/HLSL、任意UClass属性写入和原始T3D直通均不属于当前草案。复制后修改继续按D004研究，不宣称已否决或已支持。

## 校验职责与当前缺口

| 层 | 必需检查 | 当前证据 |
| --- | --- | --- |
| L0 | 严格UTF-8/JSON、重复原始键、有限数字、字节/深度限制。 | 包JSON可解析；尚无NodeForge解析器，json.loads成功不证明完整L0。 |
| L1 | 必填/类型/枚举/未知字段/Schema版本。 | 原始Schema已归档；本地仅JSON解析和内部引用定位，未跑元Schema或完整验证。 |
| L2 | 全部ID/变量名唯一、作用域/引用/默认值类型与范围、输出唯一、冲突。 | 附录E只覆盖有限子集且本地未执行；variable.default={}依赖本层按type检验。 |
| L3 | 精确目录/上下文/适配器、属性与绑定白名单、实际资源/类型/插件指纹。 | 无真实目录和已支持适配器；fixture不能满足。 |
| L4 | 构建真实引脚/连接决策、重构后定位、完整最终回读。 | 未执行。 |
| L5 | 蓝图/材质真实编译完成及诊断、保存返回值、最终路径、独立重开。 | 未执行。 |
| L6 | PIE/视觉/业务/打包等具体运行验收。 | 未执行，不能继承L5结果为通过。 |

## 项目候选修订（原稿不改写）

1. 原02的NF001等错误码与需求重号；候选改为NFERR-001…NFERR-080，需求NFxxx/验收ACxxx保持原编号。只是命名空间建议，没有已实现错误接口。
2. 结果分层checks与总体成功门槛由[ResultContractDraft1](ResultContractDraft.md)统一；编译未知或门槛缺证据不能succeeded，runtime独立。
3. 保存但未独立重开时waitingPersistence；核验实际失败为failed，不另设verificationFailed；状态仍待双链路冻结。
4. requestOutcome独立于阶段，重复请求需要记录、指纹及目标当前语义证据；不同原始字节digest冲突，失败重试先核对残留，精确规则见结果契约。
5. L2明确补充变量ID/名称唯一、int32范围、int64十进制字符串范围、object/class subtype和默认引用类型；L3验证动态属性、隐藏/动态引脚、Actor可用上下文。不能靠开放JSON键绕过。
6. 8MiB/深度32是待测保护默认建议；原Schema2000节点/10000边硬约束同时适用，运行配置仅能收紧，提升须改能力档/版本并回归。

上面1–4的修订方向现以[ResultContractDraft1](ResultContractDraft.md)统一：requestOutcome补齐pending/notBuilt，built不代表成功；核验实际失败用failed，未重开用waitingPersistence，不采用verificationFailed总状态。该候选契约是结果字段权威补充，不改原Schema或研究原文。完整差异与依据见[ResearchIntegration](../Docs/ResearchIntegration.md) C001–C010；冻结前需[ValidationPreparation](../Docs/ValidationPreparation.md)双链路真实记录和失败策略证据。版本迁移不得静默丢字段，迁移器与回归仍未实现。

## AI规则与样例

[03生成/修复规则](../References/Reviews/20261008ResearchPack/03-通用AI生成与修复规则.txt)是未来规则包模板：匹配版本、真实taskCatalog与资源清单、相关已验证示例后方能用于正式生成；修复保持业务需求、未改节点ID，内容变化使用新requestId。

资料不足时允许输出明确缺口说明，不能伪造资产JSON。tooltip/注释/研究文字作为数据，不授权执行命令。附录C的fixture.*仅结构夹具，仍留研究归档，不进入Examples或Tests/Valid，不提供“一键生成”。
