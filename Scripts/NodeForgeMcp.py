"""NodeForge local stdio MCP server; SDK owns framing, CLI owns UE authentication."""
import argparse
import builtins
import json
import os
import sys
import uuid
from types import ModuleType
from datetime import datetime, timezone
import anyio
from mcp import types
from mcp.server.lowlevel import Server
from mcp.server.lowlevel.server import NotificationOptions
from mcp.server.stdio import stdio_server
from mcp_types.version import MODERN_PROTOCOL_VERSIONS
from NodeForgeMcpBridge import Bridge, BridgeError, TransportError, VERSION, result, write_new
import NodeForgeMcpBridge as initial_bridge

def load_application(old):
    """Execute only an exact admitted release, isolated from the live generation."""
    root, home = initial_bridge.ROOT, initial_bridge.HOME
    read, parse, digest = initial_bridge.safe_read, initial_bridge.strict_json, initial_bridge.sha
    for path in ('Tools/NodeForgeMcp/DependencyManifest.json', 'Tools/NodeForgeMcp/Requirements.lock'):
        if digest(read(root / path, initial_bridge.MAX_REPORT)) != old.loaded_runtime_files[path]:
            raise BridgeError('NFMCP_SDK_RESTART_REQUIRED', 'environmentBlocked')
    config_bytes = read(old.config_path, 65536, roots=(home, old.control_reports))
    config = parse(config_bytes)
    contract_path = home / 'ToolContract.json'
    contract = parse(read(contract_path, initial_bridge.MAX_REPORT, config['toolContractSha256'], (home,)))
    manifest_path = root / contract['docsManifest']
    manifest = parse(read(manifest_path, initial_bridge.MAX_REPORT, config['docsManifestSha256'],
                         (root / 'Docs/Versions',)))
    bindings = {**manifest['cliFiles'], **{row['path']: row['sha256'] for row in manifest['bindings']}}
    names = ('RunProcessGuard', 'NodeForgeEditorTransport', 'NodeForgeMcpBridge', 'NodeForgeMcp')
    sources = {name: read(root / ('Scripts/' + name + '.py'), initial_bridge.MAX_REPORT,
                          bindings['Scripts/' + name + '.py'], (root / 'Scripts',)) for name in names}
    programs = {name: compile(data, str(root / ('Scripts/' + name + '.py')), 'exec')
                for name, data in sources.items()}
    modules = {}
    original_import = builtins.__import__
    def application_import(name, globals=None, locals=None, fromlist=(), level=0):
        if not level and name in programs:
            return load(name)
        return original_import(name, globals, locals, fromlist, level)
    def load(name):
        if name not in modules:
            module = ModuleType('_nodeforge_release.' + name)
            module.__file__ = str(root / ('Scripts/' + name + '.py'))
            module.__dict__['__builtins__'] = {**vars(builtins), '__import__': application_import}
            modules[name] = module
            exec(programs[name], module.__dict__)
        return modules[name]
    app = load('NodeForgeMcp')
    if app.APPLICATION_ABI != APPLICATION_ABI:
        raise BridgeError('NFMCP_APPLICATION_INCOMPATIBLE', 'environmentBlocked')
    try:
        candidate = app.Bridge(old.config_path)
    except (app.BridgeError, app.TransportError) as error:
        raise BridgeError(error.code, getattr(error, 'status', 'environmentBlocked')) from error
    for name, data in sources.items():
        path = 'Scripts/' + name + '.py'
        if candidate.loaded_runtime_files[path] != digest(data):
            raise BridgeError('NFMCP_FILE_SHA', 'environmentBlocked')
        read(root / path, initial_bridge.MAX_REPORT, digest(data))
    read(old.config_path, 65536, digest(config_bytes), (home, old.control_reports))
    candidate.application = app
    return candidate

APPLICATION_ABI = 1

def tool_definitions(bridge):
    rows = []
    for item in bridge.contract['tools']:
        definition = item.get('nativeDefinition', {})
        rows.append(types.Tool(name=item['name'],
            description=item.get('description', (item['operation'] or item['name']) + '; read docs_index for exact version and scope; unknown requires original calls.get, never replay.'),
            inputSchema=item['inputSchema'], outputSchema=item['outputSchema'],
            annotations=types.ToolAnnotations(readOnlyHint=definition.get('dispatch') == 'readOnlyCache'
                or item['name'] in ('nodeforge_docs_index', 'nodeforge_docs_read', 'nodeforge_report_read'),
                destructiveHint=False, idempotentHint=definition.get('dispatch') == 'readOnlyCache', openWorldHint=False)))
    return types.ListToolsResult(tools=rows)

