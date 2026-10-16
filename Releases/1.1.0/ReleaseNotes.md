## 下载

**Windows 64 位 · 官方 Unreal Engine 5.3.2**

| 文件 | 用途 |
| --- | --- |
| [NodeForge 插件](https://github.com/XRH66/NodeForge/releases/download/v1.1.0/NodeForge-1.1.0-UE5.3.2-Windows.zip) | UE 插件，无需 Python 或 MCP |
| [NodeForge AI 接入](https://github.com/XRH66/NodeForge/releases/download/v1.1.0/NodeForge-AI-1.1.0-Windows.zip) | 可选，包含连接服务、固定运行环境和配置工具 |

两个包均为 **v1.1.0**，请配套使用。

## 新增

- 支持清漆着色模型、独立清漆粗糙度、各向异性、切线输入和 IOR 折射，贯通生成、读取与导出。
- 新增 `nodeforge_disconnect`，AI 可以正常释放自己的连接，供另一对话接手，无需关闭 UE。

## 修复

- 统一检查与生成对相同输入的判定，正确返回被拒绝的请求。
- 避免复杂材质共享节点在布局时反复递归。
- 修复多个 AI 客户端在线检测占用连接容量、以及代码刷新后的异常处理。
- 改进 Windows 工程进程识别，避免进程查询长时间等待或误判目标工程。
- 修复重新登录 Windows 后旧会话目录权限导致的连接失败。

## 限制

仅支持 Windows 64 位和官方 UE5.3.2。节点目录为实验支持，不支持任意节点或所有组合。其他平台和其他客户端的专有在线检测尚未支持。

<details>
<summary>附件说明</summary>

- `ReleaseManifest.json`：版本、适用环境和配套下载信息。
- `Checksums.sha256`：下载文件的校验值。
- `Source code`：GitHub 自动生成的源码归档，安装请选择插件 ZIP。

</details>
