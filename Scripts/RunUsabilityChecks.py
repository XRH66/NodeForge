"""Project-owned verification runner; every phase writes fresh evidence and preserves assets."""
import argparse, hashlib, json
from pathlib import Path
from RunGraphRegression import (ROOT, ENGINE, ASSET_SUFFIXES, build_snapshot, files_under,
    protected_assets, read_json, ref, sha, write_new)
from RunProcessGuard import run_guarded

root, engine = ROOT, ENGINE
p = argparse.ArgumentParser()
p.add_argument('--phase', choices=['UiReopen', 'Visual', 'VisualCheck'], required=True)
p.add_argument('--run', required=True)
p.add_argument('--project', required=True, help='Existing isolated RegressionRuns project; root and RC1 are protected')
p.add_argument('--plugin-directory', required=True, help='Explicit compiled Reports stage selected by this project')
p.add_argument('--receipt', action='append', help='Exact UI import receipt; repeat to select this batch instead of historical globbing')
p.add_argument('--input', action='append', help='Additional exact preserved UI input/snapshot; never synthesized')
p.add_argument('--timeout-seconds', type=int, default=300)
a = p.parse_args()
if not a.run.isalnum():
    raise SystemExit('Use a fresh alphanumeric run name')
project = (root / a.project).resolve()
stage = (root / a.plugin_directory).resolve()
if (not project.is_relative_to(root / 'ValidationProject/RegressionRuns') or not project.is_file()
        or project.is_relative_to(root / 'ValidationProject/RegressionRuns/CandidateRC1')):
    raise SystemExit('Existing isolated non-RC1 validation project required')
binary = stage / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'
if (not stage.is_relative_to(root / 'Reports') or stage.name != 'NodeForge' or not binary.is_file()
        or stage.is_relative_to(root / 'Reports/20261009CandidateRC1')):
    raise SystemExit('Explicit isolated compiled stage required; frozen RC1 is protected')
descriptor = read_json(project)
directories = descriptor.get('AdditionalPluginDirectories', [])
if (descriptor.get('Plugins') != [{'Name': 'NodeForge', 'Enabled': True}]
        or len(directories) != 1 or (project.parent / directories[0]).resolve() != stage.parent
        or (project.parent / 'Plugins/NodeForge').exists()):
    raise SystemExit('Project must select only the explicit staged NodeForge plugin')
if not 1 <= a.timeout_seconds <= 900:
    raise SystemExit('Timeout must be 1..900 seconds')
out = root / 'Reports/20261008UsablePrototype' / a.run
out.mkdir(exist_ok=False)
steps = []
baseline = build_snapshot(stage)
protected = protected_assets()
before_assets = files_under(project.parent / 'Content', ASSET_SUFFIXES)
rc1_assets = files_under(root / 'ValidationProject/RegressionRuns/CandidateRC1/Content', ASSET_SUFFIXES)
frozen_rc1 = files_under(root / 'Reports/20261009CandidateRC1/PluginStage/NodeForge')
immutable = {ref(project): sha(project)}
write_new(out / 'Preparation.json', {'project': ref(project), 'selectedPlugin': ref(stage),
    'expectedDllSha256': sha(binary), 'phase': a.phase,
    'scope': 'Native verification/visual checks in an explicit isolated project; no new UI interaction claim'})


def guards():
    current_assets = files_under(project.parent / 'Content', ASSET_SUFFIXES)
    return (baseline == build_snapshot(stage) and protected == protected_assets()
        and all(current_assets.get(path) == digest for path, digest in before_assets.items())
        and (a.phase == 'Visual' or current_assets == before_assets)
        and rc1_assets == files_under(root / 'ValidationProject/RegressionRuns/CandidateRC1/Content', ASSET_SUFFIXES)
        and frozen_rc1 == files_under(root / 'Reports/20261009CandidateRC1/PluginStage/NodeForge')
        and all((root / path).is_file() and sha(root / path) == digest for path, digest in immutable.items()))

