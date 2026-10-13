"""Project-owned sequential scene/runtime checks; no build, install or asset overwrite."""
import argparse, copy, hashlib, json, math, re, subprocess, sys, time
from pathlib import Path
from RunProcessGuard import run_guarded, validate_isolated_project
from RunGraphRegression import automation_report_passed

ROOT = Path(__file__).resolve().parents[1]
ENGINE = Path('X:/UnrealEngine/UE_5.3/Engine/Binaries/Win64/UnrealEditor-Cmd.exe')

def blueprint_runtime_checks(data, pid, config, config_path, log_text, prior_pids=()):
    """Adapt the existing R1 exact PIE assertions to an explicit caller-selected graph."""
    actor = data.get('actualActor', '')
    blueprint_name = config['blueprint'].rsplit('/', 1)[-1]
    map_name = config['map'].rsplit('/', 1)[-1]
    class_path = config['blueprint'] + '.' + blueprint_name + '_C'
    payload = '[' + actor.rsplit('.', 1)[-1] + '] ' + config['expectedMessage']
    observed = data.get('observedSeconds')
    logged = [line.split('LogBlueprintUserMessages:', 1)[1].strip()
              for line in log_text.splitlines() if 'LogBlueprintUserMessages:' in line]
    return {
        'exactRuntimePid': data.get('pid') == pid and pid not in prior_pids,
        'exactConfigAndMap': data.get('graphConfigRequested') is True
            and data.get('configValidation') == 'passed'
            and Path(data.get('configPath', '')).resolve() == config_path.resolve()
            and data.get('requestedMap') == data.get('actualMap') == config['map']
            and data.get('blueprint') == config['blueprint'],
        'oneRealActor': data.get('actorCount') == 1 and data.get('hasActorBegunPlay') is True
            and data.get('actualClass') == data.get('expectedClass') == class_path,
        'exactCaseMessageOnce': data.get('expectedMessage') == config['expectedMessage']
            and data.get('messageCount') == data.get('attributedMessageCount') == 1
            and data.get('engineMessages') == [payload],
        'actualAttribution': bool(actor) and data.get('sourceActor') == actor
            and data.get('sourceClass') == class_path and data.get('attributionFailures') == []
            and data.get('sourceFunction') == 'ExecuteUbergraph_' + blueprint_name
            and data.get('sourceWorld') == data.get('actualPIEMap', '') + '.' + map_name,
        'realPieMap': data.get('worldType') == 'PIE' and bool(re.fullmatch(
            re.escape(config['map'].rsplit('/', 1)[0]) + '/UEDPIE_[0-9]+_' + re.escape(map_name),
            data.get('actualPIEMap', ''))),
        'stoppedAndLoggerRemoved': data.get('pieStopped') is True and data.get('loggerRemoved') is True
            and type(observed) in (int, float) and math.isfinite(observed) and observed >= 2,
        'actualNativeLogOnce': logged == [payload] and 'TEST COMPLETE. EXIT CODE: 0' in log_text,
    }

parser = argparse.ArgumentParser()
parser.add_argument('--run', required=True)
source_args = parser.add_mutually_exclusive_group(required=True)
source_args.add_argument('--graph-run')
source_args.add_argument('--export-run', help='Passing actual material roundtrip matrix; visual-only required')
source_args.add_argument('--blueprint-export-run', help='Passing actual blueprint roundtrip matrix; blueprint-only required')
source_args.add_argument('--preview-run', help='Passing actual new-copy preview smoke; two blueprint paths and material GPU, no UI claim')
source_args.add_argument('--ui-run', help='Passing exact UI receipt independent reopen; explicit visual-only or blueprint-only, same DLL and saved bytes')
parser.add_argument('--visual-only', action='store_true', help='Two native visual steps; no claim of blueprint runtime coverage')
parser.add_argument('--blueprint-only', action='store_true', help='Only independent saved blueprint PIE; no material visual claim')
parser.add_argument('--material', default='/Game/NodeForgeTests/NFGraphWater', help='Actual generated material asset in the selected frozen matrix')
parser.add_argument('--visual-config', help='Existing workspace JSON with explicit scalar/vector changes for this selected material')
parser.add_argument('--blueprint-message', help='Exact expected attributed message; required for an independently reopened UI Blueprint')
args = parser.parse_args()
source_run = args.graph_run or args.export_run or args.blueprint_export_run or args.preview_run or args.ui_run
if args.ui_run and args.visual_only == args.blueprint_only:
    raise SystemExit('UI-saved scene slice requires exactly visual-only or blueprint-only')
