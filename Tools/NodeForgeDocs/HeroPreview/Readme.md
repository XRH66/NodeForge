# 文档网站主题

本目录提供 NodeForge 文档网站使用的首页、阅读布局和交互功能。页面正文来自版本目录中的 Markdown，品牌图片位于 [Brand](Brand/)。

## 生成页面

在项目根目录运行：

```text
node Scripts/RenderNodeForgeDocsHero.mjs --bundle Docs/Versions/v0.1.0
```

渲染器读取该目录的 `Manifest.json` 和 Markdown，在同一目录生成 HTML、搜索索引及 `Assets/NodeForgeDocs`。它会更新网页生成文件；修改前请先保存源码改动。仓库中的 Markdown 是 GitHub 阅读入口，生成网站不会自动部署。

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

## 生成记录

`LayoutManifest.json` 保存输入、网页和资源文件的哈希，供发布时核对。它只记录生成结果；网站是否能够正常访问和交互，需要在浏览器中确认。
