# 原生材质函数的 AI 输入规则

适用于[函数真实目录](../Catalog/UE532MaterialFunctions.json)的32材质类/15蓝图签名及原生实例。构建至少Build14；根安装Build2尚未部署该能力，不能把新数据交给旧面板宣称支持。

NodeForgeGraph version2基本结构沿用[协议](../Contracts/GraphProtocol.md)，当前实际catalogDigest为`7d11f08760383d7b4704eab618fb098447f2558d9b456955139fbd818b7851e2`。节点class为`/Script/Engine.MaterialExpressionMaterialFunctionCall`；properties必须恰有MaterialFunction和FunctionAssetSha256。路径必须是实际存在、已保存、干净普通MaterialFunction的canonical object path，只接受/Engine或/Game/NodeForgeTests；不接受FunctionInstance或层/混合函数。

先用只读MaterialFunction操作检查具体资源，配置恰为`{"function":"完整对象路径"}`；读取返回functionSignature的properties、inputs、outputs，禁止猜引脚名或抄另一个版本的GUID。连接的input使用实际`name`，output使用实际十进制`index`。GUID用于回读身份核验，由原生节点保存，不作为替代引脚名的JSON连接字段。

节点类未绑定时目录中的空输入/输出只是未绑定状态，不是完整节点签名。FunctionAssetSha256来自实际函数文件，必须匹配当前文件，不能用目录摘要替代。哈希只锁定直接函数文件；嵌套函数/纹理暂不递归锁定，不能声称整个依赖闭包冻结。

函数调用节点不支持defaults自定义值。需要设置调用值时创建受支持的Constant/Parameter等节点并连接；必需输入不连会拒绝。可选输入未连接时沿用函数原生preview默认值，其值和使用规则记录在实际签名里，插件不会偷偷补常量节点。

本机SmoothStep实际输入为`Alpha (S)`、`Min (S)`、`Max (S)`，均原生可选；输出`0`/Result。Overlay实际输入为`Base (V3)`和`Blend (V3)`，两者必接；输出`0`/Result。来源：[SmoothStep实际签名](../Reports/20261009UsableEditor/FunctionInspect3/SmoothStep.json)、[Overlay实际签名](../Reports/20261009UsableEditor/FunctionInspect3/Overlay.json)。其它函数必须另行实际读取和验证，不能把两项通过推广到所有引擎函数。

样例在[函数组合用例](../Tests/MaterialFunctions/)，未绑定目录的能力发现不是编译通过。各测试结果由[当前实测报告](../Reports/20261009UsableEditor.md)维护；外部AI原始输出真实UI导入仍待完成，不把这里的项目自写样例当外部AI证据。

## 选中函数导出（Build18）

Build18面板“读取选中材质 / 函数”沿用同一个原生MaterialFunction后端；操作与证据见[资源选择说明](../Docs/ResourceSelection.md)。成功报告新增 `nodeTemplate`：实际MaterialExpressionMaterialFunctionCall类及同一函数路径/SHA，id可在完整图内改成唯一逻辑ID；这是单节点片段，不是完整导入JSON。不复制未绑定目录的空端口，也不把输出名当规范十进制索引。

当前新构建目录以[Catalog18原生输出](../Reports/20261009UsableEditor/ResourceSelection1/Catalog18.json)为准，摘要 `a6488c77da856b78bd72ac32d5c480008f34cdb897524c2296289f088893b786`；上文7d11是保留的Build14兼容摘要，新输入优先使用实际已加载构建的当前摘要。新签名见[SmoothStep](../Reports/20261009UsableEditor/ResourceSelection1/Smooth.json)及[Overlay](../Reports/20261009UsableEditor/ResourceSelection1/Overlay.json)，报告包含实际DLL指纹。根安装2仍未部署此入口，人工点击/剪贴板及外部AI原文闭环不得据后台测试宣称完成。

后续Build19已实际导出[当前目录](../Reports/20261009UsableEditor/ResourceSelection2/ActualCatalog.json)，摘要相同；[当前SmoothStep](../Reports/20261009UsableEditor/ResourceSelection2/Smooth.json)和[当前Overlay](../Reports/20261009UsableEditor/ResourceSelection2/Overlay.json)各记录实际19 DLL。任务规则子集是NodeForgeTaskRules，详[查询说明](../Docs/ResourceSelection.md#按任务检索规则build19)，不是可直接导入的资产JSON；不得用子集摘要替代完整catalogDigest。Build18两函数图构图/保存/重开仍只关联18收据，19只读检查不自动把它升级。
