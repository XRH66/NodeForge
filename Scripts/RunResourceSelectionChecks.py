"""Exercise the real read-only selection dispatcher and panel construction, no UI control."""
import argparse
import shutil
from RunGraphRegression import ROOT, ENGINE, ASSET_SUFFIXES, build_snapshot, files_under, protected_assets, read_json, ref, sha, write_new, automation_report_passed
from RunProcessGuard import run_guarded

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--run', required=True)
parser.add_argument('--plugin-directory', required=True)
parser.add_argument('--instance-run', required=True, help='Previously saved asset source, not new-build graph proof')
parser.add_argument('--audit-identities', action='store_true', help='Also run native canonical resource and bounded-report guards')
args = parser.parse_args()
if not all(v.isascii() and v.isalnum() and len(v) <= 24 for v in (args.run, args.instance_run)):
    raise SystemExit('Fresh ASCII alphanumeric run names required')
plugin = (ROOT / args.plugin_directory).resolve()
if not plugin.is_relative_to(ROOT / 'Reports') or plugin.name != 'NodeForge':
    raise SystemExit('An explicitly staged build is required')
matrix_dir = ROOT / 'Reports/20261008GraphV2' / args.instance_run
matrix = read_json(matrix_dir / 'Summary.json')
if matrix.get('result') != 'passed' or not matrix.get('nativeParentInspections'):
    raise SystemExit('Saved instance source with real parent evidence required')
source = (ROOT / matrix['project']).parent / 'Content'
if not source.resolve().is_relative_to(ROOT / 'ValidationProject/RegressionRuns'):
    raise SystemExit('Only isolated project source assets permitted')
for case in matrix['cases']:
    if case.get('expected') == 'succeeded':
        receipt = read_json(matrix_dir / (case['name'] + 'Import.json'))
        path = source / (receipt['assetPath'].removeprefix('/Game/') + '.uasset')
        if sha(path) != receipt['fileSha256']:
            raise SystemExit('Historical generated source bytes no longer match: ' + case['name'])
out = ROOT / 'Reports/20261009UsableEditor' / args.run
folder = ROOT / 'ValidationProject/RegressionRuns' / args.run
if out.exists() or folder.exists():
    raise SystemExit('Historical evidence/project may not be overwritten')
out.mkdir(); folder.mkdir()
source_before = files_under(source, ASSET_SUFFIXES)
shutil.copytree(source, folder / 'Content')
before = files_under(folder / 'Content', ASSET_SUFFIXES)
source_relative = {str((ROOT / p).relative_to(source)): h for p, h in source_before.items()}
copy_relative = {str((ROOT / p).relative_to(folder / 'Content')): h for p, h in before.items()}
if source_relative != copy_relative:
    raise SystemExit('Copied bytes differ')
project = folder / 'NodeForgeResourceChecks.uproject'
write_new(project, {'FileVersion': 3, 'EngineAssociation': '5.3',
    'AdditionalPluginDirectories': [str(plugin.parent)], 'Plugins': [{'Name': 'NodeForge', 'Enabled': True}]})
parents = [p['parent'] for p in matrix['nativeParentInspections'].values()]
if len(parents) != 2:
    raise SystemExit('This focused selection fixture needs the two known saved parents')
smooth = '/Engine/Functions/Engine_MaterialFunctions02/SmoothStep.SmoothStep'
overlay = '/Engine/Functions/Engine_MaterialFunctions03/Blends/Blend_Overlay.Blend_Overlay'
cases = [{'name': 'Smooth', 'selection': [smooth], 'expected': 'passed'},
         {'name': 'Overlay', 'selection': [overlay], 'expected': 'passed'},
         {'name': 'WaterParent', 'selection': [parents[0]], 'expected': 'passed'},
         {'name': 'TextureParent', 'selection': [parents[1]], 'expected': 'passed'},
         {'name': 'Empty', 'selection': [], 'expected': 'notExecuted'},
         {'name': 'Multiple', 'selection': [smooth, overlay], 'expected': 'notExecuted'},
         {'name': 'Instance', 'selection': ['/Game/NodeForgeTests/NFGraphInstanceWater.NFGraphInstanceWater'], 'expected': 'notExecuted'},
         {'name': 'Mesh', 'selection': ['/Engine/BasicShapes/Plane.Plane'], 'expected': 'notExecuted'},
         {'name': 'DirtyParent', 'selection': [parents[0]], 'dirty': True, 'expected': 'rejected'}]
write_new(out / 'Config.json', {'cases': cases})
baseline = build_snapshot(plugin)
protected_before = protected_assets()
steps = []

