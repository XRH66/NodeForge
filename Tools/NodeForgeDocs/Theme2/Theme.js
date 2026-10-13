/* Progressive enhancements; all document content and links work without JavaScript. */
(() => {
  'use strict';
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
  const icon = (name) => {
    const paths = {copy:'<rect x="8" y="8" width="11" height="11" rx="2"/><path d="M15 8V5a2 2 0 0 0-2-2H5a2 2 0 0 0-2 2v8a2 2 0 0 0 2 2h3"/>',check:'<path d="m5 12 4 4L19 6"/>',terminal:'<path d="m5 7 4 4-4 4M12 17h7"/>'};
    return '<svg class="ui-icon" viewBox="0 0 24 24" aria-hidden="true">' + (paths[name] || paths.copy) + '</svg>';
  };
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
  themeButtons.forEach(button => button.addEventListener('click', () => setTheme(button.dataset.setTheme)));

  const toast = document.querySelector('.toast');
  let toastTimer;
  function announce(message) {
    toast.textContent = message;
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => { toast.textContent = ''; }, 3200);
  }
  async function copyText(text, button) {
    try {
      if (navigator.clipboard && window.isSecureContext) {
        await navigator.clipboard.writeText(text);
      } else {
        const textarea = document.createElement('textarea');
        textarea.value = text;
        textarea.style.cssText = 'position:fixed;left:-9999px;top:0';
        document.body.append(textarea);
        textarea.select();
        const copied = document.execCommand('copy');
        textarea.remove();
        button.focus({preventScroll:true});
        if (!copied) throw Error('copy unavailable');
      }
      button.dataset.copied = 'true';
      button.innerHTML = icon('check');
      announce('已复制');
      setTimeout(() => { delete button.dataset.copied; button.innerHTML = icon('copy'); }, 1800);
    } catch { announce('浏览器未允许复制，请选中文字后手动复制。'); }
  }
  function copyButton(value, label, block = false) {
    const button = document.createElement('button');
    button.type = 'button'; button.className = 'copy-button' + (block ? ' copy-block' : '');
    button.setAttribute('aria-label', label); button.title = label; button.innerHTML = icon('copy');
    button.addEventListener('click', () => copyText(value, button));
    return button;
  }
  const content = document.querySelector('.article-content');
  // Turn existing consecutive label/value paragraphs into a semantic definition list.
  // Values are moved from the rendered document, never reconstructed or hard-coded.
  function labelOf(p) {
    if (p?.tagName !== 'P' || p.firstElementChild?.tagName !== 'STRONG') return null;
    const first = p.firstElementChild;
    if (p.textContent.trimStart().indexOf(first.textContent) !== 0 || !/[:：]\s*$/.test(first.textContent)) return null;
    return first;
  }
  for (const start of [...content.children]) {
    if (!start.isConnected || !labelOf(start)) continue;
    const fields = [];
    let current = start;
    while (labelOf(current)) {
      const label = labelOf(current);
      let next = current.nextElementSibling;
      const valueNodes = [...current.childNodes].slice([...current.childNodes].indexOf(label) + 1);
      const hasValue = valueNodes.some(node => node.textContent.trim());
      let valueParagraph = null;
      if (!hasValue && next?.tagName === 'P' && next.children.length === 1 && next.firstElementChild.tagName === 'CODE' && next.textContent.trim() === next.firstElementChild.textContent.trim()) {
        valueParagraph = next; next = next.nextElementSibling;
      }
      fields.push({paragraph:current,label,valueNodes,valueParagraph});
      current = next;
    }
    if (fields.length < 3) continue;
    const panel = document.createElement('section'); panel.className = 'config-panel'; panel.setAttribute('aria-label', '配置字段');
    const header = document.createElement('div'); header.className = 'config-panel-header';
    header.innerHTML = '<span>' + icon('terminal') + '配置参数</span><span class="config-hint">逐项填写 · 路径可复制</span>';
    const list = document.createElement('dl'); list.className = 'config-fields';
    start.before(panel); panel.append(header,list);
    for (const field of fields) {
      const item = document.createElement('div'); item.className = 'config-field';
      const term = document.createElement('dt'); term.textContent = field.label.textContent;
      const value = document.createElement('dd');
      if (field.valueParagraph) value.append(...field.valueParagraph.childNodes);
      else value.append(...field.valueNodes);
      const code = value.querySelector('code');
      if (code && (code.textContent.length > 32 || /[\\/]/.test(code.textContent))) item.classList.add('wide');
      item.append(term,value);
      if (code) item.append(copyButton(code.textContent, '复制' + term.textContent.replace(/[:：]\s*$/, '')));
      list.append(item);
      field.paragraph.remove(); field.valueParagraph?.remove();
    }
  }
  content.querySelectorAll('pre').forEach(pre => {
    const code = pre.querySelector('code');
    if (code) pre.append(copyButton(code.textContent, '复制代码', true));
  });
  content.querySelectorAll('table').forEach(table => {
    const wrap = document.createElement('div'); wrap.className = 'table-scroll'; wrap.tabIndex = 0;
    wrap.setAttribute('role','region'); wrap.setAttribute('aria-label','表格，可横向滚动');
    table.before(wrap); wrap.append(table);
  });
  content.querySelectorAll(':scope > h2').forEach(heading => {
    const first = heading.firstChild;
    const match = first?.nodeType === Node.TEXT_NODE && first.textContent.match(/^(\d+[.、])\s*/);
    if (match) {
      const number = document.createElement('span'); number.className = 'heading-number'; number.textContent = match[1];
      first.textContent = first.textContent.slice(match[0].length); heading.prepend(number);
    }
    const list = heading.nextElementSibling;
    if (list?.tagName === 'UL' && /资料|相关|进一步|参考|继续阅读/.test(heading.textContent) && [...list.children].every(li => li.children.length === 1 && li.firstElementChild.tagName === 'A')) list.classList.add('related-links');
  });

  const tocLinks = [...document.querySelectorAll('.toc a')];
  const sections = tocLinks.map(link => document.getElementById(link.hash.slice(1))).filter(Boolean);
  let scrollScheduled = false;
  function updateReading() {
    let active = sections[0];
    for (const heading of sections) { if (heading.getBoundingClientRect().top < 155) active = heading; }
    tocLinks.forEach(link => {
      const selected = link.hash === '#' + active?.id;
      link.classList.toggle('active', selected);
      if (selected) link.setAttribute('aria-current','location'); else link.removeAttribute('aria-current');
    });
    const progress = document.querySelector('.reading-progress span');
    if (progress) {
      const distance = Math.max(document.documentElement.scrollHeight - window.innerHeight, 1);
      progress.style.width = Math.min(100, Math.max(0, window.scrollY / distance * 100)) + '%';
    }
    scrollScheduled = false;
  }
  addEventListener('scroll', () => { if (!scrollScheduled) { scrollScheduled = true; requestAnimationFrame(updateReading); } }, {passive:true});
  addEventListener('resize', updateReading); updateReading();
  document.querySelector('[data-back-top]')?.addEventListener('click', () => window.scrollTo({top:0,behavior:matchMedia('(prefers-reduced-motion: reduce)').matches?'instant':'smooth'}));

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
})();
