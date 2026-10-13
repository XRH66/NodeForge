# AI 安装与接入

## 配套版本

Build137 Repair5普通插件与`Build137Repair5Ai1`接入包共用同一核心，适用Windows x64、官方UE5.3.2。接入包内含Python3.12.4、MCP SDK2.3.0及29项固定依赖；不需要系统Python或预装MCP。服务桥沿用0.3.6，配套机器资料为Mcp10R1。Build136 Repair6是历史完整候选，不能混装。

接入包不包含另一份插件DLL。已有普通插件只补AI文件；未安装插件时，工具下载同Release的固定普通ZIP并核对哈希。私有仓库需要AI已能读取附件，或将`NODEFORGE_GITHUB_TOKEN`仅放在进程环境；不要在指令、回执或配置里填写令牌。也可通过`-PluginZip`指定已下载的准确普通ZIP。

## 交给有本机权限的AI

AI先读取该Release的`AiReleaseManifest.json`，核对两个附件及SHA256；将AI ZIP解压到工程插件目录之外。普通用户不需要下载此包。

AI需要文件读写、执行本机程序、读取下载地址的能力。只有MCP协议支持而没有这些权限的客户端，不能自行安装。工程位置由当前已知工程路径确定；传工程目录时，只允许目录下恰好一个`.uproject`。UE优先从已有插件绑定读取，否则解析工程`EngineAssociation`对应的真实Windows登记；无法唯一确定时需明确`-EngineRoot`。

在AI包的解压目录调用统一入口：

```powershell
& .\Scripts\InstallNodeForgeAi.ps1 -Project '实际工程.uproject绝对路径' -ClientConfig '实际客户端配置文件绝对路径' -ClientFormat 'codex-toml' -Connect
```

支持`codex-toml`和含`mcpServers`对象的`mcp-json`文件。工具核对文件格式并添加`nodeforge`服务，保留其他配置；已存在不同身份的同名项会拒绝，使用`-ServerName`明确另一名称，避免改错正在使用的服务。配置路径必须是该客户端实际读取且AI获授权写入的文件，不猜用户全局配置路径。JSON客户端具体配置位置及刷新方式仍由其客户端决定，不宣称已经自动适配所有产品。

工具校验包内所有文件与已装核心，复用原`PrepareNodeForgeInstallation.py`和`RunProcessGuard.py`完成工程绑定、UE版本/模块身份验证、固定依赖验证及注册准备，随后实际启动stdio服务发现工具并读取配套资料。结果为JSON，包含准确工程、安装目录、配置、回执、MCP启动参数与专用编辑器启动命令。

## 连接检查

安装不会自动关闭或替换已有UE进程；选定工程正在打开时先报告必须关闭。`-Connect`会启动本工具拥有的专用编辑器并自动运行只读连接检查，返回实际`connection`；不传此选项时只准备文件、配置与stdio发现。也可按回执返回的分离参数启动专用编辑器，再执行已安装包内的：

```powershell
& '插件目录\Tools\NodeForgeMcp\Runtime\python.exe' '插件目录\Scripts\NodeForgeAiCheck.py' --config '回执中的config绝对路径' --connect
```

检查通过才返回`connected`：实际握手、发现20项工具、读取同版资料、原CLI/native身份认证与`editor.getState`只读结果，同时回读真实PID、工程和已加载DLL。不生成或修改资产。客户端配置已落盘不等于客户端已刷新；按客户端自身刷新能力载入后仍应实际发现工具，不能把这一步描述为所有客户端都已自动完成。

专用编辑器结束时使用回执中的原`editor-commands`生成一次停止命令；确认控制通道结束，再用原`close-editor`正常关闭。正常结束后再次调用统一安装入口，会产生新的注册和`Config.json`并更新固定启动入口；不要复用历史PID或`SessionPublic.json`。原会话仍有私有描述符或无法确认时停止，保留原回执。AI运行权限来自使用者任务，安装不自动发起模型业务。

重复补装只复用哈希相同的AI文件与绑定，不覆盖不一致文件。失败回执保留已写对象与原安装回执；有部分写入时先读回执处理，不删除工程、不强制杀进程。

## 下载可见性与证据

GitHub仓库仍私有，文档网站仍仅所有者可访问。配套包可内部交付；网站面对外部AI的公开安装指令仍须等公开指南和下载地址实际可读。本轮不改变权限。

安装验证只覆盖同核心接入；历史产品/节点能力证据保留原Build136身份，不转写成Build137全部业务回归。

运行环境采用[Python官方嵌入发行包](https://www.python.org/downloads/release/python-3124/)；独立`._pth`按[Python嵌入运行说明](https://docs.python.org/3.12/using/windows.html#the-embeddable-package)固定模块搜索路径。Codex配置字段依据[官方MCP配置说明](https://developers.openai.com/codex/mcp/)；具体客户端载入结果须实读核对。
