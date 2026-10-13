"""Finite A-stage queries and two assets in one registered reusable project."""
from __future__ import annotations
import copy
import json
import time
from pathlib import Path
from types import SimpleNamespace
from RunGraphRegression import (ENGINE, build_snapshot, files_under, ref, sha, write_new,
    REQUIRED_IMPORT_CHECKS, readback_errors)
from RunProcessGuard import run_guarded, observed_module, process_identity
from RunCases import run_client


def read(path):
    return json.loads(Path(path).read_text('utf-8-sig'))


def wait_json(path, seconds=15):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        try:
            return read(path)
        except (OSError, json.JSONDecodeError):
            time.sleep(.1)
    raise RuntimeError('Receipt deadline: ' + str(path) + '; preserve original run')


def allocation_checks():
    from RunEditorChecks import allocate_control_batch
    ledger = {'version': 1, 'activeRun': None, 'nextBatch': 7}
    first = allocate_control_batch(ledger)
    assert first['batch'] == 'B000007' and ledger['nextBatch'] == 7
    assert len({x['requestId'].casefold() for x in first['identities']}) == 2
    assert all(len(x['requestId']) <= 32 and len(x['assetPath'].split('/')[-1]) <= 32
        for x in first['identities'])
    assert len({x['assetPath'].casefold() for x in first['identities']}) == 2
    assert first['identities'][0]['assetPath'].split('/')[-1] == first['identities'][1]['assetPath'].split('/')[-1]
    for bad in ({'version': 1, 'activeRun': {'state': 'unknown'}, 'nextBatch': 7},
            {'version': 1, 'activeRun': None, 'nextBatch': 0},
            {'version': 1, 'activeRun': None, 'nextBatch': 1000000}):
        try:
            allocate_control_batch(bad)
        except ValueError:
            continue
        raise AssertionError('Unknown/exhausted allocation accepted')
    return {'operationStatus': 'passed', 'allocation': first,
        'scope': 'Pure identity allocation, unresolved/exhausted refusal; no UE/project creation'}


