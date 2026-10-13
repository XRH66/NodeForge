"""Exact native-log predicates and owned observer failure checkpoints; no UE run."""
import argparse
import ast
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'Scripts'))
from RunGraphRegression import read_json, write_new, sha, ref
from RunProcessGuard import run_guarded, LiveProcessPending

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--report-directory', required=True)
args = parser.parse_args()
out = (ROOT / args.report_directory).resolve()
if not out.is_relative_to(ROOT / 'Reports') or out.exists():
    raise SystemExit('Fresh Reports directory required')
out.mkdir()
source = ROOT / 'Scripts/RunStandaloneGraphChecks.py'
guard = ROOT / 'Scripts/RunProcessGuard.py'
baseline = ROOT / 'Reports/20261010NightAudit/StandaloneBaseline/RunStandaloneGraphChecks.py'
before = {ref(path): sha(path) for path in (source, guard, Path(__file__).resolve())}
tree = ast.parse(source.read_text('utf-8-sig'))
namespace = {'re': re}
functions = [node for node in tree.body if isinstance(node, ast.FunctionDef)
             and node.name in ('blueprint_user_messages', 'native_map_loaded')]
assert len(functions) == 2
exec(compile(ast.Module(body=functions, type_ignores=[]), str(source), 'exec'), namespace)
payloads = namespace['blueprint_user_messages']
map_loaded = namespace['native_map_loaded']
rows = []

def check(name, passed, detail):
    rows.append({'case': name, 'passed': bool(passed), 'detail': detail})

log = ROOT / 'Reports/20261009CandidateRC1/PackagedRuntime1/AiBlueprint.log'
text = log.read_text('utf-8-sig', errors='replace')
actor = 'NFExternalGraphBlueprint2_C_0'
message = 'NodeForge pulse material: enabled on BeginPlay.'
exact = '[' + actor + '] ' + message
map_path = '/Game/NodeForgeTests/NFCandidateAiBlueprintMap'
check('PreservedRealNativeMessageAccepted', payloads(text) == [exact],
      'Read preserved RC1 packaged log; not a new game/package execution')
check('PreservedRealNativeMapAccepted', map_loaded(text, map_path), 'Exact real Bringing World marker')
old_tree = ast.parse(baseline.read_text('utf-8-sig'))
old_assignment = next(node for node in ast.walk(old_tree) if isinstance(node, ast.Assign)
                      and any(isinstance(target, ast.Name) and target.id == 'matched' for target in node.targets))
old_predicate = compile(ast.Expression(old_assignment.value), str(baseline), 'eval')
prefix = '[2026.10.10-00.00.00:000][ 0]LogBlueprintUserMessages: '
suffix_log = prefix + exact + ' UNEXPECTED_SUFFIX\n'
check('OldTrailingPayloadFalsePassReproduced', len(eval(old_predicate, {'text': suffix_log, 'exact': exact})) == 1,
      'Actual prepatch AST accepts a longer message as an exact match')
for name, changed in [
    ('TrailingPayloadRejected', suffix_log),
    ('DuplicateExactMessageRejected', prefix + exact + '\n' + prefix + exact + '\n'),
    ('OtherActorExtraMessageRejected', prefix + exact + '\n' + prefix + '[OtherActor_C_0] unrelated\n'),
    ('WrongActorRejected', prefix + '[OtherActor_C_0] ' + message + '\n'),
    ('WrongMessageRejected', prefix + '[' + actor + '] different\n'),
    ('MissingMessageRejected', 'LogInit: no actor output\n'),
    ('TrailingWhitespaceNotSilentlyRemoved', prefix + exact + ' \n'),
]:
    check(name, payloads(changed) != [exact], 'Complete native payload/actor and one total message required')
check('ExplicitSignificantSpacesPreserved', payloads(prefix + exact + '  \n') == [exact + '  '],
      'Do not strip actual user message trailing spaces')
check('CommandMentionNotLoadedWorld', not map_loaded('LogInit: Command Line: ' + map_path, map_path),
      'Map named in the command is not a live world load')
check('DifferentWorldRejected', not map_loaded(text, map_path + 'Other'), 'Exact map identity')
check('ShorterMapPrefixRejected', not map_loaded(text, map_path[:-3]), 'Asset boundary cannot match a longer map')
calls = [node for node in ast.walk(tree) if isinstance(node, ast.Call)]
check('EngineOnlyUsesSharedGuard', any(isinstance(node.func, ast.Name) and node.func.id == 'run_guarded'
      and any(keyword.arg == 'observer' for keyword in node.keywords) for node in calls)
      and not any(isinstance(node.func, ast.Attribute) and node.func.attr == 'Popen' for node in calls),
      'No second engine Popen/poll implementation; inspector is a bounded read-only process')
check('ExactWholePayloadGatePresent', any(isinstance(node, ast.Compare)
      and isinstance(node.left, ast.Name) and node.left.id == 'payloads'
      and isinstance(node.ops[0], ast.NotEq) and isinstance(node.comparators[0], ast.List)
      and len(node.comparators[0].elts) == 1 for node in ast.walk(tree)), 'The runner uses complete message-list equality')

