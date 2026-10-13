import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
import {renderHome} from './Home.mjs';
import {readingLayers} from './Reading.mjs';
import {buildTheme, runtimeSources} from './BuildTheme.mjs';

const sourceRoot=path.dirname(fileURLToPath(import.meta.url));
const escape=s=>String(s??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const icons={search:'<circle cx="10.5" cy="10.5" r="6.5"/><path d="m16 16 5 5"/>',sun:'<circle cx="12" cy="12" r="4"/><path d="M12 2v2m0 16v2M2 12h2m16 0h2M5 5l1.5 1.5m11 11L19 19M5 19l1.5-1.5m11-11L19 5"/>',moon:'<path d="M20.3 15.5A8.6 8.6 0 0 1 8.5 3.7a8.6 8.6 0 1 0 11.8 11.8Z"/>',menu:'<path d="M4 6h16M4 12h16M4 18h16"/>',close:'<path d="m6 6 12 12M18 6 6 18"/>',chevron:'<path d="m9 5 7 7-7 7"/>',book:'<path d="M4 4h6a3 3 0 0 1 3 3v14a4 4 0 0 0-4-2H4V4Zm16 0h-5a2 2 0 0 0-2 2m0 15a4 4 0 0 1 4-2h3V4Z"/>',list:'<path d="M9 6h11M9 12h11M9 18h11M4 6h.01M4 12h.01M4 18h.01"/>',arrow:'<path d="M5 12h14m-5-5 5 5-5 5"/>',back:'<path d="M19 12H5m5-5-5 5 5 5"/>',up:'<path d="M12 19V5m-6 6 6-6 6 6"/>',external:'<path d="M14 4h6v6m0-6L10 14M10 4H5a1 1 0 0 0-1 1v14a1 1 0 0 0 1 1h14a1 1 0 0 0 1-1v-5"/>',clock:'<circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 2"/>'};
Object.assign(icons,{nodes:'<rect x="2" y="3" width="7" height="7" rx="1"/><rect x="15" y="14" width="7" height="7" rx="1"/><path d="M9 6.5h3v11h3"/>',layers:'<path d="m12 3 10 5-10 5L2 8l10-5Zm-10 9 10 5 10-5M2 16l10 5 10-5"/>',compass:'<circle cx="12" cy="12" r="9"/><path d="m16 8-3 5-5 3 3-5 5-3Z"/>'});
const icon=name=>`<svg class="ui-icon" viewBox="0 0 24 24" aria-hidden="true">${icons[name]||icons.book}</svg>`;

export function createThemeRenderer(job){
  const config=JSON.parse(fs.readFileSync(path.join(sourceRoot,'Config.json'),'utf8'));
  const root=path.resolve(job.root),assetsRoot=path.join(root,'Assets','NodeForgeDocs');
  const byId=new Map(job.pages.map(p=>[p.pageId,p]));
  const groups=new Map(config.groups.map(g=>[g.id,g]));
  const menuFor=group=>{
    const entries=(groups.get(group)?.menu||[['Readme','资料首页'],...config.groups.filter(g=>g.topNav!==false).map(g=>[g.home,g.label])]).filter(([id])=>byId.has(id));
    const known=new Set(entries.map(([id])=>id));
    for(const page of job.pages){
      const parts=page.output.split('/');
      if(parts[0]===group&&parts.length===2&&!known.has(page.pageId)){entries.push([page.pageId,page.title]);known.add(page.pageId);}
    }
    return entries;
  };
  const assets=['Theme.css','Theme.js','Blueprint.css','Blueprint.js','Home.css','Reading.css','Reading.js',...Object.values(config.brand)];
  const generatedTheme=buildTheme();
  for(const relative of assets){
    const source=path.resolve(sourceRoot,relative),target=path.resolve(assetsRoot,relative);
    if(!source.startsWith(sourceRoot+path.sep)||!target.startsWith(assetsRoot+path.sep))throw Error('Theme asset outside its directory');
    fs.mkdirSync(path.dirname(target),{recursive:true});
    if(relative in generatedTheme)fs.writeFileSync(target,generatedTheme[relative]);
    else fs.copyFileSync(source,target);
  }
  const searchPages=job.pages.map(page=>({
    title:page.pageId==='Readme'?'文档首页':page.title,href:page.output.replace(/\.md$/,'.html'),section:groups.get(page.output.split('/')[0])?.label||'文档中心',
    keywords:[...page.text.matchAll(/^#{2,3}\s+(.+)$/gm)].map(m=>m[1]).join(' ').slice(0,1400),
    excerpt:page.text.split(/\r?\n/).filter(line=>line.trim()&&!/^[#|>]/.test(line)&&!/^\s*[-*]/.test(line)).slice(0,2).join(' ').replace(/\[([^\]]+)\]\([^)]*\)/g,'$1').replace(/[*`]/g,'').slice(0,125),
    node:page.output.includes('/Nodes/')
  }));
  fs.writeFileSync(path.join(assetsRoot,'SearchIndex.js'),'window.NodeForgeDocsIndex='+JSON.stringify({version:config.themeVersion,pages:searchPages}).replace(/</g,'\\u003c')+';\n','utf8');
  assets.push('SearchIndex.js');
  const assetsManifest=assets.map(relative=>{const p=path.join(assetsRoot,relative),bytes=fs.readFileSync(p);return {path:path.relative(root,p).split(path.sep).join('/'),sha256:crypto.createHash('sha256').update(bytes).digest('hex'),bytes:bytes.length};});
  function render(page,html,headings){
    const target=path.resolve(root,page.output.replace(/\.md$/,'.html'));
    if(!target.startsWith(root+path.sep))throw Error('Render output outside package');
    const base=path.dirname(target);
    const local=p=>path.relative(base,path.join(root,p)).split(path.sep).join('/')||'./';
    const asset=relative=>escape(local('Assets/NodeForgeDocs/'+relative));
    const destination=id=>{const p=byId.get(id);return p?local(id==='Readme'?'Index.html':p.output.replace(/\.md$/,'.html')):null;};
    const group=page.output.split('/')[0],groupConfig=groups.get(group),label=groupConfig?.label||'文档中心';
    const link=(id,title,current=false,extra='')=>{const href=destination(id);return href?`<a href="${escape(href)}"${current?' class="current" aria-current="page"':''}${extra}>${escape(title)}</a>`:'';};
    const isHome=page.pageId==='Readme';
    const nav=link('Readme','首页',isHome)+config.groups.filter(g=>g.topNav!==false).map(g=>link(g.home,g.shortLabel,group===g.id)).join('');
    const menu=menuFor(group),sidebar=menu.map(([id,title])=>link(id,title,page.pageId===id)).join('');
    const currentIndex=menu.findIndex(([id])=>id===page.pageId);
    const tocHeadings=headings.filter(h=>h.depth===2||h.depth===3);
    const toc=tocHeadings.map(h=>`<a class="level${h.depth}" href="#${escape(h.id)}">${escape(h.title)}</a>`).join('');
    const landing=page.pageId==='Readme'||page.pageId.endsWith('Readme');
    const h1=html.match(/<h1\b[^>]*>[\s\S]*?<\/h1>/)?.[0]||`<h1>${escape(page.title)}</h1>`;
    const body=html.replace(h1,'');
    const minutes=Math.max(1,Math.round(page.text.replace(/\[[^\]]+\]\([^)]*\)/g,'').length/550));
    const shortVersion=job.bundleId.replace(/^NodeForge-/,'');
    const brand=`<a class="brand" href="${escape(destination('Readme')||destination(groupConfig?.home)||'#')}" aria-label="NodeForge 节点熔炉文档首页"><img class="brand-icon" src="${asset(config.brand.icon)}" width="34" height="34" alt=""><img class="brand-wordmark brand-dark" src="${asset(config.brand.dark)}" width="253" height="33" alt="NodeForge · 节点熔炉"><img class="brand-wordmark brand-light" src="${asset(config.brand.light)}" width="253" height="33" alt="NodeForge · 节点熔炉"></a>`;
    const pagination=(offset,cssClass,small,arrow)=>{const entry=menu[currentIndex+offset];return currentIndex>=0&&entry?`<a class="${cssClass}" href="${escape(destination(entry[0]))}">${offset<0?icon(arrow):''}<span><small>${small}</small><strong>${escape(entry[1])}</strong></span>${offset>0?icon(arrow):''}</a>`:'';};
    const pageNavigation=pagination(-1,'previous','上一页','back')+pagination(1,'next','下一页','arrow');
    const sourceLink=local(page.output);
    const startTheme=`try{var t=new URLSearchParams(location.search).get('theme')||localStorage.getItem('nodeforge-docs-theme');if(t==='light'||t==='dark')document.documentElement.dataset.theme=t}catch(e){}`;
    const docRoot=path.relative(base,root).split(path.sep).join('/')||'.';
    // The homepage cards already display these two README navigation sections.
    // Keep all other source sections, including version identity and delivery limits.
    const homeBody=isHome?body.split(/(?=<h2\b)/).filter(section=>!/^<h2\b[^>]*>\s*(选择阅读入口|公共参考)\s*<\/h2>/.test(section)).join(''):body;
    const homeMarkup=isHome?renderHome({config,asset,destination,icon,escape,body:homeBody,brand,link,edition:shortVersion}):null;
    const readingPresentation=isHome?null:readingLayers({config,icon,escape});
    const markup=`<!doctype html>
<html lang="zh-CN" data-theme="${escape(config.defaultTheme)}"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta name="color-scheme" content="dark light"><meta name="theme-color" content="#141414"><title>${escape(isHome?'首页':page.title)} · ${escape(config.siteTitle)}</title><script>${startTheme}</script><link rel="icon" type="image/png" href="${asset(config.brand.icon)}"><link rel="stylesheet" href="${asset('Theme.css')}"><link rel="stylesheet" href="${asset('Blueprint.css')}"><script src="${asset('Blueprint.js')}" data-blueprint-controller data-glass-finalized="${config.readingAppearance.finalized===true}" defer></script>${isHome?`<link rel="stylesheet" href="${asset('Home.css')}">`:`<link rel="stylesheet" href="${asset('Reading.css')}">${config.readingAppearance.finalized?'':`<script src="${asset('Reading.js')}" defer></script>`}`}<script src="${asset('SearchIndex.js')}" defer></script><script src="${asset('Theme.js')}" data-doc-root="${escape(docRoot+'/')}" defer></script></head>
<body class="${isHome?'home-page':'document-page'}"${readingPresentation?` style="${escape(readingPresentation.style)}"`:''}><a class="skip-link" href="#main">跳到正文</a><header class="site-header"><div class="topbar"><button type="button" class="mobile-menu-button" aria-label="打开文档导航">${icon('menu')}</button>${brand}<nav class="topnav" aria-label="手册分类">${nav}</nav><div class="header-tools"><button type="button" class="search-trigger" aria-label="搜索整个文档站点">${icon('search')}<span>搜索文档</span><kbd>Ctrl K</kbd></button><div class="theme-switch" role="group" aria-label="页面主题"><button type="button" data-set-theme="dark" aria-label="使用暗色模式" aria-pressed="${config.defaultTheme==='dark'}">${icon('moon')}<span>暗色</span></button><button type="button" data-set-theme="light" aria-label="使用亮色模式" aria-pressed="${config.defaultTheme==='light'}">${icon('sun')}<span>亮色</span></button></div></div></div></header>
${homeMarkup||`${readingPresentation.backdrop}<div class="workspace"><aside class="sidebar reading-card"><div class="side-inner"><a class="sidebar-home" href="${escape(destination('Readme'))}">${icon('back')}文档首页</a><div class="side-caption">${icon('book')}${escape(label)}</div><nav class="side-links" aria-label="章节导航">${sidebar}</nav><div class="side-divider"></div>${destination('FeatureMap')?`<a class="side-resource" href="${escape(destination('FeatureMap'))}">功能与支持范围${icon('external')}</a>`:''}${destination('VersionKnownIssues')?`<a class="side-resource" href="${escape(destination('VersionKnownIssues'))}">已知限制${icon('external')}</a>`:''}<div class="sidebar-bottom"><span class="version-badge"><span class="version-dot"></span>${escape(shortVersion)}</span><p class="offline-note">本地文档 · 离线可读</p></div></div></aside>
<div class="reading"><main id="main" class="reading-card article${landing?' landing':''}">${readingPresentation.controls}<nav class="breadcrumb" aria-label="当前位置">${link('Readme','文档')}${icon('chevron')}${groupConfig?link(groupConfig.home,label):'<span>'+escape(label)+'</span>'}${icon('chevron')}<span class="crumb-current">${escape(page.title)}</span></nav><div class="article-hero"><div class="eyebrow">${escape(groupConfig?.eyebrow||'NODEFORGE DOCUMENTATION')}${currentIndex>=0?' / '+String(currentIndex+1).padStart(2,'0'):''}</div>${h1}<div class="article-meta"><span>${icon('book')}${escape(label)}</span><span class="meta-divider"></span><span>${tocHeadings.filter(h=>h.depth===2).length} 个章节</span><span class="meta-divider"></span><span>${icon('clock')}约 ${minutes} 分钟</span></div></div><div class="article-content">${body}</div>${pageNavigation?`<nav class="page-pagination" aria-label="相邻章节">${pageNavigation}</nav>`:''}<footer class="article-footer"><span>NodeForge · 节点熔炉 / ${escape(shortVersion)}</span><span class="footer-links"><a href="${escape(sourceLink)}">查看 Markdown</a>${link('SourcesAndLicenses','资料来源')}</span></footer></main>
<aside class="toc reading-card" aria-label="本页内容"><div class="toc-inner"><div class="toc-title">${icon('list')}本页内容</div><nav>${toc}</nav><div class="toc-footer"><button type="button" data-back-top>${icon('up')}回到顶部</button><div class="reading-progress" aria-hidden="true"><span></span></div></div></div></aside></div></div>`}
<dialog class="search-dialog" aria-label="搜索文档"><div class="search-box">${icon('search')}<label class="sr-only" for="docs-search">搜索文档标题、章节或节点名称</label><input id="docs-search" class="search-input" type="search" placeholder="搜索文档、章节、节点…" autocomplete="off"><button type="button" class="dialog-close" data-close aria-label="关闭搜索">${icon('close')}</button></div><div class="search-summary" role="status"></div><ol class="search-results" aria-label="搜索结果"></ol><div class="search-bottom"><span>↑ ↓ 选择 · Enter 打开</span><span>Esc 关闭 · 本地搜索</span></div></dialog>
<dialog class="navigation-dialog" aria-label="文档导航"><div class="navigation-head"><strong>文档导航</strong><button type="button" class="dialog-close" data-close aria-label="关闭导航">${icon('close')}</button></div><nav class="mobile-groups" aria-label="手册分类">${nav}</nav><div class="side-caption">${escape(label)}</div><nav class="side-links" aria-label="章节导航">${sidebar}</nav></dialog><div class="toast" role="status" aria-live="polite"></div></body></html>`;
    fs.writeFileSync(target,markup,'utf8');
  }
  return {render,themeVersion:config.themeVersion,assets:assetsManifest,sourceFiles:['Home.mjs','Config.json','Theme.css','BuildTheme.mjs',...runtimeSources,'Runtime/Scrollbars.css','Renderer.mjs','Blueprint.mjs','Blueprint.css','Blueprint.js','Home.css','Reading.mjs','Reading.css','Reading.js',...Object.values(config.brand)]};
}
