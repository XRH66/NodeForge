/* CSS runs the graph continuously; scroll events only shape the visible backdrop range. */
(() => {
  'use strict';
  const hero=document.querySelector('[data-hero-preview]');
  if(!hero)return;
  const slider=hero.querySelector('[data-glass-slider]');
  const output=hero.querySelector('[data-glass-value]');
  const note=hero.querySelector('[data-glass-status]');
  const motion=hero.querySelector('[data-hero-motion]');
  const key='nodeforge-hero-glass-transparency-preview';
  const revisionKey=key+'-default-revision';
  const defaultValue=Number(hero.dataset.glassDefault);
  const clamp=value=>Math.round(Math.min(100,Math.max(0,value)));
  function setTransparency(value,persist=false){
    if(!Number.isFinite(value))return;
    value=clamp(value);
    slider.value=String(value);
    output.value=value+'%';output.textContent=value+'%';
    hero.style.setProperty('--hero-glass-opacity',String((100-value)/100));
    slider.setAttribute('aria-valuetext','透明度 '+value+'%');
    if(persist){
      try{localStorage.setItem(key,String(value));note.textContent='已在本机保留 · 刷新后继续使用';}
      catch{note.textContent='当前值已应用 · 此浏览器未允许保存';}
    }
  }
  let stored=null,revision=null;
  try{stored=localStorage.getItem(key);revision=localStorage.getItem(revisionKey);}catch{}
  const validStored=revision===hero.dataset.glassRevision&&stored!==null&&stored.trim()!==''&&Number.isFinite(Number(stored));
  setTransparency(validStored?Number(stored):defaultValue);
  if(!validStored){try{localStorage.setItem(key,String(defaultValue));localStorage.setItem(revisionKey,hero.dataset.glassRevision);}catch{}}
  slider.addEventListener('input',()=>setTransparency(Number(slider.value),true));
  hero.querySelector('[data-glass-reset]').addEventListener('click',()=>setTransparency(defaultValue,true));
  hero.querySelector('[data-glass-copy]').addEventListener('click',async()=>{
    const text='首页背景磨砂透明度：'+slider.value+'%';
    try{
      if(navigator.clipboard&&window.isSecureContext)await navigator.clipboard.writeText(text);
      else{
        const area=document.createElement('textarea');area.value=text;area.style.cssText='position:fixed;left:-9999px;top:0';document.body.append(area);area.select();
        const copied=document.execCommand('copy');area.remove();if(!copied)throw Error('Clipboard unavailable');
      }
      note.textContent='已复制 '+slider.value+'% · 可以发给我定稿';
    }catch{note.textContent='当前透明度 '+slider.value+'% · 可直接把数值告诉我';}
  });
  const reduced=matchMedia('(prefers-reduced-motion: reduce)');
  let paused=reduced.matches,visible=true;
  let frame=0,rootStart=0,viewportHeight=1,viewportWidth=1,scrollSpan=1,scrollStrength=1;
  let radiusX=1,radiusY=1,clearStop=88,sideStrength=0;
  const splitStart=Number(hero.dataset.splitStart);
  const splitEnd=Number(hero.dataset.splitEnd);
  function measure(){
    const headerHeight=document.querySelector('.site-header')?.getBoundingClientRect().height||0;
    const homeRect=hero.getBoundingClientRect();
    rootStart=Math.max(0,homeRect.top+window.scrollY-headerHeight);
    viewportHeight=Math.max(1,window.innerHeight-headerHeight);
    viewportWidth=document.documentElement.clientWidth||window.innerWidth;
    const contentRect=hero.querySelector('.home-content')?.getBoundingClientRect();
    const sectionRect=hero.querySelector('.home-hero')?.getBoundingClientRect();
    const copyRect=hero.querySelector('.hero-copy')?.getBoundingClientRect();
    const left=contentRect?.left??Math.max(23,(viewportWidth-1280)/2);
    const right=contentRect?.right??(viewportWidth-left);
    const sideSpace=Math.max(0,Math.min(left,viewportWidth-right));
    const edgeInset=Math.max(16,Math.min(56,viewportWidth*.025));
    const textGap=Math.max(48,Math.min(72,viewportWidth*.025));
    sideStrength=Math.max(0,Math.min(1,(sideSpace-edgeInset-textGap-40)/120));
    const clearRadius=(viewportWidth/2-Math.max(edgeInset,sideSpace-textGap))/.97;
    const feather=Math.max(64,Math.min(120,viewportWidth*.05));
    radiusX=Math.max(1,clearRadius+feather);radiusY=viewportHeight*3;
    clearStop=Math.max(1,Math.min(99,clearRadius/radiusX*100));
    const available=Math.max(0,document.documentElement.scrollHeight-window.innerHeight-rootStart);
    scrollSpan=Math.max(1,available);scrollStrength=Math.min(1,available/140);
    const sectionHeight=sectionRect?.height??580;
    const bodyTop=contentRect?contentRect.top-homeRect.top:sectionHeight+58;
    hero.style.setProperty('--backdrop-top',headerHeight+'px');
    hero.style.setProperty('--edge-inset',edgeInset+'px');
    hero.style.setProperty('--edge-feather',Math.max(48,Math.min(96,viewportWidth*.042))+'px');
    hero.style.setProperty('--content-left',left+'px');hero.style.setProperty('--content-right',right+'px');
    hero.style.setProperty('--wash-feather',Math.max(88,Math.min(144,viewportWidth*.07))+'px');
    hero.style.setProperty('--copy-center',(copyRect?copyRect.left+copyRect.width/2:left+275)+'px');
    hero.style.setProperty('--copy-y',sectionHeight*.42+'px');
    hero.style.setProperty('--copy-rx',Math.max(360,(copyRect?.width??550)*1.15)+'px');
    hero.style.setProperty('--copy-ry',Math.max(360,sectionHeight*.85)+'px');
    hero.style.setProperty('--body-wash-start',Math.max(0,bodyTop-180)+'px');
    hero.style.setProperty('--body-wash-end',Math.max(1,bodyTop)+'px');
  }
  function paint(){
    const distance=Math.max(0,window.scrollY-rootStart);
    const start=scrollSpan*splitStart,end=scrollSpan*splitEnd;
    const fraction=Math.max(0,Math.min(1,(distance-start)/(end-start)));
    const progress=fraction*fraction*(3-2*fraction)*scrollStrength;
    // A broad soft ellipse rises from below the viewport, rather than opening a central slit.
    const from=viewportHeight+radiusY*1.04,to=viewportHeight*.48;
    hero.style.setProperty('--range-cx',viewportWidth/2+'px');
    hero.style.setProperty('--range-cy',(from+(to-from)*progress)+'px');
    hero.style.setProperty('--range-rx',radiusX+'px');hero.style.setProperty('--range-ry',radiusY+'px');
    hero.style.setProperty('--range-clear-stop',clearStop+'%');
    hero.style.setProperty('--range-opacity',String(1-progress*(1-sideStrength)));
  }
  function schedule(){
    if(frame||document.hidden||!visible)return;
    frame=requestAnimationFrame(()=>{frame=0;if(!document.hidden&&visible)paint();});
  }
  function updateMotion(){
    hero.dataset.motion=paused||document.hidden||!visible?'paused':'playing';
    motion.setAttribute('aria-pressed',String(paused));
    motion.textContent=paused?'继续流动':'暂停流动';
    schedule();
  }
  motion.addEventListener('click',()=>{paused=!paused;updateMotion();});
  reduced.addEventListener('change',()=>{paused=reduced.matches;updateMotion();});
  document.addEventListener('visibilitychange',updateMotion);
  if('IntersectionObserver' in window){new IntersectionObserver(entries=>{visible=entries[0].isIntersecting;updateMotion();}).observe(hero);}
  window.addEventListener('scroll',schedule,{passive:true});
  window.addEventListener('resize',()=>{measure();schedule();},{passive:true});
  window.addEventListener('pageshow',()=>{measure();paint();});
  hero.addEventListener('toggle',()=>{measure();schedule();},true);
  if('ResizeObserver' in window)new ResizeObserver(()=>{measure();schedule();}).observe(hero);
  measure();paint();
  updateMotion();
})();
