"""Assemble Build137 Ai1 from the frozen ordinary ZIP and hash-locked runtime inputs."""
import argparse
import hashlib
import json
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VERSION = 'Build137Repair5Ai1'
DOCS = 'Docs/Versions/Build137/Mcp10R1'
EMBED_SHA = '15fea3c9367653a85086fe37216b4d1a1c78688fa5e1587e1db0b0f658856564'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def save(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')


def ref(root, path):
    return {'path': path.relative_to(root).as_posix(), 'bytes': path.stat().st_size, 'sha256': sha(path)}


def assemble(out, embed):
    if out.exists():
        raise ValueError('New output directory required; existing candidates are preserved')
    if sha(embed) != EMBED_SHA:
        raise ValueError('Official Python3.12.4 embed input changed')
    out.mkdir(parents=True)
    runtime = out / 'Tools/NodeForgeMcp/Runtime'
    runtime.mkdir(parents=True)
    with zipfile.ZipFile(embed) as archive:
        archive.extractall(runtime)
    dependencies = load(ROOT / 'Tools/NodeForgeMcp/DependencyManifest.json')
    wheels = []
    for row in dependencies['packages']:
        wheel = ROOT / row['wheel']
        if sha(wheel) != row['sha256']:
            raise ValueError('Dependency hash mismatch: ' + row['name'])
        with zipfile.ZipFile(wheel) as archive:
            for name, expected in row['licenseFilesInWheel'].items():
                if hashlib.sha256(archive.read(name)).hexdigest() != expected:
                    raise ValueError('Dependency license mismatch')
        wheels.append(str(wheel))
    subprocess.run([sys.executable, '-m', 'pip', 'install', '--no-index', '--no-deps', '--no-compile',
        '--target', str(runtime / 'Lib/site-packages'), *wheels], check=True, capture_output=True)
    for path in (runtime / 'Lib/site-packages').glob('*.dist-info/direct_url.json'):
        path.unlink()  # pip's builder-local wheel paths do not belong in the portable runtime.
    # A fixed _pth isolates imports from registry, PYTHONHOME, user/site Python and PATH.
    (runtime / 'python312._pth').write_text('python312.zip\n.\nLib/site-packages\n'
        'Lib/site-packages/win32\nLib/site-packages/win32/lib\n'
        'Lib/site-packages/pythonwin\nLib/site-packages/pywin32_system32\n../../../Scripts\nimport site\n', encoding='utf-8')
    (runtime / 'Lib/site-packages/sitecustomize.py').write_text(
        'import sys\nsys.dont_write_bytecode = True\n'
        'for stream in (sys.stdout, sys.stderr):\n'
        '    if hasattr(stream, "reconfigure"):\n'
        '        stream.reconfigure(encoding="utf-8")\n', encoding='utf-8')
    for folder in ['bin', 'Scripts']:
        # pip console launchers embed the builder interpreter; they are not used or shipped.
        generated = runtime / 'Lib/site-packages' / folder
        if generated.exists():
            for path in generated.rglob('*'):
                if path.is_file():
                    path.unlink()
            for path in sorted(generated.rglob('*'), key=lambda p: len(p.parts), reverse=True):
                if path.is_dir():
                    path.rmdir()
            generated.rmdir()
    for name in ['NodeForgeControl.py', 'NodeForgeEditorTransport.py', 'RunProcessGuard.py',
                 'PrepareNodeForgeInstallation.py', 'NodeForgeMcp.py', 'NodeForgeMcpBridge.py',
                 'NodeForgeAiInstall.py', 'NodeForgeAiCheck.py', 'NodeForgeAiLaunch.py', 'InstallNodeForgeAi.ps1']:
        destination = out / 'Scripts' / name
        destination.parent.mkdir(exist_ok=True)
        shutil.copyfile(ROOT / 'Scripts' / name, destination)
    for name in ['DependencyManifest.json', 'Requirements.lock', 'ThirdPartyNotices.md', 'ToolMetadata.json']:
        shutil.copyfile(ROOT / 'Tools/NodeForgeMcp' / name, out / 'Tools/NodeForgeMcp' / name)
    (out / 'Metadata').mkdir()
    shutil.copyfile(ROOT / 'Metadata/Operations.json', out / 'Metadata/Operations.json')
    old_root = ROOT / 'Docs/Versions/Build136/Mcp9R19'
    source = load(old_root / 'Manifest.json')
    new_root = out / DOCS
    new_root.mkdir(parents=True)
    pages = []
    for old in source['pages']:
        original = ROOT / old['path']
        relative = original.relative_to(old_root)
        destination = new_root / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        text = original.read_text(encoding='utf-8-sig')
        # Keep historical evidence identities explicit. Do not relabel old runtime tests as 137 tests.
        if old.get('kind') != 'node':
            text = ('> 当前接入身份：Build137 Repair5 / Ai1 / Mcp10R1；Windows x64官方UE5.3.2。'
                '正文功能说明继承Mcp9R19；Build136、原stage/PID/验收路径只表示历史证据。'
                '当前安装和连接统一见[AI安装](' + '../' * (len(relative.parts) - 1)
                + 'User/AiInstallation.md)。不要复制历史本机路径或环境配置。\n\n') + text
        text = text.replace('.html)', '.md)')
        destination.write_text(text, encoding='utf-8', newline='\n')
        pages.append({**old, **ref(out, destination)})
    # Current operational instructions replace inherited setup pages in this new bundle only.
    current = (ROOT / 'Docs/AiInstallation.md').read_text(encoding='utf-8')
    for page in pages:
        if page['pageId'] in ['UserAiInstallation', 'UserInstallation', 'AiPreparation']:
            destination = out / page['path']
            destination.write_text(current, encoding='utf-8', newline='\n')
            page.update(ref(out, destination))
    contract = load(ROOT / 'Tools/NodeForgeMcp/ToolContract.json')
    contract.update(bundleId='NodeForge-Build137-Mcp10R1', docsManifest=DOCS + '/Manifest.json')
    save(out / 'Tools/NodeForgeMcp/ToolContract.json', contract)
    artifacts = []
    for name in ['Catalog.json', 'Operations.json', 'GraphRequestSchema.json']:
        if name == 'Catalog.json':
            with zipfile.ZipFile(ROOT / 'Deliverables/NodeForgeBuild137Repair5PluginCandidate.zip') as ordinary:
                (new_root / name).write_bytes(ordinary.read('NodeForge/Metadata/Catalog.json'))
        else:
            shutil.copyfile(old_root / name, new_root / name)
        artifacts.append(ref(out, new_root / name))
    # Installation-only matching docs: existing node/product evidence stays historical.
    installation = load(ROOT / 'Releases/Build137Repair5/InstallationManifest.json')
    manifest = {key: source[key] for key in ['format', 'engine', 'adapter', 'graphProtocolVersion',
        'acceptedGraphVersions', 'counts', 'catalogDigest', 'readerEntrypoints']}
    manifest.update(bundleId=contract['bundleId'], buildId='Build137 Repair5', aiVersion=VERSION,
        bridgeVersion='0.3.6', binarySha256=installation['binarySha256'], editorApiVersion='1.0',
        profile='nodeforge.editor.v1', installationSchemaVersion=1, stage='.',
        reportNamespace='Reports/PluginControlStageTwo/AiAddon1', publicationStatus='candidateInstallation',
        fullUsagePackageComplete=True, validationOnly=False, pages=pages, artifacts=artifacts,
        authoringSources=[], htmlArtifacts=[], developerKnowledgeComplete=True,
        inheritedContentBundle=source['bundleId'], inheritedContentEvidence='Historical Build136 finite product checks; not relabeled as Build137 business checks',
        actualCodexBusinessAcceptance='notRunForThisAddon; real SDK installation check required',
        cliFiles={str(p.relative_to(out)).replace('\\', '/'): sha(p) for p in (out / 'Scripts').glob('*.py')
            if p.name in ['NodeForgeControl.py', 'NodeForgeEditorTransport.py', 'RunProcessGuard.py', 'PrepareNodeForgeInstallation.py']},
        bindings=[ref(out, p) for p in (out / 'Scripts').glob('*.py')
            if p.name not in ['NodeForgeControl.py', 'NodeForgeEditorTransport.py', 'RunProcessGuard.py', 'PrepareNodeForgeInstallation.py']]
            + [ref(out, out / 'Tools/NodeForgeMcp' / n) for n in ['DependencyManifest.json', 'Requirements.lock', 'ThirdPartyNotices.md']])
    save(new_root / 'Manifest.json', manifest)
    runtime_manifest = {'format': 'NodeForgeFixedRuntime1', 'pythonVersion': '3.12.4', 'platform': 'Windows x64',
        'sourceUrl': 'https://www.python.org/ftp/python/3.12.4/python-3.12.4-embed-amd64.zip',
        'sourceSha256': EMBED_SHA, 'officialReleaseMd5': '8db759b337ac4f6966f52b3662c05dd7',
        'pythonLicense': 'Tools/NodeForgeMcp/Runtime/LICENSE.txt', 'sdkVersion': dependencies['sdkVersion'],
        'isolatedPth': True, 'systemPythonRequired': False,
        'files': [ref(out, p) for p in sorted(runtime.rglob('*')) if p.is_file()]}
    save(out / 'Tools/NodeForgeMcp/RuntimeManifest.json', runtime_manifest)
    # Remove pip's relocation-specific record entries; package files themselves are hash-bound.
    plugin = ROOT / 'Deliverables/NodeForgeBuild137Repair5PluginCandidate.zip'
    if sha(plugin) != '3a7143bcf208b8e7cc7279c2a919950b30d2448d358abed70714c60d3d44bf7e':
        raise ValueError('Frozen ordinary plugin changed')
    addon = {'format': 'NodeForgeAiAddon1', 'aiVersion': VERSION, 'buildId': 'Build137 Repair5',
        'binarySha256': installation['binarySha256'], 'installationManifest': installation,
        'engineVersion': '5.3.2', 'platform': 'Windows x64', 'bridgeVersion': '0.3.6', 'sdkVersion': '2.3.0',
        'docsBundle': contract['bundleId'], 'sharedCoreIncluded': False, 'fixedRuntimeIncluded': True,
        'pluginAsset': {'name': plugin.name, 'sha256': sha(plugin),
            'url': 'https://github.com/XRH66/NodeForge/releases/download/build137-repair5/' + plugin.name,
            'apiUrl': 'https://api.github.com/repos/XRH66/NodeForge/releases/assets/635869031'},
        'files': [ref(out, p) for p in sorted(out.rglob('*')) if p.is_file()]}
    save(out / 'AiPackageManifest.json', addon)
    archive = out.with_suffix('.zip')
    with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for p in sorted(out.rglob('*')):
            if p.is_file():
                z.write(p, p.relative_to(out).as_posix())
    return {'status': 'assembled', 'aiVersion': VERSION, 'zip': str(archive), 'sha256': sha(archive),
        'bytes': archive.stat().st_size, 'fileCount': len(addon['files']) + 1}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', required=True)
    parser.add_argument('--embed', required=True)
    args = parser.parse_args()
    print(json.dumps(assemble(Path(args.out).resolve(), Path(args.embed).resolve())))
