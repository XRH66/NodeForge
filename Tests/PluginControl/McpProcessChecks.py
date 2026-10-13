"""Remaining P1 process/output/capacity/persistent-gate checks, no actual CLI or UE."""
import argparse
import copy
import json
import os
import subprocess
import sys
from pathlib import Path
from unittest.mock import patch
from McpBridgeChecks import ROOT, Fixture, config, read, save, adapt_historical_report
from NodeForgeMcpBridge import Bridge, BridgeError, encoded, sha

def main():
    parser = argparse.ArgumentParser(); parser.add_argument('--out', required=True); args = parser.parse_args()
    out = Path(args.out).resolve()
    assert out.is_relative_to(ROOT / 'Reports/PluginControlStageTwo/McpDocs1/P1') and not out.exists()
    out.mkdir(parents=True); checks = []
    def check(name, condition, details=None):
        row = {'case': name, 'passed': bool(condition), 'details': details}; checks.append(row)
        save(out / 'Checks' / ('Case' + str(len(checks)) + '.json'), row)
        if not condition: raise AssertionError(name)
    fixture = Fixture(out / 'Adapter'); bridge = fixture.bridge
    commands = []
    class Process:
        returncode = 0
        timeout_once = False
        bad_output = False
        pid = os.getpid()
        def __init__(self, command, **kwargs):
            commands.append(command); self.first = True
        def communicate(self, timeout):
            if self.timeout_once and self.first:
                self.first = False; raise subprocess.TimeoutExpired(commands[-1], timeout)
            return (b'{}' if self.bad_output else encoded({'cliExitCode': 0, 'executionState': 'finished'})), b''
        def poll(self): return self.returncode
        def terminate(self): raise AssertionError('Must not terminate an unknown child')
        def kill(self): raise AssertionError('Must not kill an unknown child')
    try:
        with patch('NodeForgeMcpBridge.subprocess.Popen', Process):
            reply = Bridge._cli(bridge, 'editor-call', 'draft.replace', {'text': '中文'}, 'Exact-Call', '0')
        command = commands[-1]
        check('ExplicitCliTokenBinding', reply['cliExitCode'] == 0 and command[command.index('--call-id') + 1] == 'Exact-Call'
            and command[command.index('--expected-revision') + 1] == '0' and command[command.index('--operation') + 1] == 'draft.replace')
        argument_file = Path(command[command.index('--arguments-file') + 1])
        check('CliUnicodeArgumentsPreserved', read(argument_file) == {'text': '中文'})
        check('CliNoShellOrArbitraryMethod', command[:2] == [sys.executable, str(ROOT / 'Scripts/NodeForgeControl.py')])
        Process.bad_output = True
        with patch('NodeForgeMcpBridge.subprocess.Popen', Process):
            malformed = Bridge._cli(bridge, 'editor-call', 'graphs.generate', {}, 'Malformed', '0')
        check('UnboundCliOutputRemainsUnknown', malformed['cliExitCode'] == 5 and malformed['executionState'] == 'unknown')
        Process.bad_output = False; Process.timeout_once = True
        with patch('NodeForgeMcpBridge.subprocess.Popen', Process), patch('NodeForgeMcpBridge.process_identity', return_value={'state': 'fixtureOnly', 'pid': os.getpid(), 'createdTicks': '1'}):
            expired = Bridge._cli(bridge, 'editor-call', 'graphs.generate', {}, 'Deadline', '0')
        check('DeadlinePreservesOriginalCallAndChild', expired['cliExitCode'] == 5 and expired['callId'] == 'Deadline'
            and len(bridge.pending_cli) == 1 and len(list((bridge.run_root / 'LiveCli').glob('*.json'))) == 1)
        bridge._collect_late()
        check('LateResultRecordedWithoutReplay', not bridge.pending_cli and len(list((bridge.run_root / 'LateCli').glob('*.json'))) == 1)
        Process.timeout_once = False
        catalog = adapt_historical_report(fixture, 'ClientCatalog', 'CapacityFixture')
        bridge.reports = {str(i): {'fixtureOnly': True} for i in range(256)}
        capacity = bridge._project_result(catalog)
        check('ReportCapacityRefusesWithoutEviction', capacity['status'] == 'unknown' and len(bridge.reports) == 256)
        bridge.unknown.clear(); bridge.reports.clear()
        fixture.overrides['PartialFailure'] = (4, 'failed')
        failed = fixture.gt('PartialFailure')
        check('DefiniteFailureRequiresResidueReview', failed['status'] == 'failed' and 'PartialFailure' in bridge.unknown
            and read(bridge.run_root / 'Calls/PartialFailure.json')['requiresReview'] is True)
        fixture.close()
        reopened = Bridge(out / 'Adapter/Config.json')
        try:
            description = copy.deepcopy(reopened.operations); description['identity'] = fixture.public
            with patch.object(reopened, '_cli', return_value={'cliExitCode': 0, 'executionState': 'finished', 'response': {'result': description}}):
                connected = reopened.call('nodeforge_connect', {})
            check('ReconnectRetainsUnresolvedGate', connected['status'] == 'connected' and 'PartialFailure' in reopened.unknown)
        finally: reopened.close()
        wrong = copy.deepcopy(read(out / 'Adapter/Config.json')); wrong['docsManifestSha256'] = '0' * 64
        path = out / 'WrongManifestConfig.json'; save(path, wrong)
        try: Bridge(path)
        except BridgeError as error: check('WrongManifestFingerprintPreventsStartup', error.code == 'NFMCP_FILE_SHA')
        else: check('WrongManifestFingerprintPreventsStartup', False)
    finally: fixture.close()
    save(out / 'Review.json', {'stepId': 'P1', 'status': 'passed', 'checks': checks,
        'scope': 'Mock subprocess adapter boundaries only; no UE/actual Codex business', 'ue': 'notRun',
        'reason': 'Necessary P1 process deadline/output/capacity and persistent failed-write gate not covered by prior receipt subset'})
    print(json.dumps({'stepId': 'P1', 'status': 'passed', 'checks': len(checks), 'ue': 'notRun'}))

if __name__ == '__main__': main()
