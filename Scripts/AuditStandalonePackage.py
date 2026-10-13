"""Audit native pak contents against a verified copy manifest; no extraction or asset writes."""
import argparse
import re
import subprocess
from pathlib import Path
from RunGraphRegression import ROOT, read_json, write_new, sha, ref

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--preparation', required=True)
parser.add_argument('--package-root', required=True)
parser.add_argument('--out', required=True)
args = parser.parse_args()
prep_path = (ROOT / args.preparation).resolve()
package = (ROOT / args.package_root).resolve()
out = (ROOT / args.out).resolve()
if not prep_path.is_relative_to(ROOT / 'Reports') or not prep_path.is_file():
    raise SystemExit('Existing project-owned copy preparation required')
if not package.is_relative_to(ROOT / 'Builds') or not package.is_dir():
    raise SystemExit('Existing isolated Builds package required')
if not out.is_relative_to(ROOT / 'Reports') or out.exists():
    raise SystemExit('Fresh Reports audit directory required')
preparation = read_json(prep_path)
project = (ROOT / preparation['project']).resolve()
if not project.is_relative_to(ROOT / 'ValidationProject/StandaloneChecks') or not project.is_file():
    raise SystemExit('Isolated no-plugin copy project required')
paks = sorted((package / project.stem / 'Content/Paks').glob('*.pak'))
if len(paks) != 1:
    raise SystemExit('Expected exactly one native non-IoStore pak; do not silently guess a package')
pak = paks[0]
native = Path('X:/UnrealEngine/UE_5.3/Engine/Binaries/Win64/UnrealPak.exe')
if not native.is_file():
    raise SystemExit('Existing UE5.3.2 native pak tool unavailable; do not install')
out.mkdir(parents=True, exist_ok=False)
command = [str(native), str(pak), '-List']
before = sha(pak)
write_new(out / 'Command.json', command)
with (out / 'PakList.txt').open('xb') as stream:
    executed = subprocess.run(command, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT,
                              timeout=120, creationflags=subprocess.CREATE_NO_WINDOW)
text = (out / 'PakList.txt').read_text('utf-8-sig', errors='replace')
entries = re.findall(r'LogPakFile: Display: "([^"]+)" offset:', text)
entry_set = set(entries)
errors = []
if executed.returncode != 0 or not entries:
    errors.append('Native pak list failed or no actual file entries were parsed')
generator = [entry for entry in entries if '/plugins/nodeforge/' in ('/' + entry).lower()
             or Path(entry).name.lower() in ('nodeforge.uplugin', 'unrealeditor-nodeforgeeditor.dll')]
if generator:
    errors.append('Generator entries present in pak')
generator_files = [ref(path) for path in package.rglob('*') if path.is_file()
                   and (path.name == 'NodeForge.uplugin' or 'NodeForgeEditor' in path.name)]
if generator_files:
    errors.append('Generator files present outside pak')
assets = []
for item in preparation['assets']:
    source, copy = (ROOT / item['source']).resolve(), (ROOT / item['copy']).resolve()
    if not source.is_relative_to(ROOT / 'ValidationProject/RegressionRuns') \
            or not copy.is_relative_to(project.parent / 'Content'):
        raise SystemExit('Preparation asset escaped the isolated source/copy scope')
    expected = project.stem + '/Content/' + copy.relative_to(project.parent / 'Content').as_posix()
    row = {'path': expected, 'present': expected in entry_set,
           'sourceBytesUnchanged': source.is_file() and sha(source) == item['sha256'],
           'copyBytesUnchanged': copy.is_file() and sha(copy) == item['sha256']}
    assets.append(row)
    if not all(row[key] for key in ('present', 'sourceBytesUnchanged', 'copyBytesUnchanged')):
        errors.append('Expected native asset missing or source/copy bytes changed: ' + expected)
if sha(pak) != before:
    errors.append('Read-only list changed pak bytes')
summary = {'result': 'passed' if not errors else 'failed', 'errors': errors,
           'preparation': ref(prep_path), 'pak': ref(pak), 'pakSha256': before,
           'nativeExitCode': executed.returncode, 'nativeEntryCount': len(entries),
           'assets': assets, 'generatorEntries': generator, 'generatorFiles': generator_files,
           'scope': 'Actual cooked pak paths plus unchanged source/copy bytes; cooked bytes are not claimed equal to uncooked assets; runtime separate'}
write_new(out / 'Summary.json', summary)
print(summary['result'], 'entries', len(entries), 'assets', len(assets), 'errors', errors, flush=True)
raise SystemExit(0 if not errors else 1)
