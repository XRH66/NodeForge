"""Sequential native saved-map/PIE checks of JSON-driven Actor position, no desktop controls."""
import argparse
import json
import math
import sys
from pathlib import Path
from RunGraphRegression import ROOT, ENGINE, build_snapshot, files_under, read_json, ref, sha, write_new, ASSET_SUFFIXES, protected_assets
from RunProcessGuard import run_guarded

def run_continuous(args):
    """One previously saved/independently verified graph through the original map and PIE paths."""
    from RunProcessGuard import validate_isolated_project
    from NodeForgeControl import assert_exclusive
    project, selected, binary = validate_isolated_project(Path(args.project), Path(args.plugin_directory), ROOT)
    assert_exclusive(project)
    input_path, gen_path, verify_path, recipe_path = map(Path, (args.continuous_input, args.generation_report, args.verify_report, args.motion_recipe))
    for p in (input_path, gen_path, verify_path, recipe_path):
        if not p.resolve().is_relative_to(ROOT / 'Reports/PluginControlStageTwo'):
            raise ValueError('Exact local input/report/recipe required')
    wire, gen, verify, recipe = map(read_json, (input_path, gen_path, verify_path, recipe_path))
    target = wire['target']['assetPath']
    if wire.get('version') != 3 or wire['asset']['type'] != 'Blueprint' or not target.startswith('/Game/NodeForgeTests/BlueprintTick/'):
        raise ValueError('Finite Tick v3 Actor input required')
    for native in (gen, verify):
        if (native['requestId'] != wire['requestId'] or native['inputDigest'] != sha(input_path)
                or native['assetPath'] != target or native['executionFingerprint']['binarySha256'] != sha(binary)
                or native['executionFingerprint']['catalog'] != wire['catalogDigest']
                or Path(native['project']).resolve() != project):
            raise ValueError('Original saved/verified identities differ')
    if gen['requestOutcome'] != 'built' or gen['overallStatus'] != 'waitingPersistence' or verify['requestOutcome'] != 'alreadyCompleted':
        raise ValueError('Exact original generation/Verify terminal semantics required')
    if gen['pid'] == verify['pid'] or verify['overallStatus'] != 'succeeded' or verify['checks']['persistence']['status'] != 'passed':
        raise ValueError('Original independent Verify in a new PID required')
    asset = project.parent / 'Content' / (target.removeprefix('/Game/') + '.uasset')
    if sha(asset) != gen['fileSha256'] or sha(asset) != verify['fileSha256']:
        raise ValueError('Saved source bytes changed')
    if (recipe['mode'] != 'ContinuousX' or recipe['velocity'] != [100, 0, 0]
            or recipe['sampleElapsedThresholds'] != [0, .5, 1] or recipe['minimumFramesPerInterval'] != 2
            or recipe['maximumDeltaSeconds'] != .25 or recipe['positionToleranceCm'] != .001
            or recipe['transformTolerance'] != .000001 or recipe['maximumFrames'] != 4096
            or recipe['observerWritesActor'] is not False):
        raise ValueError('Frozen independent timing recipe differs')
    map_path = recipe['map']
    if map_path != target.rsplit('/', 1)[0] + '/TickMap':
        raise ValueError('Only this batch auxiliary map permitted')
    map_file = project.parent / 'Content' / (map_path.removeprefix('/Game/') + '.umap')
    if map_file.exists():
        raise ValueError('Target map occupied; reconcile original report, do not overwrite/replay')
    out = Path(args.report_directory).resolve()
    if not out.is_relative_to(ROOT / 'Reports/PluginControlStageTwo'):
        raise ValueError('Bounded fresh report directory required')
    out.mkdir(parents=True, exist_ok=False)
    before = files_under(project.parent / 'Content', ASSET_SUFFIXES)
    source = build_snapshot(selected)
    protected = protected_assets()
    config = {'map': map_path, 'blueprint': target, 'expectedMessage': recipe['expectedMessage'], 'mode': 'ContinuousX'}
    config_path = out / 'Config.json'; write_new(config_path, config)
    write_new(out / 'Prerequisites.json', {'input': ref(input_path), 'inputSha256': sha(input_path),
        'generation': ref(gen_path), 'verify': ref(verify_path), 'recipe': ref(recipe_path),
        'recipeSha256': sha(recipe_path), 'newSavedBlueprints': 0, 'maximumNewMaps': 1,
        'beforeAssets': before, 'sourceAndBinary': source, 'observerDrivesActor': False})
    def execute(name, options, interactive=False):
        assert_exclusive(project)
        if build_snapshot(selected) != source or protected_assets() != protected or any(sha(ROOT/p) != h for p,h in before.items()):
            raise ValueError('Source/stage/original asset protection conflict')
        report = out / (name + '.json')
        command = [str(ENGINE.with_name('UnrealEditor.exe') if interactive else ENGINE), str(project), *options,
            '-NFReport=' + str(report), '-unattended', '-d3d11', '-RenderOffscreen', '-nosound', '-nop4', '-NoLiveCoding',
            '-abslog=' + str(out / (name + '.log')), '-stdout', '-FullStdOutLogOutput']
        process = run_guarded(command, cwd=ROOT, console=out / (name + 'Console.txt'), timeout=args.timeout_seconds, expected_module=binary)
        data = read_json(report) if report.is_file() else {}
        if process.returncode != 0 or data.get('status', data.get('result')) != 'passed':
            write_new(out / (name + 'Review.json'), {'status': 'failed', 'canAdvance': False, 'pid': process.pid,
                'exitCode': process.returncode, 'report': ref(report), 'retryAllowed': False})
            raise ValueError('Original native path failed; retain report and reconcile ' + name)
        return data
    created = execute('MapCreate', ['-run=NodeForge', '-NFMode=GraphMapCreate', '-NFInput=' + str(config_path)])
    if created.get('map') != map_path or not map_file.is_file():
        raise ValueError('Actual saved map not proven')
    write_new(out / 'MapCreateReview.json', {'status': 'passed', 'canAdvance': True, 'map': map_path,
        'mapFileSha256': sha(map_file), 'reason': 'Single saved Actor class is required by the original passive PIE observer'})
    data = execute('Runtime', ['-ExecCmds=Automation RunTests NodeForge.BlueprintRuntime; Quit',
        '-TestExit=Automation Test Queue Empty', '-NFGraphRuntimeConfig=' + str(config_path)], interactive=True)
    samples, frames = data['trajectory'], data['effectiveFrameDeltas']
    checks = {'minimumSamples': len(samples) >= 3, 'boundedFrames': 4 <= len(frames) <= recipe['maximumFrames'],
        'mode': data['observationMode'] == 'ContinuousX', 'passive': data['observerDrivesActor'] is False,
        'cleaned': data['pieStopped'] is True and data['loggerRemoved'] is True and data['motionDelegatesRemoved'] is True,
        'beginPlayOnceAttributed': data['attributedMessageCount'] == 1 and data['expectedMessage'] == recipe['expectedMessage']
            and data['sourceActor'] == samples[0]['actor'] and data['sourceWorld'] == samples[0]['world'],
        'sampleIdentity': len({(s['actor'], s['world']) for s in samples}) == 1}
    cumulative = [0.]
    for index, frame in enumerate(frames, 1):
        if frame['frame'] != index or not 0 < frame['deltaSeconds'] <= .25:
            raise ValueError('Actual frame/Delta evidence violates frozen admission')
        cumulative.append(cumulative[-1] + frame['deltaSeconds'])
    intervals = []
    for a,b in zip(samples, samples[1:]):
        dt = cumulative[b['effectiveFrames']] - cumulative[a['effectiveFrames']]
        dx = b['location'][0] - a['location'][0]
        good = (b['effectiveFrames'] - a['effectiveFrames'] >= 2 and dt > 0 and dx > 0
            and abs(dx - 100*dt) <= .001
            and abs(b['effectiveDeltaAccumulated'] - cumulative[b['effectiveFrames']]) <= 1e-12
            and all(abs(b['location'][i]-a['location'][i]) <= .001 for i in (1,2)))
        intervals.append({'effectiveDelta': dt, 'actualXDelta': dx, 'expectedXDelta': 100*dt, 'passed': good})
    checks['twoIndependentIntervals'] = len(intervals) >= 2 and all(x['passed'] for x in intervals)
    checks['allFramesIndependentX'] = all(abs(f['x'] - samples[0]['location'][0] - 100*cumulative[i]) <= .001 for i,f in enumerate(frames,1))
    checks['originalAssetsAndStage'] = (build_snapshot(selected) == source and protected_assets() == protected
        and all(sha(ROOT/p) == h for p,h in before.items()))
    after = files_under(project.parent/'Content', ASSET_SUFFIXES)
    checks['oneAuxiliaryMapOnly'] = set(after)-set(before) == {ref(map_file)}
    good = all(checks.values())
    write_new(out/'Summary.json', {'status': 'passed' if good else 'failed', 'canAdvance': good, 'checks': checks,
        'intervals': intervals, 'nativeRuntime': ref(out/'Runtime.json'), 'nativeRuntimeSha256': sha(out/'Runtime.json'),
        'project': ref(project), 'stage': ref(selected), 'binarySha256': sha(binary), 'assetsAfter': after,
        'scope': 'One independently saved Actor; exact real game Delta, passive continuous observer; no visual/package coverage'})
    return 0 if good else 1


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--run')
parser.add_argument('--graph-run')
parser.add_argument('--resume', action='store_true', help='Reuse complete native receipts in an interrupted run; never overwrite partial evidence')
parser.add_argument('--project', help='Explicit isolated project; must match the validated graph matrix')
parser.add_argument('--plugin-directory', help='Explicit staged plugin; must match the validated graph matrix')
parser.add_argument('--timeout-seconds', type=int, default=180)
parser.add_argument('--continuous-input', help='Original finite v3 Tick input; selects the finite path instead of fullMatrix')
parser.add_argument('--generation-report')
parser.add_argument('--verify-report')
parser.add_argument('--motion-recipe')
parser.add_argument('--report-directory')
args = parser.parse_args()
if args.continuous_input:
    if args.run or args.graph_run or args.resume or not all((args.project, args.plugin_directory, args.generation_report, args.verify_report, args.motion_recipe, args.report_directory)):
        parser.error('Finite input/receipt/Verify/recipe/project/stage/report group required, without legacy matrix arguments')
    if not 60 <= args.timeout_seconds <= 180:
        parser.error('Finite process deadline must be 60..180 seconds; native PIE stop/hard deadlines stay 55/60')
    sys.exit(run_continuous(args))
