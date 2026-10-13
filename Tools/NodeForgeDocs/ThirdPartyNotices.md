# 文档渲染来源与许可

离线HTML使用Marked 17.0.5（MIT），来自本机Codex已配置运行时的固定版本。只复制lib/marked.esm.js为Marked.mjs和完整LICENSE.md为License.md，不安装/下载整套项目，不改第三方原实现。标注改名只为ES模块加载，原字节SHA记DependencyManifest.json。

官方：[Marked仓库](https://github.com/markedjs/marked)、[v17.0.5许可](https://github.com/markedjs/marked/blob/v17.0.5/LICENSE.md)。许可原文随工具保留，未将UE文档或引擎代码复制成插件。

Scripts/RenderNodeForgeDocs.mjs只负责本地包导航/样式和输出包装，Markdown解析复用Marked。HTML为静态、无外部脚本/样式/遥测/服务；源码HTML标签以文本转义，作者来源受控。渲染所用Node来自当前已配置运行时，下一机器先核本机工具路径，不自动安装。
