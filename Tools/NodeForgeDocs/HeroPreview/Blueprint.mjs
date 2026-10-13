// Decorative, code-native graph artwork; never a generated UE graph or capability claim.
const nodes = [
  {x:70,y:150,title:'Event BeginPlay',tone:'event',inputs:[],outputs:['Exec','Actor']},
  {x:405,y:180,title:'Branch',tone:'flow',inputs:['Exec','Condition'],outputs:['True','False']},
  {x:750,y:80,title:'Get Actor Location',tone:'function',inputs:['Target'],outputs:['Vector']},
  {x:1090,y:205,title:'Add Actor Offset',tone:'function',inputs:['Exec','Delta'],outputs:['Exec']},
  {x:1420,y:175,title:'Output',tone:'event',inputs:['Exec','Result'],outputs:[]},
  {x:220,y:470,title:'Time',tone:'value',inputs:[],outputs:['Seconds','Delta']},
  {x:570,y:430,title:'Make Vector',tone:'value',inputs:['X','Y','Z'],outputs:['Vector']},
  {x:940,y:510,title:'Multiply',tone:'flow',inputs:['A','B'],outputs:['Value']},
  {x:1350,y:495,title:'Set Parameter',tone:'function',inputs:['Target','Value'],outputs:['Exec']}
];
const wire=(x1,y1,x2,y2,type='value')=>{
  const distance=Math.max(70,Math.abs(x2-x1)*.48);
  const d=`M${x1} ${y1}C${x1+distance} ${y1} ${x2-distance} ${y2} ${x2} ${y2}`;
  return `<path class="bp-wire bp-wire-${type}" d="${d}"/><path class="bp-signal bp-wire-${type}" d="${d}"/>`;
};
const pin=(label,x,y,output)=>`<g><circle class="bp-pin ${label==='Exec'?'bp-pin-exec':'bp-pin-value'}" cx="${x}" cy="${y}" r="4"/><text class="bp-label" x="${output?x-12:x+12}" y="${y+4}" text-anchor="${output?'end':'start'}">${label}</text></g>`;
const node=n=>`<g class="bp-node bp-tone-${n.tone}" transform="translate(${n.x} ${n.y})"><rect class="bp-node-body" width="212" height="140" rx="9"/><rect class="bp-node-header" width="212" height="33" rx="9"/><rect class="bp-node-header" y="24" width="212" height="9"/><path class="bp-node-glyph" d="M15 11l8 5-8 5Z"/><text class="bp-title" x="31" y="21">${n.title}</text><path class="bp-node-divider" d="M0 33H212"/>${n.inputs.map((s,i)=>pin(s,12,59+i*27,false)).join('')}${n.outputs.map((s,i)=>pin(s,200,59+i*27,true)).join('')}<text class="bp-node-type" x="12" y="128">NODEFORGE</text></g>`;
export function blueprintScene(){
  const wires=[
    wire(0,236,70,209,'exec'),wire(282,209,405,239,'exec'),wire(617,239,1090,264,'exec'),wire(1302,264,1420,234,'exec'),wire(1632,234,1800,236,'exec'),
    wire(282,236,750,139),wire(962,139,1090,291),wire(432,529,570,489),wire(432,556,940,569),wire(782,489,940,596),wire(1152,569,1350,581),wire(1562,554,1800,570),wire(0,570,220,529),wire(782,489,405,266)
  ].join('');
  const row=`${wires}${nodes.map(node).join('')}<circle class="bp-reroute" cx="840" cy="395" r="6"/><circle class="bp-reroute" cx="1260" cy="410" r="6"/>`;
  const tile=`<svg class="blueprint-tile" viewBox="0 0 1800 1600" width="1800" height="1600" xmlns="http://www.w3.org/2000/svg" focusable="false">${row}<g transform="translate(0 800)">${row}</g></svg>`;
  return `<div class="hero-scene" aria-hidden="true"><div class="blueprint-plane"><div class="blueprint-track">${tile.repeat(4)}</div></div></div>`;
}
