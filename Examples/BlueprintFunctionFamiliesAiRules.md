# 普通蓝图函数族：给外部AI的规则

目标为Windows、官方UE5.3.2，使用本切片隔离构建及[实际目录](../Catalog/UE532BlueprintFamilies.json)。冻结RC1及旧根DLL不支持这些增量，不在运行中替换DLL。目录原生实例可构建属于实验支持，具体通过范围见[逐项证据](../Catalog/UE532BlueprintFamiliesVerification.json)；未验证项可在独立工程试验，不代表生产可用。

目录摘要必须为`6993743a0513489069423790f678c2cccd6bccc29435cd0dd98e4156248aa069`。从catalog.blueprint.nodes匹配实际class、properties.owner/member及引脚签名，禁止用显示名、中文名、fixture占位标识或猜测的函数替代。旧a648目录不包含新函数/int/name默认值权限。发现、构建、编译重开、运行是不同事实。

输出一个完整JSON对象，九个根字段为format/version/requestId/assetType/assetPath/catalogDigest/nodes/edges/settings。format=`NodeForgeGraph`、version=2、assetType=`Blueprint`；requestId和`/Game/NodeForgeTests/`下直接资产名仅ASCII字母数字、1–64字符，用新名且拒绝覆盖。UTF-8、无重复键/注释，输入最多65536字节。nodes最多128、edges最多256、成员声明最多64。

节点必有id/class/properties，可选defaults/position；逻辑id仅ASCII字母数字、1–64字符且唯一。连接恰为from/output/to/input，按真实引脚身份逐条连接。允许乱序，不插默认图、不丢节点、不自动改引脚、不添加隐式类型转换。连接前后由目录约束与原生K2 schema检查；同一输入不能双源，也不能既连接又写默认值。

settings恰为parentClass=`/Script/Engine.Actor`和variables数组。成员变量仍限bool/float/string/vector，float为原生double，vector为[x,y,z]。本切片增加的是函数引脚int32/name与旋转连接，不要自行声明int/name/rotator/object成员。事件仍唯一Actor.ReceiveBeginPlay，已有分支/自有变量读写可自由组合。

普通调用class=`/Script/BlueprintGraph.K2Node_CallFunction`，properties恰为owner/member；使用目录中的函数族即可组合计数比较、文本处理、数值选择、向量数学和自身Actor朝向/缩放。

- defaults只写目录defaultWritable=true的未连接、非隐藏输入。int为整数JSON数字，范围[-2147483648,2147483647]；name为字符串，UTF-16长度最多1023，不能用对象路径替代。bool、string、real、vector严格匹配，有限数值/向量分量绝对值最多1e12，普通字符串最多4096字符。
- 旋转从MakeRotator的Roll/Pitch/Yaw生成，连接其ReturnValue到真实Rotator输入。不要写原始rotator数组；float与double引脚也需符合实际schema，不猜测自动转换。Conv_IntToDouble等显式节点可用于合法转换。
- Actor实例函数只绑定生成蓝图自己的implicit self。不要连接或默认填充self，不允许外部Actor/组件对象、任意类/函数、latent、宏、委托或动态引脚。新函数调用仍逐条从真实目录解析，不执行JSON携带代码。
- 执行顺序由execute/then及branch的then/else决定。先完成Actor变换再打印/回读，不把纯函数在nodes中的数组位置当执行顺序。

实际样例见[CountTrue](../Tests/BlueprintFamilies/CountTrue.json)、[CountFalse](../Tests/BlueprintFamilies/CountFalse.json)、[TextKnown](../Tests/BlueprintFamilies/TextKnown.json)、[VectorBasis](../Tests/BlueprintFamilies/VectorBasis.json)及[说明](../Tests/BlueprintFamilies/Readme.md)。这些是项目自写输入，不称外部AI原始输出。使用时只改requestId/assetPath为新名；新增效果应改变节点/默认值/连接，而不是要求插件增加专用配方。

要求生成前先列出需要的目录签名及未满足约束，再给JSON。缺少支持、引用或精确引脚时明确提出缺口，不用空节点或简单图替代。失败结果不算整体通过；运行和真实界面步骤未测必须标记未测。
