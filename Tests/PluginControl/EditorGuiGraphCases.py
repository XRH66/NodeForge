"""S4C1 formal load/preflight and actual production GUI Import, no fake RPC receipt."""
from __future__ import annotations
import json
import time
from pathlib import Path
from RunGraphRegression import sha, ref, write_new, REQUIRED_IMPORT_CHECKS, readback_errors


def run_cases(root, out, folder, identity, process, client, guard, asset_snapshot, allow_asset, freeze_asset):
    rows = []
    results = []
    revision = '0'

    def require(case, passed, evidence=None):
        row = {'case': case, 'passed': bool(passed), 'evidence': evidence or {}}
        rows.append(row)
        write_new(out / (case + 'GuiBoundaryProof.json'), row)
        if not passed:
            raise RuntimeError('GUI graph gate failed; stop subsequent writes: ' + case)

    def call(case, method, arguments=None):
        extra = ['--operation', method, '--call-id', case]
        if method != 'draft.get':
            extra += ['--expected-revision', revision]
        if arguments is not None:
            path = out / (case + 'Arguments.json')
            write_new(path, arguments)
            extra += ['--arguments-file', str(path)]
        return client(case, 'editor-call', extra)

    def wait_json(path, seconds):
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            try:
                return json.loads(path.read_text('utf-8-sig'))
            except (OSError, json.JSONDecodeError):
                time.sleep(.1)
        raise RuntimeError('Actual GUI receipt missing before deadline; preserve unknown writes')

    for case, filename in (('Blueprint', 'BlueprintConcat.json'), ('Material', 'StaticColor.json')):
        guard()
        source = root / 'Examples/GraphV2Samples' / filename
        request = json.loads(source.read_text('utf-8-sig'))
        target = (folder / 'Content' / (request['assetPath'].removeprefix('/Game/') + '.uasset')).resolve()
        require(case + 'FreshFixedTarget', target.is_relative_to(folder / 'Content') and not target.exists())
        loaded = call(case + 'GuiLoad', 'draft.loadFile', {'path': source.as_posix()})
        revision = loaded['response']['result']['result']['draft']['draftRevision']
        draft = call(case + 'GuiDraft', 'draft.get')['response']['result']
        require(case + 'ExactFormalDraft', draft['draftRevision'] == revision
            and draft['digest'] == sha(source) and draft['text'] == source.read_bytes().decode('utf-8'))
        before = asset_snapshot()
        pref = call(case + 'GuiPreflight', 'graphs.preflight')['response']['result']
        pref_result = pref['result']
        pref_path = Path(pref_result['nativeReport']['path'])
        pref_native = json.loads(pref_path.read_text('utf-8-sig'))
        side_path = Path(pref_result['controlEvidencePath'])
        side = json.loads(side_path.read_text('utf-8-sig'))
        require(case + 'FormalPreflightNoAsset', pref['state'] == 'completed'
            and pref_native['mode'] == 'GraphPreflight' and pref_native['overallStatus'] == 'succeeded'
            and pref_native['pid'] == process.pid and pref_native['inputMethod'] == 'editorProgram'
            and 'uiInput' not in pref_native and asset_snapshot() == before
            and sha(pref_path) == pref_result['nativeReport']['sha256']
            and sha(Path(pref_result['inputSnapshot'])) == sha(source) == pref_result['inputDigest']
            and sha(side_path) == pref_result['controlEvidenceSha256']
            and side['callId'] == case + 'GuiPreflight' and side['draftRevision'] == revision
            and side['controlSessionId'] == identity['controlSessionId'])
        allow_asset(target)
        write_new(out / (case + 'UiGenerateReady.json'), {'controlSessionId': identity['controlSessionId'],
            'draftRevision': revision, 'inputDigest': sha(source), 'assetPath': request['assetPath']})
        proof = wait_json(out / (case + 'UiGenerated.json'), 180)
        require(case + 'ActualProductionDriverProof', proof['operationStatus'] == 'passed'
            and proof['actualDriverClickCompleted'] and proof['source'] == 'actualGui'
            and proof['pid'] == process.pid and proof['controlSessionId'] == identity['controlSessionId']
            and proof['draftRevision'] == revision and proof['inputDigest'] == sha(source)
            and proof['assetPath'] == request['assetPath'] and proof['actualVisibleProductionTab']
            and Path(proof['screenshot']).stat().st_size > 0)
        native_path = Path(proof['nativeReport'])
        native = json.loads(native_path.read_text('utf-8-sig'))
        snapshot = Path(proof['inputSnapshot'])
        ui = native['uiInput']
        require(case + 'RealUiOriginalNativeSource', native['mode'] == 'Import'
            and native['overallStatus'] == 'waitingPersistence' and native['pid'] == process.pid
            and native['executionFingerprint']['binarySha256'] == identity['binarySha256']
            and native['requestId'] == request['requestId'] and native['assetPath'] == request['assetPath']
            and native['inputMethod'] == 'draft' and ui['contentSource'] == 'currentDraft'
            and ui['origin'] == 'file' and Path(ui['sourcePath']) == source and not ui['sourceEdited']
            and ui['sourceOriginalDigest'] == sha(source) and ui['draftRevision'] == revision
            and Path(ui['snapshotPath']) == snapshot and sha(snapshot) == native['inputDigest'] == ui['snapshotDigest'] == sha(source))
        errors = [key + ': failed' for key in REQUIRED_IMPORT_CHECKS if native['checks'][key]['status'] != 'passed']
        for key in ('initialReadback', 'finalReadback', 'publishedReadback'):
            errors += [key + ': ' + error for error in readback_errors(request, native.get(key))]
        require(case + 'IndependentActualCompileSave', not errors and target.is_file()
            and sha(target) == native['fileSha256'], {'errors': errors})
        freeze_asset(target, case)
        guard()
        results.append({'case': case, 'input': ref(snapshot), 'inputDigest': sha(source),
            'nativeReport': ref(native_path), 'nativeSha256': sha(native_path), 'pid': process.pid,
            'asset': ref(target), 'assetSha256': sha(target), 'uiProof': ref(out / (case + 'UiGenerated.json')),
            'source': 'actualGui', 'controlCallId': None})
        write_new(out / (case + 'UiChecked.json'), {'operationStatus': 'passed',
            'controlSessionId': identity['controlSessionId'], 'nativeReportSha256': sha(native_path)})
        accepted = wait_json(out / (case + 'UiCheckedAccepted.json'), 10)
        require(case + 'NativeCheckAcceptedBeforeNextDraft', accepted['operationStatus'] == 'passed'
            and accepted['controlSessionId'] == identity['controlSessionId']
            and accepted['nativeReportSha256'] == sha(native_path))
    write_new(out / 'GuiGraphChecks.json', {'operationStatus': 'passed', 'checks': rows, 'pid': process.pid})
    return results
