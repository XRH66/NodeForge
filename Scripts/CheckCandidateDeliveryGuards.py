"""Safety/contract probes only; no UE, assets or generated success receipts."""
import argparse, copy, json, subprocess, sys
from RunGraphRegression import ROOT, read_json, ref, sha, write_new
from StandaloneCases import load_cases, load_ui_cases, validate_ui_delivery

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--run', required=True)
args = parser.parse_args()
if not args.run.isascii() or not args.run.isalnum() or len(args.run) > 20:
    raise SystemExit('Fresh bounded ASCII report name required')
out = ROOT / 'Reports/20261009CandidateRC1' / args.run
out.mkdir(exist_ok=False)
base = {'format': 'NodeForgeUiCandidateCases', 'version': 1, 'cases': [
    {'case': 'Blueprint', 'map': '/Game/NodeForgeTests/BPMap', 'assetType': 'Blueprint',
     'assetPath': '/Game/NodeForgeTests/BP', 'expectedMessage': 'Explicit expected message'},
    {'case': 'Material', 'map': '/Game/NodeForgeTests/MatMap', 'assetType': 'Material',
     'assetPath': '/Game/NodeForgeTests/Mat', 'expectedMessage': None},
]}
tests = []
def probe(name, edit, accepted=False):
    data = copy.deepcopy(base)
    edit(data)
    path = out / (name + '.json')
    write_new(path, data)
    try:
        load_ui_cases(path)
        actual = True
    except ValueError:
        actual = False
    tests.append({'case': name, 'expectedAccept': accepted, 'actualAccept': actual, 'passed': actual == accepted})
probe('ValidSchemaOnly', lambda d: None, True)
probe('WrongVersion', lambda d: d.update(version=2))
probe('BooleanVersion', lambda d: d.update(version=True))
probe('MissingMaterial', lambda d: d['cases'].pop())
probe('DuplicateKind', lambda d: d['cases'][1].update(assetType='Blueprint'))
probe('DuplicateMap', lambda d: d['cases'][1].update(map=d['cases'][0]['map']))
probe('OutsideAsset', lambda d: d['cases'][0].update(assetPath='/Game/Production/BP'))
probe('UnknownField', lambda d: d['cases'][0].update(command='not allowed'))
probe('EmptyMessage', lambda d: d['cases'][0].update(expectedMessage=''))
probe('MultilineMessage', lambda d: d['cases'][0].update(expectedMessage='one\ntwo'))
probe('MaterialMessage', lambda d: d['cases'][1].update(expectedMessage='fake blueprint result'))
probe('UnknownKind', lambda d: d['cases'][1].update(assetType='MaterialInstance'))
legacy = ROOT / 'Reports/20261009UsableEditor/DeliveryStandalone1/RuntimeCases.json'
before = sha(legacy)
original = load_cases(legacy)
tests.append({'case': 'LegacyNineCaseContract', 'count': len(original),
              'sha256': before, 'passed': len(original) == 9 and sha(legacy) == before})
project = ROOT / 'ValidationProject/StandaloneChecks/CandidateDelivery1/CandidateDelivery1.uproject'
manifest = out / 'ValidSchemaOnly.json'
try:
    validate_ui_delivery({'result': 'prepared'}, project, manifest, load_ui_cases(manifest))
    refused = False
except ValueError:
    refused = True
tests.append({'case': 'PreparedFlagCannotReplaceUiEvidence', 'passed': refused})
assert not project.parent.exists(), 'Guard probe must not overwrite an existing candidate delivery project'
command = [sys.executable, str(ROOT / 'Scripts/PrepareCandidateStandalone.py'),
           '--ui-run', 'Reports/20261009CandidateRC1/CandidateVerify1', '--run', 'CandidateDelivery1']
with (out / 'MissingUiConsole.txt').open('xb') as stream:
    process = subprocess.run(command, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT, timeout=20)
text = (out / 'MissingUiConsole.txt').read_text('utf-8')
tests.append({'case': 'MissingActualUiBlocksCopy', 'exitCode': process.returncode,
              'passed': process.returncode == 1 and 'Summary absent' in text and not project.parent.exists()
                        and not (ROOT / 'Reports/20261009CandidateRC1/CandidateDelivery1').exists()})
write_new(out / 'Command.json', command)
good = all(t['passed'] for t in tests)
write_new(out / 'Summary.json', {'result': 'passed' if good else 'failed', 'tests': tests,
          'scope': 'Synthetic case-schema negatives, old real manifest unchanged and actual missing-UI copy refusal. No UE launch/import/runtime/package/visual evidence.'})
print(json.dumps({'result': 'passed' if good else 'failed', 'count': len(tests), 'evidence': ref(out)}, ensure_ascii=False))
sys.exit(0 if good else 1)
