# Sequence 输入与运行检查

数据来自Build41本机原生目录，结果见[功能说明](../../Docs/BlueprintSequence.md)。同一导入器按节点/属性/连接逐项构图，插件无样例专用分支；准备代码在Reports/20261009UsableEditor/SequenceExpansion1/PrepareFixtures.py，仅生成测试JSON。

- SequenceTwo、SequenceThree：2/3输出分别按序写入A、追加B、最后输出，预期A或AB。
- SequenceChanged只改数据得到XY；SequenceSwapped只交换两个执行输出连接，先追加后覆盖，最终A；SequenceShuffled打乱nodes/edges仍AB且布局一致。
- SequenceEight：七组状态写入/追加再输出，预期1234567，涵盖8输出上界。
- SequenceNested：外层3输出的中间分支使用内层2输出追加B和C，最后输出ABC。
- 三个旧摘要图覆盖int成员、name成员及根组件，保持本来的范围。
- 关键负例：数量低/高/非整数/字符串/缺失，未知属性/类，错误或越界输出，执行默认值，循环和执行输出扇出，旧40目录及同名目标。

Cases.json包含24例，RuntimeCases.json为8个实际保存关卡/PIE样例。测试个数不是通过声明；以原生报告、独立重开、实际消息及保护证据为准。未验证配置继续为实验支持，真实UI/外部AI原始输出/打包另行验收。
