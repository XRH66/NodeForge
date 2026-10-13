# 整数/名称成员变量样例与回归

输入从Build40实际原生目录生成，`Cases.json`控制构图检查，`RuntimeCases.json`控制保存关卡后独立PIE。实际终态见[功能说明](../../Docs/BlueprintMemberScalars.md)，仅存在JSON不表示已通过。

- MemberCount：成员Count初值3，读取→加2→写回，再读取参与阈值分支；预期count=5、Yaw90、Scale[2,1,1]。MemberLow只改初值0，预期below threshold和初始变换。MemberShuffled倒序nodes/edges，要求与MemberCount相同结构及布局。
- MemberState：name成员Mode初值Idle，执行写为NodeForgeName，读取并通过已有字符串切片/选择生成NodeForge9。MemberNone减少节点，直接读取默认None并输出。
- MemberLimits：读取int32两端精确默认值，预期输出-21474836482147483647；保存/独立重开须保留两个CDO整数。
- MemberRootLegacy、MemberCoreLegacy、MemberMaterialLegacy保留旧摘要，分别覆盖根组件、已有bool/string成员及纹理材质范围。
- 负例包括int小数/越界/bool混用、空/超长/数值name默认、旧目录使用新成员、未声明变量、错误引脚、非法类型连接及同名目标。不得写入目标或修改已有资产。

准备代码位于Reports/20261009UsableEditor/MemberScalarExpansion1/PrepareFixtures.py，属于项目测试数据准备，不是插件生成逻辑。全部样例由同一原生变量/函数构图路径消费，测试期间冻结源码及DLL。没有外部AI原始输出或人工界面通过声明。
