# NodeForge 文档主题维护

本目录是离线文档站点的共享展示层。用户于 2026-10-11 要求暗色使用“朱砂中文”、亮色使用“朱砂英文”，复用已有品牌资产，并让后续内容和页面扩展不依赖逐页改 HTML。正文、版本/业务事实继续由原 Markdown、authoring 页面清单和发布器维护。

## 文件职责

| 文件 | 修改场景 |
| --- | --- |
| [Config.json](Config.json) | 默认主题、站点名、品牌资源映射、分类、常用章节顺序和短导航标题。注册到页面清单但未手动固定的同分类一级页面，会自动追加到侧栏；节点明细不会挤满侧栏，仍进入搜索。 |
| [Theme.css](Theme.css) | 两套颜色变量、字体、间距、正文/目录/参数/表格/代码、响应式断点、打印与减少动画偏好。颜色集中在文件首部，避免每页维护。 |
| [Theme.js](Theme.js) | 主题按钮和记忆、跨本地 HTML 链接传递主题、搜索、手机导航、目录跟随、参数与代码复制；所有增强都基于现有正文元素。 |
| [Renderer.mjs](Renderer.mjs) | 页面框架、导航、资源拷贝、搜索索引、相邻章节和元信息；不定义产品文档正文。 |
| [Brand](Brand/) | 已有品牌素材的网页适配副本。SVG 只裁减画布留白，暗色中性墨色适配浅色；字形路径、比例、变换及朱砂色保持。Logo PNG 为原件字节副本。 |

原始品牌包保持在 [BrandAssets](../../../SourceData/BrandAssets/Readme.md)。本次适配来源、哈希和边界见[BrandAdaptation](../../../Reports/20261011BrandDocsDesign/BrandAdaptation.json)，没有另画 Logo 或字形。

## 修改内容与重新生成

1. 正式内容先改对应 authoring Markdown；新增文档在现有发布输入/页面清单中登记 `pageId`、输出路径和标题，沿原发布器生成候选。正文无需编写导航、主题或页面 HTML。
2. 需要固定导航位置或更短标签时，改 `Config.json` 对应 `menu`；只新增普通同分类页面时，可使用自动追加。标题、章节和节点索引会随本次渲染重建搜索数据。
3. 已有未冻结候选的展示层更新，可在项目根目录执行：

```powershell
node Scripts/RenderNodeForgeDocs.mjs --bundle Docs/Versions/Build122/Mcp2R1
```

该入口读取候选 Manifest 的页面清单和当前 Markdown，生成 HTML、`Assets/NodeForgeDocs` 与 `LayoutManifest.json`，保留 Markdown 和业务数据。页面标题取当前 Markdown 的一级标题。它拒绝修改 `publicationStatus=frozen` 的资料包；已冻结版本必须按原发布流程创建新候选。

原发布器的 stdin JSON 调用方式仍保留。它会自动调用相同主题，输出同样的资源和搜索索引；不需要逐页拷贝 CSS/JS。没有联网字体、框架、CDN、服务或额外安装步骤。

## 发布清单与合并

`LayoutManifest.json`记录本次展示层的源码、所有 Markdown、HTML 和资源 SHA256，并明确它不代表发布冻结或浏览器验收。渲染器 stdout 的 `layoutManifest` 返回该文件的指纹，`assets` 返回资源清单。

最终发布/冻结时，负责发布的任务应把本目录的 `Config.json`、`Theme.css`、`Theme.js`、`Renderer.mjs` 和三份 `Brand` 资源加入版本来源绑定，并将 `LayoutManifest.json` 与六份输出资源纳入产物检查；重新计算 HTML 指纹。不要仅沿用更新前 Manifest 的旧 `htmlArtifacts` 或把当前展示层记录当完整业务验收。本次按已确认分工没有改动另一任务正在维护的发布器、ReleaseInput 或业务 Manifest。

更新主题后先生成候选，再检查主要指南、长表格、代码/节点页，以及暗色/亮色、手机宽度、键盘导航和打印。通过后按原发布流程处理新版本。本轮浏览器 `file://` 访问受工具策略阻止；静态检查与品牌 SVG 渲染通过，真实浏览器的两主题/宽窄屏/交互验收尚待完成。

## 交互与兼容约定

- 默认暗色，可在右上切换亮色。优先读 URL 中的 `theme=dark` / `theme=light`，否则读本地保存值；站内 HTML 链接携带当前主题，以适配浏览器可能隔离本地文件存储的情况。
- 页面内容和普通链接由 HTML 直接提供；JavaScript 用于交互增强。禁用 JavaScript 时仍可阅读正文和站内链接。
- 搜索索引为本地 JS 数据，覆盖当前页面清单中的标题、章节和节点名称。`Ctrl+K`（macOS 也支持 `⌘+K`）或 `/` 打开，方向键选择、Enter 打开、Esc 关闭。
- 连续至少三组“粗体字段名＋值”的原始段落会增强为参数区。值从原 DOM 移动，不另行拼写路径；代码块和参数复制只在用户点击时执行。浏览器拒绝剪贴板时显示手动复制提示。
- 850px 以下采用抽屉导航，1100px 以下收起右侧目录；长表格只在自身容器横向滚动。断点、最小屏幕表现尚须实际浏览器复核。
