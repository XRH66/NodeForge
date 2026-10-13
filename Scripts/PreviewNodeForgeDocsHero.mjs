// Independent design preview. Never edits a frozen documentation package or its bound sources.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
import {spawnSync} from 'node:child_process';

const repositoryRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const sourceRoot = path.join(repositoryRoot, 'Docs/Versions/Build122/Mcp2R1');
const outputRoot = path.join(repositoryRoot, 'Docs/Versions/Build122/Mcp2Design2');
const readJson = file => JSON.parse(fs.readFileSync(file, 'utf8'));
const digest = file => crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
const fingerprint = file => ({path:path.relative(repositoryRoot,file).split(path.sep).join('/'),sha256:digest(file),bytes:fs.statSync(file).size});
const manifestFile = path.join(sourceRoot, 'Manifest.json');
const manifest = readJson(manifestFile);
if (manifest.publicationStatus !== 'frozen') throw Error('Expected a frozen source baseline');
const previewManifest = path.join(outputRoot, 'Manifest.json');
if (fs.existsSync(outputRoot) && (!fs.existsSync(previewManifest) || readJson(previewManifest).format !== 'NodeForgeDesignPreview1' || readJson(previewManifest).publicationStatus !== 'preview')) throw Error('Refusing to overwrite an unrelated output');
fs.mkdirSync(outputRoot, {recursive:true});
const pages = manifest.pages.map(page => {
  const source = path.resolve(repositoryRoot, page.path);
  if (!source.startsWith(sourceRoot + path.sep)) throw Error('Source page outside frozen baseline');
  const output = path.relative(sourceRoot, source).split(path.sep).join('/');
  const text = fs.readFileSync(source, 'utf8');
  const destination = path.resolve(outputRoot, output);
  if (!destination.startsWith(outputRoot + path.sep)) throw Error('Preview path escaped output root');
  fs.mkdirSync(path.dirname(destination), {recursive:true});
  fs.copyFileSync(source, destination);
  return {pageId:page.pageId, output, title:text.match(/^#\s+(.+)$/m)?.[1] || page.title, text};
});
fs.writeFileSync(previewManifest, JSON.stringify({format:'NodeForgeDesignPreview1',publicationStatus:'preview',
  purpose:'Standalone presentation preview; activation is separately archived and recorded',bundleId:manifest.bundleId,
  sourceManifest:fingerprint(manifestFile), themeVersion:readJson(path.join(repositoryRoot,'Tools/NodeForgeDocs/HeroPreview/Config.json')).themeVersion,
  pages:pages.map(({pageId,title,output})=>({pageId,title,path:path.relative(repositoryRoot,path.join(outputRoot,output)).split(path.sep).join('/')}))}, null, 2)+'\n');
const render = spawnSync(process.execPath,[path.join(repositoryRoot,'Scripts/RenderNodeForgeDocsHero.mjs'),'--bundle',outputRoot],{encoding:'utf8',windowsHide:true});
if(render.status!==0)throw Error(render.stderr||'Preview render failed');
const result=JSON.parse(render.stdout);
process.stdout.write(JSON.stringify({...result,home:path.join(outputRoot,'Index.html'),sourceManifest:fingerprint(manifestFile)}));
