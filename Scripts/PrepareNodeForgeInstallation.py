"""Explicit check/bind/rollback for one selected project; never launches UE."""
import argparse
import base64
import hashlib
import json
import os
import subprocess
import sys
import time
import uuid
from datetime import datetime, timezone
from pathlib import Path

from RunProcessGuard import (DEFAULT_READ_ROOTS, installation_json, installation_path,
                            installation_product, load_installation_binding, validate_read_roots,
                            observed_module, process_identity)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def encoded(data):
    return (json.dumps(data, ensure_ascii=False, indent=2, allow_nan=False) + '\n').encode('utf-8')


def capture(path):
    path = installation_path(path)
    if not path.exists():
        return {'path': str(path), 'exists': False, 'sha256': None, 'bytesBase64': None}
    if not path.is_file() or path.stat().st_size > 65536:
        raise ValueError('Only bounded project/binding files may be changed')
    data = path.read_bytes()
    return {'path': str(path), 'exists': True, 'sha256': sha(data),
            'bytesBase64': base64.b64encode(data).decode('ascii')}


def matches(path, expected):
    return (not path.exists()) if expected is None else path.is_file() and sha(path.read_bytes()) == expected


def replace_exact(path, expected, content):
    path = installation_path(path)
    if not matches(path, expected):
        raise ValueError('File changed since preparation; stop without replacing it')
    path.parent.mkdir(parents=True, exist_ok=True)
    pending = path.with_name('NodeForgePreparePending' + path.suffix)
    with pending.open('xb') as stream:
        stream.write(content)
        stream.flush()
        os.fsync(stream.fileno())
    if not matches(path, expected):
        raise ValueError('File changed during preparation; pending bytes preserved for review')
    os.replace(pending, path)


def selected_project_closed(project):
    # Reuse the official client's owned-process query, including fail-closed
    # missing command-line handling. No PID is stopped or restarted here.
    from NodeForgeControl import assert_exclusive
    assert_exclusive(project)


def report_directory(root, requested):
    if requested:
        path = installation_path(requested)
    else:
        path = root / 'Reports/Installations' / datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
    if not path.is_relative_to(root / 'Reports') or path.is_relative_to(root / 'Reports/PluginControlSessions'):
        raise ValueError('Fresh private installation report directory required')
    path.mkdir(parents=True, exist_ok=False)
    return path


def write_receipt(path, receipt):
    data = encoded(receipt)
    pending = path.with_name('ReceiptPending.json')
    with pending.open('xb') as stream:
        stream.write(data)
        stream.flush()
        os.fsync(stream.fileno())
    os.replace(pending, path)


def write_private_new(path, value):
    path = installation_path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('xb') as stream:
        stream.write(encoded(value))
        stream.flush()
        os.fsync(stream.fileno())


def command(executable, arguments, root):
    # Keep arguments separate: shell quoting and one-line display are not commands.
    return {'command': str(executable), 'args': [str(value) for value in arguments], 'cwd': str(root)}


