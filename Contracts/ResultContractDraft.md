# 结果、重复请求与成功条件候选契约

## 2026-10-10 明确准备新副本的增补

Build78 增加辅助模式 `PrepareNewCopy`，输入仍是现有 v1/v2 请求；不改变协议根字段。该模式 `overallStatus=succeeded` 仅代表校验及派生输入准备完成，`requestOutcome=notBuilt`，build/compile/save/persistence/runtime 为 `notRun`，不能显示“资产已保存”。`copyPreparation` 记录原、新 assetPath/requestId、来源与派生文件摘要、命名提供者、只更改两项身份字段、`assetGenerated=false`、`targetReserved=false`。UI 后续生成收据用 `uiInput.copyPreparationReport` 关联准备报告；实际生成/独立 Verify 仍采用原八门槛。

Import、GraphPreflight、GraphPreview 的已有目标或请求冲突通过 `conflict.kind` 区分 `targetExists`、`requestExists`、`targetAndRequestExist`，对应 `NFERR-TARGET-EXISTS`、`NFERR-REQUEST-EXISTS`、`NFERR-TARGET-AND-REQUEST-EXIST`。记录已有目标/请求及可取得的原请求收据，明确本次未覆盖；已有文件不直接标成本次失败残留。旧收据不回写。规则见[资产命名](../Docs/AssetNaming.md)，实测见[Build78 报告](../Reports/20261010NamingRepair1/Readme.md)；下方历史候选保留原时点。

2026-10-09 补充：新输入协议为 [NodeForgeGraph v2](GraphProtocol.md)。它沿用分层结果、create-only、waitingPersistence/独立Verify及真实残留报告，并在执行指纹中增加 protocolVersion=2 与实际加载模块的 binarySha256；v1指纹保持兼容。节点默认值、资产设置、实际边与声明映射由各领域适配器回读，失败也保存对应回读错误。运行/视觉报告仍独立，Import或Verify通过不能自动使runtime/visual通过。下方研究候选及v1历史实现说明保留原时点；完整错误码、原子锁和全故障矩阵仍未声明实现。

日期：2026-10-08。状态：ResultContractDraft1，未冻结；本轮原型已实现受限切片，详细偏差/未覆盖项见文末。对应[ResearchIntegration](../Docs/ResearchIntegration.md) C001–C003；补充[CoreCreateDraft1](CoreCreateDraft.md)，不覆盖原02、06、08或附录B原文，也不伪称它们已有这些字段。

## 三处统一修订

1. 需求继续用NFxxx、验收ACxxx；运行错误统一候选命名NFERR-xxx。研究02错误数字可保留在迁移映射中，但正式报告不能写NF001而让人误认为需求。错误登记包含code、stage、severity、message、输入位置、逻辑节点/边/pinKey、nativeDiagnostic、evidenceRef；无法定位仍保留消息。不先凭空冻结080个实际已实现错误码。
2. requestOutcome只表达请求处理结果，与pipeline阶段和overallStatus独立。使用pending、notBuilt、built、alreadyCompleted、conflict；built表示本次已对自有新候选发生构造写入，**不代表构造完整或编译/保存成功**，完整性由build check判断。输入被拒绝/构造前失败为notBuilt；构造中途失败也为built并如实列残留。
3. 构建、编译、保存和持久化分别有check。没有独立重开证明不能整体succeeded；编辑器内已保存但无法安全重载时明确waitingPersistence。UI只能显示“已保存，待重开核验”，完整成功入口由整体状态控制。

## 报告字段及候选枚举

