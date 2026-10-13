"""Same-validator preflight, representative imports/reopen and protected negative batches.

Reuses RunGraphRegression snapshots/readback and existing native automation. No
desktop control, compilation, external tools or third-party code execution.
"""
import argparse
import copy
import shutil
import subprocess
import time
from pathlib import Path
from RunGraphRegression import (ROOT, ENGINE, ASSET_SUFFIXES, build_snapshot, files_under,
    protected_assets, read_json, write_new, ref, sha, readback_errors, subset_errors)
from RunProcessGuard import run_guarded, validate_isolated_project, validate_terminal_step

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--run',required=True)
p.add_argument('--plugin-directory',required=True)
p.add_argument('--legacy-run',help='Only replay the original eleven invalid files in this passed v1 project')
p.add_argument('--resume-after-batch',action='store_true',help='Continue this exact passed native batch before imports; no batch rerun')
a=p.parse_args()
if not a.run.isascii() or not a.run.isalnum() or len(a.run)>24:
    raise SystemExit('Fresh bounded ASCII run name required')
plugin=(ROOT/a.plugin_directory).resolve()
if not plugin.is_relative_to(ROOT/'Reports') or plugin.name!='NodeForge':raise SystemExit('Explicit staged NodeForge required')
out=ROOT/'Reports/20261009UsableEditor'/a.run
if out.exists() and not a.resume_after_batch:raise SystemExit('Existing evidence protected; observe prior run, do not restart')
if not a.resume_after_batch:out.mkdir()
baseline=build_snapshot(plugin);protected_before=protected_assets()
runner_files=[Path(__file__).resolve(),ROOT/'Scripts/RunGraphRegression.py',ROOT/'Scripts/RunProcessGuard.py']
runner_hashes={ref(v):sha(v) for v in runner_files}
candidate=ROOT/'Reports/20261009CandidateRC1/PluginStage/NodeForge';candidate_before=files_under(candidate)
steps=[]

def execute(name,options,field='overallStatus',expected='succeeded'):
    if runner_hashes!={ref(v):sha(v) for v in runner_files} or baseline!=build_snapshot(plugin):
        raise SystemExit('Runner or selected source/stage changed; stop before the next process')
    report=out/(name+'.json')
    cmd=[str(ENGINE),str(project),*options,'-NFReport='+str(report),'-d3d11','-RenderOffscreen',
        '-AllowCommandletRendering','-unattended','-nop4','-nosound','-NoLiveCoding',
        '-abslog='+str(out/(name+'.log')),'-stdout','-FullStdOutLogOutput']
    process=run_guarded(cmd,cwd=ROOT,console=out/(name+'Console.txt'),timeout=240,
        expected_module=plugin/'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll')
    data=read_json(report) if report.is_file() else {}
    ok=process.returncode==0 and data.get(field)==expected
    steps.append({'step':name,'passed':ok,'pid':process.pid,'exitCode':process.returncode,'report':ref(report)})
    write_new(out/('Progress'+str(len(steps))+'.json'),{'steps':steps,'currentProject':ref(project)})
    print(name,'passed' if ok else 'failed',flush=True)
    if not ok:raise SystemExit('Native failure retained; stop this batch before remaining tasks')
    return data,report