if args.ui_run and args.blueprint_only and not args.blueprint_message:
    raise SystemExit('Exact expected UI Blueprint message required')
if not args.run.isascii() or not args.run.isalnum() or not source_run.isascii() or not source_run.isalnum():
    raise SystemExit('Use new ASCII alphanumeric run names')
out = ROOT / 'Reports/20261008GraphUpgrade' / args.run
out.mkdir(exist_ok=False)
graph = ROOT / ('Reports/20261008GraphV2' if args.graph_run else 'Reports/20261009UsableEditor') / source_run
summary = json.loads((graph / 'Summary.json').read_text('utf-8'))
if summary.get('result') != 'passed' or (args.graph_run and not summary.get('fullMatrix')):
    raise SystemExit('A complete passing frozen graph matrix is required')
project = ROOT / (summary['verificationProject'] if args.ui_run else summary['project'])
if not summary.get('selectedPlugin'):
    raise SystemExit('Explicit selected Reports stage required; no root fallback')
project,selected_plugin,selected_module=validate_isolated_project(project,ROOT/summary['selectedPlugin'],ROOT)
project_digest=hashlib.sha256(project.read_bytes()).hexdigest()
export_source = args.export_run or args.blueprint_export_run
runtime_sources = {}
material_sources = {}
preview_extra_steps=0
if args.ui_run:
    from RunGraphRegression import sha,read_json,readback_errors
    selected=(ROOT/summary['selectedPlugin']).resolve()
    actual=summary.get('actualLoadedFingerprint',{})
    if (not selected.is_relative_to(ROOT/'Reports') or selected.name!='NodeForge'
        or sha(selected/'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll')!=actual.get('binarySha256')
        or not all(summary.get('protection',{}).values())):
        raise SystemExit('Exact UI-saved verification build and protection required')
    receipt=read_json(ROOT/summary['uiReceipt']);reopened=read_json(ROOT/summary['reopenedReport']);request=read_json(ROOT/summary['input'])
    source_asset=project.parent/'Content'/(request['assetPath'].removeprefix('/Game/')+'.uasset')
    expected_kind = 'Blueprint' if args.blueprint_only else 'Material'
    if (receipt.get('mode')!='Import' or receipt.get('inputMethod') not in ('file','paste','draft') or request.get('assetType')!=expected_kind
        or (expected_kind == 'Material' and request['assetPath']!=args.material) or sha(ROOT/summary['input'])!=receipt.get('inputDigest')
        or receipt.get('overallStatus')!='waitingPersistence' or reopened.get('overallStatus')!='succeeded'
        or reopened.get('checks',{}).get('persistence',{}).get('status')!='passed'
        or reopened.get('executionFingerprint')!=actual or receipt.get('executionFingerprint')!=actual
        or reopened.get('fileSha256')!=sha(source_asset) or receipt.get('fileSha256')!=sha(source_asset)
        or readback_errors(request,reopened.get('reopenedFinalReadback'))):
        raise SystemExit('Exact real UI receipt, copied saved bytes and native reopen evidence differ')
    source_asset_before=sha(source_asset)
    original_project=Path(receipt['project']).resolve()
    validate_isolated_project(original_project,selected_plugin,ROOT)
    original_asset=original_project.parent/'Content'/(request['assetPath'].removeprefix('/Game/')+'.uasset')
    if sha(original_asset)!=source_asset_before:raise SystemExit('Actual UI source bytes changed after verification')
    (runtime_sources if expected_kind=='Blueprint' else material_sources)[original_asset]=sha(original_asset)
