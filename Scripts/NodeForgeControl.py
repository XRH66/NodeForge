"""Official local Control API 1.0 client. Stdout is one JSON result; UE logs stay in run files."""
import argparse
import ctypes
import hashlib
import json
import math
import subprocess
import sys
import time
from pathlib import Path

from RunProcessGuard import (LiveProcessPending, observed_module, process_identity,
                             run_guarded, validate_isolated_project, load_installation_binding)
from RunProcessGuard import state_directory, installation_path

ROOT = Path(__file__).resolve().parent.parent
API = '1.0'
REPORT_LIMIT = 1024 * 1024


class ControlError(Exception):
    def __init__(self, code, message, field='', exit_code=2):
        self.code, self.message, self.field, self.exit_code = code, message, field, exit_code
        super().__init__(message)


class Parser(argparse.ArgumentParser):
    def error(self, message):
        raise ControlError('NFCLI_ARGUMENT', message, 'arguments')


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def read_json(path, limit=REPORT_LIMIT):
    path = Path(path)
    if not path.is_file() or not 0 < path.stat().st_size <= limit:
        raise ControlError('NFCLI_FILE', 'Existing bounded file required: ' + str(path), str(path))
    return json.loads(path.read_text('utf-8-sig'), parse_constant=lambda x: (_ for _ in ()).throw(ValueError(x)))


def write_new(path, data):
    text = json.dumps(data, ensure_ascii=False, indent=2, allow_nan=False) + '\n'
    if len(text.encode('utf-8')) > REPORT_LIMIT:
        raise ControlError('NFCLI_REPORT_LIMIT', 'Summary exceeds 1 MiB', str(path), 3)
    with Path(path).open('x', encoding='utf-8', newline='\n') as stream:
        stream.write(text)


def scoped_path(value, directory, field):
    path = Path(value).resolve()
    if not path.is_relative_to(directory.resolve()):
        raise ControlError('NFCLI_PATH', 'Path must belong to ' + str(directory), field)
    return path


def command_arguments(text):
    """Decode Windows command lines using the OS, including quoted paths."""
    shell = ctypes.WinDLL('shell32', use_last_error=True)
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    shell.CommandLineToArgvW.argtypes = [ctypes.c_wchar_p, ctypes.POINTER(ctypes.c_int)]
    shell.CommandLineToArgvW.restype = ctypes.POINTER(ctypes.c_wchar_p)
    kernel.LocalFree.argtypes = [ctypes.c_void_p]
    kernel.LocalFree.restype = ctypes.c_void_p
    count = ctypes.c_int()
    values = shell.CommandLineToArgvW(text, ctypes.byref(count))
    if not values:
        raise ControlError('NFCLI_PROCESS_QUERY', 'Cannot decode OS command line', 'project', 3)
    try:
        return [values[i] for i in range(count.value)]
    finally:
        kernel.LocalFree(values)


def engine_processes():
    # One native read-only worker bounds the entire discovery, independently of
    # the UE operation timeout. All CLI/editor/MCP callers share this function.
    started = time.monotonic()
    diagnostic = {'method': 'Toolhelp32+winternl-x64', 'budgetSeconds': 4}
    engine_processes.last_diagnostic = diagnostic
    try:
        result = subprocess.run([sys.executable, str(Path(__file__).with_name('NodeForgeWindowsProcesses.py'))],
                                capture_output=True, text=True, encoding='utf-8', timeout=4,
                                creationflags=subprocess.CREATE_NO_WINDOW)
        if len(result.stdout) > REPORT_LIMIT:
            raise ValueError('Process snapshot exceeds report bound')
        data = json.loads(result.stdout)
        if not isinstance(data, dict):
            raise ValueError('Invalid native process snapshot')
        diagnostic.update(data)
        if result.returncode or data.get('status') != 'succeeded':
            raise ValueError(json.dumps(data.get('error', {'stage': 'worker', 'exitCode': result.returncode})))
        items = data.get('processes')
        if not isinstance(items, list) or any(not isinstance(x, dict) or type(x.get('ProcessId')) is not int
                or not isinstance(x.get('CommandLine'), str) or not x['CommandLine'] for x in items):
            raise ValueError('Incomplete UE process identity')
        return items
    except (OSError, ValueError, subprocess.TimeoutExpired) as error:
        diagnostic.update(status='failed', reason=type(error).__name__ + ': ' + str(error))
        raise ControlError('NFCLI_PROCESS_QUERY', 'Cannot establish exclusive project: ' + diagnostic['reason']
                           + '; inspect processQuery; close no UE processes automatically', 'project', 3) from error
    finally:
        diagnostic['elapsedSeconds'] = time.monotonic() - started


