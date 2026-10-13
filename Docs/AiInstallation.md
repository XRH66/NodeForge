# AI 安装与接入状态

**当前没有与Build137 Repair5配套的AI接入包或可执行一键安装入口。** Build136 Repair6是已验历史完整候选，仍需准备Python3.12及固定依赖，不能作为137补装包。

未来由有本机权限的AI读取公开指南，调用统一工具完成：匹配Release下载与校验、明确工程/UE识别、验证或安装同核心、准备固定运行时、配置受支持客户端、实际发现工具与连接检查。已有普通插件只补AI部分，复用同一DLL和绑定。

现有 `PrepareNodeForgeInstallation.py` 提供安装/环境/身份验证及准备回执。下载、工程发现、运行时部署和通用客户端配置仍未实现；支持MCP不等于已经提供NodeForge环境、执行权限或配置权限。

网站可复制指令将在工具及匹配包交付后上线。指令先读清单的AI状态；缺包或权限须返回实际缺项，不能调用尚不存在的命令。当前不提供误导性的可执行指令。

- [普通安装](Installation.md)：已验137入口。
- [136历史AI安装](Versions/Build136/Mcp9R13/User/Installation.md)：只用于对应核心。
- [当前R19 AI说明](Versions/Build136/Mcp9R19/User/AiInstallation.md)：现有流程与未实现范围。
- [统一发布门槛](GitHubReleaseDeliveryPlan.md)与[兼容基线](../Releases/Build137Repair5/CompatibilityBaseline.json)。

GitHub私有、站点owner-only，这些链接尚不是外部AI的公开安装指南。
