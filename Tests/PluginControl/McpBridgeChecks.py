"""Finite P1 adapter checks. Fake CLI results never count as UE/Codex business evidence."""
from __future__ import annotations
import argparse
import asyncio
import copy
import json
import subprocess
import sys
from pathlib import Path
from unittest.mock import patch
from mcp import StdioServerParameters
from mcp.client import Client

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'Scripts'))
from NodeForgeMcpBridge import Bridge, BridgeError, safe_read, sha, encoded, write_new, json_pointer

def read(path): return json.loads(path.read_text('utf-8-sig'))
def save(path, data): write_new(path, data)

def seed_documents():
    folder = ROOT / 'Docs/Versions/Build122/Mcp1'
    folder.mkdir(parents=True, exist_ok=True)
    page = ROOT / 'Reports/PluginControlStageTwo/McpDocs1/P1/FixturePage.md'
    if not page.exists():
        page.parent.mkdir(parents=True, exist_ok=True)
        page.write_text('# P1仅适配测试\n\n中文UTF-8。没有UE业务证据。\n', encoding='utf-8')
    target = folder / 'Manifest.json'
    if not target.exists():
        baseline = read(ROOT / 'Reports/PluginControlStageTwo/McpDocs1/P0/Baseline.json')
        pages = []
        for page_id, path in [('McpProtocol', ROOT / 'Contracts/McpProtocol.md'), ('AdapterFixture', page)]:
            pages.append({'pageId': page_id, 'path': path.relative_to(ROOT).as_posix(),
                          'sha256': sha(path.read_bytes()), 'bytes': path.stat().st_size})
        cli_paths = ['Scripts/NodeForgeControl.py', 'Scripts/NodeForgeEditorTransport.py', 'Scripts/RunProcessGuard.py']
        manifest = {'bundleId': 'NodeForge-Build122-Mcp1', 'bridgeVersion': '0.1.0',
            'project': baseline['project'], 'stage': baseline['stage'],
            'reportNamespace': 'Reports/PluginControlStageTwo/McpDocs1',
            'binarySha256': baseline['binarySha256'], 'editorApiVersion': '1.0', 'profile': 'nodeforge.editor.v1',
            'publicationStatus': 'candidatePartialP1', 'fullUsagePackageComplete': False,
            'cliFiles': {p: sha((ROOT / p).read_bytes()) for p in cli_paths}, 'pages': pages}
        save(target, manifest)
        save(ROOT / 'Reports/PluginControlStageTwo/McpDocs1/P1/SeedManifest.json', manifest)
    return target

def config(out, report_root):
    manifest = seed_documents()
    baseline = read(ROOT / 'Reports/PluginControlStageTwo/McpDocs1/P0/Baseline.json')
    value = {'format': 'NodeForgeMcpConfig', 'version': 1, 'bundleId': 'NodeForge-Build122-Mcp1',
        'project': (ROOT / baseline['project']).as_posix(), 'pluginDirectory': (ROOT / baseline['stage']).as_posix(),
        'binarySha256': baseline['binarySha256'], 'sessionPublic': (report_root / 'SessionPublic.json').as_posix(),
        'reportRoot': report_root.as_posix(), 'bridgeRunRoot': (out / 'Bridge').as_posix(),
        'toolContractSha256': sha((ROOT / 'Tools/NodeForgeMcp/ToolContract.json').read_bytes()),
        'docsManifestSha256': sha(manifest.read_bytes())}
    path = out / 'Config.json'; save(path, value)
    return path

