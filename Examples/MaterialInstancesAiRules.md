# 普通材质实例的AI输入规则

版本补充：本文保留Build6实例时点。新Build11已增加原生采样校验，Build14目录为[函数/实例真实目录](../Catalog/UE532MaterialFunctions.json)，digest `7d11f08760383d7b4704eab618fb098447f2558d9b456955139fbd818b7851e2`；新实例请读取该目录和父参数再使用，本文原1b12摘要不再作为新实例有效摘要。根安装仍Build2，新构建尚未部署。三override maps结构不变，纹理必须匹配每个实际原生采样器，颜色/法线跨类型拒绝。


Windows/UE5.3.2，配合[Build6真实目录](../Catalog/UE532MaterialInstances.json)。根安装和已打开用户Editor仍Build2，当前实例能力仅独立构建可用；[实测状态](../Reports/20261009UsableEditor.md)不能由源码或规则替代。

把本文件、真实目录和调用方提供的 `MaterialParameters.parameters` 报告交给AI。该报告从已保存父材质提取精确路径、父文件SHA及类型化真实参数。AI只输出一个UTF-8 JSON对象，不运行脚本、不猜名称和资源、不改父图。通用格式、字节上限、资产保护沿用[GraphProtocol](../Contracts/GraphProtocol.md)。

根字段恰为format/version/requestId/assetType/assetPath/catalogDigest/nodes/edges/settings。format是NodeForgeGraph、version是2、assetType是MaterialInstance，nodes/edges均为空数组；requestId及目标使用新的ASCII字母数字名称，目标 `/Game/NodeForgeTests/` 下直接接名称。当前目录摘要：

```text
1b12e94c89af0c2e18d3dbbf362222754473f341d3a353eff24ca6abc8809dc1
```

settings恰有parent、parentAssetSha256、scalarParameters、vectorParameters、textureParameters。复制只读报告的真实父路径/SHA；用该报告确认可覆盖名称与类型。标量为数字，向量必须四数线性RGBA数组，贴图为调用方确认存在的普通非虚拟UTexture2D精确对象路径。所有数值有限且±1000000内，不支持用字符串表示数值。最多128个override；三映射不能为空值或省略，无override用 `{}`。省略参数继承原生父默认，不隐式填全目录默认值。

父须普通已保存、干净Surface Material，不支持实例父级/层/静态开关；仅使用 `/Engine/` 或 `/Game/NodeForgeTests/` 实际资源。不加入材质模式、节点/连接、position、脚本或其它settings；实例复用父模式与图，不声称可单独修改这些内容。父SHA变化时先重新检查，保留原AI输出与后续修订，不擅自替换摘要假装一次成功。

可核对样例在[实例测试清单](../Tests/MaterialInstances/Cases.json)。这些原始模板含父SHA占位，**不能直接导入**；矩阵实际执行Inputs依据当前工程原生参数报告绑定实测SHA，InputTemplates/InputBinding保留来源与修订。实际可导入示例应使用报告目录的最终Inputs和该同工程父资产。外部AI原始输出真实UI闭环尚未完成，不能把本项目生成的测试数据称为外部AI输出。
