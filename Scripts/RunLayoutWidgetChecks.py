"""Inspect saved native widgets sequentially; no compile, resave or UI inputs."""
import argparse
import subprocess
import sys
import time
from pathlib import Path

from RunGraphRegression import (ROOT, ENGINE, ASSET_SUFFIXES, build_snapshot,
                                files_under, read_json, ref, sha, write_new)

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--run', required=True)
parser.add_argument('--graph-run', required=True)
parser.add_argument('--cases', nargs='+')
args = parser.parse_args()
if not all(value.isascii() and value.isalnum() for value in (args.run, args.graph_run)):
    raise SystemExit('New ASCII alphanumeric run names required')
graph = ROOT / 'Reports/20261008GraphV2' / args.graph_run
previous = read_json(graph / 'Summary.json')
if previous.get('result') != 'passed':
    raise SystemExit('Passing saved graph evidence required')
project = ROOT / previous['project']
out = ROOT / 'Reports/20261009UsableEditor' / args.run
out.mkdir(exist_ok=False)
baseline = build_snapshot()
assets_before = files_under(project.parent / 'Content', ASSET_SUFFIXES)
write_new(out / 'Baseline.json', {'sourceAndBinary': baseline, 'assets': assets_before})
cases = ['LayoutMaterialLong', 'LayoutBlueprintAuto', 'LayoutBlueprintLong',
         'LayoutMaterialAnchors', 'LayoutMaterialAnchorOverlap', 'LayoutWaterAuto']
if args.cases:
    if any(case not in cases for case in args.cases):
        raise SystemExit('Unknown widget case')
    cases = args.cases
steps = []
for case in cases:
    if build_snapshot() != baseline:
        steps.append({'case': case, 'result': 'blocked', 'errors': ['Build changed']})
        break
    source = graph / 'Inputs' / (case + '.json')
    request = read_json(source)
    report = out / (case + '.json')
    cmd = [str(ENGINE), str(project),
           '-ExecCmds=Automation RunTests NodeForge.LayoutBounds; Quit',
           '-TestExit=Automation Test Queue Empty', '-NFLayoutInput=' + str(source),
           '-NFLayoutReport=' + str(report), '-d3d11', '-RenderOffscreen',
           '-unattended', '-nop4', '-nosound', '-NoLiveCoding',
           '-abslog=' + str(out / (case + '.log')), '-stdout', '-FullStdOutLogOutput']
    write_new(out / (case + 'Command.json'), cmd)
    started = time.monotonic()
    code, error = None, None
    with (out / (case + 'Console.txt')).open('xb') as stream:
        try:
            process = subprocess.run(cmd, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT,
                                     timeout=240, creationflags=subprocess.CREATE_NO_WINDOW)
            code = process.returncode
        except (OSError, subprocess.TimeoutExpired) as exc:
            error = str(exc)
    data = read_json(report) if report.exists() else {}
    nodes = data.get('actualWidgetNodes', [])
    expected = {node['id'] for node in request['nodes']}
    if request['assetType'] == 'Material':
        expected.add('Material')
    errors = [error] if error else []
    if code != 0 or data.get('operationStatus') != 'passed':
        errors.append('Native widget inspection failed')
    if {node['id'] for node in nodes} != expected or len(nodes) != len(expected):
        errors.append('Actual native widget identities/count differ')
    if not data.get('noAutomaticWidgetOverlap'):
        errors.append('Automatic native widgets overlap')
    if files_under(project.parent / 'Content', ASSET_SUFFIXES) != assets_before:
        errors.append('Read-only inspection changed asset bytes')
    step = {'case': case, 'result': 'passed' if not errors else 'failed',
            'errors': errors, 'exitCode': code, 'inputSha256': sha(source),
            'report': ref(report), 'durationSeconds': round(time.monotonic() - started, 3)}
    steps.append(step)
    print(case, step['result'], errors, flush=True)
after = build_snapshot()
assets_after = files_under(project.parent / 'Content', ASSET_SUFFIXES)
good = len(steps) == len(cases) and all(s['result'] == 'passed' for s in steps)
good = good and after == baseline and assets_before == assets_after
write_new(out / 'Summary.json', {'result': 'passed' if good else 'failed', 'cases': steps,
          'sourceAndBinaryBefore': baseline, 'sourceAndBinaryAfter': after,
          'oneUnchangedBuild': baseline == after, 'assetsBefore': assets_before,
          'assetsAfter': assets_after, 'allSavedAssetBytesUnchanged': assets_before == assets_after,
          'scope': 'Native SGraphNode desired bounds after prepass in new processes; not screenshot or human readability acceptance'})
sys.exit(0 if good else 1)
