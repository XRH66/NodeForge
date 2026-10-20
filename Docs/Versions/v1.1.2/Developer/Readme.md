# 开发指南

NodeForge v1.1.2 是 Unreal Engine 5.3.2 的编辑器插件，支持 Windows 64 位。它把 JSON 图描述转换成可编辑的材质和蓝图，并提供资源读取、图导出、差异预览和 MCP 接口。

先从 [GitHub 仓库](https://github.com/XRH66/NodeForge)获取源码。仓库中的插件代码在 `Plugins/NodeForge/Source/`，Python 工具在 `Scripts/`。插件 ZIP 中的源码位于 `NodeForge/Source/`；AI ZIP 包含连接工具和运行环境，不重复携带插件核心。

## 从哪里开始

| 你想做什么 | 阅读 |
| --- | --- |
| 理解模块如何协作 | [架构与状态](Architecture.md) |
| 找到功能对应的源码 | [源码地图](SourceMap.md) |
| 增加节点或资产字段 | [扩展节点](NodeExtension.md) |
| 增加工具操作 | [扩展 API](OperationExtension.md) |
| 验证修改是否正确 | [验证改动](TestingAndEvidence.md) |
| 调查中断或不确定的结果 | [故障记录与恢复](RecoveryAndHandoff.md) |
| 准备新版本 | [版本发布](VersionRelease.md) |

首次使用请先完成 [安装与连接](../User/AiInstallation.md)，再按 [AI 调用流程](../Ai/QuickStart.md)理解真实使用路径。所有入口都应调用同一套业务逻辑；新增功能需要同步更新使用说明、参数定义和错误处理。
