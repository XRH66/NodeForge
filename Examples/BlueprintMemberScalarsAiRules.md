# 整数/名称成员：给外部 AI 的规则

目标Windows、官方UE5.3.2，使用Build40隔离插件及[实际目录](../Catalog/UE532BlueprintMemberScalars.json)，摘要 `cd5de7be8e3662c0ca769a729f64b844b359a25bc46d39a26c2d528bef0bb9e1`。当前实测状态见[功能说明](../Docs/BlueprintMemberScalars.md)，冻结RC1和根插件尚未升级。

继承[根组件规则](BlueprintComponentsAiRules.md)和[Graph v2协议](../Contracts/GraphProtocol.md)的结构/路径/引脚/保护约束；本页替代旧摘要与“int/name不能声明为成员”的旧限制。format=`NodeForgeGraph`、version=2，九个根字段不变；不要引入任意脚本、未知属性或生产资产路径。

settings.parentClass仍为`/Script/Engine.Actor`，variables每项恰为name/type/default。现可用bool/float/string/vector/int/name，声明名称保持原有ASCII字母开头、无重复或继承成员冲突规则。

- int默认值为整数JSON数字[-2147483648,2147483647]，不是字符串，也不是bool。不支持int64或小数。编译后的CDO原生int32须精确一致。
- name默认值为原生FName显示拼写的非空字符串，UTF-16长度最多1023；空身份写`None`，不要写空串或依赖自动大小写归一化。此约束针对成员声明，输入引脚仍按实际签名约束。运行时名称比较遵循UE原生FName语义。
- 读写节点使用`/Script/BlueprintGraph.K2Node_VariableGet`或`K2Node_VariableSet`，properties恰为`{"variable":"实际已声明的名称"}`。目录示例中的Value数据引脚需替换为此成员名；执行引脚为execute/then，Set可用输出为Output_Get。隐藏self不填写、不接线。
- 先连写入执行流，再在后续节点读取变量。纯变量getter在被消费时取值，不以nodes位置决定顺序。声明默认值保存在资产CDO，运行时实例赋值不会改写资产初值。
- 名称需要参与字符串函数时使用真实`Conv_NameToString`节点；int需要其它数值类型时使用支持的显式转换，不靠自动改线。

样例：[计数写回后分支](../Tests/BlueprintMemberScalars/MemberCount.json)、[名称状态更新与文本选择](../Tests/BlueprintMemberScalars/MemberState.json)、[直接读取None身份](../Tests/BlueprintMemberScalars/MemberNone.json)、[整数两端默认值](../Tests/BlueprintMemberScalars/MemberLimits.json)。使用新requestId/assetPath避免碰撞；它们是本地测试数据，不是外部AI原文。

旧7dea/699及更早摘要只能表达各自旧范围，不能用于新成员类型。容器、枚举、int64、对象及rotator成员仍需适配。未找到精确类型或引脚时报告缺口，不省略节点。目录可构建属于实验支持；编译重开、运行、真实UI和交付分别看证据，不因发现条目而声称全配置通过。