# Observers are internal Python callbacks, never interpreted from imported JSON.
# Faults affect only project-owned short Python children; no engine save is injected.
for name, failure in [('ObserverIoFailure', OSError('owned observer read failure')),
                      ('ObserverInspectorTimeout', subprocess.TimeoutExpired('owned inspector', .1))]:
    folder = out / name
    folder.mkdir()
    completed = folder / 'CompletedNaturally.txt'
    child = 'import time; from pathlib import Path; time.sleep(1); Path(' + repr(str(completed)) + ').write_text("done", encoding="utf-8")'
    observed = []
    def fail_observer(process):
        observed.append(process.pid)
        raise failure
    try:
        run_guarded([sys.executable, '-c', child], cwd=ROOT,
                    console=folder / 'OwnedConsole.txt', timeout=10, observer=fail_observer)
        check(name, False, 'Expected active-child checkpoint')
    except LiveProcessPending as pending:
        evidence = read_json(pending.evidence)
        alive = pending.process.poll() is None
        code = pending.process.wait(timeout=10)  # wait does not kill; this child terminates itself.
        check(name, alive and code == 0 and completed.is_file() and observed == [pending.process.pid]
              and evidence['pid'] == pending.process.pid and evidence['terminatedByRunner'] is False
              and evidence['exitCode'] is None and not (folder / 'OwnedExit.json').exists(),
              'Stops caller immediately; exact live PID retained and child naturally completes')
        write_new(folder / 'ObservedTerminal.json', {'pid': pending.process.pid,
                  'exitCode': code, 'terminatedByRunner': False, 'completedNaturally': completed.is_file()})

normal = out / 'ObserverNormal'
normal.mkdir()
observed = []
process = run_guarded([sys.executable, '-c', 'import time; time.sleep(.5); print("done")'],
                      cwd=ROOT, console=normal / 'OwnedConsole.txt', timeout=10,
                      observer=lambda child: observed.append(child.pid), expected_module=sys.executable)
ended = read_json(normal / 'OwnedExit.json')
check('ObserverNormalAndActualModule', process.returncode == 0 and observed
      and set(observed) == {process.pid} and ended['pid'] == process.pid
      and ended['actualLoadedModule']['matchesExpected'] is True
      and not (normal / 'LiveProcess.json').exists(), 'Actual owned child PID/module and natural terminal preserved')
terminal = out / 'ObserverAlreadyTerminal'
terminal.mkdir()
terminal_pid = []
def fail_after_terminal(child):
    terminal_pid.append(child.pid)
    child.wait(timeout=10)
    raise OSError('owned observation failed after natural child exit')
try:
    run_guarded([sys.executable, '-c', 'import time; time.sleep(.1); print("completed")'],
                cwd=ROOT, console=terminal / 'OwnedConsole.txt', timeout=10,
                observer=fail_after_terminal)
    check('TerminalObserverErrorStillFailsRunner', False, 'Terminal observation error must propagate')
except OSError:
    ended = read_json(terminal / 'OwnedFailedTerminal.json')
    check('TerminalObserverErrorStillFailsRunner', ended['runnerStatus'] == 'failed'
          and ended['runnerErrorType'] == 'OSError' and ended['pid'] == terminal_pid[0]
          and ended['exitCode'] == 0 and ended['terminatedByRunner'] is False,
          'Actual exit zero retained with explicit runner failure; original error propagates')
    check('TerminalFailureCannotResumeAsSuccess', not (terminal / 'OwnedExit.json').exists()
          and not (terminal / 'LiveProcess.json').exists(),
          'Independent failed-terminal evidence is not a successful Exit or a live-PID claim')
invalid = out / 'InvalidObserver'
invalid.mkdir()
try:
    run_guarded([sys.executable, '-c', 'raise SystemExit(0)'], cwd=ROOT,
                console=invalid / 'OwnedConsole.txt', timeout=10, observer='not callable')
    check('InvalidObserverRejectedBeforeLaunch', False, 'Internal callback required')
except ValueError:
    check('InvalidObserverRejectedBeforeLaunch', not list(invalid.iterdir()), 'No command, console or child started')

after = {ref(path): sha(path) for path in (source, guard, Path(__file__).resolve())}
check('TestedSourcesUnchanged', before == after, 'No source edits during the test')
good = all(row['passed'] for row in rows)
write_new(out / 'Summary.json', {'result': 'passed' if good else 'failed', 'cases': rows,
          'testedFiles': before, 'baselineSha256': sha(baseline), 'preservedLog': ref(log),
          'preservedLogSha256': sha(log), 'scope': 'Pure exact-log predicates and project-owned Python children; no new UE, UI, save, game or package verification'})
print('Night standalone guards', 'passed' if good else 'failed', len(rows), flush=True)
raise SystemExit(0 if good else 1)
