# Graph v2 结构示例

本目录提供材质和 Actor 蓝图的 JSON 示例，便于阅读节点、参数和连线的组织方式。它们来自开发期间保存的输入，目录摘要可能与当前插件不同；**不应直接当作 v0.1.0 的即用示例**。

第一次使用请运行 Release 插件包附带的 `Examples/FirstMaterial.json`，见 [入门教程](../../Docs/GettingStarted.md)。

## 示例内容

| 文件 | 展示的结构 |
| --- | --- |
| [StaticColor.json](StaticColor.json) | 颜色参数与数学节点 |
| [DynamicPulse.json](DynamicPulse.json) | 使用 Time 参与材质计算 |
| [Water.json](Water.json) | 数学节点组合的半透明材质 |
| [WaterChanged.json](WaterChanged.json) | 调整上述材质的参数 |
| [BlueprintTextChanged.json](BlueprintTextChanged.json) | 文本默认值与输出 |
| [BlueprintBranch.json](BlueprintBranch.json) | 变量、条件与分支 |
| [BlueprintBranchFalse.json](BlueprintBranchFalse.json) | 同类分支的另一条路径 |
| [BlueprintConcat.json](BlueprintConcat.json) | 字符串拼接与函数连接 |

示例名称只说明组织思路，不保证在任意场景中产生同样的视觉或运行效果。

## 用于自己的工程

1. 从当前安装的插件导出规则，或读取 [v0.1.0 目录](../../Docs/Versions/v0.1.0/Catalog.json)。
2. 另存所需示例，逐项核对节点类、参数、默认值和引脚是否仍符合当前规则。
3. 需要资源的输入使用自己工程中真实、允许读取的资源，重新取得必要的参数与文件哈希。
4. 使用当前规则形成完整输入，并填写新的 `requestId` 和目标资产名称。不要只替换目录摘要来绕过不匹配提示。
5. 在面板检查输入，通过后生成并查看结果。

同名目标不会被覆盖。若操作结果未知，先查看原报告，不要连续换名重复生成。

[通用编写规则](../GraphAiRules.md) · [JSON 格式](../../Docs/Versions/v0.1.0/Reference/GraphAuthoring.md)
