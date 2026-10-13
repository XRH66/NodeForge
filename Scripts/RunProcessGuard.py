"""Shared project-owned hidden-process guard; deadlines preserve live work instead of killing it."""
import json
import hashlib
import math
import os
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path


INSTALLATION_FIELDS = {'format', 'version', 'installationRoot', 'project',
                       'pluginDirectory', 'binarySha256', 'engineRoot', 'readRoots'}
DEFAULT_READ_ROOTS = ['/Game/NodeForgeTests', '/Engine/EngineResources',
                      '/Engine/Functions/Engine_MaterialFunctions02']

AI_INSTALLATION_FIELDS = {'format', 'version', 'enabled', 'pluginDirectory', 'engineRoot',
                          'binarySha256', 'bundleId', 'toolContractSha256', 'docsManifestSha256'}


def installation_user_directory(plugin):
    """One user's durable installation identity; never a project or secret override."""
    plugin = installation_path(plugin)
    identity = hashlib.sha256(plugin.as_posix().lower().encode('utf-8')).hexdigest()
    # Profile-level state is shared by packaged MCP clients and unpackaged UE.
    # MSIX redirects newly created AppData files into a client-private cache.
    return installation_path(Path(os.environ['USERPROFILE']) / 'NodeForge/Installations' / identity)


def read_ai_installation(path, plugin):
    path, plugin = installation_path(path), installation_path(plugin)
    if path != installation_user_directory(plugin) / 'Installation.json':
        raise ValueError('aiInstallationScope: use this user and exact installed plugin')
    from NodeForgeEditorTransport import Windows, strict_json
    value = strict_json(Windows().bounded_public_file(path, 65536, require_user_owner=True))
    if (set(value) != AI_INSTALLATION_FIELDS or value['format'] != 'NodeForgeAiInstallation1'
            or type(value['version']) is not int or value['version'] != 1
            or type(value['enabled']) is not bool or installation_path(value['pluginDirectory']) != plugin):
        raise ValueError('aiInstallationInvalid')
    return value


def load_ai_installation(path, plugin):
    value = read_ai_installation(path, plugin)
    _, _, _, _, product = installation_product(plugin, value['engineRoot'])
    if value['binarySha256'] != product['binarySha256']:
        raise ValueError('aiInstallationCoreChanged: reinstall matching addon after a recorded upgrade')
    return value


def discover_editor_sessions(plugin, engine):
    """Bounded metadata discovery; public indexes never authorize a business call."""
    plugin, engine = installation_path(plugin), installation_path(engine)
    directory = installation_user_directory(plugin) / 'Sessions'
    installation_path(directory)
    candidates, rejected = [], []
    if not directory.exists():
        return candidates, rejected
    entries = sorted(directory.glob('*.json'))
    if len(entries) > 128:
        raise ValueError('sessionDiscoveryLimit: inspect current installation index')
    for path in entries:
        try:
            identity = installation_json(path)
            session = identity.get('controlSessionId', '')
            if len(session) != 32 or any(c not in '0123456789abcdef' for c in session) or path.name != session + '.json':
                raise ValueError('sessionIndexIdentity')
            observation = process_identity(identity['pid'])
            if observation.get('state') in ('absent', 'exited') or (
                    observation.get('state') == 'running' and observation.get('createdTicks') != identity['processStart']):
                rejected.append({'session': session, 'code': 'sessionClosed'})
                continue  # Preserve historical/unknown records; no lock or credential deletion.
            if observation.get('state') != 'running':
                raise ValueError('sessionProcessUnknown')
            project = installation_path(identity['project'])
            binding = load_installation_binding(project, plugin, plugin)
            if installation_path(binding['engineRoot']) != engine:
                raise ValueError('sessionEngineMismatch')
            binary = plugin / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'
            actual = observed_module(identity['pid'], binary.name)
            state = state_directory(project)
            report = installation_path(identity['reportRoot'])
            if (not report.is_relative_to(state / 'Reports/PluginControlStageTwo')
                    or installation_path(identity['module']) != binary or actual is None or actual.resolve() != binary
                    or identity['binarySha256'] != binding['binarySha256']
                    or installation_json(report / 'SessionPublic.json') != identity
                    or installation_path(identity['sessionFile']) != state / 'Reports/PluginControlSessions' / session / 'Session.json'
                    or not Path(identity['sessionFile']).is_file()):
                raise ValueError('sessionIndexMismatch')
            candidates.append({'project': project.as_posix(), 'controlSessionId': session,
                'pid': identity['pid'], 'processStart': identity['processStart'],
                'binarySha256': identity['binarySha256'], 'config': (report / 'Config.json').as_posix()})
        except (ValueError, OSError, KeyError, TypeError) as error:
            rejected.append({'index': path.name, 'code': str(error)[:160]})
    return candidates, rejected