async def invoke(bridge, name, arguments, retained):
    from functools import partial
    value = await anyio.to_thread.run_sync(partial(bridge.call, name, arguments, retained_recovery=retained))
    if name == 'nodeforge_docs_index' and value['status'] == 'passed':
        value['serviceRuntime'] = {'pid': os.getpid(), 'applicationAbi': APPLICATION_ABI,
                                  'loadedRuntimeFiles': bridge.loaded_runtime_files}
        if len(initial_bridge.encoded(value)) > initial_bridge.MAX_SELECTED:
            return result('resultTooLarge', selectionReturned=False, limitBytes=initial_bridge.MAX_SELECTED,
                docsManifestSha256=value['docsManifestSha256'], errors=[{'code': 'NFMCP_DISCOVERY_INDEX_TOO_LARGE'}])
        path = bridge.run_root / 'RuntimeObservations' / ('Runtime' + str(os.getpid()) +
            bridge.loaded_runtime_files['Scripts/NodeForgeMcp.py'] + '.json')
        if not path.exists():
            try:
                write_new(path, {**value['serviceRuntime'], 'format': 'NodeForgeServiceRuntime1',
                    'bridgeVersion': bridge.contract['bridgeVersion'], 'bundleId': bridge.bundle_id,
                    'method': name, 'observedAtUtc': datetime.now(timezone.utc).isoformat()})
            except FileExistsError:
                pass
    return value

class ServerRuntime:
    """One SDK connection, replaceable admitted bridge, no automatic UE actions."""
    def __init__(self, config_path):
        self.bridge = Bridge(config_path)
        self.gate = anyio.Lock()
        self.inflight = 0
        self.application = sys.modules[__name__]
        self.client_info = None
        self.client_id = uuid.uuid4().hex
        self.presence_started = False
        self.presence_error = None

    def observe_client(self, ctx):
        initialized=ctx.session.client_params
        info=initialized.client_info if initialized is not None else None
        if info is not None and isinstance(info.name,str) and 0<len(info.name)<=80 and isinstance(info.version,str) and len(info.version)<=80:
            self.client_info={'id':self.client_id,'name':info.name,'version':info.version}

    async def presence(self):
        while True:
            if self.client_info is not None:
                try:
                    if not self.presence_started:
                        print(json.dumps({'event':'NodeForgeClientPresenceStarted','clientName':self.client_info['name']}),file=sys.stderr,flush=True)
                        write_new(self.bridge.run_root/'PresenceStarted.json',{'clientName':self.client_info['name'],'bridgePid':os.getpid()})
                        self.presence_started=True
                    await anyio.to_thread.run_sync(self.bridge.client_presence_tick,self.client_info)
                except (BridgeError,TransportError,self.application.BridgeError,
                        self.application.TransportError,OSError,ValueError,KeyError) as error:
                    # A missing/ambiguous editor is an accurate offline state;
                    # presence never replays business or replaces its receipts.
                    print(json.dumps({'event':'NodeForgeClientPresenceUnavailable','code':getattr(error,'code',type(error).__name__)}),file=sys.stderr,flush=True)
                    code=getattr(error,'code',type(error).__name__)
                    if code != self.presence_error:
                        write_new(self.bridge.run_root/'PresenceObservations'/('Error'+uuid.uuid4().hex+'.json'),{'code':code,'observedAtUtc':datetime.now(timezone.utc).isoformat()})
                        self.presence_error=code
            await anyio.sleep(2)

    async def acquire(self, name=None, params=None):
        async with self.gate:
            refresh = None
            retained = False
            code_changed = any(initial_bridge.sha(initial_bridge.safe_read(initial_bridge.ROOT / path,
                initial_bridge.MAX_REPORT)) != digest for path, digest in self.bridge.loaded_runtime_files.items())
            if self.bridge.configuration_changed() or code_changed:
                retained = self.bridge.retained_query(name, params or {})
                if code_changed and not self.bridge.discovery or not retained:
                    if self.inflight:
                        raise BridgeError('NFMCP_REBIND_BUSY', 'environmentBlocked')
                    from functools import partial
                    factory = self.application.load_application if code_changed else None
                    candidate, refresh = await anyio.to_thread.run_sync(partial(self.bridge.rebind, factory))
                    self.bridge = candidate
                    self.application = getattr(candidate, 'application', self.application)
                    retained = False
            self.inflight += 1
            return self.bridge, refresh, retained

    async def release(self):
        with anyio.CancelScope(shield=True):
            async with self.gate:
                self.inflight -= 1

    def close(self):
        self.bridge.close()