def process_uses_project(item, project):
    text = item.get('CommandLine')
    if not text:
        raise ControlError('NFCLI_PROCESS_QUERY', 'UE process command line unavailable', 'project', 3)
    paths = []
    for arg in command_arguments(text)[1:]:
        if arg.startswith('-') and not arg.lower().startswith('-project='):
            continue  # Other option values (for example -log=...uproject) are not project arguments.
        value = arg.split('=', 1)[1] if arg.lower().startswith('-project=') else arg
        if value.lower().endswith('.uproject'):
            path = Path(value)
            if not path.is_absolute():
                raise ControlError('NFCLI_PROCESS_QUERY', 'Cannot resolve relative uproject from another process: '
                                   + value + '; use an absolute project argument', 'project', 3)
            paths.append(path.resolve())
    if len(set(paths)) > 1:
        raise ControlError('NFCLI_PROCESS_QUERY', 'UE command line names conflicting projects', 'project', 3)
    return project.resolve() in paths


def assert_exclusive(project):
    if any(process_uses_project(item, project) for item in engine_processes()):
        raise ControlError('NFCLI_PROJECT_BUSY', 'Another UE process uses this project; inspect it before launch', 'project', 3)


def native_call(operation, params, project, binary, out, timeout):
    assert_exclusive(project)
    binding = load_installation_binding(project, binary.parent.parent.parent, ROOT)
    engine = Path(binding['engineRoot']) / 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
    report = out / 'Native.json'
    command = [str(engine), str(project), '-run=NodeForge', '-NFOperation=' + operation,
               '-NFControlApiVersion=' + API, '-NFReport=' + str(report)]
    command += ['-' + key + '=' + str(value) for key, value in params.items()]
    # UE5.3 does not start registry discovery for an unlisted commandlet.
    # Native resource admission needs the same disk metadata as the editor;
    # start the engine's normal index, without loading objects or changing guards.
    if operation in ('graphs.generateFromFile', 'graphs.preflightFromFile',
                     'graphs.verifySaved', 'resources.query'):
        command += ['-AssetGatherAll=true']
    command += ['-d3d11', '-RenderOffscreen', '-AllowCommandletRendering', '-unattended',
                '-nop4', '-nosound', '-NoLiveCoding', '-abslog=' + str(out / 'NativeLog.txt'),
                '-stdout', '-FullStdOutLogOutput']
    process = run_guarded(command, cwd=ROOT, console=out / 'NativeConsole.txt', timeout=timeout, expected_module=binary)
    if not report.is_file():
        raise ControlError('NFCLI_MISSING_REPORT', 'Child terminated without its native report; inspect preserved console/exit', 'nativeReport', 5)
    try:
        data = read_json(report)
        if not isinstance(data, dict):
            raise ValueError('native report root must be an object')
    except (ControlError, OSError, ValueError, UnicodeError) as error:
        raise ControlError('NFCLI_NATIVE_REPORT_UNREADABLE', 'Child ended but its bounded native report cannot be read: ' + str(error), 'nativeReport', 5)
    if data.get('pid') != process.pid:
        raise ControlError('NFCLI_RECEIPT_PID', 'Native report does not identify this child', 'pid', 5)
    return data, process.returncode, process.pid, report


def bound_description(path, binary):
    data = read_json(path)
    if not isinstance(data, dict):
        raise ControlError('NFCLI_DESCRIPTION', 'Runtime description root must be an object', 'description', 3)
    environment = data.get('environment', {})
    if (data.get('apiVersion') != API or data.get('operation') != 'operations.describe'
            or environment.get('moduleSha256') != digest(binary)
            or Path(environment.get('module', '')).resolve() != binary):
        raise ControlError('NFCLI_DESCRIPTION_MISMATCH', 'Description must match actual selected DLL path/SHA and API 1.0', 'description', 3)
    if not isinstance(data.get('operations'), list):
        raise ControlError('NFCLI_DESCRIPTION', 'Runtime operations missing', 'description', 3)
    return data


def summary_base(operation, run):
    return {'apiVersion': API, 'operation': operation, 'run': str(run) if run else None,
            'executionState': 'rejected', 'errors': [], 'warnings': [],
            'validation': {'runtime': {'status': 'notRun'}, 'visual': {'status': 'notRun'},
                           'independentFixedExpectation': {'status': 'notRun'}}}