cases=[];positive={};provenance={}
if a.resume_after_batch:
    preparation=read_json(out/'Preparation.json');batch=read_json(out/'ValidationBatch.json')
    if (a.legacy_run or batch.get('status')!='passed' or batch.get('actualLoadedBinarySha256')!=sha(plugin/'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll')
        or preparation['sourceAndBinaryBefore']!=baseline or any((out/(name+'Import.json')).exists() for name in ('Material','Blueprint','Instance'))):
        raise SystemExit('Resume only this completed identical preflight batch before any import; no overwrites')
    project,plugin,binary=validate_isolated_project(ROOT/preparation['project'],plugin,ROOT);folder=project.parent
    resumed=read_json(out/'Progress1.json')
    prior_steps=resumed.get('steps',[])
    if len(prior_steps)!=1 or resumed.get('currentProject')!=ref(project):
        raise SystemExit('Exactly one completed ValidationBatch step on this project required')
    prior_pid=validate_terminal_step(out,'ValidationBatch',prior_steps[0],binary,project)
    if (batch.get('test')!='NodeForge.ValidationBatch' or batch.get('pid')!=prior_pid
        or batch.get('projectAssetsAndReservationsUnchanged') is not True
        or Path(batch.get('actualLoadedBinary','')).resolve()!=binary
        or not batch.get('cases') or any(c.get('passed') is not True or c.get('actual')!=c.get('expected') for c in batch['cases'])):
        raise SystemExit('Prior native batch is not an exact successful protected terminal')
    before=preparation['sourceAssetsBefore'];provenance=preparation['provenance']
    if before!=files_under(folder/'Content',ASSET_SUFFIXES) or any(sha(ROOT/key)!=value for key,value in provenance.items()):
        raise SystemExit('Prepared assets or original source bytes changed; do not resume')
    cases=read_json(out/'Cases.json')['cases']
    original_cases={c['name']:c for c in batch['cases']}
    if len(original_cases)!=len(cases) or any(c['name'] not in original_cases
        or c['mode']!=original_cases[c['name']]['mode'] or c['expected']!=original_cases[c['name']]['expected']
        or sha(Path(c['input']))!=original_cases[c['name']]['inputSha256'] for c in cases):
        raise SystemExit('Prepared case identities or input bytes differ from executed native batch; do not resume')
    positive={c['name']:Path(c['input']) for c in cases if c['name'] in ('Material','Blueprint','Instance') and c['expected']=='succeeded'}
    if set(positive)!=set(('Material','Blueprint','Instance')):raise SystemExit('Exact three prepared imports required')
    steps=prior_steps
    write_new(out/'RunnerResume.json',{'scope':'Resume after path conversion failure, prior 28 native cases not rerun; no imports executed before failure',
        'nativeBatchReportSha256':sha(out/'ValidationBatch.json'),'originalPreparationSha256':sha(out/'Preparation.json'),
        'protectedAssetsAtResume':protected_before,'candidateAtResume':candidate_before,'sourceAndBinaryStillMatchesPreparation':True})
elif a.legacy_run:
    if not a.legacy_run.isascii() or not a.legacy_run.isalnum():raise SystemExit('Explicit legacy run required')
    legacy=read_json(ROOT/'Reports/20261008UsablePrototype'/a.legacy_run/'Summary.json')
    if legacy.get('result')!='passed' or (ROOT/legacy['selectedPlugin']).resolve()!=plugin or legacy['sourceAndBinaryAfter']!=baseline:
        raise SystemExit('Passed v1 run on identical current source/DLL required')
    project,plugin,binary=validate_isolated_project(ROOT/legacy['project'],plugin,ROOT);folder=project.parent
    for inp in sorted((ROOT/'Tests/Invalid').glob('*.json')):
        cases.append({'name':inp.stem,'input':str(inp),'mode':'Import','expected':'rejected'})
    if len(cases)!=11:raise SystemExit('Original eleven negative inputs required')
    provenance={ref(Path):sha(Path) for Path in (ROOT/'Tests/Invalid').glob('*.json')}
