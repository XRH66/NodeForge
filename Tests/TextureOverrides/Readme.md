# 实例纹理采样回归

七项数据：颜色父/法线父、颜色同类型替换、法线同类型替换、显式法线默认值、双向跨类型拒绝。模板父哈希为零，不能直接导入；回归复用原生MaterialParameters实际保存父字节哈希，另存Inputs。SamplerGap1保留旧Build9实际接受ColorToNormal导致失败的证据；TextureSamplers1/Build11七项全通过。跨类型拒绝必须为validation/notWritten，保存与重开成功不替代类型语义校验。

[当前证据](../../Reports/20261009UsableEditor.md)、[新采样目录](../../Catalog/UE532TextureSamplers.json)。原生资源类和采样类型以Inspect的textureSamplerSignatures为准，不依据名字猜测。