def execute(args, summary):
    run = scoped_path(args.run_directory, state_directory(args.project) / 'Reports', 'run-directory')
    summary['run'] = str(run)
    summary['project'] = str(installation_path(args.project))
    if run.exists():
        raise ControlError('NFCLI_RUN_EXISTS', 'Existing run protected; use status to inspect it', 'run-directory')
    try:
        run.mkdir(parents=True, exist_ok=False)
    except OSError as error:
        raise ControlError('NFCLI_REPORT_WRITE', 'Cannot create exclusive run: ' + type(error).__name__, 'run-directory', 3)
    summary['_publish'] = True
    if not math.isfinite(args.timeout_seconds) or args.timeout_seconds <= 0:
        raise ControlError('NFCLI_TIMEOUT', 'Positive finite timeout required', 'timeout-seconds')
    try:
        project, plugin, binary = validate_isolated_project(args.project, args.plugin_directory, ROOT)
    except (ValueError, OSError) as error:
        raise ControlError('NFCLI_ENVIRONMENT', str(error), 'project/plugin-directory', 3)
    summary.update(project=str(project), pluginDirectory=str(plugin), moduleSha256=digest(binary))
    operation = summary['operation']
    supplied = {}
    if args.input: supplied['inputPath'] = args.input
    if args.generation_report: supplied['generationReport'] = args.generation_report
    for item in args.param:
        if '=' not in item:
            raise ControlError('NFCLI_PARAMETER', 'Use --param NAME=VALUE', 'param')
        name, value = item.split('=', 1)
        if name in supplied:
            raise ControlError('NFCLI_PARAMETER', 'Duplicate parameter: ' + name, name)
        supplied[name] = value
    write_new(run / 'Request.json', {'operation': operation, 'project': str(project),
              'pluginDirectory': str(plugin), 'moduleSha256': digest(binary), 'parameters': supplied})
    native_params = {}
    if args.action != 'describe':
        if args.description_file:
            description_path = scoped_path(args.description_file, state_directory(project) / 'Reports', 'description-file')
        else:
            describe_run = run / 'Describe'; describe_run.mkdir()
            _, code, _, description_path = native_call('operations.describe', {}, project, binary, describe_run, args.timeout_seconds)
            if code: raise ControlError('NFCLI_DESCRIPTION', 'Runtime description failed', 'description', 3)
        description = bound_description(description_path, binary)
        summary['description'] = {'path': str(description_path), 'sha256': digest(description_path), 'moduleSha256': digest(binary)}
        definitions = [item for item in description['operations'] if item.get('name') == operation]
        if len(definitions) != 1:
            raise ControlError('NFCLI_UNKNOWN_OPERATION', 'Operation absent from selected DLL description', 'operation')
        fields = definitions[0]['parameters']
        allowed = {item['name'] for item in fields}
        if set(supplied) - allowed:
            raise ControlError('NFCLI_PARAMETER', 'Unsupported operation parameters: ' + ', '.join(sorted(set(supplied) - allowed)), 'parameters')
        for field in fields:
            value = supplied.get(field['name'])
            if field.get('required') and not value:
                raise ControlError('NFCLI_MISSING_PARAMETER', 'Required parameter: ' + field['name'], field['name'])
            if not value: continue
            argument = field.get('nativeArgument')
            if argument not in ('NFInput', 'NFPrevious'):
                raise ControlError('NFCLI_DESCRIPTION', 'Unsupported host file binding in description', field['name'], 3)
            path = installation_path(value)
            scopes = (ROOT / 'Examples', ROOT / 'Tests', state_directory(project) / 'Reports', state_directory(project) / 'RelayoutInputs') if argument == 'NFInput' else (state_directory(project) / 'Reports',)
            if not any(path.is_relative_to(scope) for scope in scopes):
                raise ControlError('NFCLI_FILE_SCOPE', 'Selected project state or installed example required', field['name'])
            limit = 65536 if argument == 'NFInput' else REPORT_LIMIT
            if not path.is_file() or not 0 < path.stat().st_size <= limit:
                raise ControlError('NFCLI_FILE', 'Existing bounded file required: ' + str(path), field['name'])
            if argument == 'NFInput':
                raw = path.read_bytes(); snapshot = run / 'Input.json'
                if not 0 < len(raw) <= limit:
                    raise ControlError('NFCLI_FILE', 'Input changed beyond size limit', field['name'])
                with snapshot.open('xb') as stream: stream.write(raw)
                summary.update(inputPath=str(path), inputSnapshot=str(snapshot), inputDigest=digest(snapshot))
                try: summary['requestId'] = json.loads(raw).get('requestId')
                except (ValueError, UnicodeError, AttributeError): summary['requestId'] = None
                path = snapshot
            else:
                read_json(path)
                summary['generationReport'] = {'path': str(path), 'sha256': digest(path)}
            native_params[argument] = path
    elif supplied:
        raise ControlError('NFCLI_PARAMETER', 'Describe takes no business parameters', 'parameters')
    data, native_exit, pid, report = native_call(operation, native_params, project, binary, run, args.timeout_seconds)
    summary.update(pid=pid, nativeReport={'path': str(report), 'sha256': digest(report), 'exitCode': native_exit},
                   nativeOverallStatus=data.get('overallStatus'), nativeOperationStatus=data.get('operationStatus'),
                   residue=data.get('residue'), executionState='finished')
    summary['validation']['nativeChecks'] = {'source': str(report), 'checks': data.get('checks', {})}
    summary['validation']['persistence'] = data.get('checks', {}).get('persistence', {'status': 'notRun'})
    if args.action == 'describe': bound_description(report, binary)
    if native_exit != 0 or data.get('overallStatus') not in ('succeeded', 'waitingPersistence'):
        summary['errors'] = data.get('errors') or [{'code': data.get('errorCode', 'NFCLI_NATIVE_FAILURE'), 'message': data.get('reason') or data.get('nextAction') or 'Native operation failed', 'field': data.get('failedStage', '')}]
        return 3 if data.get('executionState') == 'environmentBlocked' else 4
    return 0


