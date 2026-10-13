"""Actual owned child deadline and strict regression-evidence counterexamples; no Unreal launched."""
import argparse
import hashlib
import importlib.util
import json
import subprocess
import sys
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'Scripts'))
from RunProcessGuard import run_guarded, LiveProcessPending
from RunGraphRegression import subset_errors, automation_report_passed, read_json, write_new, ref, sha

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--report-directory', required=True)
a = p.parse_args()
out = (ROOT / a.report_directory).resolve()
if not out.is_relative_to(ROOT / 'Reports') or out.exists():
    raise SystemExit('Fresh Reports directory required')
out.mkdir()
rows = []

def check(name, good, detail):
    rows.append({'case': name, 'passed': bool(good), 'detail': detail})

baseline_path = ROOT / 'Reports/20261010NightAudit/Baseline/Scripts/RunGraphRegression.py'
spec = importlib.util.spec_from_file_location('NodeForgeBaselineGraphHelpers', baseline_path)
old = importlib.util.module_from_spec(spec)
spec.loader.exec_module(old)
check('OldLargeDoubleGapReproduced', old.subset_errors(999999900000.0, 999999965536.0) == [], 'Old real helper accepts +65536')
check('OldIntegerGapReproduced', old.subset_errors(2147483646, 2147483645) == [], 'Old real helper accepts changed integer')
check('LargeDoubleGapRejected', bool(subset_errors(999999900000.0, 999999965536.0)), 'Repaired bounded double comparison')
check('SmallIntegerGapRejected', bool(subset_errors(2147483646, 2147483645)), 'Integral defaults/configuration exact')
check('DoubleVectorGapRejected', bool(subset_errors([999999900000.0, -999999900000.0, 123.125], [999999965536.0, -999999900000.0, 123.125])), 'Actual axis deviation')
check('ValidDoublePreserved', subset_errors([123.125, -999999900000.0], [123.125, -999999900000.0]) == [], 'Valid double/vector baseline')
check('NativeFloat32QuantizationPreserved', subset_errors(999999.9, 999999.875, number_policy='float32') == [], 'Known float32 native storage, no double precision claim')
check('ChangedFloat32Rejected', bool(subset_errors(999999.9, 999999.9375, number_policy='float32')), 'Changed actual native value not requantized away')
check('ExactRequestMetadata', bool(subset_errors(.1, .100000001, number_policy='exact')), 'Request metadata is exact, unlike native float storage')
check('BooleanNotNumber', bool(subset_errors(True, 1)), 'Boolean identity remains distinct')
bad_panel = {'test': 'NodeForge.MinimalPanel', 'status': 'failed', 'widgetConstruction': 'passed',
    'tabOpened': True, 'tabClosed': False, 'preExistingProductionTabPreserved': False}
check('FailedPanelSubstepRejected', not automation_report_passed(bad_panel, 'NodeForge.MinimalPanel'), 'Old any-passed predicate would accept this failed terminal')
good_panel = {**bad_panel, 'status': 'passed', 'tabClosed': True, 'preExistingProductionTabPreserved': True}
check('ValidPanelReportAccepted', automation_report_passed(good_panel, 'NodeForge.MinimalPanel'), 'Native terminal and protection fields all required')
check('WrongTestRejected', not automation_report_passed(good_panel, 'NodeForge.BlueprintRuntime'), 'Test identity cannot be substituted')

late = out / 'Timeout'
late.mkdir()
late_file = late / 'CompletedAfterDeadline.txt'
child_code = 'import time; from pathlib import Path; time.sleep(1); Path(' + repr(str(late_file)) + ').write_text("owned child completed", encoding="utf-8")'
command = [sys.executable, '-c', child_code]
try:
    run_guarded(command, cwd=ROOT, console=late / 'OwnedChildConsole.txt', timeout=.15, expected_module=sys.executable)
    check('LiveDeadlinePreserved', False, 'Expected a live deadline checkpoint')
except LiveProcessPending as pending:
    live = read_json(pending.evidence)
    alive = pending.process.poll() is None
    loaded = read_json(late / 'OwnedChildLoadedModule.json')
    check('ActualModuleObserved', loaded['matchesExpected'] is True and loaded['pid'] == pending.process.pid, 'Real Windows module path and corresponding unchanged file SHA observed')
    check('LiveDeadlinePreserved', alive and live['pid'] == pending.process.pid and live['terminatedByRunner'] is False, 'Actual owned process live after deadline; no kill')
    code = pending.process.wait(timeout=10)  # wait has no kill behavior; this child naturally completes.
    check('ChildCompletesNaturally', code == 0 and late_file.read_text(encoding='utf-8') == 'owned child completed', 'Completion occurs after runner deadline')
    write_new(late / 'ObservedTerminal.json', {'pid': pending.process.pid, 'exitCode': code, 'completedAfterDeadline': late_file.is_file(), 'terminatedByRunner': False})
    try:
        run_guarded(command, cwd=ROOT, console=late / 'OwnedChildConsole.txt', timeout=.15)
        check('RepeatedBatchBlocked', False, 'Existing PID checkpoint must protect same batch')
    except ValueError:
        check('RepeatedBatchBlocked', True, 'No second child launched into existing evidence')
