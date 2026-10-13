# 蓝图普通函数族回归

所有节点和引脚取自[实际UE5.3.2目录](../../Reports/20261009UsableEditor/CapabilityExpansion1/ActualCatalog.json)，通过现有通用NodeSpawner/K2 schema构图。目录实例可构建仍属实验支持，不能代替保存重开与运行证据。

- CountTrue / CountFalse / CountShuffled：整型相加与阈值分支，真分支将自身Actor旋转到Yaw90、缩放到2/1/1并输出count=5；假分支保持初始变换；反转输入数组仍应等价。
- TextKnown / TextAbsent：名称→字符串、切片、包含判断、选择、长度与字符串连接，输出分别为NodeForge9和missing7。
- VectorBasis：读取自身Actor前/右/上方向，叉积、向量加减、点积、长度与显式整型转double，生成实际位置1/sqrt(5)/7。
- ScalarSelection：整型乘减、比较、布尔、选择、显式转换与浮点除法/限幅，生成实际位置1/0/0。
- 关键负例包含分数/越界/错误类型整型、错误/超长名称、错误函数/引脚/连线、双源输入、外部target、旧目录扩展越界与同名资产保护。

Cases.json交给现有RunGraphRegression.py，RuntimeCases.json交给扩展后的RunBlueprintMotionChecks.py。运行时旋转数组顺序为Pitch/Yaw/Roll；位置和缩放为X/Y/Z。失败报告保留，不删除断言或在源码中挑选样例配方。当前实测状态由[函数族说明](../../Docs/BlueprintFunctionFamilies.md)与[项目状态](../../ProjectStatus.md)维护。
