# NodeForge Build137 Repair5 插件与 AI 接入候选

适用于 **Windows x64、官方 Unreal Engine 5.3.2**。当前为预发布候选。

## 下载哪个包

| 使用方式 | 下载 | 包大小 | 安装说明 |
| --- | --- | --- | --- |
| 自己安装、在 UE 中使用 | [普通插件 ZIP](https://github.com/XRH66/NodeForge/releases/download/build137-repair5/NodeForgeBuild137Repair5PluginCandidate.zip) | 约 1.8 MB，解压后约 6.1 MB | [普通插件安装指南](https://github.com/XRH66/NodeForge/blob/2d471544bac80fd1cc7da503670ecf49239ce714/Docs/Installation.md) |
| 让有本机操作权限的 AI 安装或补装接入 | [AI 接入 ZIP](https://github.com/XRH66/NodeForge/releases/download/build137-repair5/NodeForgeBuild137Repair5Ai1.zip) | 约 28 MB，解压后约 69 MB | [AI 安装指南](https://github.com/XRH66/NodeForge/blob/2d471544bac80fd1cc7da503670ecf49239ce714/Docs/AiInstallation.md) |

普通使用只需普通插件 ZIP，无需 Python 或 MCP。AI 包自带固定运行环境；已有普通插件时补装 AI 部分，未安装时由工具下载此处同版普通插件。两种方式复用同一插件核心。

普通插件较小，是因为 UE 引擎由本机已有安装提供，包内主要是插件 DLL、源码、节点目录及安装文件，不含引擎、测试工程或大型资源。插件 DLL 约 3.6 MB，ZIP 压缩后约 1.3 MB。AI 包较大，主要增加 Python、MCP 依赖和配套资料。大小使用十进制 MB，GitHub 显示的二进制单位数值会略有不同。

## 其余附件是什么

| 附件 | 用途 |
| --- | --- |
| `AiReleaseManifest.json` | 当前完整版本配套清单：插件、UE、平台、AI 版本及两个 ZIP 的校验值，供 AI 安装工具核对 |
| `AiChecksums.sha256` | 当前普通插件和 AI 两个 ZIP 的 SHA256 校验值，用于确认下载文件完整 |
| `ReleaseManifest.json` | 普通插件首次发布时的清单，保留原发布记录 |
| `Checksums.sha256` | 普通插件首次发布时的校验记录 |
| GitHub 自动生成的 `Source code (zip)` / `Source code (tar.gz)` | 标签对应的源码归档，供开发者使用；不包含编译插件，也不包含后来补充的 AI1 实现 |

正常安装从上方两个入口选择即可。四个清单和校验附件用于版本核对，不是另外四个安装包；当前插件与 AI 配套关系以 `AiReleaseManifest.json` 和 `AiChecksums.sha256` 为准。测试、验证工程没有上传。

仓库仍私有，下载与指南需登录有仓库访问权限的 GitHub 账号。外部公开安装指南和网站下载入口尚未开放。

<details>
<summary>查看功能、支持范围与验证说明</summary>

插件首次自动识别工程/引擎/插件，确认保存绑定，重开免重复确认。

本Release现补充`Build137Repair5Ai1`可选接入包，共用上述DLL。包含固定Python3.12.4、SDK2.3.0及29项锁定依赖、统一安装/客户端配置工具、Mcp10R1配套资料。无需系统Python；支持全新安装或普通后补装，准确工程/UE识别，MCP JSON及Codex TOML配置。`-Connect`完成真实20工具发现、同版资料读取、DLL身份认证与只读状态查询。136 Repair6完整AI候选的DLL不同，不作为本Release匹配附件。

本候选17项真实UI、相关native机制、原CLI描述/只读预检及回退已有通过证据。目录仍experimental，71材质类/327蓝图签名；未变部分复用原有限资格。未新增资产或重跑全部节点、Tick/PIE、画面、SDK整批、无生成器打包，历史资格保留原DLL归属。

AI安装有限验收在本机通过：移除系统Python路径并指定无效PYTHONHOME/PYTHONPATH，使用自带运行时从私有Release下载普通ZIP并连接；普通后补装保留同DLL；配置保留其它服务，重复准备安装不覆盖文件，错误核心/同名配置/运行工程拒绝，原通道停止与正常关闭通过。未覆盖第二台干净Windows机器、所有客户端实际刷新、全部业务工具回归或其它UE/平台。Codex实际发现20工具；当前Work对话不在原生Codex app-server中，未声称该对话完成Codex业务调用。仓库私有、站点owner-only，外部公开安装引导尚未开放。

标签为 `build137-repair5`；`.uplugin` 保留原 `0.1.0-prototype`。准确DLL与附件由兼容清单/校验确定，不更换已验发行物。

原`ReleaseManifest.json`与`Checksums.sha256`保留普通插件首次发布事实。补充AI后的当前完整附件关系以`AiReleaseManifest.json`及`AiChecksums.sha256`为准，记录原插件SHA与AI源码提交；原标签和普通ZIP保持。

</details>
