# 首页固定背景与内页阅读卡片

这是用户2026-10-11授权的文档站展示修订，从Theme2 2.0.1派生，现已按用户明确认可定稿；正式Mcp2R1和Theme2来源保持。预览首页仍为 [Mcp2Design2/Index.html](../../../Docs/Versions/Build122/Mcp2Design2/Index.html)。


## 2026-10-13 运行时代码整理与 Pico 滚动条

本轮按用户要求使用 codebase-design 技能，保留已认可的视觉和全部文档内容。现有首页/蓝图/阅读卡片模块保持；原 Theme.js 的四组实现分别收进 Runtime/ThemeController.js、ArticleController.js、ReadingController.js、NavigationController.js，每组只公开一个初始化入口，状态在模块内维护。主题模块只向搜索模块提供主题链接处理。Runtime/Scrollbars.js 统一处理页面、侧栏、目录、搜索和代码/表格滚动，样式在 Runtime/Scrollbars.css。

BuildTheme.mjs 是唯一运行时生成入口，Renderer.mjs 直接调用；输出仍为经典脚本 Theme.js 和 Theme.css，页面引用、离线 file:// 用法、搜索索引和全部 HTML 不变。源目录旧 Theme.js 已移除，禁止手工修改生成物。单独更新现有 Site 时执行 `node Tools/NodeForgeDocs/HeroPreview/BuildTheme.mjs --output Sites/NodeForgeDocs/dist/Assets/NodeForgeDocs`；新文档渲染自动生成并把 Runtime 来源纳入原布局清单。现有冻结交付和 ZIP 不回写。

滚动条对齐用户指定 Pico 的 AppScrollArea.tsx / 04-page-and-scroll.css：8px 朱砂色圆角条，悬停/拖动 10px；滚动后 950ms 淡出、离开 220ms、拖动结束 400ms，淡出 180ms。支持轨道点击、指针捕获、横向滚动及滚轮行/页单位；保留原生滚动尺寸，避免内容位移。减少动画关闭过渡，高对比保留原生滚动条，打印隐藏装饰。搜索结果替换时取消旧节点观察，避免长期累积。

验证使用 `node Tests/NodeForgeDocsRuntimeChecks.mjs`：四组原交互实现逐段相同、原 CSS 保持、构建确定性、实际控制器事件测试通过；这些是无浏览器事件验证，不是视觉实测。发布前文件对照仅 Theme.js / Theme.css 两项变化，933 个文件中的 HTML、图片、品牌素材、蓝图与磨砂参数保持。发布状态与证据见[运行时整理报告](../../../Reports/DocsRuntime1/Readme.md)。用户明确发布后自行检查，AI 不再做发布后页面检查。

## 当前2.4.1-preview（用户已认可定稿）

用户在选值及首屏衬底修订后明确认可当前设计，并要求落实工作记录和文档。该版作为后续文档站展示维护基线；当前任务已停止，后续内容修改、重新试调或正式原址换版按新任务实施。[确认与记录收口](../../../Reports/20261011DocsReadingCards/UserApproval/Review.json)保留文档快照、链接检查和页面未变证据。设计认可与AI浏览器实测分别记录，下文验证边界保持。

首页背景固定在导航栏下的首屏范围，持续流动，不再随滚动收拢；首页调节条已移除。按最新截图，标题、说明、按钮及脚注下面的深色衬底已去掉，下方内容区保持页面底色。

内页采用正文大卡片、左导航/右目录圆角小卡片，共享一套磨砂和透明度。正文卡片从导航栏下沿铺到页面底部，上下无露出背景的留白；文字仍有内边距。两侧小卡片留间距，保持导航/目录吸附；窄屏沿用移动导航。蓝图背景固定于视口持续流动，背景磨砂从上方清晰向下渐变。

用户最新图一已确认：背景磨砂10%，渐变开始0%，完全磨砂1%，全部阅读卡片磨砂100%、透明度2%。按图中明确数值固定，不以环境URL残留的24%替代；参数见Config.json / readingAppearance，finalized=true。调节面板及Reading.js已退出活动页面；旧本机值/试调URL不能覆盖定稿值，站内链接清理nfGlass同时保留主题与锚点。

如后续明确要求重新试调，可在Config将finalized改为false并设置新revision、用专用预览脚本重新生成；Reading.mjs / Reading.js保留了五项控件、统一卡片透明度、本机保存、跨页参数、恢复与复制。未获新任务时不自行改值。

持续流动46秒/连线信号5秒共用Blueprint.css / Blueprint.js，离开可见范围/后台及减少动画偏好暂停；不会由滚动修改相位。主题圆形揭幕来源保持，下文说明仍有效。详[本次定值与卡片报告](../../../Reports/20261011DocsReadingCards/Readme.md)。

## 2.3.1历史调节方法（已停用）

以下保留上一版调节/范围说明；其首页控件和滚动蒙版已退出活动页面，不作为当前操作方法。

刷新预览页，在首屏右下角展开“调整背景效果”。磨砂透明度0–100%，越大越通透；按用户最新要求暂定100%，此时磨砂层完全透明。滑杆实时修改磨砂层整体透明度，14px模糊参数保留供继续调整；标题区和下方正文有独立衬底。

选值通过浏览器localStorage键 `nodeforge-hero-glass-transparency-preview` 保存，首页两个别名和刷新会读取该值；配套 `-default-revision` 键会在这次更新首次加载时采用用户暂定100%，避免旧62%覆盖新要求。之后再次试调仍保留。浏览器拒绝储存时继续应用当前值。可复制或告知百分比；复制权限失败时仍显示值。

