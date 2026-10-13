# 普通材质实例与资源依赖

当前v3结构见[GraphAuthoring](GraphAuthoring.md)，三族完整v3输入见[AiJsonArchitectureExamples](../Ai/JsonArchitectureExamples.md)。下方settings/nodes/edges和绑定properties是仍使用的v2详细表示；v3在asset/graphs/binding中明确分区，解码到同一生产描述。


当前v3结构见[GraphAuthoring](GraphAuthoring.md)，三族完整v3输入见[AiJsonArchitectureExamples](../../../Build134/Mcp7R5/Ai/JsonArchitectureExamples.md)。下方settings/nodes/edges和绑定properties是仍使用的v2详细表示；v3在asset/graphs/binding中明确分区，解码到同一生产描述。


当前v3结构见[GraphAuthoring](GraphAuthoring.md)，三族完整v3输入见[AiJsonArchitectureExamples](../../../Build133/Mcp6R4/Ai/JsonArchitectureExamples.md)。下方settings/nodes/edges和绑定properties是仍使用的v2详细表示；v3在asset/graphs/binding中明确分区，解码到同一生产描述。


assetType=MaterialInstance，新普通 MaterialInstanceConstant；nodes/edges必须空，settings恰为parent、parentAssetSha256、scalarParameters、vectorParameters、textureParameters。父是已保存、干净普通 Surface Material，非实例父、layer或函数实例。实际父精确路径/直接磁盘SHA保护；生成不修改父。

参数必须来自该父实际 MaterialParameters（检查选中资源UI，或正式 resources.inspect(kind=MaterialParameters)读取普通父材质），只覆盖 global scalar/vector/nonvirtual Texture2D；未知名称/类型、静态switch、层参数、虚拟纹理、不匹配sampler拒绝。override maps 与真实值、父哈希、shader继承、保存/独立重开分别回读，UE setter bool 不能充当本机成功判据。

通用 graphs.preflight/generate 从共享 Execute 管线静态可达实例，没有实例对象专用query；普通父材质参数可由正式MCP resources.inspect读取；rules.query只两族。已有 UI/后台实例样例和历史验收保留，本期 P3 只材质/蓝图，实例 MCP 实测 notRun。不要为了本期完整文档新增实例测试批次。

目录内 materialInstance 原文与 [实例示例](Examples.md)说明配置；操作者准备/注册工程及真实普通父材质后，AI可通过正式资源读取工具取得父参数/sha，缺依赖时停止，不能虚构参数名或把样例父路径当已存在资源。


当前实例目录：

```json
{
  "assetType": "MaterialInstance",
  "nativeClass": "/Script/Engine.MaterialInstanceConstant",
  "adapter": "NodeForgeMaterialInstance1",
  "scope": "new ordinary constant instance; saved clean Surface Material parent; native global scalar/vector/Texture2D overrides only",
  "schema": "nodes/edges empty; settings exactly parent,parentAssetSha256,scalarParameters,vectorParameters,textureParameters",
  "discovery": "MaterialParameters inspects actual parent parameter names/types/defaults and saved bytes before import",
  "verification": "native setters' bool is not success evidence in UE5.3.2; independently verify exact override arrays, effective values, shader inheritance, save/reopen",
  "textureSamplingPolicy": "ordinary nonvirtual Texture2D; native recursive parameter expressions and VerifySamplerType for every matching sampler; missing signature or incompatible normal/color/sRGB rejected before creation"
}
```
