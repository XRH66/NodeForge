<p align="center">
  <img src="Tools/NodeForgeDocs/HeroPreview/Brand/BrandIcon.png" width="100" alt="NodeForge 标志">
</p>
<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="Tools/NodeForgeDocs/HeroPreview/Brand/WordmarkDark.svg">
    <source media="(prefers-color-scheme: light)" srcset="Tools/NodeForgeDocs/HeroPreview/Brand/WordmarkLight.svg">
    <img src="Tools/NodeForgeDocs/HeroPreview/Brand/WordmarkLight.svg" width="520" alt="NodeForge · 节点熔炉">
  </picture>
</p>
<h3 align="center">Unreal Engine 蓝图与材质生成插件</h3>
<p align="center">把 JSON 节点描述变成可编辑的 UE 资产，也可以让 AI 帮你完成。</p>
<p align="center"><strong>v1.1.2 · Unreal Engine 5.3.2 · Windows 64 位</strong></p>
<p align="center">
  <a href="https://nodeforge.cn/">官网</a> ·
  <a href="https://github.com/XRH66/NodeForge/releases/tag/v1.1.2">下载 v1.1.2</a> ·
  <a href="Docs/Installation.md">安装</a> ·
  <a href="Docs/GettingStarted.md">开始使用</a> ·
  <a href="Docs/AiInstallation.md">连接 AI</a> ·
  <a href="Docs/Versions/v1.1.2/User/Readme.md">用户手册</a> ·
  <a href="CHANGELOG.md">更新日志</a>
</p>

NodeForge 可以把 JSON 中描述的节点和连线生成为 UE 蓝图或材质。生成的资产可以继续在 UE 中编辑。你可以在插件面板中操作，也可以连接 AI 客户端，让 AI 调用插件完成任务。

## 可以做什么

- **生成节点图**：从 JSON 创建材质和 Actor 蓝图，生成前检查节点、参数与连线。
- **读取已有资源**：查询资源信息，将支持的蓝图和材质导出为 JSON。
- **创建新副本**：以已有节点图为起点修改，先查看差异，再生成新资产。
- **材质设置**：支持清漆、各向异性与 IOR 折射，字段可读取并导出。
- **连接 AI**：让支持 MCP 的 AI 客户端查询节点规则、读取资源和调用插件。
- **批量调用回收**：v1.1.2 默认自动回收已完成记录，无需计数或手动清理，保留原调用和报告，详见[使用方法](Docs/Versions/v1.1.2/Reference/CallCapacity.md)。

## 为什么使用 NodeForge

- **生成纯正的 UE 原生资产**：生成的材质（`UMaterial`）和蓝图（`UBlueprint`）不附带任何插件运行时依赖。后续随时可以在 UE 编辑器中手动调整节点，停用或移除插件后资产仍可正常打开、编译与打包。
- **先检查再生成，拒绝 AI 盲猜**：内置从官方 UE 5.3.2 实际采集的节点目录与校验规则。生成前自动检查节点类型、引脚与参数，错误明确提示，不会用空图或损坏资产掩盖错误。
- **默认保护原有工程资产**：只创建新资产，绝不静默覆盖已有内容。需要基于现有材质或蓝图迭代时，可先导出 JSON、预览差异（Diff），确认后再生成新副本。
- **人机同核，开箱即用**：普通插件解压即可在 UE 面板中独立使用，无需安装 Python、无需注册账号、无遥测；需要 AI 协同（如 Codex 等 MCP 客户端）时，可选装自带固定运行环境的 AI 接入包。
- **支持大批量连续生成**：v1.1.2 原生自动回收已完成的调用记录，AI 连续批量生成材质或查询规则时无需计数或中断清理，同时完整保留每次调用的输入与报告。

## 下载安装

**v1.1.2 · Windows 64 位 · 官方 Unreal Engine 5.3.2**

| 你想怎样使用 | 下载 | 下一步 |
| --- | --- | --- |
| 在 UE 中使用插件 | [插件 ZIP](https://github.com/XRH66/NodeForge/releases/download/v1.1.2/NodeForge-1.1.2-UE5.3.2-Windows.zip) | [安装插件](Docs/Installation.md) |
| 让 AI 安装并连接插件 | [AI 接入 ZIP](https://github.com/XRH66/NodeForge/releases/download/v1.1.2/NodeForge-AI-1.1.2-Windows.zip) | [连接 AI](Docs/AiInstallation.md) |

普通插件无需安装 Python。AI 包也已包含所需运行环境；如果已经安装插件，可以随后补装 AI 接入部分。请使用此处同一版本的两个包。

## 快速开始

1. 解压插件 ZIP，把其中的 `NodeForge` 文件夹放进引擎的 `Engine/Plugins/Marketplace` 文件夹。
2. 打开工程，启用插件，从 **窗口 → NodeForge** 打开面板，确认工程位置。
3. 点击 **载入 → 载入案例**，点击 **检查**，再点击 **生成**。
4. 点击 **打开生成资产**，查看生成的材质节点图。

示例文件的位置、完整操作和重复生成的方法见 [入门教程](Docs/GettingStarted.md)。

## 使用范围

当前支持材质和单个 EventGraph 的 Actor 蓝图，暂不支持任意 UE 节点、蓝图多图或新增组件。可在插件面板中查询支持的节点。

插件创建新资产；需要调整已有内容时，通过新副本完成。当前生成目录为 `Content/NodeForgeTests`。更多操作见 [使用手册](Docs/Versions/v1.1.2/User/Readme.md)。

## 问题反馈

遇到问题可以 [提交 Issue](https://github.com/XRH66/NodeForge/issues)，请说明 UE 版本、操作步骤和错误提示。

## 开发

编译、目录说明和打包方法见 [开发说明](Docs/RepositoryGuide.md)。

**技术栈：** Unreal Engine C++ · Python · MCP
