"""Freeze the already tested Build29 without rebuilding or touching a running install."""
import hashlib, json, shutil
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'Reports/20261009CandidateRC1'
STAGE = ROOT / 'Reports/20261009UsableEditor/Build29/PluginStage/NodeForge'
DLL = 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'
EXPECTED = '599ddb134242ca8c4726a03d3173872ce37eedd74aba31fae13df78bbe8152a4'
checks = [
    'Reports/20261009UsableEditor/BlueprintExport3/Summary.json',
    'Reports/20261008GraphUpgrade/BPExportRuntime1/Summary.json',
    'Reports/20261008GraphV2/BPExportCore1/Summary.json',
    'Reports/20261008UsablePrototype/BPExportLegacy1/Summary.json',
    'Reports/20261009UsableEditor/BPExportLegacyNeg1/Summary.json',
    'Reports/20261009UsableEditor/MaterialExport4/Summary.json',
]
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def write(path, data):
    with path.open('x', encoding='utf-8', newline='\n') as stream:
        json.dump(data, stream, ensure_ascii=False, indent=2)

assert sha(STAGE / DLL) == EXPECTED
results = []
for name in checks:
    data = json.loads((ROOT / name).read_text('utf-8'))
    assert data['result'] == 'passed', name
    results.append({'path': name, 'sha256': sha(ROOT / name), 'result': data['result']})
root_dll = ROOT / 'Plugins/NodeForge' / DLL
root_before = sha(root_dll)
OUT.mkdir(exist_ok=False)
plugin = OUT / 'PluginStage/NodeForge'
shutil.copytree(STAGE, plugin)
descriptor = plugin / 'NodeForge.uplugin'
data = json.loads(descriptor.read_text('utf-8-sig'))
data.update(Version=20001, VersionName='0.2.0-rc.1',
            Description='UE 5.3.2 offline JSON graph candidate; Build29, real UI acceptance pending')
descriptor.write_text(json.dumps(data, ensure_ascii=False, indent=2), 'utf-8')
for source in ['Catalog/UE532Graph.json', 'Contracts/GraphProtocol.md', 'Examples/GraphAiRules.md',
               'SourceData/ExternalGraphAi2/Material.json', 'SourceData/ExternalGraphAi2/Blueprint.json',
               'SourceData/ExternalGraphAi2/Readme.md', 'SourceData/ExternalGraphAi2/Provenance.json']:
    origin = ROOT / source
    if not origin.is_file():
        if source.endswith('Provenance.json'): continue
        raise FileNotFoundError(source)
    target = OUT / 'TrialInputs' / source
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(origin, target)
actual = ROOT / 'Reports/20261009UsableEditor/BlueprintExport3/ActualCatalog.json'
shutil.copyfile(actual, OUT / 'ActualCatalog.json')
project_dir = ROOT / 'ValidationProject/RegressionRuns/CandidateRC1'
project_dir.mkdir(exist_ok=False)
project = project_dir / 'NFCandidateRC1.uproject'
write(project, {'FileVersion': 3, 'EngineAssociation': '5.3',
               'AdditionalPluginDirectories': [str(plugin.parent)],
               'Plugins': [{'Name': 'NodeForge', 'Enabled': True}]})
licenses = []
for project_name, archived, filename in [
    ('ZiggyMar', 'ZiggyMarD1a85396', 'ZiggyMarMIT.txt'),
    ('ChiR24', 'ChiR24591d3b0e', 'ChiR24MIT.txt'),
    ('Autonomix', 'Autonomix47a46ae8', 'AutonomixMIT.txt')]:
    original = ROOT / 'References/Originals' / archived / 'LICENSE'
    target = plugin / 'Licenses' / filename
    assert sha(original) == sha(target)
    licenses.append({'project': project_name, 'original': original.relative_to(ROOT).as_posix(),
                     'copied': target.relative_to(ROOT).as_posix(), 'sha256': sha(target)})
assert sha(root_dll) == root_before
assert sha(plugin / DLL) == EXPECTED
write(OUT / 'Manifest.json', {
    'version': '0.2.0-rc.1', 'frozenUtc': datetime.now(timezone.utc).isoformat(),
    'engine': '5.3.2-29314046', 'binaryBuild': 'Build29', 'dllSha256': EXPECTED,
    'plugin': plugin.relative_to(ROOT).as_posix(), 'project': project.relative_to(ROOT).as_posix(),
    'rootDllUnchanged': True, 'rootDllSha256': root_before,
    'metadataChangeOnly': ['Version', 'VersionName', 'Description'],
    'tests': results, 'licenses': licenses,
    'files': {p.relative_to(OUT).as_posix(): sha(p) for p in OUT.rglob('*') if p.is_file()},
    'acceptance': {'backend': 'passed', 'realUi': 'notRun', 'candidateDelivery': 'notRun'},
    'scope': 'Candidate for isolated local trial. Not a claim of UI acceptance or portable installation.',
})
print(json.dumps({'version': '0.2.0-rc.1', 'project': str(project), 'dllSha256': EXPECTED,
                  'realUi': 'notRun'}, ensure_ascii=False))
