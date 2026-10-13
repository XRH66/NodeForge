# 空间输入回归与样例

这些JSON由项目的[数据准备脚本](../../Reports/20261009UsableEditor/SpatialExpansion1/PrepareFixtures.py)基于实际Build42目录产生，供同一个通用导入器处理；不是外部AI原始输出。结果及精确状态见[功能记录](../../Docs/MaterialSpatialInputs.md)。

- WorldBands/WorldChanged/WorldShuffled：世界X坐标、时间和颜色参数组合；只改数据和顺序，保存结构及布局比较。
- WorldHeight：原生Z输出驱动高度颜色插值。
- ObjectLocal/ObjectRelative：世界位置减物体中心，包含普通和摄像机相对原点；验证两类原生枚举。
- ViewFacing/CameraDistance：法线与视角、摄像机与像素距离的不同拓扑。
- NormalMotion：顶点法线乘时间波形接WPO，可调速度/幅度。
- 四个旧摘要样例覆盖原始材质、Fresnel扩展、纹理采样及Sequence蓝图；保留旧版本语义。
- 关键负例覆盖未知/数字/MAX枚举、未知属性/输入/输出、旧目录不获得新族、同名保护；PixelNormalWS用于顶点或Normal输入必须产生真实编译失败并且不保存目标。

Cases.json为27项；RuntimeCases.json仅旧Sequence实际PIE，VisualConfig.json仅WorldBands保存场景的时间与参数画面检查。未执行的画面、UI或包不由测试文件存在推断通过。普通与原生枚举读回、源字节和同DLL保护由现有统一运行器验证，不另写构图实现。

终态补充：27项通过，WorldBands画面通过。NormalVisualConfig.json对应初轮NormalMotion，其像素变化通过但可见性失败；修正数据另存[MaterialSpatialMotion](../MaterialSpatialMotion/)，增加基准高度和新目标，原数据保留。修正后实际画面及可见性检查通过，详功能记录，不把初轮失败改成成功。
