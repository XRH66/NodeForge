"""Only document refresh admission; real bounded files, no SDK/CLI/UE execution."""
import copy
import json
import sys
import threading
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'Scripts'))
import NodeForgeMcpBridge as module


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def main():
    out = ROOT / 'Reports/PluginControlStageTwo/McpDocs1/P1/DocsRefresh1/Checks1'
    out.mkdir()
    package = read(ROOT / 'Docs/Versions/Build122/Mcp1/Manifest.json')
    config = read(ROOT / 'Reports/PluginControlStageTwo/McpDocs1/Client/Config.json')
    tools = read(ROOT / 'Tools/NodeForgeMcp/ToolContract.json')['tools']
    checks = []

    def check(name, condition):
        row = {'case': name, 'passed': bool(condition)}
        checks.append(row)
        (out / ('Case' + str(len(checks)) + '.json')).write_text(
            json.dumps(row) + '\n', encoding='utf-8')
        assert condition, name

    fake_root = out / 'Repository'
    docs_root = fake_root / 'Docs/Versions/Build122/Mcp1'
    docs_root.mkdir(parents=True)
    page = docs_root / 'Readme.md'
    page.write_text('# UTF-8 资料第一版\n', encoding='utf-8')
    manifest = copy.deepcopy(package)
    manifest['pages'] = [{'pageId': 'Readme', 'kind': 'guide',
        'path': 'Docs/Versions/Build122/Mcp1/Readme.md',
        'sha256': module.sha(page.read_bytes()), 'bytes': len(page.read_bytes())}]
    manifest_path = docs_root / 'Manifest.json'
    config_path = out / 'Config.json'

    def publish(value, fields=None):
        manifest_path.write_bytes(module.encoded(value) + b'\n')
        conf = copy.deepcopy(config)
        conf['docsManifestSha256'] = module.sha(manifest_path.read_bytes())
        conf.update(fields or {})
        config_path.write_bytes(module.encoded(conf) + b'\n')
        return conf

    initial_config = publish(manifest)
    bridge = module.Bridge.__new__(module.Bridge)
    bridge.config_path, bridge.manifest_path, bridge.docs_root = config_path, manifest_path, docs_root
    bridge.config, bridge.manifest = initial_config, copy.deepcopy(manifest)
    bridge.metadata_lock = threading.RLock()
    bridge.tools = {row['name']: row for row in tools}
    bridge.bundle_id = config['bundleId']
    bridge.handle = '1' * 32
    bridge.identity = {'controlSessionId': '2' * 32}
    bridge.reports = {'OriginalReport': {'original': True}}
    bridge.calls = {'OriginalUnknown': {'state': 'unknown'}}
    bridge.unknown = {'OriginalUnknown'}
    bridge.pending_cli = ['OriginalPending']
    lease = out / 'Bridge.lock'
    lease.write_text('{"instance":"original-owner","pid":1}\n', encoding='utf-8')
    original_state = (bridge.handle, copy.deepcopy(bridge.identity), copy.deepcopy(bridge.reports),
        copy.deepcopy(bridge.calls), set(bridge.unknown), list(bridge.pending_cli), lease.read_bytes())

    with patch.object(module, 'ROOT', fake_root):
        result = bridge.call('nodeforge_docs_read', {'pageId': 'Readme'})
        check('InitialPinnedDocumentReadable', result['status'] == 'passed')

        page.write_text('# UTF-8 资料第二版\n', encoding='utf-8')
        updated = copy.deepcopy(manifest)
        updated['pages'][0].update(sha256=module.sha(page.read_bytes()), bytes=len(page.read_bytes()))
        updated['frozenAt'] = 'Document-only publication'
        updated_config = publish(updated)
        index = bridge.call('nodeforge_docs_index', {})
        result = bridge.call('nodeforge_docs_read', {'pageId': 'Readme'})
        check('SameConnectionLoadsOperatorApprovedNewDocumentSha',
            index['status'] == result['status'] == 'passed'
            and index['docsManifestSha256'] == result['docsManifestSha256'] == updated_config['docsManifestSha256']
            and result['sha256'] == module.sha(page.read_bytes()) and '第二版' in result['text'])

        with patch.object(bridge, '_public', return_value=bridge.identity):
            check('BoundBusinessIdentitySurvivesDocsOnlyRefresh', bridge._bound(bridge.handle) == bridge.identity)
        check('UnknownPendingReportsCallsAndOwnerLeaseUntouched', original_state ==
            (bridge.handle, bridge.identity, bridge.reports, bridge.calls, bridge.unknown,
             bridge.pending_cli, lease.read_bytes()))

        publish(updated, {'sessionPublic': config['sessionPublic'] + 'Other'})
        rejected = bridge.call('nodeforge_docs_index', {})
        check('SessionConfigChangeRequiresReconnectAndKeepsSnapshot',
            rejected['status'] == 'environmentBlocked'
            and rejected['errors'][0]['code'] == 'NFMCP_CONFIG_RELOAD_REQUIRED'
            and bridge.config == updated_config and bridge.manifest == updated)

        changed = copy.deepcopy(updated)
        changed['binarySha256'] = '0' * 64
        publish(changed)
        rejected = bridge.call('nodeforge_docs_index', {})
        check('BusinessVersionChangeRequiresReconnect', rejected['errors'][0]['code'] == 'NFMCP_CONFIG_RELOAD_REQUIRED')

        changed = copy.deepcopy(updated)
        changed['bindings'][0]['sha256'] = '0' * 64
        publish(changed)
        rejected = bridge.call('nodeforge_docs_index', {})
        check('ExecutableOrSchemaBindingCannotRefreshAsDocs', rejected['errors'][0]['code'] == 'NFMCP_CONFIG_RELOAD_REQUIRED')

        changed = copy.deepcopy(updated)
        changed['pages'][0]['path'] = '../../Session.json'
        publish(changed)
        rejected = bridge.call('nodeforge_docs_index', {})
        check('NewPageOutsideVersionRootRejected', rejected['status'] == 'environmentBlocked'
            and rejected['errors'][0]['code'] == 'NFMCP_DOCUMENT_UNAVAILABLE')

        publish(updated)
        manifest_path.write_bytes(manifest_path.read_bytes() + b' ')
        rejected = bridge.call('nodeforge_docs_index', {})
        check('UnapprovedManifestTamperStillRejected', rejected['errors'][0]['code'] == 'NFMCP_FILE_SHA')

        publish(updated)
        old = copy.deepcopy(updated)
        old['frozenAt'] = 'Next approved update'
        publish(old)
        real_read = module.safe_read
        changed_during_read = [False]

        def race(path, *args, **kwargs):
            data = real_read(path, *args, **kwargs)
            if Path(path) == manifest_path and not changed_during_read[0]:
                changed_during_read[0] = True
                config_path.write_bytes(config_path.read_bytes() + b' ')
            return data

        with patch.object(module, 'safe_read', side_effect=race):
            rejected = bridge.call('nodeforge_docs_index', {})
        check('PublishRaceRejectedWithoutCommittingSnapshot', rejected['status'] == 'environmentBlocked'
            and bridge.config == updated_config and bridge.manifest == updated)

        publish(old)
        with ThreadPoolExecutor(max_workers=2) as pool:
            values = list(pool.map(lambda _: bridge.call('nodeforge_docs_read', {'pageId': 'Readme'}), range(2)))
        check('ConcurrentReadersAdoptOneCompleteFingerprint', all(value['status'] == 'passed'
            and value['docsManifestSha256'] == bridge.config['docsManifestSha256'] for value in values))

    summary = {'stepId': 'DocsRefresh1', 'status': 'passed', 'checks': checks,
        'scope': 'Real bounded-file adapter reads with isolated repository; mocked public identity only',
        'sdk': 'notRun', 'cli': 'notRun', 'ue': 'notRun', 'assetWrites': False,
        'existingEvidenceReused': 'P1 startup/state/schema plus P3 original Codex/UE business remain separately recorded'}
    (out / 'Review.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'status': 'passed', 'checks': len(checks), 'ue': 'notRun'}))


if __name__ == '__main__':
    main()