| 字段 | 内容/约束 |
| --- | --- |
| reportVersion | ResultContractDraft1；协议schemaVersion仍CoreCreateDraft1候选0.1.0，二者不混同。 |
| requestId / inputDigest | requestId精确标识；inputDigest候选为经L0确认后原始UTF-8字节的SHA-256，不做隐式语义归一化。只改空格也需新requestId；未来语义digest要显式版本化。 |
| executionFingerprint | 基线完整UE版本/CL/Build标识、能力档、实际catalog/adapter指纹、输出允许根及目标路径；缓存/重复请求需匹配，不凭engine=5.3放行。 |
| sessionId / requestOutcome | 本次处理会话及上述请求枚举；重复请求指向previousReportRef，不能把旧阶段报告伪装成新编译。 |
| phase | received、validating、planned、building、compiling、publishing、saving、waitingPersistence、finished。当前操作位置，不代替结果；rejected/failed是结果而非阶段。 |
| overallStatus | inProgress、rejected、failed、cancelled、cleanupRequired、waitingPersistence、succeeded。另存失败原因，不新增含混的verificationFailed总状态；核验实际失败归failed，核验缺证据归waitingPersistence并列未验证项。 |
| checks | validation、build、initialReadback、compile、finalReadback、publish、save、persistence、runtime，每项status、reason、evidenceRef和本次/复用证据来源。 |
| check.status | notRun、pending、passed、failed、unverified、notApplicable。蓝图/材质必需生成门槛不能notApplicable；runtime依据声明的验收任务定义。 |
| assets / residue | 候选/最终路径、对象类别、逻辑映射、diskState(notWritten/written/partial/unknown)、会话所有权、清理状态。无证据不写Nothing changed，不自动删除他人资产。 |
| diagnostics / nextAction | 全量诊断及精确补救动作；错误不能因顶层包装而丢失。保留阶段失败和保存后的部分结果。 |

phase允许等待重开后进入finished；错误、拒绝或取消结束也进入finished，并保存lastCompletedPhase/failedStage。不能把phase=finished显示成成功。

## 整体状态推导

- validation失败且未构造候选：rejected / requestOutcome=notBuilt；同requestId不同digest或不可对应的执行记录：rejected / conflict，不创建或覆盖。
- 任一必需check实际failed：failed；若失败/取消后残留尚需人工清理或清理失败，用cleanupRequired，同时保留failure/cancellationReason，不能抹掉原错误。取消的干净会话为cancelled。
- 构造过程中有pending/notRun：inProgress，除已结束失败/拒绝/取消外。
- validation、build、initialReadback、compile、finalReadback、publish、save都passed，但persistence为notRun/pending/unverified：waitingPersistence；persistence实际失败则failed，并写已保存资产/残留。
- 上述八个必需check全部passed且没有待清理残留：succeeded。compile必须是真实完成/无错误及对应目标的证据，save仅接受真实返回/磁盘依据，persistence必须独立重开与最终结构/身份对照。
- runtime不自动由生成成功推导。生成会话可runtime=notRun；最小原型/发布验收若要求PIE、视觉、移除插件或打包，验收报告仍须这些项passed才算对应验收完成。不能以生成succeeded称整个产品验收完成。

候选严格规则可能需进一步实测修订，尤其independent reload/发布清理的可靠性；当前没有实现可保证这些约束。

## 重复请求规则

同requestId不同原始字节digest直接conflict。相同ID/digest先核对旧记录完整、相同fingerprint、明确目标、持久化证明及当前目标身份/语义/依赖；全部可证实才alreadyCompleted，跳过重建，checks明确标“复用哪份证据+本次核对什么”。

缺旧记录、目标丢失/改变、环境/目录变化或无法确认磁盘语义时不能alreadyCompleted，更不能自动覆盖；本轮结果rejected并说明unverified/conflict原因。由用户新的操作安排重新验证或使用新requestId，不能仅基于路径存在判断相同资产。

失败重试先核对residue与所有权；本候选不提供自动续写/原位修复。安全清理与重新生成策略要由故障测试冻结。alreadyCompleted仅是请求结果，只有可复用完整持久化证据和当前核对通过才允许overallStatus=succeeded；证明不足绝无绿色成功。

## 文档级判例（不是UE测试结果）

| 情形 | requestOutcome | 必需checks | overallStatus |
| --- | --- | --- | --- |
| 新候选建成但编译真实报错 | built | compile=failed；save/persistence=notRun；列残留 | failed或cleanupRequired |
| 新资产编译、回读、发布、保存均通过，尚未独立重开 | built | 前七项passed，persistence=notRun | waitingPersistence |
| 已保存后重开图结构不符 | built | persistence=failed，diskState=written | failed |
| 相同请求且旧完整证据、当前指纹/目标核对均吻合 | alreadyCompleted | 各项明确引用旧证据及本次核对，runtime另列 | succeeded |
| 相同requestId但空格/字段字节变化 | conflict | validation=failed，未新建 | rejected |
| 资源未取得或shader未结束 | built | compile=unverified或pending；不发布保存候选完成 | inProgress；若本次停止则failed并说明未验证/残留 |

