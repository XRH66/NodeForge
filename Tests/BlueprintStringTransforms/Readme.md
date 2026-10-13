# 字符串派生与分支样例

2026-10-10导航更正：本页准备/暂停及Build58补记保留历史。当前任务以[ProjectStatus](../../ProjectStatus.md)与[最终报告](../../Reports/PluginControlStageTwo/EditorControl1/S5/FinalReport.md)为准；原[夜间整体审计](../../Docs/NightAudit20261010/Readme.md)已收尾。不从下方旧paused描述推断原生Goal现状。后续运行/导出是否通过必须读取对应真实报告，不凭本次文档更正补写结果。

**2026-10-09 R1实际构图补记**：[Transforms58R1](../../Reports/20261008GraphV2/Transforms58R1/Summary.json)在冻结58 DLL上17例/27步骤全部通过，包括10正例构图编译保存独立重开和7项保护负例。下面准备/暂停段落保留原时点；逐字运行与代表导出仍待本轮后续回执，未提前写通过。详[R1日志](../../WorkLogs/2026-10-09-R1阶段验收.md)。

本目录供既有[RunGraphRegression.py](../../Scripts/RunGraphRegression.py)读取`Cases.json`，并供[RunBlueprintMotionChecks.py](../../Scripts/RunBlueprintMotionChecks.py)读取`RuntimeCases.json`。十项正例、七项负例及十项逐字运行消息均是验收期望；准备阶段没有运行UE、构图、编译、保存、重开、PIE或导出，实际验收为`notRun`。

## 来源与端口状态

图结构模板：[StringCompose.json](../BlueprintStringIdentity/StringCompose.json)、[BlueprintBranch.json](../GraphV2/BlueprintBranch.json)。既有PrintString、Concat_StrStr、Len、Contains、GreaterEqual_IntInt、BeginPlay和Branch端口来自[Build55实际目录](../../Reports/20261009UsableEditor/StringIdentityExpansion1/Capture55/ActualCatalog.json)，engine为`5.3.2-29314046+++UE5+Release-5.3`，摘要为`af8a95377783d90668b062fef423717e1eec33b7a23139aed5de4d3206bc15aa`，文件SHA256为`d22466b534130d95687d02770a568a1c511782e2c6567c7b1babf69a18566f4c`。Len、Contains、Mid已有支持，本轮不算新增。

Left/Right/ToUpper/ToLower当前仅按本机`X:/UnrealEngine/UE_5.3/Engine/Source/Runtime/Engine/Classes/Kismet/KismetStringLibrary.h`声明准备候选：字符串输入名`SourceString`，Left/Right另有`int32 Count`，返回为FString。新函数的`ReturnValue`及默认值可写状态尚须root采集的原生Build56目录核对；头文件发现不能标为真实端口、可构造或运行通过。准备时新函数目录预检为`notRun`，既有节点子图检查不覆盖连接新函数的边。

正常模板摘要为`CATALOG_DIGEST_FROM_REAL_ENGINE`，由运行器替换为最终实际目录摘要。TransformOld55Reject固定`af8a95377783d90668b062fef423717e1eec33b7a23139aed5de4d3206bc15aa`；TransformOld52Reject固定[Build52目录](../../Reports/20261009UsableEditor/SwitchExpansion1/Capture52/ActualCatalog.json)摘要`1ae2791da31e336b3d147d1ed20e173ddfe51e721d255df040c843c743171317`。二者均有`preserveCatalogDigest:true`，要求新函数被旧摘要拒绝。

准备器：[PrepareFixtures.py](../../Reports/20261009UsableEditor/StringTransformExpansion1/PrepareFixtures.py)。仅使用标准库和现有运行器的纯`catalog_signature_errors`，不导入会在顶层执行的运行脚本。默认拒绝任何已有输出；`--check-only`只检查已有模板。root提供目录后使用`--catalog Reports/20261009UsableEditor/StringTransformExpansion1/Capture56/ActualCatalog.json --check-only`完整核对十项正例的真实端口（路径须以root实际捕获位置为准），结果打印到标准输出，不自行写新报告。

## 数据与逐字期望

| 正例 | 内容 | expectedMessage |
|---|---|---|
| TransformCompose | AbCdEfG左3转大写，右3转小写，再用Concat组合 | `ABC/efg` |
| TransformComposeReordered | 节点和边乱序；qRsTuVw左2、右4、分隔符冒号 | `QR:tuvw` |
| TransformUpperOnly | 删减为单ToUpper后直接PrintString | `MIXED/ABC` |
| TransformLowerOnly | 删减为单ToLower后直接PrintString | `mixed/abc` |
| TransformContainsSensitive | AbC-Tail先转小写，Contains查ABC，bUseCase=true，走else | `Contains Miss` |
| TransformContainsInsensitive | 同来源，bUseCase=false且bSearchFromEnd=true，走then | `Contains Match` |
| TransformLengthEnough | Left Count=4→Len→GreaterEqual_IntInt B=4，走then | `Length Enough` |
| TransformLengthShort | Left Count=2→Len→GreaterEqual_IntInt B=3，走else | `Length Short` |
| TransformZeroSlice | Left和Right均Count=0，再转大小写并组合分隔符 | `/` |
| TransformLongSlice | AbC经Left Count=99、Right Count=2147483647再ToLower，原生截取 | `abc` |

所有图的Actor位置/旋转期望为`[0,0,0]`，缩放为`[1,1,1]`。分支两出口均接不同消息，必须由实际计算决定只输出一条；不会只把expected标签当成通过。消息区分大小写，未进行Python大小写折叠或模糊匹配。

负例为旧55摘要ToUpper、旧52摘要Right、member写为`toUpper`、默认值端口写为`sourceString`、Count为`1.5`、函数properties多出`unexpected`及同名目标保护。TransformCollision使用新的requestId指向TransformCompose目标，并要求先成功运行TransformCompose；拒绝后既有资产应保持不变。

本目录不修改源插件、目录报告、旧模板、管理文档或任何UE工程。实际Build56构图、编译、保存重开、运行、导出及拒绝守卫由root统一调度，准备器的静态检查不代替上述验收。

## 2026-10-09 历史补记：Build58暂停检查点

上文与Cases.json中的Build56/pending描述属于原始数据准备时点，保留不改。后续实际Build58编译、四函数实例签名与完整静态预检已通过，见[Capture58](../../Reports/20261009UsableEditor/StringTransformExpansion1/Capture58/Summary.json)和[当次预检回执补记](../../Reports/20261009UsableEditor/StringTransformExpansion1/Capture58/FixturePreflight.json)。实际17例构图/保存重开、10图逐字运行/导出尚未启动，不能用静态检查提升为通过。

该历史时点用户要求暂停并先规划方向，Goal当时实际paused；[收尾](../../Reports/20261009UsableEditor/PanelRefresh1/Readme.md)及[路线建议](../../Docs/Roadmap.md)是当时入口。此段不覆盖随后R1完成及其它后续阶段的终态；接手须读取当前状态和实际Goal。原10正/7负模板及逐字期望保持，不改断言制造成功。
