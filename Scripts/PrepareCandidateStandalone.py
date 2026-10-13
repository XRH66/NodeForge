"""Copy only a passing real-UI candidate's two assets and two verified maps; never generate."""
import argparse, shutil
from RunGraphRegression import ROOT, read_json, ref, sha, write_new
from StandaloneCases import candidate_files, load_ui_cases, validate_ui_delivery

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--ui-run', required=True, help='Actual passing ValidateCandidateUi report directory')
parser.add_argument('--run', required=True, help='Fresh short standalone project/report name')
args = parser.parse_args()
if not args.run.isascii() or not args.run.isalnum() or len(args.run) > 20:
    raise SystemExit('Fresh ASCII alphanumeric project name up to 20 characters required')
evidence_dir = (ROOT / args.ui_run).resolve()
if not evidence_dir.is_relative_to(ROOT / 'Reports/20261009CandidateRC1'):
    raise SystemExit('Current candidate actual UI evidence directory required')
evidence = evidence_dir / 'Summary.json'
if not evidence.is_file():
    raise SystemExit('Blocked: actual UI/reopen/PIE/GPU Summary absent; no copy or UE launch performed')
summary = read_json(evidence)
if summary.get('result') != 'passed' or summary.get('oneUnchangedBuild') is not True:
    raise SystemExit('Blocked: actual UI/reopen/PIE/GPU evidence not passed; no copy performed')
manifest = read_json(ROOT / 'Reports/20261009CandidateRC1/Manifest.json')
plugin = ROOT / manifest['plugin']
source_project = (ROOT / summary['verificationCopy']).resolve()
if not source_project.is_relative_to(ROOT / 'ValidationProject/RegressionRuns') or not source_project.is_file():
    raise SystemExit('Existing isolated verification project required')
source_content = source_project.parent / 'Content'
blueprint_config = read_json(evidence_dir / 'BlueprintConfig.json')
material_config = read_json(evidence_dir / 'MaterialConfig.json')
cases = [
    {'case': 'AiBlueprint', 'assetType': 'Blueprint', 'assetPath': blueprint_config['blueprint'],
     'map': blueprint_config['map'], 'expectedMessage': blueprint_config['expectedMessage']},
    {'case': 'AiMaterial', 'assetType': 'Material', 'assetPath': material_config['material'],
     'map': material_config['map'], 'expectedMessage': None},
]
steps = {s['step']: s for s in summary.get('steps', [])}
if len(steps) != 6 or any(s.get('passed') is not True for s in steps.values()):
    raise SystemExit('Complete six-step actual UI evidence required')
runtime = read_json(ROOT / steps['BlueprintRuntime']['report'])
visual = read_json(ROOT / steps['MaterialVisualCheck']['report'])
created_bp = read_json(ROOT / steps['BlueprintMapCreate']['report'])
created_mat = read_json(ROOT / steps['MaterialSceneCreate']['report'])
if runtime.get('status') != 'passed' or runtime.get('expectedMessage') != blueprint_config['expectedMessage'] \
        or created_bp.get('map') != blueprint_config['map'] or created_bp.get('result') != 'passed' \
        or visual.get('result') != 'passed' or created_mat.get('result') != 'passed' \
        or visual.get('map') != material_config['map'] or created_mat.get('map') != material_config['map']:
    raise SystemExit('Actual native runtime/map/visual reports differ from selected cases')
sources = []
for case in cases:
    asset = source_content / (case['assetPath'][6:] + '.uasset')
    saved_map = source_content / (case['map'][6:] + '.umap')
    item = summary['receipts'][case['assetType']]
    if item['request']['assetPath'] != case['assetPath'] or not asset.is_file() or sha(asset) != item['assetSha256'] or not saved_map.is_file():
        raise SystemExit('Actual UI saved asset/map absent or changed')
    sources += [asset, saved_map]
target = ROOT / 'ValidationProject/StandaloneChecks' / args.run
out = ROOT / 'Reports/20261009CandidateRC1' / args.run
if target.exists() or out.exists():
    raise SystemExit('Use fresh project/report directories; protected evidence cannot be overwritten')
out.mkdir(exist_ok=False)
case_manifest = out / 'RuntimeCases.json'
write_new(case_manifest, {'format': 'NodeForgeUiCandidateCases', 'version': 1, 'cases': cases})
load_ui_cases(case_manifest)
target.mkdir(exist_ok=False)
project = target / (args.run + '.uproject')
write_new(project, {'FileVersion': 3, 'EngineAssociation': '5.3',
                   'Plugins': [{'Name': 'NodeForge', 'Enabled': False}]})
assets = []
for path in sources:
    copied = target / 'Content' / path.relative_to(source_content)
    copied.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(path, copied)
    if sha(path) != sha(copied): raise SystemExit('Representative copy bytes differ')
    assets.append({'source': ref(path), 'copy': ref(copied), 'sha256': sha(path)})
(target / 'Config').mkdir()
# Same copy-only rendering/cook settings as PrepareGraphStandalone; source project untouched.
(target / 'Config/DefaultEngine.ini').write_text('''[/Script/EngineSettings.GameMapsSettings]
GameDefaultMap=''' + cases[0]['map'] + '''
EditorStartupMap=''' + cases[0]['map'] + '''

[/Script/WindowsTargetPlatform.WindowsTargetSettings]
DefaultGraphicsRHI=DefaultGraphicsRHI_DX11
+D3D11TargetedShaderFormats=PCD3D_SM5

[/Script/Engine.RendererSettings]
r.DynamicGlobalIlluminationMethod=0
r.ReflectionMethod=0
r.RayTracing=False
r.DefaultFeature.AutoExposure=False
r.AntiAliasingMethod=0
''', 'utf-8')
(target / 'Config/DefaultGame.ini').write_text('''[/Script/UnrealEd.ProjectPackagingSettings]
BuildConfiguration=PPBC_Development
bCookAll=False
bUsePakFile=True
bUseIoStore=False
+DirectoriesToAlwaysCook=(Path="/Game/NodeForgeTests")
''' + ''.join('+MapsToCook=(FilePath="'+c['map']+'")\n' for c in cases), 'utf-8')
preparation = {'kind': 'NodeForgeUiCandidateDelivery1', 'result': 'prepared', 'project': ref(project),
    'uiEvidence': ref(evidence), 'uiEvidenceSha256': sha(evidence),
    'selectedPlugin': ref(plugin), 'validatedBuild': candidate_files(plugin), 'dllSha256': manifest['dllSha256'],
    'maps': [c['map'] for c in cases], 'assets': assets,
    'caseManifest': ref(case_manifest), 'caseManifestSha256': sha(case_manifest),
    'pluginEnabled': False, 'additionalPluginDirectories': [], 'pluginFilesPresent': False,
    'scope': 'Two real-UI saved assets and two verified maps only. No runtime, package or visual acceptance claim.'}
validate_ui_delivery(preparation, project, case_manifest, load_ui_cases(case_manifest))
write_new(out / 'Preparation.json', preparation)
print('Prepared actual UI representative copy:', ref(project), flush=True)
