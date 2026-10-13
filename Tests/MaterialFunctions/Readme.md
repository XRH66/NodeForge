# 材质函数组合回归

Cases.json当前16项：8正例含原生可选默认、显式三输入、节点乱序、时间函数、Overlay/交换拓扑、双函数串联、速度参数动画；8负例含不存在输入/输出、过期/缺失哈希、错误资源类、额外属性、旧目录使用新函数节点、必需引脚未连接。所有数据通过同一材质构图器，无函数/效果专用分支。

源函数properties取自实际只读签名，原始模板目录摘要由回归脚本另存到Inputs后使用当前真实目录，不覆盖源数据。RunGraphRegression.py要求--function-signature报告，报告必须实测成功并具有所选DLL的实际加载指纹；成功回读另与该签名比较GUID、默认策略和properties，不能通过类的未绑定空目录跳过检查。

MaterialFunctions1（Build13）15项中FunctionSmoothDefault回读出现虚假边，历史Inputs/报告保留；修复原生CachedInputs视图生命周期后Build14验证MaterialFunctions2。成功与否以Summary.json为准，不以源码说明标通过。VisualConfig.json指定独立固定网格场景和Speed变化；视觉须另执行原生GPU检查。
