"""Finite startup/lease regression: actual SDK stdio, mocked business; never starts UE."""
import argparse
import asyncio
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import sys
import threading
from unittest.mock import patch
from mcp import StdioServerParameters
from mcp.client import Client
from McpBridgeChecks import ROOT, Fixture, config, read, save
from NodeForgeMcpBridge import Bridge, sha, safe_read


def documents(bridge):
    index = bridge.call('nodeforge_docs_index', {})
    page = bridge.call('nodeforge_docs_read', {'pageId': 'Readme'})
    return index, page


async def two_clients(out, check):
    conf = config(out / 'Sdk', out / 'SdkNoSession')
    run = Path(read(conf)['bridgeRunRoot'])
    marker = {'pid': os.getpid(), 'instance': '1' * 32}
    save(run / 'Bridge.lock', marker)
    marker_sha = sha((run / 'Bridge.lock').read_bytes())
    parameters = StdioServerParameters(command=sys.executable,
        args=['-B', str(ROOT / 'Scripts/NodeForgeMcp.py'), '--config', str(conf)], cwd=ROOT)
    clients = {}
    first_ready, second_ready = asyncio.Event(), asyncio.Event()
    release_first, release_second = asyncio.Event(), asyncio.Event()

    async def client_owner(name, ready, release):
        async with Client(parameters, read_timeout_seconds=20) as client:
            clients[name] = client
            ready.set()
            await release.wait()

    first_task = asyncio.create_task(client_owner('first', first_ready, release_first))
    second_task = asyncio.create_task(client_owner('second', second_ready, release_second))
    try:
        await asyncio.wait_for(asyncio.gather(first_ready.wait(), second_ready.wait()), timeout=20)
        first, second = clients['first'], clients['second']
        inventories = await asyncio.gather(first.list_tools(), second.list_tools())
        contract = read(ROOT / 'Tools/NodeForgeMcp/ToolContract.json')
        expected = {row['name']: row for row in contract['tools']}
        inventory_ok = all({item.name for item in inventory.tools} == set(expected)
            and all(item.input_schema == expected[item.name]['inputSchema']
                    and item.output_schema == expected[item.name]['outputSchema']
                    for item in inventory.tools) for inventory in inventories)
        indices = await asyncio.gather(first.call_tool('nodeforge_docs_index', {}),
                                       second.call_tool('nodeforge_docs_index', {}))
        pages = await asyncio.gather(first.call_tool('nodeforge_docs_read', {'pageId': 'Readme'}),
                                    second.call_tool('nodeforge_docs_read', {'pageId': 'Readme'}))
        check('TwoSdkClientsDiscoverExactToolsAndReadSameDocs', inventory_ok
            and all(not response.is_error for response in indices + pages)
            and pages[0].structured_content['sha256'] == pages[1].structured_content['sha256'],
            {'protocols': [first.protocol_version, second.protocol_version],
             'toolNames': sorted(expected), 'pageSha256': pages[0].structured_content['sha256'],
             'scope': 'Actual SDK stdio clients; not current Codex business'})
        release_first.set()
        await asyncio.wait_for(first_task, timeout=5)
        still_readable = await second.call_tool('nodeforge_docs_read', {'pageId': 'Readme'})
        denied = await second.call_tool('nodeforge_connect', {})
        check('OneSdkClientExitLeavesOtherReadableAndNoEditorConnectBlocked',
            not still_readable.is_error and denied.is_error
            and denied.structured_content['status'] == 'environmentBlocked'
            and not (run / 'Cli').exists(), {'connectResult': denied.structured_content})
    finally:
        release_first.set(); release_second.set()
        await asyncio.gather(first_task, second_task)
    check('SdkReadersPreserveExistingBusinessMarker', sha((run / 'Bridge.lock').read_bytes()) == marker_sha
        and (run / 'ProtocolSeen.json').is_file())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', required=True)
    parser.add_argument('--group', choices=['All', 'SdkAndBusiness', 'CloseRaceOnly'], default='All')
    args = parser.parse_args()
    out = Path(args.out).resolve()
    assert out.is_relative_to(ROOT / 'Reports/PluginControlStageTwo/McpDocs1/P1/StartupRepair1')
    assert not out.exists()
    assert (ROOT / 'Reports/PluginControlStageTwo/McpDocs1/P1/FixturePage.md').is_file()
    out.mkdir(parents=True)
    checks = []

    def check(name, condition, details=None):
        row = {'case': name, 'passed': bool(condition), 'details': details}
        checks.append(row)
        save(out / 'Checks' / ('Case' + str(len(checks)) + '.json'), row)
        if not condition:
            raise AssertionError(name)

    if args.group == 'CloseRaceOnly':
        owner = Fixture(out / 'CloseOwner')
        before_calls = len(owner.invocations)
        attempts = []
        try:
            with ThreadPoolExecutor(max_workers=1) as pool:
                def during_release(path, *arguments, **keywords):
                    if Path(path) == owner.bridge.lease_path:
                        # Force a GT admission exactly inside the owner's release window.
                        attempts.append(pool.submit(owner.gt, 'DuringRelease').result(timeout=5))
                    return safe_read(path, *arguments, **keywords)
                with patch('NodeForgeMcpBridge.safe_read', side_effect=during_release):
                    owner.bridge.close()
            check('OwnerReleaseAtomicallyRefusesConcurrentGtBeforeCli', len(attempts) == 1
                and attempts[0]['errors'][0]['code'] == 'NFMCP_BUSY'
                and len(owner.invocations) == before_calls
                and not (owner.bridge.run_root / 'Calls/DuringRelease.json').exists()
                and not owner.bridge.lease_path.exists() and owner.bridge.handle is None)
        finally:
            owner.close()
        save(out / 'Review.json', {'stepId': 'R2', 'status': 'passed', 'checks': checks,
            'scope': 'Only newly identified close/GT admission race; earlier checks reused',
            'ue': 'notRun', 'nativeCli': 'notRun', 'assetWrites': False, 'canAdvance': True})
        print(json.dumps({'status': 'passed', 'checks': len(checks), 'scope': 'CloseRaceOnly', 'ue': 'notRun'}))
        return

    if args.group == 'All':
        conf = config(out / 'NoEditor', out / 'NoEditorPublic')
        reader = Bridge(conf)
        try:
            index, page = documents(reader)
            denied = reader.call('nodeforge_connect', {})
            check('FreshReaderDoesNotAcquireBusinessLeaseOrStartCli',
                index['status'] == page['status'] == 'passed'
                and denied['status'] == 'environmentBlocked'
                and not reader.lease_path.exists() and not (reader.run_root / 'Cli').exists())
        finally:
            reader.close()
    else:
        prior = out.parent / 'Checks1/Checks/Case1.json'
        assert read(prior)['passed'] is True
        check('FreshReaderNoEditorEvidenceReused', True, {'source': prior.relative_to(ROOT).as_posix()})

    asyncio.run(two_clients(out, check))

    owner = Fixture(out / 'Owner')
    other = Bridge(owner.out / 'Config.json')
    try:
        assert owner.bridge.owns_lease
        lease_sha = sha(owner.bridge.lease_path.read_bytes())
        with patch.object(other, '_cli', side_effect=AssertionError('Busy connection must not submit CLI')) as cli:
            busy = other.call('nodeforge_connect', {})
        index, page = documents(other)
        check('OtherBusinessConnectionIsBusyWithoutCliButDocsWork',
            busy['status'] == 'environmentBlocked' and busy['errors'][0]['code'] == 'NFMCP_BUSY'
            and cli.call_count == 0 and index['status'] == page['status'] == 'passed')
        other.close()
        check('ReaderCloseNeverDeletesBusinessOwnerLease', sha(owner.bridge.lease_path.read_bytes()) == lease_sha)

        entered = threading.Event()
        release = threading.Event()

        def held_cli(*arguments, **keywords):
            entered.set()
            assert release.wait(5), 'Bounded connect fixture expired'
            return owner.cli(*arguments, **keywords)

        with patch.object(owner.bridge, '_cli', side_effect=held_cli) as cli:
            with ThreadPoolExecutor(max_workers=1) as pool:
                future = pool.submit(owner.bridge.call, 'nodeforge_connect', {})
                try:
                    assert entered.wait(5)
                    same_instance_busy = owner.bridge.call('nodeforge_connect', {})
                    owner.bridge.close()
                    lease_retained = sha(owner.bridge.lease_path.read_bytes()) == lease_sha
                finally:
                    release.set()
                connected = future.result(timeout=5)
            check('ConcurrentConnectAndCloseCannotReleaseActiveConnection',
                same_instance_busy['status'] == 'environmentBlocked'
                and same_instance_busy['errors'][0]['code'] == 'NFMCP_BUSY'
                and cli.call_count == 1 and lease_retained and connected['status'] == 'connected')
    finally:
        other.close()
        owner.close()
    check('NormalOwnerExitReleasesOnlyItsLease', not owner.bridge.lease_path.exists()
        and owner.bridge.handle is None)

    pending = Fixture(out / 'PendingClose')
    try:
        lease_sha = sha(pending.bridge.lease_path.read_bytes())
        pending.bridge.pending_cli.append({'fixtureOnly': True, 'noActualChild': True})
        pending.bridge.close()
        check('PendingCliStillRetainsLeaseOnClose', sha(pending.bridge.lease_path.read_bytes()) == lease_sha,
              {'scope': 'Mock pending child only; no actual CLI or UE'})
    finally:
        pending.close()

    unresolved = Fixture(out / 'Unresolved')
    unresolved.overrides['OriginalUnknown'] = (5, 'unknown')
    try:
        unknown = unresolved.gt('OriginalUnknown')
        check('UnknownCallPersistsBeforeExit', unknown['status'] == 'unknown'
            and read(unresolved.bridge.run_root / 'Calls/OriginalUnknown.json')['requiresReview'] is True)
    finally:
        unresolved.close()
    reopened = Bridge(unresolved.out / 'Config.json')
    try:
        with patch.object(reopened, '_cli', side_effect=unresolved.cli):
            connected = reopened.call('nodeforge_connect', {})
            before_calls = len(unresolved.invocations)
            blocked = reopened.call('nodeforge_graphs_generate', {'handle': connected['handle'],
                'callId': 'NewWrite', 'expectedDraftRevision': '0', 'arguments': {}})
            check('ReconnectPreservesUnknownGateAndDoesNotReplay', connected['status'] == 'connected'
                and 'OriginalUnknown' in connected['unresolvedCalls']
                and blocked['status'] == 'unknown' and blocked['errors'][0]['code'] == 'NFMCP_UNRESOLVED_CALL'
                and len(unresolved.invocations) == before_calls
                and not (reopened.run_root / 'Calls/NewWrite.json').exists())
            original = reopened.call('nodeforge_calls_get', {'handle': connected['handle'],
                                      'arguments': {'callId': 'OriginalUnknown'}})
            check('OriginalUnknownRemainsQueryableWithoutResubmission',
                original['status'] == 'unknown' and 'OriginalUnknown' in reopened.unknown
                and sum(row['operation'] == 'graphs.generate' for row in unresolved.invocations) == 1)
    finally:
        reopened.close()

    save(out / 'Review.json', {'stepId': 'R2', 'status': 'passed', 'checks': checks,
        'scope': 'One finite startup/lease regression; SDK real, business CLI mocked',
        'ue': 'notRun', 'actualCodexDocs': 'notRun', 'nativeCli': 'notRun',
        'assetWrites': False, 'canAdvance': True})
    print(json.dumps({'stepId': 'R2', 'status': 'passed', 'checks': len(checks), 'ue': 'notRun'}))


if __name__ == '__main__':
    main()
