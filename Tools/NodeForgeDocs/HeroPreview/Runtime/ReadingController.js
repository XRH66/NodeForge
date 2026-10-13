function initializeReading() {
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

}