class Fixture:
    def __init__(self, out, config_factory=None, public_identity=None):
        self.out = out; self.report_root = out / 'Session'
        self.report_root.mkdir(parents=True)
        self.public = copy.deepcopy(public_identity) if public_identity is not None else read(ROOT / 'Reports/PluginControlStageTwo/RulesCatalog1/Run2/SessionPublic.json')
        self.public['reportRoot'] = self.report_root.as_posix()
        save(self.report_root / 'SessionPublic.json', self.public)
        self.bridge = Bridge((config_factory or config)(out, self.report_root)); self.invocations = []; self.overrides = {}
        self.mock = patch.object(self.bridge, '_cli', side_effect=self.cli); self.mock.start()
        self.handle = self.bridge.call('nodeforge_connect', {})['handle']

    def cli(self, action, operation=None, arguments=None, call_id=None, revision=None):
        self.invocations.append({'action': action, 'operation': operation, 'arguments': arguments, 'callId': call_id})
        if action == 'editor-describe':
            description = copy.deepcopy(self.bridge.operations); description['identity'] = self.public
            return {'cliExitCode': 0, 'executionState': 'finished', 'response': {'result': description}}
        if operation == 'calls.get':
            original = arguments['callId']; code, state = self.overrides.get(original, (0, 'completed'))
        else: original = call_id; code, state = self.overrides.get(call_id, (0, 'completed'))
        return {'cliExitCode': code, 'executionState': 'pending' if state == 'running' else 'unknown' if state == 'unknown'
            else 'rejected' if code == 2 else 'failed' if code == 4 else 'finished',
            'operation': operation, 'callId': original, 'controlSessionId': self.public['controlSessionId'],
            'response': {'result': {'callId': original, 'controlSessionId': self.public['controlSessionId'],
                                  'operation': operation, 'state': state, 'cliExitCode': code, 'result': {}}}}

    def gt(self, call_id, operation='graphs.generate', revision='0', arguments=None):
        return self.bridge.call('nodeforge_' + operation.replace('.', '_'),
            {'handle': self.handle, 'callId': call_id, 'expectedDraftRevision': revision, 'arguments': arguments or {}})

    def close(self): self.mock.stop(); self.bridge.close()

def adapt_historical_report(fixture, case, call_id):
    original = ROOT / 'Reports/PluginControlStageTwo/RulesCatalog1/Run2'
    summary = read(original / case / 'Summary.json')
    call = summary['response']['result']; payload = call['result']
    call['callId'] = call_id; summary['callId'] = call_id
    native = read(Path(payload['nativeReport']['path']))
    destination = fixture.report_root / (call_id + 'Native.json')
    save(destination, native)
    reference = payload['nativeReport']; reference.update(path=destination.as_posix(), sha256=sha(destination.read_bytes()), bytes=destination.stat().st_size)
    sidecar = read(Path(payload['controlEvidencePath'])); sidecar['callId'] = call_id
    for payload_path, payload_digest, sidecar_path, sidecar_digest in [
        ('querySnapshot', 'queryDigest', 'querySnapshot', 'queryDigest'),
        ('inputSnapshot', 'inputDigest', 'snapshotPath', 'snapshotDigest')]:
        if payload_path in payload:
            data = Path(payload[payload_path]).read_bytes()
            destination_input = fixture.report_root / (call_id + 'Snapshot.json')
            destination_input.write_bytes(data)
            payload[payload_path] = destination_input.as_posix(); payload[payload_digest] = sha(data)
            sidecar[sidecar_path] = destination_input.as_posix(); sidecar[sidecar_digest] = sha(data)
    if 'nativeReport' in sidecar: sidecar['nativeReport'] = copy.deepcopy(reference)
    sidecar_path = fixture.report_root / (call_id + 'Control.json'); save(sidecar_path, sidecar)
    payload['controlEvidencePath'] = sidecar_path.as_posix(); payload['controlEvidenceSha256'] = sha(sidecar_path.read_bytes())
    return summary

async def stdio_checks(out):
    conf = config(out / 'Stdio', out / 'NoLiveSession')
    async with Client(StdioServerParameters(command=sys.executable,
        args=[str(ROOT / 'Scripts/NodeForgeMcp.py'), '--config', str(conf)], cwd=ROOT), read_timeout_seconds=20) as client:
        inventory = await client.list_tools()
        assert len(inventory.tools) == 15
        assert all(item.name not in ('nodeforge_session_hello', 'nodeforge_control_disable') for item in inventory.tools)
        page = await client.call_tool('nodeforge_docs_read', {'pageId': 'AdapterFixture'})
        assert not page.is_error and '中文UTF-8' in page.structured_content['text']
        denied = await client.call_tool('nodeforge_docs_read', {'pageId': '../../Session.json'})
        assert denied.is_error and denied.structured_content['status'] == 'rejected'
        save(out / 'SdkStdio.json', {'protocolVersion': client.protocol_version,
            'toolNames': [item.name for item in inventory.tools], 'unicodeDoc': page.model_dump(mode='json'),
            'unknownDoc': denied.model_dump(mode='json'), 'scope': 'Actual MCP bridge/SDK stdio; no UE or Codex business'})

