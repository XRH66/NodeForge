# 自身Actor与向量构图回归

19项输入只改变JSON，不选择固定图配方：9正例覆盖位置默认变化、原生MakeVector、节点乱序、向量成员默认/Get/Set、BreakVector数据连接、原生GetActorLocation及文本转换、零向量；10负例覆盖长度/类型/范围/分量错误、未知函数/引脚、外部self默认、不兼容连接、旧目录声称新函数或向量。

模板中的目录占位符不可直接导入。由既有RunGraphRegression.py注入当前真实目录到新Reports/Inputs，原模板保留。MotionShuffled与MotionMade比较语义和布局。RuntimeCases.json是独立运行期预期，不是节点目录；运行器要求完整矩阵通过、同源码/DLL，再新保存九地图，每个在新Editor进程PIE同时验证准确归属PrintString和真实Actor位置，不靠消息宣称移动成功。

代码复用/本机接口见[BlueprintGraphReuse](../../Docs/BlueprintGraphReuse.md)，实际结果与未验证事项见[当前实测](../../Reports/20261009UsableEditor.md)。Build15两项smoke通过；Build16完整19项构图passed，九图运行因测试CLI逗号截断在PIE前失败并保留。Build17修复后BlueprintMotion2重新生成/编译/保存/独立重开19项passed，BlueprintMotionRuntime2九地图/九独立PIE真实位置+准确归属消息十八步passed，源码/DLL/既有生成资产字节保护true。当前17原33/v1/11/包/UI另行验证，不继承14或用目录数量声称全UE节点可用。
