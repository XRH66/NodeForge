# 源码仓库说明

普通使用请从 [安装插件](Installation.md) 开始。本页面向需要阅读源码、编译插件或制作发行包的开发者。

## 目录

| 路径 | 内容 |
| --- | --- |
| `Plugins/NodeForge/Source/NodeForgeEditor` | UE 编辑器模块，包含构图、资产读取、安装面板和控制服务 |
| `Scripts` | CLI、MCP 服务、安装和打包工具 |
| `Contracts` | JSON 与控制接口协议 |
| `Catalog` | 节点目录数据及说明 |
| `Examples` | JSON 结构示例和编写规则 |
| `Tests` | 测试脚本、输入数据和预期规则 |
| `Docs/Versions/v1.1.2` | v1.1.2 用户、AI 和开发文档，以及配套节点目录 |
| `Releases/1.1.2` | v1.1.2 安装清单、插件描述和 AI 接口契约 |
| `Tools/NodeForgeMcp` | 固定依赖清单和第三方许可 |
| `Tools/NodeForgeDocs` | 文档生成、网页主题及相关工具 |

仓库不包含 UE 工程、编译产物、Python 运行环境或依赖 wheel 文件。普通插件和 AI 接入所需的运行文件通过 [Release](https://github.com/XRH66/NodeForge/releases/tag/v1.1.2) 提供。

## 编译插件

需要官方 UE 5.3.2，以及该引擎版本要求的 Windows C++ 工具链。

1. 创建或选用自己的 UE 5.3.2 C++ 工程，关闭正在运行的编辑器。
2. 把本仓库的 `Plugins/NodeForge` 放入工程的 `Plugins` 目录。
3. 生成工程文件，在 IDE 中构建该工程的 Editor 目标，配置为 Development Editor / Win64。
4. 打开工程并启用 NodeForge。编译后的模块位于所选工程或插件的输出目录，以本次构建结果为准。

[BuildPrototype.ps1](../Scripts/BuildPrototype.ps1) 是维护者使用的构建脚本，其中固定了引擎位置、工程名和工具链版本。它依赖仓库未附带的开发工程，克隆后不能直接运行。外部开发者应使用自己的 C++ 工程进行编译。

发行包的安装清单会核对模块文件、引擎版本和节点目录。自己编译得到的 DLL 不能冒用发布包的 SHA256；要接入自动安装或 AI，必须为这套产物生成匹配的安装信息和文档。

## 制作普通插件包

[PackageNodeForgePlugin.py](../Scripts/PackageNodeForgePlugin.py) 使用已有的编译插件 ZIP 作为输入，更新所选版本 的插件描述、安装信息和用户说明。它不负责调用 UE 编译。

```text
python Scripts/PackageNodeForgePlugin.py --version 1.1.2 --base <已有编译插件ZIP> --out <新的输出ZIP>
```

输入 ZIP 应包含 `NodeForge/Binaries/Win64/UnrealEditor-NodeForgeEditor.dll`，其 SHA256 必须与 [InstallationManifest.json](../Releases/1.1.2/InstallationManifest.json) 一致。输出路径必须尚不存在。

## 制作 AI 接入包

[PackageNodeForgeAi.py](../Scripts/PackageNodeForgeAi.py) 组装固定 Python 运行环境、连接服务、安装工具和当前版本文档。准备以下输入：

- 官方 Python 3.12.4 的 Windows amd64 嵌入式 ZIP；工具会检查固定 SHA256。
- [DependencyManifest.json](../Tools/NodeForgeMcp/DependencyManifest.json) 中列出的全部 wheel 文件，放在各条目 `wheel` 指定的位置，文件和许可证哈希须匹配。
- 同版普通插件 ZIP，以及上传后得到的 GitHub 附件 ID。
- [Releases/1.1.2/ToolContract.json](../Releases/1.1.2/ToolContract.json) 与 [v1.1.2 文档清单](Versions/v1.1.2/Manifest.json)。

```text
python Scripts/PackageNodeForgeAi.py --version 1.1.2 --out <新的输出目录> --embed <Python嵌入ZIP> --plugin-zip <同版插件ZIP> --plugin-asset-id <GitHub附件ID>
```

工具从本地 wheel 离线安装依赖，不会替你下载它们。输出目录和同名 ZIP 都应为新路径。运行时文件的来源与哈希记录在生成的 `RuntimeManifest.json` 中；普通插件下载地址与哈希记录在 `AiPackageManifest.json` 中。

发布工具读取 `Releases/1.1.2` 的契约，不使用维护者本机的客户端配置或会话文件。详细附件规则见 [发布说明](GitHubReleaseDeliveryPlan.md)。

## 文档与示例

v1.1.2 的公开手册位于 [版本文档](Versions/v1.1.2/Readme.md)，用户、AI 和开发者文档都在该目录维护。正式网站为 [nodeforge.cn](https://nodeforge.cn/)，当前分支与官网同步维护正文及[来源与第三方许可](Versions/v1.1.2/Reference/SourcesAndLicenses.md)。发行标签与已上传的安装包保留发布时快照；后续文字和许可补全不替换既有附件或移动标签。AI 打包工具直接读取这里的正文和清单。修改后应同步文档清单中的文件信息，并检查相对链接。

网页共享资源的生成入口为：

```text
node Tools/NodeForgeDocs/HeroPreview/BuildTheme.mjs --output <目标目录>
```

[Examples](../Examples/GraphAiRules.md) 中的部分 JSON 来自开发时保存的结构样例，可能含不同的目录摘要或资源占位值。使用前需要读取当前插件目录，并核对输入；第一次使用推荐插件包附带的 `Examples/FirstMaterial.json`。

## 修改后检查什么

编译后先确认实际加载的是本次模块，再检查与修改相关的功能。普通 JSON 示例不证明任意组合都能运行，保存资产也不等于已经检查游戏行为或材质效果。测试需要自己的 UE 工程和真实资源；本仓库没有可直接打开的测试工程。

[开发手册](Versions/v1.1.2/Developer/Readme.md) · [插件源码](../Plugins/NodeForge/Readme.md) · [第三方许可](../Plugins/NodeForge/ThirdPartyNotices.md)