else:
    folder=ROOT/'ValidationProject/RegressionRuns'/a.run
    if folder.exists():raise SystemExit('Existing project protected')
    folder.mkdir();project=folder/(a.run+'.uproject')
    write_new(project,{'FileVersion':3,'EngineAssociation':'5.3','AdditionalPluginDirectories':[str(plugin.parent)],'Plugins':[{'Name':'NodeForge','Enabled':True}]})
    project,plugin,binary=validate_isolated_project(project,plugin,ROOT)
    verified=read_json(ROOT/'Reports/20261009UsableEditor/PreviewUiVerify1/Summary.json')
    if verified.get('result')!='passed':raise SystemExit('Actual UI saved material/reopen prerequisite missing')
    source_project=ROOT/verified['verificationProject']
    source=source_project.parent/'Content/NodeForgeTests/NFPreviewUiMaterial1.uasset'
    receipt=read_json(ROOT/verified['uiReceipt'])
    if sha(source)!=receipt['fileSha256']:raise SystemExit('Actual UI saved parent bytes differ')
    target=folder/'Content/NodeForgeTests/NFPreviewUiMaterial1.uasset';target.parent.mkdir(parents=True);shutil.copyfile(source,target)
    material=read_json(ROOT/'Reports/20261009UsableEditor/PreviewUi1/ModifiedInput.json')
    blueprint=read_json(ROOT/'SourceData/ExternalGraphAi2/Blueprint.json')
    digest=material['catalogDigest']
    def fresh(request,name):
        r=copy.deepcopy(request);r['requestId']='Preflight'+name+'1';r['assetPath']='/Game/NodeForgeTests/NFPreflight'+name+'1';return r
    def case(name,r,rejected=False,parity=False):
        inp=out/(name+'Input.json');write_new(inp,r)
        cases.append({'name':name,'input':str(inp),'mode':'GraphPreflight','expected':'rejected' if rejected else 'succeeded'})
        if parity:cases.append({'name':name+'ImportParity','input':str(inp),'mode':'Import','expected':'rejected'})
        return inp
    m=fresh(material,'Material');positive['Material']=case('Material',m)
    cases.append({'name':'RepeatedMaterial','input':str(positive['Material']),'mode':'GraphPreflight','expected':'succeeded'})
    b=fresh(blueprint,'Blueprint');positive['Blueprint']=case('Blueprint',b)
    s=fresh(b,'BlueprintShuffled');s['nodes'].reverse();s['edges'].reverse();case('BlueprintShuffled',s)
    instance={'format':'NodeForgeGraph','version':2,'requestId':'PreflightInstance1','assetType':'MaterialInstance',
        'assetPath':'/Game/NodeForgeTests/NFPreflightInstance1','catalogDigest':digest,'nodes':[],'edges':[],
        'settings':{'parent':'/Game/NodeForgeTests/NFPreviewUiMaterial1.NFPreviewUiMaterial1','parentAssetSha256':sha(source),
            'scalarParameters':{'PulseSpeed':1.1},'vectorParameters':{'PulseColor':[.04,.65,.9,1]},'textureParameters':{}}}
    positive['Instance']=case('Instance',instance)
    r=fresh(m,'UnknownClass');r['nodes'][0]['class']='/Script/Engine.NoSuchNode';case('UnknownClass',r,True,True)
    r=fresh(m,'UnknownProperty');r['nodes'][0]['properties']['NoSuchProperty']=1;case('UnknownProperty',r,True,True)
    r=fresh(m,'UnknownPin');r['edges'][0]['input']='NoSuchPin';case('UnknownPin',r,True,True)
    r=fresh(m,'DuplicateInput');r['edges'].append(copy.deepcopy(r['edges'][0]));case('DuplicateInput',r,True,True)
    r=fresh(m,'SelfConnection');r['edges'][0]['from']=r['edges'][0]['to'];case('SelfConnection',r,True,True)
    r=fresh(instance,'MissingParent');r['settings']['parent']='/Game/NodeForgeTests/NoSuchParent.NoSuchParent';case('MissingParent',r,True,True)
    r=fresh(instance,'StaleParent');r['settings']['parentAssetSha256']='0'*64;case('StaleParent',r,True,True)
    r=fresh(m,'ExistingTarget');r['assetPath']=material['assetPath'];case('ExistingTarget',r,True,True)
    r=fresh(m,'ReservedRequest');reserved=folder/'Saved/NodeForge/Requests'/(r['requestId']+'.json');reserved.parent.mkdir(parents=True);write_new(reserved,{'scope':'owned reservation rejection fixture; not a successful import'})
    case('ReservedRequest',r,True,True)
    r=fresh(m,'UnsupportedVersion');r['version']=99;case('UnsupportedVersion',r,True,True)
    duplicate=out/'DuplicateKeyInput.json';text=positive['Material'].read_text('utf-8');duplicate.write_text('{"format":"NodeForgeGraph",'+text.lstrip()[1:],encoding='utf-8')
    for mode in ('GraphPreflight','Import'):cases.append({'name':'DuplicateKey'+mode,'input':str(duplicate),'mode':mode,'expected':'rejected'})
    cases.append({'name':'LegacyPreflight','input':str(ROOT/'Tests/Valid/MinimalBlueprint.json'),'mode':'GraphPreflight','expected':'rejected'})
    provenance={ref(source):sha(source),ref(ROOT/'SourceData/ExternalGraphAi2/Blueprint.json'):sha(ROOT/'SourceData/ExternalGraphAi2/Blueprint.json'),
        ref(ROOT/'Reports/20261009UsableEditor/PreviewUi1/ModifiedInput.json'):sha(ROOT/'Reports/20261009UsableEditor/PreviewUi1/ModifiedInput.json')}

project,plugin,binary=validate_isolated_project(project,plugin,ROOT)
if not a.resume_after_batch:
    before=files_under(folder/'Content',ASSET_SUFFIXES)
    requests_before=files_under(folder/'Saved/NodeForge/Requests')
    case_file=out/'Cases.json';write_new(case_file,{'cases':cases})
    write_new(out/'Preparation.json',{'project':ref(project),'selectedPlugin':ref(plugin),'dllSha256':sha(plugin/'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'),
        'sourceAndBinaryBefore':baseline,'sourceAssetsBefore':before,'provenance':provenance,
        'protectedAssetsBefore':protected_before,'candidateBefore':candidate_before,
        'scope':'Labelled test derivatives; original external AI JSON and actual UI input preserved'})
    batch,_=execute('ValidationBatch',['-ExecCmds=Automation RunTests NodeForge.ValidationBatch; Quit','-TestExit=Automation Test Queue Empty','-NFValidationCases='+str(case_file)],'status','passed')
