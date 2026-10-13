# NodeForge 文档站点设计修订 2

本目录维护 2026-10-11 首页与品牌排版修订。用户审阅后认可其余设计，要求移除首页巨型 Logo 展示板、保留标题和橙色句号，并授权归档旧版后启用新版。后续用户要求右侧留空、清理重复内容并修复搜索按钮，Theme2 2.0.1 已应用于原 Mcp2R1 地址；正文与业务身份保持，当前展示来源由 LayoutManifest / WebsiteRevision 记录。

## 预览与维护

- [当前首页](../../../Docs/Versions/Build122/Mcp2R1/Index.html)：保留品牌标题与橙色句号，首屏右侧留白；原 Logo 用于页头与页脚，四个阅读入口与公共参考保留，底部重复入门引导已删除。
- [使用 AI 内页](../../../Docs/Versions/Build122/Mcp2R1/User/AiWorkflows.html)：同步新的页头、回首页导航、正文与参数/代码排版。原地址继续有效。
- [修改预览](../../../Docs/Versions/Build122/Mcp2Design2/Index.html)与[旧版归档](../../../Docs/Versions/Build122/Mcp2R1WebArchive1/Readme.html)独立保留。
- `Config.json`：默认主题、栏目、首页文案与入口；新增指南继续根据页面清单自动进入导航/搜索。
- `Home.mjs`：独立首页结构；版本信息、MCP阅读方法与交付边界保留在可展开说明中；原 README 中已由卡片展示的两组导航仅在首页省略，Markdown 原件保持。
- `Renderer.mjs`：共用页头/页脚、普通文档框架、资源与搜索索引。首页和内页都含可辨识的首页导航。
- `Theme.css`：色彩与间距变量、阅读组件、首页构图及响应式规则。顶部变量是颜色权威；文档组件基础规则之后集中维护修订 2 的排版与首页规则。
- `Theme.js`：沿用主题、导航、搜索和复制增强；适配新版主题底色。
- `Brand/`：透明 SVG 字标与用户确认沿用的 Logo PNG，和原主题素材逐字节一致，不描摹或重绘。

运行：

```powershell
node Scripts/PreviewNodeForgeDocs.mjs
python Reports/20261011DocsHomepageCleanup/CheckWebsite.py
```

预览脚本读取 Mcp2R1 的当前 Markdown，在同层 `Mcp2Design2` 生成 178 篇正文、站点首页别名和使用手册别名，共 180 HTML。此目录的 `Manifest.json` 明确为 `NodeForgeDesignPreview1 / preview`，不能直接作为正式 MCP 包激活。脚本拒绝输出到已有不同类型或非 preview 目录。预览与正式候选共用 `Scripts/RenderNodeForgeDocsTheme2.mjs`，避免两套渲染逻辑分叉；原渲染器和旧 Theme 保留。

`LayoutManifest.json` 记录九份来源、Markdown、HTML 与资源指纹；本版不引入在线字体、CDN、下载依赖或遥测。后续完整资料发布在新的 ReleaseInput 中显式指定 `"layoutTheme": "brand2"`，由现有 `BuildNodeForgeUsageDocs.py --release-input <新版输入路径>` 流程调用新版渲染器；未指定仍兼容旧版 classic。原冻结保护继续生效，生成器不能直接重写已冻结目录。

首次启用（2.0.0）属于用户明确授权的展示层换版：先把原站 376 文件逐字节归档到 `Mcp2R1WebArchive1`，再更新原地址 HTML/样式及清单和客户端资料指纹。一次性脚本 `Reports/20261011DocsThemeActivation/Activate.py` 固定旧指纹与全新归档目录，重复运行会拒绝；不能当作后续通用覆盖命令。实际来源与恢复基准见[换版报告](../../../Reports/20261011DocsThemeActivation/Readme.md)。归档内部清单保留原路径基准，不作为第二个 MCP 部署启用。

2.0.1 修复前的378个文件归档在 `Mcp2R1WebArchive2`，当前修复/归档/来源证据见[首页去重与搜索修复](../../../Reports/20261011DocsHomepageCleanup/Readme.md)。搜索组件的桌面/窄屏样式集中在 Theme.css 末尾，避免多处断点分别覆盖；常规维护先生成预览，本次修复不改变冻结保护。

改正文仍从 `Docs/PluginControl/DocumentationSources/` 及现有页面/节点清单进入正式候选发布流程；改栏目、首页和样式分别编辑本目录 Config / Home / Theme，先生成预览，再按新授权发布。不要手改生成的 HTML。新增页面通过页面清单进入导航和搜索；预览脚本仅刷新现有正文的展示，不负责发布正文更新。

## 验证边界

[当前修复报告](../../../Reports/20261011DocsHomepageCleanup/Readme.md)与[当前检查](../../../Reports/20261011DocsHomepageCleanup/ChecksActive.json)记录180 HTML / 8,993本地引用、去重、颜色/原素材保护、正式布局兼容及MCP实际资料回读；搜索函数与索引20组定点执行通过，不能替代浏览器弹窗与视觉检查。早期[设计报告](../../../Reports/20261011BrandDocsHome2/Readme.md)保留当时预览和 385 文件保护事实，不代表当时已启用。

实际浏览器的宽窄屏、主题切换、搜索与复制仍未执行：本次会话已有 file:// 工具策略限制。没有把静态检查、已有用户阅读反馈或品牌图片查看写成新版网页实测。用户不需要为本次设计工作提供截图；后续可用的浏览器控制环境恢复后再补真实交互验收。
