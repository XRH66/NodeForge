> 当前接入身份：Build137 Repair5 / Ai1 / Mcp10R1；Windows x64官方UE5.3.2。正文功能说明继承Mcp9R19；Build136、原stage/PID/验收路径只表示历史证据。当前安装和连接统一见[AI安装](../User/AiInstallation.md)。不要复制历史本机路径或环境配置。

# 已有功能与入口矩阵

本表说明当前声明的业务和开放边界。基础包P3固定两图和新PID Verify属于其原版本历史证据；本版是否复用须有影响Review，发布器不建立UE验收结论。目录不是全UE准入。

| 功能 | UI | 正式入口/MCP | 保护与限制 |
| --- | --- | --- | --- |
| 发现操作、状态、草稿、调用 | UI 状态 | operations.describe/editor.getState/draft.get/calls.get | 无资产写入 |
| 载入、替换、恢复草稿 | 载入文件/粘贴/恢复 | draft.loadFile/replace/restore | 修改草稿、修订；不自动生成；本版成对可选导出来源绑定，未知/冲突停止依赖写入；本期新增显式retain来源编辑，新增部分C3两族正式链通过 |
| 规则与目录查询 | 两族查询/目录 | rules.query/catalog.get；CLI getGraphCatalog | 报告只读；Material/Blueprint query，不含实例参数查询 |
| 预检与生成 Material/Actor Blueprint | 检查/生成新资产 | graphs.preflight/generate；CLI preflightGraph/generateGraph | 新资产 create-only；其它组合/运行效果分别未验 |
| 普通 MaterialInstanceConstant | Graph 输入生成 | 通用 graphs 管线接受共同描述，输入v2/v3；无独立实例查询工具 | 新实例；先取得已保存干净父材质参数/直接文件 SHA |
| 参数与材质函数签名读取 | 检查一个选中资源 | resources.inspect；MaterialParameters/MaterialFunction | 准确源对象/允许根/saved/clean/受控加载证明；函数模板是单节点片段，直接文件SHA |
| 普通材质/Actor 图导出 | 导出选中图到草稿 | graphs.export；MaterialGraphExport/BlueprintGraphExport | 准确源、新目标/request、完整结构；API纯读，显式载入原text及成对报告来源，预检生成再核源 |
| 准备新副本 | 准备新副本/恢复 | draft.prepareCopy；共享PrepareSnapshot→PrepareNewCopy | 只更改两身份；不生成/预留；保留父目录 |
| 差异预览 | 预览新副本差异 | graphs.preview | 精确结构/布局；不改稿/资产，不证明运行画面 |
| 布局与重排副本 | 生成时布局；按上次成功输入重排新副本 | 共享布局；内部 RelayoutCopy | 显式锚点保持；尺寸 estimate；重排不含后来手工修改 |
| 独立保存核验 | 结果显示待独立重开 | 原正式 CLI verifySaved | 原输入/收据/SHA/同 DLL，新 PID；不重 Import |
| 定位、打开资产/完整报告、复制 | 结果按钮及剪贴板 | UI 服务/有界结果；MCP report_read 是受控报告读 | 原报告≤1MiB；普通MCP子树≤32KiB，/exportedRequest专用完整Graph≤64KiB；UI视图不提升验收层 |
| 版本资料、来源和许可 | 本地资料 | MCP docs_index/docs_read | manifest 指定 SHA/bytes/pageId，有界非秘密内容 |
| 运行、画面、地图/布局探针、打包试用 | 历史辅助/试用 | 内部测试模式/原报告 | 生成/Verify 不继承这些结论；RC1 冻结 |
| 显式保留来源编辑与正式准备副本 | 编辑输入/准备新副本 | draft.replace(retainExportOrigin=true)/draft.prepareCopy | 完整Graph v2/v3两族；核源和真实提交，不预约不生成 |
| 普通Actor ReceiveTick与持续样例 | 同一受控图输入与原生成按钮 | 原catalog/preflight/generate/export工具；没有逐事件或公开PIE工具 | 每个精确事件最多1个；有限X轴100厘米/秒样例的实际Delta/三时刻/两区间证据见AiBlueprintTick；其它组合仍experimental |
| Surface材质细节与七类变量编辑属性 | 材质/变量原生Details；同一输入与原生成按钮 | 原catalog、preflight、generate、export、prepare/preview工具；19工具保持 | Masked/OpacityMask、四项材质设置、instanceEditable/category/tooltip；字段条件/完整v2/v3与有限实证见AiAssetSettings；画面/TAA/LOD/打包未重测 |
| Surface材质细节与七类变量编辑属性 | 材质/变量原生Details；同一输入与原生成按钮 | 原catalog、preflight、generate、export、prepare/preview工具；工具数以本包契约为准 | Masked/OpacityMask、四项材质设置、instanceEditable/category/tooltip；字段条件与完整v2/v3见AiAssetSettings；当前候选实际画面/Win64有限范围见VersionReleaseNotes，TAA/LOD仍未重测 |
| 有界资源发现、精确检查与引用 | 原资源读取与图输入区域；无新增浏览器面板 | resources.query / resources.inspect / 原graphs入口 | Texture2D、MaterialFunction、Material元数据列表；发现/政策/检查/图导出分开，实际来源准入与恢复见AiResources |


当前71材质类/327蓝图签名；普通实例沿共同描述由原生核心构造，当前实际证据层见本包发行说明；没有实例专用MCP或实例图导出。规则查询仅Material/Blueprint。resources.inspect/graphs.export已正式投影，准确对象/完整payload/来源绑定见[AiAssetRead](../Ai/AssetRead.md)。draft.prepareCopy/graphs.preview沿共享Service正式投影，显式retainExportOrigin编辑和真实准备身份/完整差异见[AiCopyPreview](../Ai/CopyPreview.md)；准确验证层以本版发行说明为准。重排和纯视图仍无独立MCP。受支持种类/复杂度见[Material](Material.md)、[Blueprint](Blueprint.md)、[实例](MaterialInstance.md)。
