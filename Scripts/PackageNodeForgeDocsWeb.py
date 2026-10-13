"""Export a frozen documentation bundle as a portable static website."""
from html import escape
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
    review, attachments = None, {}
    if args.reviewed_candidate:
        review_path = (ROOT / args.reviewed_candidate).resolve()
        if not review_path.is_relative_to(ROOT / 'Reports'):
            raise ValueError('Candidate review must be project-owned evidence')
        review = json.loads(review_path.read_text(encoding='utf-8'))
        if review.get('canAdvance') is not True or review.get('manifestSha256') != digest(bundle / 'Manifest.json'):
            raise ValueError('Exact candidate static review required')
        for row in review['publicAttachments']:
            source, public = (ROOT / row['source']).resolve(), (ROOT / row['public']).resolve()
            if not source.is_relative_to(ROOT / 'Contracts') or not public.is_relative_to(ROOT / 'Reports') or digest(public) != row['sha256'] or source in attachments:
                raise ValueError('Unique reviewed public contract derivative required')
            attachments[source] = public
    if manifest['publicationStatus'] != 'frozen' and review is None:
        raise ValueError('Select an explicitly frozen documentation bundle')
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

    for source in bundle.rglob('*'):
        if not source.is_file():
            continue
        if source.name == 'Generation.json':
            continue  # Internal renderer input/report can contain private executable paths.
        if source.parent == bundle and source.suffix == '.json':
            copy_file(source, output / 'Provenance' / source.name)
            if review is not None:
                target = target_for(source)
                mapped[source.resolve()] = target
                copy_file(source, target)
        else:
            target = target_for(source)
            mapped[source.resolve()] = target
            copy_file(source, target)
    for source in bundle.rglob('*.html'):
        for tag, key, href in Page(source.read_text(encoding='utf-8')).refs:
            url = urlsplit(href)
            if url.scheme or url.netloc or not url.path:
                continue
            destination = (source.parent / unquote(url.path)).resolve()
            if not destination.is_relative_to(bundle):
                external.add(destination)
    for source in sorted(external):
        if not source.is_relative_to(ROOT) or not source.exists():
            raise ValueError('Reference outside project or missing: ' + str(source))
        if review is not None and source not in attachments:
            raise ValueError('Candidate attachment has no reviewed public derivative: ' + str(source))
        target = target_for(source)
        mapped[source] = target
        if source.is_dir():
            files = sorted(source.glob('*.json'))
            if not files or any(p.is_dir() for p in source.iterdir()):
                raise ValueError('Directory reference requires a reviewed flat JSON evidence folder')
            for file in files:
                mapped[file.resolve()] = target / file.name
                copy_file(file, target / file.name)
            home = posixpath.relpath((site / 'index.html').as_posix(), target.as_posix())
            links = ''.join('<li><a href="' + escape(p.name, quote=True) + '">' + escape(p.name) + '</a></li>' for p in files)
            (target / 'index.html').write_text('<!doctype html><html lang="zh-CN"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>原始调用记录 · NodeForge</title><style>body{font:16px/1.8 system-ui,sans-serif;max-width:760px;margin:64px auto;padding:0 24px;background:#141414;color:#f1eee5}a{color:#e86e50}li{padding:5px 0}@media(prefers-color-scheme:light){body{background:#f8f6f0;color:#232522}}</style></head><body><a href="' + home + '">← 文档首页</a><h1>原始调用记录</h1><p>Build122 MCP P3 历史证据，保留原文件身份。下载后可查看完整 JSON；历史记录不表示当前运行状态。</p><ul>' + links + '</ul></body></html>\n', encoding='utf-8')
        else:
            if source.suffix not in ('.md', '.json'):
                raise ValueError('Unreviewed attachment type: ' + str(source))
            copy_file(attachments.get(source, source), target)

    rewrites = []
    document_sources = list(bundle.rglob('*.html')) + list(bundle.rglob('*.md'))
    document_sources += [p for p in external if p.is_file() and p.suffix in ('.md', '.html')]
    for source in document_sources:
        target = mapped[source.resolve()]
        source_text = attachments.get(source, source).read_text(encoding='utf-8')
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

本包对应 {manifest['buildId']} / 桥{manifest['bridgeVersion']} / {manifest['bundleId']}。网页主题{layout['themeVersion']}沿既定设计保持；资料候选导出和最终交付验收分别记录。

## 发布

将 `Site` 文件夹内的全部内容上传到静态网站的发布目录，默认首页为 `index.html`。上传后目录结构保持不变，首页、User、Ai、Developer、Reference、Version、Assets、Attachments 都必须一起上传。可以部署在域名根目录或子目录；链接使用相对路径。服务器使用 UTF-8，正确提供 HTML/CSS/JS/SVG/PNG/Markdown/JSON 文件类型，默认文档设为小写 `index.html`；无需单页应用路由回退。推荐使用 HTTPS，以便浏览器提供完整剪贴板能力。

网站无需安装依赖、编译、数据库或后端。暗亮主题、连续蓝图、圆形主题切换、站内搜索和复制随原页面保留；站点没有外部脚本、样式或素材依赖。页面直接引用的允许附件在 Attachments；候选契约使用同规范内容的公开衍生稿，不带内部报告、真实配置或历史会话。源码定位说明区分交付包与维护库，不表示网站含完整UE源码或开发报告。

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
