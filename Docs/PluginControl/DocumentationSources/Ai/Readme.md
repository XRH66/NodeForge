# AI 使用入口

已有资源的准确读取、函数签名、两族完整导出及带来源的新副本，先读[显式读取配方]({{page:AiAssetRead}})。工具帮助也直接导航pageId=AiAssetRead；普通从零构图继续沿QuickStart/TaskRecipes，不把单节点模板当作完整Graph。

从工具描述调用 nodeforge_docs_index，确认 {{bundleId}}、桥 {{bridgeVersion}} 和 Manifest SHA。所有下列资料都有 docs_read 的 pageId；文件链接供本地阅读，不能替代正式入口的版本确认。

发现索引提供版本、能力数量、读者入口和指南导航，最多32768字节，不把整包源码/维护证据清单塞进每次发现结果。完整知识分别通过下列pageId读取；来源与许可见SourcesAndLicenses，源码地图见DeveloperSourceMap，本机完整Manifest保留全部绑定。resultTooLarge表示没有返回完整发现摘要，不把它当正常passed或截断内容。

新资源选用先读[AiResources]({{page:AiResources}})：有界元数据查询、准确inspect、完整2/3引用输入、原正式调用与恢复。列表不建立来源或导出证明。

## 调用前阅读

- **[调用流程]({{page:AiQuickStart}})**
  pageId：AiQuickStart。理解正式工具、草稿修订和调用顺序。
- **[准备工程与身份]({{page:AiPreparation}})**
  pageId：AiPreparation。取得真实会话、批次、目标和独立 Verify 条件。

## 请求与结果

- **[任务与调用例子]({{page:AiTaskRecipes}})**
  pageId：AiTaskRecipes。沿真实参数完成既有任务。
- **[合法 JSON 样例]({{page:Examples}})**
  pageId：Examples。复用现有合法图结构，另分配实际身份。
- **[API 参考]({{page:AiApiReference}})**
  pageId：AiApiReference。查本版输入、返回和分支定义。
- **[结果与恢复]({{page:AiResultsAndRecovery}})**
  pageId：AiResultsAndRecovery。区分保存、Verify、pending 和 unknown。

## 支持范围

先按功能矩阵和族限制选择图结构，再通过精确身份节点页查引脚与属性。没有本机权限时，把人工准备步骤交给操作者，不臆测已经准备。

- [功能与限制]({{page:FeatureMap}})
- [材质节点索引]({{page:MaterialNodeIndex}})
- [蓝图节点索引]({{page:BlueprintNodeIndex}})

可直接使用交付的合法样例。构造新图属于 AI 自身能力，软件没有承诺任意复杂图。维护原理见 DeveloperReadme，发布身份见 VersionReadme。阅读资料不授予新的写入权限。


有限B已有正式证据：两项读取、两族导出、原完整payload与来源绑定、各一件副本及同stage新PIDVerify。先读[两族原结果样例]({{page:AiAssetReadExamples}})，它提供可理解的完整JSON和真实结果关系，历史身份不可重放；新任务仍从AiPreparation获取本次实际分配和修订。


- [已有图编辑、新副本准备与差异闭环]({{page:AiCopyPreview}})：来源载入/显式编辑/真实新身份/完整差异/生成与独立核验。
