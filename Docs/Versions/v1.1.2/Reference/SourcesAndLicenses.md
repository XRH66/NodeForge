# 来源与第三方许可

本页说明 NodeForge v1.1.2 的代码来源、随包依赖、文档构建工具和研究参考。2026-10-17 补充核对；第三方版本按实际采用或当时研究的固定版本记录，不代表采用上游最新版本。

## 实际改编并采用的代码

以下三个项目的部分代码进入 NodeForge。没有整体集成它们的插件、网络服务或 AI 产品。对应版权声明、完整许可和修改范围随插件保留。

| 来源与固定提交 | 实际采用范围 | 许可原文 |
| --- | --- | --- |
| [ZiggyMar/unreal-mcp](https://github.com/ZiggyMar/unreal-mcp/tree/d1a85396d9c62328521256005fe7c420725651cc) · `d1a85396` | 蓝图节点创建、函数/自身变量绑定、目录过滤、编译诊断与保存；材质实例工厂创建和父材质设置片段。接入 NodeForgePrototype、NodeForgeBlueprintGraph、NodeForgeMaterialInstances。 | [MIT](../Assets/ThirdPartyLicenses/ZiggyMarMIT.txt) |
| [ChiR24/Unreal_mcp](https://github.com/ChiR24/Unreal_mcp/tree/591d3b0ea3c46a2a1926dbcafc225afecb6a75b3) · `591d3b0e` | HandleConnectNodes 的输出索引检查与 Wire 精确连线核心，适配到 NodeForgeMaterialGraph 的 ConnectExact；未采用别名与默认引脚回退。 | [MIT](../Assets/ThirdPartyLicenses/ChiR24MIT.txt) |
| [PRQELT/Autonomix](https://github.com/PRQELT/Autonomix/tree/47a46ae8cc3f00230edceeb33f0d4f9d80cb185b) · `47a46ae8` | AutoLayoutNodes 的 Kahn 最长路径分层骨架，适配到 NodeForgeGraphLayout，并增加数据依赖、稳定排序、节点尺寸、锚点和分支排序。 | [MIT](../Assets/ThirdPartyLicenses/AutonomixMIT.txt) |

文件与函数级说明见[插件第三方声明](https://github.com/XRH66/NodeForge/blob/v1.1.2/Plugins/NodeForge/ThirdPartyNotices.md)。离线可查看插件内的 ThirdPartyNotices.md 与 Licenses 目录；上表许可原文可直接在本站打开，无需 GitHub 仓库权限。

## 运行环境、依赖与文档工具

| 组件 | 版本与用途 | 许可及交付位置 |
| --- | --- | --- |
| Unreal Engine | 官方 UE 5.3.2；节点、反射、编辑器界面、编译和资产保存的宿主与原生 API。引擎由用户自行安装，插件包不包含完整引擎。 | Epic 的 Unreal Engine 许可，见[官方许可入口](https://www.unrealengine.com/eula/unreal)。不将引擎代码重新标为 MIT。 |
| Python | Windows x64 嵌入式 Python 3.12.4，随 AI 接入包提供；普通插件不要求单独安装 Python。 | [随发行包保留的完整许可](../Assets/ThirdPartyLicenses/Python3124.txt)，含 Python 与随附组件的声明；包内 Runtime/LICENSE.txt。 |
| MCP Python SDK | mcp / mcp-types 2.3.0，固定源码 [2118f14f](https://github.com/modelcontextprotocol/python-sdk/tree/2118f14f8a19bc158d8a1cf90af58d85d187f849)；提供协议与 stdio 通信。 | MIT；详下方完整依赖表及[AI 包声明](https://github.com/XRH66/NodeForge/blob/v1.1.2/Tools/NodeForgeMcp/ThirdPartyNotices.md)。 |
| Marked | 17.0.5；在文档构建时把 Markdown 转为 HTML。本站不需要在浏览器加载 Marked 运行时。 | [MIT 及文件内随附声明](../Assets/ThirdPartyLicenses/Marked1705.txt)；[固定上游版本](https://github.com/markedjs/marked/tree/v17.0.5)。保留完整 License.md，包括其中的 Markdown 组件声明。 |
| Windows 公共 API | 本地进程身份、命名管道、权限和异步 I/O；由操作系统提供。 | 使用官方系统 API，未把第三方进程库或管道服务器整体并入产品。 |

### AI 接入包完整依赖表

以下29项来自 v1.1.2 固定依赖清单，包括直接与传递依赖。列入依赖不表示 NodeForge 启用了该依赖的所有功能；连接桥使用 stdio，未开启 HTTP/OAuth/遥测入口。各许可原文从已发行 AI 包提取并核对原清单 SHA256，保留原字节；多份许可分别列出。

| 依赖 | 固定版本 | 清单记录的许可 | 本站许可原文 |
| --- | --- | --- | --- |
| mcp | 2.3.0 | MIT | [原文1](../Assets/ThirdPartyLicenses/mcp/mcp-2.3.0.dist-info/licenses/LICENSE.txt) |
| mcp-types | 2.3.0 | MIT | [原文1](../Assets/ThirdPartyLicenses/mcp-types/mcp_types-2.3.0.dist-info/licenses/LICENSE.txt) |
| anyio | 4.15.1 | MIT | [原文1](../Assets/ThirdPartyLicenses/anyio/anyio-4.15.1.dist-info/licenses/LICENSE.txt) |
| httpx2 | 2.13.1 | BSD-3-Clause | [原文1](../Assets/ThirdPartyLicenses/httpx2/httpx2-2.13.1.dist-info/licenses/LICENSE.md.txt) |
| httpcore2 | 2.13.1 | BSD-3-Clause | [原文1](../Assets/ThirdPartyLicenses/httpcore2/httpcore2-2.13.1.dist-info/licenses/LICENSE.md.txt) |
| jsonschema | 4.26.0 | MIT | [原文1](../Assets/ThirdPartyLicenses/jsonschema/jsonschema-4.26.0.dist-info/licenses/COPYING.txt) |
| opentelemetry-api | 1.45.1 | Apache-2.0 | [原文1](../Assets/ThirdPartyLicenses/opentelemetry-api/opentelemetry_api-1.45.1.dist-info/licenses/LICENSE.txt) |
| pydantic | 2.14.0 | MIT | [原文1](../Assets/ThirdPartyLicenses/pydantic/pydantic-2.14.0.dist-info/licenses/LICENSE.txt) |
| pydantic_core | 2.50.0 | MIT | [原文1](../Assets/ThirdPartyLicenses/pydantic_core/pydantic_core-2.50.0.dist-info/licenses/LICENSE.txt) |
| PyJWT | 2.15.1 | MIT | [原文1](../Assets/ThirdPartyLicenses/PyJWT/pyjwt-2.15.1.dist-info/licenses/AUTHORS.rst.txt)、[原文2](../Assets/ThirdPartyLicenses/PyJWT/pyjwt-2.15.1.dist-info/licenses/LICENSE.txt) |
| python-multipart | 0.0.32 | Apache-2.0 | [原文1](../Assets/ThirdPartyLicenses/python-multipart/python_multipart-0.0.32.dist-info/licenses/LICENSE.txt.txt) |
| pywin32 | 312 | PSF | [原文1](../Assets/ThirdPartyLicenses/pywin32/adodbapi/license.txt.txt)、[原文2](../Assets/ThirdPartyLicenses/pywin32/pythonwin/License.txt.txt)、[原文3](../Assets/ThirdPartyLicenses/pywin32/pythonwin/Scintilla-License.txt.txt)、[原文4](../Assets/ThirdPartyLicenses/pywin32/pythonwin/pywin/idle/LICENSE.txt.txt)、[原文5](../Assets/ThirdPartyLicenses/pywin32/pywin32-312.dist-info/licenses/adodbapi/license.txt.txt)、[原文6](../Assets/ThirdPartyLicenses/pywin32/pywin32-312.dist-info/licenses/com/License.txt.txt)、[原文7](../Assets/ThirdPartyLicenses/pywin32/pywin32-312.dist-info/licenses/com/win32comext/mapi/src/MAPIStubLibrary/LICENSE.txt)、[原文8](../Assets/ThirdPartyLicenses/pywin32/pywin32-312.dist-info/licenses/isapi/README.txt.txt)、[原文9](../Assets/ThirdPartyLicenses/pywin32/pywin32-312.dist-info/licenses/pythonwin/License.txt.txt)、[原文10](../Assets/ThirdPartyLicenses/pywin32/pywin32-312.dist-info/licenses/pythonwin/Scintilla/License.txt.txt)、[原文11](../Assets/ThirdPartyLicenses/pywin32/pywin32-312.dist-info/licenses/pythonwin/pywin/idle/LICENSE.txt.txt)、[原文12](../Assets/ThirdPartyLicenses/pywin32/pywin32-312.dist-info/licenses/win32/License.txt.txt)、[原文13](../Assets/ThirdPartyLicenses/pywin32/win32/License.txt.txt)、[原文14](../Assets/ThirdPartyLicenses/pywin32/win32com/License.txt.txt)、[原文15](../Assets/ThirdPartyLicenses/pywin32/win32comext/License.txt.txt)、[原文16](../Assets/ThirdPartyLicenses/pywin32/win32comext/mapi/MAPIStubLibrary-License.txt.txt) |
| sse-starlette | 3.5.0 | BSD-3-Clause | [原文1](../Assets/ThirdPartyLicenses/sse-starlette/sse_starlette-3.5.0.dist-info/licenses/AUTHORS.txt)、[原文2](../Assets/ThirdPartyLicenses/sse-starlette/sse_starlette-3.5.0.dist-info/licenses/LICENSE.txt) |
| starlette | 1.7.0 | BSD-3-Clause | [原文1](../Assets/ThirdPartyLicenses/starlette/starlette-1.7.0.dist-info/licenses/LICENSE.md.txt) |
| typing_extensions | 4.16.0 | PSF-2.0 | [原文1](../Assets/ThirdPartyLicenses/typing_extensions/typing_extensions-4.16.0.dist-info/licenses/LICENSE.txt) |
| typing-inspection | 0.4.4 | MIT | [原文1](../Assets/ThirdPartyLicenses/typing-inspection/typing_inspection-0.4.4.dist-info/licenses/LICENSE.txt) |
| uvicorn | 0.54.0 | BSD-3-Clause | [原文1](../Assets/ThirdPartyLicenses/uvicorn/uvicorn-0.54.0.dist-info/licenses/LICENSE.md.txt) |
| annotated-types | 0.8.0 | MIT | [原文1](../Assets/ThirdPartyLicenses/annotated-types/annotated_types-0.8.0.dist-info/licenses/LICENSE.txt) |
| attrs | 26.1.0 | MIT | [原文1](../Assets/ThirdPartyLicenses/attrs/attrs-26.1.0.dist-info/licenses/LICENSE.txt) |
| click | 8.5.0 | BSD-3-Clause | [原文1](../Assets/ThirdPartyLicenses/click/click-8.5.0.dist-info/licenses/LICENSE.txt.txt) |
| cryptography | 50.0.2 | Apache-2.0 OR BSD-3-Clause | [原文1](../Assets/ThirdPartyLicenses/cryptography/cryptography-50.0.2.dist-info/licenses/LICENSE.txt)、[原文2](../Assets/ThirdPartyLicenses/cryptography/cryptography-50.0.2.dist-info/licenses/LICENSE.APACHE.txt)、[原文3](../Assets/ThirdPartyLicenses/cryptography/cryptography-50.0.2.dist-info/licenses/LICENSE.BSD.txt) |
| h11 | 0.16.0 | MIT | [原文1](../Assets/ThirdPartyLicenses/h11/h11-0.16.0.dist-info/licenses/LICENSE.txt.txt) |
| idna | 3.20 | BSD-3-Clause | [原文1](../Assets/ThirdPartyLicenses/idna/idna-3.20.dist-info/licenses/LICENSE.md.txt) |
| jsonschema-specifications | 2025.9.1 | MIT | [原文1](../Assets/ThirdPartyLicenses/jsonschema-specifications/jsonschema_specifications-2025.9.1.dist-info/licenses/COPYING.txt) |
| referencing | 0.37.0 | MIT | [原文1](../Assets/ThirdPartyLicenses/referencing/referencing-0.37.0.dist-info/licenses/COPYING.txt) |
| rpds-py | 2026.9.1 | MIT | [原文1](../Assets/ThirdPartyLicenses/rpds-py/rpds_py-2026.9.1.dist-info/licenses/LICENSE.txt) |
| truststore | 0.10.4 | MIT | [原文1](../Assets/ThirdPartyLicenses/truststore/truststore-0.10.4.dist-info/licenses/LICENSE.txt) |
| cffi | 2.1.1 | MIT-0 | [原文1](../Assets/ThirdPartyLicenses/cffi/cffi-2.1.1.dist-info/licenses/LICENSE.txt) |
| pycparser | 3.11 | BSD-3-Clause | [原文1](../Assets/ThirdPartyLicenses/pycparser/pycparser-3.11.dist-info/licenses/LICENSE.txt) |

准确 wheel 哈希和许可文件路径见[发行依赖清单](https://github.com/XRH66/NodeForge/blob/v1.1.2/Tools/NodeForgeMcp/DependencyManifest.json)。离线原文位于 AI 包 Runtime/Lib/site-packages 的相应包或 .dist-info/licenses 目录。表内许可名保留原清单表示，具体条款及随附组件以原文为准。

## 设计与实现思路参考

以下项目用于体验、协议、流程或系统设计对照。根据现有来源账本，未将它们的源码复制到 v1.1.2，也未作为运行依赖交付；这些参考不改变上方实际采用代码的范围。

| 项目与研究版本 | 参考内容与实际边界 | 当时核对的许可 |
| --- | --- | --- |
| [GraphDeck Connector](https://www.graphdeck.app/connector) · UE5.4 / 1.1.0 | AI 到 UE 的图描述、部署入口与操作体验；定点阅读官方包，未复制代码、未安装其运行系统。 | 包内 All Rights Reserved；未取得复制授权依据。 |
| [Node-RED](https://github.com/node-red/node-red/tree/935b8d3e12ec8be064f3a72ac6547131f778a7b1) · `935b8d3e` | 导入文本/文件先填入并检查、再执行的流程与范围管理；没有搬入 JS/DOM 编辑器或冲突合并机制。 | Apache-2.0；仅流程参考。 |
| [ComfyUI](https://github.com/Comfy-Org/ComfyUI/tree/08ff3c11b3a85eb07c2c06f21cb378ac2e086840) / [ComfyUI_frontend](https://github.com/Comfy-Org/ComfyUI_frontend/tree/70c0a86139dfa96b51eadeafed43f60c49e7096d) | 工作流 JSON、版本与 Schema 组织。主仓只作产品/元数据入口；具体定点阅读在前端 schema，未引入 Vue、Zod 或其代码。 | 固定版本 GPL-3.0；未作为代码或依赖采用。 |
| [KantanDocGenPlugin](https://github.com/kamrann/KantanDocGenPlugin/tree/beec8013201160cb647912b3b62a0176e52e8a82) · `beec8013` | 节点发现、Spawner 与隐藏引脚筛选的研究对照；未复制其文档工具或实现。 | MPL-2.0。 |
| [Gahzi/BlueprintBridge](https://github.com/Gahzi/BlueprintBridge/tree/b1871db8a911e3ba2aeae2ef129edd56f315b6fc) · `b1871db8` | 命名管道长度帧、命令注册/发现与游戏线程边界；本项目使用自身有限等待、ACL和取消完成机制，未采用其服务器代码。 | MIT。 |
| [DeVoe09/UnrealMCP](https://github.com/DeVoe09/UnrealMCP/tree/4cafb4ebd9ee288cba0e9d6c814a6f1f2c804451) · `4cafb4eb` | 入队、Tick 和线程分派模式对照；未并入其 UE5.7 插件。 | MIT。 |
| [adamrehn/ue4cli](https://github.com/adamrehn/ue4cli/tree/fed71c1af4cffe3fed9358b08430068ad9454b77) · `fed71c1a` | 编辑器路径、参数和进程输出机制对照；继续使用 NodeForge 自有 CLI/guard，未采用该库。 | MIT。 |
| [giampaolo/psutil](https://github.com/giampaolo/psutil/tree/482856954e42496499730ded1b1b264039cf6afc) · `48285695` | PID 加创建时间的进程身份原则、查询权限与未知状态处理；使用标准库 ctypes 调 Windows API，未复制或安装 psutil。 | BSD-3-Clause。 |
| [db-lyon/ue-mcp](https://github.com/db-lyon/ue-mcp/tree/5c4e4b43a1e9d38220d96082130229036e0f9d91) · `5c4e4b43` | 普通节点通用创建、分类初始化和反射 schema 的对照研究；未移植其控制系统或 schema 解释器。 | MIT。 |

## 评估过但未采用的候选

以下条目保留研究来源与取舍，不能据此推断已集成、已复制代码或已在 UE5.3.2 验证。

| 候选与研究版本 | 评估方向及未采用边界 | 当时核对的许可 |
| --- | --- | --- |
| [GraphFormatter](https://github.com/howaajin/graphformatter/tree/c144d9f5bc2622541ac6f63ba3550e0346eca58c) · `c144d9f5` | 曾计划作为布局改进的优先候选；未进入当前产品，现有布局采用 Autonomix 片段。 | MIT。 |
| [NodeToCode](https://github.com/protospatial/NodeToCode/tree/cb1483a3c105c54f45535844be7cb47da1d482fe) · `cb1483a3` | 旧图节点/引脚读取与 processor 候选；未采用其 processor 或 LLM 网络模块。 | Apache-2.0。 |
| [dagre](https://github.com/dagrejs/dagre/tree/32e7f30f97f029fbfc89ce50e072d4fc16145838) · `32e7f30f` | 布局算法对照；未移植算法或新增 JS 执行器。 | MIT。 |
| [AutoSizeComments](https://github.com/AutoSizeComments/AutoSizeComments/tree/0f1ec9320ef0e8c92bf626a4bbc4351abdfdb797) · `0f1ec932` | 注释框边界与交互研究；未复制或集成插件。 | CC-BY-4.0，不能误标 MIT。 |
| [chongdashu/unreal-mcp](https://github.com/chongdashu/unreal-mcp/tree/4e5f00da50733190481311e254d16d137a84ef33) · `4e5f00da` | 蓝图调用/连接实现对照；当时 README 称 MIT，但固定树未找到完整 LICENSE，未采用代码。 | 复制许可依据未核足。 |
| [BlueprintAssistWiki](https://github.com/fpwong/BlueprintAssistWiki/tree/b597f8080febc9dead18c09548f6813dc2a97d07) · `b597f808` | 产品资料与问题跟踪入口；不是已取得的实现源码。 | 未取得实现源码的复制许可。 |
| [microsoft/wil](https://github.com/microsoft/wil/tree/f6f3069ab39fb19c1f89fa5f2d3aa8a1118ace43) · `f6f3069a` | Windows 句柄 RAII 对照；未引入或裁切该库。 | MIT。 |
| [jsonrpcx/json-rpc-cxx](https://github.com/jsonrpcx/json-rpc-cxx/tree/a0e195b575d62cb07016321ac9cd7e1b9e048fe5) · `a0e195b5` | JSON-RPC 分派与错误模型候选；现有 UE Json 足够，未引入该库或其 nlohmann/json 依赖。 | MIT。 |
| [Hal-cell/unreal-blueprint-mcp](https://github.com/Hal-cell/unreal-blueprint-mcp/tree/84ed184e8db3116e167dc6c3d2b1e9a1a52ddcd9) · `84ed184e` | 只读 README、许可与模块入口的辅助比较；没有完成节点实现审查，未采用。 | 根 MIT；不据此声称已核完全部实现。 |
| [BAIB / Blueprint AI Bridge](https://xrvp.io/tools/baib-blueprint-ai-bridge/) | 早期紧凑图描述和文本回导候选；未取得、读取或采用其源码，实际能力未在本项目验证。 | 未核实，不能当作已获授权代码来源。 |

上述来源按实际采用或研究时的固定版本记录。列为参考或未采用的项目，其源码未随 NodeForge 分发；表中的许可证信息也不表示这些项目已被集成。

## 协议资料、权利与核对范围

NodeForge 的实现还参考 [Unreal Engine 官方开发文档](https://dev.epicgames.com/documentation/en-us/unreal-engine)、[Windows 官方命名管道文档](https://learn.microsoft.com/en-us/windows/win32/ipc/named-pipes)、[JSON-RPC 2.0](https://www.jsonrpc.org/specification)、[MCP 规范](https://modelcontextprotocol.io/specification)与 [JSON Schema](https://json-schema.org/)。这些是 API/协议资料入口，不是新增打包组件。

各第三方许可适用于对应组件，版权归原作者；使用 Unreal Engine 仍需遵守 Epic 的许可要求。NodeForge 整体尚未选择统一开源许可证。本页说明已核对的来源、版本和采用范围，不提供对上游项目的全面审计、兼容性或安全保证。
