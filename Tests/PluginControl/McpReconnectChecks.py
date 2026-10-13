"""Focused lifecycle/official SDK checks. No UE, business dispatch or asset writes."""
from pathlib import Path
import asyncio,copy,json,os,sys
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'Scripts'))
import NodeForgeMcpBridge as module
from NodeForgeMcpBridge import Bridge,BridgeError,encoded,sha
from NodeForgeMcp import ServerRuntime,serve
from mcp import StdioServerParameters
from mcp.client import Client
BASE=ROOT/'Reports/PluginControlStageTwo/AssetRead1/B3/ReconnectRepair1'
OUT=BASE/'Checks1';DOC=ROOT/'Docs/Versions/Build128/Mcp3R2'
FIXDOC=ROOT/'Docs/Versions/Build128/McpReconnectValidation1'
def read(p):return json.loads(p.read_text('utf-8-sig'))
def save(p,value):
 p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(encoded(value)+b'\n')
rows=[]
def check(name,ok,details=None):
 row={'case':name,'passed':bool(ok),'details':details};rows.append(row)
 save(OUT/'Cases'/('Case'+str(len(rows))+'.json'),row)
 assert ok,name
def expect(name,code,action):
 try:action()
 except BridgeError as e:check(name,e.code==code,{'actualCode':e.code});return
 raise AssertionError(name+' did not reject')

def fixture(name):
 folder=OUT/name;home=folder/'Contract';home.mkdir(parents=True,exist_ok=False)
 contract=read(DOC/'ToolContract.json');contract['docsManifest']=FIXDOC.relative_to(ROOT).as_posix()+'/Manifest.json'
 save(home/'ToolContract.json',contract)
 current=read(ROOT/'Reports/PluginControlStageTwo/AssetRead1/B3/Run3/Config.json')
 current.update(bundleId=contract['bundleId'],docsManifestSha256=sha((FIXDOC/'Manifest.json').read_bytes()),
  toolContractSha256=sha((home/'ToolContract.json').read_bytes()))
 def configuration(suffix):
  report=folder/suffix
  return {**current,'reportRoot':report.as_posix(),'sessionPublic':(report/'SessionPublic.json').as_posix(),
          'bridgeRunRoot':(report/'Bridge').as_posix()}
 path=folder/'Config.json';save(path,configuration('Old'))
 return path,home,configuration('New')

def old_identity(bridge):
 public=read(ROOT/'Reports/PluginControlStageTwo/AssetRead1/B3/Run2/SessionPublic.json')
 public.update(pid=42,processStart='1234',controlSessionId='a'*32,project=bridge.project.as_posix(),
  module=bridge.binary.as_posix(),binarySha256=bridge.config['binarySha256'],reportRoot=bridge.report_root.as_posix(),
  sessionFile=(ROOT/'Reports/PluginControlSessions'/('a'*32)/'Session.json').as_posix())
 assert not Path(public['sessionFile']).exists()
 bridge.identity=public;bridge.handle='b'*32
 return public