if not args.run or not args.graph_run or any((args.generation_report, args.verify_report, args.motion_recipe, args.report_directory)):
    parser.error('Original fullMatrix path requires --run and --graph-run')
if not all(name.isascii() and name.isalnum() and len(name) <= 24 for name in (args.run, args.graph_run)):
    raise SystemExit('Fresh ASCII alphanumeric run names, at most 24 characters, required')
graph_dir = ROOT / 'Reports/20261008GraphV2' / args.graph_run
graph = read_json(graph_dir / 'Summary.json')
if graph.get('result') != 'passed' or not graph.get('fullMatrix'):
    raise SystemExit('Complete passing graph matrix required; a smoke run is insufficient')
project = (ROOT / graph['project']).resolve()
selected = (ROOT / graph['selectedPlugin']).resolve()
if (not project.is_relative_to(ROOT / 'ValidationProject/RegressionRuns') or not project.is_file()
        or project.is_relative_to(ROOT / 'ValidationProject/RegressionRuns/CandidateRC1')):
    raise SystemExit('Existing isolated non-RC1 graph validation project required')
if (args.project and (ROOT / args.project).resolve() != project) or (args.plugin_directory and (ROOT / args.plugin_directory).resolve() != selected):
    raise SystemExit('Explicit project/stage must match the validated graph matrix')