这些是契约人工一致性检查，未运行解析器、Schema、编译或功能测试。将来错误数字表、报告Schema、真实输入输出与回归由双链路及负例验证后冻结。

## 2026-10-08 双链路实测增补

上述“未实现/未编译”和判例为静态研究时点，原研究文本未改写。本轮已按[实测报告](../Reports/20261008Prototype.md)验证保存后waitingPersistence、独立重开后succeeded、材质编译不完整拒绝和同名拒绝。Verify通过时为alreadyCompleted并标记verificationOnly，本次没有重新构造。早期built误用已在日志追加更正。

固定输入使用NodeForgePrototype v1，不冒称CoreCreateDraft1完整实现；错误细分/输入精确定位、全部phase、取消/故障/并发、源码二进制自动fingerprint和通用重复复用仍未完整实现。当前执行指纹使用能力标签及真实catalog摘要，构建DLL SHA另留证据；不能跨未知代码版本信任旧receipt。Catalog与CreateMap是辅助操作，不能套用资产生成八门槛。协议仍是候选，进一步冻结需补这些缺口与测试。

## 2026-10-10 夜间审计实际收据与输入边界

本节局部说明夜间实现，保留上方历史候选。对应[契约审查](../Reports/20261010NightAudit/ContractReview1.md)、[界面实现审查](../Reports/20261010NightAudit/UiImplementationReview2.md)及[本夜实测](../Reports/20261010NightAudit/Readme.md)，不得把源码完成或辅助操作成功当成资产验收完成。

- **按操作解释状态**：上方八门槛适用于 Import / Verify 的资产生成持久化。`GraphPreflight` 仅完成同一 v2 校验及计划摘要，其 `overallStatus=succeeded` 表示预检通过；build、compile、save、persistence 未执行，不能显示成资产已生成。`GraphRules` 用 `operationStatus` 表示规则查询，Catalog、MaterialFunction / MaterialParameters、图导出 / GraphPreview 与 CreateMap 按各自只读或辅助检查解释；缺少生成证据时不能套用绿色生成成功。运行和视觉仍须单独证据。
- **草稿与执行分开**：“载入 JSON 文件”仅替换输入草稿，不自动导入；取消、空/超限载入保留原稿。“粘贴剪贴板”按钮全文替换，Ctrl+V / 右键保持原生光标或选区编辑。编辑增加修订并使旧检查/预览失效，不能将旧结果归给新输入。
- **首次写入的来源证据**：当前面板执行使用 `inputMethod=draft`，`uiInput` 记录 manual / file / clipboard / export 来源、来源路径、是否已编辑、修订及 `snapshotPath` / `snapshotDigest`。完整 UTF-8 原文保存为新的不可变快照，后端在构造前核对实际摘要。来源路径仅是归属，不会在生成时偷偷重读可变原文件；这些是收据字段，不放入 Graph 请求根对象。
- **成功输入与新副本**：安全重排使用上次成功生成的输入快照和摘要，派生结果关联该成功输入，不能借当前无关草稿声明归属。已保存、失败、旧成功结果分别保留；同名资产/报告拒绝覆盖。完整错误保存到报告，面板仅显示有界摘要。
- **两种容量**：可导入输入 1–65536 字节；完整外部报告读取上限 1048576 字节。均先查大小再分配、拒绝无效数据，不静默截断。这是当前产品保护边界，不是 UE 图规模上限。
- **报告发布的已验证切片**：Build73 的 `FILEWRITE_NoReplaceExisting` 在本机真实并发测试未保证单写入者，失败证据保留。Build74 先用 UE 文件接口写完独有同目录临时文件，再以拒绝替换的原生改名发布；[Canonical74](../Reports/20261010NightAudit/Canonical74/Summary.json)实测 8 个线程只有 1 个成功，完整 JSON 有效，38 项检查及保护通过。这不是断电耐久性、全故障矩阵或多进程压力验收；完整取消、幂等恢复与并发资产构建未由此宣称实现。
