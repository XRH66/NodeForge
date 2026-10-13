"""Run native v2 assets without NodeForge; use UE's own bounded game loop/capture."""
import argparse
import json
import re
import subprocess
from RunGraphRegression import ROOT, ENGINE, ASSET_SUFFIXES, files_under, read_json, ref, write_new, sha, build_snapshot
from StandaloneCases import load_cases, load_ui_cases, validate_ui_delivery, candidate_files
from RunProcessGuard import run_guarded


def blueprint_user_messages(text):
    """Keep each complete native payload, including significant message spaces."""
    return [line.split('LogBlueprintUserMessages:', 1)[1].removeprefix(' ')
            for line in text.splitlines() if 'LogBlueprintUserMessages:' in line]


def native_map_loaded(text, map_path):
    """Require the exact live world's load marker, not a command-line mention."""
    return re.search(r'LogWorld: Bringing World ' + re.escape(map_path)
                     + r'\.[A-Za-z0-9_]+ up for play(?:\s|\(|$)', text) is not None

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--project', required=True)
parser.add_argument('--out', required=True)
parser.add_argument('--packaged-root', help='Existing staged Windows directory under Builds; run native packaged UnrealGame directly')
parser.add_argument('--visual-only', action='store_true', help='Saved visual map runtime only; no six-blueprint claim')
parser.add_argument('--case-manifest', help='Hash-locked RuntimeCases.json from explicit blueprint delivery preparation; no fixed six-case mapping')
parser.add_argument('--ui-case-manifest', help='Hash-locked representative actual UI Blueprint/Material preparation')
parser.add_argument('--personal-case-manifest', help='Finite same-byte PC Material + reused Tick in one native map')
args = parser.parse_args()
project = (ROOT / args.project).resolve()
out = (ROOT / args.out).resolve()
if not project.is_relative_to(ROOT / 'ValidationProject/StandaloneChecks') or not project.is_file():
    raise SystemExit('Existing standalone copy required')
if not out.is_relative_to(ROOT / 'Reports') or out.exists():
    raise SystemExit('Fresh Reports directory required')
descriptor = read_json(project)
if descriptor.get('AdditionalPluginDirectories') or (project.parent / 'Plugins').exists():
    raise SystemExit('Generator search paths/files must be absent')
if not any(p.get('Name') == 'NodeForge' and p.get('Enabled') is False for p in descriptor.get('Plugins', [])):
    raise SystemExit('NodeForge must explicitly be disabled')
before = files_under(project.parent / 'Content', ASSET_SUFFIXES)
package = (ROOT / args.packaged_root).resolve() if args.packaged_root else None
package_before = {}
if package:
    if not package.is_relative_to(ROOT / 'Builds') or not package.is_dir():
        raise SystemExit('Existing isolated packaged Builds directory required')
    executable = package / 'Engine/Binaries/Win64/UnrealGame.exe'
    if not executable.is_file() or not (package / project.stem / 'Content/Paks').is_dir():
        raise SystemExit('Native pure-blueprint package layout missing')
    if any('NodeForgeEditor' in p.name or p.name == 'NodeForge.uplugin' for p in package.rglob('*') if p.is_file()):
        raise SystemExit('Generator found in packaged files')
    package_before = files_under(package, {'.exe', '.dll', '.pak', '.ucas', '.utoc'})
legacy_cases = {'BlueprintText': 'NodeForge data-driven greeting',
         'BlueprintTextChanged': 'A different JSON produced this message',
         'BlueprintBranch': 'Message set by graph input',
         'BlueprintBranchFalse': 'Disabled branch ran',
         'BlueprintConcat': 'Data-driven function output',
         'BlueprintZeroDefaults': 'Disabled branch ran', 'Water': None}
if args.visual_only:
    legacy_cases = {'Water': None}
cases = [{'case': name, 'expectedMessage': message,
          'map': '/Game/NodeForgeTests/NF'+name+'Map' if message else '/Game/NodeForgeTests/NFGraphVisualMap',
          'actorName': 'NFGraph'+name+'_C_0', 'expectedLocation': None} for name, message in legacy_cases.items()]
