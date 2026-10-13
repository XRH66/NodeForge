function initializeTheme() {
  const root = document.documentElement;
  const docBase = new URL(document.querySelector('script[data-doc-root]').dataset.docRoot, location.href);
  let selectedTheme = root.dataset.theme;
  function syncThemeLink(link) {
    const raw = link.getAttribute('href');
    if (!raw || raw.startsWith('#')) return;
    const url = new URL(raw, location.href);
    if (url.origin === docBase.origin && url.pathname.startsWith(docBase.pathname) && /\.html$/i.test(url.pathname)) {
      url.searchParams.set('theme', selectedTheme);
      link.href = url.href;
    }
  }
  const themeButtons = [...document.querySelectorAll('[data-set-theme]')];
  function selectTheme(theme, persist = true) {
    selectedTheme = theme;
    document.querySelectorAll('a[href]').forEach(syncThemeLink);
    if (persist) {
      try { localStorage.setItem('nodeforge-docs-theme', theme); } catch {}
      const url = new URL(location.href);
      url.searchParams.set('theme', theme);
      history.replaceState(history.state, '', url);
    }
  }
  function setTheme(theme) {
    if (theme !== 'light' && theme !== 'dark') return;
    root.dataset.theme = theme;
    document.querySelector('meta[name="theme-color"]')?.setAttribute('content', theme === 'dark' ? '#141414' : '#f8f6f0');
    themeButtons.forEach(button => button.setAttribute('aria-pressed', String(button.dataset.setTheme === theme)));
  }
  selectTheme(selectedTheme, false);
  setTheme(selectedTheme);
  // Capture actual before/after theme views; animate only a circular clip on the new view.
  const themeReduced=matchMedia('(prefers-reduced-motion: reduce)');
  let themeTransitionBusy=false,queuedTheme=null,activeThemeTransition=null,activeThemeAnimation=null;
  function interruptTransition() {
    activeThemeAnimation?.cancel();
    activeThemeTransition?.skipTransition();
  }
  async function revealTheme(theme,point){
    selectTheme(theme);
    if(themeTransitionBusy){queuedTheme={theme,point};interruptTransition();return;}
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
      activeThemeAnimation=animation;
      await animation.finished;
      await transition.finished;
    }catch{
      transition?.skipTransition();
      // A skipped visual transition still updates the real theme. Always leave a usable page.
      if(transition){try{await transition.updateCallbackDone;}catch{}}
      if(root.dataset.theme!==theme)setTheme(theme);
    }finally{
      activeThemeAnimation?.cancel();
      activeThemeAnimation=null;
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
  // Native root snapshots temporarily remove their contents from hit testing.
  // Finish that snapshot, then deliver the user's click to the same live control.
  document.addEventListener('click',async event=>{
    if(!themeTransitionBusy||event.target!==root)return;
    const point={x:event.clientX,y:event.clientY};
    const transition=activeThemeTransition;
    interruptTransition();
    try{await transition?.finished;}catch{}
    requestAnimationFrame(()=>{
      const target=document.elementFromPoint(point.x,point.y)?.closest('a,button');
      if(target?.dataset.setTheme)revealTheme(target.dataset.setTheme,point);
      else target?.click();
    });
  });
  window.addEventListener('resize',interruptTransition,{passive:true});
  window.visualViewport?.addEventListener('resize',interruptTransition,{passive:true});
  document.addEventListener('visibilitychange',()=>{if(document.hidden)interruptTransition();});

  return {syncThemeLink};
}
