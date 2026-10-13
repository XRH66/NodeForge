# 文档网站主题

本目录提供 NodeForge 文档网站使用的首页、阅读布局和交互功能。页面正文来自版本目录中的 Markdown，品牌图片位于 [Brand](Brand/)。

## 生成页面

在项目根目录运行：

```text
node Scripts/RenderNodeForgeDocsHero.mjs --bundle Docs/Versions/v0.1.0 --output Deliverables/WebsiteContent1/Candidate1/Rendered
python Scripts/PackageNodeForgeDocsWeb.py --bundle Deliverables/WebsiteContent1/Candidate1/Rendered --output Deliverables/WebsiteContent1/Candidate1/Web
```

渲染器读取来源的 `Manifest.json`，核对 Markdown 与公开 JSON 的 SHA，在一个未占用的候选目录生成 HTML、搜索索引及 `Assets/NodeForgeDocs`。released/frozen 来源不能原位生成。下一次更新请使用新的候选目录名；生成网站不会自动部署。

导出器只收录清单声明的页面、资源和公开 JSON；外部工程附件会明确报错，不再沿链接复制内部记录。仅部署 `Web/Site`，维护源码、清单与生成记录留在本地。`LayoutManifest.json` 保存来源SHA与pageId/URL映射，`PackageManifest.json` 保存实际输出清单；时间戳单独记录。

只更新已有网站的共享脚本和样式时，可运行：

```text
node Tools/NodeForgeDocs/HeroPreview/BuildTheme.mjs --output <网站资源目录>
```

## 修改什么文件

| 文件 | 用途 |
| --- | --- |
| `Config.json` | 网站名称、导航、首页文字和阅读外观参数 |
| `Renderer.mjs` | 页面框架、导航、资源和搜索索引 |
| `Home.mjs` | 首页结构 |
| `Reading.mjs`、`Reading.css` | 正文和两侧导航的布局 |
| `Blueprint.js`、`Blueprint.css` | 背景节点图动画 |
| `Runtime/` | 主题、正文、导航、搜索和滚动条交互 |
| `BuildTheme.mjs` | 将运行时代码组装为网站使用的 `Theme.js` 与 `Theme.css` |
| `Brand/` | LOGO 与深浅主题字标 |

修改运行时代码后，通过构建工具更新网站资源。请勿直接修改生成的 `Theme.js`。页面依赖的字体、脚本和图片均来自本地文件，无需 CDN。

## 外观与交互

首页使用动态节点背景；内页包含正文、章节导航和目录。窄屏会折叠导航。深浅主题由页面按钮切换，并保存选择。搜索、代码复制和滚动条由 JavaScript 提供，普通正文与链接可直接阅读。

当前阅读外观参数集中在 `Config.json` 的 `readingAppearance`。背景动画会在页面不可见或用户启用减少动画时暂停。调整样式后，应查看首页、长表格、代码页、深浅主题和窄屏阅读效果。

主题选择在点击时立即同步页面地址、站内链接和本机记忆，跨页与刷新使用同一个选择。桌面和触摸共用 ThemeController；视口变化或再次操作会取消旧圆形裁切。原生 View Transition 捕获根页面时会暂停内容命中测试，此时的点击在结束快照后交给同一实际控件，不另造手机实现。相关回归为 Tests/NodeForgeDocsThemeChecks.mjs，运行时检查会一并执行；Pico 控制器与 CSS 保持。

GitHub 项目入口统一由 Config.json 的 projectUrl 管理，桌面和手机导航共用；仓库权限由 GitHub 独立控制。

## 生成记录

`LayoutManifest.json` 保存输入、网页和资源文件的哈希，供发布时核对。它只记录生成结果；网站是否能够正常访问和交互，需要在浏览器中确认。
