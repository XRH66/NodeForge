function initializeArticle() {
  const icon = (name) => {
    const paths = {copy:'<rect x="8" y="8" width="11" height="11" rx="2"/><path d="M15 8V5a2 2 0 0 0-2-2H5a2 2 0 0 0-2 2v8a2 2 0 0 0 2 2h3"/>',check:'<path d="m5 12 4 4L19 6"/>',terminal:'<path d="m5 7 4 4-4 4M12 17h7"/>'};
    return '<svg class="ui-icon" viewBox="0 0 24 24" aria-hidden="true">' + (paths[name] || paths.copy) + '</svg>';
  };
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

}
