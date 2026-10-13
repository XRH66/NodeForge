"""Native readonly preview checks, then two new-copy imports and independent reopen.

Reuse existing protected snapshots, import/readback validators and commandlet.
No UI automation, source assets edited, plugin compilation or dependencies installed.
"""
import argparse,copy,shutil,subprocess,time
from RunGraphRegression import ROOT,ENGINE,ASSET_SUFFIXES,read_json,write_new,ref,sha,files_under,build_snapshot,protected_assets,readback_errors

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--run',required=True);p.add_argument('--plugin-directory',required=True)
p.add_argument('--recover-material-from')
p.add_argument('--smoke',action='store_true')
a=p.parse_args()
if not a.run.isascii() or not a.run.isalnum() or len(a.run)>20:raise SystemExit('Fresh bounded ASCII run required')
plugin=(ROOT/a.plugin_directory).resolve()
if not plugin.is_relative_to(ROOT/'Reports') or plugin.name!='NodeForge':raise SystemExit('Explicit staged plugin required')
source=ROOT/'ValidationProject/RegressionRuns/CandidateVerify1/Content'
prior=read_json(ROOT/'Reports/20261009CandidateRC1/CandidateVerify1/Summary.json')
if prior['result']!='passed':raise SystemExit('Actual UI asset verification required')
out=ROOT/'Reports/20261009UsableEditor'/a.run;folder=ROOT/'ValidationProject/RegressionRuns'/a.run
out.mkdir(exist_ok=False);folder.mkdir(exist_ok=False);shutil.copytree(source,folder/'Content')
project=folder/(a.run+'.uproject')
write_new(project,{'FileVersion':3,'EngineAssociation':'5.3','AdditionalPluginDirectories':[str(plugin.parent)],'Plugins':[{'Name':'NodeForge','Enabled':True}]})
baseline=build_snapshot(plugin);protected=protected_assets();source_before=files_under(source,ASSET_SUFFIXES)
initial=files_under(folder/'Content',ASSET_SUFFIXES);steps=[];cases=[]
binary=sha(plugin/'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll')
def execute(name,mode,request,previous=None):
    inp=out/(name+'Input.json');report=out/(name+'.json');write_new(inp,request)
    cmd=[str(ENGINE),str(project),'-run=NodeForge','-NFMode='+mode,'-NFInput='+str(inp),'-NFReport='+str(report),'-unattended','-d3d11','-RenderOffscreen','-AllowCommandletRendering','-nop4','-nosound','-NoLiveCoding','-abslog='+str(out/(name+'.log')),'-stdout']
    if previous:cmd.append('-NFPrevious='+str(previous))
    write_new(out/(name+'Command.json'),cmd)
    with (out/(name+'Console.txt')).open('xb') as stream:
        process=subprocess.Popen(cmd,cwd=ROOT,stdout=stream,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
        start=time.monotonic()
        while process.poll() is None and time.monotonic()-start<240:time.sleep(.2)
        if process.poll() is None:
            write_new(out/'LiveProcess.json',{'pid':process.pid,'command':cmd});raise SystemExit('Native child live; do not restart')
    data=read_json(report) if report.exists() else {}
    steps.append({'step':name,'pid':process.pid,'exitCode':process.returncode,'report':ref(report)})
    return data,process.returncode,inp,report

def preview(name,request,previous,expected):
    if a.smoke and name not in ['BlueprintSame','BlueprintDefault','BlueprintWrongDll','MaterialSame','MaterialProperty','MaterialWrongDll']:
        return None,None
    before=files_under(folder/'Content',ASSET_SUFFIXES)
    data,code,inp,report=execute(name,'GraphPreview',request,previous)
    delta=data.get('graphDifference',{});errors=[]
    if expected is None:
        if code==0 or data.get('overallStatus')!='rejected':errors.append('Expected explicit rejection')
    else:
        if code!=0 or data.get('operationStatus')!='passed':errors.append('Preview failed')
        for key,value in expected.items():
            observed=delta.get(key)
            if isinstance(value,bool):ok=observed is value
            else:ok=isinstance(observed,list) and len(observed)==value
            if not ok:errors.append(key+' differs: '+str(observed))
        for check in ['build','compile','save']:
            if data.get('checks',{}).get(check,{}).get('status')!='notRun':errors.append('Preview claimed '+check)
        if data.get('residue',{}).get('saveAttempted') is not False:errors.append('Preview save side effect')
        if data.get('readOnlyExecutionFingerprint',{}).get('binarySha256')!=binary:errors.append('Actual loaded DLL differs')
    if before!=files_under(folder/'Content',ASSET_SUFFIXES):errors.append('Preview modified saved assets')
    row={'case':name,'passed':not errors,'errors':errors,'input':ref(inp),'report':ref(report)};cases.append(row)
    print(name,'passed' if not errors else 'failed',errors,flush=True)
    return inp,report

modified={}
recovery_evidence=None
if a.recover_material_from:
    old=(ROOT/a.recover_material_from).resolve()
    if not old.is_relative_to(ROOT/'Reports/20261009UsableEditor'):raise SystemExit('Bounded old evidence required')
    old_summary=read_json(old/'Summary.json')
    failed=[c['case'] for c in old_summary['cases'] if not c['passed']]
    if old_summary['dllSha256']!=binary or failed!=['MaterialReviewedCopy'] or not old_summary['oneUnchangedBuildAndProtectedAssets'] or not old_summary['copiedOriginalAssetsUnchanged']:
        raise SystemExit('Recovery only applies to this sole failed material compile with identical DLL and protected assets')
    failed_import=read_json(old/'MaterialCopyImport.json')
    if failed_import.get('failedStage')!='compile' or failed_import.get('residue',{}).get('saveAttempted') is not False:
        raise SystemExit('Unexpected prior side effects; no automatic recovery')
    recovery_evidence={'summary':ref(old/'Summary.json'),'summarySha256':sha(old/'Summary.json'),'scope':'Only material preview/import/reopen rerun with D3D11; previous failed overall result preserved; unchanged DLL'}
    r=read_json(old/'MaterialCopyImportInput.json')
    preview('MaterialPreviewRecovery',r,old/'MaterialExport.json',{'requestStructureOrValuesChanged':True,'nodesChanged':1})
    modified['Material']=r
for kind in ([] if a.recover_material_from else ['Blueprint','Material']):
    path='/Game/NodeForgeTests/NFExternalGraph'+kind+'2'
    config={'source':path+'.'+path.split('/')[-1],'requestId':'PreviewBaseline'+kind,'assetPath':'/Game/NodeForgeTests/NFPreviewBaseline'+kind}
    exported,code,_,receipt=execute(kind+'Export',kind+'GraphExport',config)
    if code or exported.get('operationStatus')!='passed':raise SystemExit('Source native export failed; preserved report')
    request=exported['exportedRequest']
    def fresh(name):
        r=copy.deepcopy(request);r['requestId']=name;r['assetPath']='/Game/NodeForgeTests/NF'+name;return r
    preview(kind+'Same',fresh(kind+'Same'),receipt,{'requestStructureOrValuesChanged':False,'layoutChanged':False})
    r=fresh(kind+'Shuffled');r['nodes'].reverse();r['edges'].reverse()
    # Native JSON equality also ignores object key order.
    r['settings']=dict(reversed(list(r['settings'].items())))
    preview(kind+'Shuffled',r,receipt,{'requestStructureOrValuesChanged':False,'layoutChanged':False})
    r=fresh(kind+'Layout');r['nodes'][0]['position'][0]+=64
    preview(kind+'Layout',r,receipt,{'requestStructureOrValuesChanged':False,'layoutChanged':True,'positionsChanged':1})
    if kind=='Blueprint':
        r=fresh('BlueprintVariable');r['settings']['variables'][0]['default']=False
        preview('BlueprintVariable',r,receipt,{'requestStructureOrValuesChanged':True,'settingsChanged':1})
        r=fresh('BlueprintDefault');node=next(n for n in r['nodes'] if 'InString' in n.get('defaults',{}))
        node['defaults']['InString']='Changed by preview data'
        preview('BlueprintDefault',r,receipt,{'requestStructureOrValuesChanged':True,'nodesChanged':1})
        modified[kind]=r
        r=fresh('BlueprintTopology');branch=next(n['id'] for n in r['nodes'] if n['class'].endswith('K2Node_IfThenElse'))
        edges=[e for e in r['edges'] if e['from']==branch and e['output'] in ['then','else']]
        edges[0]['to'],edges[1]['to']=edges[1]['to'],edges[0]['to']
        preview('BlueprintTopology',r,receipt,{'requestStructureOrValuesChanged':True,'edgesAdded':2,'edgesRemoved':2})
    else:
        r=fresh('MaterialProperty');node=next(n for n in r['nodes'] if n['class'].endswith('MaterialExpressionScalarParameter'))
        node['properties']['DefaultValue']=0.42
        preview('MaterialProperty',r,receipt,{'requestStructureOrValuesChanged':True,'nodesChanged':1})
        modified[kind]=r
        r=fresh('MaterialAdd');root=next(e for e in r['edges'] if e['to']=='Material' and e['input']=='EmissiveColor')
        old_from,old_output=root['from'],root['output'];root['from']='ExtraScale';root['output']='0'
        r['nodes'] += [{'id':'ExtraScalar','class':'/Script/Engine.MaterialExpressionConstant','properties':{'R':0.25}}, {'id':'ExtraScale','class':'/Script/Engine.MaterialExpressionMultiply','properties':{}}]
        r['edges'] += [{'from':old_from,'output':old_output,'to':'ExtraScale','input':'A'},{'from':'ExtraScalar','output':'0','to':'ExtraScale','input':'B'}]
        preview('MaterialAdd',r,receipt,{'requestStructureOrValuesChanged':True,'nodesAdded':2,'edgesAdded':3,'edgesRemoved':1})
        r=fresh('MaterialRemove');sine=next(n['id'] for n in r['nodes'] if n['class'].endswith('MaterialExpressionSine'))
        incoming=next(e for e in r['edges'] if e['to']==sine)
        for edge in r['edges']:
            if edge['from']==sine:edge['from'],edge['output']=incoming['from'],incoming['output']
        r['edges'].remove(incoming);r['nodes']=[n for n in r['nodes'] if n['id']!=sine]
        preview('MaterialRemove',r,receipt,{'requestStructureOrValuesChanged':True,'nodesRemoved':1,'edgesAdded':1,'edgesRemoved':2})
    r=fresh(kind+'InvalidNode');r['nodes'][0]['class']='/Script/Engine.NotANode';preview(kind+'InvalidNode',r,receipt,None)
    r=fresh(kind+'InvalidProperty');r['nodes'][0]['properties']['NotAProperty']=1;preview(kind+'InvalidProperty',r,receipt,None)
    r=fresh(kind+'InvalidPin');r['edges'][0]['input']='NotAPin';preview(kind+'InvalidPin',r,receipt,None)
    r=fresh(kind+'Existing');r['assetPath']=path;preview(kind+'Existing',r,receipt,None)
    stale=copy.deepcopy(exported);stale['exportEvidence']['sourceAssetSha256']='0'*64
    stale_path=out/(kind+'StaleBaseline.json');write_new(stale_path,stale)
    preview(kind+'Stale',fresh(kind+'Stale'),stale_path,None)
    wrong=copy.deepcopy(exported);wrong['readOnlyExecutionFingerprint']['binarySha256']='0'*64
    wrong_path=out/(kind+'WrongDllBaseline.json');write_new(wrong_path,wrong)
    preview(kind+'WrongDll',fresh(kind+'WrongDll'),wrong_path,None)

# Separate import proves the review is not a second builder. Preview itself did not create these targets.
for kind,r in modified.items():
    generated,code,inp,receipt=execute(kind+'CopyImport','Import',r)
    errors=readback_errors(r,generated.get('publishedReadback'))
    if code or generated.get('overallStatus')!='waitingPersistence':errors.append('Import failed')
    if not errors:
        reopened,code,_,_=execute(kind+'CopyReopen','Verify',r,receipt)
        errors+=readback_errors(r,reopened.get('reopenedFinalReadback'))
        if code or reopened.get('overallStatus')!='succeeded':errors.append('Independent reopen failed')
        if reopened.get('pid')==generated.get('pid'):errors.append('Reopen PID not independent')
    cases.append({'case':kind+'ReviewedCopy','passed':not errors,'errors':errors})
    print(kind+'ReviewedCopy','passed' if not errors else 'failed',errors,flush=True)

unchanged=baseline==build_snapshot(plugin) and protected==protected_assets() and source_before==files_under(source,ASSET_SUFFIXES)
after=files_under(folder/'Content',ASSET_SUFFIXES)
original_unchanged=all(after.get(path)==digest for path,digest in initial.items())
good=all(c['passed'] for c in cases) and unchanged and original_unchanged
write_new(out/'Summary.json',{'result':'passed' if good else 'failed','project':ref(project),'selectedPlugin':ref(plugin),'dllSha256':binary,'cases':cases,'steps':steps,'oneUnchangedBuildAndProtectedAssets':unchanged,'copiedOriginalAssetsUnchanged':original_unchanged,'recoveryEvidence':recovery_evidence,'smoke':a.smoke,'runnerSha256':sha(ROOT/'Scripts/RunGraphPreviewChecks.py'),'scope':('Affected material preview/import/reopen recovery; prior failures preserved' if recovery_evidence else ('Representative native preview/import/reopen smoke after UI-only guard; not full preview matrix' if a.smoke else 'Native readonly request preview and two separately imported/compiled/saved independently reopened copies'))+'; no UI button, PIE, visual or packaging claim for the new build.'})
raise SystemExit(0 if good else 1)
