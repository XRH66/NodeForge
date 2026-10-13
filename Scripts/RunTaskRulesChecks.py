"""Native task-rule export, exact catalog parity and rejection checks; no UI control."""
import argparse
from RunGraphRegression import ROOT, ENGINE, ASSET_SUFFIXES, build_snapshot, files_under, protected_assets, read_json, ref, sha, write_new
from RunProcessGuard import run_guarded

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--run', required=True)
parser.add_argument('--selection-run', required=True)
args = parser.parse_args()
if not all(v.isascii() and v.isalnum() and len(v) <= 24 for v in (args.run, args.selection_run)):
    raise SystemExit('Fresh ASCII alphanumeric run names required')
selection_dir = ROOT / 'Reports/20261009UsableEditor' / args.selection_run
selection = read_json(selection_dir / 'Summary.json')
if selection.get('result') != 'passed':
    raise SystemExit('Successful actual selection/panel run required')
plugin = ROOT / selection['selectedPlugin']
project = ROOT / selection['project']
baseline = build_snapshot(plugin)
if baseline != selection['sourceAndBinaryAfter']:
    raise SystemExit('Current build differs from selection evidence')
catalog_report = read_json(selection_dir / 'ActualCatalog.json')
catalog = catalog_report['catalog']
out = ROOT / 'Reports/20261009UsableEditor' / args.run
out.mkdir(exist_ok=False)
before = files_under(project.parent / 'Content', ASSET_SUFFIXES)
protected_before = protected_assets()
cases = [
 ('Multiply', {'assetKind': 'Material', 'query': 'multiply'}, False, 1),
 ('MathUnion', {'assetKind': 'Material', 'query': 'Multiply Add'}, False, 2),
 ('Animation', {'assetKind': 'Material', 'query': 'Time Panner MaterialExpressionSine'}, False, 3),
 ('MaterialAll', {'assetKind': 'Material', 'query': ''}, False, None),
 ('MaterialWhitespace', {'assetKind': 'Material', 'query': '   '}, False, None),
 ('RepeatedQuery', {'assetKind': 'Material', 'query': 'multiply MULTIPLY'}, False, 1),
 ('BlueprintMotion', {'assetKind': 'Blueprint', 'query': 'MakeVector K2_GetActorLocation'}, False, 2),
 ('BlueprintAll', {'assetKind': 'Blueprint', 'query': ''}, False, None),
 ('UnknownNode', {'assetKind': 'Material', 'query': 'NoSuchSupportedNode'}, True, 0),
 ('WrongFamily', {'assetKind': 'MaterialInstance', 'query': ''}, True, 0),
 ('WrongType', {'assetKind': 'Material', 'query': False}, True, 0),
 ('ExtraField', {'assetKind': 'Material', 'query': 'Multiply', 'extra': True}, True, 0),
 ('KindCaseAlias', {'assetKind': 'material', 'query': 'Multiply'}, True, 0),
 ('FieldCaseAlias', {'AssetKind': 'Material', 'query': 'Multiply'}, True, 0),
 ('QueryFieldCaseAlias', {'assetKind': 'Material', 'Query': 'Multiply'}, True, 0),
 ('LongQuery', {'assetKind': 'Material', 'query': 'x' * 257}, True, 0),
 ('ManyTerms', {'assetKind': 'Material', 'query': ' '.join(['Time'] * 9)}, True, 0),
]
steps = []
for name, config, rejected, wanted_count in cases:
    input_path = out / (name + 'Input.json'); write_new(input_path, config)
    report = out / (name + '.json')
    cmd = [str(ENGINE), str(project), '-run=NodeForge', '-NFMode=GraphRules',
        '-NFInput=' + str(input_path), '-NFReport=' + str(report), '-d3d11', '-RenderOffscreen',
        '-AllowCommandletRendering', '-unattended', '-nop4', '-nosound', '-NoLiveCoding',
        '-abslog=' + str(out / (name + '.log')), '-stdout', '-FullStdOutLogOutput']
    process = run_guarded(cmd, cwd=ROOT, console=out / (name + 'Console.txt'), timeout=180,
        expected_module=plugin / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll')
    data = read_json(report) if report.is_file() else {}
    errors = []
    if rejected:
        if process.returncode == 0 or data.get('overallStatus') != 'rejected' or data.get('residue', {}).get('diskState') != 'notWritten' or 'taskRules' in data:
            errors.append('Invalid query did not reject without a replacement bundle')
    else:
        bundle = data.get('taskRules', {})
        section = catalog['material' if config['assetKind'] == 'Material' else 'blueprint']
        terms = config['query'].lower().split()
        expected = []
        for entry in section['nodes']:
            identity = ' '.join([entry['class'], entry.get('properties', {}).get('owner', ''),
                entry.get('properties', {}).get('member', ''), entry.get('variableType', '')]).lower()
            if not terms or any(term in identity for term in terms):
                expected.append(entry)
        expected_section = {**section, 'nodes': expected}
        if wanted_count is None:
            wanted_count = len(expected)
        if process.returncode != 0 or data.get('operationStatus') != 'passed':
            errors.append('Actual native export failed')
        if bundle.get('catalogDigest') != catalog_report['catalogDigest'] or data.get('catalogDigest') != catalog_report['catalogDigest']:
            errors.append('Full native catalog digest was not retained')
        if bundle.get('capabilities') != expected_section or len(expected) != wanted_count or bundle.get('matchedNodeCount') != wanted_count:
            errors.append('Actual subset/pins/properties/policies/count differs from the full native source')
        if bundle.get('format') != 'NodeForgeTaskRules' or bundle.get('inputVersion') != 2:
            errors.append('Rules improperly presented as an import request')
    if data.get('readOnlyExecutionFingerprint', {}).get('binarySha256') != sha(plugin / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'):
        errors.append('Native loaded build identity missing or wrong')
    if before != files_under(project.parent / 'Content', ASSET_SUFFIXES) or protected_before != protected_assets():
        errors.append('Read-only export changed saved assets')
    steps.append({'case': name, 'result': 'passed' if not errors else 'failed', 'errors': errors,
        'pid': process.pid, 'exitCode': process.returncode, 'report': ref(report), 'expectedCount': wanted_count})
    print(name, steps[-1]['result'], errors, flush=True)
after = build_snapshot(plugin)
good = all(step['result'] == 'passed' for step in steps) and after == baseline
write_new(out / 'Summary.json', {'result': 'passed' if good else 'failed', 'steps': steps,
    'project': ref(project), 'selectedPlugin': ref(plugin), 'selectedBinarySha256': sha(plugin / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'),
    'sourceAndBinaryBefore': baseline, 'sourceAndBinaryAfter': after, 'oneUnchangedBuild': baseline == after,
    'copiedAssetBytesUnchanged': before == files_under(project.parent / 'Content', ASSET_SUFFIXES),
    'rootProtectedAssetsUnchanged': protected_before == protected_assets(), 'fullCatalogDigest': catalog_report['catalogDigest'],
    'scope': 'Native read-only task subsets/rejections; exact source signature/policy equality; no new graph compilation, human buttons/clipboard or external AI proof'})
raise SystemExit(0 if good else 1)
