# 版本身份与资料


## 当前个人候选状态

当前 {{bundleId}} / {{buildId}} / 桥 {{bridgeVersion}} / SDK2.3.0 / ABI1。
本次增加有界资源查询、Texture2D精确检查和显式工程/安装根绑定；工具数来自本包契约，目录为 {{materialCount}}材质类/{{blueprintCount}}蓝图签名。JSON2/3、Tick、设置和其它旧有限范围保持。

此版为PC2源码预期资料：新原生目录/操作在PC3同窗取得并准确核对，随后才执行新生成/Verify。编译和离线资料不是产品业务、安装、画面、打包或包后运行通过。Manifest.publicationStatus是当前准确发布层，最终证据由本轮终审更新；根Build2/RC1不升级。

安装与本机配置见[UserInstallation]({{page:UserInstallation}})，资源查询→检查→引用→调用/恢复的完整输入见[AiResources]({{page:AiResources}})。公开机器元数据与私有SourceInput已分离；全部文档/附件发布闭包仍待PC6/PC7内容和隐私终审，不能现在分发为最终候选。

## 以下保留既有阶段的历史记录

## 本版身份

| 项目 | 当前值 |
| --- | --- |
| 资料包 | {{bundleId}} |
| MCP 桥 | {{bridgeVersion}} |
| UE 核心 | {{buildId}} |
| 引擎 | {{engine}} |
| 目录适配器 | {{adapter}} |
| 图协议 | Graph v2 / v3，共同描述 |
| Editor API | {{editorApiVersion}} / {{profile}} |

Manifest 绑定每页与数据的 SHA 和准确来源；页面与节点数量来自实际输入。

## 按需要查阅

- **[发行说明]({{page:VersionReleaseNotes}})**
  本版新增、改变、保留的内容与对应证据。
- **[兼容、迁移与回退]({{page:VersionCompatibilityAndMigration}})**
  成套脚本、契约、配置和客户端的更新与恢复步骤。
- **[已知限制]({{page:VersionKnownIssues}})**
  区分未支持、实验支持和未实测的内容。

## 精确绑定

**DLL SHA：** `{{binarySha256}}`

**CatalogDigest：** `{{catalogDigest}}`

磁盘资料包、当前运行实例和已安装插件可能不同。客户端必须实际读取确认，文件写入不能被称为已经加载。


本包有限C业务的真实分层结果见发行说明，原报告/SHA在EvidenceIndex.finiteCopyPreviewRelease；旧B在finiteAssetReadRelease保留其原版本依据。Manifest.publicationStatus= frozen表示本资料包不再原位重建；它不替代运行/画面/打包或全产品验收。安装根与RC1仍是旧版，不从开发资料推定已部署。