manifest = None
preparation = None
validated_build = None
selected = None
if args.personal_case_manifest:
    if args.case_manifest or args.ui_case_manifest or args.visual_only:
        raise SystemExit('Finite PC map cannot mix with legacy/UI matrices')
    from PersonalCandidateStandalone import load_runtime, selected_files, masked_picture
    manifest = (ROOT / args.personal_case_manifest).resolve()
    cases, preparation, selected, validated_build = load_runtime(manifest, project)
if args.ui_case_manifest:
    if args.case_manifest or args.visual_only:
        raise SystemExit('UI candidate cases cannot mix with legacy/location cases')
    manifest = (ROOT / args.ui_case_manifest).resolve()
    cases = load_ui_cases(manifest)
    preparation = read_json(manifest.parent / 'Preparation.json')
    selected = validate_ui_delivery(preparation, project, manifest, cases)
    validated_build = candidate_files(selected)
    for item in cases:
        item['actorName'] = item['assetPath'].split('/')[-1]+'_C_0' if item['assetType'] == 'Blueprint' else None
        item['expectedLocation'] = None
if args.case_manifest:
    if args.visual_only:
        raise SystemExit('Explicit blueprint cases cannot mix with visual-only scope')
    manifest = (ROOT / args.case_manifest).resolve()
    cases = load_cases(manifest)
    preparation = read_json(manifest.parent / 'Preparation.json')
    if preparation.get('result') != 'prepared' or preparation.get('project') != ref(project) \
        or preparation.get('caseManifest') != ref(manifest) or preparation.get('caseManifestSha256') != sha(manifest) \
        or preparation.get('maps') != [c['map'] for c in cases] or len(preparation.get('nativeLocationProbeChecks', [])) != len(cases):
        raise SystemExit('Explicit cases must match actual hash-locked blueprint delivery preparation')
    for item in preparation['assets']:
        source, copied = (ROOT / item['source']).resolve(), (ROOT / item['copy']).resolve()
        if not source.is_relative_to(ROOT / 'ValidationProject/RegressionRuns') or not copied.is_relative_to(project.parent / 'Content') \
            or sha(source) != item['sha256'] or sha(copied) != item['sha256']:
            raise SystemExit('Explicit source/copy assets changed since preparation')
    graph = read_json(ROOT / 'Reports/20261008GraphV2' / preparation['graphRun'] / 'Summary.json')
    selected = ROOT / graph['selectedPlugin']
    validated_build = build_snapshot(selected)
    if validated_build != preparation['validatedBuild']:
        raise SystemExit('Current build differs from explicit delivery evidence')
    for item in cases:
        item['actorName'] = item['blueprint'].split('/')[-1]+'_C_0'
for item in cases:
    if not (project.parent / 'Content' / (item['map'].removeprefix('/Game/')+'.umap')).is_file():
        raise SystemExit('Native map missing before any game is launched: '+item['map'])
out.mkdir(parents=True)
write_new(out / 'Baseline.json', {'assets': before, 'descriptor': descriptor, 'packagedFiles': package_before,
          'caseManifest': ref(manifest) if manifest else None, 'caseManifestSha256': sha(manifest) if manifest else None,
          'validatedBuild': validated_build})
steps = []
runner_hashes = {ref(path): sha(path) for path in (ROOT / 'Scripts/RunStandaloneGraphChecks.py', ROOT / 'Scripts/RunProcessGuard.py')}
if args.personal_case_manifest:
    runner_hashes[ref(ROOT / 'Scripts/PersonalCandidateStandalone.py')] = sha(ROOT / 'Scripts/PersonalCandidateStandalone.py')
def current_validated_build():
    if args.personal_case_manifest:
        return selected_files(ref(selected))
    return candidate_files(selected) if args.ui_case_manifest else build_snapshot(selected)