def foundation_checks(out):
    """Small filesystem fixtures. No real engine project is started or adopted."""
    from unittest.mock import patch
    import RunEditorChecks as runner
    fixture = out / 'Foundation'
    fixture.mkdir()
    stage = fixture / 'Reports/Build/PluginStage/NodeForge'
    (stage / 'Binaries/Win64').mkdir(parents=True)
    (stage / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll').write_bytes(b'fixture-only')
    (stage / 'NodeForge.uplugin').write_text('{}', encoding='utf-8')
    def validated(project, plugin, root):
        descriptor = read(project)
        assert descriptor['AdditionalPluginDirectories'] == [str(plugin.parent)]
        assert descriptor['Plugins'] == [{'Name': 'NodeForge', 'Enabled': True}]
        return project, plugin, plugin / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'
    rows = []
    with patch.object(runner, 'ROOT', fixture), patch.object(runner, 'validate_isolated_project', validated), \
            patch('NodeForgeControl.assert_exclusive') as exclusive:
        for name in ('A' * 21, 'Node Forge', '中文', 'e\u0301'):
            try:
                runner.ReusableEditorProject(name, stage, fixture / 'Reports/PluginControlStageTwo/Invalid')
            except ValueError:
                rows.append({'case': 'InvalidName:' + name, 'passed': True})
            else:
                raise AssertionError('Invalid project name reached mutation')
        first = runner.ReusableEditorProject('NFCLab', stage, fixture / 'Reports/PluginControlStageTwo/First')
        assert first.allocation['batch'] == 'B000001'
        (first.folder / 'Content').mkdir()
        asset = first.folder / 'Content/Keep.txt'; asset.write_text('protected fixture', encoding='utf-8')
        receipt_folder = first.folder / 'Saved/NodeForge/Requests'; receipt_folder.mkdir()
        receipt = receipt_folder / 'Keep.json'; receipt.write_text('{"fixture":true}', encoding='utf-8')
        first.finish('fixture terminal proof')
        second = runner.ReusableEditorProject('NFCLab', stage, fixture / 'Reports/PluginControlStageTwo/Second')
        assert second.project == first.project and second.allocation['batch'] == 'B000002'
        assert sha(asset) == first.ledger['assets'][ref(asset)] and sha(receipt) == first.ledger['requests'][ref(receipt)]
        second.finish('fixture terminal proof')
        rows.append({'case': 'RegisteredReuseRestoresCounterProtectsAssetsAndReceipts', 'passed': True})
        second.ledger['activeRun'] = {'state': 'unresolved', 'batch': second.allocation['batch'],
            'identities': second.allocation['identities'], 'runDirectory': second.out.as_posix()}
        second.save()
        write_new(second.lease_path, {'runDirectory': second.out.as_posix(), 'runnerPid': 4242})
        write_new(second.out / 'OwnershipDiagnosis.json', {'businessSubmissions': 0,
            'operationStatus': 'confirmedFixtureError', 'pid': 4242})
        write_new(second.out / 'Identity.json', {'pid': 4242, 'exitRequest': 'normalOwnedEditorAfterChannelStop'})
        write_new(second.out / 'StopReceipt.json', {'state': 'stopped', 'descriptorDeleted': True,
            'workers': [{'threadDone': True, 'pendingIo': False}]})
        write_new(second.out / 'Preparation.json', {'assetsBefore': files_under(second.folder / 'Content'),
            'requestsBefore': files_under(second.folder / 'Saved/NodeForge/Requests')})
        with patch('RunProcessGuard.process_identity', return_value={'state': 'absent'}):
            recovered = runner.ReusableEditorProject('NFCLab', stage,
                fixture / 'Reports/PluginControlStageTwo/Recovered', second.out)
        assert recovered.allocation == second.allocation
        recovered.finish('fixture recovery terminal proof'); second = recovered
        rows.append({'case': 'OnlyProvenUnsubmittedTerminalRunResumesOriginalBatchAndIdentities', 'passed': True})
        before = second.ledger_path.read_bytes(); descriptor_before = second.project.read_bytes()
        exclusive.side_effect = ValueError('fixture project busy')
        try:
            runner.ReusableEditorProject('NFCLab', stage, fixture / 'Reports/PluginControlStageTwo/Busy')
        except ValueError:
            assert second.ledger_path.read_bytes() == before and second.project.read_bytes() == descriptor_before
            rows.append({'case': 'BusyRejectedBeforeMutation', 'passed': True})
        else:
            raise AssertionError('Busy project admitted')
        exclusive.side_effect = None
        second.ledger['activeRun'] = {'state': 'unknown'}; second.save()
        unknown_before = second.ledger_path.read_bytes()
        try:
            runner.ReusableEditorProject('NFCLab', stage, fixture / 'Reports/PluginControlStageTwo/Unknown')
        except ValueError:
            assert second.ledger_path.read_bytes() == unknown_before
            rows.append({'case': 'UnknownRunCannotAllocateOrClearReceipt', 'passed': True})
        else:
            raise AssertionError('Unresolved run admitted')
        unregistered = fixture / 'ValidationProject/RegressionRuns/NFCUnregistered'; unregistered.mkdir()
        try:
            runner.ReusableEditorProject('NFCUnregistered', stage, fixture / 'Reports/PluginControlStageTwo/Adopt')
        except ValueError:
            assert not (unregistered / 'NFCUnregistered.uproject').exists()
            rows.append({'case': 'ExistingUnregisteredFolderNotAdopted', 'passed': True})
        else:
            raise AssertionError('Existing folder adopted')
    proof = {'operationStatus': 'passed', 'checks': rows,
        'scope': 'Tiny report-local filesystem fixtures; process/stage checks stubbed, real equivalents required by production guard'}
    write_new(out / 'FoundationChecks.json', proof)
    return proof


def run_stage(args):
    from RunEditorChecks import ROOT, ReusableEditorProject
    from NodeForgeControl import assert_exclusive, engine_processes, process_uses_project, digest
    from NodeForgeEditorTransport import Session
    stage = (ROOT / args.plugin_directory).resolve()
    out = (ROOT / args.report_directory).resolve()
    # Verify frozen protected evidence before even initializing a new reusable project.
    s5 = ROOT / 'Reports/PluginControlStageTwo/EditorControl1/S5'
    protected = read(s5 / 'Protection.json')['protectedFiles']
    for path, expected in read(s5 / 'StageManifest.json')['files'].items():
        if path in protected and protected[path] != expected:
            raise ValueError('Conflicting frozen protection maps')
        protected[path] = expected
    for extra in args.asset_protection:
        values = read((ROOT / extra).resolve()); values = values.get('protectionBefore', values)
        for path, expected in values.items():
            target = (ROOT / path).resolve()
            if not target.is_relative_to(ROOT) or not isinstance(expected, str) or len(expected) != 64:
                raise ValueError('Invalid additional protection map')
            if path in protected and protected[path] != expected:
                raise ValueError('Conflicting additional protection map')
            protected[path] = expected
    if any(not (ROOT / path).is_file() or sha(ROOT / path) != expected for path, expected in protected.items()):
        raise ValueError('Frozen source evidence/stage/assets changed; stop before project initialization')
    allocation_proof = allocation_checks()
    static_path = ROOT / 'Reports/PluginControlStageTwo/RulesCatalog1/P1Static3/StaticReview.json'
    static = read(static_path)
    if (static.get('operationStatus') != 'passed' or static.get('runnerSha256') != sha(ROOT / 'Tests/PluginControl/RunEditorChecks.py')
            or static.get('queryCasesSha256') != sha(ROOT / 'Tests/PluginControl/EditorQueryCases.py')):
        raise ValueError('Matching foundation/static review required before UE; no repeated blind fixture batch')
    recovery = (ROOT / args.resume_unsubmitted_run).resolve() if args.resume_unsubmitted_run else None
    project_run = ReusableEditorProject(args.project_name, stage, out, recovery)
    project, folder, binary = project_run.project, project_run.folder, project_run.binary
    write_new(out / 'AllocationChecks.json', allocation_proof)
    write_new(out / 'StaticEvidenceReuse.json', {'status': 'reused', 'report': ref(static_path), 'sha256': sha(static_path)})
    frozen = build_snapshot(stage)
    scripts = {ref(path): sha(path) for path in (ROOT / 'Tests/PluginControl/RunEditorChecks.py',
        ROOT / 'Tests/PluginControl/EditorQueryCases.py', ROOT / 'Scripts/NodeForgeControl.py',
        ROOT / 'Scripts/NodeForgeEditorTransport.py', ROOT / 'Scripts/RunProcessGuard.py',
        ROOT / 'Scripts/RunGraphRegression.py', ROOT / 'Tests/PluginControl/RunCases.py')}
    before = files_under(folder / 'Content'); assets = dict(before); allowed = set()
    before_requests = files_under(folder / 'Saved/NodeForge/Requests')
    allowed_receipts = {ref(folder / 'Saved/NodeForge/Requests' / (x['requestId'] + '.json'))
        for x in project_run.allocation['identities']}
    write_new(out / 'Preparation.json', {'phase': 'RulesCatalog', 'project': ref(project),
        'stage': ref(stage), 'moduleSha256': sha(binary), 'sourceAndBinary': frozen,
        'clientAndRunnerSources': scripts, 'protectionBefore': protected,
        'assetsBefore': before, 'requestsBefore': before_requests,
        'allocation': project_run.allocation, 'spaceBudgetBytes': 256 * 1024 * 1024})
    rows = []; generated = []; queried = {}; identity = None
    revision = '0'; dispatched = False; finished = False; signaled = False

    def require(case, passed, evidence=None):
        proof = {'case': case, 'passed': bool(passed), 'evidence': evidence}
        rows.append(proof); write_new(out / (case + 'Proof.json'), proof)
        if not passed:
            raise RuntimeError('A-stage gate failed: ' + case + '; stop dependent writes')

    def guard():
        changed = [path for path, expected in {**protected, **scripts}.items()
            if not (ROOT / path).is_file() or sha(ROOT / path) != expected]
        actual = files_under(folder / 'Content')
        requests = files_under(folder / 'Saved/NodeForge/Requests')
        if (changed or build_snapshot(stage) != frozen
                or any(actual.get(path) != value for path, value in assets.items())
                or set(actual) - set(assets) - allowed
                or any(requests.get(path) != value for path, value in before_requests.items())
                or set(requests) - set(before_requests) - allowed_receipts):
            raise RuntimeError('Protected bytes/content/request set changed; preserve original run: ' + repr(changed))

    def client(case, action='editor-call', extra=(), expected=0, timeout=180):
        guard()
        command = [action, '--session-file', identity['sessionFile'], '--project', str(project),
            '--plugin-directory', str(stage), '--run-directory', str(out / ('Client' + case)),
            '--timeout-seconds', str(timeout), *extra]
        result, value = run_client(command, out / (case + 'ClientConsole.txt'), timeout)
        write_new(out / (case + 'ClientResult.json'), value)
        if result.returncode == 5:
            project_run.checkpoint('unknown', callId=case, clientEvidence=ref(out / (case + 'ClientResult.json')))
            if action == 'editor-call':
                status, observed = run_client(['editor-status', '--session-file', identity['sessionFile'],
                    '--project', str(project), '--plugin-directory', str(stage),
                    '--run-directory', str(out / (case + 'UnknownStatus')), '--call-id', case,
                    '--timeout-seconds', '15'], out / (case + 'UnknownStatusConsole.txt'), 15)
                write_new(out / (case + 'UnknownStatusResult.json'), observed)
            raise RuntimeError('Unknown original call; no alternate identity or subsequent writes')
        require(case + 'Exit', result.returncode == expected == value.get('cliExitCode'), value.get('executionState'))
        guard(); return value

    def call(case, method, arguments=None, expected=0, selected_revision=None):
        extra = ['--operation', method, '--call-id', case]
        if method not in ('editor.getState', 'draft.get'):
            extra += ['--expected-revision', revision if selected_revision is None else selected_revision]
        if arguments is not None:
            path = out / (case + 'Arguments.json'); write_new(path, arguments)
            extra += ['--arguments-file', str(path)]
        return client(case, extra=extra, expected=expected)

    def state(case):
        return call(case, 'editor.getState')['response']['result']

    def query(case, method, arguments=None, expected=0):
        initial = call(case + 'DraftBefore', 'draft.get')['response']['result']
        value = call(case, method, arguments, expected)
        record = value['response']['result']; result = record['result']
        final = call(case + 'DraftAfter', 'draft.get')['response']['result']
        require(case + 'DraftUnchanged', initial == final and record['state'] == ('completed' if expected == 0 else 'rejected'))
        reference = result['nativeReport']; native_path = Path(reference['path']); native = read(native_path)
        side_path = Path(result['controlEvidencePath']); side = read(side_path)
        require(case + 'NativeBinding', native_path.is_relative_to(out) and sha(native_path) == reference['sha256']
            and native_path.stat().st_size == reference['bytes'] <= 1048576 and native['pid'] == identity['pid']
            and Path(native['project']).resolve() == project and native['sessionId'] == reference['nativeSessionId']
            and native['mode'] == ('GraphRules' if method == 'rules.query' else 'GraphCatalog')
            and 'inputMethod' not in native and 'uiInput' not in native
            and side_path.is_relative_to(out) and sha(side_path) == result['controlEvidenceSha256']
            and side['controlSessionId'] == identity['controlSessionId'] and side['callId'] == case
            and side['operation'] == method and str(side['pid']) == str(identity['pid'])
            and side['processStart'] == identity['processStart'] and Path(side['project']).resolve() == project
            and Path(side['module']).resolve() == binary and side['binarySha256'] == sha(binary)
            and side['nativeReport'] == reference and side['inputMethod'] == 'editorProgram'
            and read(side['querySnapshot']) == (arguments or {}) and sha(Path(side['querySnapshot'])) == result['queryDigest'])
        status = client(case + 'Status', 'editor-status', ['--call-id', case], expected)
        require(case + 'OriginalCallStable', status['response']['result'] == record)
        if expected == 0:
            require(case + 'SuccessProjection', native['operationStatus'] == 'passed'
                and result['catalogDigest'] == side['catalogDigest'] == native['catalogDigest']
                and ((native.get('overallStatus') == 'succeeded' and native['reportVersion'] == 'NodeForgeTaskRules1')
                    if method == 'rules.query' else ('overallStatus' not in native and native['reportVersion'] == 'NodeForgeActualCatalog2')))
        else:
            require(case + 'ClosedRejection', native['overallStatus'] == 'rejected' and bool(native['nextAction']))
        return native, native_path

    def batch(process):
        nonlocal revision
        require('OwnedLiveIdentity', identity['pid'] == process.pid
            and process_identity(process.pid).get('createdTicks') == identity['processStart']
            and observed_module(process.pid, binary.name).resolve() == binary)
        description = client('Describe', 'editor-describe')['response']['result']
        definitions = {x['name']: x for x in description['operations']}
        expected_methods = {'operations.describe', 'session.hello', 'editor.getState', 'draft.get', 'draft.loadFile',
            'draft.replace', 'draft.restore', 'graphs.preflight', 'graphs.generate', 'calls.get', 'control.disable',
            'rules.query', 'catalog.get'}
        require('ExactMethodDiscovery', set(definitions) == expected_methods and len(description['operations']) == 13)
        require('NewMethodSchemas', all(definitions[x]['dispatch'] == 'gameThread'
            and definitions[x]['expectedDraftRevisionRequired'] for x in ('rules.query', 'catalog.get'))
            and set(definitions['rules.query']['argumentsSchema']['required']) == {'assetKind', 'query'}
            and definitions['catalog.get']['argumentsSchema']['properties'] == {})
        # Exercise new server schema directly through the existing authenticated transport;
        # the official CLI rejects these before submission, so that alone would be insufficient.
        session_args = SimpleNamespace(session_file=identity['sessionFile'], project=str(project),
            plugin_directory=str(stage), timeout_seconds=20)
        session = Session(session_args, ROOT, digest, engine_processes, process_uses_project)
        try:
            for case, method, arguments in (('MissingQuery', 'rules.query', {'assetKind': 'Material'}),
                    ('WrongQueryType', 'rules.query', {'assetKind': 'Material', 'query': 1}),
                    ('CatalogExtra', 'catalog.get', {'unexpected': 'value'})):
                response = session.request(method, arguments, case, revision)
                write_new(out / (case + 'ServerResponse.json'), response)
                require(case + 'ServerRejected', response.get('error', {}).get('code') == -32602)
        finally:
            session.close()
        catalog, catalog_path = query('Catalog', 'catalog.get')
        queried['Catalog'] = catalog_path
        mat, mat_path = query('MaterialRules', 'rules.query', {'assetKind': 'Material', 'query': 'VectorParameter ScalarParameter'})
        bp, bp_path = query('BlueprintRules', 'rules.query', {'assetKind': 'Blueprint', 'query': 'ReceiveBeginPlay Concat_StrStr PrintString'})
        full, _ = query('EmptyRules', 'rules.query', {'assetKind': 'Material', 'query': ''})
        require('FullDigestAndFamily', all(x['catalogDigest'] == catalog['catalogDigest'] for x in (mat, bp, full))
            and full['taskRules']['capabilities'] == catalog['catalog']['material'])
        for kind, native in (('Material', mat), ('Blueprint', bp)):
            section = catalog['catalog'][kind.lower()]
            chosen = native['taskRules']['capabilities']['nodes']
            fixture_path = ROOT / 'Examples/GraphV2Samples' / ('StaticColor.json' if kind == 'Material' else 'BlueprintConcat.json')
            fixture = read(fixture_path)
            require(kind + 'FixedIdentities', all(any(item['class'] == node['class']
                and all(item.get('properties', {}).get(key) == value for key, value in node.get('properties', {}).items()
                    if key in ('owner', 'member')) for item in chosen) for node in fixture['nodes'])
                and all(item in section['nodes'] for item in chosen))
            allocation = next(x for x in project_run.allocation['identities'] if x['case'] == kind)
            request = copy.deepcopy(fixture); request.update(requestId=allocation['requestId'],
                assetPath=allocation['assetPath'], catalogDigest=catalog['catalogDigest'])
            write_new(out / (kind + 'Input.json'), request)
            write_new(out / (kind + 'Expected.json'), {'sourceFixture': ref(fixture_path), 'sourceSha256': sha(fixture_path),
                'identityMapping': allocation, 'queryReport': ref(mat_path if kind == 'Material' else bp_path),
                'catalogReport': ref(catalog_path), 'expectedGraph': request})
        flat = read(out / 'MaterialInput.json'); flat.update(requestId='NFA' + project_run.allocation['batch'][1:] + 'FlatReq',
            assetPath='/Game/NodeForgeTests/FlatSample01'); write_new(out / 'FlatInput.json', flat)
        loaded = call('InvalidDraft', 'draft.replace', {'text': '{invalid'})
        revision = loaded['response']['result']['result']['draft']['draftRevision']
        query('InvalidDraftCatalog', 'catalog.get')
        call('StaleQuery', 'catalog.get', expected=2, selected_revision='0')
        for case, config in (('InvalidKind', {'assetKind': 'material', 'query': ''}),
                ('LongQuery', {'assetKind': 'Material', 'query': 'x' * 257}),
                ('NineTerms', {'assetKind': 'Material', 'query': 'a b c d e f g h i'}),
                ('NoMatch', {'assetKind': 'Material', 'query': 'NFDefinitelyMissingIdentity'})):
            query(case, 'rules.query', config, 2)
        write_new(out / 'QueryBoundaryReady.json', {'controlSessionId': identity['controlSessionId'],
            'GraphRules': str(mat_path), 'GraphCatalog': str(catalog_path), 'CopyFlat': str(out / 'FlatInput.json'),
            'CopySubdirectory': str(out / 'MaterialInput.json')})
        boundary = wait_json(out / 'QueryBoundaryDone.json')
        require('NativeQueryPathCopyBoundaries', boundary['operationStatus'] == 'passed'
            and all(x['passed'] for x in boundary['checks']))
        write_new(out / 'P1Review.json', {'stepId': 'P1', 'result': 'passed', 'canAdvance': True,
            'planSha256': sha(ROOT / 'Docs/PluginControl/ThirdStagePlan.md'), 'NF': ['NF056'], 'AC': ['AC040'],
            'AT': ['AT01', 'AT02', 'AT03-initial', 'AT04'], 'pid': process.pid,
            'processStart': identity['processStart'], 'moduleSha256': sha(binary), 'project': ref(project),
            'evidence': [ref(out / 'DescribeClientResult.json'), ref(out / 'QueryBoundaryDone.json')],
            'deferredToP2': 'Actual generated lastSuccess preservation and two UI clicks; no extra assets'})
        project_run.checkpoint('P1Passed', pid=process.pid, processStart=identity['processStart'])
        # Same edited parser, original flat input compatibility; no extra asset created.
        loaded = call('FlatLoad', 'draft.loadFile', {'path': str(out / 'FlatInput.json')})
        revision = loaded['response']['result']['result']['draft']['draftRevision']
        pref = call('FlatPreflight', 'graphs.preflight')['response']['result']['result']
        require('FlatPreflightAccepted', read(pref['nativeReport']['path'])['overallStatus'] == 'succeeded')
        for item in project_run.allocation['identities']:
            kind = item['case']; source = out / (kind + 'Input.json'); request = read(source)
            loaded = call(kind + 'Load', 'draft.loadFile', {'path': str(source)})
            revision = loaded['response']['result']['result']['draft']['draftRevision']
            pref = call(kind + 'Preflight', 'graphs.preflight')['response']['result']
            require(kind + 'RealPreflight', pref['state'] == 'completed'
                and read(pref['result']['nativeReport']['path'])['overallStatus'] == 'succeeded')
            target = folder / 'Content' / (request['assetPath'].removeprefix('/Game/') + '.uasset')
            require(kind + 'AllocatedTargetFree', not target.exists()); allowed.add(ref(target))
            project_run.checkpoint('generating', callId=kind + 'Generate', assetPath=request['assetPath'])
            value = call(kind + 'Generate', 'graphs.generate')['response']['result']; result = value['result']
            native_path = Path(result['nativeReport']['path']); native = read(native_path)
            errors = [key for key in REQUIRED_IMPORT_CHECKS if native['checks'][key]['status'] != 'passed']
            for key in ('initialReadback', 'finalReadback', 'publishedReadback'):
                errors += [key + ': ' + error for error in readback_errors(request, native.get(key))]
            require(kind + 'ExactGeneratedAsset', value['state'] == 'completed' and native['overallStatus'] == 'waitingPersistence'
                and native['pid'] == process.pid and native['requestId'] == request['requestId']
                and native['assetPath'] == request['assetPath'] and sha(source) == result['inputDigest']
                and sha(native_path) == result['nativeReport']['sha256'] and not errors
                and target.is_file() and sha(target) == native['fileSha256'], {'readbackErrors': errors})
            assets[ref(target)] = sha(target); allowed.clear(); guard()
            receipt = folder / 'Saved/NodeForge/Requests' / (request['requestId'] + '.json')
            require(kind + 'SavedRequestReceipt', receipt.is_file())
            before_requests[ref(receipt)] = sha(receipt)
            status = client(kind + 'GenerateStatus', 'editor-status', ['--call-id', kind + 'Generate'])
            require(kind + 'GenerateCallStable', status['response']['result'] == value)
            saved_state = state(kind + 'SuccessBefore')['lastSuccess']
            query(kind + 'AfterSaveQuery', 'catalog.get')
            require(kind + 'SavedSuccessPreserved', state(kind + 'SuccessAfter')['lastSuccess'] == saved_state)
            generated.append({'case': kind, 'input': ref(source), 'generationReport': ref(native_path),
                'asset': ref(target), 'sha256': sha(target), 'generationPid': process.pid,
                'requestId': request['requestId'], 'assetPath': request['assetPath']})
            write_new(out / (kind + 'SavedProtection.json'), dict(assets))
            project_run.checkpoint('saved', generated=generated)
            if kind == 'Material':
                for case, changed in (('OccupiedTarget', {'requestId': request['requestId'] + 'Other'}),
                        ('CaseFoldOccupiedTarget', {'requestId': request['requestId'] + 'Case',
                            'assetPath': request['assetPath'].rsplit('/', 1)[0] + '/sample01'}),
                        ('CrossDirectoryRequest', {'assetPath': project_run.allocation['identities'][1]['assetPath']})):
                    collision = copy.deepcopy(request); collision.update(changed)
                    path = out / (case + 'Input.json'); write_new(path, collision)
                    loaded = call(case + 'Load', 'draft.loadFile', {'path': str(path)})
                    revision = loaded['response']['result']['result']['draft']['draftRevision']
                    rejected = call(case + 'Preflight', 'graphs.preflight', expected=2)['response']['result']
                    require(case + 'RefusesWithoutAssetChange', rejected['state'] == 'rejected'
                        and read(rejected['result']['nativeReport']['path'])['overallStatus'] == 'rejected')
        for case, mode in (('QueryRules', 'GraphRules'), ('QueryCatalog', 'GraphCatalog')):
            draft = call(case + 'Draft', 'draft.get')['response']['result']
            write_new(out / (case + 'UiReady.json'), {'controlSessionId': identity['controlSessionId'],
                'draftRevision': draft['draftRevision'], 'inputDigest': draft['digest']})
            proof = wait_json(out / (case + 'UiDone.json'), 15); native_path = Path(proof['after']['nativeReport'])
            native = read(native_path)
            preserve = ('draftRevision', 'inputText', 'inputDigest', 'lastSuccessReport', 'lastSuccessAsset',
                'lastImportInput', 'lastImportDigest', 'lastExportReport')
            require(case + 'RealUiSharedQuery', proof['source'] == 'actualGui' and proof['actualDriverClickCompleted']
                and proof['actualVisibleProductionTab'] and proof['pid'] == process.pid
                and proof['controlSessionId'] == identity['controlSessionId'] and native['mode'] == mode
                and native['operationStatus'] == 'passed' and native['catalogDigest'] == catalog['catalogDigest']
                and 'inputMethod' not in native and 'uiInput' not in native
                and all(proof['before'][key] == proof['after'][key] for key in preserve))
            write_new(out / (case + 'UiChecked.json'), {'operationStatus': 'passed',
                'controlSessionId': identity['controlSessionId'], 'nativeReportSha256': sha(native_path)})
            require(case + 'CheckAccepted', wait_json(out / (case + 'UiCheckedAccepted.json'))['operationStatus'] == 'passed')
        write_new(out / 'GenerationChecks.json', {'operationStatus': 'passed', 'checks': rows, 'generated': generated})
        guard()

    def observe(process):
        nonlocal identity, dispatched, finished, signaled
        if finished and not signaled and (out / 'IdentityLoadedModule.json').is_file():
            write_new(out / 'ClientDone.json', {'operationStatus': 'passed',
                'controlSessionId': identity['controlSessionId'], 'assetPaths': [x['assetPath'] for x in generated]})
            signaled = True; return
        if dispatched or not (out / 'ClientReady.json').is_file():
            return
        try:
            ready = read(out / 'ClientReady.json')
        except (OSError, json.JSONDecodeError):
            return
        identity = ready['identity']; dispatched = True
        project_run.checkpoint('running', pid=process.pid, processStart=identity['processStart'])
        batch(process); finished = True

    guard(); assert_exclusive(project)
    write_new(out / 'Registration.json', {'project': project.as_posix(), 'pluginDirectory': stage.as_posix(),
        'binarySha256': sha(binary), 'reportDirectory': out.as_posix()})
    command = [str(ENGINE.with_name('UnrealEditor.exe')), str(project),
        '-ExecCmds=Automation RunTests NodeForge.EditorControlIdentity', '-NFEditorControlReport=' + str(out / 'Identity.json'),
        '-NFEditorControlQueryUiChecks', '-NFControlEnable', '-NFControlRegistration=' + str(out / 'Registration.json'),
        '-windowed', '-ResX=1280', '-ResY=900', '-d3d11', '-unattended', '-nop4', '-nosound', '-NoLiveCoding',
        '-abslog=' + str(out / 'Identity.log'), '-stdout', '-FullStdOutLogOutput']
    try:
        process = run_guarded(command, cwd=ROOT, console=out / 'IdentityConsole.txt', timeout=660,
            expected_module=binary, observer=observe)
        native = read(out / 'Identity.json')
        stop = read(out / 'StopReceipt.json')
        require('NormalChannelRelease', stop['state'] == 'stopped' and stop['descriptorDeleted']
            and stop['controlSessionId'] == identity['controlSessionId']
            and not Path(identity['sessionFile']).exists() and bool(stop['workers'])
            and all(x['threadDone'] and not x['pendingIo'] and x['cancelRequested'] == x['cancelCompleted'] for x in stop['workers']))
        require('OwnedEditorNormalExit', finished and signaled and process.returncode == 0
            and native['operationStatus'] == 'passed' and all(x['passed'] for x in native['checks'])
            and read(out / 'IdentityExit.json')['terminatedByRunner'] is False)
        guard(); assert_exclusive(project)
        description_run = out / 'CommandletDescribe'
        result, value = run_client(['describe', '--project', str(project), '--plugin-directory', str(stage),
            '--run-directory', str(description_run), '--timeout-seconds', '180'], out / 'CommandletDescribeConsole.txt', 180)
        require('OriginalFourCommandletMethods', result.returncode == 0
            and {x['name'] for x in read(description_run / 'Native.json')['operations']}
                == {'operations.describe', 'graphs.generateFromFile', 'graphs.preflightFromFile', 'graphs.verifySaved'})
        verification = []
        for item in generated:
            guard(); assert_exclusive(project)
            verify_run = out / (item['case'] + 'Verify')
            result, value = run_client(['execute', '--operation', 'graphs.verifySaved', '--project', str(project),
                '--plugin-directory', str(stage), '--run-directory', str(verify_run),
                '--input', str(ROOT / item['input']), '--generation-report', str(ROOT / item['generationReport']),
                '--description-file', str(description_run / 'Native.json'), '--timeout-seconds', '240'],
                out / (item['case'] + 'VerifyConsole.txt'), 240)
            native = read(verify_run / 'Native.json'); expected = read(ROOT / item['input'])
            errors = []
            for key in ('reopenedReadback', 'reopenedFinalReadback'):
                errors += [key + ': ' + error for error in readback_errors(expected, native.get(key))]
            require(item['case'] + 'IndependentVerify', result.returncode == 0 and native['overallStatus'] == 'succeeded'
                and native['pid'] != item['generationPid'] and Path(native['project']).resolve() == project
                and native['checks']['persistence']['status'] == 'passed' and not errors
                and native['inputDigest'] == value['inputDigest'] == sha(ROOT / item['input'])
                and sha(ROOT / item['asset']) == item['sha256'], {'errors': errors})
            verification.append({'case': item['case'], 'pid': native['pid'], 'report': ref(verify_run / 'Native.json')})
        guard(); assert_exclusive(project)
        write_new(out / 'SavedAssetProtection.json', assets)
        write_new(out / 'P2Review.json', {'stepId': 'P2', 'result': 'passed', 'canAdvance': True,
            'planSha256': sha(ROOT / 'Docs/PluginControl/ThirdStagePlan.md'), 'NF': ['NF056'], 'AC': ['AC040'],
            'AT': ['AT03-final', 'AT05', 'AT06', 'AT07'], 'project': ref(project), 'stage': ref(stage),
            'generationPid': process.pid, 'generated': generated, 'independentVerify': verification,
            'validation': {'generationAndSave': 'passed', 'newPidVerify': 'passed',
                'runtime': 'notRun', 'materialVisual': 'notRun', 'packaging': 'notRun'}})
        project_run.finish(ref(out / 'P2Review.json'))
        write_new(out / 'Summary.json', {'operationStatus': 'passed', 'phase': 'RulesCatalog',
            'project': ref(project), 'stage': ref(stage), 'checks': rows, 'generated': generated, 'verify': verification})
        print(json.dumps({'operationStatus': 'passed', 'report': ref(out / 'Summary.json')}, ensure_ascii=False))
        return 0
    except BaseException as error:
        project_run.checkpoint('unresolved', errorType=type(error).__name__, generated=generated,
            evidence=ref(out / 'LiveProcess.json') if (out / 'LiveProcess.json').is_file() else ref(out))
        if not (out / 'Failure.json').exists():
            write_new(out / 'Failure.json', {'operationStatus': 'failed', 'errorType': type(error).__name__,
                'reason': str(error), 'generated': generated, 'nextAction': 'Inspect original run/call/PID/partial writes; no new batch'})
        raise
