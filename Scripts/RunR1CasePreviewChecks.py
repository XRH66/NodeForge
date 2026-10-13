"""Read-only R1 integration checks for exact-case preview against saved UE assets.

Root schedules this runner; importing it or --help never launches Unreal. Reuses
RunGraphRegression snapshot/JSON helpers and RunGraphPreviewChecks' isolated
project, real source export and commandlet execution pattern. No plugin build,
asset import, source mutation, UI automation or third-party dependency install.
"""
from __future__ import annotations

import argparse
import copy
import json
import re
import shutil
import subprocess
import time
from pathlib import Path

from RunGraphRegression import (
    ROOT, ENGINE, ASSET_SUFFIXES, build_snapshot, files_under, protected_assets,
    read_json, ref, sha, write_new,
)


BUILD58_PLUGIN = 'Reports/20261009UsableEditor/Build58/PluginStage/NodeForge'
BUILD58_SHA = '56d302e9bbef3f5cd528f896cb631a29cd5ad3d14e9b6b6fbd47b94548571aba'
DIFFERENCE_FIELDS = (
    'nodesAdded', 'nodesRemoved', 'nodesChanged', 'positionsChanged',
    'edgesAdded', 'edgesRemoved', 'settingsChanged',
)
NO_EXECUTION_CHECKS = (
    'build', 'initialReadback', 'compile', 'finalReadback', 'publish',
    'save', 'persistence', 'runtime',
)


def canonical(value):
    return json.dumps(value, ensure_ascii=False, sort_keys=True,
                      separators=(',', ':'), allow_nan=False)


def unordered(values):
    return sorted(canonical(value) for value in values)


def empty_difference():
    return {**{key: [] for key in DIFFERENCE_FIELDS},
            'requestStructureOrValuesChanged': False, 'layoutChanged': False}


def order_projection(request):
    """Compare JSON structure/values, not an assertion of native semantic equivalence."""
    result = copy.deepcopy(request)
    result.pop('requestId', None)
    result.pop('assetPath', None)
    for key in ('nodes', 'edges'):
        result[key] = sorted(result[key], key=canonical)
    return canonical(result)


class NativeStillRunning(Exception):
    pass


