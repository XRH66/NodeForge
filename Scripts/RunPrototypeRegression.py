"""Replay unchanged two positive fixtures in a fresh local test project; preserve all evidence."""
import argparse, hashlib, json, subprocess
from pathlib import Path
from RunGraphRegression import build_snapshot, protected_assets, automation_report_passed
from RunProcessGuard import run_guarded
root = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--run', required=True)
p.add_argument('--plugin-directory', help='Explicit staged NodeForge directory under Reports')
p.add_argument('--skip-panel', action='store_true', help='Run core assets/runtime only; panel remains explicitly untested')
a = p.parse_args()
plugin = (root/a.plugin_directory).resolve() if a.plugin_directory else root/'Plugins/NodeForge'
if a.plugin_directory and (not plugin.is_relative_to(root/'Reports') or plugin.name != 'NodeForge'):
    raise SystemExit('Only staged NodeForge under Reports is permitted')
if not (plugin/'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll').is_file():
    raise SystemExit('Selected built DLL required')
before_build = build_snapshot(plugin if a.plugin_directory else None)
before_assets = protected_assets()
fixture_before = {f: hashlib.sha256((root / f).read_bytes()).hexdigest()
    for f in ['Tests/Valid/MinimalBlueprint.json', 'Tests/Valid/MinimalMaterialReady.json']}
runner_before = {f: hashlib.sha256((root / f).read_bytes()).hexdigest()
    for f in ['Scripts/RunPrototypeRegression.py', 'Scripts/RunGraphRegression.py', 'Scripts/RunProcessGuard.py']}
if not a.run.isalnum():
    raise SystemExit('Use a fresh alphanumeric run name')
out = root / 'Reports/20261008UsablePrototype' / a.run
project_dir = root / 'ValidationProject/RegressionRuns' / a.run
out.mkdir(parents=True, exist_ok=False)
project_dir.mkdir(parents=True, exist_ok=False)
project = project_dir / 'NodeForgeRegression.uproject'
project.write_text(json.dumps({'FileVersion': 3, 'EngineAssociation': '5.3',
    'AdditionalPluginDirectories': [str(plugin.parent)], 'Plugins': [{'Name': 'NodeForge', 'Enabled': True}]}, indent=2), 'utf-8')
(project_dir / 'Config').mkdir()
(project_dir / 'Config/DefaultEngine.ini').write_text('[/Script/Engine.RendererSettings]\nr.DynamicGlobalIlluminationMethod=0\nr.ReflectionMethod=0\nr.DefaultFeature.AutoExposure=False\n', 'utf-8')
engine = Path('X:/UnrealEngine/UE_5.3/Engine/Binaries/Win64/UnrealEditor-Cmd.exe')
steps = []
def run(name, options, expected='succeeded', automation=False):
    report = out / (name + '.json')
    command = [str(engine), str(project), *options, '-NFReport=' + str(report),
        '-d3d11', '-RenderOffscreen', '-AllowCommandletRendering', '-unattended', '-nop4', '-nosound', '-NoLiveCoding',
        '-abslog=' + str(out / (name + '.log')), '-stdout', '-FullStdOutLogOutput']
    if name == 'MinimalPanel':
        command.append('-NFPanelReport=' + str(report))
    proc = run_guarded(command, cwd=root, console=out / (name + 'Console.txt'), timeout=240,
        expected_module=plugin / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll')
    data = json.loads(report.read_text('utf-8')) if report.exists() else {}
    actual = data.get('overallStatus', data.get('result', data.get('status')))
    inputs_safe = all(hashlib.sha256((root / f).read_bytes()).hexdigest() == value for f, value in fixture_before.items())
    runner_safe = all(hashlib.sha256((root / f).read_bytes()).hexdigest() == value for f, value in runner_before.items())
    good = inputs_safe and runner_safe and proc.returncode == 0 and (automation_report_passed(data, 'NodeForge.' + name) if automation else actual == expected)
    steps.append({'step': name, 'command': command, 'exitCode': proc.returncode,
        'report': report.relative_to(root).as_posix(), 'actual': actual, 'passed': good})
    (out / 'Progress.json').write_text(json.dumps(steps, indent=2), 'utf-8')
    print(name, proc.returncode, actual, flush=True)
    if not good:
        raise SystemExit('Regression failed; evidence retained: ' + name)
    return report
for domain, file in [('Blueprint', 'MinimalBlueprint.json'), ('Material', 'MinimalMaterialReady.json')]:
    inp = root / 'Tests/Valid' / file
    previous = run(domain + 'Import', ['-run=NodeForge', '-NFMode=Import', '-NFInput=' + str(inp)], 'waitingPersistence')
    run(domain + 'Reopen', ['-run=NodeForge', '-NFMode=Verify', '-NFInput=' + str(inp), '-NFPrevious=' + str(previous)])
run('CreateMap', ['-run=NodeForge', '-NFMode=CreateMap'], 'waitingPersistence')
run('BlueprintRuntime', ['-ExecCmds=Automation RunTests NodeForge.BlueprintRuntime; Quit', '-TestExit=Automation Test Queue Empty'], automation=True)
if not a.skip_panel:
    run('MinimalPanel', ['-ExecCmds=Automation RunTests NodeForge.MinimalPanel; Quit', '-TestExit=Automation Test Queue Empty'], automation=True)
after_build = build_snapshot(plugin if a.plugin_directory else None)
if before_build != after_build or before_assets != protected_assets():
    raise SystemExit('Core regression changed protected assets or selected source/binary')
(out / 'Summary.json').write_text(json.dumps({'result': 'passed', 'steps': steps,
    'fixtureHashes': fixture_before,
    'fixtureBytesUnchanged': all(hashlib.sha256((root / f).read_bytes()).hexdigest() == value for f, value in fixture_before.items()),
    'runnerHashes': runner_before,
    'project': project.relative_to(root).as_posix(), 'selectedPlugin':plugin.relative_to(root).as_posix(),
    'sourceAndBinaryBefore':before_build,'sourceAndBinaryAfter':after_build,
    'oneUnchangedBuild':True,'allProtectedAssetBytesUnchanged':True,
    'panel':'notRun' if a.skip_panel else 'passed','scope':'v1 two assets compile/save/reopen and real PIE; panel scope explicit'}, indent=2), 'utf-8')
