# 基础文档主题

本目录提供基础文档页面布局。当前 NodeForge 网站使用 [HeroPreview 主题](../HeroPreview/Readme.md)；两者共用 Markdown 文档与品牌素材。

## 文件

- `Config.json`：主题、导航和分类设置。
- `Renderer.mjs`：页面、目录、搜索索引和资源输出。
- `Theme.css`：颜色、间距、表格、代码、窄屏与打印样式。
- `Theme.js`：主题切换、搜索、导航和复制功能。
- `Brand/`：LOGO 与深浅主题字标。

## 使用

```text
node Scripts/RenderNodeForgeDocs.mjs --bundle Docs/Versions/v0.1.0
```

工具读取版本清单与 Markdown，并在版本目录生成 HTML 和网页资源。正文先改 Markdown；导航和外观分别改配置与样式，再重新生成页面。命令不会自动部署网站。

页面资源不依赖在线字体或 CDN。主题设置支持 `theme=dark` 与 `theme=light`；搜索支持 `Ctrl+K`、方向键、Enter 和 Esc。JavaScript 不可用时，正文和普通链接仍可阅读。
