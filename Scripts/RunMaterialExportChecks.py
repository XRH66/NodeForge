"""Read actual saved graphs, import new copies and independently reopen; no UI control.

Material is the historical default; --asset-kind Blueprint reuses the same protection,
native execution, receipts and structural checks instead of a second runner.
"""
import argparse
import shutil
import subprocess
import time
from RunGraphRegression import (ROOT, ENGINE, ASSET_SUFFIXES, REQUIRED_IMPORT_CHECKS,
    build_snapshot, files_under, protected_assets, read_json, ref, sha, write_new,
    readback_errors, subset_errors)
from RunProcessGuard import run_guarded

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--run', required=True)
parser.add_argument('--source-run', required=True)
parser.add_argument('--plugin-directory', required=True)
parser.add_argument('--asset-kind', choices=['Material', 'Blueprint'], default='Material')
parser.add_argument('--case', action='append', help='Representative saved source cases only; synthetic fixture suite is explicitly not run')
args = parser.parse_args()
if not all(v.isascii() and v.isalnum() and 0 < len(v) <= 24 for v in (args.run, args.source_run)):
    raise SystemExit('Fresh ASCII alphanumeric run names, at most 24 characters required')
plugin = (ROOT / args.plugin_directory).resolve()
if not plugin.is_relative_to(ROOT / 'Reports') or plugin.name != 'NodeForge':
    raise SystemExit('Explicit staged plugin required')
matrix_dir = ROOT / 'Reports/20261008GraphV2' / args.source_run
matrix = read_json(matrix_dir / 'Summary.json')
if matrix.get('result') != 'passed':
    raise SystemExit('Passing saved graph source required, not inherited current-build verification')
source = (ROOT / matrix['project']).parent / 'Content'
if not source.resolve().is_relative_to(ROOT / 'ValidationProject/RegressionRuns'):
    raise SystemExit('Only isolated project source permitted')
blueprint = args.asset_kind == 'Blueprint'
names = args.case or (['BlueprintText', 'BlueprintTextChanged', 'BlueprintBranch', 'BlueprintBranchFalse', 'BlueprintConcat', 'BlueprintZeroDefaults']
         if blueprint else ['StaticColor', 'MaterialDefaults', 'TextureUv', 'DynamicPulse', 'Water'])
if args.case:
    known = {c['name']: c for c in matrix.get('cases', [])}
    if len(set(names)) != len(names) or any(known.get(n, {}).get('result') != 'passed'
            or known[n].get('expected') != 'succeeded' for n in names):
        raise SystemExit('Selected source cases must be unique passing generated graphs')
receipts = {}
for name in names:
    receipt = read_json(matrix_dir / (name + 'Import.json'))
    file = source / (receipt['assetPath'].removeprefix('/Game/') + '.uasset')
    if receipt.get('assetType') != args.asset_kind or receipt.get('fileSha256') != sha(file):
        raise SystemExit('Source saved bytes or type differ: ' + name)
    receipts[name] = receipt
out = ROOT / 'Reports/20261009UsableEditor' / args.run
folder = ROOT / 'ValidationProject/RegressionRuns' / args.run
if out.exists() or folder.exists():
    raise SystemExit('Use fresh project and report; no evidence overwritten')
out.mkdir(); folder.mkdir()
source_before = files_under(source, ASSET_SUFFIXES)
shutil.copytree(source, folder / 'Content')
project = folder / ('NodeForgeBlueprintExport.uproject' if blueprint else 'NodeForgeMaterialExport.uproject')
write_new(project, {'FileVersion': 3, 'EngineAssociation': '5.3',
    'AdditionalPluginDirectories': [str(plugin.parent)], 'Plugins': [{'Name': 'NodeForge', 'Enabled': True}]})
initial = files_under(folder / 'Content', ASSET_SUFFIXES)
if {str((ROOT / p).relative_to(source)): h for p, h in source_before.items()} != {
    str((ROOT / p).relative_to(folder / 'Content')): h for p, h in initial.items()}:
    raise SystemExit('Copied source assets differ')
