"""Run only the fixed prototype rejection cases; never install dependencies or delete assets.
Use a fresh report directory. Paths are anchored to this workspace and the verified UE install.
"""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import sys
from RunProcessGuard import run_guarded

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--report-dir', required=True)
parser.add_argument('--project', help='Existing isolated RegressionRuns project; default remains DevProject')
args = parser.parse_args()
reports = (root / args.report_dir).resolve()
if not reports.is_relative_to(root / 'Reports'):
    raise SystemExit('Reports must remain inside this workspace Reports directory')
reports.mkdir(parents=True, exist_ok=False)
engine = Path('X:/UnrealEngine/UE_5.3/Engine/Binaries/Win64/UnrealEditor-Cmd.exe')
project = (root/args.project).resolve() if args.project else root / 'DevProject/NodeForgeDev.uproject'
if args.project and (not project.is_relative_to(root/'ValidationProject/RegressionRuns') or not project.is_file()):
    raise SystemExit('Existing isolated RegressionRuns project required')

def assets():
    folders = ['DevProject/Content', 'ValidationProject/Content']
    if args.project:
        folders.append((project.parent/'Content').relative_to(root).as_posix())
    return {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for folder in folders
            for p in (root / folder).rglob('*') if p.suffix in ['.uasset', '.umap']}

before = assets()
cases = json.loads((root / 'Tests/Regression/PrototypeCases.json').read_text('utf-8'))
results = []
for case in cases:
    name = case['name']
    report = reports / (name + '.json')
    command = [str(engine), str(project), '-run=NodeForge', '-NFMode=Import',
               '-NFInput=' + str(root / case['input']), '-NFReport=' + str(report),
               '-unattended', '-nop4', '-nosound', '-nullrhi', '-NoLiveCoding',
               '-abslog=' + str(reports / (name + '.log')), '-stdout', '-FullStdOutLogOutput']
    process = run_guarded(command, cwd=root, console=reports / (name + 'Console.txt'), timeout=180)
    data = json.loads(report.read_text('utf-8')) if report.exists() else {}
    good = (process.returncode != 0 and data.get('overallStatus') == case['expected']
            and data.get('checks', {}).get('build', {}).get('status') == 'notRun'
            and data.get('residue', {}).get('diskState') == 'notWritten')
    results.append({'name': name, 'command': command, 'exitCode': process.returncode,
                    'expectedStatus': case['expected'], 'actualStatus': data.get('overallStatus'),
                    'result': 'passed' if good else 'failed',
                    'reportRef': report.relative_to(root).as_posix(), 'reason': data.get('nextAction')})
    print(name + ': ' + ('passed' if good else 'failed'), flush=True)
after = assets()
summary = {'purpose': 'minimal rejection checks, not full schema or fault injection coverage',
           'cases': results, 'assetsBefore': before, 'assetsAfter': after,
           'allAssetBytesUnchanged': before == after,
           'result': 'passed' if before == after and all(r['result'] == 'passed' for r in results) else 'failed'}
with (reports / 'Summary.json').open('x', encoding='utf-8') as output:
    json.dump(summary, output, indent=2)
sys.exit(0 if summary['result'] == 'passed' else 1)
