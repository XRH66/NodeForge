# 在两个 AI 对话之间交接连接

适用于 NodeForge 1.1.0 / Windows UE5.3.2，AI 桥0.4.3。原生DLL、材质契约、Catalog和Schema保持1.1.0身份；AI工具契约和资料摘要已更新。本版新增公开 `nodeforge_disconnect`，无需关闭UE。

## 原对话正常释放

先从本对话的原handle读取 `editor.getState`，并核对自己的Bridge调用记录。`pending`、`unknown`、`requiresReview`或未确认退出的CLI必须使用原callId的 `nodeforge_calls_get` 收尾；不换ID重发，不删除锁/调用记录。另一客户端连接失败时的 `unresolvedCalls:[]` 不是旧所有者的状态证明。

确认没有未解决调用后，调用：

```text
nodeforge_disconnect({"handle":"本对话connect返回的当前handle"})
```

只接收handle，没有 `force`、PID、工程路径或租约文件参数。成功返回 `status:"finished"`，以及 `released:true`、`leaseReleased:true`、`handleInvalidated:true`、`recordsPreserved:true`、`unresolvedCalls:[]` 和实际bridgePid/原公开identity。旧handle随即失效；原调用和报告保留。

该操作复用 `NodeForgeMcpBridge.Bridge.close()`。先检查原调用、连接/业务/CLI状态，再核对内存拥有标志、当前服务PID以及锁内完整pid+instance。仅正常移除完全匹配的自有租约，关闭自己的在线连接；不结束AI服务或UE。

有未解决调用时返回 `unknown / NFMCP_UNRESOLVED_CALL`，`released:false`，保留handle和租约，继续查询原callId。连接或CLI正在工作时返回 `NFMCP_BUSY`。非本客户端handle返回 `NFMCP_HANDLE`；不拥有租约或租约已变更分别返回 `NFMCP_NOT_LEASE_OWNER`、`NFMCP_LEASE_CHANGED`。失败不能当作交接完成。

## 新对话接手

新对话先 `nodeforge_docs_index`，核对实际bundle、build、AI桥版本和资料/工具摘要，读取Readme、AiQuickStart、FeatureMap、AiPreparation、SessionHandoff及需要的材质契约。随后显式：

```text
nodeforge_connect({"project":"C:/Projects/MyProject/MyProject.uproject"})
```

仅使用新对话自己收到的 `connected.handle`。核对规范project、实际module、binarySha256、pid和processStart；依次读取operations.describe、editor.getState、draft.get，再按本次draftRevision查询需要的六字段。旧handle不能传给新客户端代为释放。

新对话使用完，也要在没有未解决调用时调用disconnect，供后续对话接手。MCP服务和UE可以继续运行；在线心跳不占业务租约。若仍被占用，应让准确所有者核对原调用并正常释放，不能手动删除他人的Bridge.lock、强杀服务或绕过安装/工程/原生身份检查。

仅更新资料/工具代码时，既有稳定AI入口会验证完整绑定后正常刷新；新增工具通过tools/list发现。正常刷新也会调用旧所有者的close核心，未解决调用则阻止刷新。现场是否真的释放、新客户端是否真的连接，应分别留证，不能以配置文件更新代替。
