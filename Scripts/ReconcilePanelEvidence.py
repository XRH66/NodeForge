"""Append a routing correction without rewriting the preserved runner failure."""
import hashlib, json
from pathlib import Path
root = Path(__file__).resolve().parents[1]
out = root / 'Reports/20261008UsablePrototype/Regression3'
source = root / 'ValidationProject/RegressionRuns/Regression3/Saved/NodeForge/MinimalPanel.json'
data = json.loads(source.read_text('utf-8'))
assert data['status'] == 'passed' and data['tabClosed'] and data['widgetConstruction'] == 'passed'
steps = json.loads((out / 'Progress.json').read_text('utf-8'))
assert len(steps) == 7 and all(s['passed'] for s in steps[:-1]) and steps[-1]['exitCode'] == 0
destination = out / 'MinimalPanelRecovered.json'
with destination.open('xb') as f:
    f.write(source.read_bytes())
summary = {'result': 'passed', 'correction': 'Runner looked for NFReport; panel writes NFPanelReport or Saved/NodeForge/MinimalPanel.json. Original failed Progress.json is preserved.',
    'panelSource': source.relative_to(root).as_posix(), 'panelSourceSha256': hashlib.sha256(source.read_bytes()).hexdigest(),
    'panelRecovered': destination.relative_to(root).as_posix(), 'steps': steps,
    'fixtureHashes': {f: hashlib.sha256((root / f).read_bytes()).hexdigest() for f in
        ['Tests/Valid/MinimalBlueprint.json', 'Tests/Valid/MinimalMaterialReady.json']}}
with (out / 'SummaryCorrected.json').open('x', encoding='utf-8') as f:
    json.dump(summary, f, indent=2)
print('Actual original panel test passed; correction appended, prior evidence preserved.')
