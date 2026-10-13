"""Verify relayout-as-new-copy through the same UI backend; no UI claim."""
import argparse
from pathlib import Path
from RunGraphRegression import (ROOT, ENGINE, REQUIRED_IMPORT_CHECKS, read_json, write_new,
    sha, ref, build_snapshot, protected_assets, readback_errors)
from RunProcessGuard import run_guarded


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--project', required=True)
    p.add_argument('--material', required=True)
    p.add_argument('--blueprint', required=True)
    p.add_argument('--out', required=True)
    p.add_argument('--plugin-directory', required=True, help='Explicit isolated stage matching this project')
    args = p.parse_args()
    project = (ROOT / args.project).resolve()
    out = (ROOT / args.out).resolve()
    if not project.is_relative_to(ROOT / 'ValidationProject/RegressionRuns') or not project.is_file():
        raise SystemExit('Use an existing isolated validation project')
    plugin = (ROOT / args.plugin_directory).resolve()
    dll = plugin / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'
    if not plugin.is_relative_to(ROOT / 'Reports') or plugin.name != 'NodeForge' or not dll.is_file():
        raise SystemExit('Explicit isolated compiled plugin stage required')
    binary_sha = sha(dll)
    if not out.is_relative_to(ROOT / 'Reports') or out.exists():
        raise SystemExit('Use a fresh workspace report directory')
    out.mkdir(parents=True)
    baseline = build_snapshot(plugin)
    protected = protected_assets()
    runner_files = [Path(__file__).resolve(), ROOT / 'Scripts/RunGraphRegression.py', ROOT / 'Scripts/RunProcessGuard.py']
    runner_hashes = {ref(file): sha(file) for file in runner_files}
    write_new(out / 'Prerequisites.json', {'project': ref(project), 'selectedPlugin': ref(plugin),
        'expectedDllSha256': binary_sha, 'runnerHashes': runner_hashes})
    results = []

    def execute(label, mode, input_path, previous=None):
        if baseline != build_snapshot(plugin) or protected != protected_assets() or runner_hashes != {ref(file): sha(file) for file in runner_files}:
            raise SystemExit('Build, runner or protected asset changed; stop before starting another process')
        report = out / (label + '.json')
        command = [str(ENGINE), str(project), '-run=NodeForge', '-NFMode=' + mode,
                   '-NFInput=' + str(input_path), '-NFReport=' + str(report),
                   '-d3d11', '-RenderOffscreen', '-AllowCommandletRendering',
                   '-unattended', '-nop4', '-nosound', '-NoLiveCoding',
                   '-abslog=' + str(out / (label + '.log')), '-stdout', '-FullStdOutLogOutput']
        if previous:
            command.append('-NFPrevious=' + str(previous))
        completed = run_guarded(command, cwd=ROOT, console=out / (label + 'Console.txt'),
            timeout=360, expected_module=dll)
        data = read_json(report) if report.is_file() else {}
        return completed.returncode, data, report

    for label, relative in [('Material', args.material), ('Blueprint', args.blueprint)]:
        source = (ROOT / relative).resolve()
        if not source.is_relative_to(ROOT) or not source.is_file():
            raise SystemExit('Input must be an existing workspace file')
        request = read_json(source)
        source_hash = sha(source)
        asset = project.parent / 'Content' / (request['assetPath'].removeprefix('/Game/') + '.uasset')
        source_asset_hash = sha(asset) if asset.is_file() else None
        if not source_asset_hash:
            raise SystemExit('Expected original generated asset before relayout check')
        code, result, report = execute(label + 'Copy', 'RelayoutCopy', source)
        errors = []
        if code or result.get('overallStatus') != 'waitingPersistence':
            errors.append('Copy import did not reach saved waitingPersistence')
        if result.get('executionFingerprint', {}).get('binarySha256') != binary_sha:
            errors.append('Copy receipt does not identify the selected actual plugin')
        for check in REQUIRED_IMPORT_CHECKS:
            if result.get('checks', {}).get(check, {}).get('status') != 'passed':
                errors.append('Copy check did not pass: ' + check)
        if result.get('sourceInputDigest') != source_hash or result.get('sourceAssetPath') != request['assetPath']:
            errors.append('Copy provenance does not identify the original exact input/target')
        derived_path = Path(result.get('derivedInputPath', '')).resolve()
        if not derived_path.is_relative_to(project.parent / 'Saved') or not derived_path.is_file():
            errors.append('No isolated saved derived input')
        else:
            derived = read_json(derived_path)
            if result.get('inputDigest') != sha(derived_path):
                errors.append('Copy receipt does not identify its derived exact input bytes')
            if derived.get('assetPath') == request['assetPath'] or derived.get('requestId') == request['requestId']:
                errors.append('New copy identity was not new')
            expected_nodes = [{k: v for k, v in node.items() if k != 'position'} for node in request['nodes']]
            if derived.get('nodes') != expected_nodes or derived.get('edges') != request['edges'] or derived.get('settings') != request['settings']:
                errors.append('Relayout altered graph semantics rather than only identity/positions')
            for key in ('initialReadback', 'finalReadback', 'publishedReadback'):
                errors.extend(key + ': ' + e for e in readback_errors(derived, result.get(key)))
            verify_code, verify, _ = execute(label + 'CopyReopen', 'Verify', derived_path, report)
            if verify_code or verify.get('overallStatus') != 'succeeded' or verify.get('pid') == result.get('pid'):
                errors.append('Independent persistence verification failed')
            if verify.get('executionFingerprint', {}).get('binarySha256') != binary_sha or verify.get('checks', {}).get('persistence', {}).get('status') != 'passed':
                errors.append('Reopen module identity or persistence check missing')
            for key in ('reopenedReadback', 'reopenedFinalReadback'):
                errors.extend(key + ': ' + e for e in readback_errors(derived, verify.get(key)))
                positions = lambda obj: {n['id']: n['position'] for n in obj.get('nodes', [])}
                if positions(verify.get(key, {})) != positions(result.get('publishedReadback', {})):
                    errors.append('Saved/reopened copy positions differ')
        if sha(source) != source_hash or sha(asset) != source_asset_hash:
            errors.append('Original input or original asset bytes changed')
        results.append({'case': label, 'result': 'failed' if errors else 'passed', 'errors': errors,
                        'source': ref(source), 'sourceSha256': source_hash, 'sourceAssetSha256': source_asset_hash,
                        'copyReport': ref(report), 'copyAssetPath': result.get('assetPath')})
        print(label + ': ' + results[-1]['result'], flush=True)
    unchanged_build = build_snapshot(plugin) == baseline
    unchanged_assets = protected_assets() == protected
    unchanged_runners = runner_hashes == {ref(file): sha(file) for file in runner_files}
    summary = {'result': 'passed' if len(results) == 2 and all(not x['errors'] for x in results) and unchanged_build and unchanged_assets and unchanged_runners else 'failed',
               'cases': results, 'oneUnchangedBuild': unchanged_build, 'protectedOriginalAssetsUnchanged': unchanged_assets,
               'runnerFilesUnchanged': unchanged_runners, 'runnerHashes': runner_hashes,
               'selectedPlugin': ref(plugin), 'selectedBinarySha256': binary_sha,
               'scope': 'Same RelayoutCopy backend and independent persistence; real button interaction not tested'}
    write_new(out / 'Summary.json', summary)
    return 0 if summary['result'] == 'passed' else 1


if __name__ == '__main__':
    raise SystemExit(main())