binary = selected / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'
if (not selected.is_relative_to(ROOT / 'Reports') or selected.name != 'NodeForge' or not binary.is_file()
        or selected.is_relative_to(ROOT / 'Reports/20261009CandidateRC1')):
    raise SystemExit('Use the explicitly frozen staged build')
descriptor = read_json(project)
directories = descriptor.get('AdditionalPluginDirectories', [])
if (descriptor.get('Plugins') != [{'Name': 'NodeForge', 'Enabled': True}]
        or len(directories) != 1 or (project.parent / directories[0]).resolve() != selected.parent
        or (project.parent / 'Plugins/NodeForge').exists()):
    raise SystemExit('Project must select only the validated staged plugin')
if not 1 <= args.timeout_seconds <= 900:
    raise SystemExit('Timeout must be 1..900 seconds')
baseline = build_snapshot(selected)
if baseline != graph['sourceAndBinaryAfter']:
    raise SystemExit('Current source/build differs from the validated graph matrix')
manifest = read_json(ROOT / graph['fixtures'] / 'RuntimeCases.json')
if not isinstance(manifest.get('cases'), list) or not manifest['cases']:
    raise SystemExit('Nonempty runtime case manifest required')
cases = {case['name']: case for case in graph['cases']}
for case in manifest['cases']:
    if cases.get(case['case'], {}).get('result') != 'passed' or cases[case['case']]['expected'] != 'succeeded':
        raise SystemExit('Runtime fixture has no successful generated graph')
    location = case['expectedLocation']
    if not isinstance(location, list) or len(location) != 3 or any(type(v) not in (int, float) or not math.isfinite(v) or abs(v) > 1e12 for v in location):
        raise SystemExit('Invalid bounded runtime location expectation')
    for field in ('expectedRotation', 'expectedScale'):
        value = case.get(field)
        if value is not None and (not isinstance(value, list) or len(value) != 3
                or any(type(v) not in (int, float) or not math.isfinite(v) or abs(v) > 1e12 for v in value)):
            raise SystemExit('Invalid bounded runtime transform expectation: ' + field)
