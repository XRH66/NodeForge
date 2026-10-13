# 夜间核心受影响回归

复用原GraphV2和MathRotation样例，另外保存标记清楚的大值默认/float32派生数据。只有JSON变化，不为用例添加生成分支。[准备脚本](../../Reports/20261010NightAudit/PrepareGraphCases.py)与生成的Provenance记录来源、变更和字节；运行由现有RunGraphRegression统一调度。

LargeFloatPin验证K2储存文本和保存重开；函数原生float运行仍会量化，不能说成为double精确。LargeMaterialFloat验证HDR标量999999.9原生float32存储999999.875；只编译/保存/读回，无画面正确声明，不作为实用材质效果。

本文件夹不含新节点族；测试目标均为独立工程的新资产。真实UI、画面、打包另需相应证据。
