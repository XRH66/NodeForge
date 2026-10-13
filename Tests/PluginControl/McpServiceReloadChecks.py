"""One bounded SDK session proves real source loading; no UE/native calls."""
from pathlib import Path
import asyncio
import copy
import hashlib
import json
import os
import sys
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'Reports/PluginControlStageTwo/AssetRead1/B3/ServiceReloadRepair1/Checks2'
DOC = ROOT / 'Docs/Versions/Build129/McpServiceReloadValidation2'
sys.path.insert(0, str(ROOT / 'Scripts'))
import NodeForgeMcpBridge as bridge_module
import NodeForgeMcp as server_module
from mcp import StdioServerParameters
from mcp.client import Client

def read(path): return json.loads(path.read_text(encoding='utf-8-sig'))
def sha(data): return hashlib.sha256(data).hexdigest()
def save(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes((json.dumps(value, ensure_ascii=False, indent=2) + '\n').encode('utf-8'))

rows = []
def check(name, ok, details=None):
    rows.append({'case': name, 'passed': bool(ok), 'details': details})
    save(OUT / 'Cases.json', rows)
    assert ok, name

def prepare_generation(label, version, code):
    folder = DOC / label
    folder.mkdir(parents=True, exist_ok=False)
    from BuildNodeForgeUsageDocs import build_tool_contract
    base = read(ROOT / 'Reports/PluginControlStageTwo/AssetRead1/B2/ReviewedAdapterContract.json')
    operations = read(ROOT / 'Reports/PluginControlStageTwo/AssetRead1/B2/OperationsCandidate.json')
    metadata = read(ROOT / 'Tools/NodeForgeMcp/ToolMetadata.json')
    contract = build_tool_contract(base, operations, metadata, 'Fixture', version,
                                  (folder / 'Manifest.json').relative_to(ROOT).as_posix())
    manifest = read(ROOT / 'Docs/Versions/Build129/Mcp3/Manifest.json')
    bundle = 'NodeForge-Build129-SelfReloadValidation' + label
    contract.update(bundleId=bundle, bridgeVersion=version,
                    docsManifest=(folder / 'Manifest.json').relative_to(ROOT).as_posix())
    save(folder / 'ToolContract.json', contract)
    page = folder / 'ReleaseProbe.md'
    page.write_text('仅无UE服务载入证据；不是产品业务验收。' + label, encoding='utf-8')
    manifest.update(bundleId=bundle, bridgeVersion=version, validationOnly=True,
                    publicationStatus='serviceReloadValidationOnly',
                    sessionDirectory=OUT.relative_to(ROOT).as_posix() + '/' + label,
                    pages=[{'pageId': 'ReleaseProbe', 'path': page.relative_to(ROOT).as_posix(),
                            'bytes': page.stat().st_size, 'sha256': sha(page.read_bytes()), 'kind': 'guide'}],
                    artifacts=[], bindings=[row for row in manifest['bindings']
                        if row['path'] not in ('Scripts/NodeForgeMcp.py', 'Scripts/NodeForgeMcpBridge.py')])
    for name, data in code.items():
        path = 'Scripts/' + name
        manifest['bindings'].append({'path': path, 'bytes': len(data), 'sha256': sha(data)})
        if path in manifest['cliFiles']: manifest['cliFiles'][path] = sha(data)
        target = OUT / 'Sources' / label / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
    save(folder / 'Manifest.json', manifest)
    config = read(ROOT / 'Reports/PluginControlStageTwo/AssetRead1/B3/Run4/Config.json')
    report = OUT / label
    config.update(bundleId=bundle, reportRoot=report.as_posix(),
                  sessionPublic=(report / 'SessionPublic.json').as_posix(),
                  bridgeRunRoot=(report / 'Bridge').as_posix(),
                  docsManifestSha256=sha((folder / 'Manifest.json').read_bytes()),
                  toolContractSha256=sha((folder / 'ToolContract.json').read_bytes()))
    return config, (folder / 'ToolContract.json').read_bytes()

def install(label):
    code, config, contract = generations[label]
    for name, data in code.items(): (ROOT / 'Scripts' / name).write_bytes(data)
    (ROOT / 'Tools/NodeForgeMcp/ToolContract.json').write_bytes(contract)
    save(OUT / 'Config.json', config)

async def check_guards():
    runtime = server_module.ServerRuntime(OUT / 'Config.json')
    old = runtime.bridge
    old.unknown.add('Original')
    install('B')
    try: await runtime.acquire()
    except bridge_module.BridgeError as error:
        check('UnknownStopsBeforeLoading', error.code == 'NFMCP_REBIND_UNRESOLVED' and runtime.bridge is old)
    else: raise AssertionError('unknown admitted')
    old.unknown.clear()
    runtime.inflight = 1
    try: await runtime.acquire()
    except bridge_module.BridgeError as error:
        check('InflightStopsBeforeLoading', error.code == 'NFMCP_REBIND_BUSY' and runtime.bridge is old)
    else: raise AssertionError('inflight admitted')
    runtime.inflight = 0
    old.pending_cli.append({'fixture': True})
    try: await runtime.acquire()
    except bridge_module.BridgeError as error:
        check('PendingCliStopsBeforeLoading', error.code == 'NFMCP_REBIND_UNRESOLVED' and runtime.bridge is old)
    else: raise AssertionError('pending admitted')
    old.pending_cli.clear()
    with patch.object(server_module.initial_bridge, 'safe_read', wraps=server_module.initial_bridge.safe_read) as reader:
        changed = dict(old.loaded_runtime_files)
        old.loaded_runtime_files['Tools/NodeForgeMcp/Requirements.lock'] = '0' * 64
        try: server_module.load_application(old)
        except bridge_module.BridgeError as error:
            check('SdkChangeRequiresRealProcessLoad', error.code == 'NFMCP_SDK_RESTART_REQUIRED'
                  and not any(str(call.args[0]).endswith('NodeForgeMcp.py') for call in reader.call_args_list))
        else: raise AssertionError('SDK change admitted')
        old.loaded_runtime_files = changed
    source = ROOT / 'Scripts/NodeForgeMcp.py'
    saved = source.read_bytes()
    source.write_bytes(saved + b'\n# unapproved bytes\n')
    try:
        try: server_module.load_application(old)
        except bridge_module.BridgeError as error:
            check('UnapprovedSourceRejectedBeforeExecution', error.code == 'NFMCP_FILE_SHA' and runtime.bridge is old)
        else: raise AssertionError('unapproved source executed')
    finally: source.write_bytes(saved)
    runtime.close()
    install('A')

async def sdk_check():
    parameter = StdioServerParameters(command=sys.executable,
        args=[str(ROOT / 'Scripts/NodeForgeMcp.py'), '--config', str(OUT / 'Config.json')], cwd=ROOT)
    async with Client(parameter, read_timeout_seconds=20, mode='2026-07-28', cache=None) as client:
        tools = await client.list_tools()
        first = await client.call_tool('nodeforge_docs_index', {})
        check('InitialRealService', not first.is_error and len(tools.tools) == 17
              and first.structured_content['bridgeVersion'] == '0.3.2')
        before = read(OUT / 'A/Bridge/ProtocolSeen.json')
        install('B')
        second = await client.call_tool('nodeforge_docs_index', {})
        save(OUT / 'ActualForward.json', second.model_dump(mode='json'))
        check('NextCallExecutesFreshBridgeAndServer', not second.is_error
              and second.structured_content['bridgeVersion'] == '0.3.3'
              and second.structured_content.get('serviceSourceProbe') == 'new-service-function-loaded')
        after = read(OUT / 'B/Bridge/ProtocolSeen.json')
        refresh = second.structured_content['configurationRefresh']
        check('SamePidSameSdkConnectionNoManualReload', before['bridgePid'] == after['bridgePid']
              and before['protocolVersion'] == after['protocolVersion'] == client.protocol_version
              and refresh['applicationCodeReloaded'])
        check('LoadedBytesExactlyMatchApprovedRelease', refresh['loadedRuntimeFiles'] == after['loadedRuntimeFiles']
              and refresh['loadedRuntimeFiles']['Scripts/NodeForgeMcp.py'] == sha(generations['B'][0]['NodeForgeMcp.py'])
              and refresh['loadedRuntimeFiles']['Scripts/NodeForgeMcpBridge.py'] == sha(generations['B'][0]['NodeForgeMcpBridge.py']))
        discovered = await client.list_tools()
        check('FreshSchemaDiscovered', len(discovered.tools) == 17 and all(
            tool.output_schema['properties']['bridgeVersion']['type'] == 'string'
            and 'const' not in tool.output_schema['properties']['bridgeVersion'] for tool in discovered.tools))
        page = await client.call_tool('nodeforge_docs_read', {'pageId': 'ReleaseProbe'})
        check('Utf8AndFreshFunctionRemainLoaded', not page.is_error and '载入证据' in page.structured_content['text']
              and page.structured_content['serviceSourceProbe'] == 'new-service-function-loaded')
        install('A')
        rollback = await client.call_tool('nodeforge_docs_index', {})
        save(OUT / 'ActualRollback.json', rollback.model_dump(mode='json'))
        check('AdmittedRollbackReallyUnloadsNewFunction', not rollback.is_error
              and rollback.structured_content['bridgeVersion'] == '0.3.2'
              and 'serviceSourceProbe' not in rollback.structured_content
              and rollback.structured_content['configurationRefresh']['applicationCodeReloaded'])
        check('NoKernelOrUeRestart', read(OUT / 'A/Bridge/ProtocolSeen.json')['bridgePid'] == before['bridgePid'])
        save(OUT / 'SdkIdentity.json', {'pid': before['bridgePid'], 'protocol': client.protocol_version,
            'source': 'actual official SDK stdio, isolated metadata fixture; not current Codex client or UE business'})

def main():
    global generations
    OUT.mkdir(parents=True, exist_ok=False)
    DOC.mkdir(parents=True, exist_ok=False)
    names = ('NodeForgeMcp.py', 'NodeForgeMcpBridge.py', 'NodeForgeEditorTransport.py', 'RunProcessGuard.py')
    baseline = {name: (ROOT / 'Scripts' / name).read_bytes() for name in names}
    original_contract = (ROOT / 'Tools/NodeForgeMcp/ToolContract.json').read_bytes()
    fixed_config = (ROOT / 'Reports/PluginControlStageTwo/McpDocs1/Client/Config.json').read_bytes()
    other_sources = {name: (ROOT / 'Scripts' / name).read_bytes() for name in names[2:]}
    code_a = {name: baseline[name] for name in names[:2]}
    code_b = dict(code_a)
    code_b['NodeForgeMcpBridge.py'] = code_b['NodeForgeMcpBridge.py'].replace(b"VERSION = '0.3.2'", b"VERSION = '0.3.3'")
    code_b['NodeForgeMcp.py'] += (b'\r\n_original_invoke = invoke\r\nasync def invoke(bridge, name, arguments, retained):\r\n'
        b'    value = await _original_invoke(bridge, name, arguments, retained)\r\n'
        b"    value['serviceSourceProbe'] = 'new-service-function-loaded'\r\n    return value\r\n")
    a = prepare_generation('A', '0.3.2', code_a)
    b = prepare_generation('B', '0.3.3', code_b)
    generations = {'A': (code_a, *a), 'B': (code_b, *b)}
    failure = None
    try:
        install('A')
        previous = read(OUT.parent / 'Checks1/Review.json')
        assert all(row['passed'] for row in previous['cases'][:5])
        save(OUT / 'ReusedGuards.json', {'source': 'Checks1/Review.json', 'cases': previous['cases'][:5],
            'reason': 'source loader and guard implementation unchanged; output schema publication only corrected'})
        asyncio.run(sdk_check())
    except BaseException as error:
        failure = type(error).__name__ + ': ' + str(error)
        raise
    finally:
        restored = True
        for name in names[:2]:
            path = ROOT / 'Scripts' / name
            if path.read_bytes() not in (code_a[name], code_b[name]):
                restored = False
                continue  # An external edit must never be overwritten.
            path.write_bytes(baseline[name])
        contract = ROOT / 'Tools/NodeForgeMcp/ToolContract.json'
        if contract.read_bytes() in (a[1], b[1]): contract.write_bytes(original_contract)
        else: restored = False
        restored = restored and all((ROOT / 'Scripts' / name).read_bytes() == data for name, data in baseline.items())
        unchanged = fixed_config == (ROOT / 'Reports/PluginControlStageTwo/McpDocs1/Client/Config.json').read_bytes()
        unchanged = unchanged and all((ROOT / 'Scripts' / name).read_bytes() == data for name, data in other_sources.items())
        save(OUT / 'Review.json', {'status': 'passed' if not failure and restored and unchanged else 'failed',
            'canAdvance': not failure and restored and unchanged, 'cases': rows, 'failure': failure,
            'candidateSourceBytesRestored': restored, 'fixedClientConfigAndTransportSdkUnchanged': unchanged,
            'scope': 'one actual official SDK connection and exact production source loader; metadata-only fixture',
            'ueStarted': False, 'nativeBusiness': 'notRun', 'currentCodexLoaded': False, 'newAssets': 0})

if __name__ == '__main__': main()
