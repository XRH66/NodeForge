"""Reuse the minimal no-plugin delivery approach for a verified v2 graph/scene run."""
import argparse
import json
import shutil
import sys
from pathlib import Path
from RunGraphRegression import ROOT, read_json, write_new, sha, ref, build_snapshot
from StandaloneCases import load_cases, verify_location_probe

# Explicit finite PC mode. Existing legacy matrix preparation remains historical.
if '--personal-manifest' in sys.argv:
    from PersonalCandidateStandalone import prepare_main
    raise SystemExit(prepare_main(sys.argv[1:]))

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--graph-run', required=True)
source_mode = parser.add_mutually_exclusive_group(required=True)
source_mode.add_argument('--scene-run', help='Existing core/material visual scene evidence')
source_mode.add_argument('--runtime-run', help='Explicit blueprint saved-map/PIE evidence under Reports/20261009UsableEditor')
parser.add_argument('--run', required=True)
parser.add_argument('--visual-only', action='store_true', help='Copy only the verified visual map; no blueprint-runtime scope implied')
args = parser.parse_args()
if not all(value.isascii() and value.isalnum() for value in (args.graph_run, args.scene_run or args.runtime_run, args.run)):
    raise SystemExit('ASCII alphanumeric run names required')
graph = read_json(ROOT / 'Reports/20261008GraphV2' / args.graph_run / 'Summary.json')
scene_folder = ROOT / ('Reports/20261008GraphUpgrade' if args.scene_run else 'Reports/20261009UsableEditor') / (args.scene_run or args.runtime_run)
scene = read_json(scene_folder / 'Summary.json')
if graph.get('result') != 'passed' or not graph.get('fullMatrix') or scene.get('result') != 'passed':
    raise SystemExit('Actual passing graph and runtime/scene evidence required')
selected = (ROOT / graph['selectedPlugin']).resolve() if graph.get('selectedPlugin') else None
if selected and not selected.is_relative_to(ROOT):
    raise SystemExit('Selected validated build must stay in workspace')
current = build_snapshot(selected if selected != ROOT / 'Plugins/NodeForge' else None)
if graph['sourceAndBinaryAfter'] != current:
    raise SystemExit('Current source/binary differs from validated graph run')
if not scene.get('oneUnchangedBuild') or any(current.get(key) != value for key, value in
                                           scene.get('sourceAndBinaryAfter', {}).items()):
    raise SystemExit('Scene/runtime evidence uses a different build')
explicit_cases = None
probe_checks = []
if args.runtime_run:
    if args.visual_only or scene.get('graphRun') != args.graph_run or scene.get('project') != graph['project'] or scene.get('selectedPlugin') != graph['selectedPlugin']:
        raise SystemExit('Explicit runtime evidence must belong to this exact graph project/build; no visual-only mixing')
    items = read_json(ROOT / graph['fixtures'] / 'RuntimeCases.json')['cases']
    explicit_cases = []
    actual_steps = {s['step']: s for s in scene.get('steps', [])}
    positive_cases = {c['name'] for c in graph['cases'] if c.get('expected') == 'succeeded'}
    if {item['case'] for item in items} != positive_cases or len(items) != len(positive_cases):
        raise SystemExit('Explicit runtime cases must cover every successful blueprint graph once')
    for item in items:
        name = item['case']
        created = actual_steps.get(name+'Create', {})
        played = actual_steps.get(name+'Runtime', {})
        if not created.get('passed') or not played.get('passed') or not played.get('independentLocationAndAttribution'):
            raise SystemExit('Native map/actual-location PIE not passed: '+name)
        config = read_json(scene_folder / (name+'Config.json'))
        runtime = read_json(scene_folder / (name+'Runtime.json'))
        actual_map = read_json(scene_folder / (name+'Create.json'))
        request = read_json(ROOT / 'Reports/20261008GraphV2' / args.graph_run / 'Inputs' / (name+'.json'))
        row = {'case': name, 'map': config['map'], 'blueprint': config['blueprint'],
               'expectedMessage': item['expectedMessage'], 'expectedLocation': item['expectedLocation']}
        if actual_map.get('map') != row['map'] or actual_map.get('actualClass') != row['blueprint']+'.'+row['blueprint'].split('/')[-1]+'_C' \
            or runtime.get('expectedMessage') != row['expectedMessage'] or runtime.get('expectedActorLocation') != row['expectedLocation']:
            raise SystemExit('Native map/runtime evidence differs from explicit case: '+name)
        proof = verify_location_probe(request, row);proof['case'] = name
        probe_checks.append(proof);explicit_cases.append(row)
else:
    scene_command = read_json(scene_folder / 'VisualCreateCommand.json')
    if (ROOT / graph['project']).resolve() != Path(scene_command[1]).resolve():
        raise SystemExit('Scene/runtime evidence belongs to a different project')
