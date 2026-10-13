# 扩展 API

新增操作应先实现共享业务逻辑，再接入界面、CLI 和 MCP。这样手动使用和 AI 调用才能获得一致的行为与错误信息。

## 实现顺序

1. 检查现有共享 Service 是否已有所需功能。
2. 在 C++ 业务层实现操作，明确输入、输出、只读或写入行为，以及失败后可能留下的结果。
3. 在 `NodeForgeControlOperations` 登记操作名称、host、dispatch、修订要求和参数 Schema。
4. 接入 CLI 参数校验和调用路径，保留 `busy`、`pending`、`unknown` 等原生状态。
5. 让界面复用相同 Service，避免另写一套点击流程。
6. 确认适合向 AI 开放后，更新 `Tools/NodeForgeMcp/ToolMetadata.json`、连接服务 adapter 和工具契约。
7. 补充调用示例、结果解释、失败处理和功能说明，再验证真实调用路径。

## AI 开放规则

工具元数据使用 `defaultAiExposure=deny`。新增原生操作不会自动成为 MCP 工具；需要明确开放政策和对应 adapter。管理操作如 `session.hello`、`control.disable` 不作为任意业务转发入口。

Schema 应准确表达必需字段、可选字段和 `dependentRequired` 等约束。结果必须保留 CLI 与原生业务状态，不能把进程退出成功当成资产生成成功。

## 验证重点

读操作检查资源范围、返回完整性和是否改变草稿；写操作检查修订冲突、重复调用、目标占用、部分写入和恢复路径。仅在相关实现变化时扩大回归范围。

发布前同步 [API 参考](../Ai/ApiReference.md)、[功能说明](../Reference/FeatureMap.md)和工具契约，按照 [版本发布](VersionRelease.md)更新完整安装包。
