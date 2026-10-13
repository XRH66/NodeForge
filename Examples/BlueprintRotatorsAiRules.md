# 给外部AI的旋转成员规则

仅适用于UE5.3.2和精度修正Build44及以后明确兼容版本。Build43保留历史证据；根Build2、RC1 Build29不能使用此能力。协议仍是[GraphProtocol](../Contracts/GraphProtocol.md)中的NodeForgeGraph version2；准确节点、函数、引脚取自[真实目录](../Catalog/UE532BlueprintRotators.json)，摘要必须为`272ec558cb541bb1d69b4d998c4aced90cbcbe32ec42f3926a8005618eb52950`。

1. `settings.parentClass`为`/Script/Engine.Actor`。旋转成员形如`{"name":"Facing","type":"rotator","default":{"pitch":15,"yaw":40,"roll":-25}}`。角度为度，恰含三键、有限数字、abs<=1e12；不要传数组、字符串、布尔值或额外字段。
2. 变量节点使用真实`/Script/BlueprintGraph.K2Node_VariableGet`或`K2Node_VariableSet`，`properties.variable`是实际声明名。目录样本Value引脚须替换为声明名Facing，类型保持`/Script/CoreUObject.Rotator`。不要猜测或手写不存在的引脚。
3. `K2_SetActorRotation`使用CallFunction、owner=`/Script/Engine.Actor`、member=`K2_SetActorRotation`；NewRotation可接旋转变量/原生MakeRotator输出，或在未连接时写相同三键对象默认。不得同时默认赋值和接线。
4. 节点间执行顺序由edges指定；先写成员再读取应用，与先应用旧值再写成员有不同结果。nodes数组顺序无执行语义。明确声明BeginPlay、执行节点和每条边，导入器不补效果配方。
5. 仅现有自身Actor和已适配的根组件上下文，不因此开放任意Actor、组件创建、结构体、容器、拆分引脚、任意脚本或反射属性。
6. requestId与assetPath使用新值，目标保持`/Game/NodeForgeTests/`测试目录或协议允许路径；同名拒绝，不要求覆盖。

完整已绑定真实目录的数据入口为[RotatorRead](../Reports/20261008GraphV2/Rotators2/Inputs/RotatorRead.json)、[变量写回](../Reports/20261008GraphV2/Rotators2/Inputs/RotatorStore.json)、[直接默认](../Reports/20261008GraphV2/Rotators2/Inputs/RotatorDirect.json)。它们的目标已经在测试工程存在，再次导入需另存并更换requestId/assetPath。Tests目录中的CATALOG_DIGEST_FROM_REAL_ENGINE是测试模板占位符，不可作为正式输入。

目录采集的experimental只说明原生实例可构建；实际已验证范围见[功能说明](../Docs/BlueprintRotators.md)。本文件和本项目自写样例不冒充外部AI原始输出，新族真实界面闭环另待验收。
