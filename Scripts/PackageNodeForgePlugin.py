"""Package NodeForge 0.1.0 using the existing compiled plugin and current documentation."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED

ROOT = Path(__file__).resolve().parents[1]
VERSION = '0.1.0'


def encoded(value):
    return (json.dumps(value, ensure_ascii=False, indent=2) + '\n').encode('utf-8')


def assemble(base, output, version=VERSION):
    if not re.fullmatch(r'\d+\.\d+\.\d+', version):
        raise ValueError('Release version required')
    if output.exists():
        raise ValueError('Output already exists; choose a new output path')
    release = ROOT / 'Releases' / version
    installation = json.loads((release / 'InstallationManifest.json').read_text(encoding='utf-8'))
    with ZipFile(base) as source:
        files = {entry.filename: source.read(entry) for entry in source.infolist() if not entry.is_dir()}
    binary = files['NodeForge/Binaries/Win64/UnrealEditor-NodeForgeEditor.dll']
    if hashlib.sha256(binary).hexdigest() != installation['binarySha256']:
        raise ValueError('The compiled plugin does not match the release metadata')
    files.pop('PackageManifest.json', None)
    # Protocol references are published with the full manual rather than duplicated here.
    files = {name: data for name, data in files.items() if not name.startswith('NodeForge/Contracts/')}
    for name in ['NodeForge.uplugin', 'InstallationManifest.json']:
        files['NodeForge/' + name] = (release / name).read_bytes()
    source_manifest = release / 'SourceManifest.json'
    if source_manifest.exists():
        frozen = json.loads(source_manifest.read_text(encoding='utf-8'))
        if frozen['binarySha256'] != installation['binarySha256']:
            raise ValueError('Source snapshot is not bound to this core')
        files = {name:data for name,data in files.items() if not name.startswith('NodeForge/Source/')}
        for row in frozen['files']:
            source = (ROOT / row['path']).resolve()
            if not source.is_relative_to(ROOT / 'Plugins/NodeForge/Source'):
                raise ValueError('Source snapshot scope invalid')
            data = source.read_bytes()
            if len(data) != row['bytes'] or hashlib.sha256(data).hexdigest() != row['sha256']:
                raise ValueError('Source snapshot changed')
            files['NodeForge/' + source.relative_to(ROOT / 'Plugins/NodeForge').as_posix()] = data
    files['NodeForge/ThirdPartyNotices.md'] = (ROOT / 'Plugins/NodeForge/ThirdPartyNotices.md').read_bytes()
    for name in ['Installation.md', 'GettingStarted.md', 'AiInstallation.md']:
        text = (ROOT / 'Docs' / name).read_text(encoding='utf-8')
        text = text.replace('(Versions/', '(https://github.com/XRH66/NodeForge/blob/v' + version + '/Docs/Versions/')
        files['NodeForge/Docs/' + name] = text.encode('utf-8')
    files['Readme.md'] = ('# NodeForge v' + version + '\n\nWindows 64 位 · 官方 Unreal Engine 5.3.2。\n\n'
        '将整个 NodeForge 文件夹放入引擎 Engine/Plugins/Marketplace，然后在工程中启用插件。\n\n'
        '详细步骤：[安装](NodeForge/Docs/Installation.md)、[生成第一个材质](NodeForge/Docs/GettingStarted.md)、'
        '[连接 AI](NodeForge/Docs/AiInstallation.md)。\n').encode('utf-8')
    files['NodeForge/Readme.md'] = ('# NodeForge v' + version + '\n\n从 JSON 创建 UE 蓝图和材质。\n\n'
        '[安装](Docs/Installation.md) · [开始使用](Docs/GettingStarted.md) · [连接 AI](Docs/AiInstallation.md)\n').encode('utf-8')
    manifest = {'format': 'NodeForgePluginPackage1', 'version': version, 'buildId': version,
        'engineVersion': '5.3.2', 'platform': 'Windows x64', 'installationSchemaVersion': 1,
        'installLayout': installation['installLayout'], 'binarySha256': installation['binarySha256'],
        'sdkIncluded': False, 'files': [{'path': name, 'sha256': hashlib.sha256(data).hexdigest(),
            'bytes': len(data)} for name, data in sorted(files.items())]}
    files['PackageManifest.json'] = encoded(manifest)
    output.parent.mkdir(parents=True, exist_ok=True)
    with ZipFile(output, 'x', ZIP_DEFLATED, compresslevel=6) as archive:
        for name, data in sorted(files.items()):
            archive.writestr(name, data)
    return {'version': version, 'path': str(output), 'bytes': output.stat().st_size,
        'sha256': hashlib.sha256(output.read_bytes()).hexdigest(), 'files': len(files)}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--base', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    parser.add_argument('--version', default=VERSION)
    args = parser.parse_args()
    print(json.dumps(assemble(args.base.resolve(), args.out.resolve(), args.version), ensure_ascii=False))
