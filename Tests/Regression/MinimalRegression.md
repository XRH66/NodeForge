# 固定最小回归

本组保留两个最小输入和固定拒绝例，用于检查基础解析、生成、保存及错误处理。它使用 `NodeForgePrototype` 固定格式，不能代替当前 NodeForgeGraph v2/v3 的完整测试。

## 文件与运行器

- [MinimalBlueprint.json](../Valid/MinimalBlueprint.json)：最小蓝图输入。
- [MinimalMaterialReady.json](../Valid/MinimalMaterialReady.json)：最小材质输入。
- [PrototypeCases.json](PrototypeCases.json)：错误类型、未知标识或引脚、摘要、重复键、深度、未知字段、同名及请求内容冲突等拒绝预期。
- [RunPrototypeRegression.py](../../Scripts/RunPrototypeRegression.py)：创建专用空工程，依次生成、独立重开、创建固定场景、检查 PIE 和最小面板。
- [ValidatePrototype.py](../../Scripts/ValidatePrototype.py)：在指定专用工程中执行固定拒绝用例。

## 执行前提

按[开发测试说明](../TestPlan.md)准备匹配的引擎和插件。RunPrototypeRegression 使用新的 `--run` 和显式 `--plugin-directory`；ValidatePrototype 使用新的 `--report-dir` 与明确的 `--project`。不要依赖脚本中的默认开发工程或固定引擎位置。

两个运行器不会为测试编译插件。所选插件必须已经构建，输入中的目录要求也需匹配。报告、工程和目标名称不得与已有运行冲突。

## 检查范围

固定用例和预期应保持不变。正例检查实际结构、编译、保存和独立重开；负例应明确拒绝且不修改已有资产。若只运行部分检查，应在结果中列出选定范围。

最小面板检查覆盖 Slate 构造，不代替文件选择、粘贴等真实交互。材质画面、停用插件后的使用、打包和其他格式或节点需要对应的单独检查。