for item in cases:
    case, message, map_path = item['case'], item['expectedMessage'], item['map']
    if (read_json(project) != descriptor or before != files_under(project.parent / 'Content', ASSET_SUFFIXES)
            or (package and package_before != files_under(package, {'.exe', '.dll', '.pak', '.ucas', '.utoc'}))
            or (validated_build is not None and current_validated_build() != validated_build)):
        raise SystemExit('Protected project/assets/package/build changed before the next child; stop batch')
    if not (project.parent / 'Content' / (map_path.removeprefix('/Game/') + '.umap')).exists():
        raise SystemExit('Native map missing: ' + map_path)
    log_path = out / (case + '.log')
    # Match local UE5.3.2 WinPlatform.Automation.cs bootstrap arguments for a pure BP project.
    # Invoke its actual engine executable so Popen owns the runtime PID and observes its true exit.
    command_head = [str(executable), '../../../' + project.stem + '/' + project.stem + '.uproject'] if package else [str(ENGINE), str(project)]
    cmd = command_head + [map_path, '-game', '-d3d11', '-RenderOffscreen',
           '-unattended', '-nop4', '-nosound', '-NoLiveCoding', '-seconds=4',
           '-abslog=' + str(log_path), '-stdout', '-FullStdOutLogOutput', '-FORCELOGFLUSH']
    image = out / (case+'.png')
    if message is None or args.personal_case_manifest:
        cmd.append('-ExecCmds=HighResShot '+('256x256' if args.personal_case_manifest else '640x640')+' filename=' + str(image))
    write_new(out / (case + 'Command.json'), cmd)
    errors, module_snapshot = [], None

    def observe_native_modules(process):
        global module_snapshot
        text = log_path.read_text('utf-8-sig', errors='replace') if log_path.exists() else ''
        if module_snapshot is not None or not native_map_loaded(text, map_path):
            return
        # Only this child PID is queried. The short timeout affects the read-only
        # inspector, never the engine; any error is checkpointed by run_guarded.
        query = 'Get-Process -Id ' + str(process.pid) + ' -ErrorAction Stop | ForEach-Object { $_.Modules | Select-Object -ExpandProperty FileName } | ConvertTo-Json'
        inspected = subprocess.run(['powershell', '-NoProfile', '-Command', query],
                                   capture_output=True, text=True, timeout=15,
                                   creationflags=subprocess.CREATE_NO_WINDOW)
        if inspected.returncode != 0:
            raise OSError('Read-only child module inspector failed')
        modules = json.loads(inspected.stdout)
        if isinstance(modules, str):
            modules = [modules]
        if not isinstance(modules, list) or not modules or any(not isinstance(path, str) or not path for path in modules):
            raise ValueError('Nonempty actual child module paths required')
        write_new(out / (case + 'Modules.json'), {'pid': process.pid, 'modules': modules})
        module_snapshot = modules

    from NodeForgeControl import assert_exclusive
    assert_exclusive(project)
    process = run_guarded(cmd, cwd=executable.parent if package else ROOT,
                          console=out / (case + 'Console.txt'), timeout=180,
                          observer=observe_native_modules)
    code = process.returncode
    text = log_path.read_text('utf-8-sig', errors='replace') if log_path.exists() else ''
    if code != 0 or not native_map_loaded(text, map_path):
        errors.append('Native game/map did not finish normally')
    if 'Mounting Project plugin NodeForge' in text or 'NodeForge Editor module loaded' in text:
        errors.append('Generator loaded unexpectedly')
    if not module_snapshot or any('NodeForgeEditor' in path for path in module_snapshot):
        errors.append('Live module absence not proven')
    if package and (not module_snapshot or str(executable).lower() not in [p.lower() for p in module_snapshot]):
        errors.append('Packaged executable identity not proven')
    actual_location = None
    if message is not None:
        exact = '['+item['actorName']+'] '+message
        payloads = blueprint_user_messages(text)
        matched = [payload for payload in payloads if payload == exact]
        if payloads != [exact]:
            errors.append('Exact generated actor BeginPlay message not observed once')
        if item.get('expectedLocation') is not None:
            # This is native live GetActorLocation dataflow, not a configured literal.
            # The preparation verifies the compiled/saved graph wiring independently.
            match = re.search(r'\['+re.escape(item['actorName'])+r'\] X=(-?\d+\.\d{3}) Y=(-?\d+\.\d{3}) Z=(-?\d+\.\d{3})\s*$', matched[0]) if len(matched)==1 else None
            actual_location = [float(v) for v in match.groups()] if match else None
            if actual_location is None or any(abs(a-b)>0.0005001 for a,b in zip(actual_location,item['expectedLocation'])):
                errors.append('Native self location readback missing or differs at its three-decimal text precision')
    elif not image.is_file():
        errors.append('Native material screenshot not captured')
    visual = None
    if args.personal_case_manifest:
        if not image.is_file():
            errors.append('Actual finite map screenshot missing')
        else:
            visual = masked_picture(image)
            if not visual['passed']:
                errors.append('Frozen Masked on/off/background pixel criteria failed')
    if before != files_under(project.parent / 'Content', ASSET_SUFFIXES):
        errors.append('Native game changed saved asset bytes')
    step = {'case': case, 'result': 'passed' if not errors else 'failed', 'errors': errors,
            'pid': process.pid, 'exitCode': code, 'map': map_path, 'expectedMessage': message,
            'expectedLocation': item.get('expectedLocation'), 'actualLocationFromNativeReadback': actual_location,
            'exactMessagePayloads': blueprint_user_messages(text) if message is not None else None,
            'locationEvidence': 'Prepared verified native self GetActorLocation dataflow; FVector ToString three-decimal output' if args.case_manifest else 'notRequested',
            'reportLog': ref(log_path), 'screenshot': ref(image) if (not message or args.personal_case_manifest) and image.exists() else None,
            'visualReview': visual if args.personal_case_manifest else 'notRun' if not message else 'notApplicable'}
    steps.append(step)
    print(case, step['result'], errors, flush=True)
