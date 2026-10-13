// Marked owns Markdown parsing; this module only wraps local author-controlled pages.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
import {marked} from '../Tools/NodeForgeDocs/Marked.mjs';
import {createThemeRenderer} from '../Tools/NodeForgeDocs/HeroPreview/Renderer.mjs';
const repositoryRoot=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const argumentsList=process.argv.slice(2);
let job;
if(argumentsList.length){
  if(![2,4].includes(argumentsList.length)||argumentsList[0]!=='--bundle'||(argumentsList.length===4&&argumentsList[2]!=='--output'))throw Error('Usage: node Scripts/RenderNodeForgeDocsHero.mjs --bundle <source-directory> [--output <new-candidate-directory>]; or provide a render job on stdin');
  const sourceRoot=path.resolve(argumentsList[1]);
  const root=argumentsList.length===4?path.resolve(argumentsList[3]):sourceRoot;
  if(!sourceRoot.startsWith(repositoryRoot+path.sep)||!root.startsWith(repositoryRoot+path.sep))throw Error('Source and candidate must stay inside project');
  const manifestPath=path.join(sourceRoot,'Manifest.json');
  const manifest=JSON.parse(fs.readFileSync(manifestPath,'utf8'));
  if(['frozen','released'].includes(manifest.publicationStatus)&&root===sourceRoot)throw Error('Published documentation is immutable during rendering; provide --output');
  if(root!==sourceRoot){
    if(root.startsWith(sourceRoot+path.sep)||sourceRoot.startsWith(root+path.sep))throw Error('Source and candidate must be separate directories');
    if(fs.existsSync(root))throw Error('Output already exists; use a new candidate directory');
  }
  job={root,bundleId:manifest.bundleId,sourceBundle:path.relative(repositoryRoot,sourceRoot).split(path.sep).join('/'),sourceManifestSha256:crypto.createHash('sha256').update(fs.readFileSync(manifestPath)).digest('hex'),pages:manifest.pages.map(page=>{
    const source=path.resolve(repositoryRoot,page.path);
    if(!source.startsWith(sourceRoot+path.sep))throw Error('Source outside selected bundle');
    const text=fs.readFileSync(source,'utf8');
    const bytes=fs.readFileSync(source);
    if(bytes.length!==page.bytes||crypto.createHash('sha256').update(bytes).digest('hex')!==page.sha256)throw Error('Manifest content mismatch: '+page.path);
    return {pageId:page.pageId,title:(text.match(/^#\s+(.+)$/m)?.[1]||page.title).trim(),source:page.path,output:path.relative(sourceRoot,source).split(path.sep).join('/'),text};
  })};
  if(root!==sourceRoot){
    fs.mkdirSync(root,{recursive:true});
    fs.copyFileSync(manifestPath,path.join(root,'Manifest.json'));
    for(const page of job.pages){const target=path.join(root,page.output);fs.mkdirSync(path.dirname(target),{recursive:true});fs.copyFileSync(path.join(repositoryRoot,page.source),target);}
    for(const artifact of manifest.artifacts){
      const source=path.resolve(repositoryRoot,artifact.path);
      if(!source.startsWith(sourceRoot+path.sep))throw Error('Public artifact outside source bundle');
      const bytes=fs.readFileSync(source);
      if(bytes.length!==artifact.bytes||crypto.createHash('sha256').update(bytes).digest('hex')!==artifact.sha256)throw Error('Manifest artifact mismatch: '+artifact.path);
      const target=path.join(root,path.relative(sourceRoot,source));fs.mkdirSync(path.dirname(target),{recursive:true});fs.copyFileSync(source,target);
    }
  }
}else{job=JSON.parse(fs.readFileSync(0,'utf8'));}
if(!path.resolve(job.root).startsWith(repositoryRoot+path.sep))throw Error('Render root outside project');
const manifestPath=path.join(job.root,'Manifest.json');
if(fs.existsSync(manifestPath)&&['frozen','released'].includes(JSON.parse(fs.readFileSync(manifestPath,'utf8')).publicationStatus)&&!job.sourceBundle)throw Error('Published documentation requires a separate source and candidate');
const escape=s=>String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
let headings=[];
marked.use({renderer:{
  html({text}) {return escape(text);},
  heading({tokens,depth,text}) {
    const id='section-'+headings.length;
    headings.push({id,depth,title:text.replace(/\[([^\]]+)\]\([^)]*\)/g,'$1').replace(/[*`]/g,'')});
    return `<h${depth} id="${id}">${this.parser.parseInline(tokens)}</h${depth}>\n`;
  }
}});
// The theme owns presentation and navigation; Marked retains content parsing.
const theme=createThemeRenderer(job);
for(const page of job.pages){
  const target=path.resolve(job.root,page.output.replace(/\.md$/,'.html'));
  if(!target.startsWith(path.resolve(job.root)+path.sep))throw Error('Render output outside package');
  const base=path.dirname(target);
  headings=[];
  const html=marked.parse(page.text,{gfm:true,walkTokens(token){
    if(token.type==='link' && !/^[a-z]+:/i.test(token.href)) {
      const destination=path.resolve(base,token.href.split('#')[0]);
      if(destination.startsWith(path.resolve(job.root)+path.sep))token.href=token.href.replace(/\.md(?=$|#)/,'.html');
    }
  }});
  theme.render(page,html,headings);
}
if(job.pages.some(page=>page.pageId==='Readme'))fs.copyFileSync(path.join(job.root,'Readme.html'),path.join(job.root,'Index.html'));
if(job.pages.some(page=>page.pageId==='UserReadme')){
  fs.copyFileSync(path.join(job.root,'User/Readme.html'),path.join(job.root,'User/Index.html'));
}
const fingerprint=file=>{const bytes=fs.readFileSync(file);return {path:path.relative(repositoryRoot,file).split(path.sep).join('/'),sha256:crypto.createHash('sha256').update(bytes).digest('hex'),bytes:bytes.length};};
const htmlFiles=job.pages.map(page=>path.resolve(job.root,page.output.replace(/\.md$/,'.html')));
if(job.pages.some(page=>page.pageId==='UserReadme'))htmlFiles.push(path.join(job.root,'User/Index.html'));
if(job.pages.some(page=>page.pageId==='Readme'))htmlFiles.push(path.join(job.root,'Index.html'));
const themeDirectory=path.join(repositoryRoot,'Tools/NodeForgeDocs/HeroPreview');
const layoutManifest={format:'NodeForgeDocsLayout1',themeVersion:theme.themeVersion,bundleId:job.bundleId,renderedAt:new Date().toISOString(),
  sourceBundle:job.sourceBundle,sourceManifestSha256:job.sourceManifestSha256,pageMap:job.pages.map(page=>({pageId:page.pageId,source:page.source,markdown:page.output,html:page.output.replace(/\.md$/,'.html')})),
  scope:'Presentation only; not a release or business acceptance',browserVisualAcceptance:'notEstablishedByRenderer',
  sourceFiles:[fingerprint(fileURLToPath(import.meta.url)),...theme.sourceFiles.map(file=>fingerprint(path.join(themeDirectory,file)))],
  markdown:job.pages.map(page=>fingerprint(path.resolve(job.root,page.output))),htmlArtifacts:htmlFiles.map(fingerprint),
  assets:theme.assets.map(asset=>fingerprint(path.resolve(job.root,asset.path)))};
const layoutPath=path.join(job.root,'LayoutManifest.json');
fs.writeFileSync(layoutPath,JSON.stringify(layoutManifest,null,2)+'\n','utf8');
process.stdout.write(JSON.stringify({renderedPages:job.pages.length,offline:true,externalAssets:0,themeVersion:theme.themeVersion,assets:theme.assets,themeSourceFiles:theme.sourceFiles,layoutManifest:fingerprint(layoutPath)}));
