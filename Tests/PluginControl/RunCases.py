"""Sequential acceptance runner; only the official CLI creates/reads UE assets."""
import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'Scripts'))
from NodeForgeControl import digest, read_json, write_new
from RunGraphRegression import ASSET_SUFFIXES, REQUIRED_IMPORT_CHECKS, files_under, readback_errors, build_snapshot, ref
from RunProcessGuard import LiveProcessPending, run_guarded, validate_terminal_step


def run_client(arguments, console, timeout):
    """One shared client subprocess boundary for positive and refusal cases."""
    process = run_guarded([sys.executable, str(ROOT / 'Scripts/NodeForgeControl.py'), *arguments],
                          cwd=ROOT, console=console, timeout=timeout + 30)
    return process, json.loads(Path(console).read_text('utf-8'))


def protection_guard(paths, plugin):
    """Optional project-owned maps; old callers retain their original invocation."""
    protected = {}
    for value in paths:
        path = Path(value).resolve()
        if not path.is_relative_to(ROOT / 'Reports'):
            raise ValueError('Protection report must be inside workspace Reports')
        data = read_json(path)
        data = data.get('protectionBefore', data)
        if not isinstance(data, dict) or not data:
            raise ValueError('Nonempty protection map required')
        for relative, expected in data.items():
            target = (ROOT / relative).resolve()
            if (Path(relative).is_absolute() or not target.is_relative_to(ROOT)
                    or not isinstance(expected, str) or not re.fullmatch('[0-9a-f]{64}', expected)):
                raise ValueError('Invalid project-owned protection entry')
            if relative in protected and protected[relative] != expected:
                raise ValueError('Conflicting protection entry')
            protected[relative] = expected
    snapshot = build_snapshot(plugin)
    scripts = {relative: digest(ROOT / relative) for relative in (
        'Scripts/NodeForgeControl.py', 'Scripts/NodeForgeEditorTransport.py',
        'Scripts/RunProcessGuard.py', 'Scripts/RunGraphRegression.py',
        'Tests/PluginControl/RunCases.py', 'Tests/PluginControl/RunBoundaries.py')}

    def guard():
        changed = [relative for relative, expected in {**protected, **scripts}.items()
            if not (ROOT / relative).is_file() or digest(ROOT / relative) != expected]
        if changed or build_snapshot(plugin) != snapshot:
            raise RuntimeError('Protected source/stage/old bytes changed; stop next process: ' + repr(changed))
    guard()
    return guard, protected


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--manifest', default=str(Path(__file__).with_name('Cases.json')))
    for name in ('project', 'plugin-directory', 'description-file', 'report-directory'):
        p.add_argument('--' + name, required=True)
    p.add_argument('--timeout-seconds', type=float, default=180)
    p.add_argument('--asset-protection', action='append', default=[])
    args = p.parse_args()
    project = Path(args.project).resolve(); plugin = Path(args.plugin_directory).resolve()
    binary = plugin / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'
    guard, protected = protection_guard(args.asset_protection, plugin)
    out = Path(args.report_directory).resolve()
    if not out.is_relative_to(ROOT / 'Reports'):
        raise SystemExit('Fresh workspace Reports directory required')
    out.mkdir(parents=True, exist_ok=False)
    cases = read_json(args.manifest)['cases']
    manifest_sha = digest(args.manifest)
    owned_assets = files_under(project.parent / 'Content')
    saved_assets = {}
    def asset_guard(allowed=None):
        guard()
        if digest(args.manifest) != manifest_sha:
            raise RuntimeError('Fixed case manifest changed')
        actual = files_under(project.parent / 'Content')
        if (any(actual.get(key) != value for key, value in owned_assets.items())
                or set(actual) - set(owned_assets) - ({ref(allowed)} if allowed else set())):
            raise RuntimeError('Known saved asset or unexpected owned Content changed; stop next process')
    results = []; stopped = False
    for case in cases:
        name = case['name']
        if not re.fullmatch('[A-Za-z][A-Za-z0-9]*', name):
            raise SystemExit('Safe case name required')
        if stopped:
            results.append({'name': name, 'status': 'notRun', 'reason': 'previous case failed/pending'})
            continue
        asset_guard()
        source = (ROOT / case['input']).resolve(); request = read_json(source, 65536)
        original_sha = digest(source)
        target = project.parent / 'Content' / (request['assetPath'].removeprefix('/Game/') + '.uasset')
        if not target.resolve().is_relative_to(project.parent / 'Content') or target.exists():
            raise RuntimeError('Positive case requires one exact fresh owned target')
        case_out = out / name; case_out.mkdir()
        before = files_under(project.parent / 'Content', ASSET_SUFFIXES)
        errors = []; phases = []; generation = None; generated_bytes = None
        for phase, operation in [('Preflight', 'graphs.preflightFromFile'), ('Generate', 'graphs.generateFromFile'), ('Verify', 'graphs.verifySaved')]:
            asset_guard()
            run = case_out / phase
            command = ['execute',
                       '--operation', operation, '--input', str(source), '--project', str(project),
                       '--plugin-directory', str(plugin), '--run-directory', str(run),
                       '--description-file', str(Path(args.description_file).resolve()),
                       '--timeout-seconds', str(args.timeout_seconds)]
            if phase == 'Verify': command += ['--generation-report', str(case_out / 'Generate/Native.json')]
            try:
                process, summary = run_client(command, case_out / (phase + 'ClientConsole.txt'), args.timeout_seconds)
            except LiveProcessPending as error:
                phases.append({'phase': phase, 'status': 'pending', 'pid': error.process.pid, 'checkpoint': str(error.evidence)})
                stopped = True; break
            if process.returncode != 0:
                phases.append({'phase': phase, 'status': 'pending' if process.returncode == 5 else 'failed', 'summary': summary})
                errors.append('Official CLI did not fulfill ' + operation); stopped = True; break
            asset_guard(target if phase == 'Generate' else None)
            data = read_json(run / 'Native.json')
            validate_terminal_step(run, 'Native', {'step': 'Native', 'passed': True, 'pid': summary['pid'], 'exitCode': 0}, binary, project)
            after = files_under(project.parent / 'Content', ASSET_SUFFIXES)
            if summary.get('inputDigest') != original_sha or data.get('inputDigest') != original_sha:
                errors.append(phase + ': original byte digest differs')
            if phase == 'Preflight':
                if data.get('operationStatus') != 'passed' or after != before:
                    errors.append('Preflight did not pass without asset mutation')
            elif phase == 'Generate':
                generation = data
                for check in REQUIRED_IMPORT_CHECKS:
                    if data.get('checks', {}).get(check, {}).get('status') != 'passed': errors.append(check + ': not passed')
                for key in ('initialReadback', 'finalReadback', 'publishedReadback'):
                    errors += [key + ': ' + e for e in readback_errors(request, data.get(key))]
                if not target.is_file() or data.get('fileSha256') != digest(target): errors.append('Actual save bytes differ')
                if any(after.get(key) != value for key, value in before.items()): errors.append('Existing asset changed')
                if data.get('overallStatus') != 'waitingPersistence' or data.get('requestId') != request['requestId']: errors.append('Save/identity differs')
                generated_bytes = after
                if not errors:
                    owned_assets[ref(target)] = digest(target)
                    saved_assets[ref(target)] = digest(target)
                    write_new(case_out / 'SavedAssetProtection.json', dict(saved_assets))
                    asset_guard()
            else:
                if data.get('pid') == generation.get('pid') or data.get('checks', {}).get('persistence', {}).get('status') != 'passed':
                    errors.append('Different PID persistence proof missing')
                for key in ('reopenedReadback', 'reopenedFinalReadback'):
                    errors += [key + ': ' + e for e in readback_errors(request, data.get(key))]
                if after != generated_bytes: errors.append('Verify mutated actual saved bytes')
            phases.append({'phase': phase, 'status': 'failed' if errors else 'passed', 'pid': data['pid'],
                           'nativeReport': str(run / 'Native.json'), 'sha256': digest(run / 'Native.json'), 'errors': list(errors)})
            if errors: stopped = True; break
        if digest(source) != original_sha: errors.append('Original input changed')
        result = {'name': name, 'input': str(source), 'inputDigest': original_sha,
                  'status': 'passed' if len(phases) == 3 and all(x['status'] == 'passed' for x in phases) and not errors else 'failed',
                  'phases': phases, 'errors': errors, 'assetsBefore': before,
                  'assetsAfter': files_under(project.parent / 'Content', ASSET_SUFFIXES), 'runtime': 'notRun', 'visual': 'notRun', 'gui': 'notRun'}
        write_new(case_out / 'Checks.json', result); results.append(result)
        print(json.dumps({'case': name, 'status': result['status'], 'errors': errors}), flush=True)
    final = {'status': 'passed' if results and all(x['status'] == 'passed' for x in results) else 'failed',
             'project': str(project), 'pluginDirectory': str(plugin), 'moduleSha256': digest(binary), 'cases': results,
             'protectedFiles': len(protected), 'savedAssetProtection': saved_assets,
             'actualAssetsAfter': files_under(project.parent / 'Content'),
             'unconfirmedAssetWrites': {p:h for p,h in files_under(project.parent / 'Content').items() if p not in owned_assets}}
    guard()
    if saved_assets:
        write_new(out / 'SavedAssetProtection.json', saved_assets)
    write_new(out / 'Checks.json', final)
    return 0 if final['status'] == 'passed' else 1


if __name__ == '__main__':
    sys.exit(main())
