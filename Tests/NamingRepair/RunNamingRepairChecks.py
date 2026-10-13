"""Serial isolated naming regressions; reuse native import, verify and process guards.

Never builds or installs, modifies original AI JSON, substitutes graphs, overwrites
assets, or treats native automation as human UI acceptance. Use a fresh run name.
"""
import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'Scripts'))
from RunGraphRegression import (ENGINE, build_snapshot, protected_assets, files_under,
    read_json, write_new, ref, sha)
from RunProcessGuard import run_guarded, validate_isolated_project

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--run', required=True)
p.add_argument('--plugin-directory', required=True)
a = p.parse_args()
if not a.run.isascii() or not a.run.isalnum() or not 0 < len(a.run) <= 24:
    raise SystemExit('Fresh ASCII alphanumeric run at most 24 characters required')
plugin = (ROOT / a.plugin_directory).resolve()
if not plugin.is_relative_to(ROOT / 'Reports') or plugin.name != 'NodeForge':
    raise SystemExit('Explicit staged plugin required')
out = ROOT / 'Reports/20261010NamingRepair1' / a.run
folder = ROOT / 'ValidationProject/RegressionRuns' / a.run
if out.exists() or folder.exists():
    raise SystemExit('Prior batch protected; inspect existing terminal state')
out.mkdir(); folder.mkdir()
project = folder / (a.run + '.uproject')
write_new(project, {'FileVersion': 3, 'EngineAssociation': '5.3',
    'AdditionalPluginDirectories': [str(plugin.parent)],
    'Plugins': [{'Name': 'NodeForge', 'Enabled': True}]})
_, _, dll = validate_isolated_project(project, plugin, ROOT)
baseline, protected = build_snapshot(plugin), protected_assets()
rc1 = ROOT / 'Reports/20261009CandidateRC1/PluginStage/NodeForge'
rc1_before = files_under(rc1)
runners = [Path(__file__).resolve(), ROOT / 'Scripts/RunProcessGuard.py', ROOT / 'Scripts/RunGraphRegression.py']
runner_hashes = {ref(path): sha(path) for path in runners}
sources = {kind: ROOT / 'Reports/20261010NightAudit/Ui74' / (kind + 'Original.json')
           for kind in ['Blueprint', 'Material']}
source_hashes = {ref(path): sha(path) for path in sources.values()}
steps = []
write_new(out / 'Prerequisites.json', {'project': ref(project), 'plugin': ref(plugin),
    'binarySha256': sha(dll), 'sourceHashes': source_hashes, 'runnerHashes': runner_hashes,
    'scope': 'Actual native naming plus two graph families, identity-only copies, compile/save/independent reopen. No real UI click or new visual/runtime claims.'})


def execute(name, mode=None, input_path=None, previous=None, test=None, argument='NFReport', rejected=False):
    if runner_hashes != {ref(path): sha(path) for path in runners}:
        raise SystemExit('Runner changed during batch')
    report = out / (name + '.json')
    options = (['-ExecCmds=Automation RunTests NodeForge.' + test + '; Quit',
                '-TestExit=Automation Test Queue Empty', '-' + argument + '=' + str(report)]
               if test else ['-run=NodeForge', '-NFMode=' + mode, '-NFReport=' + str(report)])
    if input_path:
        options.append('-NFInput=' + str(input_path))
    if previous:
        options.append('-NFPrevious=' + str(previous))
    command = [str(ENGINE), str(project), *options, '-d3d11', '-RenderOffscreen',
        '-AllowCommandletRendering', '-unattended', '-nop4', '-nosound', '-NoLiveCoding',
        '-abslog=' + str(out / (name + '.log')), '-stdout', '-FullStdOutLogOutput']
    proc = run_guarded(command, cwd=ROOT, console=out / (name + 'Console.txt'),
        timeout=240, expected_module=dll)
    data = read_json(report) if report.is_file() else {}
    good = (proc.returncode != 0 and data.get('overallStatus') == 'rejected') if rejected else proc.returncode == 0
    if test:
        good = good and data.get('test') == 'NodeForge.' + test and data.get('status') == 'passed'
        good = good and bool(data.get('cases')) and all(row.get('passed') is True for row in data['cases'])
    elif not rejected:
        good = good and data.get('overallStatus') == ('waitingPersistence' if mode == 'Import' else 'succeeded')
    steps.append({'step': name, 'mode': mode or test, 'pid': proc.pid, 'exitCode': proc.returncode,
        'passed': good, 'report': ref(report)})
    write_new(out / (name + 'Checked.json'), steps[-1])
    print(name, 'passed' if good else 'failed', flush=True)
    if not good:
        raise SystemExit('Native regression failed; retain evidence: ' + name)
    return report, data