after = files_under(project.parent / 'Content', ASSET_SUFFIXES)
package_after = files_under(package, {'.exe', '.dll', '.pak', '.ucas', '.utoc'}) if package else {}
explicit_source_unchanged = not preparation or all(sha(ROOT / p['source']) == p['sha256'] and sha(ROOT / p['copy']) == p['sha256'] for p in preparation['assets'])
build_unchanged = validated_build is None or current_validated_build() == validated_build
good = len(steps) == len(cases) and all(step['result'] == 'passed' for step in steps) and before == after and package_before == package_after and explicit_source_unchanged and build_unchanged
write_new(out / 'Summary.json', {'result': 'passed' if good else 'failed', 'project': ref(project),
          'cases': steps, 'pluginEnabled': False, 'pluginFilesPresent': False,
          'assetsBefore': before, 'assetsAfter': after, 'allAssetBytesUnchanged': before == after,
          'packagedRoot': ref(package) if package else None, 'packagedFilesBefore': package_before,
          'packagedFilesAfter': package_after, 'allPackagedBytesUnchanged': package_before == package_after,
          'scope': 'Packaged UnrealGame runtime' if package else 'Native Editor -game runtime',
          'visualOnly': args.visual_only,
          'caseManifest': ref(manifest) if manifest else None, 'caseManifestSha256': sha(manifest) if manifest else None,
          'explicitSourceBytesUnchanged': explicit_source_unchanged, 'oneUnchangedBuild': build_unchanged,
          'uiCandidateCases': bool(args.ui_case_manifest),
          'personalCandidateCase': bool(args.personal_case_manifest),
          'runnerHashes': runner_hashes,
          'limitations': ('One frozen Masked map/image and unchanged BT BeginPlay attribution; exact Tick speed reused from BT4, not remeasured here' if args.personal_case_manifest else ('Representative raw UI Blueprint message and Material single-frame capture; no all-assets or packed animation claim' if args.ui_case_manifest else ('Explicit prepared blueprint cases and native self position at FVector three-decimal text precision; separate PIE uses 1e-6' if manifest else ('Visual map and live module absence only; no blueprint runtime tested' if args.visual_only else 'Six exact BeginPlay messages and live module absence')))) + '; same-machine Win64 D3D11 only; no-plugin Editor asset tabs and packaged bootstrap not exercised'})
raise SystemExit(0 if good else 1)