if args.preview_run:
    from RunGraphRegression import sha,read_json,write_new,readback_errors
    if args.visual_only or args.blueprint_only:raise SystemExit('Preview scene slice requires both domains')
    selected=(ROOT/summary['selectedPlugin']).resolve()
    if not selected.is_relative_to(ROOT/'Reports') or sha(selected/'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll')!=summary.get('dllSha256') or not summary.get('oneUnchangedBuildAndProtectedAssets'):
        raise SystemExit('Exact passing preview DLL and protected source required')
    for kind in ['Blueprint','Material']:
        receipt=read_json(graph/(kind+'CopyImport.json'));reopened=read_json(graph/(kind+'CopyReopen.json'))
        asset=project.parent/'Content'/(receipt['assetPath'].removeprefix('/Game/')+'.uasset')
        if receipt.get('overallStatus')!='waitingPersistence' or receipt.get('assetType')!=kind or sha(asset)!=receipt.get('fileSha256') or reopened.get('overallStatus')!='succeeded' or reopened.get('fileSha256')!=sha(asset) or receipt.get('executionFingerprint',{}).get('binarySha256')!=summary['dllSha256']:
            raise SystemExit('Preview copy saved bytes, actual DLL or independent reopen differs: '+kind)
        if kind=='Blueprint':runtime_sources[asset]=sha(asset)
        else:
            if receipt['assetPath']!=args.material:raise SystemExit('Explicit material must be the actual reviewed material copy')
            source_asset=asset;source_asset_before=sha(asset)
if args.blueprint_only != bool(args.blueprint_export_run or (args.ui_run and args.blueprint_only)) or (args.blueprint_only and args.visual_only):
    raise SystemExit('Blueprint-only requires explicit blueprint-export-run and no visual-only')
if export_source:
    from RunGraphRegression import build_snapshot, sha, read_json
    if (args.export_run and not args.visual_only) or summary.get('sourceAndBinaryAfter') != build_snapshot(ROOT / summary['selectedPlugin']):
        raise SystemExit('Actual roundtrip matrix on the unchanged source/stage required')
if args.export_run:
    matches = []
    for case in summary.get('cases', []):
        if case.get('expected') != 'passed' or case.get('result') != 'passed': continue
        name = case['case']; receipt = read_json(graph / (name + 'Import.json'))
        if receipt.get('assetPath') == args.material: matches.append((name, receipt))
    if len(matches) != 1: raise SystemExit('Exactly one successfully roundtripped generated material required')
    selected_name, receipt = matches[0]
    source_asset = project.parent / 'Content' / (args.material.removeprefix('/Game/') + '.uasset')
    reopened = read_json(graph / (selected_name + 'Reopen.json'))
    if sha(source_asset) != receipt.get('fileSha256') or reopened.get('overallStatus') != 'succeeded' or reopened.get('checks', {}).get('persistence', {}).get('status') != 'passed':
        raise SystemExit('Saved current roundtrip material bytes/reopen do not match evidence')
    source_asset_before = sha(source_asset)
if args.blueprint_export_run:
    if summary.get('assetKind') != 'Blueprint': raise SystemExit('Actual Blueprint export matrix required')
    expected_names = {'BlueprintText', 'BlueprintTextChanged', 'BlueprintBranch', 'BlueprintBranchFalse',
                      'BlueprintConcat', 'BlueprintZeroDefaults', 'Untagged', 'Changed'}
    positives = {c['case'] for c in summary['cases'] if c.get('expected') == 'passed' and c.get('result') == 'passed'}
    if positives != expected_names: raise SystemExit('This PIE slice requires the exact eight independently roundtripped core/native-source blueprints')
    for name in sorted(positives):
        receipt = read_json(graph / (name + 'Import.json')); reopened = read_json(graph / (name + 'Reopen.json'))
        asset_file = project.parent / 'Content' / (receipt['assetPath'].removeprefix('/Game/') + '.uasset')
        if receipt.get('assetType') != 'Blueprint' or sha(asset_file) != receipt.get('fileSha256') or reopened.get('overallStatus') != 'succeeded':
            raise SystemExit('Actual saved/reopened Blueprint receipt differs: ' + name)
        runtime_sources[asset_file] = sha(asset_file)