source = (ROOT / graph['project']).parent / 'Content'
verified_assets = []
for case in graph.get('cases', []):
    if case.get('expected') != 'succeeded':
        continue
    receipt = read_json(ROOT / 'Reports/20261008GraphV2' / args.graph_run / (case['name'] + 'Import.json'))
    asset = source / (receipt['assetPath'].removeprefix('/Game/') + '.uasset')
    if not asset.is_file() or sha(asset) != receipt.get('fileSha256'):
        raise SystemExit('Saved graph/instance changed after verified import: ' + case['name'])
    verified_assets.append({'case': case['name'], 'source': ref(asset), 'receiptSha256': receipt['fileSha256']})
target = ROOT / 'ValidationProject/StandaloneChecks' / args.run
out = ROOT / 'Reports/20261009UsableEditor' / args.run
if target.exists() or out.exists():
    raise SystemExit('Use new copy/report directories; existing evidence is protected')
maps = [c['map'] for c in explicit_cases] if explicit_cases else ['/Game/NodeForgeTests/NFGraphVisualMap']
if not args.visual_only and not explicit_cases:
    maps += ['/Game/NodeForgeTests/NF' + case + 'Map' for case in
         ('BlueprintText', 'BlueprintTextChanged', 'BlueprintBranch', 'BlueprintBranchFalse',
          'BlueprintConcat', 'BlueprintZeroDefaults')]
for path in maps:
    if not (source / (path.removeprefix('/Game/') + '.umap')).is_file():
        raise SystemExit('Verified map missing: ' + path)
target.mkdir(parents=True, exist_ok=False)
out.mkdir(exist_ok=False)
shutil.copytree(source, target / 'Content')
project = target / (args.run + '.uproject')
write_new(project, {'FileVersion': 3, 'EngineAssociation': '5.3',
                   'Description': 'Native v2 asset delivery copy; no generator plugin',
                   'Plugins': [{'Name': 'NodeForge', 'Enabled': False}]})
(target / 'Config').mkdir()
# Reuses PrepareStandaloneChecks.py's safe copy-only configuration; source config untouched.
(target / 'Config/DefaultEngine.ini').write_text('''[/Script/EngineSettings.GameMapsSettings]
GameDefaultMap='''+maps[0]+'''
EditorStartupMap='''+maps[0]+'''

[/Script/WindowsTargetPlatform.WindowsTargetSettings]
DefaultGraphicsRHI=DefaultGraphicsRHI_DX11
+D3D11TargetedShaderFormats=PCD3D_SM5

[/Script/Engine.RendererSettings]
r.DynamicGlobalIlluminationMethod=0
r.ReflectionMethod=0
r.RayTracing=False
r.DefaultFeature.AutoExposure=False
r.AntiAliasingMethod=0
''', encoding='utf-8')
(target / 'Config/DefaultGame.ini').write_text('''[/Script/UnrealEd.ProjectPackagingSettings]
BuildConfiguration=PPBC_Development
bCookAll=False
bUsePakFile=True
bUseIoStore=False
+DirectoriesToAlwaysCook=(Path="/Game/NodeForgeTests")
''' + ''.join('+MapsToCook=(FilePath="' + path + '")\n' for path in maps), encoding='utf-8')
assets = []
for path in sorted(source.rglob('*')):
    if not path.is_file():
        continue
    copied = target / 'Content' / path.relative_to(source)
    digest = sha(path)
    if sha(copied) != digest:
        raise SystemExit('Copied asset bytes differ: ' + str(path))
    assets.append({'source': ref(path), 'copy': ref(copied), 'sha256': digest})
case_manifest = out / 'RuntimeCases.json' if explicit_cases else None
if case_manifest:
    write_new(case_manifest, {'format': 'NodeForgeStandaloneCases', 'version': 1, 'cases': explicit_cases})
    load_cases(case_manifest)
write_new(out / 'Preparation.json', {'result': 'prepared', 'project': ref(project),
          'graphRun': args.graph_run, 'sceneRun': args.scene_run, 'validatedBuild': current,
          'pluginEnabled': False, 'additionalPluginDirectories': [], 'pluginFilesPresent': False,
          'maps': maps, 'assets': assets,
          'generationReceiptByteChecks': verified_assets,
          'runtimeRun': args.runtime_run, 'caseManifest': ref(case_manifest) if case_manifest else None,
          'caseManifestSha256': sha(case_manifest) if case_manifest else None, 'nativeLocationProbeChecks': probe_checks,
          'visualOnly': args.visual_only,
          'scope': 'Hash-matched native asset copy only; opens/runtime/packaging not yet validated'})
print('Prepared:', ref(project), 'assets:', len(assets), flush=True)