def synchronous_checks():
 conf,home,new=fixture('Idle');module.HOME=home;old=Bridge(conf)
 old_identity(old);old._claim_business_lease()
 receipt={'callId':'Original','state':'finished','requiresReview':False,'controlSessionId':'a'*32}
 save(old.run_root/'Calls/Original.json',receipt);before=(old.run_root/'Calls/Original.json').read_bytes()
 old.calls['Original']=receipt;old.reports['OldReport']={'fixture':True}
 save(conf,new)
 with patch.object(module,'process_identity',return_value={'state':'absent','windowsError':87}):candidate,proof=old.rebind()
 check('IdleRebindUsesValidatedFreshBridge',candidate.config==new and candidate.handle is None and candidate.identity is None)
 check('OldHandleAndOnlyOwnLeaseReleased',old.handle is None and not old.lease_path.exists() and not candidate.owns_lease)
 check('OldCallBytesAndOldReportsPreserved',before==(old.run_root/'Calls/Original.json').read_bytes() and not candidate.calls and not candidate.reports)
 check('CommittedReceiptNoUEOrReplay',proof['status']=='passed' and not proof['ueStarted'] and not proof['processTerminated'] and proof['oldCallsPreserved'])
 check('OldHandleRejectedBeforeDispatch',candidate.call('nodeforge_draft_get',{'handle':'b'*32,'arguments':{}})['errors'][0]['code']=='NFMCP_HANDLE')
 candidate.close()
 for name,kind,code in [('Unknown','unknown','NFMCP_REBIND_UNRESOLVED'),('PendingCli','cli','NFMCP_REBIND_UNRESOLVED'),
                         ('DurablePending','durable','NFMCP_REBIND_UNRESOLVED'),('ForeignLease','foreign','NFMCP_LEASE_CHANGED')]:
  conf,home,new=fixture(name);module.HOME=home;bridge=Bridge(conf);bridge.handle='b'*32
  bridge._claim_business_lease();own_before=bridge.lease_path.read_bytes()
  if kind=='unknown':bridge.unknown.add('Original')
  elif kind=='cli':bridge.pending_cli.append({'fixture':True})
  elif kind=='durable':save(bridge.run_root/'Calls/Original.json',{'state':'submitted','requiresReview':False})
  elif kind=='foreign':save(bridge.lease_path,{'pid':1,'instance':'foreign'});own_before=bridge.lease_path.read_bytes()
  save(conf,new);expect(name+'KeepsOriginalOwner',code,bridge.rebind)
  check(name+'NoPartialSwitch',bridge.handle=='b'*32 and bridge.lease_path.read_bytes()==own_before)
 for name,observation,code in [('LiveEditor',{'state':'running','createdTicks':'1234'},'NFMCP_REBIND_EDITOR_ACTIVE'),
                              ('UnknownProcess',{'state':'unknown'},'NFMCP_REBIND_EDITOR_ACTIVE')]:
  conf,home,new=fixture(name);module.HOME=home;bridge=Bridge(conf);old_identity(bridge);save(conf,new)
  with patch.object(module,'process_identity',return_value=observation):expect(name,code,bridge.rebind)
 conf,home,new=fixture('RuntimeChange');module.HOME=home;bridge=Bridge(conf);save(conf,new)
 bridge.loaded_runtime_files['Scripts/NodeForgeMcp.py']='0'*64
 expect('PythonCodeCannotBeHashHotSwapped','NFMCP_RUNTIME_RESTART_REQUIRED',bridge.rebind)
 conf,home,new=fixture('InvalidCandidate');module.HOME=home;bridge=Bridge(conf);old=copy.deepcopy(bridge.config)
 save(conf,{**new,'binarySha256':'0'*64});expect('InvalidBundleRejected','NFMCP_VERSION',bridge.rebind)
 check('InvalidCandidateKeepsOldSnapshot',bridge.config==old and bridge.handle is None)
 save(conf,[]);expect('MalformedConfigStructuredRejection','NFMCP_CONFIG',bridge.configuration_changed)
 conf,home,new=fixture('PublishingRace');module.HOME=home;bridge=Bridge(conf);save(conf,new);actual=module.safe_read;count=0
 def racing(path,*args,**kwargs):
  nonlocal count
  if Path(path)==conf:
   count+=1
   if count==3:save(conf,{**new,'binarySha256':'0'*64})
  return actual(path,*args,**kwargs)
 with patch.object(module,'safe_read',side_effect=racing):expect('PublicationRaceNoCommit','NFMCP_FILE_SHA',bridge.rebind)
 check('PublicationRaceOldSnapshotPreserved',bridge.config!=new and not list(Path(new['bridgeRunRoot']).glob('Rebindings/*Committed.json')))

async def runtime_checks():
 conf,home,new=fixture('Concurrent');module.HOME=home;runtime=ServerRuntime(conf)
 bridge,_,_=await runtime.acquire();save(conf,new)
 try:await runtime.acquire()
 except BridgeError as e:check('InFlightPreventsRebind',e.code=='NFMCP_REBIND_BUSY' and runtime.bridge is bridge)
 else:raise AssertionError('inflight admitted')
 await runtime.release();fresh,proof,_=await runtime.acquire()
 check('AfterCompletionSameRuntimeCanRebind',fresh is not bridge and proof['status']=='passed')
 await runtime.release();check('NoLeakedInflightCounter',runtime.inflight==0);runtime.close()

