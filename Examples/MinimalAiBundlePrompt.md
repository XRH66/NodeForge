# 最小示例提示词

这段提示词用于理解 `NodeForgePrototype` v1 的固定模板格式。新建实际节点图，请使用 [Graph 编写规则](GraphAiRules.md)，并提供当前插件导出的节点目录。

## 模板练习

> 请阅读随附的 BlueprintTemplate.json、MaterialTemplate.json 和 MinimalAiBundleRules.md。分别输出两份完整 JSON，不合并成数组。
>
> 蓝图保持模板定义的 BeginPlay 和 PrintString 结构。材质保持两个常量相乘并连接自发光输出，常量分别使用 0.75 和 0.5。
>
> 保留模板的格式、字段和节点顺序。如果没有取得目录或规则，指出缺少哪份资料，不猜测字段、节点或摘要。

所需文件：[规则](MinimalAiBundleRules.md)、[蓝图模板](MinimalAiBundle/BlueprintTemplate.json)、[材质模板](MinimalAiBundle/MaterialTemplate.json)、[配套目录](MinimalAiBundle/RealCatalog.json)。

这个练习只产生输入数据，不负责安装或生成资产。模板中的请求编号与资产路径不能假定在自己的工程中尚未使用；不要把输出直接当作 v0.1.0 已验证的生成结果。
