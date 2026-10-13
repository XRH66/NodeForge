"""S4C2 fixed real GUI business actions, independently checked against original inputs."""
from __future__ import annotations
import copy
import hashlib
import json
import re
import time
from pathlib import Path
from RunGraphRegression import sha, ref, write_new, REQUIRED_IMPORT_CHECKS, readback_errors


def run_cases(root, out, folder, identity, process, client, guard, asset_snapshot,
              allow_asset, allow_relayout, freeze_asset):
    rows, results = [], []
    revision = '0'

    def require(case, passed, evidence=None):
        value = {'case': case, 'passed': bool(passed), 'evidence': evidence or {}}
        rows.append(value)
        write_new(out / (case + 'BusinessCheck.json'), value)
        if not passed:
            raise RuntimeError('Business UI gate failed; preserve evidence and stop writes: ' + case)

    def read(path):
        return json.loads(Path(path).read_text('utf-8-sig'))

    def digest(text):
        return hashlib.sha256(text.encode('utf-8')).hexdigest()

    def call(case, method, arguments=None, timeout=30):
        extra = ['--operation', method, '--call-id', case]
        if method != 'draft.get':
            extra += ['--expected-revision', revision]
        if arguments is not None:
            path = out / (case + 'Arguments.json')
            write_new(path, arguments)
            extra += ['--arguments-file', str(path)]
        return client(case, 'editor-call', extra, call_timeout=timeout)['response']['result']

    def draft(case):
        return call(case, 'draft.get')

    def wait_json(path, seconds):
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            try:
                return read(path)
            except (OSError, json.JSONDecodeError):
                if process.poll() is not None:
                    raise RuntimeError('Owned editor actually exited before this marker; preserve original native failure')
                time.sleep(.1)
        raise RuntimeError('Business marker absent at deadline; do not repeat the UI action')

    def gui(case):
        nonlocal revision
        guard()
        current = draft(case + 'BeforeDraft')
        revision = current['draftRevision']
        write_new(out / (case + 'UiReady.json'), {'controlSessionId': identity['controlSessionId'],
            'draftRevision': revision, 'inputDigest': current['digest']})
        proof = wait_json(out / (case + 'UiDone.json'), 180)
        before, after = proof['before'], proof['after']
        require(case + 'ActualProductionProof', proof['operationStatus'] == 'passed'
            and proof['source'] == 'actualGui' and proof['actualDriverClickCompleted']
            and proof['actualVisibleProductionTab'] and proof['pid'] == process.pid
            and proof['controlSessionId'] == identity['controlSessionId']
            and before['draftRevision'] == revision and before['inputDigest'] == current['digest']
            and before['inputText'] == current['text'] and Path(proof['screenshot']).stat().st_size > 0)
        observed = draft(case + 'AfterDraft')
        require(case + 'FormalDraftEqualsActualUi', observed['draftRevision'] == after['draftRevision']
            and observed['text'] == after['inputText'] and observed['digest'] == after['inputDigest'])
        revision = observed['draftRevision']
        if after['nativeReport']:
            native_path = Path(after['nativeReport']).resolve()
            require(case + 'ClosedOwnedNativeReceipt', native_path.is_relative_to(root / 'Reports')
                and native_path.is_file() and native_path.stat().st_size <= 1048576)
            native = read(native_path)
            require(case + 'ActualBackendPid', native['pid'] == process.pid)
        else:
            native = None
        guard()
        return proof, native

    def ack(case, proof):
        path = proof['after']['nativeReport']
        value = {'operationStatus': 'passed', 'controlSessionId': identity['controlSessionId'],
            'nativeReportSha256': sha(Path(path)) if path else ''}
        write_new(out / (case + 'UiChecked.json'), value)
        require(case + 'NativeAckBeforeNextMutation', wait_json(out / (case + 'UiCheckedAccepted.json'), 10) == value)

    def no_asset(case, before, proof, native=None):
        after = proof['after']
        require(case + 'NoAssetOrSourceMutation', asset_snapshot() == before and not after['assetPath'])
        if native and native['mode'] != 'GraphCatalog':
            require(case + 'NoBuildCompileSave', all(native['checks'][key]['status'] == 'notRun'
                for key in ('build', 'compile', 'save')))
        guard()

    def imported(case, native, snapshot, target, source_kind):
        request = read(snapshot)
        errors = [key + ': failed' for key in REQUIRED_IMPORT_CHECKS if native['checks'][key]['status'] != 'passed']
        for key in ('initialReadback', 'finalReadback', 'publishedReadback'):
            errors += [key + ': ' + error for error in readback_errors(request, native.get(key))]
        require(case + 'OriginalActualCompileSaveReadbacks', native['mode'] == 'Import'
            and native['overallStatus'] == 'waitingPersistence' and native['pid'] == process.pid
            and native['executionFingerprint']['binarySha256'] == identity['binarySha256']
            and native['assetPath'] == request['assetPath'] and native['requestId'] == request['requestId']
            and sha(snapshot) == native['inputDigest'] and not errors and target.is_file()
            and sha(target) == native['fileSha256'], {'errors': errors})
        freeze_asset(target, case)
        guard()
        return {'case': case, 'status': 'passed', 'input': ref(snapshot), 'inputDigest': sha(snapshot),
            'pid': process.pid, 'asset': ref(target), 'assetSha256': sha(target), 'source': source_kind,
            'nativeReport': None}

    before_assets = asset_snapshot()
    proof, native = gui('EmptyExport')
    require('EmptyExportExplicitRefusal', native is None and '构图导出未执行' in proof['after']['resultText']
        and proof['after']['inputText'] == proof['before']['inputText'] and not proof['after']['lastExportReport'])
    no_asset('EmptyExport', before_assets, proof)
    ack('EmptyExport', proof)

    proof, native = gui('Catalog')
    require('CatalogRealReadOnlyResult', native['mode'] == 'GraphCatalog' and native['operationStatus'] == 'passed'
        and 'checks' not in native and 'readOnlyExecutionFingerprint' not in native and 'catalog' in native
        and proof['after']['readOnlyLength'] > 0 and proof['after']['inputText'] == proof['before']['inputText'])
    no_asset('Catalog', before_assets, proof, native)
    ack('Catalog', proof)

    proof, native = gui('InspectNegative')
    require('InspectEmptySelectionRefusal', native is None and '只读检查未执行' in proof['after']['resultText']
        and proof['after']['readOnlyLength'] == 0 and proof['after']['inputText'] == proof['before']['inputText'])
    no_asset('InspectNegative', before_assets, proof)
    ack('InspectNegative', proof)

    source = root / 'Examples/GraphV2Samples/BlueprintConcat.json'
    original = read(source)
    target = folder / 'Content' / (original['assetPath'].removeprefix('/Game/') + '.uasset')
    require('BaselineFreshExactTarget', not target.exists())
    loaded = call('BaselineLoad', 'draft.loadFile', {'path': source.as_posix()})
    revision = loaded['result']['draft']['draftRevision']
    exact = draft('BaselineExactDraft')
    require('BaselineExactOriginalInput', exact['digest'] == sha(source) and exact['text'] == source.read_bytes().decode('utf-8'))
    pref = call('BaselinePreflight', 'graphs.preflight')['result']
    pref_native = read(pref['nativeReport']['path'])
    require('BaselinePreflightOriginalNoWrite', pref_native['overallStatus'] == 'succeeded'
        and pref_native['mode'] == 'GraphPreflight' and asset_snapshot() == before_assets)
    allow_asset(target)
    generated = call('BaselineGenerate', 'graphs.generate', timeout=180)['result']
    native_path = Path(generated['nativeReport']['path'])
    native = read(native_path)
    require('BaselineRealProgramSource', native['inputMethod'] == 'editorProgram' and 'uiInput' not in native
        and sha(native_path) == generated['nativeReport']['sha256'])
    item = imported('Baseline', native, Path(generated['inputSnapshot']), target, 'editorProgram')
    item.update(nativeReport=ref(native_path), nativeSha256=sha(native_path))
    results.append(item)

    before_assets = asset_snapshot()
    proof, native = gui('Prepare')
    prepared = json.loads(proof['after']['inputText'])
    expected_copy = copy.deepcopy(original)
    expected_copy['requestId'] = prepared['requestId']
    expected_copy['assetPath'] = original['assetPath'] + 'Copy1'
    preparation = native.get('copyPreparation', {})
    prepared_path = Path(preparation.get('preparedInputPath', ''))
    require('PrepareOnlyNewIdentityAndOneRevision', native['mode'] == 'PrepareNewCopy'
        and native['operationStatus'] == 'passed' and prepared == expected_copy
        and prepared['requestId'] != original['requestId']
        and int(proof['after']['draftRevision']) == int(proof['before']['draftRevision']) + 1
        and prepared_path.resolve().is_relative_to(root / 'Reports/PreparedCopies')
        and sha(prepared_path) == preparation['preparedInputDigest'] == proof['after']['inputDigest'])
    no_asset('Prepare', before_assets, proof, native)
    preparation_report = proof['after']['nativeReport']
    ack('Prepare', proof)

    copy_target = folder / 'Content' / (prepared['assetPath'].removeprefix('/Game/') + '.uasset')
    allow_asset(copy_target)
    proof, native = gui('CopyGenerate')
    copy_snapshot = Path(proof['after']['lastImportInput'])
    require('CopyRealUiPreparationSource', native['inputMethod'] == 'draft' and native['uiInput']['contentSource'] == 'currentDraft'
        and native['uiInput']['copyPreparationReport'] == preparation_report
        and read(copy_snapshot) == prepared and sha(copy_snapshot) == proof['before']['inputDigest']
        and proof['after']['lastSuccessReport'] == proof['after']['nativeReport'] and not proof['after']['resultStale'])
    item = imported('Copy', native, copy_snapshot, copy_target, 'actualGui')
    item.update(nativeReport=ref(Path(proof['after']['nativeReport'])),
        nativeSha256=sha(Path(proof['after']['nativeReport'])), uiProof=ref(out / 'CopyGenerateUiDone.json'))
    results.append(item)
    ack('CopyGenerate', proof)

    replaced = call('MalformedBeforeRelayout', 'draft.replace', {'text': '{'})
    revision = replaced['result']['draft']['draftRevision']
    before_assets = asset_snapshot()
    allow_relayout()
    proof, native = gui('Relayout')
    derived = Path(native['derivedInputPath']).resolve()
    derived_request = read(derived)
    relayout_target = folder / 'Content' / (derived_request['assetPath'].removeprefix('/Game/') + '.uasset')
    expected_relayout = copy.deepcopy(prepared)
    for node in expected_relayout['nodes']:
        node.pop('position', None)
    expected_relayout['requestId'] = derived_request['requestId']
    expected_relayout['assetPath'] = derived_request['assetPath']
    require('RelayoutUsesArchivedSuccessNotBadCurrentDraft', re.fullmatch('GraphLayout[0-9A-Fa-f]{32}', derived_request['requestId']) is not None
        and derived_request['assetPath'] == '/Game/NodeForgeTests/' + derived_request['requestId']
        and derived.is_relative_to(folder / 'Saved/NodeForge/RelayoutInputs') and derived_request == expected_relayout
        and native['inputMethod'] == 'relayoutCopy' and Path(native['sourceInput']) == copy_snapshot
        and native['sourceInputDigest'] == sha(copy_snapshot) and native['sourceAssetPath'] == prepared['assetPath']
        and native['uiInput']['contentSource'] == 'lastSuccessfulSnapshot'
        and native['uiInput']['sourceSnapshotDigest'] == sha(copy_snapshot)
        and proof['after']['inputText'] == proof['before']['inputText'] == '{'
        and proof['after']['draftRevision'] == proof['before']['draftRevision']
        and proof['after']['resultRelayout'] and not proof['after']['resultStale'])
    require('RelayoutExactlyOneNewOwnedAsset', set(asset_snapshot()) - set(before_assets) == {ref(relayout_target)})
    item = imported('Relayout', native, derived, relayout_target, 'actualGui')
    item.update(nativeReport=ref(Path(proof['after']['nativeReport'])),
        nativeSha256=sha(Path(proof['after']['nativeReport'])), uiProof=ref(out / 'RelayoutUiDone.json'))
    results.append(item)
    ack('Relayout', proof)

    before_assets = asset_snapshot()
    proof, native = gui('Export')
    exported = native['exportedRequest']
    evidence = native['exportEvidence']
    object_path = derived_request['assetPath'] + '.' + derived_request['assetPath'].rsplit('/', 1)[1]
    mapping = {entry['sourceComment'].removeprefix('NodeForge:'): entry['id'] for entry in evidence['logicalIdMapping']}
    remapped = copy.deepcopy(derived_request)
    prior_relayout = read(root / results[-1]['nativeReport'])
    require('ExportPriorSavedReadbackUnchanged', sha(root / results[-1]['nativeReport']) == results[-1]['nativeSha256'])
    source_positions = {node['id']: node['position'] for node in prior_relayout['publishedReadback']['nodes']}
    expected_ids = {node['id'] for node in remapped['nodes']}
    require('ExportExactNativeMappingFromOwnSource', set(mapping) == expected_ids and len(set(mapping.values())) == len(expected_ids)
        and evidence['source'] == object_path and evidence['sourceAssetSha256'] == sha(relayout_target)
        and evidence['sourceBytesUnchanged'] and evidence['sourceRemainsClean'])
    for node in remapped['nodes']:
        node['position'] = source_positions[node['id']]
        node['id'] = mapping[node['id']]
    for edge in remapped['edges']:
        edge['from'], edge['to'] = mapping[edge['from']], mapping[edge['to']]
    errors = readback_errors(remapped, evidence['sourceReadback']) + readback_errors(exported, evidence['sourceReadback'])
    require('ExportRealSourceGraphAndSharedDraft', native['mode'] == 'BlueprintGraphExport'
        and native['operationStatus'] == 'passed' and not errors
        and json.loads(proof['after']['inputText']) == exported
        and proof['after']['readOnlyDigest'] == proof['after']['inputDigest']
        and proof['after']['lastExportReport'] == proof['after']['nativeReport'], {'errors': errors})
    no_asset('Export', before_assets, proof, native)
    export_report = proof['after']['nativeReport']
    ack('Export', proof)

    proof, native = gui('Preview')
    require('PreviewRealUnchangedDelta', native['mode'] == 'GraphPreview' and native['operationStatus'] == 'passed'
        and native['previousExportReport'] == export_report
        and native['sourceEvidence']['sourceAssetSha256'] == sha(relayout_target)
        and not native['graphDifference']['requestStructureOrValuesChanged'] and not native['graphDifference']['layoutChanged']
        and proof['after']['inputDigest'] == proof['before']['inputDigest'])
    no_asset('Preview', before_assets, proof, native)
    ack('Preview', proof)

    replaced = call('MalformedBeforePreview', 'draft.replace', {'text': '{'})
    revision = replaced['result']['draft']['draftRevision']
    for case, mode in (('InvalidPreview', 'GraphPreview'), ('InvalidPrepare', 'PrepareNewCopy')):
        proof, native = gui(case)
        require(case + 'RealRejectionKeepsDraftAndSuccess', native['mode'] == mode and native['overallStatus'] == 'rejected'
            and proof['after']['inputText'] == proof['before']['inputText'] == '{'
            and proof['after']['draftRevision'] == proof['before']['draftRevision']
            and proof['after']['lastSuccessAsset'] == derived_request['assetPath']
            and proof['after']['lastExportReport'] == export_report and 'copyPreparation' not in native)
        no_asset(case, before_assets, proof, native)
        ack(case, proof)
    write_new(out / 'BusinessUiChecks.json', {'operationStatus': 'passed', 'checks': rows, 'pid': process.pid})
    return results