def status(args):
    run = installation_path(args.run_directory)
    prior = read_json(run / 'Summary.json')
    if not isinstance(prior, dict):
        raise ControlError('NFCLI_STATUS_EVIDENCE', 'Saved summary root is not an object; outcome unknown', 'run-directory', 5)
    if not run.is_relative_to(ROOT / 'Reports') and not run.is_relative_to(state_directory(prior.get('project', '')) / 'Reports'):
        raise ControlError('NFCLI_STATUS_SCOPE', 'Original selected project report required', 'run-directory', 5)
    result = {**prior, 'statusQuery': True}
    if (run / 'NativeExit.json').is_file():
        ended = read_json(run / 'NativeExit.json'); started = read_json(run / 'NativeStarted.json')
        loaded = read_json(run / 'NativeLoadedModule.json')
        ref = prior.get('nativeReport', {})
        if (ended.get('pid') == started.get('pid') == loaded.get('pid') == prior.get('pid')
                and ended.get('terminatedByRunner') is False and loaded.get('matchesExpected') is True
                and ended.get('actualLoadedModule') == loaded and ref.get('exitCode') == ended.get('exitCode')
                and ref.get('sha256') == digest(ref.get('path', ''))):
            result['executionState'] = 'finished'
            return result, prior.get('cliExitCode', 5)
        result['executionState'] = 'unknown'
        result['errors'] = [{'code': 'NFCLI_STATUS_EVIDENCE', 'message': 'Terminal evidence does not match this run'}]
        return result, 5
    candidates = list(run.rglob('LiveProcess.json'))
    if len(candidates) != 1:
        # A rejected pre-launch run has no process; preserve that explicit result.
        if not list(run.rglob('*Started.json')):
            return result, prior.get('cliExitCode', 5)
        result['executionState'] = 'unknown'
        result['errors'] = [{'code': 'NFCLI_STATUS_EVIDENCE', 'message': 'Unique live checkpoint unavailable'}]
        return result, 5
    live = read_json(candidates[0]); pid = live['pid']; identity = process_identity(pid)
    result.update(pid=pid, observedProcess=identity, executionState='unknown')
    expected_identity = live.get('processIdentity', {})
    if identity.get('state') == 'running' and identity.get('createdTicks') == expected_identity.get('createdTicks') and identity.get('createdTicks'):
        project = Path(prior['project']).resolve()
        items = [x for x in engine_processes() if x.get('ProcessId') == pid]
        module = observed_module(pid, 'UnrealEditor-NodeForgeEditor.dll')
        binary = Path(prior['pluginDirectory']) / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'
        if len(items) == 1 and process_uses_project(items[0], project) and module and module.resolve() == binary.resolve() and digest(module) == prior['moduleSha256']:
            result['executionState'] = 'pending'
    result['nextAction'] = 'Inspect this preserved run; no restart or automatic cleanup. Missing terminal exit remains unknown.'
    return result, 5


