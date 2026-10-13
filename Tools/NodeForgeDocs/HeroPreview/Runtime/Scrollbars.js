function initializeScrollbars() {
  if (matchMedia('(forced-colors: active)').matches) return;
  const root = document.documentElement;
  const records = [];
  let scheduled = false;
  const header = document.querySelector('.site-header');

  function schedule() {
    if (!scheduled) {
      scheduled = true;
      requestAnimationFrame(() => { scheduled = false; records.forEach(record => record.update()); });
    }
  }

  function attach(viewport, axis, page = false) {
    const vertical = axis === 'y';
    const position = vertical ? 'scrollTop' : 'scrollLeft';
    const length = vertical ? 'clientHeight' : 'clientWidth';
    const extent = vertical ? 'scrollHeight' : 'scrollWidth';
    const coordinate = vertical ? 'clientY' : 'clientX';
    const rail = document.createElement('div');
    const thumb = document.createElement('div');
    rail.className = 'nf-scroll-rail nf-scroll-' + axis;
    rail.setAttribute('aria-hidden', 'true');
    thumb.className = 'nf-scroll-thumb';
    rail.append(thumb);
    const dialog = viewport.closest('dialog');
    (dialog || document.body).append(rail);
    viewport.classList.add('nf-scroll-viewport');
    let travel = 0, range = 0, size = 0, hideTimer, over = false, drag = null;

    function hideAfter(delay) {
      clearTimeout(hideTimer);
      hideTimer = setTimeout(() => {
        if (!over && !drag) rail.classList.remove('is-visible');
      }, delay);
    }
    function reveal() {
      if (range > 1) { rail.classList.add('is-visible'); hideAfter(950); }
    }
    function update() {
      const rect = page ? {top:0, left:0, right:innerWidth, bottom:innerHeight} : viewport.getBoundingClientRect();
      const visibleLength = viewport[length];
      range = Math.max(0, viewport[extent] - visibleLength);
      rail.hidden = range <= 1 || visibleLength === 0 || (dialog && !dialog.open);
      if (rail.hidden) { rail.classList.remove('is-visible'); return; }
      size = Math.min(visibleLength, Math.max(32, visibleLength ** 2 / viewport[extent]));
      travel = visibleLength - size;
      const offset = Math.max(0, Math.min(range, viewport[position])) / range * travel;
      const top = page ? 0 : rect.top + viewport.clientTop;
      const left = page ? innerWidth - 16 : rect.left + viewport.clientLeft + viewport.clientWidth + (viewport.offsetWidth - viewport.clientWidth - viewport.clientLeft * 2) - 16;
      Object.assign(rail.style, vertical ? {top:top+'px', left:left+'px', height:visibleLength+'px'} : {
        top:(rect.top + viewport.offsetHeight - viewport.clientTop - 16)+'px', left:(rect.left+viewport.clientLeft)+'px', width:visibleLength+'px'
      });
      Object.assign(thumb.style, vertical ? {height:size+'px', transform:`translateY(${offset}px)`} : {width:size+'px', transform:`translateX(${offset}px)`});
      // A sticky side rail must never paint or capture input over the header.
      const ceiling = page || dialog ? 0 : header?.getBoundingClientRect().bottom || 0;
      rail.style.clipPath = vertical ? `inset(${Math.max(0, ceiling-top)}px 0 ${Math.max(0,top+visibleLength-innerHeight)}px 0)` : 'none';
    }
    function move(value) {
      viewport.scrollTo({[vertical ? 'top' : 'left']: Math.max(0, Math.min(range, value)), behavior:'instant'});
      update(); reveal();
    }
    const scrollTarget = page ? window : viewport;
    scrollTarget.addEventListener('scroll', () => { schedule(); reveal(); }, {passive:true});
    viewport.addEventListener('wheel', reveal, {passive:true});
    viewport.addEventListener('focusin', reveal);
    rail.addEventListener('pointerenter', () => { over = true; clearTimeout(hideTimer); rail.classList.add('is-visible'); });
    rail.addEventListener('pointerleave', () => { over = false; hideAfter(220); });
    rail.addEventListener('pointerdown', event => {
      if (event.button !== 0) return;
      event.preventDefault();
      if (event.target === rail) {
        const rect = rail.getBoundingClientRect();
        move((event[coordinate] - (vertical ? rect.top : rect.left) - size/2) / Math.max(1, travel) * range);
      } else {
        thumb.setPointerCapture(event.pointerId);
        drag = {coordinate:event[coordinate], position:viewport[position]};
        rail.classList.add('is-dragging', 'is-visible');
      }
    });
    thumb.addEventListener('pointermove', event => {
      if (drag && travel > 0) move(drag.position + (event[coordinate]-drag.coordinate) * range/travel);
    });
    function endDrag() { drag = null; rail.classList.remove('is-dragging'); hideAfter(400); }
    thumb.addEventListener('pointerup', event => { if (thumb.hasPointerCapture(event.pointerId)) thumb.releasePointerCapture(event.pointerId); endDrag(); });
    thumb.addEventListener('pointercancel', endDrag);
    thumb.addEventListener('lostpointercapture', endDrag);
    rail.addEventListener('wheel', event => {
      const delta = vertical ? event.deltaY : event.deltaX || event.deltaY;
      const amount = delta * (event.deltaMode === 1 ? 16 : event.deltaMode === 2 ? viewport[length] : 1);
      const canScroll = amount > 0 ? viewport[position] < range-1 : amount < 0 && viewport[position] > 1;
      if (canScroll) { event.preventDefault(); event.stopPropagation(); move(viewport[position]+amount); }
      reveal();
    }, {passive:false});
    records.push({update});
    const resize = new ResizeObserver(schedule);
    resize.observe(page ? document.body : viewport);
    const observed = new Set();
    const observeChildren = () => {
      const children = new Set([...viewport.children].filter(child => !child.classList.contains('nf-scroll-rail')));
      for (const child of observed) if (!children.has(child)) { resize.unobserve(child); observed.delete(child); }
      for (const child of children) if (!observed.has(child)) { resize.observe(child); observed.add(child); }
      schedule();
    };
    if (!page) {
      new MutationObserver(observeChildren).observe(viewport, {childList:true, subtree:true, characterData:true});
      observeChildren();
    }
    if (dialog) new MutationObserver(schedule).observe(dialog, {attributes:true, attributeFilter:['open']});
    update();
  }

  attach(document.scrollingElement, 'y', true);
  document.querySelectorAll('.side-inner,.toc-inner,.search-results,.navigation-dialog,.article-content pre,.table-scroll').forEach(viewport => {
    attach(viewport, 'y');
    // Code and tables keep their existing horizontal overflow and keyboard access.
    if (viewport.matches('pre,.table-scroll')) attach(viewport, 'x');
  });
  window.addEventListener('scroll', schedule, {passive:true, capture:true});
  window.addEventListener('resize', schedule, {passive:true});
}
