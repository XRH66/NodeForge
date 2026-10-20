"""Assemble the matching AI package with its fixed runtime and published documentation."""
import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VERSION = '1.1.2'
DOCS = 'Docs/Versions/v1.1.2'
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


def assemble(out, embed, plugin, plugin_asset_id, version=VERSION):
    if not re.fullmatch(r'\d+\.\d+\.\d+', version):
        raise ValueError('Release version required')
    docs = 'Docs/Versions/v' + version
    release = ROOT / 'Releases' / version
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
    for name in ['NodeForgeControl.py', 'NodeForgeWindowsProcesses.py', 'NodeForgeEditorTransport.py', 'RunProcessGuard.py',
                 'PrepareNodeForgeInstallation.py', 'NodeForgeMcp.py', 'NodeForgeMcpBridge.py',
                 'NodeForgeAiInstall.py', 'NodeForgeAiCheck.py', 'NodeForgeAiLaunch.py', 'InstallNodeForgeAi.ps1']:
        destination = out / 'Scripts' / name
        destination.parent.mkdir(exist_ok=True)
        shutil.copyfile(ROOT / 'Scripts' / name, destination)
    for name in ['DependencyManifest.json', 'Requirements.lock', 'ThirdPartyNotices.md', 'ToolMetadata.json']:
        shutil.copyfile(ROOT / 'Tools/NodeForgeMcp' / name, out / 'Tools/NodeForgeMcp' / name)
    (out / 'Metadata').mkdir()
    shutil.copyfile(ROOT / 'Metadata/Operations.json', out / 'Metadata/Operations.json')
    source = load(ROOT / docs / 'Manifest.json')
    new_root = out / docs
    new_root.mkdir(parents=True)
    # Keep the manual's third-party license links usable offline.
    if (ROOT / docs / 'Assets').is_dir():
        shutil.copytree(ROOT / docs / 'Assets', new_root / 'Assets')
    pages = []
    for page in source['pages']:
        original = ROOT / page['path']
        destination = out / page['path']
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(original, destination)
        pages.append({**page, **ref(out, destination)})
    contract = load(release / 'ToolContract.json')
    save(out / 'Tools/NodeForgeMcp/ToolContract.json', contract)
    artifacts = []
    for name in ['Catalog.json', 'Operations.json', 'GraphRequestSchema.json']:
        shutil.copyfile(ROOT / docs / name, new_root / name)
        artifacts.append(ref(out, new_root / name))
    installation = load(release / 'InstallationManifest.json')
    with zipfile.ZipFile(plugin) as ordinary:
        if json.loads(ordinary.read('NodeForge/InstallationManifest.json')) != installation:
            raise ValueError('Plugin installation metadata does not match this release')
    manifest = {key: source[key] for key in ['format', 'engine', 'adapter', 'graphProtocolVersion',
        'acceptedGraphVersions', 'counts', 'catalogDigest', 'readerEntrypoints']}
    manifest.update(bundleId=contract['bundleId'], buildId=version, aiVersion=version,
        bridgeVersion=contract['bridgeVersion'], binarySha256=installation['binarySha256'], editorApiVersion='1.0',
        profile='nodeforge.editor.v1', installationSchemaVersion=1, stage='.',
        reportNamespace='Reports/PluginControlStageTwo/NodeForge', publicationStatus='released',
        fullUsagePackageComplete=True, validationOnly=False, pages=pages, artifacts=artifacts,
        authoringSources=[], htmlArtifacts=[], developerKnowledgeComplete=True,
        cliFiles={str(p.relative_to(out)).replace('\\', '/'): sha(p) for p in (out / 'Scripts').glob('*.py')
            if p.name in ['NodeForgeControl.py', 'NodeForgeWindowsProcesses.py', 'NodeForgeEditorTransport.py', 'RunProcessGuard.py', 'PrepareNodeForgeInstallation.py']},
        bindings=[ref(out, p) for p in (out / 'Scripts').glob('*.py')
            if p.name not in ['NodeForgeControl.py', 'NodeForgeWindowsProcesses.py', 'NodeForgeEditorTransport.py', 'RunProcessGuard.py', 'PrepareNodeForgeInstallation.py']]
            + [ref(out, out / 'Tools/NodeForgeMcp' / n) for n in ['DependencyManifest.json', 'Requirements.lock', 'ThirdPartyNotices.md']])
    save(new_root / 'Manifest.json', manifest)
    runtime_manifest = {'format': 'NodeForgeFixedRuntime1', 'pythonVersion': '3.12.4', 'platform': 'Windows x64',
        'sourceUrl': 'https://www.python.org/ftp/python/3.12.4/python-3.12.4-embed-amd64.zip',
        'sourceSha256': EMBED_SHA, 'officialReleaseMd5': '8db759b337ac4f6966f52b3662c05dd7',
        'pythonLicense': 'Tools/NodeForgeMcp/Runtime/LICENSE.txt', 'sdkVersion': dependencies['sdkVersion'],
        'isolatedPth': True, 'systemPythonRequired': False,
        'files': [ref(out, p) for p in sorted(runtime.rglob('*')) if p.is_file()]}
    save(out / 'Tools/NodeForgeMcp/RuntimeManifest.json', runtime_manifest)
    (out / 'AiReadme.md').write_text(
        '# NodeForge AI 接入 v' + version + '\n\n'
        '适用于 Windows 64 位、官方 Unreal Engine 5.3.2。包内已包含 Python，'
        '无需单独安装运行环境。\n\n'
        '先保存并关闭使用插件的 UE 编辑器，再按照[安装指南]('
        + docs + '/User/AiInstallation.md)运行 '
        '`Scripts/InstallNodeForgeAi.ps1`，指定引擎目录和客户端配置文件。'
        '尚未安装插件时，工具会下载同版插件；已有插件时可以补装 AI 部分。\n\n'
        '配置一次后正常打开工程即可连接，无需每次重跑安装工具。\n\n'
        '[用户手册](' + docs + '/User/Readme.md) · '
        '[AI 调用流程](' + docs + '/Ai/QuickStart.md)\n',
        encoding='utf-8', newline='\n')
    addon = {'format': 'NodeForgeAiAddon1', 'aiVersion': version, 'buildId': version,
        'binarySha256': installation['binarySha256'], 'installationManifest': installation,
        'engineVersion': '5.3.2', 'platform': 'Windows x64', 'bridgeVersion': contract['bridgeVersion'], 'sdkVersion': '2.3.0',
        'docsBundle': contract['bundleId'], 'sharedCoreIncluded': False, 'fixedRuntimeIncluded': True,
        'pluginAsset': {'name': plugin.name, 'sha256': sha(plugin),
            'url': 'https://github.com/XRH66/NodeForge/releases/download/v' + version + '/' + plugin.name,
            'apiUrl': 'https://api.github.com/repos/XRH66/NodeForge/releases/assets/' + str(plugin_asset_id)},
        'files': [ref(out, p) for p in sorted(out.rglob('*')) if p.is_file()]}
    save(out / 'AiPackageManifest.json', addon)
    archive = out.with_name(out.name + '.zip')
    with zipfile.ZipFile(archive, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for p in sorted(out.rglob('*')):
            if p.is_file():
                z.write(p, p.relative_to(out).as_posix())
    return {'status': 'assembled', 'aiVersion': version, 'zip': str(archive), 'sha256': sha(archive),
        'bytes': archive.stat().st_size, 'fileCount': len(addon['files']) + 1}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', required=True)
    parser.add_argument('--embed', required=True)
    parser.add_argument('--plugin-zip', required=True)
    parser.add_argument('--plugin-asset-id', required=True, type=int)
    parser.add_argument('--version', default=VERSION)
    args = parser.parse_args()
    print(json.dumps(assemble(Path(args.out).resolve(), Path(args.embed).resolve(), Path(args.plugin_zip).resolve(), args.plugin_asset_id, args.version)))