cases={} if args.visual_only else {'BlueprintText':'NodeForge data-driven greeting',
       'BlueprintTextChanged':'A different JSON produced this message',
       'BlueprintBranch':'Message set by graph input',
       'BlueprintBranchFalse':'Disabled branch ran',
       'BlueprintConcat':'Data-driven function output',
       'BlueprintZeroDefaults':'Disabled branch ran'}
if args.blueprint_export_run:
    cases.update({'Untagged': 'NodeForge data-driven greeting', 'Changed': 'Exported native changed graph'})
if args.preview_run:cases={'PreviewTrue':'NodeForge pulse material: enabled on BeginPlay.','PreviewFalse':'Changed by preview data'}
if args.ui_run and args.blueprint_only:cases={'UiBlueprint':args.blueprint_message}

def case_input(case):
    if args.ui_run:return ROOT/summary['input']
    if args.preview_run:return graph/'BlueprintCopyImportInput.json' if case=='PreviewTrue' else false_input
    if args.blueprint_export_run:return graph/(case+'RoundtripInput.json')
    return graph/'Inputs'/(case+'.json')

# Capture every existing caller-selected source before the first engine child.
# PreviewFalse is a labelled new derivative, captured after its separate creation.
for case in cases:
    if args.preview_run and case=='PreviewFalse':continue
    request=json.loads(case_input(case).read_text('utf-8'))
    asset=project.parent/'Content'/(request['assetPath'].removeprefix('/Game/')+'.uasset')
    if not asset.is_file():raise SystemExit('Exact saved Blueprint source required before any scene work')
    runtime_sources.setdefault(asset,hashlib.sha256(asset.read_bytes()).hexdigest())
if not args.blueprint_only:
    source_asset=project.parent/'Content'/(args.material.removeprefix('/Game/')+'.uasset')
    if not source_asset.is_file():raise SystemExit('Exact saved source material required before any scene work')
    source_asset_before=hashlib.sha256(source_asset.read_bytes()).hexdigest()
    material_sources[source_asset]=source_asset_before
def write(path, data):
    with path.open('x', encoding='utf-8', newline='\n') as file:
        json.dump(data, file, indent=2, ensure_ascii=False)
def snapshot():
    paths = list((ROOT/'Plugins/NodeForge/Source').rglob('*')) + [ROOT/'Plugins/NodeForge/Binaries/Win64/UnrealEditor-NodeForgeEditor.dll']
    selected = summary.get('selectedPlugin')
    if selected:
        plugin = (ROOT/selected).resolve()
        if not plugin.is_relative_to(ROOT):
            raise SystemExit('Selected build outside workspace')
        paths += list(plugin.rglob('*'))
    return {p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in paths if p.is_file()}
baseline = snapshot()
steps=[]
def execute(name, options, field=None, expected='passed'):
    validate_isolated_project(project,selected_plugin,ROOT)
    if snapshot()!=baseline or hashlib.sha256(project.read_bytes()).hexdigest()!=project_digest:
        raise SystemExit('Selected stage/source/project changed; stop before the next child')
    if any(hashlib.sha256(p.read_bytes()).hexdigest()!=h for assets in (runtime_sources,material_sources) for p,h in assets.items()):
        raise SystemExit('Caller-selected source asset changed; stop before the next child')
    report=out/(name+'.json')
    cmd=[str(ENGINE),str(project),*options,'-NFReport='+str(report),'-d3d11','-RenderOffscreen',
         '-AllowCommandletRendering','-unattended','-nop4','-nosound','-NoLiveCoding',
         '-abslog='+str(out/(name+'.log')),'-stdout','-FullStdOutLogOutput']
    result=run_guarded(cmd,cwd=ROOT,console=out/(name+'Console.txt'),timeout=240,expected_module=selected_module)
    data=json.loads(report.read_text('utf-8')) if report.exists() else {}
    actual=data.get(field) if field else data.get('result',data.get('status'))
    good=result.returncode==0 and actual==expected
    runtime_checks = None
    if name.endswith('Runtime'):
        good=good and automation_report_passed(data,'NodeForge.BlueprintRuntime')
        config_path=out/(name.removesuffix('Runtime')+'Config.json')
        config=json.loads(config_path.read_text('utf-8'))
        log_path=out/(name+'.log')
        log_text=log_path.read_text('utf-8-sig',errors='replace') if log_path.is_file() else ''
        prior_pids=[s['pid'] for s in steps]
        if args.ui_run:prior_pids += [summary['uiPid'],summary['nativePid']]
        runtime_checks=blueprint_runtime_checks(data,result.pid,config,config_path,log_text,prior_pids)
        good=good and all(runtime_checks.values())
    steps.append({'step':name,'pid':result.pid,'exitCode':result.returncode,'actual':actual,'passed':good,'report':report.relative_to(ROOT).as_posix(),'runtimeChecks':runtime_checks})
    print(name,result.returncode,actual,flush=True)
    return good
