"""Export declared documentation files as a portable static website."""
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit, urlunsplit
import argparse
import hashlib
import json
import posixpath
import re
import shutil
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class Page(HTMLParser):
    def __init__(self, text):
        super().__init__()
        self.refs, self.ids = [], set()
        self.feed(text)

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if attrs.get('id'):
            self.ids.add(attrs['id'])
        for key in ('href', 'src'):
            if attrs.get(key):
                self.refs.append((tag, key, attrs[key]))


def write_json(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bundle', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--resume', action='store_true', help='Resume an incomplete export with matching provenance')
    parser.add_argument('--reviewed-candidate', help='Exact static review and public attachment derivatives for a finite candidate; not a hosted deployment or final acceptance')
    args = parser.parse_args()
    bundle, output = (ROOT / args.bundle).resolve(), (ROOT / args.output).resolve()
    if not bundle.is_relative_to(ROOT) or not output.is_relative_to(ROOT):
        raise ValueError('Bundle and output must stay inside this project')
    if output.with_suffix('.zip').exists() or (output.exists() and not args.resume):
        raise FileExistsError('Output already exists; no existing delivery is overwritten')
    if output.exists():
        for name in ('Manifest.json', 'LayoutManifest.json'):
            saved = output / 'Provenance' / name
            if not saved.is_file() or digest(saved) != digest(bundle / name):
                raise ValueError('Incomplete export provenance does not match the selected bundle')
    manifest = json.loads((bundle / 'Manifest.json').read_text(encoding='utf-8'))
    layout = json.loads((bundle / 'LayoutManifest.json').read_text(encoding='utf-8'))
    review = None
    if args.reviewed_candidate:
        review_path = (ROOT / args.reviewed_candidate).resolve()
        if not review_path.is_relative_to(ROOT / 'Reports'):
            raise ValueError('Candidate review must be project-owned evidence')
        review = json.loads(review_path.read_text(encoding='utf-8'))
        if review.get('canAdvance') is not True or review.get('manifestSha256') != digest(bundle / 'Manifest.json'):
            raise ValueError('Exact candidate static review required')
    released_source = layout.get('sourceBundle')
    if manifest['publicationStatus'] == 'released':
        source_bundle = (ROOT / released_source).resolve() if released_source else bundle
        if source_bundle == bundle or not source_bundle.is_relative_to(ROOT) or digest(source_bundle / 'Manifest.json') != layout.get('sourceManifestSha256'):
            raise ValueError('Released input must be rendered into a separate candidate with exact source provenance')
    elif manifest['publicationStatus'] != 'frozen' and review is None:
        raise ValueError('Select a frozen bundle, a reviewed candidate, or a separately rendered released source')
    for section in ('markdown', 'htmlArtifacts', 'assets'):
        for item in layout[section]:
            if digest(ROOT / item['path']) != item['sha256']:
                raise ValueError('Frozen content hash mismatch: ' + item['path'])
    original_hashes = {p.relative_to(ROOT).as_posix(): digest(p) for p in bundle.rglob('*') if p.is_file()}
    output.mkdir(parents=True, exist_ok=args.resume)
    site = output / 'Site'
    site.mkdir(exist_ok=args.resume)
    mapped, origin, external = {}, {}, set()

    def copy_file(source, target):
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        origin[target.relative_to(output).as_posix()] = source.relative_to(ROOT).as_posix()

    def target_for(source):
        if source.is_relative_to(bundle):
            relative = source.relative_to(bundle)
            if relative.name == 'Index.html':
                relative = relative.with_name('index.html')
            return site / relative
        return site / 'Attachments' / source.relative_to(ROOT)

    # Public files are declared inputs, never a recursive copy of the bundle.
    public_sources = { (ROOT / item['path']).resolve() for section in ('markdown','htmlArtifacts','assets') for item in layout[section] }
    for artifact in manifest['artifacts']:
        original = (ROOT / artifact['path']).resolve()
        source = bundle / original.relative_to(source_bundle) if manifest['publicationStatus'] == 'released' else original
        if digest(source) != artifact['sha256']:
            raise ValueError('Public artifact hash mismatch: ' + artifact['path'])
        public_sources.add(source.resolve())
    for source in sorted(public_sources):
        if not source.is_relative_to(bundle) or not source.is_file():
            raise ValueError('Declared public file outside rendered bundle: ' + str(source))
        target = target_for(source)
        mapped[source] = target
        copy_file(source, target)
    for name in ('Manifest.json','LayoutManifest.json'):
        copy_file(bundle / name, output / 'Provenance' / name)
    for source in sorted(p for p in public_sources if p.suffix == '.html'):
        for tag, key, href in Page(source.read_text(encoding='utf-8')).refs:
            url = urlsplit(href)
            if url.scheme or url.netloc or not url.path:
                continue
            destination = (source.parent / unquote(url.path)).resolve()
            if not destination.is_relative_to(bundle):
                external.add(destination)
    if external:
        raise ValueError('Undeclared external attachment; replace with a public source link before export: ' + str(sorted(external)[0]))

    rewrites = []
    document_sources = sorted(p for p in public_sources if p.suffix in ('.html','.md'))
    for source in document_sources:
        target = mapped[source.resolve()]
        source_text = source.read_text(encoding='utf-8')
        text = source_text

        def portable(href):
            url = urlsplit(href)
            if url.scheme or url.netloc or not url.path:
                return href
            old = (source.parent / unquote(url.path)).resolve()
            if old not in mapped:
                return href
            new = mapped[old]
            if old.is_dir():
                new = new / 'index.html'
            relative = posixpath.relpath(new.as_posix(), target.parent.as_posix())
            return urlunsplit(('', '', relative, url.query, url.fragment))

        if source.suffix == '.html':
            text = re.sub(r'\b(href|src)="([^"]*)"', lambda m: m[1] + '="' + portable(m[2]) + '"', text)
        else:
            text = re.sub(r'(\[[^\]]*\]\()([^)]*)(\))', lambda m: m[1] + portable(m[2]) + m[3], text)
        if text != source_text:
            target.write_text(text, encoding='utf-8')
            rewrites.append(target.relative_to(output).as_posix())

    # Keep existing authoring code available without uploading it with the website.
    for item in layout['sourceFiles']:
        source = ROOT / item['path']
        if not source.is_file() or digest(source) != item['sha256']:
            raise ValueError('Authoring source differs from this frozen layout: ' + item['path'])
        copy_file(source, output / 'Source' / item['path'])
    for name in ('Marked.mjs', 'License.md', 'ThirdPartyNotices.md', 'DependencyManifest.json'):
        source = ROOT / 'Tools/NodeForgeDocs' / name
        copy_file(source, output / 'Source/Tools/NodeForgeDocs' / name)

    instructions = f'''# NodeForge 文档网站发布包

本包对应 NodeForge v{manifest['buildId']}。网页主题{layout['themeVersion']}沿既定设计保持；网站内容修订与产品发行版本分别记录。

## 发布

将 `Site` 文件夹内的全部内容上传到静态网站的发布目录，默认首页为 `index.html`。上传后目录结构保持不变，首页、User、Ai、Developer、Reference、Version、Assets 和三份公开 JSON 都必须一起上传。可以部署在域名根目录或子目录；链接使用相对路径。服务器使用 UTF-8，正确提供 HTML/CSS/JS/SVG/PNG/Markdown/JSON 文件类型，默认文档设为小写 `index.html`；无需单页应用路由回退。推荐使用 HTTPS，以便浏览器提供完整剪贴板能力。

网站无需安装依赖、编译、数据库或后端。暗亮主题、连续蓝图、圆形主题切换、Pico 滚动条、站内搜索和复制随原页面保留；站点没有外部脚本、样式或素材依赖。发布集合来自明确的页面、资源和公开 JSON 清单，不沿链接自动带入工程文件、报告或配置。只上传 Site；Source 和 Provenance 用于本地维护。

## 内容与后续更新

- Site：全部网页、Markdown 正文、搜索索引、主题和品牌素材，以及页面直接引用的审核附件。仅上传本目录。
- Source：对应布局的模板、渲染器、固定版本Markdown解析器及许可，供后续设计维护；不需要上传。
- Provenance：所选公开资料元数据原字节，排除私有Generation回执；用于追溯，不是部署后的网页哈希清单。
- PackageManifest.json：本次交付文件清单、SHA-256、来源及链接迁移记录。

当前固定磨砂参数为背景10%、渐变0%→1%、卡片磨砂100%、卡片透明度2%，首页无调节条和文字黑色衬底；中央正文卡片上下顶满。不要在发布时重新启用试调控件。

正文的 Markdown 与其 HTML 一一对应。修改 Markdown 后需要重新生成 HTML 和 SearchIndex.js，不能只改 Markdown 或只替换搜索索引。原项目维护入口是 Tools/NodeForgeDocs/HeroPreview/Readme.md；采用 Scripts/RenderNodeForgeDocsHero.mjs 渲染新的候选文档目录，冻结包不可原位生成。本包 Source 是维护源的有限导出，完整版本文档生成仍使用原项目的发布工作流。更新站点时上传同一候选生成的整套 Site；缓存刷新需包含主题资源与搜索索引。

本导出器核压缩包完整性、路径大小写、链接/锚点与搜索索引。实际浏览器必要样本、HTTP与公开闭包由本期Review另行记录；不把静态检查当浏览器通过。未上传托管平台或公开上线。
'''
    (output / 'Readme.md').write_text(instructions, encoding='utf-8')
    pages = {p.relative_to(site).as_posix(): Page(p.read_text(encoding='utf-8')) for p in site.rglob('*.html')}
    names = {p.relative_to(site).as_posix() for p in site.rglob('*') if p.is_file()}
    references, asset_refs = 0, 0
    for name, page in pages.items():
        for tag, key, ref in page.refs:
            url = urlsplit(ref)
            if url.scheme or url.netloc:
                if tag in ('script', 'link', 'img'):
                    raise ValueError('External runtime asset: ' + ref)
                continue
            normalized = posixpath.normpath(posixpath.join(posixpath.dirname(name), unquote(url.path))) if url.path else name
            if normalized not in names:
                raise ValueError('Missing or case-mismatched reference: ' + name + ' => ' + ref)
            if url.fragment and normalized in pages and unquote(url.fragment) not in pages[normalized].ids:
                raise ValueError('Missing HTML anchor: ' + name + ' => ' + ref)
            references += 1
            asset_refs += tag in ('script', 'link', 'img')
    index_path = site / 'Assets/NodeForgeDocs/SearchIndex.js'
    index_text = index_path.read_text(encoding='utf-8')
    search = json.loads(index_text.removeprefix('window.NodeForgeDocsIndex=').strip().removesuffix(';'))
    for page in search['pages']:
        if page['href'] not in names:
            raise ValueError('Search result missing or case mismatched: ' + page['href'])
    for relative, expected in original_hashes.items():
        if digest(ROOT / relative) != expected:
            raise ValueError('Frozen original changed during export: ' + relative)
    files = []
    for path in sorted(p for p in output.rglob('*') if p.is_file()):
        relative = path.relative_to(output).as_posix()
        entry = {'path': relative, 'bytes': path.stat().st_size, 'sha256': digest(path)}
        if relative in origin:
            entry['source'] = origin[relative]
            entry['sourceSha256'] = digest(ROOT / origin[relative])
        files.append(entry)
    from datetime import date
    package = {'format':'NodeForgeDocsWebPackage1', 'date':date.today().isoformat(), 'bundle':args.bundle,
               'bundleId':manifest['bundleId'], 'themeVersion':layout['themeVersion'],
               'sourceManifestSha256':digest(bundle/'Manifest.json'), 'sourceLayoutSha256':digest(bundle/'LayoutManifest.json'),
               'entry':'Site/index.html', 'publicDirectory':'Site', 'sourceFilesProtected':len(original_hashes),
               'htmlFiles':len(pages), 'searchPages':len(search['pages']), 'htmlReferencesChecked':references,
               'runtimeAssetReferencesChecked':asset_refs, 'externalDependencyTargets':len(external),
               'rewrittenFiles':rewrites, 'files':files, 'aiBrowserVisualTest':'notRun', 'hostedDeployment':'notRun',
               'candidateReviewProvided':review is not None, 'privateGenerationReceiptExcluded':True}
    write_json(output / 'PackageManifest.json', package)
    archive = output.with_suffix('.zip')
    with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for path in sorted(p for p in output.rglob('*') if p.is_file()):
            z.write(path, path.relative_to(output).as_posix())
    with zipfile.ZipFile(archive) as z:
        if z.testzip() is not None:
            raise ValueError('ZIP integrity check failed')
        for item in files:
            if hashlib.sha256(z.read(item['path'])).hexdigest() != item['sha256']:
                raise ValueError('ZIP entry hash mismatch: ' + item['path'])
        if z.read('PackageManifest.json') != (output / 'PackageManifest.json').read_bytes():
            raise ValueError('ZIP manifest differs')
    print(json.dumps({'status':'passed', 'archive':archive.relative_to(ROOT).as_posix(), 'sha256':digest(archive),
                      'bytes':archive.stat().st_size, 'publicFiles':len(names), 'htmlFiles':len(pages),
                      'searchPages':len(search['pages']), 'referencesChecked':references,
                      'externalDependencyTargets':len(external), 'originalsUnchanged':len(original_hashes),
                      'zipEntries':len(files)+1, 'rewrittenFiles':len(rewrites)}, ensure_ascii=False))


if __name__ == '__main__':
    main()
