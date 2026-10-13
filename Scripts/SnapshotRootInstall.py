"""Read/copy the current root installation for rollback; never deploy or close an Editor."""
import json, shutil
from datetime import datetime, timezone
from RunGraphRegression import ROOT, ref, sha, write_new

source = ROOT / 'Plugins/NodeForge'
out = ROOT / 'Archive/20261009BeforeCandidateDeployment'
expected = '0d74048237700b9785c5df93fd06dd9f802d0a89e1e85b19b7e1edcbbbb608a1'
dll = source / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'
if sha(dll) != expected:
    raise SystemExit('Root binary differs from observed Build2; inspect live state before making a snapshot')
paths = [p for p in source.iterdir() if p.is_file()]
for name in ['Binaries', 'Source', 'Licenses', 'Resources', 'Content', 'Config']:
    folder = source / name
    if folder.is_dir(): paths += [p for p in folder.rglob('*') if p.is_file()]
before = {p.relative_to(source).as_posix(): sha(p) for p in paths}
out.mkdir(exist_ok=False)
snapshot = out / 'NodeForge'
for p in paths:
    copy = snapshot / p.relative_to(source)
    copy.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(p, copy)
after = {p.relative_to(source).as_posix(): sha(p) for p in paths}
copied = {p.relative_to(snapshot).as_posix(): sha(p) for p in snapshot.rglob('*') if p.is_file()}
if before != after or before != copied:
    raise SystemExit('Snapshot/source SHA differs; keep evidence, do not deploy or retry over this directory')
stage = ROOT / 'Reports/20261009UsableEditor/Build29/PluginStage/NodeForge'
current_sources_match_29 = all((stage / name).is_file() and sha(stage / name) == digest
                             for name, digest in before.items() if name.startswith('Source/'))
write_new(out / 'Snapshot.json', {
    'createdUtc': datetime.now(timezone.utc).isoformat(), 'result': 'passed',
    'source': ref(source), 'snapshot': ref(snapshot), 'files': copied,
    'installedBinaryBuild': 'Build2', 'installedDllSha256': expected,
    'currentSourceMatchesFrozenBuild29': current_sources_match_29,
    'reproducibleBuild2Source': 'notProven; this is an exact current installation snapshot, not historical Build2 source',
    'sourceUnchanged': before == after, 'copyBytesEqual': before == copied,
    'excluded': ['Intermediate (rebuildable cache)'],
    'scope': 'Read-only backup preparation; no root replacement, shutdown, restore or deployment performed',
})
print(json.dumps({'result': 'passed', 'files': len(copied), 'snapshot': ref(out),
                  'sourceMatchesBuild29': current_sources_match_29}, ensure_ascii=False))