def execute(name, options, expected):
    if not guards():
        raise SystemExit('Build, input or protected asset changed; stop before another process')
    report = out / (name + '.json')
    command = [str(engine), str(project), '-run=NodeForge', *options,
        '-NFReport=' + str(report), '-d3d11', '-AllowCommandletRendering',
        '-unattended', '-nop4', '-nosound', '-NoLiveCoding',
        '-abslog=' + str(out / (name + '.log')), '-stdout', '-FullStdOutLogOutput']
    result = run_guarded(command, cwd=root, console=out / (name + 'Console.txt'),
        timeout=a.timeout_seconds, expected_module=binary)
    data = read_json(report) if report.is_file() else {}
    actual = data.get('overallStatus', data.get('result'))
    good = result.returncode == 0 and actual == expected and guards()
    if a.phase == 'UiReopen':
        good = good and data.get('pid') == result.pid and data.get('checks', {}).get('persistence', {}).get('status') == 'passed'
    else:
        good = good and data.get('mode') == name
    steps.append({'step': name, 'command': command, 'exitCode': result.returncode,
        'pid': result.pid, 'actual': actual, 'passed': good, 'report': report.relative_to(root).as_posix()})
    (out / 'Progress.json').write_text(json.dumps(steps, indent=2), 'utf-8')
    print(name, result.returncode, actual, flush=True)
    if not good:
        raise SystemExit('Failed; evidence retained: ' + name)

if a.phase == 'UiReopen':
    # Locate exact input bytes by digest. UI-paste inputs are stored verbatim; file inputs stay unchanged.
    candidates = [* (root / 'Reports/UiInputs').glob('*.json'), * (root / 'Tests/Valid').glob('*.json'),
        *[(root / value).resolve() for value in (a.input or [])]]
    if any(not source.is_relative_to(root) or not source.is_file() for source in candidates):
        raise SystemExit('Exact inputs must be existing workspace files')
    digests = {hashlib.sha256(f.read_bytes()).hexdigest(): f for f in candidates}
    imports = []
    receipts = [(root / value).resolve() for value in a.receipt] if a.receipt else sorted((root / 'Reports/UiImports').glob('*.json'))
    for receipt in receipts:
        if not receipt.is_relative_to(root / 'Reports/UiImports') or not receipt.is_file():
            raise SystemExit('UI receipts must be existing Reports/UiImports files')
        d = read_json(receipt)
        if d.get('overallStatus') != 'waitingPersistence':
            if a.receipt:
                raise SystemExit('Explicit UI receipt is not waitingPersistence: ' + str(receipt))
            continue
        source = digests.get(d['inputDigest'])
        if source is None:
            raise SystemExit('Exact input bytes unavailable: ' + str(receipt))
        asset = (project.parent / 'Content' / (d['assetPath'].removeprefix('/Game/') + '.uasset')).resolve()
        if (not d['assetPath'].startswith('/Game/NodeForgeTests/')
                or not asset.is_relative_to(project.parent / 'Content/NodeForgeTests')
                or not asset.is_file() or sha(asset) != d.get('fileSha256')):
            raise SystemExit('Exact already saved isolated UI asset bytes unavailable: ' + str(receipt))
        if any(item['asset'] == d['assetPath'] for item in imports):
            raise SystemExit('Duplicate UI target receipts are not an independent batch')
        immutable.update({ref(source): sha(source), ref(receipt): sha(receipt)})
        imports.append({'receipt': receipt.relative_to(root).as_posix(),
            'source': source.relative_to(root).as_posix(), 'asset': d['assetPath']})
    write_new(out / 'InputMap.json', imports)
    if not imports:
        write_new(out / 'Summary.json', {'result': 'blocked', 'phase': a.phase, 'steps': [],
            'reason': 'No waitingPersistence UI receipts; no verification process was started'})
        raise SystemExit(2)
    for index, item in enumerate(imports, 1):
        execute('UiReopen' + str(index),
            ['-NFMode=Verify', '-NFInput=' + str(root / item['source']),
             '-NFPrevious=' + str(root / item['receipt'])], 'succeeded')
else:
    if a.phase == 'Visual':
        execute('VisualCreate', ['-NFMode=VisualCreate'], 'passed')
    execute('VisualCheck', ['-NFMode=VisualCheck'], 'passed')
good = bool(steps) and all(step['passed'] for step in steps) and guards()
write_new(out / 'Summary.json', {'result': 'passed' if good else 'failed', 'phase': a.phase,
    'steps': steps, 'project': ref(project), 'selectedPlugin': ref(stage),
    'sourceAndBinaryBefore': baseline, 'sourceAndBinaryAfter': build_snapshot(stage),
    'protectedOriginalBytesUnchanged': guards(),
    'scope': 'Native isolated reopen/visual checks; does not prove a new UI interaction'})
raise SystemExit(0 if good else 1)
