"""Read actual engine screenshots/logs and compare delivery copy bytes; never alters evidence."""
import hashlib, json, re
from pathlib import Path
from PIL import Image, ImageStat
root = Path(__file__).resolve().parents[1]
out = root / 'Reports/20261008UsablePrototype'

def picture(name):
    path = out / name
    image = Image.open(path).convert('RGB')
    w, h = image.size
    side = min(w, h)
    x0, y0 = (w-side)/2, (h-side)/2
    values = []
    for y in [0.25, 0.75]:
        row = []
        for x in [0.25, 0.75]:
            cx, cy = round(x0+side*x), round(y0+side*y)
            row.append(ImageStat.Stat(image.crop((cx-10,cy-10,cx+10,cy+10))).mean)
        values.append(row)
    assert all(max(abs(row[0][i]-row[1][i]) for i in range(3)) <= 3 for row in values), values
    assert min(values[1][0]) - max(values[0][0]) >= 80, values
    return {'image': path.relative_to(root).as_posix(), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
        'dimensions': [w,h], 'srgbCenterPatches': values,
        'criterion': 'Native fixed camera: generated/reference RGB agree within 3/255 each row; bright minus dark >=80/255. sRGB screenshot values are not linear emissive values.'}

def runtime(name):
    path = out / name
    text = path.read_text('utf-8', errors='replace')
    pattern = r'LogBlueprintUserMessages: \[NFUiBlueprintReady2_C_0\] NodeForge Blueprint Runtime OK'
    lines = [line for line in text.splitlines() if re.search(pattern,line)]
    assert len(lines) == 1, lines
    assert 'NodeForge Editor module loaded' not in text and 'Mounting External plugin NodeForge' not in text
    assert 'Fatal error:' not in text
    return {'log': path.relative_to(root).as_posix(), 'actualBlueprintMessageCount': len(lines), 'attributedActor': 'NFUiBlueprintReady2_C_0',
        'generatorLoaded': False, 'actualMessage': lines[0]}

prep = json.loads((out / 'StandalonePreparation.json').read_text('utf-8-sig'))
assert all(hashlib.sha256((root / entry[key]).read_bytes()).hexdigest()==entry['sha256']
    for entry in prep['assets'] for key in ['source','copy'])
modules = json.loads((out / 'StandaloneModules1.json').read_text('utf-8-sig'))
packaged = json.loads((out / 'PackageModules1.json').read_text('utf-8-sig'))
assert not modules['generatorModules'] and not packaged['generatorModules']
summary = {'result': 'passed', 'copiedAssets': len(prep['assets']), 'sourceAndCopyBytesUnchanged': True,
    'editor': {'visual': picture('StandalonePie.png'), 'runtime': runtime('StandaloneUi1.log'), 'moduleEvidence': 'StandaloneModules1.json'},
    'packagedWin64': {'visual': picture('PackageRuntime.png'), 'runtime': runtime('PackageRuntime1.log'), 'moduleEvidence': 'PackageModules1.json'},
    'limitations': 'Same machine D3D11 Win64 Development. No clean second machine or Shipping configuration tested. Human independent review not performed.'}
with (out / 'DeliverySummary.json').open('x', encoding='utf-8') as f:
    json.dump(summary,f,indent=2)
print('Independent editor and actual Win64 package delivery checks passed.')
