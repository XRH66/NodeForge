"""Only changed version/data bindings and actual SDK stdio; no repeated adapter matrix."""
import argparse
import asyncio
import copy
import json
from pathlib import Path
from McpBridgeChecks import ROOT, config, read, save, stdio_checks
from NodeForgeMcpBridge import Bridge, BridgeError

def main():
    parser = argparse.ArgumentParser(); parser.add_argument('--out', required=True); args = parser.parse_args()
    out = Path(args.out).resolve()
    assert out.is_relative_to(ROOT / 'Reports/PluginControlStageTwo/McpDocs1/P1') and not out.exists()
    out.mkdir(parents=True); checks = []
    original_path = config(out / 'Valid', out / 'NoLiveSession')
    original = read(original_path)
    bridge = Bridge(original_path)
    try:
        checks.append({'case': 'FrozenVersionRoles', 'passed': bridge.project.as_posix() == original['project']
            and bridge.stage.as_posix() == original['pluginDirectory'] and bridge.bundle_id == original['bundleId']})
    finally: bridge.close()
    for case, field, value in [
        ('WrongBundle', 'bundleId', 'NodeForge-Build999-Mcp1'),
        ('WrongSha', 'binarySha256', '0' * 64),
        ('WrongReportScope', 'reportRoot', (ROOT / 'Reports/Unrelated').as_posix()),
        ('WrongRunScope', 'bridgeRunRoot', (ROOT / 'Reports/Unrelated').as_posix())]:
        wrong = copy.deepcopy(original); wrong[field] = value
        path = out / (case + 'Config.json'); save(path, wrong)
        try: instance = Bridge(path)
        except (BridgeError, ValueError): checks.append({'case': case, 'passed': True})
        else:
            instance.close(); raise AssertionError(case)
    assert all(row['passed'] for row in checks)
    asyncio.run(stdio_checks(out))
    protocol = read(out / 'Stdio/Bridge/ProtocolSeen.json')
    assert protocol['protocolVersion'] == read(out / 'SdkStdio.json')['protocolVersion']
    checks.append({'case': 'ChangedBindingsSdkStdioAndNegotiatedProtocol', 'passed': True})
    save(out / 'Review.json', {'stepId': 'P1', 'status': 'passed', 'checks': checks,
        'scope': 'Only changed release-role binding and server protocol observation; no UE/Codex business',
        'reason': 'Remove Build122 path/bundle constants from bridge logic. The approved current release remains exact Build122 metadata.',
        'nativeOperationTableAndCppChanged': False, 'ue': 'notRun'})
    print(json.dumps({'status': 'passed', 'checks': len(checks), 'ue': 'notRun'}))

if __name__ == '__main__': main()
