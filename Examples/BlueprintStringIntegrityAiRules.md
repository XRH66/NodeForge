# 外部AI蓝图文字与精确身份规则

沿用[v2协议](../Contracts/GraphProtocol.md)和此前[Switch规则](BlueprintSwitchesAiRules.md)、[数学规则](BlueprintMathInterpolationAiRules.md)。可用节点/函数签名从当前真实导出获得，[Build55目录](../Catalog/UE532BlueprintStringIdentity55.json)供本地核对，不使用研究fixture标识。

- 当前catalogDigest为`af8a95377783d90668b062fef423717e1eec33b7a23139aed5de4d3206bc15aa`。旧52/45数据允许各自原能力，不静默升级摘要。
- class/parentClass/owner/member/type/变量引用和引脚逐字使用真实目录拼写；例如`PrintString`不能写`printString`，`InString`不能写`inString`。JSON节点/属性/连接字段键同样使用协议规范拼写。
- nodes数组的id与edges的from/to逐字相同，禁止只有大小写不同的重复身份。settings.variables声明的Message不能被变量节点引用为message。
- string默认保存字面内容，AbC与abc不同。name成员空身份只写`None`；none/NONE和空字符串拒绝。不要把原生FName运行匹配忽略大小写误解为协议标识可任意拼写。
- 本轮不开放自定义变量FriendlyName、元数据、任意对象/容器默认；未支持配置必须明确报错，不能靠省略字段生成近似图。
- 保持同名拒绝，输出新的requestId与独立assetPath。外部AI原文和人工修订须分别保留，不能声明本切片已经通过真实UI/外部AI闭环。

已准备的输入和消息期望见[样例](../Tests/BlueprintStringIdentity/Readme.md)，实际层级和未测限制见[能力](../Docs/BlueprintStringIntegrity.md)。