baseline, protected_before = build_snapshot(plugin), protected_assets()
write_new(out / 'RunnerEvidence.json', {'runner': ref(ROOT / 'Scripts/RunMaterialExportChecks.py'),
    'runnerSha256': sha(ROOT / 'Scripts/RunMaterialExportChecks.py'),
    'sharedRunner': ref(ROOT / 'Scripts/RunGraphRegression.py'),
    'sharedRunnerSha256': sha(ROOT / 'Scripts/RunGraphRegression.py'), 'sourceGraphMatrix': args.source_run,
    'sourceGraphMatrixSha256': sha(matrix_dir / 'Summary.json')})
binary_sha = sha(plugin / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll')
steps, cases = [], []


def execute(name, options):
    report = out / (name + '.json')
    cmd = [str(ENGINE), str(project), *options, '-d3d11', '-RenderOffscreen', '-AllowCommandletRendering',
        '-unattended', '-nop4', '-nosound', '-NoLiveCoding', '-abslog=' + str(out / (name + '.log')),
        '-stdout', '-FullStdOutLogOutput']
    write_new(out / (name + 'Command.json'), cmd)
    process = run_guarded(cmd, cwd=ROOT, console=out / (name + 'Console.txt'), timeout=240,
        expected_module=plugin / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll')
    data = read_json(report) if report.is_file() else {}
    steps.append({'step': name, 'pid': process.pid, 'exitCode': process.returncode, 'report': ref(report)})
    return data, process.returncode


def native(name, mode, input_path=None, previous=None):
    options = ['-run=NodeForge', '-NFMode=' + mode, '-NFReport=' + str(out / (name + '.json'))]
    if input_path: options.append('-NFInput=' + str(input_path))
    if previous: options.append('-NFPrevious=' + str(previous))
    return execute(name, options)


catalog, code = native('ActualCatalog', 'GraphCatalog')
if code != 0 or catalog.get('operationStatus') != 'passed':
    raise SystemExit('Current actual catalog export failed')
fixture_report = {'cases': []}
if not args.case:
    fixture_mode = 'BlueprintExportFixtures' if blueprint else 'MaterialExportFixtures'
    fixture_name = names[0]
    fixture_report, code = execute('Fixtures', ['-ExecCmds=Automation RunTests NodeForge.' + fixture_mode + '; Quit',
        '-TestExit=Automation Test Queue Empty', '-NFExportSource=' + receipts[fixture_name]['assetPath'] + '.' + receipts[fixture_name]['assetPath'].rsplit('/', 1)[-1],
        '-NFExportFixtureReport=' + str(out / 'Fixtures.json')])
    if code != 0 or fixture_report.get('status') != 'passed' or len(fixture_report.get('cases', [])) != (13 if blueprint else 9):
        write_new(out / 'FixtureFailure.json', {'result': 'failed', 'steps': steps, 'assetKind': args.asset_kind,
            'reason': 'Native fixture setup did not finish; partial isolated assets/evidence retained; no matrix pass claimed',
            'project': ref(project), 'selectedBinarySha256': binary_sha, 'selectedPlugin': ref(plugin)})
        raise SystemExit('Native fixture setup failed; preserve partial assets/evidence')
saved_sources = files_under(folder / 'Content', ASSET_SUFFIXES)
export_cases = [{'case': name, 'source': r['assetPath'] + '.' + r['assetPath'].rsplit('/', 1)[-1],
                 'expected': 'passed', 'originalReadback': r['publishedReadback']} for name, r in receipts.items()]
export_cases += fixture_report['cases']
export_cases += [
 {'case': 'Missing', 'source': '/Game/NodeForgeTests/NFNotPresent.NFNotPresent', 'expected': 'rejected'},
 {'case': 'WrongKind', 'source': '/Engine/BasicShapes/Plane.Plane', 'expected': 'rejected'},
 {'case': 'OutsidePath', 'source': '/Game/Unrelated.Asset', 'expected': 'rejected'},
 {'case': 'ExistingTarget', 'source': export_cases[0]['source'], 'expected': 'rejected', 'target': receipts[names[0]]['assetPath']},
 {'case': 'ExtraConfig', 'source': export_cases[0]['source'], 'expected': 'rejected', 'extra': True},
]
for item in export_cases:
    name, expected = item['case'], item['expected']
    config = {'source': item['source'], 'requestId': 'GraphExport' + name,
        'assetPath': item.get('target', '/Game/NodeForgeTests/NFExportCopy' + name)}
    if item.get('extra'): config['extra'] = True
    config_file = out / (name + 'Config.json'); write_new(config_file, config)
    before = files_under(folder / 'Content', ASSET_SUFFIXES)
    data, code = native(name + 'Export', 'BlueprintGraphExport' if blueprint else 'MaterialGraphExport', config_file)
    errors = []
    if before != files_under(folder / 'Content', ASSET_SUFFIXES): errors.append('Export changed saved assets')
    if data.get('readOnlyExecutionFingerprint', {}).get('binarySha256') != binary_sha: errors.append('Actual export DLL mismatch')
    if expected == 'rejected':
        if code == 0 or data.get('overallStatus') != 'rejected' or 'exportedRequest' in data or data.get('residue', {}).get('diskState') != 'notWritten':
            errors.append('Unsupported export was not explicitly rejected without a partial request')
    else:
        request, evidence = data.get('exportedRequest', {}), data.get('exportEvidence', {})
        if code != 0 or data.get('operationStatus') != 'passed': errors.append('Native actual export failed')
        if request.get('catalogDigest') != catalog.get('catalogDigest') or request.get('requestId') != config['requestId'] or request.get('assetPath') != config['assetPath']:
            errors.append('Export envelope does not match new request/target/current catalog')
        source_file = folder / 'Content' / (item['source'].split('.')[0].removeprefix('/Game/') + '.uasset')
        if evidence.get('sourceAssetSha256') != sha(source_file) or not evidence.get('sourceBytesUnchanged') or not evidence.get('sourceRemainsClean'):
            errors.append('Read-only source identity missing')
        if item.get('originalReadback'):
            original = item['originalReadback']
            mapping = {r['sourceComment' if blueprint else 'sourceDescription'].removeprefix('NodeForge:'): r['id'] for r in evidence.get('logicalIdMapping', [])}
            if len(mapping) != len(original['nodes']): errors.append('Actual exported logical ID mapping missing')
            else:
                nodes = {n['id']: n for n in request['nodes']}
                for n in original['nodes']:
                    found = nodes.get(mapping.get(n['id']), {})
                    if blueprint:
                        effective = found.get('properties', {})
                        errors.extend(subset_errors(n.get('defaults', {}), found.get('defaults', {}), n['id'] + '.defaults'))
                    else:
                        signature = next(e for e in catalog['catalog']['material']['nodes'] if e['class'] == n['class'])
                        effective = {k: v.get('default') for k, v in signature['properties'].items()}
                        effective.update(found.get('properties', {}))
                    errors.extend(subset_errors(n['properties'], effective, n['id'] + '.effectiveProperties', 'double' if blueprint else 'float32'))
                    if n['class'] != found.get('class') or n['position'] != found.get('position'): errors.append('Source class or saved position changed')
                actual_edges = sorted(tuple(e[k] for k in ('from', 'output', 'to', 'input')) for e in request['edges'])
                old_edges = sorted((mapping[e['from']], e['output'], mapping[e['to']] if e['to'] != 'Material' else 'Material', e['input']) for e in original['edges'])
                source_settings = original['settings']
                if blueprint:
                    source_settings = {'parentClass': source_settings['parentClass'], 'variables': [
                        {k: v[k] for k in ('name', 'type', 'default')} for v in source_settings['variables']]}
                if actual_edges != old_edges or request['settings'] != source_settings: errors.append('Export differs from independent saved graph receipt')
        if blueprint:
            errors.extend(readback_errors(request, evidence.get('sourceReadback')))
        if name in ('Untagged', 'Changed') and not blueprint:
            if len(request.get('nodes', [])) != 2 or len(request.get('edges', [])) != 2 or any(r.get('sourceDescription', '').startswith('NodeForge:') for r in evidence.get('logicalIdMapping', [])):
                errors.append('Native untagged fixture shape or no-tag condition differs')
            if name == 'Changed':
                parameters = {n['properties'].get('ParameterName'): n['properties'].get('DefaultValue') for n in request.get('nodes', [])}
                errors.extend(subset_errors({'SurfaceColor': [.7, .1, .3, 1], 'SurfaceRoughness': .65}, parameters, number_policy='float32'))
        if blueprint and name in ('Untagged', 'Changed'):
            if len(request.get('nodes', [])) != 2 or len(request.get('edges', [])) != 1 or any(
                    r.get('sourceComment', '').startswith('NodeForge:') for r in evidence.get('logicalIdMapping', [])):
                errors.append('Native untagged source graph shape/identity differs')
            if name == 'Changed':
                actual_vars = {v['name']: v['default'] for v in request['settings']['variables']}
                errors.extend(subset_errors({'ExportBool': False, 'ExportNumber': 0, 'ExportText': 'Native saved default', 'ExportVector': [1, 2, 3]}, actual_vars))
                if not any(n.get('defaults', {}).get('InString') == 'Exported native changed graph' for n in request['nodes']):
                    errors.append('Changed native pin default was not actually exported')
        if not errors:
            input_file = out / (name + 'RoundtripInput.json'); write_new(input_file, request)
            imported, code = native(name + 'Import', 'Import', input_file)
            if code != 0 or imported.get('overallStatus') != 'waitingPersistence': errors.append('Roundtrip import did not compile/save')
            if imported.get('executionFingerprint', {}).get('binarySha256') != binary_sha: errors.append('Import DLL mismatch')
            if imported.get('inputDigest') != sha(input_file): errors.append('Import receipt bytes differ')
            for check in REQUIRED_IMPORT_CHECKS:
                if imported.get('checks', {}).get(check, {}).get('status') != 'passed': errors.append('Import check not passed: ' + check)
            for field in ('initialReadback', 'finalReadback', 'publishedReadback'): errors.extend(readback_errors(request, imported.get(field)))
            target = folder / 'Content' / (request['assetPath'].removeprefix('/Game/') + '.uasset')
            if not target.is_file() or imported.get('fileSha256') != sha(target): errors.append('Saved copy missing or wrong bytes')
            if target.is_file() and not errors:
                after_import = files_under(folder / 'Content', ASSET_SUFFIXES)
                verify, code = native(name + 'Reopen', 'Verify', input_file, out / (name + 'Import.json'))
                if code != 0 or verify.get('overallStatus') != 'succeeded' or verify.get('checks', {}).get('persistence', {}).get('status') != 'passed': errors.append('Independent reopen failed')
                if imported.get('pid') == verify.get('pid'): errors.append('Reopen process not independent')
                for field in ('reopenedReadback', 'reopenedFinalReadback'): errors.extend(readback_errors(request, verify.get(field)))
                if after_import != files_under(folder / 'Content', ASSET_SUFFIXES): errors.append('Reopen changed saved assets')
    if any(files_under(folder / 'Content', ASSET_SUFFIXES).get(p) != h for p, h in saved_sources.items()): errors.append('An original/copied native source asset changed')
    row = {'case': name, 'expected': expected, 'result': 'passed' if not errors else 'failed', 'errors': errors,
           'export': ref(out / (name + 'Export.json'))}
    cases.append(row); write_new(out / (name + 'Result.json'), row)
    print(name, row['result'], errors, flush=True)
after = build_snapshot(plugin)
unchanged = source_before == files_under(source, ASSET_SUFFIXES) and protected_before == protected_assets()
passed = all(c['result'] == 'passed' for c in cases) and baseline == after and unchanged
runner_evidence = read_json(out / 'RunnerEvidence.json')
runner_unchanged = sha(ROOT / runner_evidence['runner']) == runner_evidence['runnerSha256'] and sha(ROOT / runner_evidence['sharedRunner']) == runner_evidence['sharedRunnerSha256']
passed = passed and runner_unchanged
write_new(out / 'Summary.json', {'result': 'passed' if passed else 'failed', 'cases': cases, 'steps': steps,
    'project': ref(project), 'assetKind': args.asset_kind, 'selectedPlugin': ref(plugin), 'selectedBinarySha256': binary_sha,
    'sourceAndBinaryBefore': baseline, 'sourceAndBinaryAfter': after, 'oneUnchangedBuild': baseline == after,
    'originalSourceAssetsUnchanged': unchanged, 'allCopiedSourceAssetBytesUnchanged': all(files_under(folder / 'Content', ASSET_SUFFIXES).get(p) == h for p, h in saved_sources.items()),
    'runnerFilesUnchanged': runner_unchanged,
    'selectedSourceCases': names, 'fullStandardMatrix': not bool(args.case),
    'nativeFixtures': 'notRun' if args.case else 'passed',
    'scope': 'Readonly actual native graph export, guarded unsupported configurations, supported new copies compile/save/independent reopen; no human button, runtime, visual or packaging proof'})
raise SystemExit(0 if passed else 1)
