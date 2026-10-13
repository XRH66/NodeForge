"""Export reviewed rule artifacts only; does not call an AI or claim external generation."""
import hashlib, json
from pathlib import Path
root = Path(__file__).resolve().parents[1]
out = root / 'Examples/MinimalAiBundle'
out.mkdir(exist_ok=False)
source = root / 'Catalog/UE532Minimal.json'
catalog = json.loads(source.read_text('utf-8-sig'))
assert catalog['catalogDigest'] == 'b0e510f2affbf6dd59e60bd60fc42c728cfcbf90ef35cfab82f09f2b2f1817d6'
(out / 'RealCatalog.json').write_bytes(source.read_bytes())
for original, name in [('Tests/Valid/MinimalBlueprint.json', 'BlueprintTemplate.json'),
        ('Tests/Valid/UiMaterialFileDark.json', 'MaterialTemplate.json')]:
    # Templates are reference structures, not external AI output and not import-ready new targets.
    (out / name).write_bytes((root / original).read_bytes())
manifest = {'engine': '5.3.2-29314046', 'capabilityRevision': 'UsablePrototype1',
    'catalogDigest': catalog['catalogDigest'], 'sourceCatalog': source.relative_to(root).as_posix(),
    'sourceCatalogSha256': hashlib.sha256(source.read_bytes()).hexdigest(),
    'implementation': 'Plugins/NodeForge/Source/NodeForgeEditor/Private/NodeForgePrototype.cpp',
    'buildEvidence': 'Reports/20261008UsablePrototype/Build8/BinaryEvidence.json',
    'scope': 'Only two fixed native structures, material constants finite 0..4; independent AI generation not yet performed'}
(out / 'Manifest.json').write_text(json.dumps(manifest, indent=2), 'utf-8')
print('Real minimal rule bundle exported; independent AI generation remains separate.')