def execute(name, options):
    cmd = [str(ENGINE), str(project), *options, '-d3d11', '-RenderOffscreen',
        '-AllowCommandletRendering', '-unattended', '-nop4', '-nosound', '-NoLiveCoding',
        '-abslog=' + str(out / (name + '.log')), '-stdout', '-FullStdOutLogOutput']
    process = run_guarded(cmd, cwd=ROOT, console=out / (name + 'Console.txt'), timeout=180,
        expected_module=plugin / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll')
    data = read_json(out / (name + '.json')) if (out / (name + '.json')).is_file() else {}
    good = process.returncode == 0 and data.get('status', data.get('operationStatus')) == 'passed'
    if name == 'Panel':
        good = good and automation_report_passed(data, 'NodeForge.MinimalPanel')
    steps.append({'step': name, 'result': 'passed' if good else 'failed', 'exitCode': process.returncode,
                  'pid': process.pid, 'report': ref(out / (name + '.json'))})
    print(name, steps[-1]['result'], flush=True)
    return good, data

catalog_ok, catalog = execute('ActualCatalog', ['-run=NodeForge', '-NFMode=GraphCatalog', '-NFReport=' + str(out / 'ActualCatalog.json')])
resource_ok, resources = execute('Resources', ['-ExecCmds=Automation RunTests NodeForge.ResourceSelection; Quit',
    '-TestExit=Automation Test Queue Empty', '-NFResourceConfig=' + str(out / 'Config.json'), '-NFResourceReport=' + str(out / 'Resources.json')])
independent = resource_ok and len(resources.get('cases', [])) == len(cases)
for item in cases:
    row = next((r for r in resources.get('cases', []) if r.get('case') == item['name']), {})
    independent = independent and row.get('result') == 'passed' and row.get('expected') == item['expected']
    if item['expected'] == 'passed':
        report = read_json(out / (item['name'] + '.json'))
        independent = independent and report.get('readOnlyExecutionFingerprint', {}).get('binarySha256') == sha(plugin / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll')
        if item['name'] in ('WaterParent', 'TextureParent'):
            expected = next(p for p in matrix['nativeParentInspections'].values() if p['parent'] == item['selection'][0])
            independent = independent and report.get('parameters') == expected
steps[-1]['independentSelectionChecks'] = bool(independent)
audit_ok = True
if args.audit_identities:
    audit_input = out / 'CanonicalInput.json'; write_new(audit_input, {'parents': parents})
    audit_ok, audit = execute('Canonical', ['-ExecCmds=Automation RunTests NodeForge.CanonicalResourceGuards; Quit',
        '-TestExit=Automation Test Queue Empty', '-NFInput=' + str(audit_input), '-NFReport=' + str(out / 'Canonical.json')])
    required = {'ActualNativeCatalogReady', 'CanonicalSavedFunctionInspected', 'FunctionHashCaseAliasRejected',
        'TaskRuleFieldAliasRejected', 'AllThreeActualParameterKindsCovered', 'ConcurrentReportHasExactlyOneWriter',
        'ReportBytes1048576', 'ReportBytes1048577', 'SavedAssetsAndInputUnchanged'}
    rows = audit.get('cases', [])
    audit_ok = audit_ok and audit.get('test') == 'NodeForge.CanonicalResourceGuards' and bool(rows) and all(r.get('passed') is True for r in rows) and required.issubset({r.get('case') for r in rows})
    independent = independent and all(r.get('dirtyFlagsPreservedDuringDispatch') is True for r in resources.get('cases', []))
    steps[-1]['requiredAuditChecks'] = bool(audit_ok)
panel_ok, panel = execute('Panel', ['-ExecCmds=Automation RunTests NodeForge.MinimalPanel; Quit',
    '-TestExit=Automation Test Queue Empty', '-NFPanelReport=' + str(out / 'Panel.json')])
after = build_snapshot(plugin)
unchanged = before == files_under(folder / 'Content', ASSET_SUFFIXES) and source_before == files_under(source, ASSET_SUFFIXES)
good = catalog_ok and independent and audit_ok and panel_ok and baseline == after and unchanged and protected_before == protected_assets()
write_new(out / 'Summary.json', {'result': 'passed' if good else 'failed', 'steps': steps,
    'project': ref(project), 'selectedPlugin': ref(plugin), 'selectedBinarySha256': sha(plugin / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'),
    'catalogDigest': catalog.get('catalogDigest'), 'sourceAssetMatrix': args.instance_run,
    'canonicalResourceAudit': 'passed' if args.audit_identities and audit_ok else 'failed' if args.audit_identities else 'notRun',
    'sourceAndBinaryBefore': baseline, 'sourceAndBinaryAfter': after, 'oneUnchangedBuild': baseline == after,
    'sourceAssetsBefore': source_before, 'sourceAssetsAfter': files_under(source, ASSET_SUFFIXES),
    'copiedAssetBytesUnchanged': unchanged, 'rootProtectedAssetsUnchanged': protected_before == protected_assets(),
    'scope': 'Native production selection dispatcher nine cases, own report overwrite protection, native bound signatures/node fragments and same panel construction; previous assets are sources only, not new-build graph evidence',
    'humanButtonClicks': 'notRun', 'clipboard': 'notTouched', 'externalAiUi': 'notRun'})
raise SystemExit(0 if good else 1)
