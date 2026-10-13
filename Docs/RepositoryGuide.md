# 源码仓库开发说明

当前native源码对应Build137 Repair5，AI1接入包使用相同核心。根Tools/NodeForgeMcp/ToolContract仍保留原136开发服务绑定，保护现有服务；发布契约位于Releases/Build137Repair5/Ai1/ToolContract.json，由PackageNodeForgeAi工具放入接入包的准确位置。不要直接拿根开发配置混装137。

插件Editor模块在 `Plugins/NodeForge/Source/NodeForgeEditor/`，可放入自己的官方UE5.3.2开发工程编译。新的编译DLL需要准确安装清单与资料，不能套旧哈希。已验137发行物清单保存在 `Releases/Build137Repair5/`。

原 `BuildPrototype.ps1` 使用本机引擎与 `DevProject/NodeForgeDev.uproject`；历史测试工具也有专用工程/报告/权限前提。验证工程不随仓库分发，克隆后不能直接重现全部历史流程。插件Readme中的旧Build/PID/报告为本机历史，实际使用以根README及当前Release为准。

CLI、Transport、RunProcessGuard和native Service复用同核心。PrepareNodeForgeInstallation验证安装/环境并生成准备回执和客户端参数。Tools/NodeForgeMcp保存锁定依赖、来源与许可，没有提交venv、系统Python、wheel缓存、客户端配置或会话。

Docs/PluginControl/DocumentationSources保存正文；Tools/NodeForgeDocs保存渲染/主题。Build135 Reference的六份文档为现有编写依赖，按原路径保留。保留136/R13实际AI资料与R19阅读资料；新核心需要新资料身份，不修改旧Manifest冒充新验收。历史发布输入留本机，从克隆仓库重建所有历史冻结站点不是本次承诺。

托管站点后续展示更新的源码在HeroPreview/Runtime及BuildTheme.mjs。生成入口为 `node Tools/NodeForgeDocs/HeroPreview/BuildTheme.mjs --output <目标目录>`。Sites内嵌Git仓库留本机，不作为本仓库子模块。

Tests、native Checks和JSON保留；实际UE验证按目标创建/选择本机专用工程，保护原资产，不复制私人绑定。本次只核上传文件、源码/发行物对应、ZIP/哈希、私有配置排除及远端提交/附件，不重跑既有产品整批。

AI包通过`PackageNodeForgeAi.py`显式组装到新目录；依赖wheel仅从本机按锁定清单校验后离线安装，运行时使用固定官方嵌入ZIP。克隆仓库不包含wheel和运行时，普通/AI使用从Release取包。固定Python官方输入URL/SHA及每个运行文件见AI1的RuntimeManifest；Source/AI包/普通包身份分开记录，旧核心Release标签保持原提交。

AI1新增统一安装、固定环境、配置和只读连接检查；有限验收包括无系统Python路径下的全新安装、普通后补装、版本/客户端冲突拒绝与正常关闭，不重跑旧产品业务整批。公开引导的待办见[Release方案](GitHubReleaseDeliveryPlan.md)。
