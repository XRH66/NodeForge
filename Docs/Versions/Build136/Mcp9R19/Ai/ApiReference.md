# API参考入口

原工具/输入/结果schema来自本包ToolContract，帮助来自ToolMetadata；字段原生定义来自Operations。Graph请求、RPC call、报告与handle不可互换。

- [工具分卷 AiApiTools1](AiApiTools1.md)
- [工具分卷 AiApiTools2](AiApiTools2.md)
- [工具分卷 AiApiTools3](AiApiTools3.md)
- [工具分卷 AiApiTools4](AiApiTools4.md)

稳定公共字段：bridgeVersion/status；业务保留cliExitCode/nativeSummary/reports/unresolvedCalls；错误保留errors.code。docs/connect/report成功分支分别有准确字段；report_read超限给navigation且selectionReturned=false，docs_index发现摘要超32768字节时明确拒绝、不截断冒称完整。unknown/pending/stopping停后续GT；waitingPersistence在原Import报告内，不能改成资产最终通过。额外原生/兼容字段保持，schema不会转换返回值。完整序列见[AiTaskRecipes](TaskRecipes.md)和[结果恢复](ResultsAndRecovery.md)。

/exportedRequest是完整导出专用pointer：原Graph最多65536 UTF-8字节，一次返回text/value/payloadSha256/sourceRef，不截断。其它子树仍限32768字节。draft.replace/loadFile只传完整text或原文件path，以及sourceRef中的exportReportPath/exportReportSha256两项；不能把整个sourceRef塞入arguments。详[AiAssetRead](AssetRead.md)。