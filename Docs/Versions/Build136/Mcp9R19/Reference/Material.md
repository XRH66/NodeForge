# 普通 Material：71 类与组合边界

公开副本中的请求、目标、进程、会话及来源报告身份为虚构示例；历史产品结论保持，原始证明由维护者内部保留。实际调用须使用本次分配和返回的完整原 text/sourceRef，示例身份与零摘要不能作为来源证明。

当前v3结构见[GraphAuthoring](GraphAuthoring.md)，三族完整v3输入见[AiJsonArchitectureExamples](../Ai/JsonArchitectureExamples.md)。下方settings/nodes/edges和绑定properties是仍使用的v2详细表示；v3在asset/graphs/binding中明确分区，解码到同一生产描述。


当前v3结构见[GraphAuthoring](GraphAuthoring.md)，三族完整v3输入见[当前完整JSON例子；历史原件内部保留](../Ai/JsonArchitectureExamples.md)。下方settings/nodes/edges和绑定properties是仍使用的v2详细表示；v3在asset/graphs/binding中明确分区，解码到同一生产描述。


当前v3结构见[GraphAuthoring](GraphAuthoring.md)，三族完整v3输入见[当前完整JSON例子；历史原件内部保留](../Ai/JsonArchitectureExamples.md)。下方settings/nodes/edges和绑定properties是仍使用的v2详细表示；v3在asset/graphs/binding中明确分区，解码到同一生产描述。


当前v3结构见[GraphAuthoring](GraphAuthoring.md)，三族完整v3输入见[当前完整JSON例子；历史原件内部保留](../Ai/JsonArchitectureExamples.md)。下方settings/nodes/edges和绑定properties是仍使用的v2详细表示；v3在asset/graphs/binding中明确分区，解码到同一生产描述。


71 类，全部 experimental/discovered/constructible 目录实例；不代表71类每种配置都编译。最多256节点/1024边、至少1节点/1输出边；是输入上限，未作为最大复杂图性能实测。所有表达式必须到达有效 Material 输出；孤立/无输出支链明确拒绝，不删除或补连线。

Surface、Opaque/Translucent/Masked、Unlit/DefaultLit、twoSided boolean；Translucent 需显式 SurfaceForwardShading。输出仅当前模式实际 active 的 BaseColor/EmissiveColor/Metallic/Normal/Opacity/OpacityMask/Roughness/Specular/WorldPositionOffset。Unlit 通常写 EmissiveColor；Opaque 不接 Opacity。真实编译继续判断维度、采样器和着色阶段。

output 是规范数字字符串索引，例如 "0"，不能 RGB/00/number。input 等原生实际 GetInputName。properties 仅目录允许字段；省略用记录的原生默认，不插入常量。defaults 只映射唯一原生 numeric OverridingInputProperty，无唯一映射拒绝。参数名必须独立，当前不允许同名共享。

贴图引用仅已存 `/Engine/` 或测试根精确普通 UTexture2D、非虚拟；不导入新资源。缺省 sampler 原生推断并回读，显式不匹配拒绝。TextureObjectParameter 继承的 UV/mip 输入并不被消费，不能照继承显示误连接。

MaterialFunctionCall 要绑定真实已保存干净普通函数，实际 direct file SHA、输入/输出 GUID/signature、原生必需性；未绑定目录空签名不是规则。资源检查UI及正式 nodeforge_resources_inspect(kind=MaterialFunction, source=准确路径)取得真实签名/模板/源SHA；没有专用函数工具，函数本体创建/图导出、函数默认值/实例/层未适配。任意 Custom HLSL、Material Attributes、未知表达式/函数、任意对象导入都未开放。

Spatial world/object origins 使用精确原生枚举，方向/法线为世界空间，不自动转切线。PixelNormalWS 对 Normal/WPO 不适用；具体着色阶段由实际 shader 判定。颜色/距离/UV等族在同一适配器中组合，无水面专用生成器。

读 [71节点索引](MaterialNodeIndex.md)、对应精确属性/引脚页和历史 EvidenceIndex。该页不继承历史画面验证到新材质，也不保证任意大图/数值范围正确。


本包新增 Masked/OpacityMask 和四项材质细节；Masked 显式提供 opacityMaskClipValue，allowNegativeEmissiveColor=true 仅适用于 Unlit。省略、inactive 存储、原生回读及完整 v2/v3 输入见[当前资产设置说明；历史原件内部保留](../Ai/AssetSettings.md)。


本包新增 Masked/OpacityMask 和四项材质细节；Masked 显式提供 opacityMaskClipValue，allowNegativeEmissiveColor=true 仅适用于 Unlit。省略、inactive 存储、原生回读及完整 v2/v3 输入见[统一资产设置](../Ai/AssetSettings.md)。