execute('NativeNaming', test='NamingCopies', argument='NFNamingReport')
execute('PanelInputGuards', test='PanelInputGuards', argument='NFPanelInputReport')
execute('CompactResults', test='CompactResults')
graphs = {}
for kind, source in sources.items():
    original = read_json(source)
    original_report, original_receipt = execute(kind + 'Original', 'Import', source)
    original_asset = folder / 'Content/NodeForgeTests' / (original['assetPath'].split('/')[-1] + '.uasset')
    original_asset_hash = sha(original_asset)
    _, refused = execute(kind + 'Duplicate', 'Import', source, rejected=True)
    if (refused.get('conflict', {}).get('kind') != 'targetAndRequestExist'
            or refused.get('residue', {}).get('diskState') != 'notWritten'
            or refused.get('checks', {}).get('build', {}).get('status') != 'notRun'
            or sha(original_asset) != original_asset_hash):
        raise SystemExit('Duplicate protection/diagnosis mismatch')
    copies = []
    for number in [1, 2]:
        prep_report, preparation = execute(kind + 'Prepare' + str(number), 'PrepareNewCopy', source)
        evidence = preparation['copyPreparation']
        derived = Path(evidence['preparedInputPath']).resolve()
        if not derived.is_relative_to(ROOT / 'Reports/PreparedCopies') or sha(derived) != evidence['preparedInputDigest']:
            raise SystemExit('Prepared input boundary/hash mismatch')
        request = read_json(derived)
        unchanged_fields = {k: v for k, v in original.items() if k not in ['assetPath', 'requestId']}
        if ({k: v for k, v in request.items() if k not in ['assetPath', 'requestId']} != unchanged_fields
                or request['requestId'] == original['requestId']
                or request['assetPath'] != original['assetPath'] + 'Copy' + str(number)
                or evidence['assetGenerated'] is not False or evidence['targetReserved'] is not False
                or sha(original_asset) != original_asset_hash
                or (folder / 'Saved/NodeForge/Requests' / (request['requestId'] + '.json')).exists()
                or any(preparation['checks'][key]['status'] != 'notRun' for key in ['build', 'compile', 'save', 'persistence'])):
            raise SystemExit('Preparation altered graph/asset or claimed generation/reservation')
        generated_report, generated = execute(kind + 'Copy' + str(number), 'Import', derived)
        asset = folder / 'Content/NodeForgeTests' / (request['assetPath'].split('/')[-1] + '.uasset')
        saved_hash = sha(asset)
        verify_report, verification = execute(kind + 'Reopen' + str(number), 'Verify', derived, generated_report)
        if (verification['pid'] == generated['pid'] or sha(asset) != saved_hash
                or verification['checks']['persistence']['status'] != 'passed'
                or sha(original_asset) != original_asset_hash
                or any(generated['checks'][key]['status'] != 'passed' for key in
                       ['validation', 'build', 'initialReadback', 'compile', 'finalReadback', 'publish', 'save'])):
            raise SystemExit('Copy save/reopen or original byte protection failed')
        copies.append({'number': number, 'preparedInput': ref(derived), 'preparationReport': ref(prep_report),
            'assetPath': request['assetPath'], 'savedSha256': saved_hash,
            'importReport': ref(generated_report), 'verifyReport': ref(verify_report),
            'nonIdentityFieldsEqual': True, 'sourceAssetBytesUnchanged': True})
    graphs[kind] = {'originalReport': ref(original_report), 'originalAssetSha256': original_asset_hash, 'copies': copies}

unchanged = (baseline == build_snapshot(plugin) and protected == protected_assets()
    and rc1_before == files_under(rc1) and source_hashes == {ref(path): sha(path) for path in sources.values()})
write_new(out / 'Summary.json', {'result': 'passed' if unchanged else 'failed', 'steps': steps,
    'graphs': graphs, 'project': ref(project), 'selectedPlugin': ref(plugin),
    'selectedBinarySha256': sha(dll), 'sourceInputHashes': source_hashes,
    'sourceBinaryRootAssetsRc1AndAiInputsUnchanged': unchanged,
    'scope': 'Native automation and actual independent UE import/save/verify; no human UI, animation or new runtime proof.'})
raise SystemExit(0 if unchanged else 1)