def installation_path(value):
    """Reject traversal/reparse before resolving; do not turn a junction into permission."""
    raw = str(value).replace('\\', '/')
    path = Path(raw)
    if not path.is_absolute() or any(part in {'.', '..'} for part in raw.split('/')):
        raise ValueError('Explicit absolute installation path without traversal required')
    for item in [path, *path.parents]:
        try:
            info = item.lstat()
        except FileNotFoundError:
            continue
        if item.is_symlink() or getattr(info, 'st_file_attributes', 0) & 0x400:
            raise ValueError('Installation reparse path rejected')
    return path.resolve()


def installation_json(path, limit=65536):
    path = installation_path(path)
    if not path.is_file() or not 0 < path.stat().st_size <= limit:
        raise ValueError('Existing bounded installation JSON required')

    def unique(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError('Duplicate installation JSON field')
            result[key] = value
        return result

    data = json.loads(path.read_text(encoding='utf-8-sig'), object_pairs_hook=unique,
                      parse_constant=lambda _: (_ for _ in ()).throw(ValueError('Nonfinite installation JSON')))
    if not isinstance(data, dict):
        raise ValueError('Installation JSON object required')
    return data


def validate_read_roots(values, project, engine):
    if (not isinstance(values, list) or not 1 <= len(values) <= 32
            or not all(isinstance(value, str) for value in values) or len(values) != len(set(values))):
        raise ValueError('One to 32 unique explicit read roots required')
    for value in values:
        if (not isinstance(value, str) or not 0 < len(value) <= 256
                or not value.startswith(('/Game/', '/Engine/'))
                or any(c in value for c in '\\:.?*"<>| ,\t\r\n')
                or any(part in {'', '.', '..'} for part in value[1:].split('/'))):
            raise ValueError('Canonical registered Game/Engine content subdirectory required')
        mount, _, remainder = value[1:].partition('/')
        content = Path(project).parent / 'Content' if mount == 'Game' else Path(engine) / 'Engine/Content'
        mapped = installation_path(content / remainder)
        if not mapped.is_relative_to(content.resolve()):
            raise ValueError('Read root outside its native content mount')
    return values


def installation_product(root, engine):
    root, engine = installation_path(root), installation_path(engine)
    manifest = installation_json(root / 'InstallationManifest.json')
    required = {'product', 'buildId', 'engineVersion', 'pluginDirectory', 'binarySha256',
                'moduleManifestSha256', 'pluginDescriptorSha256', 'installationSchemaVersion'}
    if (not required <= manifest.keys() or manifest['product'] != 'NodeForge'
            or type(manifest['installationSchemaVersion']) is not int or manifest['installationSchemaVersion'] != 1
            or not isinstance(manifest['buildId'], str) or not manifest['buildId']):
        raise ValueError('NodeForge product installation manifest schema/identity required')
    relative = manifest['pluginDirectory']
    copied_layout = manifest.get('installLayout') in ('projectPlugin', 'portablePlugin') and relative == '.'
    if not copied_layout and (not isinstance(relative, str) or Path(relative).is_absolute() or ':' in relative
            or any(part in {'.', '..'} for part in relative.replace('\\', '/').split('/'))):
        raise ValueError('Product-relative plugin directory required')
    plugin = root if copied_layout else installation_path(root / relative)
    if not plugin.is_relative_to(root) or (plugin == root and not copied_layout) or plugin.name != 'NodeForge':
        raise ValueError('Selected product-relative NodeForge stage required')
    binary = installation_path(plugin / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll')
    modules = installation_path(plugin / 'Binaries/Win64/UnrealEditor.modules')
    descriptor = installation_path(plugin / 'NodeForge.uplugin')
    for path, field in [(binary, 'binarySha256'), (modules, 'moduleManifestSha256'), (descriptor, 'pluginDescriptorSha256')]:
        if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != manifest[field]:
            raise ValueError('Product file SHA mismatch: ' + field)
    plugin_modules = installation_json(modules)
    engine_modules = installation_json(engine / 'Engine/Binaries/Win64/UnrealEditor.modules', 262144)
    version = installation_json(engine / 'Engine/Build/Build.version')
    if any(type(version.get(key)) is not int for key in ['MajorVersion', 'MinorVersion', 'PatchVersion']):
        raise ValueError('Engine version fields must be integers')
    version_text = '.'.join(str(version[key]) for key in ['MajorVersion', 'MinorVersion', 'PatchVersion'])
    if (manifest['engineVersion'] != version_text or version_text != '5.3.2'
            or not plugin_modules.get('BuildId') or plugin_modules['BuildId'] != engine_modules.get('BuildId')
            or plugin_modules.get('Modules') != {'NodeForgeEditor': binary.name}
            or not (engine / 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe').is_file()):
        raise ValueError('Official UE5.3.2 engine/module identity mismatch')
    return root, engine, plugin, binary, manifest


def load_installation_binding(project, plugin=None, root=None):
    project = installation_path(project)
    if project.suffix != '.uproject' or not project.is_file():
        raise ValueError('Explicit existing selected uproject required')
    config = installation_json(project.parent / 'Config/NodeForgeInstallation.json')
    if (set(config) != INSTALLATION_FIELDS or config['format'] != 'NodeForgeInstallationBinding'
            or type(config['version']) is not int or config['version'] != 1
            or installation_path(config['project']) != project):
        raise ValueError('Selected project installation binding required')
    actual_root, engine, actual_plugin, binary, manifest = installation_product(config['installationRoot'], config['engineRoot'])
    if (installation_path(config['pluginDirectory']) != actual_plugin or config['binarySha256'] != manifest['binarySha256']
            or (plugin is not None and installation_path(plugin) != actual_plugin)
            or (root is not None and installation_path(root) != actual_root)):
        raise ValueError('CLI/project/product plugin-root identity mismatch')
    validate_read_roots(config['readRoots'], project, engine)
    descriptor = installation_json(project)
    entries = descriptor.get('Plugins', [])
    if not isinstance(entries, list) or any(not isinstance(entry, dict) for entry in entries):
        raise ValueError('Project plugin references must be an array of objects')
    nodeforge = [entry for entry in entries if isinstance(entry, dict) and entry.get('Name') == 'NodeForge']
    directories = descriptor.get('AdditionalPluginDirectories', [])
    if (len(nodeforge) != 1 or nodeforge[0].get('Enabled') is not True
            or not isinstance(directories, list) or not all(isinstance(value, str) for value in directories)):
        raise ValueError('Exactly one enabled NodeForge project reference required')
    layout = installation_layout(project, engine, actual_plugin)
    copied = layout != 'externalStage'
    if layout.startswith('engine') and manifest.get('installLayout') != 'portablePlugin':
        raise ValueError('Engine install requires a portablePlugin release')
    if layout == 'externalStage' and manifest.get('pluginDirectory') == '.':
        raise ValueError('Copied package must occupy an exact supported installation location')
    selections = 0
    for directory in directories:
        selected = installation_path(project.parent / directory)
        if selected == actual_plugin.parent:
            selections += 1
        elif (selected / 'NodeForge/NodeForge.uplugin').exists():
            raise ValueError('Duplicate NodeForge in another plugin directory')
    locations = (project.parent / 'Plugins/NodeForge', engine / 'Engine/Plugins/NodeForge',
                 engine / 'Engine/Plugins/Marketplace/NodeForge')
    if (selections != (0 if copied else 1)
            or any(location.resolve() != actual_plugin and (location / 'NodeForge.uplugin').exists() for location in locations)):
        raise ValueError('Ambiguous NodeForge installation rejected')
    return config


def installation_layout(project, engine, plugin):
    """Classify exact supported locations; external stages still require explicit selection."""
    plugin = installation_path(plugin)
    for name, location in (
            ('projectPlugin', Path(project).parent / 'Plugins/NodeForge'),
            ('enginePlugin', Path(engine) / 'Engine/Plugins/NodeForge'),
            ('engineMarketplace', Path(engine) / 'Engine/Plugins/Marketplace/NodeForge')):
        if plugin == installation_path(location):
            return name
    return 'externalStage'


def state_directory(project):
    """Shared writable root, independent of the program installation location."""
    project = installation_path(project)
    if project.suffix != '.uproject' or not project.is_file():
        raise ValueError('Explicit existing selected uproject required for state')
    return installation_path(project.parent / 'Saved/NodeForge')


def validate_isolated_project(project, plugin, root):
    """Original prelaunch entry now consumes the explicit selected-project binding."""
    config = load_installation_binding(project, plugin, root)
    project, plugin = installation_path(config['project']), installation_path(config['pluginDirectory'])
    binary = installation_path(plugin / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll')
    return project, plugin, binary


def validate_terminal_step(out, name, step, binary, project):
    """Require preserved OS terminal/module evidence, not merely a native passed field."""
    out, binary, project = Path(out), Path(binary).resolve(), Path(project).resolve()
    started = json.loads((out / (name + 'Started.json')).read_text('utf-8-sig'))
    ended = json.loads((out / (name + 'Exit.json')).read_text('utf-8-sig'))
    loaded = json.loads((out / (name + 'LoadedModule.json')).read_text('utf-8-sig'))
    pid = step.get('pid')
    command = started.get('command', [])
    if (step.get('step') != name or step.get('passed') is not True
            or type(pid) is not int or pid <= 0 or type(step.get('exitCode')) is not int or step['exitCode'] != 0
            or started.get('pid') != pid or ended.get('pid') != pid or ended.get('exitCode') != 0
            or ended.get('terminatedByRunner') is not False or not ended.get('finishedAtUtc')
            or len(command) < 2 or Path(command[1]).resolve() != project
            or '-NFReport=' + str(out / (name + '.json')) not in command
            or ended.get('actualLoadedModule') != loaded or loaded.get('pid') != pid
            or loaded.get('matchesExpected') is not True
            or Path(loaded.get('path', '')).resolve() != binary
            or loaded.get('diskFileSha256') != hashlib.sha256(binary.read_bytes()).hexdigest()):
        raise ValueError('Completed exact child PID, exit zero, selected project and actual module evidence required')
    if (out / 'LiveProcess.json').exists():
        raise ValueError('Live-process checkpoint must be reconciled before resume')
    return pid


class LiveProcessPending(SystemExit):
    def __init__(self, process, evidence, reason=''):
        self.process = process
        self.evidence = evidence
        location = 'checkpoint saved at ' + str(evidence) if evidence else 'checkpoint unavailable; command=' + repr(process.args)
        super().__init__('Process still active; PID=' + str(process.pid) + '; ' + location + '; ' + reason + '; inspect this PID, do not restart the batch')


def observed_module(pid, basename):
    """Read one module's actual Windows path; retain no unrelated module inventory."""
    import ctypes
    from ctypes import wintypes
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    psapi = ctypes.WinDLL('psapi', use_last_error=True)
    kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    kernel.OpenProcess.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    psapi.EnumProcessModulesEx.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.HMODULE), wintypes.DWORD, ctypes.POINTER(wintypes.DWORD), wintypes.DWORD]
    psapi.GetModuleFileNameExW.argtypes = [wintypes.HANDLE, wintypes.HMODULE, wintypes.LPWSTR, wintypes.DWORD]
    handle = kernel.OpenProcess(0x0410, False, pid)  # Read/query only, no termination access.
    if not handle:
        return None
    try:
        modules = (wintypes.HMODULE * 2048)()
        needed = wintypes.DWORD()
        if not psapi.EnumProcessModulesEx(handle, modules, ctypes.sizeof(modules), ctypes.byref(needed), 3) or needed.value > ctypes.sizeof(modules):
            return None
        buffer = ctypes.create_unicode_buffer(32768)
        for module in modules[:needed.value // ctypes.sizeof(wintypes.HMODULE)]:
            if psapi.GetModuleFileNameExW(handle, module, buffer, len(buffer)):
                path = Path(buffer.value)
                if path.name.casefold() == basename.casefold():
                    return path
        return None
    finally:
        kernel.CloseHandle(handle)


def process_identity(pid):
    """Read Windows creation ticks and liveness; PID alone is never an identity."""
    import ctypes
    from ctypes import wintypes
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    kernel.OpenProcess.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    kernel.GetProcessTimes.argtypes = [wintypes.HANDLE, *([ctypes.POINTER(wintypes.FILETIME)] * 4)]
    kernel.WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]
    kernel.WaitForSingleObject.restype = wintypes.DWORD
    handle = kernel.OpenProcess(0x101000, False, pid)  # Query limited + synchronize, no write/kill.
    if not handle:
        error = ctypes.get_last_error()
        return {'state': 'absent' if error == 87 else 'unknown', 'windowsError': error}
    try:
        times = [wintypes.FILETIME() for _ in range(4)]
        if not kernel.GetProcessTimes(handle, *(ctypes.byref(value) for value in times)):
            return {'state': 'unknown', 'windowsError': ctypes.get_last_error()}
        ticks = (times[0].dwHighDateTime << 32) | times[0].dwLowDateTime
        wait = kernel.WaitForSingleObject(handle, 0)
        return {'state': 'running' if wait == 258 else 'exited' if wait == 0 else 'unknown',
                'createdTicks': str(ticks)}
    finally:
        kernel.CloseHandle(handle)


def run_guarded(command, *, cwd, console, timeout, expected_module=None, observer=None):
    """Return a terminated Popen, or stop the caller with an owned live-PID checkpoint.

    No Popen context manager and no subprocess.run timeout: neither deadline nor
    interruption terminates a potentially saving engine process.
    An optional trusted internal observer shares this same live-error guard;
    it never comes from graph input and must bound its own read-only probes.
    """
    if not math.isfinite(timeout) or timeout <= 0:
        raise ValueError('Positive finite timeout required')
    if observer is not None and not callable(observer):
        raise ValueError('Internal observer must be callable')
    expected_module = Path(expected_module).resolve() if expected_module else None
    expected_sha = hashlib.sha256(expected_module.read_bytes()).hexdigest() if expected_module else None
    console = Path(console)
    name = console.name.removesuffix('Console.txt')
    if not name or name == console.name:
        raise ValueError('Named fresh Console.txt path required')

    def write_new(path, data):
        with path.open('x', encoding='utf-8', newline='\n') as stream:
            json.dump(data, stream, indent=2, ensure_ascii=False, allow_nan=False)
            stream.write('\n')

    command_path = console.parent / (name + 'Command.json')
    if command_path.exists():
        if json.loads(command_path.read_text(encoding='utf-8-sig')) != command:
            raise ValueError('Existing command differs; historical evidence protected')
    else:
        write_new(command_path, command)
    started_path = console.parent / (name + 'Started.json')
    if started_path.exists() or (console.parent / 'LiveProcess.json').exists():
        raise ValueError('Existing process checkpoint protected; reconcile the prior PID first')
    with console.open('xb') as stream:
        process = subprocess.Popen(command, cwd=cwd, stdout=stream, stderr=subprocess.STDOUT,
            creationflags=subprocess.CREATE_NO_WINDOW)
        started = {'pid': process.pid, 'command': command, 'cwd': str(cwd),
            'startedAtUtc': datetime.now(timezone.utc).isoformat(), 'timeoutSeconds': timeout,
            'processIdentity': process_identity(process.pid)}
        live_path = console.parent / 'LiveProcess.json'
        loaded = None

        def checkpoint(reason):
            if not live_path.exists():
                write_new(live_path, {**started, 'reason': reason, 'observedAtUtc': datetime.now(timezone.utc).isoformat(),
                    'exitCode': process.poll(), 'terminatedByRunner': False,
                    'actualLoadedModule': loaded,
                    'nextAction': 'Observe this PID and preserved output; no new batch or source/build mutation until reconciled'})
            return live_path

        try:
            write_new(started_path, started)
            deadline = time.monotonic() + timeout
            while process.poll() is None:
                if observer is not None:
                    observer(process)
                if expected_module and loaded is None:
                    path = observed_module(process.pid, expected_module.name)
                    if path:
                        loaded = {'path': str(path), 'diskFileSha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                            'observedAtUtc': datetime.now(timezone.utc).isoformat(), 'pid': process.pid}
                        loaded['matchesExpected'] = str(path.resolve()).casefold() == str(expected_module).casefold() and loaded['diskFileSha256'] == expected_sha
                        write_new(console.parent / (name + 'LoadedModule.json'), loaded)
                        if not loaded['matchesExpected']:
                            raise LiveProcessPending(process, checkpoint('actual loaded module differs; stop the batch without killing process'))
                if time.monotonic() >= deadline:
                    raise LiveProcessPending(process, checkpoint('deadline reached while process remains live'))
                time.sleep(min(.2, max(.01, deadline - time.monotonic())))
        except BaseException as exc:
            if process.poll() is None:
                try:
                    evidence = checkpoint('caller stopped while process remains live: ' + type(exc).__name__)
                except OSError as write_error:
                    raise LiveProcessPending(process, None, 'checkpoint write failed: ' + type(write_error).__name__) from exc
                if not isinstance(exc, LiveProcessPending):
                    raise LiveProcessPending(process, evidence, 'live error: ' + type(exc).__name__) from exc
            else:
                # The observer can fail after the child naturally finishes. Keep
                # its real terminal facts without creating a reusable success
                # Exit.json or pretending that exit zero means runner success.
                try:
                    write_new(console.parent / (name + 'FailedTerminal.json'), {
                        'pid': process.pid, 'exitCode': process.returncode,
                        'finishedAtUtc': datetime.now(timezone.utc).isoformat(),
                        'terminatedByRunner': False, 'runnerStatus': 'failed',
                        'runnerErrorType': type(exc).__name__, 'actualLoadedModule': loaded})
                except OSError as write_error:
                    raise SystemExit('Owned child already terminal; PID=' + str(process.pid)
                        + '; exitCode=' + str(process.returncode)
                        + '; failed terminal receipt unavailable: ' + type(write_error).__name__
                        + '; original runner error: ' + type(exc).__name__) from exc
            raise
    write_new(console.parent / (name + 'Exit.json'), {'pid': process.pid, 'exitCode': process.returncode,
        'finishedAtUtc': datetime.now(timezone.utc).isoformat(), 'terminatedByRunner': False,
        'actualLoadedModule': loaded})
    if expected_module and (loaded is None or not loaded['matchesExpected']):
        raise SystemExit('No matching actual loaded module observed; terminal process evidence retained, do not claim DLL verified')
    return process
