# 可交给任意外部AI的提示词

2026-10-10说明：以下保留首次v1两样例闭环的历史提示词，不是本夜扩大/修改功能的指令。新构图任务请使用[Graph v2规则](GraphAiRules.md)和匹配实际DLL的目录；原始AI输出与后续修订须分开保存。

请依据随附真实目录和两样例实际规则生成两个全新的JSON文件内容。只生成数据，不给代码/脚本，不调用工具或第三方服务。一个文件一个对象，不添加说明字段。先自检字段、类型、节点和边的顺序、唯一名称及范围。

蓝图文件：requestId=`ExternalAiBlueprintRun1`、assetPath=`/Game/NodeForgeTests/NFExternalAiBlueprint`，其余完全遵守BlueprintTemplate和规则。此最小版本必须保留固定打印字符串及false/true设置。

材质文件：requestId=`ExternalAiMaterialRun1`、assetPath=`/Game/NodeForgeTests/NFExternalAiMaterial`，First.value=0.75、Second.value=0.5，Multiply连EmissiveColor，Surface/Opaque/Unlit，严格遵守MaterialTemplate/规则。预期线性输出0.375。

请分别以标题Blueprint.json和Material.json的代码块输出，不能合并为顶层数组。规则包不包含凭据、正式工程或用户私人资料。使用[实际规则](MinimalAiBundleRules.md)和[目录](MinimalAiBundle/RealCatalog.json)；若你无法读取附件，明确指出缺材料，不凭记忆猜节点。
