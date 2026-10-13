function initializeNavigation(syncThemeLink) {
  const searchDialog = document.querySelector('.search-dialog');
  const navigationDialog = document.querySelector('.navigation-dialog');
  for (const dialog of [searchDialog,navigationDialog]) {
    dialog?.querySelector('[data-close]')?.addEventListener('click', () => dialog.close());
    dialog?.addEventListener('click', event => { if (event.target === dialog) { const r=dialog.getBoundingClientRect(); if(event.clientX<r.left||event.clientX>r.right||event.clientY<r.top||event.clientY>r.bottom)dialog.close(); } });
  }
  document.querySelector('.mobile-menu-button')?.addEventListener('click', () => navigationDialog.showModal());
  const input = document.querySelector('.search-input');
  const results = document.querySelector('.search-results');
  const summary = document.querySelector('.search-summary');
  const pageRoot = document.querySelector('script[data-doc-root]').dataset.docRoot;
  const index = window.NodeForgeDocsIndex?.pages || [];
  const baseUrl = new URL(pageRoot, location.href);
  function runSearch() {
    const query = input.value.trim().toLocaleLowerCase();
    const terms = query.split(/\s+/).filter(Boolean);
    const matches = index.map(page => {
      const title = page.title.toLocaleLowerCase();
      const haystack = (title + ' ' + page.section + ' ' + page.keywords).toLocaleLowerCase();
      return {page,score:terms.every(term => haystack.includes(term)) ? terms.reduce((score,term) => score+(title.includes(term)?10:1),0) : -1};
    }).filter(item => item.score >= 0 && (query || !item.page.node)).sort((a,b) => b.score-a.score);
    results.replaceChildren();
    const shown = matches.slice(0,16);
    summary.textContent = query ? matches.length ? '找到 ' + matches.length + ' 篇资料' + (matches.length>16?' · 显示前 16 项':'') : '没有找到匹配资料，试试节点名或更短的关键词。' : '浏览指南，或输入标题、章节、节点名称';
    for (const {page} of shown) {
      const li = document.createElement('li'), a = document.createElement('a'), group = document.createElement('small'), title = document.createElement('strong'), excerpt = document.createElement('p');
      a.href = new URL(page.href, baseUrl).href; syncThemeLink(a); group.textContent = page.section; title.textContent = page.title; excerpt.textContent = page.excerpt;
      a.append(group,title,excerpt);li.append(a);results.append(li);
    }
  }
  function openSearch() { navigationDialog?.close();runSearch();searchDialog.showModal();input.focus(); }
  document.querySelector('.search-trigger')?.addEventListener('click', openSearch);
  input.addEventListener('input', runSearch);
  input.addEventListener('keydown', event => { if (event.key === 'ArrowDown') { event.preventDefault();results.querySelector('a')?.focus(); } });
  results.addEventListener('keydown', event => {
    const links = [...results.querySelectorAll('a')]; const current = links.indexOf(document.activeElement);
    if (event.key === 'ArrowDown') { event.preventDefault();links[Math.min(current+1,links.length-1)]?.focus(); }
    if (event.key === 'ArrowUp') { event.preventDefault();if(current<=0)input.focus();else links[current-1]?.focus(); }
  });
  document.addEventListener('keydown', event => {
    const editing = /^(INPUT|TEXTAREA|SELECT)$/.test(event.target.tagName) || event.target.isContentEditable;
    if (((event.ctrlKey || event.metaKey) && event.key.toLowerCase()==='k') || (event.key==='/' && !editing && !event.ctrlKey && !event.altKey && !event.metaKey)) { event.preventDefault();if(!searchDialog.open)openSearch(); }
  });
}