normal = out / 'Normal'
normal.mkdir()
result = run_guarded([sys.executable, '-c', 'print("owned failure"); raise SystemExit(3)'],
    cwd=ROOT, console=normal / 'ExitCodeConsole.txt', timeout=10)
check('NonzeroExitPreserved', result.returncode == 3 and read_json(normal / 'ExitCodeExit.json')['exitCode'] == 3, 'No success synthesis for nonzero child')

# Real owned children plus narrowly injected evidence/probe IO failures. No disk-full
# or asset-save failure is injected into Unreal.
real_popen = subprocess.Popen
for name, fail_checkpoint in [('StartedWriteFailure', False), ('CheckpointWriteFailure', True)]:
    fault = out / name
    fault.mkdir()
    late_file = fault / 'ChildCompleted.txt'
    code = 'import time; from pathlib import Path; time.sleep(1); Path(' + repr(str(late_file)) + ').write_text("done", encoding="utf-8")'
    def launch_and_collide(*args, **kwargs):
        child = real_popen(*args, **kwargs)
        (fault / 'FaultStarted.json').write_text('preserved sentinel', encoding='utf-8')
        return child
    real_open = Path.open
    def fail_live_open(path, *args, **kwargs):
        if fail_checkpoint and path == fault / 'LiveProcess.json':
            raise OSError('owned injected evidence write failure')
        return real_open(path, *args, **kwargs)
    try:
        with patch('RunProcessGuard.subprocess.Popen', side_effect=launch_and_collide), patch.object(Path, 'open', fail_live_open):
            run_guarded([sys.executable, '-c', code], cwd=ROOT, console=fault / 'FaultConsole.txt', timeout=10)
        check(name, False, 'Expected terminal live-process exception')
    except LiveProcessPending as pending:
        alive = pending.process.poll() is None
        accurate = (pending.evidence is None and 'checkpoint unavailable' in str(pending) and str(pending.process.pid) in str(pending)) if fail_checkpoint else read_json(pending.evidence)['pid'] == pending.process.pid
        exit_code = pending.process.wait(timeout=10)
        check(name, alive and accurate and exit_code == 0 and late_file.is_file(), 'Stops caller, preserves live child; unavailable checkpoint is stated honestly')
        write_new(fault / 'ObservedTerminal.json', {'pid': pending.process.pid, 'exitCode': exit_code, 'checkpointAvailable': pending.evidence is not None, 'terminatedByRunner': False})
probe_fault = out / 'ProbeIoFailure'
probe_fault.mkdir()
try:
    with patch('RunProcessGuard.observed_module', return_value=probe_fault / 'MissingModule.dll'):
        run_guarded([sys.executable, '-c', 'import time; time.sleep(1)'], cwd=ROOT,
            console=probe_fault / 'ProbeConsole.txt', timeout=10, expected_module=sys.executable)
    check('ProbeIoFailure', False, 'Expected live probe IO error to stop batch')
except LiveProcessPending as pending:
    alive = pending.process.poll() is None
    exit_code = pending.process.wait(timeout=10)
    check('ProbeIoFailure', alive and exit_code == 0 and read_json(pending.evidence)['pid'] == pending.process.pid, 'No OSError escapes to a caller that could continue the matrix')
files = ['Scripts/RunProcessGuard.py', 'Scripts/RunGraphRegression.py', 'Scripts/RunPrototypeRegression.py',
    'Scripts/ValidatePrototype.py', 'Scripts/RunPreflightChecks.py', 'Scripts/RunMaterialExportChecks.py', 'Tests/RunToolingGuards.py']
good = all(row['passed'] for row in rows)
write_new(out / 'Summary.json', {'result': 'passed' if good else 'failed', 'cases': rows,
    'testedFiles': {path: sha(ROOT / path) for path in files}, 'baselineHelperSha256': sha(baseline_path),
    'scope': 'Real project-owned child process and strict helper counterexamples; no UE, UI, save, runtime or package claims'})
print('Tooling guards ' + ('passed' if good else 'failed') + ': ' + str(len(rows)), flush=True)
raise SystemExit(0 if good else 1)
