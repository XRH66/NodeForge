# NodeForge 仓库与 Release 交付方案

2026-10-13，用户已确定一套插件核心、两个安装入口、同仓库同Release。统一身份为Build137 Repair5普通插件与可选Build137Repair5Ai1；AI包已实施并通过有限安装/连接验收。对外公开引导仍待开放。本页最新状态取代首次上传时的AI未交付状态，历史清单保留原时点。

## 统一兼容基线

| 字段 | 当前准确值或状态 |
| --- | --- |
| 仓库/标签 | XRH66/NodeForge，当前私有；`build137-repair5`普通插件预发布候选 |
| 核心 | Build137 Repair5，DLL SHA256 `88f805cd0461138570c00b653982c423ace693f380717b97f5c7c20b64b38b80` |
| 引擎/平台 | Windows x64，官方UE5.3.2；安装验证核对实际引擎及模块BuildId，不混用定制引擎 |
| 安装 | Schema1，`projectPlugin`，工程 `Plugins/NodeForge` |
| 接口来源 | 现有ABI1/API1.0、Graph JSON2/3；接口资格不替代137 AI整链验收 |
| 目录 | digest `be97bf57d226778102038c261c556b89b262197c78158fe16ca293de709d8847`，71材质类、327蓝图签名，experimental |
| AI版本 | Build137Repair5Ai1，沿用桥0.3.6/SDK2.3.0/ABI1，自带Windows x64 Python3.12.4及29项锁定依赖 |
| 137 AI附件 | `NodeForgeBuild137Repair5Ai1.zip`，可新装/补装；MCP JSON与Codex TOML配置，真实20工具发现及只读连接通过 |
| 文档 | 配套Mcp10R1/452页；内容继承Mcp9R19，清单与目录绑定137 DLL；136原R13/R19不改身份 |

准确机器字段见[兼容基线](../Releases/Build137Repair5/CompatibilityBaseline.json)。当前使用准确Build标签，保留 `.uplugin` 原 `0.1.0-prototype` 与原冻结包，不为上传更换已验DLL或虚构统一语义版本。

普通ZIP的83个native源码和插件描述与当前工作区逐字一致。136/137的CLI与Transport未变，137安装准备和进程守卫支持工程Plugins布局。现有 `editor_preparation` 要求产品、契约、资料和DLL完全同源；AI1重新绑定137，提供真实固定运行时。已验证同核心安装/SDK只读连接；产品业务/画面/打包的136历史证据不转写成137全量回归。

## 两个入口与同一核心

普通用户下载插件ZIP，复制到选定工程，首次页面识别实际工程/引擎/插件并确认绑定，随后进入原功能页；无需Python/MCP。

具备本机权限的AI按[安装指南](AiInstallation.md)读取当前清单、下载并校验AI ZIP，调用`InstallNodeForgeAi.ps1`：识别确定工程与UE，核已有插件或下载同核心普通ZIP，部署固定运行时和服务，写明确客户端文件；`-Connect`实际发现工具、读取同版资料并连接准确工程。网站公开指南/可复制入口仍待开放。

统一工具复用 `PrepareNodeForgeInstallation.py`、`RunProcessGuard.py` 和native安装验证。已实现同Release普通附件下载/校验、单一工程发现、已有绑定/真实EngineAssociation登记识别、固定环境部署、MCP JSON/Codex TOML添加配置与真实SDK连接检查。配置保留其它服务/字段；同名不同条目拒绝。当前明确客户端文件写入已验证；客户端运行中的热刷新及所有产品自动发现配置路径不属于已验能力。

客户端支持MCP不代表允许本机执行、下载或写配置。缺权限、工程不明确或客户端没有受支持入口时，返回准确缺项/需用户操作结果，保护原配置并保存实际写入回执。

AI补装围绕普通插件根的Scripts/Tools/Docs及固定Runtime目录组织，保留同一安装根和绑定；匹配时只增加AI部分，复用原DLL，不建立第二个NodeForge副本。普通包现有四个高级CLI/验证脚本无需普通用户执行；补装的同名脚本必须与该Release声明的哈希一致，不能用136守卫覆盖137。实现阶段须验证上述布局在原守卫下真实成立。

## Release 与源码内容

| 附件 | 本次处理与后续设计 |
| --- | --- |
| 普通插件ZIP | 上传137 Repair5原准确ZIP，保留字节/哈希；手动与AI下载安装最终均使用这一核心 |
| 可选AI接入ZIP | 提供Ai1，包含连接服务、固定Python3.12.4运行时、锁定依赖、配置工具及Mcp10R1资料，不含第二份插件核心 |
| ReleaseManifest与Checksums | 原普通清单保留首次发布事实；当前完整配套关系由新增AiReleaseManifest/AiChecksums记录，原标签与ZIP不变 |
| 安装与版本说明 | 提供当前真实安装步骤，网站后续链接同Release附件与公开指南，展示完整版本配套 |

如果实现AI接入需要更改核心DLL，则同批生成新Build/Release和普通插件ZIP，不能继续沿用137标签却换核心。核心不变时可提供明确AI组件修订，保留原附件与历史验收，记录组件修订及新联调证据。

Git保存源码、脚本、协议/目录、Tests/JSON、固定依赖清单与许可、正文来源、必要版本资料和发布身份。DLL、固定运行时和wheel放Release附件，不放源码提交。整个ValidationProject排除；开发宿主、历史Builds/Deliverables、Reports副本/会话、缓存、原始输入、上游整仓快照及本机管理日志留磁盘。GitHub产品入口是README和本方案。

保留Mcp9R13用于原136开发服务契约/资料依赖，Mcp9R19保留阅读基线，新增137/Mcp10R1配套资料；另保留六份Build135 Reference编写来源。历史脚本带本机工程/报告条件，克隆后不承诺自动重现全部历史阶段。旧冻结资料不改字节或DLL身份。

## 已通过的AI有限门槛与公开入口待办

1. 核心、UE/平台/DLL/Schema、桥/ABI、SDK/运行时、契约、目录与资料准确绑定137，普通ZIP保持。
2. 真实嵌入Python/29项依赖；本机移除系统Python路径并指定无效PYTHONHOME/PYTHONPATH，独立运行、新装下载和真实连接通过。第二台干净Windows机器未测。
3. 全新安装及普通后补AI通过；保留同DLL、绑定和其他客户端配置。已有会话结束后重新准备，固定启动器指向新注册；重复已准备安装不复制文件。
4. 真实SDK发现20工具、读取配套页、认证准确工程/PID/DLL、只读状态通过；正常停止/关闭通过。Codex原生app-server实际发现20工具，但当前Work对话不在该后端，未声称该对话完成原生Codex工具调用。其它客户端界面刷新未验。
5. 错核心、运行工程、同名配置冲突拒绝；包文件/ZIP CRC/452页/契约哈希通过。沿用原守卫的UE/模块及重复插件限制，未重跑未改整批。没有生成资产。
6. 公开指南/下载实际可读与网站可复制指令仍为待办；此内部交付不改变可见性。

仓库当前私有、站点owner-only，内部发布不能称为外部用户已可安装。仓库/站点可见性或另一个公开指南需要相应对外发布任务。自动下载安装/固定环境/两个已声明配置格式已实施；任意客户端适配、运行中的配置刷新、其它平台/UE未覆盖。
