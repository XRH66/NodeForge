import {blueprintScene} from './Blueprint.mjs';

// Appearance controls are shared by every inner page and reading card.
export function readingLayers({config,icon,escape}) {
  const appearance=config.readingAppearance;
  const keys=['backgroundStrength','gradientStart','gradientEnd','cardStrength','cardTransparency'];
  if(!appearance||keys.some(key=>!Number.isInteger(appearance[key])||appearance[key]<0||appearance[key]>100)||appearance.gradientStart>=appearance.gradientEnd||!appearance.revision||!Number.isFinite(appearance.motionSeconds)||appearance.motionSeconds<15)throw Error('Invalid reading appearance');
  const slider=(key,label,low,high,min=0,max=100)=>`<div class="reading-setting"><div class="reading-setting-label"><label for="reading-${key}">${label}</label><output for="reading-${key}" data-reading-value="${key}">${appearance[key]}%</output></div><input id="reading-${key}" data-reading-setting="${key}" type="range" min="${min}" max="${max}" step="1" value="${appearance[key]}"><div class="reading-range-labels"><span>${low}</span><span>${high}</span></div></div>`;
  return {
    style:`--hero-motion-duration:${appearance.motionSeconds}s;--backdrop-blur:${appearance.backgroundStrength*.4}px;--backdrop-veil:${appearance.backgroundStrength*.0018};--frost-start:${appearance.gradientStart}%;--frost-end:${appearance.gradientEnd}%;--card-blur:${appearance.cardStrength*.4}px;--card-opacity:${(100-appearance.cardTransparency)/100}`,
    backdrop:`<div class="document-backdrop blueprint-surface" data-blueprint-scene aria-hidden="true">${blueprintScene()}<div class="document-background-frost"></div></div>`,
    controls:appearance.finalized?'':`<div class="document-toolbar"><details class="reading-tuning" data-reading-tuning data-reading-defaults="${escape(JSON.stringify(appearance))}"><summary>${icon('layers')}磨砂效果${icon('chevron')}</summary><div class="reading-tuning-body"><p>背景从上方清晰，向下逐渐磨砂；所有阅读卡片共用一套磨砂与透明度。</p><div class="reading-settings-grid"><fieldset><legend>背景渐变</legend>${slider('backgroundStrength','背景磨砂强度','无磨砂','最强磨砂')}${slider('gradientStart','渐变开始位置','页面上方','页面下方',0,99)}${slider('gradientEnd','完全磨砂位置','页面上方','页面下方',1,100)}</fieldset><fieldset><legend>全部阅读卡片</legend>${slider('cardStrength','卡片磨砂强度','无磨砂','最强磨砂')}${slider('cardTransparency','卡片透明度','更实','更通透')}<p class="reading-control-note">正文、导航与目录一起调；数值在站内导航时沿用，确认后再固定。</p></fieldset></div><div class="reading-tuning-actions"><button type="button" data-blueprint-toggle aria-pressed="false">暂停流动</button><button type="button" data-reading-reset>恢复默认</button><button type="button" data-reading-copy>复制全部数值</button><small data-reading-status role="status">试调数值 · 本机保存</small></div></div></details></div>`
  };
}
