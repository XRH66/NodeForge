"""Installed Codex app-server stdio client, bound to this existing thread only.

No thread creation, model turn, agent delegation, global configuration, or UE launch.
"""
import json
import queue
import shutil
import subprocess
import threading
import time
from pathlib import Path

THREAD = '01a12351-ed01-7023-80ca-3a709a0b5c83'

class CodexMcpClient:
    def __init__(self, root, config, out):
        self.root, self.out = Path(root), Path(out)
        self.out.mkdir(parents=True, exist_ok=True)
        self.number = 0; self.queue = queue.Queue()
        self.stderr = (self.out / 'CodexStderr.txt').open('x', encoding='utf-8')
        self.server = 'nodeforge'
        python = self.root / 'Tools/NodeForgeMcp/Environment/Scripts/python.exe'
        script = self.root / 'Scripts/NodeForgeMcp.py'
        self.server_config = {'command': str(python), 'args': [str(script), '--config', str(config)],
            'startup_timeout_sec': 30, 'tool_timeout_sec': 200}
        inline = 'mcp_servers={nodeforge={command=' + json.dumps(str(python)) + ',args=[' + ','.join(json.dumps(x) for x in self.server_config['args']) + '],startup_timeout_sec=30,tool_timeout_sec=200}}'
        self.codex = shutil.which('codex')
        if not self.codex: raise RuntimeError('Actual installed Codex required')
        self.process = subprocess.Popen([self.codex, 'app-server', '--stdio', '-c', inline, '-c', 'features.apps=false'],
            cwd=self.root, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=self.stderr,
            text=True, encoding='utf-8', creationflags=subprocess.CREATE_NO_WINDOW)
        def pump():
            for line in self.process.stdout:
                try: self.queue.put(json.loads(line))
                except ValueError: self.queue.put({'framingError': True})
            self.queue.put({'eof': True})
        self.worker = threading.Thread(target=pump, daemon=True); self.worker.start()
        self.request('initialize', {'clientInfo': {'name': 'nodeforge_mcp_acceptance', 'version': '0.1.0'},
            'capabilities': {'experimentalApi': True}})
        self.process.stdin.write(json.dumps({'method': 'initialized'}) + '\n'); self.process.stdin.flush()

    def request(self, method, params, timeout=45):
        self.number += 1; number = self.number
        self.process.stdin.write(json.dumps({'id': number, 'method': method, 'params': params}, ensure_ascii=False) + '\n')
        self.process.stdin.flush(); end = time.monotonic() + timeout
        while time.monotonic() < end:
            message = self.queue.get(timeout=max(.01, end-time.monotonic()))
            if message.get('id') == number:
                if 'error' in message:
                    error = RuntimeError('Codex app-server ' + str(message['error'].get('code')) + ': ' + message['error'].get('message', 'error'))
                    error.rpc_error = message['error']; raise error
                return message['result']
            if message.get('eof') or message.get('framingError'): raise RuntimeError('Codex framing/exit unknown')
            if 'id' in message:
                # This client never starts model turns; no auto-approval or hidden interactive action.
                raise RuntimeError('Unexpected Codex server request: ' + str(message.get('method')))
        raise TimeoutError('Codex request result unknown; preserve original call')

    def inventory(self, thread=True):
        params = {'serverName': self.server, 'detail': 'full'}
        if thread: params['threadId'] = THREAD
        return self.request('mcpServerStatus/list', params)

    def call(self, tool, arguments):
        return self.request('mcpServer/tool/call', {'threadId': THREAD, 'server': self.server,
            'tool': tool, 'arguments': arguments}, timeout=210)

    def close(self):
        if self.process.stdin: self.process.stdin.close()
        try: self.process.wait(timeout=15)
        except subprocess.TimeoutExpired:
            # Preserve unknown; do not terminate an app-server with a possibly live bridge/CLI.
            (self.out / 'LiveCodex.json').write_text(json.dumps({'pid': self.process.pid,
                'threadId': THREAD, 'nextAction': 'Inspect original owned client; no replay or process kill'}), encoding='utf-8')
        self.worker.join(timeout=2); self.stderr.close()

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(); parser.add_argument('--config', required=True); parser.add_argument('--out', required=True)
    args = parser.parse_args(); root = Path(__file__).resolve().parents[2]; out = Path(args.out).resolve()
    assert out.is_relative_to(root / 'Reports/PluginControlStageTwo/McpDocs1/P3') and not out.exists()
    client = CodexMcpClient(root, Path(args.config).resolve(), out)
    try:
        inventory = client.inventory(thread=False)
        (out / 'Inventory.json').write_text(json.dumps(inventory, indent=2, ensure_ascii=False), encoding='utf-8')
        result = client.call('nodeforge_docs_index', {})
        (out / 'ExistingThreadDocsCall.json').write_text(json.dumps(result, indent=2, ensure_ascii=False), encoding='utf-8')
        print(json.dumps({'status': 'passed', 'scope': 'Actual Codex existing-thread document call; no UE/model turn'}))
    except Exception as error:
        (out / 'Failure.json').write_text(json.dumps({'status': 'blocked', 'type': type(error).__name__,
            'message': str(error), 'rpcError': getattr(error, 'rpc_error', None), 'ue': 'notRun',
            'threadCreated': False, 'modelTurnStarted': False}, indent=2, ensure_ascii=False), encoding='utf-8')
        print(json.dumps({'status': 'blocked', 'type': type(error).__name__, 'reason': str(error)}))
        raise
    finally: client.close()