async def sdk_check(mode,suffix=''):
 conf,home,new=fixture('Sdk'+('Legacy' if mode=='legacy' else 'Modern')+suffix)
 parameter=StdioServerParameters(command=sys.executable,args=[str(Path(__file__).resolve()),'--stdio',str(conf),str(home)],cwd=ROOT)
 notices=[]
 async def handler(message):
  # Retain only method names, never generic input messages or credentials.
  method=getattr(message,'method',None)
  if method:notices.append(method)
 async with Client(parameter,read_timeout_seconds=20,mode=mode,message_handler=handler,cache=None) as client:
  listed=await client.list_tools();first=await client.call_tool('nodeforge_docs_index',{})
  assert not first.is_error and len(listed.tools)==17
  original=read(Path(new['bridgeRunRoot']).parent.parent/'Old/Bridge/ProtocolSeen.json')
  contract=read(home/'ToolContract.json');item=next(t for t in contract['tools'] if t['name']=='nodeforge_docs_read')
  item['description']+='; Lifecycle fixture updated help.';save(home/'ToolContract.json',contract)
  new['toolContractSha256']=sha((home/'ToolContract.json').read_bytes());save(conf,new)
  second=await client.call_tool('nodeforge_docs_index',{})
  assert not second.is_error
  inventory=await client.list_tools()
  fresh=read(Path(new['bridgeRunRoot'])/'ProtocolSeen.json')
  check(mode+'SdkSameProcessNoManualReload',original['bridgePid']==fresh['bridgePid'] and second.structured_content['configurationRefresh']['toolsChanged'])
  check(mode+'SdkFreshToolDefinition',len(inventory.tools)==17 and any('Lifecycle fixture updated help.' in t.description for t in inventory.tools))
  check(mode+'SdkNegotiation',fresh['protocolVersion']==client.protocol_version,{'protocolVersion':client.protocol_version})
  if mode=='legacy':check('LegacyTypedToolListNotification','notifications/tools/list_changed' in notices,notices)
  else:check('ModernNoUnsupportedLegacyNotification','notifications/tools/list_changed' not in notices,notices)
  page=await client.call_tool('nodeforge_docs_read',{'pageId':'LifecycleFixture'})
  check(mode+'SdkUtf8AfterRebind',not page.is_error and '安全接续' in page.structured_content['text'])
  save(OUT/('Sdk'+mode+'.json'),{'status':'passed','protocolVersion':client.protocol_version,
   'bridgePidBefore':original['bridgePid'],'bridgePidAfter':fresh['bridgePid'],
   'notifications':notices,'first':first.model_dump(mode='json'),'second':second.model_dump(mode='json'),
   'tools':inventory.model_dump(mode='json'),'page':page.model_dump(mode='json'),
   'scope':'actual official SDK stdio and metadata; fixture config, no UE or native business'})

async def main_async():
 await runtime_checks()
 await sdk_check('2026-07-28');await sdk_check('legacy')

def retained_stdio_factory(config):
 bridge=Bridge(config)
 public=old_identity(bridge);save(bridge.public_path,public);bridge._claim_business_lease()
 original={'callId':'Original','operation':'graphs.export','state':'unknown','requiresReview':True,
           'controlSessionId':public['controlSessionId']}
 bridge.calls['Original']=original;bridge.unknown.add('Original');save(bridge.run_root/'Calls/Original.json',original)
 report=bridge.report_root/'OriginalReport.json';save(report,{'detail':'原报告字节保留'})
 bridge.reports['c'*64]={'callId':'Original','reference':{'path':report.as_posix(),
  'sha256':sha(report.read_bytes()),'bytes':report.stat().st_size}}
 def cli(action,operation=None,arguments=None,call_id=None,revision=None):
  assert action=='editor-call' and operation=='calls.get' and arguments=={'callId':'Original'}
  return {'cliExitCode':0,'executionState':'finished','response':{'result':{
   'callId':'Original','controlSessionId':public['controlSessionId'],'operation':'graphs.export',
   'state':'completed','cliExitCode':0,'result':{}}}}
 bridge._cli=cli
 return bridge

