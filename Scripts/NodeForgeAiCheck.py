"""Real MCP stdio discovery and optional read-only UE connection check."""
import argparse
import json
import os
import sys
from pathlib import Path
import anyio
from mcp import StdioServerParameters
from mcp.client import Client


async def check(config, connect):
    root = Path(__file__).resolve().parents[1]
    server = StdioServerParameters(command=sys.executable,
        args=[str(root / 'Scripts/NodeForgeMcp.py'), '--config', str(config)], cwd=str(root),
        env={**{k: v for k, v in os.environ.items() if k != 'NODEFORGE_GITHUB_TOKEN'},
             'PYTHONPATH': '', 'PYTHONHOME': '', 'PYTHONIOENCODING': 'utf-8'})
    async with Client(server, read_timeout_seconds=30) as session:
            tools = await session.list_tools()
            async def call(name, args):
                result = await session.call_tool(name, args)
                value = result.structured_content or json.loads(result.content[0].text)
                if result.is_error or value.get('status') not in ('passed', 'connected', 'finished'):
                    raise RuntimeError('MCP check failed: ' + json.dumps(value, ensure_ascii=False))
                return value
            docs = await call('nodeforge_docs_index', {})
            page = await call('nodeforge_docs_read', {'pageId': 'AiQuickStart'})
            result = {'status': 'passed', 'protocolVersion': session.protocol_version,
                'toolCount': len(tools.tools), 'bundleId': docs['manifest']['bundleId'],
                'binarySha256': docs['manifest']['binarySha256'], 'python': sys.executable,
                'documentRead': page['pageId'], 'connection': 'notRequested', 'assetWrites': False}
            if connect:
                connected = await call('nodeforge_connect', {})
                handle = connected['handle']
                state = await call('nodeforge_editor_getState', {'handle': handle, 'arguments': {}})
                if (state.get('cliExitCode') != 0 or state.get('nativeSummary', {}).get('response', {}).get('result', {}).get('identity') != connected['identity']):
                    raise RuntimeError('Read-only native state identity/exit mismatch')
                result.update(connection='connected', identity=connected['identity'],
                    readOnlyStateStatus=state['status'])
            print(json.dumps(result, ensure_ascii=False))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config', required=True)
    parser.add_argument('--connect', action='store_true')
    args = parser.parse_args()
    anyio.run(check, Path(args.config), args.connect)
