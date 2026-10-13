# 材质族扩展验证输入

输入遵守NodeForgeGraph v2，来自本机UE5.3.2日常阶段Build4/Catalog4的实际签名。不是第三方fixture标识。根安装仍Build2，测试使用独立PluginStage，见[当前状态](../../ProjectStatus.md)。

[Cases.json](Cases.json)定义9正例、6负例。TextureParameter、Tint、Shuffle核对纹理参数、节点增加/连线变化/参数变化及顺序无关；FractionMath覆盖Frac/Floor/Saturate；VectorProducts覆盖CrossProduct/DotProduct；FacingColor与FacingDefaults覆盖Fresnel及其真实输入默认映射；WaterFacing是原水面数据增加视角颜色插值，仍通过同一个构图器，无水面代码分支；ArchivedColor用归档digest核对旧组合兼容。

负例检查缺Texture/ParameterName、未开放属性、不存在的参数节点输入、新类使用旧摘要、未知摘要。preserveCatalogDigest只在显式兼容/摘要测试保留fixture的原值，其它案例注入指定实际目录摘要；保存的执行Inputs及源码fixture分别记录哈希。

历史执行模板：RunGraphRegression.py --run MaterialFamilies1 --catalog Reports/20261009UsableEditor/Catalog4.json --fixtures Tests/MaterialFamilies --plugin-directory Reports/20261009UsableEditor/Build4/PluginStage/NodeForge。生成、编译、保存及独立重开结果在Reports/20261008GraphV2/MaterialFamilies1，不据本清单存在写通过。视觉另由RunGraphSceneChecks.py --visual-only在新图验证；旧33和v1负例的历史独立批次不混作本矩阵覆盖。

新验证应使用未占用的run名称、明确的当前stage及由该DLL实际导出的目录；按修改影响选择本族正负例及回归，不重复启动MaterialFamilies1，也不为普通目录/文档变化机械重跑全部旧矩阵。夜间历史范围见[夜间任务](../../Docs/NightAudit20261010/ExecutionPlan.md)和[夜间报告](../../Reports/20261010NightAudit/Readme.md)。 当前项目状态见[ProjectStatus](../../ProjectStatus.md)与[第二阶段最终报告](../../Reports/PluginControlStageTwo/EditorControl1/S5/FinalReport.md)；本页原构建/夜间证据仍限定于各自时点，不因此继承新版全部验收。