class SafetyFailure(Exception):
    pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--run', required=True, help='Fresh ASCII alphanumeric run, at most 20 characters')
    parser.add_argument('--plugin-directory', default=BUILD58_PLUGIN)
    parser.add_argument('--expected-dll-sha256', default=BUILD58_SHA,
                        help='Explicit frozen build fingerprint; defaults to Build58')
    parser.add_argument('--timeout-seconds', type=int, default=240,
                        help='Per native step; timeout records live child and never restarts it')
    args = parser.parse_args()
    if not re.fullmatch(r'[A-Za-z0-9]{1,20}', args.run):
        parser.error('--run must be fresh ASCII alphanumeric, at most 20 characters')
    if not re.fullmatch(r'[0-9a-fA-F]{64}', args.expected_dll_sha256):
        parser.error('Expected DLL SHA256 must contain 64 hexadecimal characters')
    if not 30 <= args.timeout_seconds <= 900:
        parser.error('--timeout-seconds must be between 30 and 900')
    plugin = (ROOT / args.plugin_directory).resolve()
    if not plugin.is_relative_to((ROOT / 'Reports').resolve()) or plugin.name != 'NodeForge':
        parser.error('Only an explicitly staged NodeForge plugin under Reports is allowed')
    binary_path = plugin / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'
    if not binary_path.is_file() or sha(binary_path) != args.expected_dll_sha256.lower():
        parser.error('Selected staged DLL does not match its expected frozen fingerprint')
    if not ENGINE.is_file():
        parser.error('Existing official UE5.3 commandlet executable is unavailable')
    source = ROOT / 'ValidationProject/RegressionRuns/CandidateVerify1/Content'
    source_summary = ROOT / 'Reports/20261009CandidateRC1/CandidateVerify1/Summary.json'
    source_summary_sha = sha(source_summary)
    source_data = read_json(source_summary)
    if source_data.get('result') != 'passed':
        parser.error('Existing actual UI asset verification evidence is not passed')
    verified_source_hashes = {}
    for kind in ('Blueprint', 'Material'):
        source_asset = source / 'NodeForgeTests' / ('NFExternalGraph' + kind + '2.uasset')
        if not source_asset.is_file():
            parser.error('Required real saved source asset is missing: ' + kind)
        expected_source_sha = source_data.get('receipts', {}).get(kind, {}).get('assetSha256')
        observed_source_sha = sha(source_asset)
        if observed_source_sha != expected_source_sha:
            parser.error('Saved source does not exactly match verified UI receipt assetSha256: ' + kind)
        verified_source_hashes[kind] = {'file': ref(source_asset), 'sha256': observed_source_sha,
                                      'receiptField': 'receipts.' + kind + '.assetSha256'}
    if sha(source_summary) != source_summary_sha:
        parser.error('Source verification summary changed during preflight')
    runner = Path(__file__).resolve()
    runner_sha = sha(runner)
    out = ROOT / 'Reports/20261009UsableEditor' / args.run
    folder = ROOT / 'ValidationProject/RegressionRuns' / args.run
    if out.exists() or folder.exists():
        parser.error('Run/report or project already exists; inspect its state, do not restart or overwrite')

    # Take protection snapshots before any copy. Existing root and RC1 remain untouched.
    baseline = build_snapshot(plugin)
    protected = protected_assets()
    frozen_rc1 = files_under(ROOT / 'Reports/20261009CandidateRC1/PluginStage/NodeForge')
    rc1_assets = files_under(ROOT / 'ValidationProject/RegressionRuns/CandidateRC1/Content', ASSET_SUFFIXES)
    source_before = files_under(source, ASSET_SUFFIXES)
    out.mkdir(exist_ok=False)
    folder.mkdir(exist_ok=False)
    shutil.copytree(source, folder / 'Content')
    project = folder / (args.run + '.uproject')
    write_new(project, {'FileVersion': 3, 'EngineAssociation': '5.3',
                        'AdditionalPluginDirectories': [str(plugin.parent)],
                        'Plugins': [{'Name': 'NodeForge', 'Enabled': True}]})
    initial = files_under(folder / 'Content', ASSET_SUFFIXES)
    binary = sha(binary_path)
    steps, cases = [], []
    write_new(out / 'Preparation.json', {
        'project': ref(project), 'selectedPlugin': ref(plugin), 'dllSha256': binary,
        'sourceEvidence': ref(source_summary), 'sourceEvidenceSha256': source_summary_sha,
        'verifiedSourceReceiptHashes': verified_source_hashes,
        'sourceAssetsBefore': source_before, 'copiedAssetsBefore': initial,
        'protectedAssetsBefore': protected, 'rc1AssetsBefore': rc1_assets,
        'frozenRc1Before': frozen_rc1, 'buildBefore': baseline,
        'runner': ref(runner), 'runnerSha256': runner_sha,
        'scope': 'Real saved source export and exact-case readonly preview; no Import, Verify, UI, runtime or packaging claim',
    })

    def request_receipts():
        return files_under(folder / 'Saved/NodeForge/Requests')

    def guards():
        return {
            'runnerUnchanged': sha(runner) == runner_sha,
            'sourceVerificationSummaryUnchanged': sha(source_summary) == source_summary_sha,
            'buildUnchanged': baseline == build_snapshot(plugin),
            'protectedAssetsUnchanged': protected == protected_assets(),
            'frozenRc1Unchanged': frozen_rc1 == files_under(ROOT / 'Reports/20261009CandidateRC1/PluginStage/NodeForge'),
            'rc1AssetsUnchanged': rc1_assets == files_under(ROOT / 'ValidationProject/RegressionRuns/CandidateRC1/Content', ASSET_SUFFIXES),
            'sourceAssetsUnchanged': source_before == files_under(source, ASSET_SUFFIXES),
            'copiedAssetsUnchanged': initial == files_under(folder / 'Content', ASSET_SUFFIXES),
            'noRequestReservations': not request_receipts(),
        }

    def execute(name, mode, request, previous=None):
        before_step = guards()
        if not all(before_step.values()):
            raise SafetyFailure('Protection invariant changed before native step: ' +
                                ', '.join(key for key, passed in before_step.items() if not passed))
        inp, report = out / (name + 'Input.json'), out / (name + '.json')
        write_new(inp, request)
        command = [str(ENGINE), str(project), '-run=NodeForge', '-NFMode=' + mode,
                   '-NFInput=' + str(inp), '-NFReport=' + str(report),
                   '-unattended', '-d3d11', '-RenderOffscreen', '-AllowCommandletRendering',
                   '-nop4', '-nosound', '-NoLiveCoding', '-abslog=' + str(out / (name + '.log')), '-stdout']
        if previous:
            command.append('-NFPrevious=' + str(previous))
        write_new(out / (name + 'Command.json'), command)
        with (out / (name + 'Console.txt')).open('xb') as stream:
            process = subprocess.Popen(command, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT,
                                       creationflags=subprocess.CREATE_NO_WINDOW)
            started = {'step': name, 'pid': process.pid, 'command': command,
                       'project': ref(project), 'report': ref(report)}
            write_new(out / (name + 'Started.json'), started)
            start = time.monotonic()
            while process.poll() is None and time.monotonic() - start < args.timeout_seconds:
                time.sleep(.2)
            if process.poll() is None:
                write_new(out / 'LiveProcess.json', {**started, 'state': 'liveAtObservationTimeout',
                                                    'nextAction': 'Inspect this PID/command and terminal report; do not restart this run'})
                raise NativeStillRunning('Native child remains live at timeout: ' + name)
        data = read_json(report) if report.is_file() else {}
        step = {'step': name, 'pid': process.pid, 'exitCode': process.returncode,
                'input': ref(inp), 'inputSha256': sha(inp), 'report': ref(report),
                'reportSha256': sha(report) if report.is_file() else None}
        steps.append(step)
        write_new(out / (name + 'Terminal.json'), step)
        return data, process.returncode, inp, report, process.pid

    def readonly_errors(data, code, mode, request, process_id, expected_success):
        errors = []
        if data.get('mode') != mode or data.get('pid') != process_id:
            errors.append('Actual mode/PID differs from the launched step')
        if data.get('phase') != 'finished':
            errors.append('Native operation has no terminal phase')
        if Path(data.get('project', '')).resolve() != project.resolve():
            errors.append('Native project differs from the isolated project')
        fingerprint = data.get('readOnlyExecutionFingerprint', {})
        if fingerprint.get('binarySha256') != binary:
            errors.append('Actual loaded DLL SHA256 differs')
        if Path(fingerprint.get('binary', '')).resolve() != binary_path.resolve():
            errors.append('Actual loaded DLL path differs')
        if expected_success:
            if code != 0 or data.get('overallStatus') != 'succeeded' or data.get('operationStatus') != 'passed':
                errors.append('Expected successful native readonly operation')
        elif code == 0 or data.get('overallStatus') != 'rejected' or data.get('failedStage') != 'validation':
            errors.append('Expected explicit validation rejection')
        if data.get('requestOutcome') != 'notBuilt':
            errors.append('Readonly step claimed an asset build')
        for check in NO_EXECUTION_CHECKS:
            if data.get('checks', {}).get(check, {}).get('status') != 'notRun':
                errors.append('Readonly step claimed execution: ' + check)
        residue = data.get('residue', {})
        if (residue.get('saveAttempted') is not False or residue.get('targetFilePresent') is not False
                or residue.get('diskState') != 'notWritten'):
            errors.append('Readonly step reports save/disk side effects')
        target = request['assetPath']
        if not target.startswith('/Game/NodeForgeTests/'):
            errors.append('Input target left the bounded test namespace')
        else:
            file = folder / 'Content' / (target.removeprefix('/Game/') + '.uasset')
            if file.exists():
                errors.append('Preview/export created a target asset')
        if (folder / 'Saved/NodeForge/Requests' / (request['requestId'] + '.json')).exists():
            errors.append('Preview/export reserved the new request')
        return errors

    def finish_case(name, errors, report, extra=None):
        protection = guards()
        errors.extend('Protection failed: ' + key for key, passed in protection.items() if not passed)
        row = {'case': name, 'passed': not errors, 'errors': errors,
               'report': ref(report), 'protection': protection, **(extra or {})}
        cases.append(row)
        write_new(out / (name + 'Assertions.json'), row)
        print(name, 'passed' if not errors else 'failed', errors, flush=True)
        if not all(protection.values()):
            raise SafetyFailure('Protection invariant failed; no further native steps will start')

    def export(kind):
        source_name = 'NFExternalGraph' + kind + '2'
        source_object = '/Game/NodeForgeTests/' + source_name + '.' + source_name
        config = {'source': source_object, 'requestId': args.run + kind + 'Baseline',
                  'assetPath': '/Game/NodeForgeTests/NF' + args.run + kind + 'Baseline'}
        name = kind + 'Export'
        data, code, _, report, process_id = execute(name, kind + 'GraphExport', config)
        errors = readonly_errors(data, code, kind + 'GraphExport', config, process_id, True)
        evidence = data.get('exportEvidence', {})
        source_file = folder / 'Content/NodeForgeTests' / (source_name + '.uasset')
        source_hash = sha(source_file)
        if (evidence.get('source') != source_object or evidence.get('sourceAssetSha256') != source_hash
                or evidence.get('sourceBytesUnchanged') is not True or evidence.get('sourceRemainsClean') is not True):
            errors.append('Actual export source identity/bytes/clean evidence differs')
        request = data.get('exportedRequest')
        if not isinstance(request, dict) or request.get('assetType') != kind or request.get('version') != 2:
            errors.append('Current native exported v2 request is missing')
        finish_case(name, errors, report, {'sourceFile': ref(source_file), 'sourceSha256': source_hash})
        if errors:
            raise SafetyFailure('Real source export failed; no synthetic baseline substituted')
        return request, report, source_object, source_hash

    def fresh(request, name):
        result = copy.deepcopy(request)
        result['requestId'] = args.run + name
        result['assetPath'] = '/Game/NodeForgeTests/NF' + args.run + name
        return result

    def preview(name, request, export_report, source_object, source_hash, expected, rejection_text=None,
                order_baseline=None):
        order_only = None
        if order_baseline is not None:
            order_only = order_projection(request) == order_projection(order_baseline)
            if not order_only:
                raise SafetyFailure('Reordered input also changes graph structure/values: ' + name)
        data, code, inp, report, process_id = execute(name, 'GraphPreview', request, export_report)
        errors = readonly_errors(data, code, 'GraphPreview', request, process_id, expected is not None)
        # Early ReadRequest validation failures intentionally have no parsed
        # inputDigest; the exact command and persisted input SHA remain recorded.
        if (expected is not None or 'inputDigest' in data) and data.get('inputDigest') != sha(inp):
            errors.append('Native input digest differs from the exact persisted input')
        if expected is not None:
            observed = data.get('graphDifference', {})
            if observed.get('format') != 'NodeForgeGraphDifference' or observed.get('version') != 1:
                errors.append('Missing actual versioned graph difference')
            for key in DIFFERENCE_FIELDS:
                if not isinstance(observed.get(key), list) or unordered(observed[key]) != unordered(expected[key]):
                    errors.append('Exact difference content differs: ' + key)
            for key in ('requestStructureOrValuesChanged', 'layoutChanged'):
                if observed.get(key) is not expected[key]:
                    errors.append('Difference flag differs: ' + key)
            evidence = data.get('sourceEvidence', {})
            if (evidence.get('source') != source_object or evidence.get('sourceAssetSha256') != source_hash
                    or evidence.get('sourceBytesUnchanged') is not True or evidence.get('sourceRemainsClean') is not True):
                errors.append('Preview did not prove the real saved source remains clean and byte-identical')
            if Path(data.get('previousExportReport', '')).resolve() != export_report.resolve():
                errors.append('Preview did not use this current-DLL actual export report')
        else:
            if 'graphDifference' in data:
                errors.append('Rejected malformed identity still produced a graph difference')
            if rejection_text and rejection_text not in data.get('nextAction', ''):
                errors.append('Rejection reason differs from the expected exact-endpoint validation')
        finish_case(name, errors, report, {'input': ref(inp), 'expectedDifference': expected,
                                         'rejectionReasonContains': rejection_text,
                                         'inputOnlyChangesOrderAndNewRequestIdentity': order_only,
                                         'comparisonScope': 'Exact JSON delta against real saved source; no claim of native semantic equivalence'})

    result, error, exit_code = 'failed', None, 1
    try:
        blueprint, blueprint_report, bp_source, bp_hash = export('Blueprint')
        same = fresh(blueprint, 'SameShuffled')
        same['nodes'].reverse()
        same['edges'].reverse()
        same['settings'] = dict(reversed(list(same['settings'].items())))
        preview('BlueprintSameShuffled', same, blueprint_report, bp_source, bp_hash, empty_difference(),
                order_baseline=blueprint)

        candidates = [node for node in blueprint['nodes']
                      if node.get('properties', {}).get('member') == 'PrintString'
                      and isinstance(node.get('defaults', {}).get('InString'), str)
                      and any('A' <= char <= 'Z' or 'a' <= char <= 'z' for char in node['defaults']['InString'])
                      and any(edge['from'] == node['id'] or edge['to'] == node['id'] for edge in blueprint['edges'])]
        if not candidates:
            raise SafetyFailure('Actual source export has no suitable connected PrintString with a string default')
        original = candidates[0]
        old_id, old_text = original['id'], original['defaults']['InString']
        new_text = ''.join(char.swapcase() if char.isascii() and char.isalpha() else char for char in old_text)
        changed = fresh(blueprint, 'DefaultCase')
        changed_node = next(node for node in changed['nodes'] if node['id'] == old_id)
        changed_node['defaults']['InString'] = new_text
        expected = empty_difference()
        expected['nodesChanged'] = [{'id': old_id, 'changes': [{'field': 'defaults',
            'beforePresent': True, 'afterPresent': True,
            'before': copy.deepcopy(original['defaults']), 'after': copy.deepcopy(changed_node['defaults'])}]}]
        expected['requestStructureOrValuesChanged'] = True
        preview('BlueprintDefaultCase', changed, blueprint_report, bp_source, bp_hash, expected)

        new_id = old_id[0].swapcase() + old_id[1:]
        if new_id == old_id or any(node['id'] == new_id for node in blueprint['nodes']):
            raise SafetyFailure('Actual exported logical ID cannot be safely case-renamed')
        renamed = fresh(blueprint, 'LogicalIdCase')
        renamed_node = next(node for node in renamed['nodes'] if node['id'] == old_id)
        renamed_node['id'] = new_id
        removed_edges, added_edges = [], []
        for edge in renamed['edges']:
            if edge['from'] == old_id or edge['to'] == old_id:
                removed_edges.append(copy.deepcopy(edge))
                for key in ('from', 'to'):
                    if edge[key] == old_id:
                        edge[key] = new_id
                added_edges.append(copy.deepcopy(edge))
        expected = empty_difference()
        expected.update({'nodesAdded': [copy.deepcopy(renamed_node)], 'nodesRemoved': [copy.deepcopy(original)],
                         'edgesAdded': added_edges, 'edgesRemoved': removed_edges,
                         'requestStructureOrValuesChanged': True})
        preview('BlueprintLogicalIdCase', renamed, blueprint_report, bp_source, bp_hash, expected)

        invalid = fresh(blueprint, 'StaleEndpointCase')
        next(node for node in invalid['nodes'] if node['id'] == old_id)['id'] = new_id
        preview('BlueprintStaleEndpointCase', invalid, blueprint_report, bp_source, bp_hash, None,
                'Invalid edge endpoint')

        material, material_report, mat_source, mat_hash = export('Material')
        same_material = fresh(material, 'MaterialSame')
        same_material['nodes'].reverse()
        same_material['edges'].reverse()
        preview('MaterialSameShuffled', same_material, material_report, mat_source, mat_hash, empty_difference(),
                order_baseline=material)
        final_protection = guards()
        if all(case['passed'] for case in cases) and all(final_protection.values()) and len(cases) == 7:
            result, exit_code = 'passed', 0
    except NativeStillRunning as exc:
        result, error, exit_code = 'running', str(exc), 2
    except Exception as exc:
        error = type(exc).__name__ + ': ' + str(exc)
    summary = {'result': result, 'error': error, 'project': ref(project), 'selectedPlugin': ref(plugin),
               'dllSha256': binary, 'cases': cases, 'steps': steps,
               'protectionAtObservation': guards(), 'runnerSha256': runner_sha,
               'sourceEvidenceSha256': source_summary_sha,
               'scope': 'Actual saved-source export and readonly exact-case delta integration only; no Import, independent asset Verify, UI clicks, runtime, visual or packaging proof'}
    if result == 'running':
        summary['nextAction'] = 'Read LiveProcess.json and verify its PID/terminal report; this is an observation timeout, not a finished native process. Never restart this run.'
        write_new(out / 'ObservationCheckpoint.json', summary)
    else:
        write_new(out / 'Summary.json', summary)
    print(result, error or '', flush=True)
    return exit_code


if __name__ == '__main__':
    raise SystemExit(main())
