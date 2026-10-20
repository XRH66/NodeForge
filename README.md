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

NodeForge 是一个 Unreal Engine 插件：把 JSON 里描述的节点和连线生成为蓝图或材质，生成的资产可以继续在 UE 中编辑。用法可以很简单——向生成式 AI 描述你想要的蓝图逻辑或材质效果，AI 按对应的编写规则写出 JSON，插件检查后生成资产；也可以不接 AI，手动写 JSON，或直接从附带案例开始。

## 可以做什么

- **生成节点图**：从 JSON 创建材质和 Actor 蓝图，生成前检查节点、参数与连线。
- **读取已有资源**：查询资源信息，将支持的蓝图和材质导出为 JSON。
- **创建新副本**：以已有节点图为起点修改，先查看差异，再生成新资产。
- **材质设置**：支持清漆、各向异性与 IOR 折射，字段可读取并导出。
- **让 AI 参与**：把编写规则交给普通聊天里的生成式 AI，由它写出 JSON；需要 AI 直接操作插件时，再接入支持 MCP 的客户端。
- **连续生成不中断**：v1.1.2 默认自动回收已完成记录，无需计数或手动清理，保留原调用和报告，详见[使用方法](Docs/Versions/v1.1.2/Reference/CallCapacity.md)。

## 为什么使用 NodeForge

- **生成纯正的 UE 原生资产**：生成的材质（`UMaterial`）和蓝图（`UBlueprint`）不附带任何插件运行时依赖。后续随时可以在 UE 编辑器中手动调整节点，停用或移除插件后资产仍可正常打开、编译与打包。
- **生成前检查节点和连线**：内置从官方 UE 5.3.2 实际采集的节点目录与校验规则。生成前会检查节点类型、引脚、参数和连线，不符合规则时明确指出原因，不会用空图或损坏资产掩盖问题。检查通过只表示输入符合规则，实际效果仍需在 UE 中确认。
- **默认保护原有工程资产**：只创建新资产，绝不静默覆盖已有内容。需要基于现有材质或蓝图迭代时，可先导出 JSON、预览差异（Diff），确认后再生成新副本。
- **在面板里就能用**：普通插件解压后在 UE 面板中即可独立使用，不需要安装 Python、注册账号，也没有遥测；只是让 AI 写 JSON 时，普通聊天工具就够，不必安装接入包。
- **支持大批量连续生成**：v1.1.2 原生自动回收已完成的调用记录，AI 连续批量生成材质或查询规则时无需计数或中断清理，同时完整保留每次调用的输入与报告。

## 下载安装

**v1.1.2 · Windows 64 位 · 官方 Unreal Engine 5.3.2**

| 你想怎样使用 | 下载 | 下一步 |
| --- | --- | --- |
| 在 UE 中使用插件 | [插件 ZIP](https://github.com/XRH66/NodeForge/releases/download/v1.1.2/NodeForge-1.1.2-UE5.3.2-Windows.zip) | [安装插件](Docs/Installation.md) |
| 让 AI 安装并连接插件 | [AI 接入 ZIP](https://github.com/XRH66/NodeForge/releases/download/v1.1.2/NodeForge-AI-1.1.2-Windows.zip) | [连接 AI](Docs/AiInstallation.md) |

普通插件无需安装 Python。AI 包也已包含所需运行环境；如果已经安装插件，可以随后补装 AI 接入部分。请使用此处同一版本的两个包。

## 先试用附带案例

1. 解压插件 ZIP，把其中的 `NodeForge` 文件夹放进引擎的 `Engine/Plugins/Marketplace` 文件夹。
2. 打开工程，启用插件，从 **窗口 → NodeForge** 打开面板，确认工程位置。
3. 点击 **载入 → 载入案例**，点击 **检查**，再点击 **生成**。
4. 点击 **打开生成资产**，查看生成的材质节点图。

示例文件的位置、完整操作和重复生成的方法见 [入门教程](Docs/GettingStarted.md)。

## 生成自己的蓝图或材质

不需要安装接入包，也不需要配置 MCP。把对应规则文档和需求交给能读取附件的生成式 AI，让它按 NodeForge 格式编写 JSON；AI 写出完整 JSON 后，由插件检查并生成。

| 想生成 | 规则文档 | 下载 Markdown |
| --- | --- | --- |
| 材质 | [材质编写规则](https://github.com/XRH66/NodeForge/blob/main/Docs/Versions/v1.1.2/Reference/MaterialRules.md) | [直接下载](https://github.com/XRH66/NodeForge/raw/main/Docs/Versions/v1.1.2/Reference/MaterialRules.md) |
| Actor 蓝图 | [蓝图编写规则](https://github.com/XRH66/NodeForge/blob/main/Docs/Versions/v1.1.2/Reference/BlueprintRules.md) | [直接下载](https://github.com/XRH66/NodeForge/raw/main/Docs/Versions/v1.1.2/Reference/BlueprintRules.md) |

1. 下载要用的那一份规则，例如你要一个材质效果就下载材质规则。规则里已经写清字段、限制和可用节点，不必再让 AI 去插件里查询。
2. 把规则文档、你想要的效果，以及必要的资源信息（例如已经保存好的贴图路径）一起发给 AI，让它输出完整的 JSON。
3. 复制 AI 返回的 JSON，在插件面板点击 **粘贴**；也可以先保存成 `.json` 文件，再点击 **载入**。
4. 点击 **检查**，按提示改到通过，再点击 **生成**。
5. 在 UE 中打开生成的资产，运行蓝图或查看材质效果，再按实际结果继续调整。

GitHub 的文件页面右上角有 **Download raw file**，可以下载同一份 Markdown；规则文档也提供了网页版，方便直接阅读。

蓝图逻辑和材质效果由 AI 设计，质量取决于 AI 的能力、需求描述的清晰程度和插件支持的节点范围。不同 AI 一次写对的程度并不相同；插件负责检查格式、节点与连线并生成资产，检查通过后仍应在 UE 中运行蓝图或查看材质效果，按实际结果调整。

## 让 AI 直接操作插件（可选）

如果你希望 AI 自己查询节点规则、读取资源并提交任务，可以使用 AI 接入包：它需要能操作本机、并支持 MCP 的客户端，配置步骤见 [连接 AI](Docs/AiInstallation.md)。只让 AI 写 JSON 时，用上面两份规则文档就够了，不必安装接入包。

## 使用范围

当前支持材质和单个 EventGraph 的 Actor 蓝图，暂不支持任意 UE 节点、蓝图多图或新增组件。可在插件面板中查询支持的节点，也可以直接看规则文档里的节点目录。

插件创建新资产；需要调整已有内容时，通过新副本完成。当前生成目录为 `Content/NodeForgeTests`。更多操作见 [使用手册](Docs/Versions/v1.1.2/User/Readme.md)。

## 问题反馈

遇到问题可以 [提交 Issue](https://github.com/XRH66/NodeForge/issues)，请说明 UE 版本、操作步骤和错误提示。

## 开发

编译、目录说明和打包方法见 [开发说明](Docs/RepositoryGuide.md)。

**技术栈：** Unreal Engine C++ · Python · MCP
