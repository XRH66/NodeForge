"""Native catalog/parent discovery and isolated panel construction; no real UI click claim."""
import argparse
import shutil
from RunGraphRegression import ROOT, ENGINE, read_json, write_new, files_under, ASSET_SUFFIXES, build_snapshot, sha, ref, protected_assets, automation_report_passed
from RunProcessGuard import run_guarded

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--run', required=True)
parser.add_argument('--plugin-directory', required=True)
parser.add_argument('--graph-run', required=True)
args = parser.parse_args()
if not all(v.isascii() and v.isalnum() for v in (args.run, args.graph_run)):
    raise SystemExit('Fresh ASCII alphanumeric run names required')
plugin = (ROOT / args.plugin_directory).resolve()
if not plugin.is_relative_to(ROOT / 'Reports') or plugin.name != 'NodeForge':
    raise SystemExit('Explicit staged NodeForge build required')
graph_folder = ROOT / 'Reports/20261008GraphV2' / args.graph_run
graph = read_json(graph_folder / 'Summary.json')
if graph.get('result') != 'passed' or not graph.get('fullMatrix') or not graph.get('nativeParentInspections'):
    raise SystemExit('Passing native instance matrix with actual parent inspections required')
out = ROOT / 'Reports/20261009UsableEditor' / args.run
project_folder = ROOT / 'ValidationProject/RegressionRuns' / args.run
if out.exists() or project_folder.exists():
    raise SystemExit('Existing reports/projects are protected')
out.mkdir(); project_folder.mkdir()
project = project_folder / 'NodeForgeRulesChecks.uproject'
write_new(project, {'FileVersion': 3, 'EngineAssociation': '5.3',
                   'AdditionalPluginDirectories': [str(plugin.parent)],
                   'Plugins': [{'Name': 'NodeForge', 'Enabled': True}]})
source = (ROOT / graph['project']).parent / 'Content'
if not source.resolve().is_relative_to(ROOT / 'ValidationProject/RegressionRuns'):
    raise SystemExit('Only isolated source assets permitted')
before_source = files_under(source, ASSET_SUFFIXES)
shutil.copytree(source, project_folder / 'Content')
before_assets = files_under(project_folder / 'Content', ASSET_SUFFIXES)
before_build = build_snapshot(plugin)
before_protected = protected_assets()
steps = []
actual_catalog_digest = None

def execute(name, mode, config=None, rejected=False, panel=False):
    global actual_catalog_digest
    report = out / (name + '.json')
    options = ['-ExecCmds=Automation RunTests NodeForge.MinimalPanel; Quit', '-TestExit=Automation Test Queue Empty', '-NFPanelReport=' + str(report)] if panel else ['-run=NodeForge', '-NFMode=' + mode, '-NFReport=' + str(report)]
    if config is not None:
        path = out / (name + 'Input.json'); write_new(path, config); options += ['-NFInput=' + str(path)]
    cmd = [str(ENGINE), str(project), *options, '-d3d11', '-RenderOffscreen', '-AllowCommandletRendering',
           '-unattended', '-nop4', '-nosound', '-NoLiveCoding', '-abslog=' + str(out / (name + '.log')), '-stdout']
    ran = run_guarded(cmd, cwd=ROOT, console=out / (name + 'Console.txt'), timeout=180,
        expected_module=plugin / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll')
    data = read_json(report) if report.is_file() else {}
    good = (ran.returncode != 0 and data.get('overallStatus') == 'rejected' and data.get('residue', {}).get('diskState') == 'notWritten') if rejected else (ran.returncode == 0 and (data.get('operationStatus') == 'passed' or panel and data.get('status') == 'passed'))
    if mode == 'GraphCatalog' and not panel:
        actual_catalog_digest = data.get('catalogDigest')
        current = data.get('catalog', {})
        good = good and isinstance(actual_catalog_digest, str) and len(actual_catalog_digest) == 64 and all(
            isinstance(current.get(kind, {}).get('nodes'), list) and current[kind]['nodes'] for kind in ('material', 'blueprint'))
    if panel:
        good = good and automation_report_passed(data, 'NodeForge.MinimalPanel')
    if mode == 'MaterialParameters' and not rejected:
        expected = next((v for v in graph['nativeParentInspections'].values() if v['parent'] == config['parent']), None)
        good = good and expected == data.get('parameters')
    good = good and before_assets == files_under(project_folder / 'Content', ASSET_SUFFIXES)
    steps.append({'step': name, 'result': 'passed' if good else 'failed', 'exitCode': ran.returncode,
                  'report': ref(report), 'assetBytesUnchanged': before_assets == files_under(project_folder / 'Content', ASSET_SUFFIXES)})
    print(name, steps[-1]['result'], flush=True)

execute('ActualCatalog', 'GraphCatalog')
for name, values in graph['nativeParentInspections'].items():
    execute(name, 'MaterialParameters', {'parent': values['parent']})
execute('InstanceParentRejected', 'MaterialParameters', {'parent': '/Game/NodeForgeTests/NFGraphInstanceWater.NFGraphInstanceWater'}, True)
execute('MissingParentRejected', 'MaterialParameters', {'parent': '/Game/NodeForgeTests/NoSuchParent.NoSuchParent'}, True)
execute('ExtraFieldRejected', 'MaterialParameters', {'parent': next(iter(graph['nativeParentInspections'].values()))['parent'], 'extra': True}, True)
execute('PanelConstruction', '', panel=True)
after_build = build_snapshot(plugin)
source_unchanged = before_source == files_under(source, ASSET_SUFFIXES)
protected_unchanged = before_protected == protected_assets()
good = len(steps) == 7 and all(x['result'] == 'passed' for x in steps) and before_build == after_build and source_unchanged and protected_unchanged
write_new(out / 'Summary.json', {'result': 'passed' if good else 'failed', 'steps': steps,
          'project': ref(project), 'selectedPlugin': ref(plugin), 'selectedBinarySha256': sha(plugin / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'),
          'sourceAndBinaryBefore': before_build, 'sourceAndBinaryAfter': after_build,
          'oneUnchangedBuild': before_build == after_build, 'allCopiedAssetBytesUnchanged': before_assets == files_under(project_folder / 'Content', ASSET_SUFFIXES),
          'historicalAssetSourceCatalogDigest': graph.get('catalogDigest'), 'currentCatalogDigest': actual_catalog_digest,
          'originalSourceAssetsUnchanged': source_unchanged, 'protectedRootAssetsUnchanged': protected_unchanged,
          'actualButtonClicks': 'notRun', 'clipboard': 'notTouched', 'externalAiUi': 'notRun',
          'scope': 'Actual read-only native outputs/rejections and isolated Slate panel construction; no button/clipboard/file-picker interaction proof'})
raise SystemExit(0 if good else 1)
