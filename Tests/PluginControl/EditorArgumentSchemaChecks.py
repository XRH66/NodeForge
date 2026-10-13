"""No-UE checks of formal execute admission against real operation metadata."""
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch
import copy,json,sys,time
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'Scripts'))
import NodeForgeEditorTransport as transport
from RunGraphRegression import write_new,sha,ref
OUT=ROOT/'Reports/PluginControlStageTwo/AssetRead1/B3/TransportSchemaRepair1/ChecksCorrected'
RUN=OUT.parent.parent/'Run3'
def read(p):return json.loads(p.read_text('utf-8-sig'))
assert not OUT.exists();OUT.mkdir()
runtime=read(RUN/'Describe/Response.json')['result']
definitions={r['name']:r for r in runtime['operations']}
payload=read(RUN/'Codex/MaterialExportPayload.json')['result']['structuredContent']
original={'text':payload['text'],'exportReportPath':payload['sourceRef']['exportReportPath'],
          'exportReportSha256':payload['sourceRef']['exportReportSha256']}
rows=[]
def check(name,args,schema,allowed):
    error=None
    try:transport.validate_arguments(args,schema)
    except transport.TransportError as e:error=e.code
    assert (error is None)==allowed,(name,error)
    rows.append({'case':name,'passed':True,'expectedAdmission':allowed,'actualError':error})
replace=definitions['draft.replace']['argumentsSchema'];load=definitions['draft.loadFile']['argumentsSchema']
check('OriginalExportPayloadAndPairedOriginAccepted',original,replace,True)
check('ExistingPlainJsonStillAccepted',{'text':payload['text']},replace,True)
check('FileLoadWithPairedOriginAccepted',{'path':str(RUN/'MaterialInput.json'),**{k:v for k,v in original.items() if k!='text'}},load,True)
check('MissingRequiredTextRejected',{k:v for k,v in original.items() if k!='text'},replace,False)
check('UnknownFieldRejected',{**original,'unexpected':'x'},replace,False)
check('WrongDeclaredStringTypeRejected',{**original,'exportReportSha256':1},replace,False)
for missing in ('exportReportPath','exportReportSha256'):
    check('UnpairedOriginRejected'+missing,{k:v for k,v in original.items() if k!=missing},replace,False)
future=copy.deepcopy(replace);future['properties']['declaredOptional']={'type':'string'}
check('MetadataOptionalFieldRequiresNoPerOperationAdapter',{**original,'declaredOptional':'future'},future,True)
for name,row in definitions.items():
    if row['dispatch']=='handshake':continue  # execute rejects these before argument admission.
    schema=row['argumentsSchema']
    args={key:('x') for key in schema['required']}
    check('ExistingRequiredShape-'+name,args,schema,True)
class PublicFiles:
    def no_reparse(self,path):pass
    def bounded_public_file(self,path,limit):
        data=Path(path).read_bytes();assert 0<len(data)<=limit;return data
class Session:
    def __init__(self,args,*unused):
        self.win=PublicFiles();self.data=runtime['identity'];self.evidence=self.data
        self.deadline=time.monotonic()+args.timeout_seconds;self.sent=[]
    def request(self,method,args,call_id,revision=None):
        self.sent.append({'method':method,'arguments':args,'callId':call_id,'revision':revision})
        if method=='operations.describe':return {'jsonrpc':'2.0','id':call_id,'result':runtime}
        assert method=='draft.replace' and args==original and revision=='1'
        return {'jsonrpc':'2.0','id':call_id,'result':{'state':'completed','cliExitCode':0}}
    def close(self):Session.observed=self.sent
argsfile=OUT/'Arguments.json';write_new(argsfile,original)
args=SimpleNamespace(timeout_seconds=10,run_directory=str(OUT/'FormalExecute'),action='editor-call',
    operation='draft.replace',arguments_file=str(argsfile),expected_revision='1',call_id='SchemaAdmissionProof')
summary={}
with patch.object(transport,'Session',Session):
    code=transport.execute(args,summary,ROOT,sha,write_new,None,None)
assert code==0 and summary['executionState']=='finished'
assert Session.observed==[{'method':'operations.describe','arguments':{},'callId':'Describe-1','revision':None},
                         {'method':'draft.replace','arguments':original,'callId':'SchemaAdmissionProof','revision':'1'}]
assert (OUT/'FormalExecute/Request.json').exists()
rows.append({'case':'FormalExecuteReachesExactlyOneNativeDispatchWithOriginalFields','passed':True,
             'mockSession':True,'UEStarted':False,'liveBusinessProof':False})
write_new(OUT/'Review.json',{'status':'passed','canAdvance':True,'allowedNext':'candidate publication then actual current MCP original-payload load',
    'cases':rows,'transportSha256':sha(ROOT/'Scripts/NodeForgeEditorTransport.py'),
    'operationMetadata':ref(RUN/'Describe/Response.json'),'newDependencies':False,
    'scope':'formal execute tested through real metadata and payload with mocked native Session; no UE or product asset writes',
    'SDKWholeBatchRerun':False,'liveBusiness':'notRun'})
print(json.dumps({'status':'passed','checks':len(rows),'formalExecuteDispatch':1,'UEStarted':False}))
