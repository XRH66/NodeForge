"""Fixed refusal cases using the same official client invocation as RunCases."""
import argparse
import json
import sys
from pathlib import Path

from RunCases import ROOT, run_client, protection_guard
from NodeForgeControl import digest, read_json, write_new
from RunGraphRegression import ASSET_SUFFIXES, files_under


def main():
    if hasattr(sys.stdout, 'reconfigure'): sys.stdout.reconfigure(encoding='utf-8', newline='\n')
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--manifest', default=str(Path(__file__).with_name('BoundaryCases.json')))
    for name in ('project', 'plugin-directory', 'description-file', 'report-directory'):
        p.add_argument('--' + name, required=True)
    p.add_argument('--asset-protection', action='append', default=[])
    args = p.parse_args()
    guard, protected = protection_guard(args.asset_protection, Path(args.plugin_directory).resolve())
    out = Path(args.report_directory).resolve()
    if not out.is_relative_to(ROOT / 'Reports'): raise SystemExit('Fresh Reports path required')
    out.mkdir(parents=True, exist_ok=False)
    project = Path(args.project).resolve(); cases = read_json(args.manifest)['cases']
    if any(x.get('blockedReport') for x in cases):
        with (out / 'BlockedContainer.txt').open('x', encoding='utf-8') as stream: stream.write('Intentional file blocks a child report directory; no ACL mutation.\n')
    results = []
    for case in cases:
        guard()
        before = files_under(project.parent / 'Content', ASSET_SUFFIXES)
        run = ROOT / case['overrideRun'] if case.get('overrideRun') else out / 'BlockedContainer.txt/Child' if case.get('blockedReport') else out / case['name']
        protected_receipt = run / 'Summary.json'
        protected_sha = digest(protected_receipt) if protected_receipt.is_file() else None
        argv = ['execute', '--operation', case['operation'], '--project',
                str(ROOT / case['overrideProject']) if case.get('overrideProject') else str(project),
                '--plugin-directory', str(ROOT / case['overrideStage']) if case.get('overrideStage') else str(Path(args.plugin_directory).resolve()),
                '--description-file', str(ROOT / case['overrideDescription']) if case.get('overrideDescription') else str(Path(args.description_file).resolve()),
                '--run-directory', str(run), '--timeout-seconds', case.get('timeout', '180')]
        if case.get('input'): argv += ['--input', str(ROOT / case['input'])]
        if case.get('source'):
            source = read_json(ROOT / case['source'], 65536)
            for field, value in case.get('replace', {}).items():
                if field not in ('assetPath', 'requestId'): raise ValueError('Only explicit negative identity fixture edits allowed')
                source[field] = value
            test_input = out / (case['name'] + 'Input.json'); write_new(test_input, source)
            argv += ['--input', str(test_input)]
        if case.get('param'): argv += ['--param', case['param']]
        process, summary = run_client(argv, out / (case['name'] + 'ClientConsole.txt'), 180)
        guard()
        errors = []
        if process.returncode != case['expectedExit']: errors.append('Expected CLI exit ' + str(case['expectedExit']))
        reported = summary.get('errors', [])
        expected_code = case.get('errorCode') or case.get('nativeCode')
        if expected_code and not any(x.get('code') == expected_code for x in reported): errors.append('Specific error code missing')
        if not reported or not reported[0].get('message') or reported[0]['message'] == 'Native operation failed': errors.append('Specific reason missing')
        if files_under(project.parent / 'Content', ASSET_SUFFIXES) != before: errors.append('Existing assets changed')
        if protected_sha and digest(protected_receipt) != protected_sha: errors.append('Existing run receipt changed')
        if case.get('nativeCode') or case['name'] in ('ExistingTarget', 'ExistingPreflight'):
            native = read_json(run / 'Native.json')
            if native.get('residue', {}).get('saveAttempted') is not False: errors.append('Refusal attempted asset save')
            if native.get('checks', {}).get('validation', {}).get('status') != 'failed': errors.append('Expected validation refusal missing')
        result = {'name': case['name'], 'status': 'failed' if errors else 'passed',
                  'cliExitCode': process.returncode, 'errors': errors, 'reportedErrors': reported,
                  'assetHashesUnchanged': files_under(project.parent / 'Content', ASSET_SUFFIXES) == before,
                  'existingSummaryUnchanged': not protected_sha or digest(protected_receipt) == protected_sha,
                  'run': str(run), 'nativeReport': summary.get('nativeReport')}
        write_new(out / (case['name'] + 'Checks.json'), result); results.append(result)
        print(json.dumps(result, ensure_ascii=False), flush=True)
        if errors or process.returncode == 5: break
    final = {'status': 'passed' if len(results) == len(cases) and all(x['status'] == 'passed' for x in results) else 'failed',
        'cases': results, 'protectedFiles': len(protected)}
    write_new(out / 'Checks.json', final)
    return 0 if final['status'] == 'passed' else 1


if __name__ == '__main__':
    sys.exit(main())
