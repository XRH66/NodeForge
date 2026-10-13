function initializeTheme() {
  const root = document.documentElement;
  const docBase = new URL(document.querySelector('script[data-doc-root]').dataset.docRoot, location.href);
  function syncThemeLink(link) {
    const raw = link.getAttribute('href');
    if (!raw || raw.startsWith('#')) return;
    const url = new URL(raw, location.href);
    if (url.origin === docBase.origin && url.pathname.startsWith(docBase.pathname) && /\.html$/i.test(url.pathname)) {
      url.searchParams.set('theme', root.dataset.theme);
      link.href = url.href;
    }
  }
  const themeButtons = [...document.querySelectorAll('[data-set-theme]')];
  function setTheme(theme, persist = true) {
    if (theme !== 'light' && theme !== 'dark') return;
    root.dataset.theme = theme;
    document.querySelector('meta[name="theme-color"]')?.setAttribute('content', theme === 'dark' ? '#141414' : '#f8f6f0');
    themeButtons.forEach(button => button.setAttribute('aria-pressed', String(button.dataset.setTheme === theme)));
    document.querySelectorAll('a[href]').forEach(syncThemeLink);
    if (persist) { try { localStorage.setItem('nodeforge-docs-theme', theme); } catch {} }
  }
  setTheme(root.dataset.theme, false);
  // Capture actual before/after theme views; animate only a circular clip on the new view.
  const themeReduced=matchMedia('(prefers-reduced-motion: reduce)');
  let themeTransitionBusy=false,queuedTheme=null,activeThemeTransition=null;
  async function revealTheme(theme,point){
    if(themeTransitionBusy){queuedTheme={theme,point};return;}
    if(theme===root.dataset.theme)return;
    if(!document.startViewTransition||!root.animate||themeReduced.matches||document.hidden){setTheme(theme);return;}
    const width=window.innerWidth,height=window.innerHeight;
    const x=Math.max(0,Math.min(width,point.x)),y=Math.max(0,Math.min(height,point.y));
    const radius=Math.hypot(Math.max(x,width-x),Math.max(y,height-y));
    themeTransitionBusy=true;
    root.dataset.themeReveal='true';
    root.style.setProperty('--theme-reveal-x',x+'px');
    root.style.setProperty('--theme-reveal-y',y+'px');
    let transition;
    try{
      transition=document.startViewTransition(()=>setTheme(theme));
      activeThemeTransition=transition;
      transition.finished.catch(()=>{});transition.updateCallbackDone.catch(()=>{});
      await transition.ready;
      const animation=root.animate({clipPath:[`circle(0px at ${x}px ${y}px)`,`circle(${radius}px at ${x}px ${y}px)`]},
        {duration:900,easing:'cubic-bezier(.22,.68,.16,1)',fill:'forwards',pseudoElement:'::view-transition-new(root)'});
      await animation.finished;
      await transition.finished;
    }catch{
      transition?.skipTransition();
      // A skipped visual transition still updates the real theme. Always leave a usable page.
      if(transition){try{await transition.updateCallbackDone;}catch{}}
      if(root.dataset.theme!==theme)setTheme(theme);
    }finally{
      delete root.dataset.themeReveal;
      root.style.removeProperty('--theme-reveal-x');root.style.removeProperty('--theme-reveal-y');
      themeTransitionBusy=false;
      activeThemeTransition=null;
      const next=queuedTheme;queuedTheme=null;
      if(next)revealTheme(next.theme,next.point);
    }
  }
  themeButtons.forEach(button=>{
    let press=null;
    button.addEventListener('pointerdown',event=>{press={x:event.clientX,y:event.clientY};});
    button.addEventListener('pointercancel',()=>{press=null;});
    button.addEventListener('keydown',()=>{press=null;});
    button.addEventListener('click',event=>{
      const rect=button.getBoundingClientRect();
      const point=press||(event.detail>0?{x:event.clientX,y:event.clientY}:{x:rect.left+rect.width/2,y:rect.top+rect.height/2});
      press=null;
      return revealTheme(button.dataset.setTheme,point);
    });
  });
  window.addEventListener('resize',()=>activeThemeTransition?.skipTransition(),{passive:true});
  document.addEventListener('visibilitychange',()=>{if(document.hidden)activeThemeTransition?.skipTransition();});

  return {syncThemeLink};
}