def parser():
    p = Parser(description='NodeForge explicit local hosts: isolated Commandlet or already running registered editor.')
    subs = p.add_subparsers(dest='action', required=True, parser_class=Parser)
    for action in ('describe', 'execute'):
        q = subs.add_parser(action)
        for name in ('project', 'plugin-directory', 'run-directory'):
            q.add_argument('--' + name, required=True)
        q.add_argument('--timeout-seconds', type=float, default=240, help='Positive finite child deadline; live UE is preserved on timeout')
        q.add_argument('--input', help='Workspace JSON path; exact bytes snapshotted, maximum 64 KiB')
        q.add_argument('--generation-report', help='Original successful save receipt for independent Verify')
        q.add_argument('--param', action='append', default=[], metavar='NAME=VALUE', help='Generic parameter from actual DLL description')
        q.add_argument('--description-file', help='Optional captured runtime description, accepted only for same actual DLL path/SHA/API')
        if action == 'execute': q.add_argument('--operation', required=True)
    q = subs.add_parser('status'); q.add_argument('--run-directory', required=True)
    for action in ('editor-describe', 'editor-call', 'editor-status'):
        q = subs.add_parser(action)
        for name in ('session-file', 'project', 'plugin-directory', 'run-directory'):
            q.add_argument('--' + name, required=True)
        q.add_argument('--timeout-seconds', type=float, default=180)
        q.add_argument('--mcp-client-info',help='Optional bounded initialize client metadata forwarded by the existing MCP bridge')
        if action != 'editor-describe': q.add_argument('--call-id', required=True)
        if action == 'editor-call':
            q.add_argument('--operation', required=True)
            q.add_argument('--expected-revision')
            q.add_argument('--arguments-file', help='Bounded workspace JSON object; never interpreted as code')
    return p


def main(argv=None):
    if hasattr(sys.stdout, 'reconfigure'):
        sys.stdout.reconfigure(encoding='utf-8', newline='\n')
    summary = summary_base('', None); code = 2
    try:
        args = parser().parse_args(argv)
        if args.action == 'status':
            summary, code = status(args)
        elif args.action.startswith('editor-'):
            from NodeForgeEditorTransport import execute as editor_execute, TransportError
            if args.mcp_client_info:
                if len(args.mcp_client_info.encode('utf-8'))>1024:raise ControlError('NFCLI_CLIENT_IDENTITY','MCP client metadata exceeds bound')
                args.mcp_client=json.loads(args.mcp_client_info)
            try:
                code = editor_execute(args, summary, ROOT, digest, write_new, engine_processes, process_uses_project)
            except TransportError as error:
                raise ControlError(error.code, error.code, 'editor', error.exit_code) from error
        else:
            summary['operation'] = 'operations.describe' if args.action == 'describe' else args.operation
            code = execute(args, summary)
    except LiveProcessPending as error:
        summary.update(executionState='pending', pid=error.process.pid, residue={'diskState': 'unknown'},
                       errors=[{'code': 'NFCLI_PENDING', 'message': str(error)}], liveCheckpoint=str(error.evidence) if error.evidence else None)
        code = 5
    except ControlError as error:
        code = error.exit_code
        summary.update(executionState='environmentBlocked' if code == 3 else 'unknown' if code == 5 else 'rejected',
                       errors=[{'code': error.code, 'field': error.field, 'message': error.message}])
        if code == 5 and summary.get('run'):
            # A terminal report/read failure can occur after a real child wrote assets.
            # Preserve known process facts, never relabel it as a pre-launch rejection.
            for started_path in Path(summary['run']).rglob('NativeStarted.json'):
                try:
                    started = read_json(started_path)
                    summary.update(pid=started['pid'], residue={'diskState': 'unknown'},
                                   processEvidence=str(started_path))
                except (ControlError, OSError, ValueError): pass
    except (OSError, ValueError, KeyError, TypeError) as error:
        code = 3
        summary.update(executionState='environmentBlocked', errors=[{'code': 'NFCLI_ENVIRONMENT', 'message': type(error).__name__ + ': ' + str(error)}])
    except SystemExit as error:
        if error.code == 0: return 0  # argparse help
        code = 5
        summary.update(executionState='unknown', errors=[{'code': 'NFCLI_GUARD_EVIDENCE', 'message': str(error)}])
    summary['cliExitCode'] = code
    if hasattr(engine_processes, 'last_diagnostic'):
        summary['processQuery'] = engine_processes.last_diagnostic
    if summary.pop('_publish', False):
        try: write_new(Path(summary['run']) / 'Summary.json', summary)
        except (OSError, ControlError) as error:
            code = 3; summary['cliExitCode'] = code
            summary['errors'].append({'code': 'NFCLI_SUMMARY_WRITE', 'message': str(error)})
    print(json.dumps(summary, ensure_ascii=False, allow_nan=False))
    return code


if __name__ == '__main__':
    sys.exit(main())
