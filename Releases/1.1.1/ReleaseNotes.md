## 下载

**Windows 64 位 · 官方 Unreal Engine 5.3.2**

| 文件 | 用途 |
| --- | --- |
| [NodeForge 插件](https://github.com/XRH66/NodeForge/releases/download/v1.1.1/NodeForge-1.1.1-UE5.3.2-Windows.zip) | 普通 UE 插件，无需 Python |
| [NodeForge AI 接入](https://github.com/XRH66/NodeForge/releases/download/v1.1.1/NodeForge-AI-1.1.1-Windows.zip) | 可选，包含匹配的连接服务与固定运行环境 |

两个包均为 v1.1.1，请配套使用。

## 修复

- 新增 `calls.releaseCompleted` / `nodeforge_calls_releaseCompleted`，可在达到 128 条容量上限时回收已完成调用，并继续批量业务。
- 保留原输入、调用与报告；回收后的原 callId 仍可读取，重复提交继续核对身份和参数，未解决调用阻止回收。
- `editor.getState.callCapacity` 返回实际驻留、可用和可回收数量。
- 修正大型工具契约文件使用声明的文件解析上限；通信帧上限、身份、权限和租约保护保持。

## 使用

每 50 次业务调用主动清理并核对结果后继续。载入和预检分别计数；本版没有加入自动清理。pending/unknown 查询原 callId，不更换编号重发。插件与可选 AI 包必须同版，升级需要保存并正常关闭使用该插件的编辑器。

## 限制

仅 Windows 64 位、官方 UE5.3.2。节点目录仍为实验支持。接入向导草稿、其它平台、新节点及自动回收策略未纳入本版。

<details>
<summary>附件说明</summary>

- `ReleaseManifest.json`：适用环境、身份和配套附件。
- `Checksums.sha256`：下载校验值。
- `Source code`：源码归档，安装请选插件 ZIP。

</details>