if args.preview_run:
    # Derive an explicitly labelled new data case, not an external-AI original or a new builder.
    modified=copy.deepcopy(read_json(graph/'BlueprintCopyImportInput.json'))
    modified['requestId']='PreviewRuntimeFalse1';modified['assetPath']='/Game/NodeForgeTests/NFPreviewRuntimeFalse1'
    variable=next(v for v in modified['settings']['variables'] if v['name']=='EnablePulseLog')
    if variable['type']!='bool' or variable['default'] is not True:raise SystemExit('Unexpected source variable fixture')
    variable['default']=False
    false_input=out/'FalseBranchInput.json';write(false_input,modified)
    write(out/'DerivedInputProvenance.json',{'source':str(graph/'BlueprintCopyImportInput.json'),'sourceSha256':sha(graph/'BlueprintCopyImportInput.json'),'changes':['requestId/new assetPath','EnablePulseLog default false'],'scope':'Labelled test derivative; not new raw AI output; same unchanged plugin'})
    chain=execute('FalseBranchPreview',['-run=NodeForge','-NFMode=GraphPreview','-NFInput='+str(false_input),'-NFPrevious='+str(graph/'BlueprintExport.json')],'operationStatus')
    if chain:chain=execute('FalseBranchImport',['-run=NodeForge','-NFMode=Import','-NFInput='+str(false_input)],'overallStatus','waitingPersistence')
    if chain:chain=execute('FalseBranchReopen',['-run=NodeForge','-NFMode=Verify','-NFInput='+str(false_input),'-NFPrevious='+str(out/'FalseBranchImport.json')],'overallStatus','succeeded')
    preview_extra_steps=3
    if not chain:raise SystemExit('False branch preview/import/reopen failed; preserved reports, no other tests started')
    reopened=read_json(out/'FalseBranchReopen.json')
    errors=readback_errors(modified,reopened.get('reopenedFinalReadback'))
    if errors:raise SystemExit('False branch independent readback differs: '+str(errors))
    false_asset=project.parent/'Content'/(modified['assetPath'].removeprefix('/Game/')+'.uasset')
    runtime_sources[false_asset]=sha(false_asset)
visual={'map':'/Game/NodeForgeTests/NFGraphVisualMap','material':args.material,
        'scalarChanges':{'Speed':1.1,'Scale':2,'WaveStrength':0.35,'Roughness':0.28},
        'vectorChanges':{'WaterColor':[0.05,0.4,0.7,1]}}
if args.visual_config:
    config_path = (ROOT / args.visual_config).resolve()
    if not config_path.is_relative_to(ROOT) or not config_path.is_file():
        raise SystemExit('Visual config must be an existing workspace file')
    visual = json.loads(config_path.read_text('utf-8-sig'))
    if set(visual) != {'map', 'material', 'scalarChanges', 'vectorChanges'} or visual['material'] != args.material:
        raise SystemExit('Explicit visual config must identify the selected generated material')
if args.preview_run and not args.visual_config:
    visual={'map':'/Game/NodeForgeTests/NFPreviewMaterialMap','material':args.material,'scalarChanges':{'PulseSpeed':1.2,'MinimumBrightness':0.5},'vectorChanges':{'PulseColor':[0.04,0.65,0.9,1]}}