out = ROOT / 'Reports/20261009UsableEditor' / args.run
if args.resume:
    if not out.is_dir() or (out / 'Summary.json').exists():
        raise SystemExit('Resume requires an interrupted run without a final Summary')
    if (out / 'LiveProcess.json').exists():
        raise SystemExit('Prior live PID checkpoint requires reconciliation; do not resume or restart')
else:
    out.mkdir(exist_ok=False)
content = project.parent / 'Content'
before_assets = files_under(content, ASSET_SUFFIXES)
protected_before = protected_assets()
rc1_before = files_under(ROOT / 'ValidationProject/RegressionRuns/CandidateRC1/Content', ASSET_SUFFIXES)
frozen_rc1 = files_under(ROOT / 'Reports/20261009CandidateRC1/PluginStage/NodeForge')
steps = []
existing_evidence = files_under(out) if args.resume else {}
if args.resume:
    write_new(out / 'ResumeStart.json', {'graphRun': args.graph_run, 'project': ref(project),
        'sourceAndBinary': baseline, 'assetsAtResume': before_assets, 'previousEvidence': existing_evidence})


def config_write(path, value):
    if args.resume and path.is_file():
        if read_json(path) != value:
            raise SystemExit('Existing runtime configuration differs: ' + ref(path))
    else:
        write_new(path, value)

