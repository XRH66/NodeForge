# 品牌文档主题

本目录提供带品牌首页的文档主题。当前网站的动态背景和阅读卡片由 [HeroPreview](../HeroPreview/Readme.md) 维护。

## 文件

| 文件 | 用途 |
| --- | --- |
| `Config.json` | 网站名称、导航和首页文字 |
| `Home.mjs` | 品牌首页结构 |
| `Renderer.mjs` | 页头、页脚、正文、目录和搜索索引 |
| `Theme.css`、`Theme.js` | 样式和交互 |
| `Brand/` | LOGO 与深浅主题字标 |

## 使用

```text
node Scripts/RenderNodeForgeDocsTheme2.mjs --bundle Docs/Versions/v0.1.0
```

工具在指定版本目录生成网页和共享资源，不会部署网站。产品正文来自版本目录的 Markdown；生成的 HTML 应通过渲染器更新。

修改主题后，请查看首页、主要指南、长表格和代码页，并检查深浅主题、窄屏导航、搜索与复制。`LayoutManifest.json` 记录生成文件的哈希，可用于核对发布内容。
