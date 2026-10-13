# 自身根组件调用回归

输入根据[本机Build39实际目录](../../Reports/20261009UsableEditor/ComponentExpansion1/ActualCatalog.json)生成。相同插件接收不同节点、默认值和连接，没有按图名选择配方。每项是否通过以[项目状态](../../ProjectStatus.md)及[组件能力](../../Docs/BlueprintComponents.md)的实际报告为准。

- RootMotion：BeginPlay取得Actor自身Root，组件函数设置世界位置[120,-40,15]、Yaw90、Scale[2,1,1]；三个组件getter回读并供给原Actor setter，最后输出Root motion A。运行器独立检查实际Actor变换，getter不能仅作为未消费装饰节点。
- RootChanged：只改输入位置、角度、缩放和消息，运行期望[-15,22,8]、Yaw-45、Scale[0.5,3,2]和Root motion B。
- RootShuffled：反转nodes/edges，与RootMotion图/布局对照，运行结果应相同。
- RootOffset：先通过组件设置位置，再读取组件位置并加[5,0,2]，经第二个组件调用写回，最终[15,20,32]。节点数量及连接结构不同。
- RootLegacyCount、RootMaterialLegacy：保留699旧蓝图与340纹理材质摘要，在新插件中保持其原能力。
- 十个负例：缺self、对象路径默认、错误类型来源、多源self、错误输出名、非UFUNCTION别名、未适配函数、699/340旧目录越权及同名保护。实际拒绝后必须未生成目标、原资产保持。

所有组件self都从同图Actor.K2_GetRootComponent.ReturnValue直连；没有外部对象加载、用户组件声明或对象成员变量。旋转由MakeRotator连接，不写原始Rotator默认值。样例bSweep=false，不宣称碰撞扫掠或自定义组件行为已验证。