def execute(name, options):
    if (build_snapshot(selected) != baseline or protected_assets() != protected_before
            or rc1_before != files_under(ROOT / 'ValidationProject/RegressionRuns/CandidateRC1/Content', ASSET_SUFFIXES)
            or frozen_rc1 != files_under(ROOT / 'Reports/20261009CandidateRC1/PluginStage/NodeForge')):
        raise SystemExit('Build or protected asset changed; stop before another process')
    report = out / (name + '.json')
    command = [str(ENGINE), str(project), *options, '-NFReport=' + str(report), '-unattended',
               '-d3d11', '-RenderOffscreen', '-nosound', '-nop4', '-NoLiveCoding',
               '-abslog=' + str(out / (name + '.log')), '-stdout', '-FullStdOutLogOutput']
    command_file = out / (name + 'Command.json')
    log = out / (name + '.log')
    reused = args.resume and report.is_file()
    if reused:
        if not command_file.is_file() or read_json(command_file) != command or not log.is_file():
            raise SystemExit('Cannot reuse incomplete or mismatching command/log: ' + name)
        exit_path = out / (name + 'Exit.json')
        if not exit_path.is_file():
            raise SystemExit('Prior native terminal PID/module evidence unavailable; no automatic reuse: ' + name)
        terminal = read_json(exit_path)
        loaded = terminal.get('actualLoadedModule') or {}
        if (terminal.get('exitCode') != 0 or type(terminal.get('pid')) is not int
                or terminal['pid'] <= 0 or loaded.get('pid') != terminal['pid']
                or loaded.get('matchesExpected') is not True
                or Path(loaded.get('path', '')).resolve() != binary.resolve()
                or loaded.get('diskFileSha256') != sha(binary)):
            raise SystemExit('Prior process termination/actual staged DLL not proven: ' + name)
        data = read_json(report)
        if data.get('result', data.get('status')) != 'passed':
            raise SystemExit('Existing failure is retained; reconcile in a fresh run: ' + name)
        if name.endswith('Runtime') and 'TEST COMPLETE. EXIT CODE: 0' not in log.read_text(encoding='utf-8-sig', errors='replace'):
            raise SystemExit('Runtime has no successful native exit marker: ' + name)
        code, pid = terminal['exitCode'], terminal['pid']
    else:
        if any(path.exists() for path in (report, command_file, log, out / (name + 'Console.txt'))):
            raise SystemExit('Partial evidence exists; no overwrite or automatic retry: ' + name)
        process = run_guarded(command, cwd=ROOT, console=out / (name + 'Console.txt'),
            timeout=args.timeout_seconds, expected_module=binary)
        code, pid = process.returncode, process.pid
        data = read_json(report) if report.is_file() else {}
    actual = data.get('result', data.get('status'))
    good = code == 0 and actual == 'passed'
    if name.endswith('Runtime'):
        good = good and data.get('pid') == pid
    steps.append({'step': name, 'passed': good, 'actual': actual, 'exitCode': code,
        'pid': pid, 'report': ref(report), 'reusedNativeEvidence': reused, 'reportSha256': sha(report) if report.is_file() else None})
    print(name, 'reused' if reused else code, actual, flush=True)
    return good, data

