"""S4 real production graph refusals and accepted Import disconnection cases."""
from __future__ import annotations
import json
import time
from datetime import datetime, timezone
from pathlib import Path
from types import SimpleNamespace
from RunGraphRegression import sha, ref, write_new, files_under, REQUIRED_IMPORT_CHECKS, readback_errors


def run_cases(root, out, folder, identity, process, client, guard, asset_snapshot, allow_asset, freeze_asset):
    from NodeForgeEditorTransport import Session
    from NodeForgeControl import digest, engine_processes, process_uses_project
    checks = []
    revision = '0'

    def require(case, passed, evidence=None):
        checks.append({'case': case, 'passed': bool(passed), 'evidence': evidence or {}})
        write_new(out / (case + 'BoundaryProof.json'), checks[-1])
        if not passed:
            raise RuntimeError('Asset boundary failed; stop subsequent writes: ' + case)

    def call(case, method, arguments=None, expected=0, selected_revision=None, call_id=None):
        guard()
        extra = ['--operation', method, '--call-id', call_id or case]
        if selected_revision is not None:
            extra += ['--expected-revision', selected_revision]
        if arguments is not None:
            path = out / (case + 'Arguments.json')
            write_new(path, arguments)
            extra += ['--arguments-file', str(path)]
        value = client(case, 'editor-call', extra, expected)
        guard()
        return value

    def draft(case):
        return call(case, 'draft.get')['response']['result']

    def load(case, source):
        nonlocal revision
        old = revision
        value = call(case, 'draft.loadFile', {'path': source.as_posix()}, selected_revision=revision)
        actual = draft(case + 'Read')
        revision = actual['draftRevision']
        require(case + 'ExactDraft', int(revision) == int(old) + 1 and actual['digest'] == sha(source)
            and actual['text'] == source.read_bytes().decode('utf-8'))
        return value

    def receipt(case, value, mode, source, status):
        record = value['response']['result']
        result = record['result']
        native_path = Path(result['nativeReport']['path'])
        native = json.loads(native_path.read_text('utf-8-sig'))
        sidecar_path = Path(result['controlEvidencePath'])
        sidecar = json.loads(sidecar_path.read_text('utf-8-sig'))
        snapshot = Path(result['inputSnapshot'])
        require(case + 'ExactNativeBinding', record['state'] == ('rejected' if status == 'rejected' else 'completed') and native['pid'] == process.pid
            and native['mode'] == mode and native['overallStatus'] == status
            and native['inputMethod'] == 'editorProgram' and 'uiInput' not in native
            and native['sessionId'] == result['nativeReport']['nativeSessionId']
            and sha(snapshot) == sha(source) == result['inputDigest'] == sidecar['snapshotDigest']
            and sha(native_path) == result['nativeReport']['sha256']
            and sha(sidecar_path) == result['controlEvidenceSha256']
            and sidecar['controlSessionId'] == identity['controlSessionId']
            and sidecar['callId'] == record['callId'] and sidecar['draftRevision'] == revision
            and sidecar['binarySha256'] == identity['binarySha256'])
        return record, result, native_path, native

    fixed = root / 'Examples/GraphV2Samples/DynamicPulse.json'
    original_request = json.loads(fixed.read_text('utf-8-sig'))
    first = load('BaselineLoad', fixed)
    original_draft = draft('BaselineRead')
    call('MissingFile', 'draft.loadFile', {'path': (out / 'MissingGraph.json').as_posix()},
        expected=2, selected_revision=revision)
    require('MissingFileKeepsOriginalDraftAndAssets', draft('AfterMissing') == original_draft and not asset_snapshot())
    call('UnknownOperation', 'nodeforge.invalidOperation', expected=2)
    require('UnknownOperationNotAccepted', not (out / 'Calls/UnknownOperationAccepted.json').exists())
    stale = call('StaleGraph', 'graphs.generate', expected=2, selected_revision='0')['response']['result']
    require('StaleBeforeGraphExecutionAndWrite', stale['state'] == 'rejected'
        and stale['result']['code'] == 'staleRevision' and 'nativeReport' not in stale['result']
        and not (out / 'Calls/StaleGraphNative.json').exists() and not (out / 'Calls/StaleGraphInput.json').exists()
        and draft('AfterStale') == original_draft and not asset_snapshot())
    repeat = call('RepeatLoad', 'draft.loadFile', {'path': fixed.as_posix()}, selected_revision='0', call_id='BaselineLoad')
    require('SameCallKeepsOriginalResultAndRevision', first['response']['result'] == repeat['response']['result']
        and draft('AfterRepeat') == original_draft)
    call('ConflictCall', 'draft.loadFile', {'path': (root / 'Tests/Invalid/UnknownPin.json').as_posix()},
        expected=2, selected_revision='0', call_id='BaselineLoad')
    require('ConflictCallDoesNotMutate', draft('AfterConflict') == original_draft and not asset_snapshot())

    invalid = out / 'InvalidJson.json'
    with invalid.open('x', encoding='utf-8', newline='') as stream:
        stream.write('{')
    for case, source in (('InvalidJson', invalid), ('UnknownPin', root / 'Tests/Invalid/UnknownPin.json'),
                         ('DuplicateKey', root / 'Tests/Invalid/DuplicateKey.json')):
        load(case + 'Load', source)
        for suffix, method, mode in (('Preflight', 'graphs.preflight', 'GraphPreflight'),
                                     ('Generate', 'graphs.generate', 'Import')):
            name = case + suffix
            before = asset_snapshot()
            value = call(name, method, expected=2, selected_revision=revision)
            _, _, _, native = receipt(name, value, mode, source, 'rejected')
            require(name + 'BeforeAssetWrite', native['checks']['validation']['status'] == 'failed'
                and asset_snapshot() == before and not before)

    load('DynamicLoad', fixed)
    preflight = call('DynamicPreflight', 'graphs.preflight', selected_revision=revision)
    record, result, native_path, native = receipt('DynamicPreflight', preflight, 'GraphPreflight', fixed, 'succeeded')
    require('DynamicPreflightNoAsset', native['operationStatus'] == 'passed' and not asset_snapshot())
    preflight_hash = sha(native_path)
    repeated = call('DynamicPreflightRepeat', 'graphs.preflight', selected_revision=revision, call_id='DynamicPreflight')
    require('RepeatedGraphCallDoesNotRerun', repeated['response']['result'] == record and sha(native_path) == preflight_hash)

    target = (folder / 'Content' / (original_request['assetPath'].removeprefix('/Game/') + '.uasset')).resolve()
    require('DisconnectFreshExactTarget', target.is_relative_to(folder / 'Content') and not target.exists())
    allow_asset(target)
    args = SimpleNamespace(session_file=identity['sessionFile'], project=str(folder / (folder.name + '.uproject')),
        plugin_directory=str(Path(identity['module']).parents[2]), timeout_seconds=30)
    bridge = Session(args, root, digest, engine_processes, process_uses_project)
    try:
        response = bridge.request('graphs.generate', {}, 'DisconnectImport', revision)
        before_close = datetime.now(timezone.utc).isoformat()
        finished_at_close = (out / 'Calls/DisconnectImportFinished.json').exists()
    finally:
        bridge.close()
    after_close = datetime.now(timezone.utc).isoformat()
    write_new(out / 'ActualDisconnect.json', {'response': response, 'beforeCloseUtc': before_close,
        'afterCloseUtc': after_close, 'finishedAtClose': finished_at_close, 'pid': process.pid,
        'scope': 'Production Session/OS identity and original Import; after Close only calls.get of same ID'})
    require('RealAcceptedUnfinishedDisconnect', response.get('result', {}).get('state') in ('accepted', 'queued', 'running')
        and not finished_at_close and (out / 'Calls/DisconnectImportAccepted.json').is_file())
    deadline = time.monotonic() + 180
    query_index = 0
    while True:
        if time.monotonic() >= deadline:
            raise RuntimeError('Original disconnected Import still unknown; no subsequent write')
        value = client('DisconnectStatus' + str(query_index), 'editor-status', ['--call-id', 'DisconnectImport'], (0, 5), call_timeout=10)
        guard()
        record = value['response']['result']
        if record.get('state') == 'completed':
            break
        require('DisconnectPending' + str(query_index), record.get('state') in ('accepted', 'queued', 'running'))
        query_index += 1
        time.sleep(.2)
    record, result, native_path, native = receipt('DisconnectedImport', value, 'Import', fixed, 'waitingPersistence')
    errors = [key + ': failed' for key in REQUIRED_IMPORT_CHECKS if native['checks'][key]['status'] != 'passed']
    for key in ('initialReadback', 'finalReadback', 'publishedReadback'):
        errors += [key + ': ' + error for error in readback_errors(original_request, native.get(key))]
    require('DisconnectedOriginalStructureCompileSave', not errors and target.is_file() and sha(target) == native['fileSha256'], {'errors': errors})
    freeze_asset(target)
    write_new(out / 'DisconnectedSavedProtection.json', {ref(target): sha(target)})
    receipts_dir = folder / 'Saved/NodeForge/Requests'
    request_receipts = files_under(receipts_dir)
    require('OneOriginalRequestReceipt', len(request_receipts) == 1)
    journal_sha = {name: sha(out / ('Calls/DisconnectImport' + name + '.json')) for name in ('Accepted', 'Finished')}
    original_native_sha = sha(native_path)
    query = client('DisconnectFinalQuery', 'editor-status', ['--call-id', 'DisconnectImport'])
    require('OnlyQueryKeepsSameOriginalCall', query['response']['result'] == record
        and all(sha(out / ('Calls/DisconnectImport' + name + '.json')) == h for name, h in journal_sha.items())
        and sha(native_path) == original_native_sha)

    for case, overrides, kind in (('BothConflict', {}, 'targetAndRequestExist'),
        ('TargetConflict', {'requestId': 'BoundaryUnusedRequest'}, 'targetExists'),
        ('RequestConflict', {'assetPath': '/Game/NodeForgeTests/NFBoundaryUnusedTarget'}, 'requestExists')):
        source = out / (case + 'Input.json')
        changed = dict(original_request, **overrides)
        write_new(source, changed)
        load(case + 'Load', source)
        before = asset_snapshot()
        rejected = call(case, 'graphs.generate', expected=2, selected_revision=revision)
        _, _, _, refused_native = receipt(case, rejected, 'Import', source, 'rejected')
        state = call(case + 'State', 'editor.getState')['response']['result']
        require(case + 'SpecificRefusalNoOverwrite', refused_native['conflict']['kind'] == kind
            and asset_snapshot() == before and files_under(receipts_dir) == request_receipts
            and sha(native_path) == original_native_sha and state['lastSuccess']['assetPath'] == original_request['assetPath']
            and not state['assetPath'])
    guard()
    write_new(out / 'AssetBoundaryChecks.json', {'operationStatus': 'passed', 'checks': checks, 'pid': process.pid})
    return [{'case': 'Disconnect', 'input': ref(Path(result['inputSnapshot'])), 'inputDigest': sha(fixed),
        'nativeReport': ref(native_path), 'nativeSha256': original_native_sha, 'pid': process.pid,
        'asset': ref(target), 'assetSha256': sha(target)}]
