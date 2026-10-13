/* Shared, local preview settings. Background and reading-card filters are independent. */
(() => {
  'use strict';
  const tuning=document.querySelector('[data-reading-tuning]');
  if(!tuning)return;
  const defaults=JSON.parse(tuning.dataset.readingDefaults);
  const keys=['backgroundStrength','gradientStart','gradientEnd','cardStrength','cardTransparency'];
  const storageKey='nodeforge-reading-glass-preview';
  const status=tuning.querySelector('[data-reading-status]');
  const sliders=new Map(keys.map(key=>[key,tuning.querySelector('[data-reading-setting="'+key+'"]')]));
  const values=new Map(keys.map(key=>[key,tuning.querySelector('[data-reading-value="'+key+'"]')]));
  const clamp=(value,min=0,max=100)=>Math.round(Math.min(max,Math.max(min,value)));
  let state={};
  function normalize(candidate){
    const result={};
    for(const key of keys){
      const value=candidate?.[key];
      result[key]=typeof value==='number'&&Number.isFinite(value)?clamp(value):defaults[key];
    }
    result.gradientStart=clamp(result.gradientStart,0,99);
    result.gradientEnd=clamp(result.gradientEnd,1,100);
    if(result.gradientStart>=result.gradientEnd)result.gradientEnd=result.gradientStart+1;
    return result;
  }
  function apply(candidate,persist=false){
    state=normalize(candidate);
    for(const key of keys){sliders.get(key).value=String(state[key]);values.get(key).value=state[key]+'%';values.get(key).textContent=state[key]+'%';sliders.get(key).setAttribute('aria-valuetext',state[key]+'%');}
    const style=document.body.style;
    style.setProperty('--backdrop-blur',state.backgroundStrength*.4+'px');
    style.setProperty('--backdrop-veil',String(state.backgroundStrength*.0018));
    style.setProperty('--frost-start',state.gradientStart+'%');
    style.setProperty('--frost-end',state.gradientEnd+'%');
    style.setProperty('--card-blur',state.cardStrength*.4+'px');
    style.setProperty('--card-opacity',String((100-state.cardTransparency)/100));
    const encoded=keys.map(key=>state[key]).join(',');
    document.documentElement.dataset.readingGlass=encoded;
    document.dispatchEvent(new Event('nodeforge:reading-glass'));
    try{const url=new URL(location.href);url.searchParams.set('nfGlass',encoded);history.replaceState(history.state,'',url.href);}catch{}
    if(persist){
      try{localStorage.setItem(storageKey,JSON.stringify({revision:defaults.revision,...state}));status.textContent='已在本机保存 · 其他子页面沿用';}
      catch{status.textContent='数值已应用 · 此浏览器未允许保存';}
    }
  }
  function restore(preferUrl=true){
    let saved=null;try{saved=JSON.parse(localStorage.getItem(storageKey));}catch{}
    let supplied=null;
    if(preferUrl){
      const parameter=new URL(location.href).searchParams.get('nfGlass');
      if(parameter&&/^\d{1,3}(,\d{1,3}){4}$/.test(parameter))supplied=Object.fromEntries(parameter.split(',').map((value,index)=>[keys[index],Number(value)]));
    }
    apply(supplied||(saved?.revision===defaults.revision?saved:defaults));
  }
  restore();
  for(const key of keys)sliders.get(key).addEventListener('input',()=>{
    const next={...state,[key]:Number(sliders.get(key).value)};
    if(key==='gradientStart'&&next.gradientStart>=next.gradientEnd)next.gradientEnd=next.gradientStart+1;
    if(key==='gradientEnd'&&next.gradientEnd<=next.gradientStart)next.gradientStart=next.gradientEnd-1;
    apply(next,true);
  });
  tuning.querySelector('[data-reading-reset]').addEventListener('click',()=>apply(defaults,true));
  tuning.querySelector('[data-reading-copy]').addEventListener('click',async()=>{
    const text='NodeForge 内页磨砂：\n背景磨砂强度 '+state.backgroundStrength+'%\n渐变开始位置 '+state.gradientStart+'%\n完全磨砂位置 '+state.gradientEnd+'%\n卡片磨砂强度 '+state.cardStrength+'%\n卡片透明度 '+state.cardTransparency+'%';
    try{
      if(navigator.clipboard&&window.isSecureContext)await navigator.clipboard.writeText(text);
      else{const area=document.createElement('textarea');area.value=text;area.style.cssText='position:fixed;left:-9999px;top:0';document.body.append(area);area.select();const copied=document.execCommand('copy');area.remove();if(!copied)throw Error('Clipboard unavailable');}
      status.textContent='已复制全部数值 · 可以发给我定稿';
    }catch{status.textContent='复制未获允许 · 可直接告诉我面板中的五个百分比';}
  });
  window.addEventListener('storage',event=>{if(event.key===storageKey)restore(false);});
  window.addEventListener('pageshow',()=>restore());
})();