if not args.blueprint_only:
    source_asset=project.parent/'Content'/(args.material.removeprefix('/Game/')+'.uasset')
    if not source_asset.is_file():raise SystemExit('Exact saved source material required before visual work')
    if hashlib.sha256(source_asset.read_bytes()).hexdigest()!=source_asset_before:
        raise SystemExit('Original material changed after batch source capture')
    write(out/'VisualConfig.json',visual)
    if execute('VisualCreate',['-run=NodeForge','-NFMode=GraphVisualCreate','-NFInput='+str(out/'VisualConfig.json')]):
        execute('VisualCheck',['-run=NodeForge','-NFMode=GraphVisualCheck','-NFInput='+str(out/'VisualConfig.json')])
for case,message in cases.items():
    input_file = case_input(case)
    request=json.loads(input_file.read_text('utf-8'))
    asset=project.parent/'Content'/(request['assetPath'].removeprefix('/Game/')+'.uasset')
    if not asset.is_file():raise SystemExit('Exact saved Blueprint source required before runtime work')
    asset_sha=hashlib.sha256(asset.read_bytes()).hexdigest()
    if asset in runtime_sources and runtime_sources[asset]!=asset_sha:
        raise SystemExit('Original Blueprint changed after batch source capture')
    runtime_sources.setdefault(asset,asset_sha)
    config={'map':'/Game/NodeForgeTests/'+('NFExportBP' if args.blueprint_export_run else 'NF')+case+'Map','blueprint':request['assetPath'],'expectedMessage':message}
    path=out/(case+'Config.json'); write(path,config)
    if execute(case+'Create',['-run=NodeForge','-NFMode=GraphMapCreate','-NFInput='+str(path)]):
        execute(case+'Runtime',['-ExecCmds=Automation RunTests NodeForge.BlueprintRuntime; Quit',
                               '-TestExit=Automation Test Queue Empty','-NFGraphRuntimeConfig='+str(path)])
after=snapshot()
source_unchanged = all(hashlib.sha256(p.read_bytes()).hexdigest()==h for assets in (runtime_sources,material_sources) for p,h in assets.items())
good=all(step['passed'] for step in steps) and len(steps)==preview_extra_steps+(0 if args.blueprint_only else 2)+len(cases)*2 and baseline==after and source_unchanged
if args.ui_run and args.blueprint_only:
    scope = 'Actual independently reopened real UI Blueprint saved bytes and exact attributed PIE; no material visual or package claim'
elif args.preview_run:
    scope = 'Reviewed Blueprint copy true path and separately previewed/compiled/reopened data-modified false path exact attributed PIE; reviewed material actual GPU time/parameter changes, no UI or packaging claim'
elif args.blueprint_only:
    scope = 'Actual independently reopened saved eight Blueprint copies, exact attributed PIE messages; no material visual or package claim'
else:
    scope = 'Actual saved scene GPU time/parameter changes' + ('; no blueprint runtime tested' if args.visual_only else ' and six exact attributed PIE messages') + '; visual quality requires image inspection'
write(out/'Summary.json',{'result':'passed' if good else 'failed','steps':steps,'sourceAndBinaryBefore':baseline,
                         'sourceAndBinaryAfter':after,'oneUnchangedBuild':baseline==after,
                         'selectedPlugin':summary.get('selectedPlugin'),'visualOnly':args.visual_only,
                         'sourceGraphRun':args.graph_run,'sourceExportRun':args.export_run,
                         'sourceBlueprintExportRun':args.blueprint_export_run,'blueprintOnly':args.blueprint_only,
                         'sourcePreviewRun':args.preview_run,
                         'sourceUiVerifyRun':args.ui_run,
                         'sourceBlueprintBytesUnchanged':(all(hashlib.sha256(p.read_bytes()).hexdigest()==h for p,h in runtime_sources.items()) if runtime_sources else 'notApplicable'),
                         'sourceMaterialBytesUnchanged':(all(hashlib.sha256(p.read_bytes()).hexdigest()==h for p,h in material_sources.items()) if material_sources else 'notApplicable'),
                         'sourceSavedAssetBytesUnchanged':source_unchanged,
                         'sourceAssetHashesBefore':{kind:{p.relative_to(ROOT).as_posix():h for p,h in assets.items()} for kind,assets in [('Blueprint',runtime_sources),('Material',material_sources)]},
                         'scope':scope})
sys.exit(0 if good else 1)