def editor_preparation(args):
    root, engine, plugin, binary, product = installation_product(args.installation_root, args.engine_root)
    project = installation_path(args.project)
    binding = load_installation_binding(project, plugin, root)
    selected_project_closed(project)
    interpreter = installation_path(args.python or sys.executable)
    dependencies = installation_json(root / 'Tools/NodeForgeMcp/DependencyManifest.json', 262144)
    if not interpreter.is_file():
        raise ValueError('Existing Python interpreter with the fixed MCP dependencies required; nothing installed')
    probe = subprocess.run([str(interpreter), '-c',
        'import importlib.metadata,json,sys; print(json.dumps({"python":list(sys.version_info[:2]),"packages":{n:importlib.metadata.version(n) for n in json.loads(sys.argv[1])}}))',
        json.dumps([row['name'] for row in dependencies['packages']])], cwd=root, capture_output=True,
        text=True, encoding='utf-8', timeout=15, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
    if probe.returncode != 0:
        raise ValueError('Selected Python lacks the fixed MCP environment; use the documented local dependency setup first')
    versions = json.loads(probe.stdout)
    if (versions['python'] != [3, 12] or dependencies['sdkVersion'] != '2.3.0'
            or any(versions['packages'].get(row['name']) != row['version'] for row in dependencies['packages'])):
        raise ValueError('Selected Python/SDK dependency versions differ from the approved manifest')
    contract_path = root / 'Tools/NodeForgeMcp/ToolContract.json'
    contract = installation_json(contract_path, 1048576)
    manifest_path = installation_path(root / contract['docsManifest'])
    if not manifest_path.is_relative_to(root / 'Docs/Versions'):
        raise ValueError('Approved product-relative version documentation required')
    manifest = installation_json(manifest_path, 1048576)
    namespace = installation_path(root / manifest['reportNamespace'])
    if (manifest.get('installationSchemaVersion') != 1 or manifest.get('bundleId') != contract['bundleId']
            or manifest.get('bridgeVersion') != contract['bridgeVersion']
            or manifest.get('binarySha256') != product['binarySha256']
            or not namespace.is_relative_to(root / 'Reports/PluginControlStageTwo')):
        raise ValueError('Prepare requires one complete product/bridge/docs/stage identity')
    requested = installation_path(args.report_directory) if args.report_directory else namespace / (
        'Editor' + datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S%fZ'))
    if not requested.is_relative_to(namespace) or requested == namespace:
        raise ValueError('New editor registration must be inside the approved report namespace')
    editor_exe = engine / 'Engine/Binaries/Win64/UnrealEditor.exe'
    if not editor_exe.is_file():
        raise ValueError('Selected UE editor executable missing; no process started')
    requested.mkdir(parents=True, exist_ok=False)
    receipt_path = requested / 'PreparationReceipt.json'
    receipt = {'format': 'NodeForgeEditorPreparationReceipt', 'version': 1, 'status': 'preparing',
               'installationRoot': str(root), 'project': str(project), 'binarySha256': product['binarySha256'],
               'bindingSha256': sha((project.parent / 'Config/NodeForgeInstallation.json').read_bytes()),
               'completedWrites': [], 'requiresReview': False, 'ueStarted': False, 'codexConfigChanged': False}
    write_receipt(receipt_path, receipt)
    try:
        registration = {'project': project.as_posix(), 'pluginDirectory': plugin.as_posix(),
                        'binarySha256': product['binarySha256'], 'reportDirectory': requested.as_posix()}
        config = {'format': 'NodeForgeMcpConfig', 'version': 1, 'bundleId': manifest['bundleId'],
                  'project': project.as_posix(), 'pluginDirectory': plugin.as_posix(),
                  'binarySha256': product['binarySha256'], 'sessionPublic': (requested / 'SessionPublic.json').as_posix(),
                  'reportRoot': requested.as_posix(), 'bridgeRunRoot': (requested / 'Bridge').as_posix(),
                  'toolContractSha256': sha(contract_path.read_bytes()), 'docsManifestSha256': sha(manifest_path.read_bytes())}
        stop_call = 'PreparationStop' + uuid.uuid4().hex
        recipes = {'format': 'NodeForgePrivateCommands', 'version': 1,
            'stopCallId': stop_call,
            'mcp': command(interpreter, [root / 'Scripts/NodeForgeMcp.py', '--config', requested / 'Config.json'], root),
            'editor': command(editor_exe, [project, '-NFControlEnable', '-NFControlRegistration=' + str(requested / 'Registration.json'),
                '-NoLiveCoding', '-d3d11', '-nop4', '-nosound', '-abslog=' + str(requested / 'Editor.log')], root),
            'afterStartup': command(interpreter, [root / 'Scripts/PrepareNodeForgeInstallation.py', '--action', 'editor-commands',
                '--installation-root', root, '--project', project, '--engine-root', engine,
                '--report-directory', requested, '--call-id', stop_call, '--python', interpreter], root),
            'normalCloseAfterChannelStop': command(interpreter, [root / 'Scripts/PrepareNodeForgeInstallation.py', '--action', 'close-editor',
                '--installation-root', root, '--project', project, '--engine-root', engine, '--report-directory', requested], root),
            'pythonIdentity': versions, 'readRoots': binding['readRoots'], 'ueStarted': False,
            'manualClientRegistration': 'Use command, each args element and cwd exactly; existing clients retain their fixed approved config path after a safe grouped activation.'}
        for filename, value in [('Registration.json', registration), ('Config.json', config), ('Commands.json', recipes)]:
            selected_project_closed(project)
            write_private_new(requested / filename, value)
            receipt['completedWrites'].append({'path': str(requested / filename), 'sha256': sha((requested / filename).read_bytes())})
            write_receipt(receipt_path, receipt)
        receipt.update(status='passed', finishedAtUtc=datetime.now(timezone.utc).isoformat())
        write_receipt(receipt_path, receipt)
        return {'status': 'passed', 'action': 'prepare-editor', 'receipt': str(receipt_path),
                'commands': recipes, 'config': str(requested / 'Config.json'), 'ueStarted': False, 'codexConfigChanged': False}
    except BaseException as error:
        receipt.update(status='failed', requiresReview=bool(receipt['completedWrites']),
                       errorType=type(error).__name__, message=str(error))
        write_receipt(receipt_path, receipt)
        error.installation_receipt = str(receipt_path)
        error.installation_requires_review = receipt['requiresReview']
        raise


def owned_editor(args):
    root, engine, plugin, binary, product = installation_product(args.installation_root, args.engine_root)
    project = installation_path(args.project)
    load_installation_binding(project, plugin, root)
    if not args.report_directory:
        raise ValueError('Exact current registration report directory required')
    out = installation_path(args.report_directory)
    if not out.is_relative_to(root / 'Reports/PluginControlStageTwo'):
        raise ValueError('Original private editor registration required')
    preparation = installation_json(out / 'PreparationReceipt.json')
    public = installation_json(out / 'SessionPublic.json')
    registration = installation_json(out / 'Registration.json')
    if (preparation.get('status') != 'passed' or preparation.get('requiresReview') is not False
            or preparation.get('bindingSha256') != sha((project.parent / 'Config/NodeForgeInstallation.json').read_bytes())
            or preparation.get('binarySha256') != product['binarySha256']
            or registration != {'project': project.as_posix(), 'pluginDirectory': plugin.as_posix(),
                                'binarySha256': product['binarySha256'], 'reportDirectory': out.as_posix()}
            or installation_path(public['project']) != project or installation_path(public['module']) != binary
            or public['binarySha256'] != product['binarySha256'] or installation_path(public['reportRoot']) != out
            or installation_path(public['sessionFile']) != root / 'Reports/PluginControlSessions' / public['controlSessionId'] / 'Session.json'
            or type(public['pid']) is not int or public['pid'] <= 0):
        raise ValueError('Current preparation/registration/selected project/module identity conflict')
    for item in preparation['completedWrites']:
        path = installation_path(item['path'])
        if path.parent != out or sha(path.read_bytes()) != item['sha256']:
            raise ValueError('Original private preparation file changed')
    return root, engine, plugin, binary, project, out, public


def editor_commands(args):
    root, engine, plugin, binary, project, out, public = owned_editor(args)
    identity = process_identity(public['pid'])
    module = observed_module(public['pid'], binary.name)
    if (identity.get('state') != 'running' or identity.get('createdTicks') != public['processStart']
            or module is None or module.resolve() != binary):
        raise ValueError('Exact current owned editor process required')
    if not args.call_id or not args.call_id.strip() or len(args.call_id) > 128:
        raise ValueError('One explicit new stop callId required; do not change it after dispatch')
    common = [root / 'Scripts/NodeForgeControl.py', '--session-file', public['sessionFile'], '--project', project,
              '--plugin-directory', plugin, '--timeout-seconds', '180']
    python = installation_path(args.python or sys.executable)
    return {'status': 'passed', 'action': 'editor-commands', 'identity': public, 'executed': False,
        'describe': command(python, [common[0], 'editor-describe', *common[1:], '--run-directory', out / 'Describe'], root),
        'stop': command(python, [common[0], 'editor-call', *common[1:], '--operation', 'control.disable',
            '--call-id', args.call_id, '--run-directory', out / 'Stop'], root),
        'recoverOriginalStop': command(python, [common[0], 'editor-status', *common[1:],
            '--call-id', args.call_id, '--run-directory', out / 'StopStatus'], root)}


def close_editor(args):
    """Normal close only AFTER the original channel confirmed all workers stopped."""
    import ctypes
    from ctypes import wintypes
    root, engine, plugin, binary, project, out, public = owned_editor(args)
    stop = installation_json(out / 'StopReceipt.json')
    if (stop.get('controlSessionId') != public['controlSessionId'] or stop.get('state') != 'stopped'
            or stop.get('descriptorDeleted') is not True or stop.get('transportHandlesReleased') is not True
            or stop.get('ioCompletionUnknownObserved') is not False or Path(public['sessionFile']).exists()
            or not isinstance(stop.get('workers'), list) or not stop['workers']
            or any(w.get('pendingIo') is not False or w.get('threadDone') is not True for w in stop['workers'])):
        raise ValueError('Original channel stop/descriptor/worker completion unresolved; no window closed')
    before = process_identity(public['pid'])
    if before.get('state') in ('absent', 'exited'):
        return {'status': 'passed', 'action': 'close-editor', 'alreadyExited': True, 'process': before, 'forceKill': False}
    module = observed_module(public['pid'], binary.name)
    if (before.get('state') != 'running' or before.get('createdTicks') != public['processStart']
            or module is None or module.resolve() != binary):
        raise ValueError('Exact PID creation time/loaded module required; no window closed')
    user = ctypes.WinDLL('user32', use_last_error=True)
    user.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
    user.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
    user.IsWindowVisible.argtypes = [wintypes.HWND]
    user.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
    callback = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    windows = []
    @callback
    def visit(hwnd, unused):
        pid = wintypes.DWORD()
        user.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
        if pid.value == public['pid'] and user.IsWindowVisible(hwnd):
            text = ctypes.create_unicode_buffer(1024)
            user.GetWindowTextW(hwnd, text, len(text))
            if project.stem in text.value:
                windows.append((int(hwnd), text.value))
        return True
    user.EnumWindows.argtypes = [callback, wintypes.LPARAM]
    if not user.EnumWindows(visit, 0) or len(windows) != 1:
        raise ValueError('One exact owned editor main window required; no window closed')
    if process_identity(public['pid']) != before:
        raise ValueError('Owned process identity changed before close')
    if not 0 < args.timeout_seconds <= 60:
        raise ValueError('Finite normal close timeout must be within 60 seconds')
    admission = out / 'NormalCloseAdmission.json'
    write_private_new(admission, {'status': 'admitted', 'identity': public, 'windows': windows,
        'stopReceiptSha256': sha((out / 'StopReceipt.json').read_bytes()), 'forceKill': False})
    if not user.PostMessageW(windows[0][0], 0x0010, 0, 0):
        raise ValueError('Normal WM_CLOSE failed; preserve admission and inspect original process')
    write_private_new(out / 'NormalCloseRequested.json', {'pid': public['pid'], 'processStart': public['processStart'],
                                                       'wmClosePostedOnce': True, 'forceKill': False})
    deadline = time.monotonic() + args.timeout_seconds
    observation = process_identity(public['pid'])
    while observation.get('state') == 'running' and observation.get('createdTicks') == public['processStart'] and time.monotonic() < deadline:
        time.sleep(0.2)
        observation = process_identity(public['pid'])
    exited = observation.get('state') in ('absent', 'exited') or (
        observation.get('state') == 'running' and observation.get('createdTicks') != public['processStart'])
    value = {'status': 'passed' if exited else 'pending', 'action': 'close-editor', 'process': observation,
             'originalPid': public['pid'], 'originalCreationTime': public['processStart'], 'forceKill': False,
             'nextAction': 'Verify selected project is closed before independent Verify' if exited else 'Inspect the original process; do not post again or force kill'}
    write_private_new(out / 'NormalCloseObservation.json', value)
    return value


def prepare(args):
    root, engine, plugin, binary, manifest = installation_product(args.installation_root, args.engine_root)
    project = installation_path(args.project)
    if not project.is_file() or project.suffix != '.uproject':
        raise ValueError('Explicit existing selected uproject required')
    selected_project_closed(project)
    roots = validate_read_roots(args.read_root or DEFAULT_READ_ROOTS, project, engine)
    if args.action == 'check':
        current = load_installation_binding(project, plugin, root)
        return {'status': 'passed', 'action': 'check', 'project': str(project),
                'pluginDirectory': str(plugin), 'binarySha256': current['binarySha256'], 'changed': False}
    out = report_directory(root, args.report_directory)
    receipt_path = out / 'Receipt.json'
    binding_path = project.parent / 'Config/NodeForgeInstallation.json'
    receipt = {'format': 'NodeForgeInstallationReceipt', 'version': 1, 'action': args.action,
               'installationRoot': str(root), 'project': str(project), 'engineRoot': str(engine),
               'startedAtUtc': datetime.now(timezone.utc).isoformat(), 'status': 'preparing',
               'changes': [], 'completedWrites': [], 'requiresReview': False}
    write_receipt(receipt_path, receipt)
    try:
        if args.action == 'rollback':
            if not args.receipt:
                raise ValueError('Exact original bind receipt required for rollback')
            original_path = installation_path(args.receipt)
            if not original_path.is_relative_to(root / 'Reports'):
                raise ValueError('Original receipt must belong to this explicit installation')
            original = installation_json(original_path, 262144)
            if (original.get('format') != 'NodeForgeInstallationReceipt' or original.get('version') != 1
                    or original.get('action') != 'bind' or original.get('status') != 'passed'
                    or installation_path(original.get('project', '')) != project
                    or installation_path(original.get('installationRoot', '')) != root
                    or original.get('requiresReview') is not False):
                raise ValueError('Completed same-project bind receipt required')
            changes = original.get('changes')
            if not isinstance(changes, list) or len(changes) != 2:
                raise ValueError('Exact original two-file installation receipt required')
            permitted = {project, binding_path}
            restorations = []
            for change in changes:
                path = installation_path(change['path'])
                before = change['before']
                if path not in permitted or installation_path(before['path']) != path or not matches(path, change['afterSha256']):
                    raise ValueError('Rollback file identity changed; preserve external edits')
                restored = base64.b64decode(before['bytesBase64'], validate=True) if before['exists'] else None
                if restored is not None and sha(restored) != before['sha256']:
                    raise ValueError('Rollback original bytes SHA mismatch')
                restorations.append((path, change['afterSha256'], restored))
            if {item[0] for item in restorations} != permitted:
                raise ValueError('Rollback receipt has duplicate/missing target')
            receipt['changes'] = changes
            write_receipt(receipt_path, receipt)
            for path, expected, content in reversed(restorations):
                selected_project_closed(project)
                if content is None:
                    if not matches(path, expected):
                        raise ValueError('Rollback changed binding rejected')
                    path.unlink()  # Only own freshly created exact after-SHA binding.
                else:
                    replace_exact(path, expected, content)
                receipt['completedWrites'].append(str(path))
                write_receipt(receipt_path, receipt)
        else:
            descriptor = installation_json(project)
            entries = descriptor.get('Plugins', [])
            if not isinstance(entries, list) or any(not isinstance(entry, dict) for entry in entries):
                raise ValueError('Project plugin references must be objects')
            nodeforge = [entry for entry in entries if entry.get('Name') == 'NodeForge']
            copied = plugin == installation_path(project.parent / 'Plugins/NodeForge')
            if len(nodeforge) > 1 or (not copied and (project.parent / 'Plugins/NodeForge/NodeForge.uplugin').exists()):
                raise ValueError('Ambiguous NodeForge installation rejected')
            directories = descriptor.get('AdditionalPluginDirectories', [])
            if not isinstance(directories, list) or not all(isinstance(value, str) for value in directories):
                raise ValueError('Project additional plugin directories must be strings')
            kept = []
            for directory in directories:
                path = installation_path(project.parent / directory)
                if (path / 'NodeForge/NodeForge.uplugin').is_file():
                    # Only remove a dedicated NodeForge stage entry, preserving
                    # directories that carry unrelated plugins.
                    children = list(path.iterdir())
                    if len(children) != 1 or children[0].name != 'NodeForge':
                        raise ValueError('Existing shared plugin directory cannot be replaced automatically')
                else:
                    kept.append(directory)
            reference = dict(nodeforge[0]) if nodeforge else {'Name': 'NodeForge'}
            reference['Enabled'] = True
            descriptor['Plugins'] = [reference if entry.get('Name') == 'NodeForge' else entry for entry in entries]
            if not nodeforge:
                descriptor['Plugins'].append(reference)
            descriptor['AdditionalPluginDirectories'] = kept if copied else kept + [str(plugin.parent)]
            binding = {'format': 'NodeForgeInstallationBinding', 'version': 1, 'installationRoot': str(root),
                       'project': str(project), 'pluginDirectory': str(plugin), 'binarySha256': manifest['binarySha256'],
                       'engineRoot': str(engine), 'readRoots': roots}
            planned = [(project, encoded(descriptor)), (binding_path, encoded(binding))]
            for path, content in planned:
                receipt['changes'].append({'path': str(path), 'before': capture(path), 'afterSha256': sha(content)})
            write_receipt(receipt_path, receipt)
            for (path, content), change in zip(planned, receipt['changes']):
                selected_project_closed(project)
                replace_exact(path, change['before']['sha256'], content)
                receipt['completedWrites'].append(str(path))
                write_receipt(receipt_path, receipt)
            load_installation_binding(project, plugin, root)
        receipt.update(status='passed', finishedAtUtc=datetime.now(timezone.utc).isoformat())
        write_receipt(receipt_path, receipt)
        return {'status': 'passed', 'action': args.action, 'project': str(project),
                'pluginDirectory': str(plugin), 'receipt': str(receipt_path), 'changed': True}
    except BaseException as error:
        receipt.update(status='failed', errorType=type(error).__name__, message=str(error),
                       requiresReview=bool(receipt['completedWrites']), finishedAtUtc=datetime.now(timezone.utc).isoformat())
        write_receipt(receipt_path, receipt)
        error.installation_receipt = str(receipt_path)
        error.installation_requires_review = receipt['requiresReview']
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--action', choices=['check', 'bind', 'rollback', 'prepare-editor', 'editor-commands', 'close-editor'], default='check')
    parser.add_argument('--installation-root', required=True)
    parser.add_argument('--project', required=True)
    parser.add_argument('--engine-root', required=True)
    parser.add_argument('--read-root', action='append')
    parser.add_argument('--report-directory')
    parser.add_argument('--receipt')
    parser.add_argument('--python', help='Existing fixed SDK Python interpreter; never installed automatically')
    parser.add_argument('--call-id', help='One new stop call identity used by editor-commands only')
    parser.add_argument('--timeout-seconds', type=float, default=30)
    args = parser.parse_args()
    try:
        action = {'prepare-editor': editor_preparation, 'editor-commands': editor_commands, 'close-editor': close_editor}.get(args.action, prepare)
        result = action(args)
    except Exception as error:
        print(json.dumps({'status': 'requiresReview' if getattr(error, 'installation_requires_review', False) else 'rejected',
                          'errorType': type(error).__name__, 'message': str(error),
                          'receipt': getattr(error, 'installation_receipt', None)}, ensure_ascii=False))
        return 2
    print(json.dumps(result, ensure_ascii=False))
    return 5 if result['status'] == 'pending' else 0


if __name__ == '__main__':
    raise SystemExit(main())
