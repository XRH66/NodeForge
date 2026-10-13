/* The clock is independent of scrolling. Only visibility and the explicit pause change playback. */
(() => {
  'use strict';
  const scene=document.querySelector('[data-blueprint-scene]');
  if(!scene)return;
  const reduced=matchMedia('(prefers-reduced-motion: reduce)');
  const toggle=document.querySelector('[data-blueprint-toggle]');
  let paused=reduced.matches,visible=true;
  function update(){
    document.documentElement.dataset.blueprintMotion=paused||document.hidden||!visible?'paused':'playing';
    if(toggle){toggle.setAttribute('aria-pressed',String(paused));toggle.textContent=paused?'继续流动':'暂停流动';}
  }
  toggle?.addEventListener('click',()=>{paused=!paused;update();});
  reduced.addEventListener('change',()=>{paused=reduced.matches;update();});
  document.addEventListener('visibilitychange',update);
  if('IntersectionObserver' in window)new IntersectionObserver(entries=>{visible=entries[0].isIntersecting;update();}).observe(scene);
  update();
  // Carry preview choices through local HTML navigation, even when file storage is per-page.
  const docBase=new URL(document.querySelector('script[data-doc-root]').dataset.docRoot,location.href);
  const finalized=document.querySelector('script[data-blueprint-controller]')?.dataset.glassFinalized==='true';
  const parameter=new URL(location.href).searchParams.get('nfGlass');
  if(!finalized&&parameter&&/^\d{1,3}(,\d{1,3}){4}$/.test(parameter))document.documentElement.dataset.readingGlass=parameter;
  if(finalized){
    delete document.documentElement.dataset.readingGlass;
    try{const url=new URL(location.href);url.searchParams.delete('nfGlass');history.replaceState(history.state,'',url.href);}catch{}
  }
  function syncLink(link){
    const settings=document.documentElement.dataset.readingGlass;
    const raw=link.getAttribute('href');
    if((!finalized&&!settings)||!raw||raw.startsWith('#'))return;
    const url=new URL(raw,location.href);
    if(url.origin===docBase.origin&&url.pathname.startsWith(docBase.pathname)&&/\.html$/i.test(url.pathname)){
      if(finalized)url.searchParams.delete('nfGlass');else url.searchParams.set('nfGlass',settings);
      link.href=url.href;
    }
  }
  function syncAll(){document.querySelectorAll('a[href]').forEach(syncLink);}
  document.addEventListener('nodeforge:reading-glass',syncAll);
  syncAll();
  if('MutationObserver' in window)new MutationObserver(records=>{
    for(const record of records)for(const node of record.addedNodes){
      if(node.nodeType!==1)continue;
      if(node.matches('a[href]'))syncLink(node);
      node.querySelectorAll('a[href]').forEach(syncLink);
    }
  }).observe(document.body,{childList:true,subtree:true});
})();
