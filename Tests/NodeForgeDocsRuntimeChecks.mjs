import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';
import {buildTheme} from '../Tools/NodeForgeDocs/HeroPreview/BuildTheme.mjs';

const source = new URL('../Tools/NodeForgeDocs/HeroPreview/', import.meta.url);
const baseline = new URL('../Deliverables/NodeForgeDocsWebBuild136Mcp9R19VisualInstall/Site/Assets/NodeForgeDocs/', import.meta.url);
const read = (root, name) => fs.readFileSync(new URL(name, root), 'utf8').replace(/\r\n/g, '\n');
const old = read(baseline, 'Theme.js');
const between = (start, end) => old.slice(old.indexOf(start), old.indexOf(end));
const original = {
  'ThemeController.js': between('  const root =', '  const icon =') + between('  const themeButtons', '  const toast =') + '  return {syncThemeLink};\n',
  'ArticleController.js': between('  const icon =', '  const themeButtons') + between('  const toast =', '  const tocLinks ='),
  'ReadingController.js': between('  const tocLinks =', '  const searchDialog ='),
  'NavigationController.js': old.slice(old.indexOf('  const searchDialog ='), old.lastIndexOf('})();'))
};
for (const [file, expected] of Object.entries(original)) {
  const actual = read(source, 'Runtime/' + file);
  assert.equal(actual.slice(actual.indexOf('\n')+1, actual.lastIndexOf('}')), expected, file + ' preserves its original statements');
}
const built = buildTheme();
new vm.Script(built['Theme.js']);
assert.ok(built['Theme.css'].replace(/\r\n/g, '\n').startsWith(read(baseline, 'Theme.css')));
assert.deepEqual(built, buildTheme(), 'deterministic build');

// Exercise the actual DOM-event controller without a browser, network, or test-only product entry.
class Element {
  constructor(className = '') {
    this.className = className; this.children = []; this.events = {}; this.style = {}; this.clientTop = this.clientLeft = 0;
    this.clientHeight = this.offsetHeight = 200; this.clientWidth = this.offsetWidth = 300;
    this.scrollHeight = 1000; this.scrollWidth = 300; this.scrollTop = this.scrollLeft = 0;
    this.classList = {add:(...names) => { for (const name of names) if (!this.classList.contains(name)) this.className += ' '+name; }, remove:(...names) => { this.className = this.className.split(' ').filter(n=>!names.includes(n)).join(' '); }, contains:name=>this.className.split(' ').includes(name)};
  }
  append(child) { this.children.push(child); }
  setAttribute() {}
  addEventListener(type, handler) { (this.events[type] ||= []).push(handler); }
  emit(type, props = {}) { const event = {target:this, button:0, pointerId:1, preventDefault(){this.prevented=true;}, stopPropagation(){}, ...props}; for (const handler of this.events[type] || []) handler(event); return event; }
  getBoundingClientRect() { return {top:100,left:100,right:400,bottom:300}; }
  closest() { return null; }
  matches(selector) { return selector === 'pre,.table-scroll' && this.className.includes('table-scroll'); }
  scrollTo(options) { if ('top' in options) this.scrollTop = options.top; if ('left' in options) this.scrollLeft = options.left; }
  setPointerCapture() { this.captured = true; }
  hasPointerCapture() { return this.captured; }
  releasePointerCapture() { this.captured = false; }
}
const body = new Element(), root = new Element(), table = new Element('table-scroll');
root.clientHeight = root.offsetHeight = 800; root.scrollHeight = 3200;
table.scrollHeight = table.clientHeight; table.scrollWidth = 1200;
const win = new Element();
let timerId = 0;
const timers = new Map(), frames = [], resizeObservers = [], mutations = [];
const context = {
  document:{documentElement:root, scrollingElement:root, body, createElement:()=>new Element(), querySelector:()=>null, querySelectorAll:()=>[table]},
  window:win, innerWidth:1200, innerHeight:800, matchMedia:()=>({matches:false}),
  requestAnimationFrame:callback=>frames.push(callback),
  setTimeout:(callback,delay)=>{timers.set(++timerId,{callback,delay});return timerId;}, clearTimeout:id=>timers.delete(id),
  ResizeObserver:class {constructor(callback){this.callback=callback;this.targets=new Set();resizeObservers.push(this);} observe(el){this.targets.add(el);} unobserve(el){this.targets.delete(el);}},
  MutationObserver:class {constructor(callback){this.callback=callback;mutations.push(this);} observe(){}}
};
const flush = () => { while(frames.length) frames.shift()(); };
const expire = delay => { for (const [id,timer] of [...timers]) if (timer.delay===delay) {timers.delete(id);timer.callback();} };
vm.runInNewContext(read(source,'Runtime/Scrollbars.js')+'\ninitializeScrollbars();', context);
flush();
const [pageRail, tableY, tableX] = body.children;
assert.equal(tableY.hidden,true,'no unnecessary vertical rail on horizontal table');
assert.equal(pageRail.children[0].style.height,'200px');
root.scrollTop=1200; win.emit('scroll'); flush();
assert.equal(pageRail.children[0].style.transform,'translateY(300px)');
assert.ok(pageRail.classList.contains('is-visible'));
expire(950); assert.ok(!pageRail.classList.contains('is-visible'),'idle fade');
pageRail.emit('pointerenter');expire(950);assert.ok(pageRail.classList.contains('is-visible'));
pageRail.emit('pointerleave');expire(220);assert.ok(!pageRail.classList.contains('is-visible'));
const thumb=pageRail.children[0];
pageRail.emit('pointerdown',{target:thumb,clientY:350});
thumb.emit('pointermove',{clientY:500});assert.equal(root.scrollTop,1800,'drag follows pointer');
thumb.emit('pointermove',{clientY:9999});assert.equal(root.scrollTop,2400,'drag clamps at end');
thumb.emit('pointercancel');assert.ok(!pageRail.classList.contains('is-dragging'));
const bubble=pageRail.emit('wheel',{deltaY:100,deltaX:0,deltaMode:0});assert.ok(!bubble.prevented,'wheel can leave a finished scroll area');
root.scrollTop=0;pageRail.emit('wheel',{deltaY:2,deltaX:0,deltaMode:1});assert.equal(root.scrollTop,32,'line-mode wheel');
tableX.emit('pointerdown',{clientX:250});assert.equal(table.scrollLeft,450,'horizontal track click');
tableX.emit('wheel',{deltaX:100,deltaY:0,deltaMode:0});assert.equal(table.scrollLeft,550,'horizontal wheel');
root.scrollHeight=800;resizeObservers[0].callback();flush();assert.equal(pageRail.hidden,true,'rail disappears when content no longer overflows');
const child=new Element();table.children=[child];mutations[0].callback();flush();
assert.ok(resizeObservers[1].targets.has(child));table.children=[];mutations[0].callback();
assert.ok(!resizeObservers[1].targets.has(child),'replaced search/content nodes do not remain observed');
const highContrastBody=new Element();
vm.runInNewContext(read(source,'Runtime/Scrollbars.js')+'\ninitializeScrollbars();',{...context,document:{...context.document,body:highContrastBody},matchMedia:()=>({matches:true})});
assert.equal(highContrastBody.children.length,0,'high contrast keeps native scrollbars');
console.log(JSON.stringify({passed:true,originalFeatureBodies:4,deterministicBuild:true,unchangedExistingCss:true,scrollbarEventChecks:true,browserVisualCheck:false}));
