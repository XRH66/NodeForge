# 蓝图资产独立使用测试

本组检查生成的 Actor 蓝图在停用或移除 NodeForge 后是否仍能使用，以及打包后的运行结果。环境要求见[开发测试说明](../TestPlan.md)；专用 UE 工程和打包文件不随仓库提供。

输入基于[Actor 位移测试](../BlueprintMotion/Readme.md)，包括直接向量、向量变化、MakeVector、数组乱序、成员读取与赋值、BreakVector→MakeVector、位置读取和零向量。图中增加 `GetActorLocation → Conv_VectorToString → PrintString`，执行顺序保证移动后才打印位置。变化记录见[FixtureProvenance.json](FixtureProvenance.json)。

[Cases.json](Cases.json)还包含外部 self 和 string→vector 错误连接，预期为拒绝。模板中的目录摘要需绑定实际插件，不能直接导入。

## 检查顺序

1. 在专用工程生成、编译、保存并独立重开资产。
2. 进入 PIE，读取实际 Actor 位置和消息，位置容差按运行配置执行。
3. 把所需资产及正常依赖复制到独立工程，停用或移除 NodeForge，再检查使用情况。
4. 对独立工程打包，并实际运行包内内容。

每一步分别记录结果。`FVector.ToString` 仅保留三位小数，通过日志观察位置时不能声称达到 1e-6 的直接数值精度。消息必须来自实际 Getter 和对应 Actor，不能用常量坐标字符串代替。

这个用例组只覆盖清单中的资产和行为，不代表所有节点、材质画面或任意项目都已验证。
