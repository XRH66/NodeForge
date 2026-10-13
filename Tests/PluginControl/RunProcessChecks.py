"""Bounded real child/UE timeout and read-only identity checks; never kill a child."""
import argparse
import ctypes
import json
import subprocess
import sys
import time
from ctypes import wintypes
from pathlib import Path

from RunCases import ROOT, run_client
from NodeForgeControl import digest, read_json, write_new
from RunGraphRegression import ASSET_SUFFIXES, files_under
from RunProcessGuard import LiveProcessPending, observed_module, process_identity, run_guarded


def query_status(run):
    result = subprocess.run([sys.executable, str(ROOT / 'Scripts/NodeForgeControl.py'), 'status', '--run-directory', str(run)],
                            capture_output=True, timeout=20, creationflags=subprocess.CREATE_NO_WINDOW)
    return result.returncode, json.loads(result.stdout.decode('utf-8'))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('project', 'plugin-directory', 'report-directory'):
        p.add_argument('--' + name, required=True)
    args = p.parse_args(); out = Path(args.report_directory).resolve()
    if not out.is_relative_to(ROOT / 'Reports'): raise SystemExit('Fresh Reports directory required')
    out.mkdir(parents=True, exist_ok=False)
    checks = {}; children = []
    for name, fail_observer in [('GuardDeadline', False), ('GuardObserver', True)]:
        folder = out / name; folder.mkdir()
        def observer(process): raise RuntimeError('Intentional trusted read observer failure')
        try:
            run_guarded([sys.executable, '-c', 'import time;time.sleep(1)'], cwd=ROOT,
                        console=folder / 'ChildConsole.txt', timeout=.05 if not fail_observer else 3,
                        observer=observer if fail_observer else None)
            raise AssertionError('Guard did not stop with live checkpoint')
        except LiveProcessPending as error:
            actual = process_identity(error.process.pid); live = read_json(error.evidence)
            preserved = error.process.poll() is None and live['terminatedByRunner'] is False
            matches = actual.get('createdTicks') == live['processIdentity'].get('createdTicks') and bool(actual.get('createdTicks'))
            code = error.process.wait(timeout=4)
            children.append({'case': name, 'pid': error.process.pid, 'liveBeforeNaturalExit': preserved,
                             'actualCreationIdentityMatched': matches, 'observedNaturalExitCode': code})
            checks[name] = preserved and matches and code == 0
    project = Path(args.project).resolve(); plugin = Path(args.plugin_directory).resolve()
    before = files_under(project.parent / 'Content', ASSET_SUFFIXES)
    run = out / 'UeDeadline'
    process, summary = run_client(['describe', '--project', str(project), '--plugin-directory', str(plugin),
                                  '--run-directory', str(run), '--timeout-seconds', '.01'], out / 'DeadlineClientConsole.txt', 30)
    checks['officialCliDeadlinePending'] = process.returncode == 5 and summary.get('executionState') == 'pending'
    live = read_json(run / 'LiveProcess.json'); pid = live['pid']
    identity = process_identity(pid)
    checks['ueStillLiveAndCreationRecorded'] = identity.get('state') == 'running' and bool(identity.get('createdTicks')) and identity.get('createdTicks') == live['processIdentity'].get('createdTicks')
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]; kernel.OpenProcess.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    kernel.WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]; kernel.WaitForSingleObject.restype = wintypes.DWORD
    kernel.GetExitCodeProcess.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD)]
    handle = kernel.OpenProcess(0x101000, False, pid)
    if not handle: raise SystemExit('UE timeout checkpoint remains unconfirmed; preserve run and stop')
    status_rows = []
    try:
        # A fresh poisoned copy tests PID reuse handling without editing actual receipts.
        wrong = out / 'WrongCreationIdentity'; wrong.mkdir()
        copy = json.loads(json.dumps(live)); copy['processIdentity']['createdTicks'] = str(int(identity['createdTicks']) + 1)
        write_new(wrong / 'LiveProcess.json', copy)
        write_new(wrong / 'Summary.json', {**summary, 'run': str(wrong)})
        code, data = query_status(wrong)
        status_rows.append({'case': 'wrongCreationTicks', 'cliExitCode': code, 'result': data})
        checks['wrongCreationIsUnknown'] = code == 5 and data['executionState'] == 'unknown'
        deadline = time.monotonic() + 20
        while kernel.WaitForSingleObject(handle, 0) == 258 and time.monotonic() < deadline:
            if observed_module(pid, 'UnrealEditor-NodeForgeEditor.dll'): break
            time.sleep(.1)
        original_sha = digest(run / 'Summary.json')
        code, data = query_status(run)
        status_rows.append({'case': 'actualOldRun', 'cliExitCode': code, 'result': data})
        checks['statusDoesNotInventCompletion'] = code == 5 and data['executionState'] in ('pending', 'unknown')
        checks['statusIsReadOnly'] = original_sha == digest(run / 'Summary.json')
        wait = kernel.WaitForSingleObject(handle, 20000)
        if wait != 0: raise SystemExit('UE still pending; preserve run and stop, no next writes')
        exit_code = wintypes.DWORD()
        if not kernel.GetExitCodeProcess(handle, ctypes.byref(exit_code)): raise SystemExit('Cannot observe held UE handle exit')
        write_new(out / 'UeObservedExit.json', {'pid': pid, 'processIdentity': identity, 'exitCode': exit_code.value,
                  'terminatedByRunner': False, 'source': 'held query-only Windows process handle',
                  'note': 'Original guard timed out; this separate read-only observer is not a fabricated NativeExit receipt.'})
        checks['ueNaturallyFinished'] = exit_code.value == 0
        late = read_json(run / 'Native.json')
        checks['lateRealNativeDescriptor'] = late.get('pid') == pid and late.get('operation') == 'operations.describe' and late.get('environment', {}).get('moduleSha256') == digest(plugin / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll')
        code, data = query_status(run); status_rows.append({'case': 'terminalWithoutGuardExit', 'cliExitCode': code, 'result': data})
        checks['missingGuardExitStaysUnknown'] = code == 5 and data['executionState'] == 'unknown'
    finally:
        kernel.CloseHandle(handle)
    checks['timeoutWasReadOnly'] = before == files_under(project.parent / 'Content', ASSET_SUFFIXES)
    final = {'status': 'passed' if all(checks.values()) else 'failed', 'checks': checks, 'children': children, 'statusQueries': status_rows,
             'ueRun': str(run), 'originalLiveCheckpointPreserved': True, 'nextAction': 'Timeout child naturally exited; original status remains unknown because guard exit was not obtained. No blind retry.'}
    write_new(out / 'Checks.json', final); print(json.dumps({'status': final['status'], 'checks': checks}), flush=True)
    return 0 if final['status'] == 'passed' else 1


if __name__ == '__main__':
    sys.exit(main())
