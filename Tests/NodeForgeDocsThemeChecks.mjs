import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';

const source=fs.readFileSync(new URL('../Tools/NodeForgeDocs/HeroPreview/Runtime/ThemeController.js',import.meta.url),'utf8');
const deferred=()=>{let resolve,reject;const promise=new Promise((yes,no)=>{resolve=yes;reject=no;});return {promise,resolve,reject};};
const tick=()=>new Promise(resolve=>setImmediate(resolve));
function harness(width=1200,height=800){
  const events={},saved={},buttons=['dark','light'].map(theme=>({dataset:{setTheme:theme},events:{},attrs:{},addEventListener(type,fn){this.events[type]=fn;},setAttribute(name,value){this.attrs[name]=value;},getBoundingClientRect(){return {left:width-90,top:16,width:30,height:30};}}));
  const location={href:'https://docs.example/index.html?theme=dark'};
  const links=[{dataset:{},href:'https://docs.example/User/Readme.html?theme=dark',getAttribute(){return this.href;},click(){h.clicks++;}}];
  const root={dataset:{theme:'dark'},style:{setProperty(){},removeProperty(){}},animate(keyframes,options){const done=deferred();const animation={keyframes,options,finished:done.promise,cancelled:false,cancel(){this.cancelled=true;done.reject(new Error('cancelled'));}};h.animations.push(animation);return animation;}};
  const document={documentElement:root,hidden:false,querySelector(selector){return selector.startsWith('script')?{dataset:{docRoot:'./'}}:{setAttribute(){}};},querySelectorAll(selector){return selector==='[data-set-theme]'?buttons:links;},addEventListener(type,fn){events[type]=fn;},startViewTransition(callback){const ready=deferred(),updated=deferred(),finished=deferred();const transition={ready:ready.promise,updateCallbackDone:updated.promise,finished:finished.promise,skipped:false,apply(){callback();updated.resolve();ready.resolve();},skipTransition(){this.skipped=true;ready.reject(new Error('skipped'));if(root.dataset.theme!==h.requested)callback();updated.resolve();finished.resolve();}};h.transitions.push(transition);return transition;}};
  const window={innerWidth:width,innerHeight:height,visualViewport:{addEventListener(type,fn){events['visual:'+type]=fn;}},addEventListener(type,fn){events[type]=fn;}};
  document.elementFromPoint=()=>({closest:()=>links[0]});
  const h={root,links,buttons,events,saved,location,animations:[],transitions:[],requested:'dark',clicks:0};
  const context={document,window,location,URL,requestAnimationFrame:fn=>setImmediate(fn),history:{state:{kept:true},replaceState(state,unused,url){assert.deepEqual(state,{kept:true});location.href=String(url);}},localStorage:{setItem(name,value){saved[name]=value;}},matchMedia:()=>({matches:false})};
  vm.runInNewContext(source+'\ninitializeTheme();',context);
  h.click=theme=>{h.requested=theme;const button=buttons.find(b=>b.dataset.setTheme===theme);button.events.pointerdown({clientX:width-76,clientY:31});return button.events.click({detail:1,clientX:width-76,clientY:31});};
  return h;
}
// The native update callback may be deferred while the user follows a real document link.
const desktop=harness();const switching=desktop.click('light');
assert.equal(new URL(desktop.links[0].href).searchParams.get('theme'),'light','document links carry requested theme before native snapshot callback');
assert.equal(new URL(desktop.location.href).searchParams.get('theme'),'light','reload/share URL carries selected theme');
assert.equal(desktop.saved['nodeforge-docs-theme'],'light');
desktop.transitions[0].apply();await tick();
desktop.events.resize();await switching;
assert.ok(desktop.animations[0].cancelled,'resize cancels the circular clip animation as well as the snapshot transition');
assert.equal(desktop.root.dataset.theme,'light');
assert.equal(desktop.buttons[1].attrs['aria-pressed'],'true');
assert.ok(!('themeReveal' in desktop.root.dataset));

const navigation=harness();const navigating=navigation.click('light');navigation.transitions[0].apply();await tick();
await navigation.events.click({target:navigation.root,clientX:200,clientY:500});await navigating;await tick();
assert.equal(navigation.clicks,1,'a root-snapshot click reaches the original link once after the transition ends');

const mobile=harness(390,844);const first=mobile.click('light');mobile.transitions[0].apply();await tick();
const frames=mobile.animations[0].keyframes.clipPath;
const radius=Math.hypot(Math.max(314,390-314),Math.max(31,844-31));
assert.equal(frames[0],'circle(0px at 314px 31px)');
assert.equal(frames[1],`circle(${radius}px at 314px 31px)`,'touch circle reaches the farthest viewport corner');
mobile.click('dark');await first;await tick();
assert.ok(mobile.animations[0].cancelled,'latest tap interrupts old animation');
mobile.transitions[1].apply();await tick();mobile.events['visual:resize']();await tick();
assert.equal(mobile.root.dataset.theme,'dark');
assert.equal(mobile.buttons[0].attrs['aria-pressed'],'true');
assert.equal(new URL(mobile.links[0].href).searchParams.get('theme'),'dark');
assert.ok(!('themeReveal' in mobile.root.dataset));
console.log('Theme checks passed: deferred navigation, reload URL, resize cleanup, touch geometry and latest selection');