async def serve(config_path):
    runtime = ServerRuntime(config_path)
    def protocol_seen(ctx, bridge):
        path = bridge.run_root / 'ProtocolSeen.json'
        if not path.exists():
            try:
                write_new(path, {'protocolVersion': ctx.protocol_version, 'method': ctx.method,
                                 'clientInfo': runtime.client_info,
                                 'observedAtUtc': datetime.now(timezone.utc).isoformat(),
                                 'bridgePid': os.getpid(),
                                 'bridgeVersion': bridge.contract['bridgeVersion'], 'transport': 'stdio',
                                 'loadedRuntimeFiles': bridge.loaded_runtime_files})
            except FileExistsError:
                # A concurrent read-only client already recorded the first observation.
                pass
    async def list_tools(ctx, params):
        runtime.observe_client(ctx)
        bridge, refresh, _ = await runtime.acquire()
        try:
            protocol_seen(ctx, bridge)
            return runtime.application.tool_definitions(bridge)
        finally:
            await runtime.release()
    async def call_tool(ctx, params):
        runtime.observe_client(ctx)
        # A client's cancellation cannot abandon an issued UE operation or its durable receipt.
        with anyio.CancelScope(shield=True):
            try:
                bridge, refresh, retained = await runtime.acquire(params.name, params.arguments or {})
                try:
                    protocol_seen(ctx, bridge)
                    if refresh and refresh['toolsChanged'] and ctx.protocol_version not in MODERN_PROTOCOL_VERSIONS:
                        # Modern revisions use subscriptions/listen, which this server
                        # does not advertise. Never send a legacy notification there.
                        await ctx.session.send_tool_list_changed()
                    value = await runtime.application.invoke(bridge, params.name, params.arguments or {}, retained)
                    if retained:
                        value['configurationRefreshPending'] = True
                    if refresh:
                        value['configurationRefresh'] = {k: refresh[k] for k in (
                            'oldBundleId', 'newBundleId', 'oldHandleInvalidated', 'newConnectionRequired', 'toolsChanged',
                            'applicationCodeReloaded', 'loadedRuntimeFiles')}
                        value['configurationRefresh']['toolDiscovery'] = 'tools/list'
                finally:
                    await runtime.release()
            except Exception as error:
                if not isinstance(error, (BridgeError, TransportError, runtime.application.BridgeError,
                    runtime.application.TransportError, ValueError, OSError, KeyError, TypeError, SyntaxError)):
                    raise
                value = runtime.application.result(getattr(error, 'status', 'environmentBlocked'),
                    errors=[{'code': getattr(error, 'code', 'NFMCP_REBIND_UNAVAILABLE')}],
                    unresolvedCalls=sorted(runtime.bridge.unknown))
        is_error = value['status'] not in ('passed', 'connected', 'finished')
        return types.CallToolResult(content=[types.TextContent(type='text', text=json.dumps(value, ensure_ascii=False, allow_nan=False))],
                                    structuredContent=value, isError=is_error)
    async def resources(ctx, params): return types.ListResourcesResult(resources=[])
    async def templates(ctx, params): return types.ListResourceTemplatesResult(resourceTemplates=[])
    server = Server('NodeForge', version=VERSION, instructions=runtime.bridge.contract.get('instructions', 'Read nodeforge_docs_index first; business requires an explicitly prepared current editor.'), on_list_tools=list_tools, on_call_tool=call_tool,
        on_list_resources=resources, on_list_resource_templates=templates,
        get_tool_input_schema=lambda name: runtime.bridge.tools.get(name, {}).get('inputSchema'))
    async def initialization_observer(ctx, call_next):
        value=await call_next(ctx)
        if ctx.method=='notifications/initialized':runtime.observe_client(ctx)
        return value
    server.middleware.append(initialization_observer)
    try:
        async with stdio_server() as (incoming, outgoing):
            async with anyio.create_task_group() as tasks:
                tasks.start_soon(runtime.presence)
                try:
                    await server.run(incoming, outgoing, server.create_initialization_options(
                        notification_options=NotificationOptions(tools_changed=True)))
                finally:tasks.cancel_scope.cancel()
    finally: runtime.close()

def main():
    parser = argparse.ArgumentParser(description='Explicit local NodeForge stdio bridge; no automatic UE process management')
    parser.add_argument('--config', required=True)
    args = parser.parse_args()
    try: anyio.run(serve, args.config)
    except Exception as error:
        print(json.dumps({'bridgeError': getattr(error, 'code', type(error).__name__)}), file=sys.stderr)
        return 3
    return 0

if __name__ == '__main__': sys.exit(main())