async def recovery_sdk(mode,suffix=''):
 conf,home,new=fixture('RecoverySdk'+('Legacy' if mode=='legacy' else 'Modern')+suffix)
 tools={t['name']:t for t in read(DOC/'ToolContract.json')['tools']}
 for name,args in [('nodeforge_report_read',{'handle':'b'*32,'reportId':'c'*64,'pointer':'/detail'}),
                   ('nodeforge_calls_get',{'handle':'b'*32,'arguments':{'callId':'Original'}}),
                   ('nodeforge_graphs_generate',{'handle':'b'*32,'callId':'New','expectedDraftRevision':'0','arguments':{}})]:
  module.Draft202012Validator(tools[name]['inputSchema']).validate(args)
 parameters=StdioServerParameters(command=sys.executable,args=[str(Path(__file__).resolve()),
  '--retainedstdio',str(conf),str(home)],cwd=ROOT)
 async with Client(parameters,read_timeout_seconds=20,mode=mode,cache=None) as client:
  first=await client.call_tool('nodeforge_docs_index',{});assert not first.is_error
  original=read(Path(new['bridgeRunRoot']).parent.parent/'Old/Bridge/ProtocolSeen.json')
  report=Path(new['bridgeRunRoot']).parent.parent/'Old/OriginalReport.json';original_bytes=report.read_bytes()
  save(conf,new)
  blocked=await client.call_tool('nodeforge_docs_index',{})
  check(mode+'PendingRebindStopsDiscovery',blocked.is_error and blocked.structured_content['errors'][0]['code']=='NFMCP_REBIND_UNRESOLVED')
  retained=await client.call_tool('nodeforge_report_read',{'handle':'b'*32,'reportId':'c'*64,'pointer':'/detail'})
  save(OUT/('RetainedProbe'+mode+suffix+'.json'),retained.model_dump(mode='json'))
  check(mode+'OriginalReportReadableDuringPendingRebind',not retained.is_error and retained.structured_content['configurationRefreshPending']
        and retained.structured_content['value']=='原报告字节保留' and report.read_bytes()==original_bytes)
  rejected=await client.call_tool('nodeforge_graphs_generate',{'handle':'b'*32,'callId':'New','expectedDraftRevision':'0','arguments':{}})
  check(mode+'NoNewBusinessOnOldBinding',rejected.is_error and rejected.structured_content['errors'][0]['code']=='NFMCP_REBIND_UNRESOLVED')
  queried=await client.call_tool('nodeforge_calls_get',{'handle':'b'*32,'arguments':{'callId':'Original'}})
  check(mode+'OriginalCallCanResolveBeforeRebind',not queried.is_error and queried.structured_content['configurationRefreshPending'])
  fresh=await client.call_tool('nodeforge_docs_index',{})
  later=read(Path(new['bridgeRunRoot'])/'ProtocolSeen.json')
  check(mode+'ResolvedRebindKeepsSameSdkProcess',not fresh.is_error and original['bridgePid']==later['bridgePid']
    and fresh.structured_content['configurationRefresh']['oldHandleInvalidated'])
  check(mode+'OriginalReportAndReceiptRetained',report.read_bytes()==original_bytes and
        read(Path(new['bridgeRunRoot']).parent.parent/'Old/Bridge/Calls/Original.json')['state']=='finished')
  save(OUT/('RecoverySdk'+mode+'.json'),{'status':'passed','protocolVersion':client.protocol_version,
   'bridgePidBefore':original['bridgePid'],'bridgePidAfter':later['bridgePid'],
   'blocked':blocked.model_dump(mode='json'),'originalReport':retained.model_dump(mode='json'),
   'newBusinessBlocked':rejected.model_dump(mode='json'),'originalCallQuery':queried.model_dump(mode='json'),
   'fresh':fresh.model_dump(mode='json'),'scope':'official SDK, simulated native CLI query only; no UE or native business'})

async def recovery_checks():
 conf,home,new=fixture('UnknownClose');module.HOME=home;bridge=Bridge(conf);bridge._claim_business_lease();bridge.unknown.add('Original')
 old=bridge.lease_path.read_bytes();bridge.close();check('UnknownOnShutdownKeepsOriginalLease',bridge.owns_lease and bridge.lease_path.read_bytes()==old)
 bridge.unknown.clear();save(bridge.run_root/'Calls/Original.json',{'state':'submitted','requiresReview':False})
 bridge.close();check('DurablePendingOnShutdownKeepsOriginalLease',bridge.owns_lease and bridge.lease_path.read_bytes()==old)
 save(bridge.run_root/'Calls/Original.json',{'state':'finished','requiresReview':False});bridge.close()
 check('ResolvedOwnLeaseNormalRelease',not bridge.owns_lease and not bridge.lease_path.exists())
 await runtime_checks()
 await recovery_sdk('2026-07-28');await recovery_sdk('legacy')
 await sdk_check('2026-07-28');await sdk_check('legacy')

