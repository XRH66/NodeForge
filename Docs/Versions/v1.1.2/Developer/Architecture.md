# 架构与状态

NodeForge 的主要工作分为三部分：解析 JSON、检查图和依赖、在 UE 中创建并保存资产。界面与自动化接口共用这套业务逻辑。

## 调用链

AI 客户端通过 MCP 调用 Python 连接服务。连接服务检查工程、版本和会话，再调用 `NodeForgeControl.py`。CLI 通过编辑器控制通道转交操作，C++ 共享服务执行草稿编辑、规则查询、资源读取或图生成。

编辑器面板直接调用同一个共享服务。需要独立启动进程的操作由 Commandlet 执行。MCP 层负责参数和结果传递，不自行构建图，也不为每个节点实现一套处理逻辑。

## JSON 与节点定义

`NodeForgeGraphCodec` 负责 v2/v3 的解析与序列化，`NodeForgeGraphDocument` 保存统一的图模型。材质、蓝图和材质实例使用这个模型；较早的 v1 格式有独立解析路径。

`NodeForgeAssetDefinitions` 定义资产字段、类型、值范围以及字段出现和省略的规则。`NodeForgeNodeDefinitions` 定义节点、函数、事件、成员类型和允许的配置。生成、校验、导出和回读都消费同一份定义。

普通节点优先使用 UE 反射和原生 Spawner。动态函数签名、Sequence、Switch、成员变量和 SceneComponent 上下文需要额外校验。节点目录描述插件允许表达的范围，并不意味着 UE 的所有节点或组合都可用。目录在 Game Thread 初始化后按当前 DLL 缓存；发现节点和验证其实际构造是两个步骤。

## 草稿、调用与资产

| 字段 | 标识什么 |
| --- | --- |
| MCP 请求 ID | 客户端与连接服务之间的一次协议请求 |
| `handle`、`controlSession` | 当前编辑器连接 |
| `callId` | 一次业务操作，查询结果时继续使用原 ID |
| `draftRevision` | 当前草稿修订，必须使用服务返回的十进制字符串 |
| `requestId`、`assetPath` | 生成请求和资产目标 |
| `reportId` | 当前会话登记的报告及其文件指纹 |

每次实际修改草稿都会增加修订号，使旧预检和预览失效。业务在 Game Thread 串行执行，忙碌时返回 `busy`。连接服务先记录调用，再提交执行；结果为 `pending` 或 `unknown` 时会停止新的写入。

生成只创建新资产，已占用的目标或请求会被拒绝。保存后的资产还可以通过独立进程执行 `graphs.verifySaved` 检查重开结果。运行效果、材质画面和打包需要各自验证。

## 读取与来源保护

`NodeForgeResourceSource` 检查精确对象路径、原生类型、注册表和保存状态。读取时保护源文件，并记录文件 SHA。已加载对象如果缺少可确认的保存文件来源，返回 `sourceMemoryUnproven`；插件不会自动重载、保存或编译源资产。

`resources.inspect` 与 `graphs.export` 返回结果和报告，不自动替换 API 调用方的草稿。界面在执行相同读取后，还会检查操作归属与草稿修订，再按按钮行为载入结果。

导出报告同时包含完整对象、`exportedRequestText` 和来源指纹。`report_read` 的 `/exportedRequest` 返回完整 `text` 与 `sourceRef`。草稿初次载入时必须校验原报告、源文件和导出内容；后续显式设置 `retainExportOrigin:true` 才保留来源关系进行编辑。预检和生成会再次检查来源。

## 副本与预览

`RetainOriginEditCore` 处理保留来源的草稿编辑。`PrepareSnapshot` 调用共享的 `PrepareNewCopy` 逻辑，产生新的请求和目标，并提交派生草稿。`PreviewSnapshot` 使用当前来源和 `GraphDiff` 比较差异。

准备记录已写出不等于草稿提交成功，需要检查 `draftApplied` 和修订号。差异预览不修改资产，也不预留名称。失败或结果不明确时，查询原调用与当前草稿，详见 [结果与恢复](../Ai/ResultsAndRecovery.md)。

## 安装与会话生命周期

`NodeForgeInstallation` 统一判断引擎直接、Marketplace 和工程三种布局，核实际 DLL、UE 补丁与 BuildId；存在同名副本时拒绝。程序目录只读；工程绑定在 `Config/NodeForgeInstallation`，运行记录在工程 `Saved/NodeForge`，用户安装启用信息和发现索引在用户目录 `NodeForge`。私有控制会话保持当前用户/登录会话的原 ACL，不在发现索引复制凭据。

一次 AI 安装配置指向同一插件的固定环境和启动入口。原生 `OnReady` 触发原控制通道，普通启动登记新工程会话；面板连接/停止复用此核心。MCP 启动时可以零工程，按明确工程/唯一候选连接，多候选返回选择；每个句柄固定工程与会话。重开不复用旧句柄，未知写入保留原归属与查询路径并停止依赖写入。旧显式准备器保留高级用途，不是日常启动前提。

## 连接服务与文档

Python 连接服务保留 CLI 和原生返回，不把所有状态合并成一个“成功”。报告只允许从当前会话已登记的文件读取，超过单次大小限制会返回导航信息。

文档和工具由固定配置中的 SHA 绑定。仅更新已批准的文档时可以刷新资料；执行代码、SDK、工程或业务定义变化需要同步更新相应配置。兼容的应用代码可在同一 SDK 进程重新载入，不兼容的 SDK、解释器或连接核心需要重启服务。刷新不会清除未完成调用或重复执行业务。

## 原生界面与客户端身份

`NodeForgePanel.cpp` 用 Slate/FAppStyle 组织主面板与次级窗口，按钮调用原共享服务；`NodeForgePanelPreferences` 管理每工程设置草稿及应用，`NodeForgeClientIcons` 有界解码并保存受控图标。空初态来自空草稿和空 ResultText，用户已有配置不清空。

控制通道按认证客户端的身份报告在线，presence 不占业务独占槽；实际执行仍经过原业务门槛。Codex SDK client_info 进入原 Hello 身份链，定向断开保留其它连接。Claude/DeepSeek 的 UI/config 框架与检测能力分开，未实现检测时返回未检测。弱引用回调避免窗口销毁后访问，选择不被后台回调抢占。