else:
    requests_before=files_under(folder/'Saved/NodeForge/Requests')
assert batch['actualLoadedBinarySha256']==sha(plugin/'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll')
errors=[]
for c in cases:
    report=read_json(out/(c['name']+'.json'))
    if c['expected']=='succeeded':
        request=read_json(Path(c['input']));summary=report.get('preflightSummary',{})
        expected_nodes={n['id']:{k:n[k] for k in ('id','class','properties')} for n in request['nodes']}
        actual_nodes={n['id']:n for n in summary.get('nodeIdentities',[])}
        errors+=subset_errors(expected_nodes,actual_nodes,c['name']+'.identities','exact')
        if (set(expected_nodes)!=set(actual_nodes) or summary.get('plannedNodeCount')!=len(request['nodes'])
            or summary.get('plannedEdgeCount')!=len(request['edges']) or summary.get('settings')!=request['settings']
            or summary.get('requestReserved') is not False):errors.append(c['name']+': plan differs from exact request')
if errors:write_new(out/'ComparisonFailure.json',errors);raise SystemExit('Preflight summary comparison failed')
if before!=files_under(folder/'Content',ASSET_SUFFIXES) or requests_before!=files_under(folder/'Saved/NodeForge/Requests'):
    raise SystemExit('Preflight/negative batch changed assets or reservations')

if not a.legacy_run:
    for name,inp in positive.items():
        request=read_json(inp)
        imported,receipt=execute(name+'Import',['-run=NodeForge','-NFMode=Import','-NFInput='+str(inp)],expected='waitingPersistence')
        errors+=readback_errors(request,imported.get('publishedReadback'))
        reopened,_=execute(name+'Reopen',['-run=NodeForge','-NFMode=Verify','-NFInput='+str(inp),'-NFPrevious='+str(receipt)])
        errors+=readback_errors(request,reopened.get('reopenedFinalReadback'))
        if imported.get('executionFingerprint')!=reopened.get('executionFingerprint') or imported.get('pid')==reopened.get('pid'):
            errors.append(name+': reopened identity or PID differs')
    execute('ResultView',['-ExecCmds=Automation RunTests NodeForge.ResultView; Quit','-TestExit=Automation Test Queue Empty'],'status','passed')
    execute('PanelConstruction',['-ExecCmds=Automation RunTests NodeForge.MinimalPanel; Quit','-TestExit=Automation Test Queue Empty','-NFPanelReport='+str(out/'PanelConstruction.json')],'status','passed')

protection={'sourceAndBinary':baseline==build_snapshot(plugin),'rootAssets':protected_before==protected_assets(),
    'frozenCandidate':candidate_before==files_under(candidate),'originalSources':all(sha(ROOT/key)==value for key,value in provenance.items()),
    'copiedSources':all(sha(ROOT/key)==value for key,value in before.items()),
    'runnerFilesUnchanged':runner_hashes=={ref(v):sha(v) for v in runner_files}}
expected_steps=(['ValidationBatch'] if a.legacy_run else ['ValidationBatch',
    *[name+phase for name in ('Material','Blueprint','Instance') for phase in ('Import','Reopen')],
    'ResultView','PanelConstruction'])
ok=not errors and all(protection.values()) and [s['step'] for s in steps]==expected_steps and all(s.get('passed') is True and s.get('exitCode')==0 for s in steps)
write_new(out/'Summary.json',{'result':'passed' if ok else 'failed','errors':errors,'steps':steps,'nativeCaseCount':len(cases),
    'project':ref(project),'selectedPlugin':ref(plugin),'dllSha256':sha(plugin/'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'),
    'actualNativeLoadedBinarySha256':batch['actualLoadedBinarySha256'],'protection':protection,'sourceAndBinaryAfter':build_snapshot(plugin),'runnerHashes':runner_hashes,
    'scope':'Original eleven negative inputs only' if a.legacy_run else 'Same-validator preflight and rejection parity; three ordinary assets compile/save/independent reopen; actual UI/clipboard and runtime not run',
    'realUi':'notRun','clipboard':'notTouched','runtime':'notRun','package':'notRun'})
raise SystemExit(0 if ok else 1)
