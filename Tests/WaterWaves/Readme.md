# 纯数学三方向波纹

**2026-10-10 夜间阶段历史入口**：本页WaterWaves1/Visual1及较强默认值版本保留各自输入/构建时点；当时独立构建与实测边界见[夜间审计记录](../../Reports/20261010NightAudit/Readme.md)。非零GPU像素差仍不等于人工美术通过，不据旧图片或保存重开认证新构建；按[Workflow](../../Docs/Workflow.md#后续能力扩展优先级2026-10-09)选择受影响回归。 当前项目状态见[ProjectStatus](../../ProjectStatus.md)与[第二阶段最终报告](../../Reports/PluginControlStageTwo/EditorControl1/S5/FinalReport.md)；本页原构建/夜间证据仍限定于各自时点，不因此继承新版全部验收。

[WaterWaves.json](WaterWaves.json)是52节点/61边的普通NodeForgeGraph输入，使用真实扩展目录的DotProduct投影、Cosine、标量参数、常见数学、法线和Fresnel；三组不同方向/频率/速度避免只用正交两波。没有贴图或材质函数依赖，没有插件专用水面分支；浅蓝半透明DefaultLit/SurfaceForwardShading，无折射、泡沫或几何浪高。

WaterWaves1实际构图/编译/保存/独立重开通过；WaterWavesVisual1保存场景新进程重开、GPU时间/参数变化通过，三张图片实际查看。默认WaveStrength0.12下波纹很弱，像素差非零不代表肉眼效果足够明显，未据此写美术通过。

[较强默认值版本](../WaterWavesVisible/WaterWavesVisible.json)仅改JSON参数和新目标/请求，不改插件源码、节点拓扑或重新编译，旧输入和原证据保留。该版本的实际状态见[日常实测](../../Reports/20261009UsableEditor.md)。Scale/Speed/WaveStrength/Roughness/Opacity/Specular及WaterColor/EdgeColor可在材质参数中调整；不模拟流体物理。
