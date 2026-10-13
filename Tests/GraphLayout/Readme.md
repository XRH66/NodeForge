# 布局独立回归素材

本目录从已有 [GraphV2](../GraphV2/Cases.json) 的输入派生，保留原始语义，只增加布局检验与新资产名称。原33项素材未修改。本目录当前19项：12正例、7负例；这些是测试输入，UE实测结果以对应新运行目录为准。

使用已有runner：[RunGraphRegression.py](../../Scripts/RunGraphRegression.py)。默认仍运行 `Tests/GraphV2`；传 `--fixtures Tests/GraphLayout` 才选择本矩阵。runner把本轮真实GraphCatalog的digest写入新报告目录下的输入，不改变这里的素材。

根调度者在一次插件构建、同步和真实目录导出后串行执行：

```powershell
python Scripts/RunGraphRegression.py --run LayoutRun1 --catalog Reports/本轮目录/Catalog.json --fixtures Tests/GraphLayout --timeout 360
```

命令中的目录文件必须替换为本轮实际导出的目录。`--prepare-only` 只准备输入和独立工程，不能报告UE通过。旧目录不可复用；同名资产拒绝，失败证据保留。

## 检查范围

- 材质无位置、非常规整数部分锚点、显式锚点重叠保留、长参数标题。
- 蓝图无位置、部分锚点、两分支、长引脚默认值。
- 水面33节点/40边，完整普通构图器生成。
- 材质/蓝图/水面的nodes与edges倒序，与先前对应输入按逻辑ID比对实际坐标，不重新实现布局算法计算预期。
- 两领域各三项非法坐标：非整数、越界、数组长度错误；复用同名保护路径进行负例。
- 初始、编译后、发布、独立新进程重开节点与布局报告逐ID一致；输入显式坐标精确保留。
- 使用实际报告的 `boundsSize` 检查矩形，含材质虚拟根；任何自动节点不得重叠。两个显式锚点可保留重叠，但必须产生 `explicitAnchorOverlapWarnings`。

布局报告尺寸是原生保守估算，不是Slate绘制实测。此矩阵证明坐标、报告矩形及保存的一致性，不替代必要的真实编辑器可读性观察；也不运行PIE、验证水面画面或打包。

## 验证状态

素材已通过当前真实 [UE532Graph](../../Catalog/UE532Graph.json) 的12项正例类、属性和引脚签名预检。runner已检查Python语法，且用独立合成读回验证：正确位置接受、锚点移动拒绝、自动矩形重叠拒绝、显式锚点冲突缺warning拒绝。以上是测试器静态核对；尚不表示布局插件已构建或UE回归通过。
