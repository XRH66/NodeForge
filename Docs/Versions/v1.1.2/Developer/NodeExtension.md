# 扩展节点与资产字段

先确认需求属于普通节点、特殊节点语义，还是新增资产字段。实现位置不同，但校验、生成、回读、导出和文档必须保持一致。

## 增加普通节点

1. 在 UE 5.3.2 的官方文档和源码中核对准确的类、函数和类型。
2. 查看 `NodeForgeNodeDefinitions.h/.cpp`，确认能否使用已有反射、Spawner 和属性处理逻辑。
3. 增加必要的节点定义或发现规则。函数必须使用准确的 `owner/member`，不能只按显示名称匹配。
4. 用原生构造检查真实引脚、默认值、类型和配置，再将节点加入可用目录。
5. 同步节点说明，给出用途、前提、限制、参数和有效 JSON 片段。
6. 按改动范围验证合法输入、应拒绝的输入，以及生成、编译、保存和重开结果。

`Pure` 或 `CPF_Edit` 本身不足以证明节点可用。未知类型、外部状态、Latent、CustomThunk、未支持的引用参数和动态资源依赖需要单独处理。

## 增加特殊语义

动态 MaterialFunction 签名、Sequence/Switch、成员变量和 Actor/RootComponent 上下文会影响多个环节。修改前检查解析、构图、回读、导出、来源校验和预览是否都能表达同一语义。

动态 GUID 和函数签名必须来自实际资源。不能用固定占位值开放节点，也不能只放宽白名单而缺少生成或导出支持。新增多图、循环、数组或父类支持时，应先定义完整行为和限制。

## 增加资产字段

在 `NodeForgeAssetDefinitions` 登记字段、类型、值范围和省略规则，再实现原生 `Apply`、`ReadActual`、导出和 codec。JSON Schema 和目录从这些定义生成，避免在 Python 层复制业务规则。

事件、函数、成员和材质函数绑定继续由 `NodeForgeNodeDefinitions` 管理。特殊字段不应混入普通 `properties`，以免绕过专门校验。

## 同步 API 与文档

普通节点扩展使用现有 `catalog_get` 和 `rules_query`，通常不需要新增 MCP 工具。只有增加真正的新操作时，才按 [扩展 API](OperationExtension.md)处理。

节点页以准确的 class、binding、variableType 和 configuration 标识节点。目录变动后需要刷新 `catalogDigest` 和对应文档；旧输入必须按新目录重新检查，不能仅替换摘要就视为兼容。只记录实际验证过的节点和组合，不由一个示例推断全部配置都可用。


## 1.1.2 清漆与折射契约

当前新增字段、根输入类型、切线空间和 CLI 进程查询见[完整契约](../Reference/LayeredMaterials.md)。新 Catalog/Schema 来自原生共享定义；旧输入需重新预检。
