"""Independently reopen one real UI-saved self-contained v2 graph; never import.

Reuse ValidateCandidateUi's receipt/copy/Verify flow and RunGraphRegression's
structural comparison and protected snapshots. Stdlib only, no desktop control.
"""
import argparse
import shutil
from RunGraphRegression import (ROOT, ENGINE, ASSET_SUFFIXES, REQUIRED_IMPORT_CHECKS,
    read_json, write_new, ref, sha, files_under, build_snapshot, protected_assets,
    readback_errors, layout_positions)
from RunProcessGuard import run_guarded

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--run', required=True)
parser.add_argument('--input', required=True)
parser.add_argument('--receipt', required=True)
parser.add_argument('--plugin-directory', required=True)
parser.add_argument('--loaded-evidence', required=True)
args = parser.parse_args()
if not args.run.isascii() or not args.run.isalnum() or len(args.run) > 24:
    raise SystemExit('Fresh ASCII alphanumeric run required, at most 24 characters')

def local(value, parent=ROOT):
    path = (ROOT / value).resolve()
    if not path.is_relative_to(parent.resolve()):
        raise SystemExit('Explicit local project path required')
    return path

inp = local(args.input)
receipt = local(args.receipt, ROOT / 'Reports/UiImports')
plugin = local(args.plugin_directory, ROOT / 'Reports')
loaded_path = local(args.loaded_evidence, ROOT / 'Reports')
if plugin.name != 'NodeForge':
    raise SystemExit('Explicit staged NodeForge plugin required')
out = ROOT / 'Reports/20261009UsableEditor' / args.run
folder = ROOT / 'ValidationProject/RegressionRuns' / args.run
if out.exists() or folder.exists():
    raise SystemExit('Existing evidence/project protected; inspect previous run, do not restart')
out.mkdir()
request, original, loaded = read_json(inp), read_json(receipt), read_json(loaded_path)
issues = []
ui_project = local(original.get('project', ''), ROOT / 'ValidationProject/RegressionRuns')
asset_path = request.get('assetPath', '')
if (request.get('format') != 'NodeForgeGraph' or request.get('version') != 2
    or request.get('assetType') not in ('Material', 'Blueprint')
    or not asset_path.startswith('/Game/NodeForgeTests/')
    or '..' in asset_path or '\\' in asset_path):
    issues.append('Only self-contained ordinary Material/Actor Blueprint v2 targets supported')
asset = (ui_project.parent / 'Content' / (asset_path.removeprefix('/Game/') + '.uasset')).resolve()
if not asset.is_relative_to((ui_project.parent / 'Content/NodeForgeTests').resolve()):
    raise SystemExit('Saved UI asset must be inside protected test content')
dll = plugin / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'
selected_sha = sha(dll)
if (original.get('mode') != 'Import' or original.get('overallStatus') != 'waitingPersistence'
    or original.get('inputMethod') not in ('file', 'paste', 'draft')
    or original.get('inputDigest') != sha(inp)
    or original.get('requestId') != request.get('requestId')
    or original.get('assetPath') != asset_path):
    issues.append('Requires exact actual UI import receipt, input bytes and target')
if original.get('inputMethod') == 'draft':
    ui = original.get('uiInput', {})
    if (not isinstance(ui, dict) or ui.get('contentSource') != 'currentDraft'
        or ui.get('origin') not in ('manual', 'file', 'clipboard', 'export')
        or not isinstance(ui.get('sourceEdited'), bool)
        or not isinstance(ui.get('draftRevision'), str) or not ui['draftRevision'].isascii() or not ui['draftRevision'].isdigit()
        or ui.get('snapshotDigest') != sha(inp)
        or local(ui.get('snapshotPath', ''), ROOT / 'Reports/UiInputs') != inp):
        issues.append('Current-draft provenance must identify exact immutable UI snapshot, revision and origin')
if (original.get('executionFingerprint', {}).get('binarySha256') != selected_sha
    or loaded.get('sha256') != selected_sha or loaded.get('pid') != original.get('pid')
    or local(loaded.get('path', ''), ROOT / 'Reports') != dll.resolve()):
    issues.append('Actual UI loaded DLL evidence does not match selected unchanged build')
for check in REQUIRED_IMPORT_CHECKS:
    if original.get('checks', {}).get(check, {}).get('status') != 'passed':
        issues.append('Actual UI generation check not passed: ' + check)
if not asset.is_file() or sha(asset) != original.get('fileSha256'):
    issues.append('Actual saved UI asset bytes differ from receipt')
for node in request.get('nodes', []):
    if node.get('class') == '/Script/Engine.MaterialExpressionMaterialFunctionCall':
        issues.append('Bound function signatures/dependencies require separate verifier; no guesses')
    for value in node.get('properties', {}).values():
        if isinstance(value, str) and value.startswith('/Game/'):
            issues.append('Additional /Game resource dependencies require an explicit copy manifest')
