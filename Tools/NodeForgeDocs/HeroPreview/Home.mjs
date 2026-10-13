// Home copy and destinations live in Config.json; this module only lays them out.
import {blueprintScene} from './Blueprint.mjs';
export function renderHome({config, asset, destination, icon, escape, body, brand, link, edition}) {
  const home = config.home;
  const appearance=home.heroAppearance;
  if(!appearance||appearance.glassTransparency!==100||!Number.isFinite(appearance.motionSeconds)||appearance.motionSeconds<15)throw Error('Invalid fixed hero appearance');
  const route = (id) => escape(destination(id) || '#');
  const cards = home.routes.filter(item => destination(item.pageId)).map((item, index) => `
    <a class="route-card" href="${route(item.pageId)}">
      <span class="route-top"><span class="route-number">0${index + 1}</span>${icon(item.icon)}</span>
      <h3>${escape(item.title)}</h3><p>${escape(item.description)}</p>
      <span class="route-bottom">${escape(item.action)}${icon('arrow')}</span>
    </a>`).join('');
  const references = home.references.filter(item => destination(item.pageId)).map(item => `
    <a class="reference-row" href="${route(item.pageId)}"><span class="reference-glyph">${icon(item.icon)}</span>
      <span><strong>${escape(item.title)}</strong><small>${escape(item.description)}</small></span>${icon('arrow')}</a>`).join('');
  return `<div class="home-experience" style="--hero-motion-duration:${appearance.motionSeconds}s">
    <main id="main">
    <section class="home-hero-band" aria-labelledby="section-0">
      <div class="home-backdrop blueprint-surface" data-blueprint-scene aria-hidden="true">${blueprintScene()}</div>
      <div class="home-main"><div class="home-hero">
      <div class="hero-copy"><p class="home-kicker"><span></span>NODEFORGE · 文档中心 / v${escape(edition)}</p>
        <h1 id="section-0">${escape(home.headline[0])}<br><span>${escape(home.headline[1])}</span><i>。</i></h1>
        <p class="hero-description">${escape(home.description)}</p>
        <div class="hero-actions"><a class="button-primary" href="${route(home.primary.pageId)}">${escape(home.primary.label)}${icon('arrow')}</a><a class="button-secondary" href="${route(home.secondary.pageId)}">${escape(home.secondary.label)}${icon('chevron')}</a></div>
        <div class="hero-footnote"><span>UE 5.3.2 · WINDOWS 64 位</span><span class="tiny-line"></span><span>材质 · Actor 蓝图 · 材质实例</span></div>
      </div>
      </div></div>
    </section>
    <div class="home-main home-content">
    <section class="home-paths" aria-labelledby="home-paths-title"><div class="home-section-heading"><div><p class="section-kicker">01 / EXPLORE THE DOCS</p><h2 id="home-paths-title">找到你的起点</h2></div><p>第一次使用，交给 AI，或继续开发。</p></div><div class="route-grid">${cards}</div></section>
    <section class="home-reference" aria-labelledby="home-reference-title"><div class="reference-intro"><p class="section-kicker">02 / REFERENCE</p><h2 id="home-reference-title">想知道能做什么？</h2><p>先看支持范围，再查节点与使用条件。<br>把每一次构建，放在明确的边界内。</p></div><div class="reference-list">${references}</div></section>
    <details class="release-details"><summary><span>当前版本与使用说明</span><small>环境要求、安装包与支持范围</small>${icon('chevron')}</summary><div class="article-content">${body}</div></details>
    <footer class="home-footer"><div>${brand}</div><nav aria-label="页脚导航">${link('UserReadme','使用手册')}${link('FeatureMap','功能与限制')}${link('VersionKnownIssues','已知限制')}${link('SourcesAndLicenses','来源与许可')}</nav><p class="footer-edition">${escape(edition)}<br><span>本地文档 / 离线可读</span></p></footer>
    </div></main></div>`;
}