def run_recovery():
 global OUT,FIXDOC
 previous=OUT;OUT=BASE/'Checks2';FIXDOC=ROOT/'Docs/Versions/Build128/McpReconnectValidation2'
 assert read(previous/'Review.json')['canAdvance'] and not OUT.exists() and not FIXDOC.exists()
 OUT.mkdir();FIXDOC.mkdir()
 manifest=read(ROOT/'Docs/Versions/Build128/McpReconnectValidation1/Manifest.json')
 page=FIXDOC/'LifecycleFixture.md';page.write_text('# 安全接续\n\n只读查询原现场的适配材料。\n',encoding='utf-8')
 manifest['pages']=[{'pageId':'LifecycleFixture','path':page.relative_to(ROOT).as_posix(),
  'sha256':sha(page.read_bytes()),'bytes':page.stat().st_size}]
 for row in manifest['bindings']:
  if row['path'] in ('Scripts/NodeForgeMcp.py','Scripts/NodeForgeMcpBridge.py'):
   p=ROOT/row['path'];row.update(sha256=sha(p.read_bytes()),bytes=p.stat().st_size)
 save(FIXDOC/'Manifest.json',manifest);asyncio.run(recovery_checks())
 save(OUT/'Review.json',{'status':'passed','canAdvance':True,'allowedNext':'candidate binding review and grouped activation; current Codex still needs actual load',
  'cases':len(rows),'checks':rows,'nativeBuild':'Build128','bridgeVersion':'0.3.1',
  'previousUnaffectedGuards':(previous/'Review.json').relative_to(ROOT).as_posix(),
  'onlyNewRecoveryCloseAndAffectedSdkChecked':True,'SDKUpgrade':False,'UEStarted':False,
  'nativeBusiness':'notRun','newAssets':0,'currentCodex':'notRun'})
 print(json.dumps({'status':'passed','cases':len(rows),'UEStarted':False}))

def resume_recovery():
 global OUT,FIXDOC
 OUT=BASE/'Checks2';FIXDOC=ROOT/'Docs/Versions/Build128/McpReconnectValidation2'
 assert OUT.exists() and not (OUT/'Review.json').exists()
 previous=[read(p) for p in sorted((OUT/'Cases').glob('*.json'),key=lambda p:int(p.stem[4:]))]
 assert sum(not r['passed'] for r in previous)==1 and previous[-1]['case']=='2026-07-28OriginalReportReadableDuringPendingRebind'
 save(OUT/'FailureDiagnosis.json',{'status':'diagnosed','cause':'test reportId violated existing exact 64hex schema; schema validation moved before SDK fixture start',
  'productCodeChanged':False,'originalCloseAndConcurrencyChecksReused':True,
  'originalFailedObservationRetained':True,'UEStarted':False,'nativeCalls':0})
 rows.extend(previous)
 async def affected():
  await recovery_sdk('2026-07-28','Corrected');await recovery_sdk('legacy','Corrected')
  await sdk_check('2026-07-28');await sdk_check('legacy')
 asyncio.run(affected());assert all(r['passed'] for r in rows[len(previous):])
 save(OUT/'Review.json',{'status':'passed','canAdvance':True,'allowedNext':'candidate binding review and grouped activation; actual current Codex load remains required',
  'cases':len(rows),'checks':rows,'supersededFailedObservation':{'originalCase':len(previous),'reason':'invalid fixture ID corrected without changing product schema',
   'replacementCase':next(i+1 for i,r in enumerate(rows) if i>=len(previous) and r['case']=='2026-07-28OriginalReportReadableDuringPendingRebind')},
  'nativeBuild':'Build128','bridgeVersion':'0.3.1','previousUnaffectedGuards':'../Checks1/Review.json',
  'SDKUpgrade':False,'UEStarted':False,'nativeBusiness':'notRun','newAssets':0,'currentCodex':'notRun'})
 print(json.dumps({'status':'passed','cases':len(rows),'affectedRecoveryAndSdkOnly':True,'UEStarted':False}))