issues += readback_errors(request, original.get('publishedReadback'))
baseline = build_snapshot(plugin)
source_before = files_under(ui_project.parent / 'Content', ASSET_SUFFIXES)
protected_before = protected_assets()
immutable = {ref(p): sha(p) for p in (inp, receipt, loaded_path)}
candidate = ROOT / 'Reports/20261009CandidateRC1/PluginStage/NodeForge'
candidate_before = files_under(candidate)
write_new(out / 'Prerequisites.json', {
    'result': 'ready' if not issues else 'blocked', 'issues': issues,
    'uiProject': ref(ui_project), 'input': ref(inp), 'receipt': ref(receipt),
    'actualUiInputMethod': original.get('inputMethod'), 'uiPid': original.get('pid'),
    'savedAsset': ref(asset), 'selectedPlugin': ref(plugin), 'dllSha256': selected_sha,
    'scope': 'Real UI receipt and saved bytes; no backend import or desktop control',
    'sourceAssetsBefore': source_before, 'sourceAndBinaryBefore': baseline,
    'immutableInputsBefore': immutable, 'candidateBefore': candidate_before})
if issues:
    write_new(out / 'Summary.json', {'result': 'blocked', 'issues': issues, 'nativeRun': 'notRun'})
    raise SystemExit(2)

folder.mkdir()
project = folder / (args.run + '.uproject')
write_new(project, {'FileVersion': 3, 'EngineAssociation': '5.3',
    'AdditionalPluginDirectories': [str(plugin.parent)],
    'Plugins': [{'Name': 'NodeForge', 'Enabled': True}]})
copy = folder / 'Content' / asset.relative_to(ui_project.parent / 'Content')
copy.parent.mkdir(parents=True)
shutil.copyfile(asset, copy)
copy_before = files_under(folder / 'Content', ASSET_SUFFIXES)
report = out / 'Reopen.json'
cmd = [str(ENGINE), str(project), '-run=NodeForge', '-NFMode=Verify',
    '-NFInput=' + str(inp), '-NFPrevious=' + str(receipt), '-NFReport=' + str(report),
    '-d3d11', '-RenderOffscreen', '-AllowCommandletRendering', '-unattended',
    '-nop4', '-nosound', '-NoLiveCoding', '-abslog=' + str(out / 'Reopen.log'),
    '-stdout', '-FullStdOutLogOutput']
process = run_guarded(cmd, cwd=ROOT, console=out / 'ReopenConsole.txt', timeout=240, expected_module=dll)

data = read_json(report) if report.is_file() else {}
errors = readback_errors(request, data.get('reopenedFinalReadback'))
if (process.returncode != 0 or data.get('overallStatus') != 'succeeded'
    or data.get('checks', {}).get('persistence', {}).get('status') != 'passed'
    or data.get('pid') == original.get('pid')
    or data.get('executionFingerprint') != original.get('executionFingerprint')
    or data.get('fileSha256') != original.get('fileSha256')):
    errors.append('Independent persistence, fresh PID, exact loaded fingerprint or saved SHA failed')
if layout_positions(original.get('publishedReadback')) != layout_positions(data.get('reopenedFinalReadback')):
    errors.append('Saved layout differs after independent reopen')
unchanged = {
    'uiSourceAssets': source_before == files_under(ui_project.parent / 'Content', ASSET_SUFFIXES),
    'copiedAssetBytes': copy_before == files_under(folder / 'Content', ASSET_SUFFIXES),
    'sourceAndBinary': baseline == build_snapshot(plugin),
    'rootProtectedAssets': protected_before == protected_assets(),
    'immutableInputAndReceipt': immutable == {ref(p): sha(p) for p in (inp, receipt, loaded_path)},
    'frozenCandidate': candidate_before == files_under(candidate)}
if not all(unchanged.values()):
    errors.append('Protected byte snapshots changed')
write_new(out / 'Summary.json', {'result': 'passed' if not errors else 'failed',
    'errors': errors, 'nativeExitCode': process.returncode, 'nativePid': data.get('pid'),
    'uiPid': original['pid'], 'uiInputMethod': original['inputMethod'],
    'uiReceipt': ref(receipt), 'input': ref(inp), 'verificationProject': ref(project),
    'selectedPlugin': ref(plugin), 'actualLoadedFingerprint': data.get('executionFingerprint'),
    'reopenedReport': ref(report), 'protection': unchanged,
    'scope': 'One exact UI-saved graph/parameters/edges/layout reloaded and compiled in fresh process; original receipt preserved',
    'visualRuntime': 'notRun', 'preImportUiPreview': 'notVerified',
    'build33Buttons': 'notRun'})
print('UI saved asset independent reopen:', 'passed' if not errors else 'failed', flush=True)
raise SystemExit(0 if not errors else 1)