背景铺满首页范围：节点按46秒循环持续流动，连线信号按5秒循环，停滚或上滚均不改变方向、速度或相位。滚动只改变显示范围：宽缓的椭圆透明区从视口下沿向上推进，将可见背景逐渐留在两侧；上滚使范围原路展开。进度按首页实际可滚动长度计算，3%开始、72%完成，短首页无需加长内容也能走完效果；展开详情或改变窗口会重新测量。

屏幕外沿留16–56px并柔和渐隐，正文旁保留48–72px缓冲。依据实际内容列宽测量，侧边空间不足时逐渐淡出背景，窄屏不挤出装饰条。标题和正文衬底均为连续柔和渐变，不再用有矩形边缘的遮罩块。原纵向渐隐保持。

“暂停流动”只暂停节点和连线，滚动仍正常调整范围。减少动画偏好默认暂停持续流动，用户可主动继续；离开首页视口或切后台临时暂停。调节透明度不重启动画。

## 圆形主题切换

首页及全部内页共享Theme.js / Theme.css。浏览器捕获旧主题和实际更新后的新主题画面，新主题始终完整排版，只在上层做从0到最终半径的圆形裁切，旧主题留在下层；无整体缩放、淡入混合或布局变形。圆心来自按钮pointerdown位置，键盘操作回退按钮中心；半径为触点到视口最远角落距离，900ms扩散。布局尺寸规则保持，原中英字标viewBox相同。

参考[Chrome官方同文档视图切换](https://developer.chrome.com/docs/web-platform/view-transitions/same-document#animating_with_javascript)，使用startViewTransition与新视图clipPath，未新增库。快速点击保留最后请求，拒绝或窗口变化结束覆盖层；不支持或减少动画则直接更新真实主题。浏览器效果仍待验，不能把Node模拟写为实测。

## 来源与维护

- Config.json：当前2.4.1-preview，home.heroAppearance固定100%通透/46秒；readingAppearance保存用户确认的10 / 0 / 1 / 100 / 2，finalized=true。
- Home.mjs / Home.css：固定首屏背景、原标题/橙色句号/按钮、下方独立内容区；无衬底、无调节条、无滚动蒙版。
- Blueprint.mjs：单一SVG场景、四份双行连续瓦片；不是UE生成或验收样例，不改变支持清单。
- Blueprint.css / Blueprint.js：首页及内页共享持续动效、主题配色、可见性/减少动画节制；定稿后清理旧试调参数，原Theme.js字节保持。
- Reading.mjs / Reading.css：内页正文大卡片和两侧圆角小卡片；中央上下顶满、侧栏吸附、统一滤镜变量、外部蓝图渐变磨砂、窄屏/打印。调节面板由finalized控制。
- Reading.js：保留可调能力供后续明确任务；当前定稿页面不加载，旧值不会覆盖内联固定参数。
- Hero.css / Hero.js：2.3.1历史来源保留，当前HTML、来源与资源清单不引用。
- Renderer.mjs：首页引用Home资源、内页引用Reading资源；所有页面共享Blueprint和已认可主题，正文/导航逻辑保持。
- Brand：正式Theme2原件直接复用。Theme.css仅在原规则末尾增加视图层规则；Theme.js在主题设置处增加圆形揭幕，搜索、复制、正文增强和阅读逻辑保持原字节。

运行 `node Scripts/PreviewNodeForgeDocsHero.mjs` 生成预览。不要运行普通PreviewNodeForgeDocs覆盖正在调节的新版预览。新渲染器仍拒绝冻结输出；本版没有被正式发布器激活。

当前证据见[内页卡片与磨砂定值](../../../Reports/20261011DocsReadingCards/Readme.md)；[范围与留白修复](../../../Reports/20261011DocsBackdropSpacing/Readme.md)、[主题揭幕](../../../Reports/20261011DocsMotionAndTheme/Readme.md)、[旧滚动背景](../../../Reports/20261011DocsScrollBackdrop/Readme.md)与[最初卡片](../../../Reports/20261011DocsHeroPreview/Readme.md)保留历史。当前值已按截图固定、临时面板退出；正式Mcp2R1仍2.0.1，本次未原址激活。

## 2026-10-12 当前网站发布包

后续Mcp4R2已经采用本布局；当前发布交付为[Build131/Mcp4R2 ZIP](../../../Deliverables/NodeForgeDocsWeb-Build131-Mcp4R2-20261012.zip)，默认入口Site/index.html，部署见[包内说明](../../../Deliverables/NodeForgeDocsWeb-Build131-Mcp4R2-20261012/Readme.md)。上节Mcp2R1/预览未激活叙述是旧Build122地址的历史，不覆盖最新阅读版。

复用 `python Scripts/PackageNodeForgeDocsWeb.py --bundle Docs/Versions/Build131/Mcp4R2 --output Deliverables/<新交付名>` 导出冻结包。它核对原字节，保留目录结构，规范默认首页为小写、将页面直接引用的工程外协议/历史证据收进Attachments，并核对服务器大小写敏感的链接/锚点及搜索目标；不会改原冻结站。Source和Provenance用于维护/追溯，仅上传Site。本次实际根/子目录HTTP取件与ZIP完整性通过，AI浏览器视觉和托管发布未执行；见[证据](../../../Reports/20261012DocsWebPackage/Review.json)。

已验证的打包约束：Windows绝对路径必须先以as_posix统一斜杠，再计算网页相对URL；否则posixpath.relpath会把盘符混入href。适用本打包器/Windows路径，首轮链接检查抓获并修正，见[失败记录](../../../Reports/20261012DocsWebPackage/FailedAttempt.json)。打包器路径实现或运行平台变化时重新核对应检查，不重复旧网页整批交互测试。