def main():
 assert not OUT.exists() and not FIXDOC.exists();OUT.mkdir();FIXDOC.mkdir()
 manifest=read(DOC/'Manifest.json');page=FIXDOC/'LifecycleFixture.md';page.write_text('# 安全接续\n\n仅无UE适配材料，非正式业务证据。\n',encoding='utf-8')
 manifest.update(validationOnly=True,publicationStatus='lifecycleFixtureOnly',
  pages=[{'pageId':'LifecycleFixture','path':page.relative_to(ROOT).as_posix(),'sha256':sha(page.read_bytes()),'bytes':page.stat().st_size}],
  artifacts=[],htmlArtifacts=[],authoringSources=[])
 for row in manifest['bindings']:
  if row['path'] in ('Scripts/NodeForgeMcp.py','Scripts/NodeForgeMcpBridge.py'):
   p=ROOT/row['path'];row.update(sha256=sha(p.read_bytes()),bytes=p.stat().st_size)
 save(FIXDOC/'Manifest.json',manifest)
 synchronous_checks();asyncio.run(main_async())
 save(OUT/'Review.json',{'status':'passed','canAdvance':True,'allowedNext':'review candidate and grouped activation, then actual original Codex discovery',
  'cases':len(rows),'checks':rows,'nativeBuild':'Build128','bridgeVersion':'0.3.1',
  'SDKUpgrade':False,'UEStarted':False,'nativeBusiness':'notRun','newAssets':0,'currentCodex':'notRun'})
 print(json.dumps({'status':'passed','cases':len(rows),'UEStarted':False}))

def resume_legacy():
 assert OUT.exists() and not (OUT/'Review.json').exists()
 previous=[read(p) for p in sorted((OUT/'Cases').glob('*.json'),key=lambda p:int(p.stem[4:]))]
 assert len([r for r in previous if not r['passed']])==1 and previous[-1]['case']=='LegacyTypedToolListNotification'
 assert read(OUT/'Sdk2026-07-28.json')['status']=='passed'
 save(OUT/'FailureDiagnosis.json',{'status':'diagnosed','cause':'locked SDK ServerNotification is a direct typed model, not an old root wrapper',
  'actualModernSDKAndIdleGuards':'passed','actualLegacyRebindAndFreshTools':'passed',
  'affectedCheck':'notification observer only','productCodeChanged':False,'rerun':'legacy SDK notification branch only',
  'originalFailedCheckRetained':True,'UEStarted':False})
 rows.extend(previous);asyncio.run(sdk_check('legacy','Corrected'))
 assert all(r['passed'] for r in rows[len(previous):])
 save(OUT/'Review.json',{'status':'passed','canAdvance':True,'allowedNext':'review candidate and grouped activation, then actual original Codex discovery',
  'cases':len(rows),'checks':rows,'supersededFailedObservation':{'originalCase':len(previous),'reason':'typed SDK observer corrected',
    'replacementCase':next(i+1 for i,r in enumerate(rows) if i>=len(previous) and r['case']=='LegacyTypedToolListNotification')},
  'nativeBuild':'Build128','bridgeVersion':'0.3.1','SDKUpgrade':False,'UEStarted':False,
  'nativeBusiness':'notRun','newAssets':0,'currentCodex':'notRun','unaffectedModernAndGuardChecksReused':True})
 print(json.dumps({'status':'passed','cases':len(rows),'rerun':'legacy only','UEStarted':False}))

if __name__=='__main__':
 if sys.argv[1:2] in (['--stdio'],['--retainedstdio']):
  if sys.argv[1:2]==['--retainedstdio']:
   OUT=BASE/'Checks2';FIXDOC=ROOT/'Docs/Versions/Build128/McpReconnectValidation2'
   import NodeForgeMcp as server_module
   server_module.Bridge=retained_stdio_factory
  elif str(BASE/'Checks2') in str(sys.argv[2]):
   OUT=BASE/'Checks2';FIXDOC=ROOT/'Docs/Versions/Build128/McpReconnectValidation2'
  conf,home=map(Path,sys.argv[2:]);assert home.resolve().is_relative_to(OUT) and conf.resolve().is_relative_to(OUT)
  module.HOME=home;asyncio.run(serve(conf))
 elif sys.argv[1:]==['--resume-legacy']:resume_legacy()
 elif sys.argv[1:]==['--recovery']:run_recovery()
 elif sys.argv[1:]==['--resume-recovery']:resume_recovery()
 else:main()
