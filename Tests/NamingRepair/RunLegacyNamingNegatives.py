"""Reuse the unchanged 11-case runner, adding actual staged-DLL observation.

No cases/assertions are copied, removed or weakened. Only the trusted process
guard's existing expected_module argument is supplied for every old-runner child.
"""
import argparse
import runpy
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'Scripts'))
import RunProcessGuard

p = argparse.ArgumentParser()
p.add_argument('--project', required=True)
p.add_argument('--plugin-directory', required=True)
p.add_argument('--report-dir', required=True)
a = p.parse_args()
project, plugin, binary = RunProcessGuard.validate_isolated_project(
    ROOT / a.project, ROOT / a.plugin_directory, ROOT)
original_guard = RunProcessGuard.run_guarded


def staged_guard(command, **kwargs):
    if Path(command[1]).resolve() != project:
        raise ValueError('Original negative runner selected an unexpected project')
    if kwargs.get('expected_module') not in [None, binary]:
        raise ValueError('Unexpected DLL selection')
    kwargs['expected_module'] = binary
    return original_guard(command, **kwargs)


RunProcessGuard.run_guarded = staged_guard
sys.argv = [str(ROOT / 'Scripts/ValidatePrototype.py'), '--project', str(project),
            '--report-dir', a.report_dir]
try:
    runpy.run_path(str(ROOT / 'Scripts/ValidatePrototype.py'), run_name='__main__')
finally:
    RunProcessGuard.run_guarded = original_guard