def main():
    parser = argparse.ArgumentParser(); parser.add_argument('--out', required=True)
    parser.add_argument('--group', choices=['All', 'Receipts'], default='All'); args = parser.parse_args()
    out = Path(args.out).resolve()
    assert out.is_relative_to(ROOT / 'Reports/PluginControlStageTwo/McpDocs1/P1') and not out.exists()
    out.mkdir(parents=True)
    rows = []
    def check(name, passed, details=None):
        rows.append({'case': name, 'passed': bool(passed), 'details': details})
        save(out / 'Checks' / ('Case' + str(len(rows)) + '.json'), rows[-1])
        if not passed: raise AssertionError(name)
    fixture = Fixture(out / 'Adapter')
    try:
        bridge = fixture.bridge; before = len(fixture.invocations)
        if args.group == 'All':
            for name, arguments in [
                ('nodeforge_session_hello', {}), ('nodeforge_control_disable', {}), ('anything', {}),
                ('nodeforge_graphs_generate', {'handle': fixture.handle, 'arguments': {}}),
                ('nodeforge_graphs_generate', {'handle': fixture.handle, 'arguments': {}, 'callId': 'Extra', 'expectedDraftRevision': '0', 'method': 'eval'}),
                ('nodeforge_rules_query', {'handle': fixture.handle, 'arguments': {'assetKind': 'Other', 'query': ''}, 'callId': 'Wrong', 'expectedDraftRevision': '0'})]:
                check('Schema:' + name + ':' + str(len(rows)), bridge.call(name, arguments)['status'] == 'rejected')
            check('SchemaRejectionNeverCallsCli', len(fixture.invocations) == before)
            check('Uint64RevisionBound', fixture.gt('Overflow', revision='18446744073709551616')['status'] == 'rejected')
            check('GraphUtf8ByteBound', fixture.gt('UnicodeLarge', 'draft.replace', arguments={'text': '字' * 22000})['status'] == 'rejected')
            fixture.overrides['Stale'] = (2, 'rejected')
            check('NativeStalePreservedNoRevisionRetry', fixture.gt('Stale')['cliExitCode'] == 2)
            bridge.business_lock.acquire()
            try: check('BridgeBusyNoQueueOrAutoRevision', fixture.gt('Busy')['status'] == 'rejected')
            finally: bridge.business_lock.release()
            fixture.overrides['Original'] = (5, 'unknown')
            check('UnknownNativeStatePreserved', fixture.gt('Original')['status'] == 'unknown')
            before = len(fixture.invocations)
            check('UnknownStopsNextWrite', fixture.gt('AfterUnknown')['status'] == 'unknown' and len(fixture.invocations) == before)
            fixture.overrides['Original'] = (0, 'completed')
            queried = bridge.call('nodeforge_calls_get', {'handle': fixture.handle, 'arguments': {'callId': 'Original'}})
            check('OriginalCallQueryCanResolveUnknown', queried['cliExitCode'] == 0 and not bridge.unknown)
            before = len(fixture.invocations); repeated = fixture.gt('Original')
            check('SameCallIdOnlyQueriesNeverResubmits', repeated['cliExitCode'] == 0 and len(fixture.invocations) == before + 1
                  and fixture.invocations[-1]['operation'] == 'calls.get')
            check('DifferentSameCallParamsRefused', fixture.gt('Original', revision='1')['status'] == 'rejected')
        catalog = adapt_historical_report(fixture, 'ClientCatalog', 'CatalogFixture')
        projection = bridge._project_result(catalog)
        check('ActualCatalogReceiptProjection', projection['status'] == 'finished' and len(projection['reports']) == 1)
        report_id = projection['reports'][0]['reportId']
        selected = bridge.call('nodeforge_report_read', {'handle': fixture.handle, 'reportId': report_id, 'pointer': '/catalog/material/nodes/0'})
        check('SelectedRealNodeData', selected['status'] == 'passed' and 'class' in selected['value'])
        large = bridge.call('nodeforge_report_read', {'handle': fixture.handle, 'reportId': report_id, 'pointer': ''})
        check('LargeReportExplicitNavigationNoTruncation', large['status'] == 'resultTooLarge' and large['selectionReturned'] is False)
        check('NoInventedCatalogOverallStatus', 'overallStatus' not in read(Path(projection['reports'][0]['path'])))
        generation = adapt_historical_report(fixture, 'ClientMaterialGenerate', 'GenerationFixture')
        generated_projection = bridge._project_result(generation)
        check('GraphSpecificSidecarPreservedWaitingPersistence', generated_projection['status'] == 'finished'
            and generated_projection['reports'][0]['overallStatus'] == 'waitingPersistence')
        save(out / 'HistoricalAdapterEvidence.json', {'catalog': projection, 'generation': generated_projection,
            'scope': 'Copied historical receipts for adapter comparison, not new assets/UE/Codex business'})
        try: safe_read(ROOT / 'Reports/PluginControlSessions/not-a-session/Session.json', 65536)
        except BridgeError as error: check('CredentialFileDeniedBeforeRead', error.code == 'NFMCP_FILE_SCOPE')
        else: check('CredentialFileDeniedBeforeRead', False)
        try: safe_read(Path('C:/Windows/win.ini'), 65536)
        except BridgeError as error: check('OutsideProjectDeniedBeforeRead', error.code == 'NFMCP_FILE_SCOPE')
        else: check('OutsideProjectDeniedBeforeRead', False)
        target = out / 'Inside'; target.mkdir(); (target / 'File.json').write_bytes(b'{}')
        junction = out / 'Junction'
        linked = subprocess.run(['cmd.exe', '/d', '/c', 'mklink', '/J', str(junction), str(target)],
            capture_output=True, creationflags=subprocess.CREATE_NO_WINDOW)
        check('OwnedJunctionFixtureCreated', linked.returncode == 0)
        try: safe_read(junction / 'File.json', 65536)
        except Exception as error: check('ReparseReadDenied', getattr(error, 'code', '') == 'NFEDITOR_REPARSE')
        else: check('ReparseReadDenied', False)
        path = Path(projection['reports'][0]['path']); path.write_bytes(path.read_bytes() + b' ')
        changed = bridge.call('nodeforge_report_read', {'handle': fixture.handle, 'reportId': report_id, 'pointer': ''})
        check('ReportTamperStopsDependentWrite', changed['status'] == 'unknown' and fixture.gt('AfterTamper')['status'] == 'unknown')
        public = copy.deepcopy(fixture.public); public['controlSessionId'] = 'a' * 32
        (fixture.report_root / 'SessionPublic.json').write_bytes(encoded(public))
        state = bridge.call('nodeforge_editor_getState', {'handle': fixture.handle, 'arguments': {}})
        check('ChangedSessionInvalidatesHandleAndReports', state['status'] == 'environmentBlocked' and bridge.handle is None and not bridge.reports)
    finally: fixture.close()
    asyncio.run(stdio_checks(out))
    check('ActualSdkStdioBridge', True, 'SdkStdio.json')
    save(out / 'Review.json', {'stepId': 'P1', 'status': 'passed', 'canAdvance': True, 'checks': rows,
        'scope': 'Adapter/SDK only; fake CLI and copied historical evidence explicitly separated',
        'ue': 'notRun', 'actualCodexBusiness': 'notRun', 'legacyCliAndCppUnchanged': True})
    print(json.dumps({'stepId': 'P1', 'status': 'passed', 'checks': len(rows), 'ue': 'notRun'}))

if __name__ == '__main__': main()