for case in manifest['cases']:
    name = case['case']
    request = read_json(graph_dir / 'Inputs' / (name + '.json'))
    receipt = read_json(graph_dir / (name + 'Import.json'))
    asset = content / (request['assetPath'].removeprefix('/Game/') + '.uasset')
    if sha(asset) != receipt['fileSha256']:
        raise SystemExit('Generated graph bytes changed before runtime: ' + name)
    config = {'map': '/Game/NodeForgeTests/NF' + name + 'Map', 'blueprint': request['assetPath'],
              'expectedMessage': case['expectedMessage']}
    config_path = out / (name + 'Config.json')
    config_write(config_path, config)
    created, create_data = execute(name + 'Create', ['-run=NodeForge', '-NFMode=GraphMapCreate', '-NFInput=' + str(config_path)])
    saved_map = content / (config['map'].removeprefix('/Game/') + '.umap')
    created = created and create_data.get('map') == config['map'] \
        and create_data.get('actualClass') == request['assetPath'] + '.' + request['assetPath'].rsplit('/', 1)[-1] + '_C' \
        and saved_map.is_file()
    steps[-1]['nativeMapAndClassMatch'] = created
    steps[-1]['passed'] = created
    if not created:
        continue
    location = ','.join(format(value, '.17g') for value in case['expectedLocation'])
    options = ['-ExecCmds=Automation RunTests NodeForge.BlueprintRuntime; Quit',
        '-TestExit=Automation Test Queue Empty', '-NFGraphRuntimeConfig=' + str(config_path),
        '-NFExpectedActorLocation=' + location]
    for field, flag in (('expectedRotation', 'NFExpectedActorRotation'), ('expectedScale', 'NFExpectedActorScale')):
        if field in case:
            options.append('-' + flag + '=' + ','.join(format(value, '.17g') for value in case[field]))
    passed, data = execute(name + 'Runtime', options)
    actual_location = data.get('actualActorLocation')
    independent = passed and data.get('locationCheckRequested') is True and isinstance(actual_location, list) \
        and len(actual_location) == 3 and all(type(v) in (int, float) and math.isfinite(v) for v in actual_location) \
        and all(abs(a - b) <= 1e-6 for a, b in zip(actual_location, case['expectedLocation'])) \
        and data.get('expectedMessage') == case['expectedMessage'] and data.get('attributedMessageCount') == 1 \
        and data.get('sourceClass') == data.get('expectedClass') and data.get('pieStopped') is True
    steps[-1]['independentLocationAndAttribution'] = independent
    transform = True
    for field, actual_field, requested in (('expectedRotation', 'actualActorRotation', 'rotationCheckRequested'),
                                          ('expectedScale', 'actualActorScale', 'scaleCheckRequested')):
        if field in case:
            actual = data.get(actual_field)
            transform = transform and data.get(requested) is True and isinstance(actual, list) and len(actual) == 3 \
                and all(type(v) in (int, float) and math.isfinite(v) for v in actual) \
                and all(abs(a - b) <= 1e-6 for a, b in zip(actual, case[field]))
    steps[-1]['independentRotationAndScale'] = transform
    steps[-1]['passed'] = independent and transform
after_assets = files_under(content, ASSET_SUFFIXES)
protected = all(after_assets.get(path) == digest for path, digest in before_assets.items())
after = build_snapshot(selected)
evidence_unchanged = all((ROOT / path).is_file() and sha(ROOT / path) == digest for path, digest in existing_evidence.items())
protected_root = (protected_assets() == protected_before
    and rc1_before == files_under(ROOT / 'ValidationProject/RegressionRuns/CandidateRC1/Content', ASSET_SUFFIXES)
    and frozen_rc1 == files_under(ROOT / 'Reports/20261009CandidateRC1/PluginStage/NodeForge'))
good = len(steps) == len(manifest['cases']) * 2 and all(step['passed'] for step in steps) and baseline == after and protected and protected_root and evidence_unchanged
write_new(out / 'Summary.json', {'result': 'passed' if good else 'failed', 'steps': steps,
    'project': ref(project), 'graphRun': args.graph_run, 'selectedPlugin': ref(selected),
    'selectedBinarySha256': sha(selected / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'),
    'sourceAndBinaryBefore': baseline, 'sourceAndBinaryAfter': after, 'oneUnchangedBuild': baseline == after,
    'generatedAssetsBefore': before_assets, 'assetsAfter': after_assets, 'allOriginalAssetBytesUnchanged': protected,
    'rootAndRc1ProtectedBytesUnchanged': protected_root,
    'resumed': args.resume, 'reusedStepCount': sum(step['reusedNativeEvidence'] for step in steps),
    'previousEvidenceUnchanged': evidence_unchanged,
    'scope': 'Separately saved maps reopened in fresh native Editor PIE; own Actor location and optional rotation/scale, exact attributed messages; no real panel/packaging coverage'})
sys.exit(0 if good else 1)
