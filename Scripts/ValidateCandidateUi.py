"""Validate actual UI receipts first; never import or rewrite an AI request in the backend."""
import argparse, json, shutil, subprocess, sys
from pathlib import Path
from RunGraphRegression import ROOT, read_json, sha, ref, readback_errors, files_under, ASSET_SUFFIXES

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--run', required=True)
parser.add_argument('--check-only', action='store_true')
args = parser.parse_args()
if not args.run.isascii() or not args.run.isalnum() or len(args.run) > 20:
    raise SystemExit('Fresh ASCII alphanumeric run name, at most 20 characters required')
candidate = ROOT / 'Reports/20261009CandidateRC1'
manifest = read_json(candidate / 'Manifest.json')
plugin = ROOT / manifest['plugin']
project = ROOT / 'ValidationProject/RegressionRuns/CandidateRC1/NFCandidateRC1.uproject'
loaded = read_json(candidate / 'LoadedDll.json')
out = candidate / args.run
out.mkdir(exist_ok=False)
def write(path, data):
    with path.open('x', encoding='utf-8', newline='\n') as stream:
        json.dump(data, stream, ensure_ascii=False, indent=2)
def snapshot():
    return {ref(p): sha(p) for p in plugin.rglob('*') if p.is_file()}
baseline = snapshot()
assert sha(plugin / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll') == manifest['dllSha256']
assert loaded['dllSha256'] == manifest['dllSha256']
receipts, issues = {}, []
for kind, method in [('Blueprint', 'file'), ('Material', 'paste')]:
    raw = ROOT / 'SourceData/ExternalGraphAi2' / (kind + '.json')
    request = read_json(raw)
    matches = []
    for report in (ROOT / 'Reports/UiImports').glob('*.json'):
        data = read_json(report)
        if data.get('requestId') == request['requestId'] and data.get('project', '').replace('\\', '/').lower() == project.as_posix().lower():
            matches.append((report, data))
    good = [(path, data) for path, data in matches if data.get('overallStatus') == 'waitingPersistence'
            and data.get('inputMethod') == method
            and data.get('executionFingerprint', {}).get('binarySha256') == manifest['dllSha256']
            and data.get('pid') == loaded['pid'] and data.get('assetPath') == request['assetPath']]
    if len(good) != 1:
        issues.append(kind + ': requires exactly one actual raw-JSON ' + method + ' UI success receipt from this candidate PID/DLL')
        continue
    path, data = good[0]
    effective_input = raw
    transport = 'byteIdentical'
    if data.get('inputDigest') != sha(raw):
        # Slate/clipboard may normalize CRLF. Preserve both artifacts and permit only that transport difference.
        stored = [p for p in (ROOT / 'Reports/UiInputs').glob('*.json')
                  if p.stat().st_size <= 65536 and sha(p) == data.get('inputDigest')]
        raw_text = raw.read_bytes().decode('utf-8')
        stored = [p for p in stored if p.read_bytes().decode('utf-8').replace('\r\n', '\n') == raw_text.replace('\r\n', '\n')]
        if method != 'paste' or len(stored) != 1:
            issues.append(kind + ': saved UI input differs from the original; no automatic correction allowed')
            continue
        effective_input = stored[0]
        transport = 'clipboardLineEndingsOnly; original preserved separately'
    errors = readback_errors(request, data.get('publishedReadback'))
    for check in ['validation', 'build', 'initialReadback', 'compile', 'finalReadback', 'publish', 'save']:
        if data.get('checks', {}).get(check, {}).get('status') != 'passed': errors.append(check + ' not passed')
    asset = project.parent / 'Content' / (request['assetPath'].removeprefix('/Game/') + '.uasset')
    if not asset.is_file() or sha(asset) != data.get('fileSha256'): errors.append('Saved asset SHA differs')
    if errors:
        issues.extend(kind + ': ' + e for e in errors)
        continue
    receipts[kind] = {'report': ref(path), 'reportSha256': sha(path), 'input': ref(effective_input),
                      'inputSha256': sha(effective_input), 'originalInput': ref(raw),
                      'originalInputSha256': sha(raw), 'transport': transport,
                      'asset': ref(asset), 'assetSha256': sha(asset),
                      'inputMethod': method, 'pid': data['pid'], 'request': request}
write(out / 'UiPrerequisites.json', {'result': 'ready' if not issues else 'blocked',
      'issues': issues, 'receipts': receipts, 'candidateFiles': baseline,
      'scope': 'Actual UI receipt and raw bytes only; no artificial Import or claim of visual/button acceptance'})
if issues or args.check_only:
    print(json.dumps({'result': 'blocked' if issues else 'ready', 'issues': issues, 'evidence': ref(out)}, ensure_ascii=False))
    sys.exit(0 if args.check_only else 2)

# Only copy bytes produced by the real UI; a fresh process reads them with the exact same frozen DLL.
copy_dir = ROOT / 'ValidationProject/RegressionRuns' / args.run
copy_dir.mkdir(exist_ok=False)
copy_project = copy_dir / (args.run + '.uproject')
write(copy_project, {'FileVersion': 3, 'EngineAssociation': '5.3',
      'AdditionalPluginDirectories': [str(plugin.parent)], 'Plugins': [{'Name': 'NodeForge', 'Enabled': True}]})
for item in receipts.values():
    origin = ROOT / item['asset']
    target = copy_dir / 'Content' / origin.relative_to(project.parent / 'Content')
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(origin, target)
assets_before = files_under(project.parent / 'Content', ASSET_SUFFIXES)
steps = []
engine = ROOT.parent / 'UnrealEngine/UE_5.3/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
def execute(name, options, field='result', expected='passed'):
    report = out / (name + '.json')
    cmd = [str(engine), str(copy_project), *options, '-NFReport=' + str(report),
           '-d3d11', '-RenderOffscreen', '-AllowCommandletRendering', '-unattended',
           '-nop4', '-nosound', '-NoLiveCoding', '-abslog=' + str(out / (name + '.log')),
           '-stdout', '-FullStdOutLogOutput']
    write(out / (name + 'Command.json'), cmd)
    with (out / (name + 'Console.txt')).open('xb') as stream:
        result = subprocess.run(cmd, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT,
                                timeout=240, creationflags=subprocess.CREATE_NO_WINDOW)
    data = read_json(report) if report.exists() else {}
    good = result.returncode == 0 and data.get(field) == expected
    steps.append({'step': name, 'passed': good, 'exitCode': result.returncode, 'report': ref(report)})
    print(name, 'passed' if good else 'failed', flush=True)
    return good, data

success = True
for kind, item in receipts.items():
    ok, data = execute(kind + 'Reopen', ['-run=NodeForge', '-NFMode=Verify',
        '-NFInput=' + str(ROOT / item['input']), '-NFPrevious=' + str(ROOT / item['report'])],
        'overallStatus', 'succeeded')
    errors = readback_errors(item['request'], data.get('reopenedFinalReadback'))
    ok = ok and not errors and data.get('pid') != item['pid'] and data.get('fileSha256') == item['assetSha256']
    steps[-1]['passed'] = ok; steps[-1]['independentReadbackErrors'] = errors
    success = success and ok
if success:
    config = out / 'BlueprintConfig.json'
    write(config, {'map': '/Game/NodeForgeTests/NFCandidateAiBlueprintMap',
          'blueprint': receipts['Blueprint']['request']['assetPath'],
          'expectedMessage': 'NodeForge pulse material: enabled on BeginPlay.'})
    ok, _ = execute('BlueprintMapCreate', ['-run=NodeForge', '-NFMode=GraphMapCreate', '-NFInput=' + str(config)])
    if ok:
        ok, _ = execute('BlueprintRuntime', ['-ExecCmds=Automation RunTests NodeForge.BlueprintRuntime; Quit',
               '-TestExit=Automation Test Queue Empty', '-NFGraphRuntimeConfig=' + str(config)], 'status')
    success = success and ok
    config = out / 'MaterialConfig.json'
    write(config, {'map': '/Game/NodeForgeTests/NFCandidateAiMaterialMap',
          'material': receipts['Material']['request']['assetPath'],
          'scalarChanges': {'PulseSpeed': 1.2, 'MinimumBrightness': 0.5},
          'vectorChanges': {'PulseColor': [0.04, 0.65, 0.9, 1]}})
    ok, _ = execute('MaterialSceneCreate', ['-run=NodeForge', '-NFMode=GraphVisualCreate', '-NFInput=' + str(config)])
    if ok:
        ok, _ = execute('MaterialVisualCheck', ['-run=NodeForge', '-NFMode=GraphVisualCheck', '-NFInput=' + str(config)])
    success = success and ok
unchanged = baseline == snapshot() and assets_before == files_under(project.parent / 'Content', ASSET_SUFFIXES)
success = success and unchanged
write(out / 'Summary.json', {'result': 'passed' if success else 'failed', 'steps': steps,
      'uiProject': ref(project), 'verificationCopy': ref(copy_project), 'receipts': receipts,
      'oneUnchangedBuild': unchanged,
      'scope': 'Exact raw UI receipts and copied saved bytes; native independent reopen, PIE and GPU evidence. Human graph readability, all buttons and no-plugin package remain separate.'})
sys.exit(0 if success else 1)
