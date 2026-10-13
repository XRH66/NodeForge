# 数学与插值数据回归

新能力Tests输入为模板，Scripts/RunGraphRegression.py绑定[真实Build46目录](../../Catalog/UE532BlueprintMathInterpolation.json)；历史兼容/权限负例保留原摘要。可导入数据位于Reports/20261008GraphV2/MathInterpolation1/Inputs。准备脚本不执行引擎，结果以报告为准。

- MathRange：Abs(-12)→Min(8)→Max(3)→Lerp(20,0.25)，期望Actor X=11、转换消息math=11.0。
- MathChanged只改A/Alpha，X=16；MathTopology改连接和未连默认，X=6.75；MathReduced删除两节点重连，X=14；MathShuffled反序数组，图与布局应等同MathRange。
- MathDegrees/MathDegreesChanged：度制正弦余弦乘10后构造位置，期望[10,-10,0]/[0,10,0]。
- MathRotation：0→90°按0.5，Yaw45°；MathShortest/MathLinear同170→-150°数据，仅改变bShortestPath，期望-170°/10°。MathMember从Rotator成员取A，10→70°按0.25，期望25°。
- MathMaterialCompat/MathRotatorCompat：保留Build45材质/Build44旋转摘要，代表性编译保存重开。
- 十个关键负例：错误标量/旋转默认、引脚大小写、未知属性/函数、非法类型连接、连线且默认、两个旧摘要越权、同名拒绝保护既有资产。

Scripts/RunBlueprintMotionChecks.py独立保存关卡、新进程PIE，检查实际Actor位置/旋转及归属消息，不以编译替代运行。本批不覆盖新UI、外部AI及无插件打包。
