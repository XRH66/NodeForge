"""One finite B2 adapter/SDK check; fixtures are never UE/Codex business evidence."""
import asyncio, copy, json, sys
from pathlib import Path
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT/'Scripts'),str(ROOT/'Tests/PluginControl')]
import NodeForgeMcpBridge as module
from McpBridgeChecks import Fixture, read, save
from NodeForgeMcpBridge import Bridge, BridgeError, encoded, sha, resource_payload
from jsonschema import Draft202012Validator
from mcp import StdioServerParameters
from mcp.client import Client

BASE=ROOT/'Reports/PluginControlStageTwo/AssetRead1/B2'
DOC=ROOT/'Docs/Versions/Build125/Mcp3R1'
OUT=BASE/'Adapter2'
manifest=read(DOC/'Manifest.json')
contract=read(DOC/'ToolContract.json')

def config(out, report_root):
    data={'format':'NodeForgeMcpConfig','version':1,'bundleId':manifest['bundleId'],
        'project':(ROOT/manifest['project']).as_posix(),'pluginDirectory':(ROOT/manifest['stage']).as_posix(),
        'binarySha256':manifest['binarySha256'],'reportRoot':report_root.as_posix(),
        'sessionPublic':(report_root/'SessionPublic.json').as_posix(),'bridgeRunRoot':(out/'Bridge').as_posix(),
        'toolContractSha256':sha((DOC/'ToolContract.json').read_bytes()),'docsManifestSha256':sha((DOC/'Manifest.json').read_bytes())}
    path=out/'Config.json';save(path,data);return path

def public():
    # Reuse the real field shape; all process/session identity below is synthetic.
    data=read(ROOT/'Reports/PluginControlStageTwo/RulesCatalog1/Run2/SessionPublic.json')
    data.update(pid=42,processStart='1',controlSessionId='c'*32,project=(ROOT/manifest['project']).as_posix(),
        module=(ROOT/manifest['stage']/'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll').as_posix(),
        binarySha256=manifest['binarySha256'],pipe='SyntheticAssetReadFixture',
        sessionFile='SyntheticPrivateDescriptorNotRead',engine=manifest['engine'])
    return data

class ReadFixture(Fixture):
    def __init__(self,out):
        self.native_results={};self.originals={}
        super().__init__(out,config,public())

    def cli(self,action,operation=None,arguments=None,call_id=None,revision=None):
        summary=super().cli(action,operation,arguments,call_id,revision)
        if operation=='calls.get' and arguments['callId'] in self.originals:
            summary['response']['result']=copy.deepcopy(self.originals[arguments['callId']])
            return summary
        if operation not in ('resources.inspect','graphs.export') or summary['cliExitCode']!=0:return summary
        mode=arguments.get('kind') or ('MaterialGraphExport' if arguments['assetKind']=='Material' else 'BlueprintGraphExport')
        kind=arguments.get('assetKind') or ('MaterialFunction' if mode=='MaterialFunction' else 'Material')
        src_sha='a'*64
        native={'reportVersion':'ResultContractDraft1','mode':mode,'overallStatus':'succeeded','operationStatus':'passed',
            'pid':42,'project':self.public['project'],'sessionId':'Native'+call_id,
            'resourceRead':{'source':arguments['source'],'kind':kind,'savedSourceSha256':src_sha,
                'configurationDigest':sha(encoded(arguments)),'originProof':'controlledSavedLoad','sourceFileHeldReadOnly':True}}
        catalog=''
        if mode=='MaterialParameters':
            native['parameters']={'parent':arguments['source'],'parentAssetSha256':src_sha,
                'scalarParameters':{'ExampleScalar':0.3},'vectorParameters':{},'textureParameters':{},'textureSamplerSignatures':[]}
        elif mode=='MaterialFunction':
            properties={'MaterialFunction':arguments['source'],'FunctionAssetSha256':src_sha}
            native.update(functionSignature={'properties':properties,'inputs':[],'outputs':[],
                'defaultPolicy':'Native preview default only','hashScope':'Direct saved function file'},
                nodeTemplate={'id':'Function','class':'/Script/Engine.MaterialExpressionMaterialFunctionCall','properties':properties})
        else:
            sample='StaticColor' if kind=='Material' else 'BlueprintConcat'
            request=read(ROOT/('Examples/GraphV2Samples/'+sample+'.json'))
            request.update(requestId=arguments['requestId'],assetPath=arguments['assetPath'],catalogDigest=manifest['catalogDigest'])
            text=json.dumps(request,ensure_ascii=False,indent=2)+' '*40000
            catalog=manifest['catalogDigest']
            native.update(catalogDigest=catalog,exportedRequest=request,exportedRequestText=text,
                exportedRequestSha256=sha(text.encode('utf-8')),exportEvidence={'source':arguments['source'],
                    'sourceAssetSha256':src_sha,'sourceBytesUnchanged':True,'sourceRemainsClean':True})
        native_path=self.report_root/(call_id+'Native.json');save(native_path,native)
        snapshot=self.report_root/(call_id+'Snapshot.json');save(snapshot,arguments)
        reference={'path':native_path.as_posix(),'sha256':sha(native_path.read_bytes()),'bytes':native_path.stat().st_size,
            'nativeSessionId':native['sessionId'],'overallStatus':native['overallStatus']}
        sidecar={'controlSessionId':self.public['controlSessionId'],'binarySha256':self.public['binarySha256'],
            'callId':call_id,'operation':operation,'host':'activeEditor','inputMethod':'editorProgram','draftRevision':revision,
            'pid':42,'processStart':'1','project':self.public['project'],'module':self.public['module'],
            'nativeReport':reference,'querySnapshot':snapshot.as_posix(),'queryDigest':sha(snapshot.read_bytes()),'catalogDigest':catalog}
        sidecar_path=self.report_root/(call_id+'Control.json');save(sidecar_path,sidecar)
        call=summary['response']['result']
        call.update(expectedDraftRevision=revision,result={'nativeReport':reference,'controlEvidencePath':sidecar_path.as_posix(),
            'controlEvidenceSha256':sha(sidecar_path.read_bytes()),'querySnapshot':snapshot.as_posix(),
            'queryDigest':sha(snapshot.read_bytes()),'catalogDigest':catalog,'cliExitCode':0})
        self.native_results[call_id]=native;self.originals[call_id]=copy.deepcopy(call)
        return summary

async def sdk():
    conf=config(OUT/'Sdk',OUT/'NoLiveSession')
    async with Client(StdioServerParameters(command=sys.executable,
        args=[str(BASE/'SdkCandidateServer.py'),'--config',str(conf)],cwd=ROOT),read_timeout_seconds=20) as client:
        inventory=await client.list_tools()
        assert {r.name for r in inventory.tools}=={r['name'] for r in contract['tools']}
        descriptions={r.name:r.description for r in inventory.tools}
        assert all('AiAssetRead' in descriptions[n] for n in ('nodeforge_resources_inspect','nodeforge_graphs_export'))
        index=await client.call_tool('nodeforge_docs_index',{})
        page=await client.call_tool('nodeforge_docs_read',{'pageId':'AiAssetRead'})
        assert not index.is_error and not page.is_error
        assert index.structured_content['manifest']['bundleId']==manifest['bundleId']
        assert page.structured_content['text']==(DOC/'Ai/AssetRead.md').read_text('utf-8')
        assert page.structured_content['docsManifestSha256']==sha((DOC/'Manifest.json').read_bytes())
        save(OUT/'Sdk.json',{'protocolVersion':client.protocol_version,'tools':[r.model_dump(mode='json') for r in inventory.tools],
            'index':index.model_dump(mode='json'),'page':page.model_dump(mode='json'),
            'entry':'production NodeForgeMcp.serve + Bridge with exact isolated candidate ToolContract HOME',
            'scope':'official SDK stdio and new discovery/docs only; no UE/CLI dispatch/current Codex acceptance'})

def main():
    assert read(BASE/'PublicationInputReview.json')['canAdvance'] and not OUT.exists()
    OUT.mkdir();checks=[]
    sys.path.insert(0,str(BASE))
    from SdkCandidateServer import candidate_project_selection
    with patch.object(module,'HOME',DOC),candidate_project_selection():
        fixture=ReadFixture(OUT/'Receipts')
        try:
            assert fixture.handle and len(fixture.bridge.tools)==17
            before=len(fixture.invocations)
            bad=fixture.gt('InvalidKind','resources.inspect',arguments={'kind':'Other','source':'/Engine/Example.Example'})
            one=fixture.gt('UnpairedOrigin','draft.replace',arguments={'text':'{}','exportReportPath':'not-a-report'})
            assert bad['status']==one['status']=='rejected' and len(fixture.invocations)==before
            checks.append({'area':'new input modes and paired origin schema','status':'passed','cliDispatched':False})
            for cid,op,args in [
                ('MatRead','resources.inspect',{'kind':'MaterialParameters','source':'/Game/NodeForgeTests/Material/B000002/Sample01.Sample01'}),
                ('FnRead','resources.inspect',{'kind':'MaterialFunction','source':'/Engine/Functions/Engine_MaterialFunctions02/SmoothStep.SmoothStep'}),
                ('MatExport','graphs.export',{'assetKind':'Material','source':'/Game/NodeForgeTests/Material/B000002/Sample01.Sample01','requestId':'FixtureMaterial','assetPath':'/Game/NodeForgeTests/Fixture/Material'}),
                ('BpExport','graphs.export',{'assetKind':'Blueprint','source':'/Game/NodeForgeTests/Blueprint/B000002/Sample01.Sample01','requestId':'FixtureBlueprint','assetPath':'/Game/NodeForgeTests/Fixture/Blueprint'})]:
                value=fixture.gt(cid,op,arguments=args)
                assert value['status']=='finished' and value['cliExitCode']==0 and not value['unresolvedCalls'],value
                tool=fixture.bridge.tools['nodeforge_'+op.replace('.','_')]
                Draft202012Validator(tool['outputSchema']).validate(value)
                rid=value['reports'][0]['reportId']
                pointer='/exportedRequest' if op=='graphs.export' else '/parameters' if cid=='MatRead' else '/functionSignature'
                selected=fixture.bridge.call('nodeforge_report_read',{'handle':fixture.handle,'reportId':rid,'pointer':pointer})
                assert selected['status']=='passed',selected
                Draft202012Validator(fixture.bridge.tools['nodeforge_report_read']['outputSchema']).validate(selected)
                if op=='graphs.export':
                    assert len(selected['text'].encode('utf-8'))>32768
                    assert selected['text']==fixture.native_results[cid]['exportedRequestText']
                    assert selected['value']==json.loads(selected['text'])
                    assert selected['sourceRef']['exportReportSha256']==value['reports'][0]['sha256']
                    assert selected['sourceRef']['payloadSha256']==sha(selected['text'].encode('utf-8'))
                    ordinary=fixture.bridge.call('nodeforge_report_read',{'handle':fixture.handle,'reportId':rid,'pointer':'/exportedRequestText'})
                    assert ordinary['status']=='resultTooLarge' and ordinary['selectionReturned'] is False
                save(OUT/(cid+'Result.json'),{'projected':value,'selected':selected,'syntheticUeReceipt':True})
            checks.append({'area':'four mode native receipts + complete >32KiB Graph text + ordinary subtree limit','status':'passed','syntheticOnly':True})
            for cid,remove in [('MatRead','parameters'),('FnRead','nodeTemplate'),('MatExport','exportedRequestSha256')]:
                native=copy.deepcopy(fixture.native_results[cid]);native.pop(remove)
                try:resource_payload(native,fixture.bridge.calls[cid]['arguments'])
                except BridgeError as error:assert error.status=='unknown'
                else:raise AssertionError('Malformed successful native receipt admitted')
            checks.append({'area':'missing required native payload remains unknown','status':'passed','syntheticOnly':True})
            fixture.overrides['OriginalUnknown']=(5,'unknown')
            unknown=fixture.gt('OriginalUnknown','graphs.export',arguments={'assetKind':'Material','source':'/Engine/Example.Example','requestId':'FixtureNew','assetPath':'/Game/NodeForgeTests/Fixture/New'})
            before=len(fixture.invocations)
            following=fixture.gt('AfterUnknown','resources.inspect',arguments={'kind':'MaterialParameters','source':'/Engine/Example.Example'})
            assert unknown['status']==following['status']=='unknown' and len(fixture.invocations)==before
            checks.append({'area':'new operations preserve unknown and stop dependent dispatch','status':'passed','syntheticOnly':True})
        finally:fixture.close()
    asyncio.run(sdk())
    save(OUT/'Review.json',{'format':'NodeForgeAssetReadStepReview1','step':'B2 one targeted adapter/SDK check',
        'status':'passed','canAdvance':True,'checks':checks,'actualSdkDiscovery':'17 tools, candidate bundle/help/AiAssetRead via production serve',
        'ueStarted':False,'realBusiness':'notRun','currentCodex':'notRun','newAssets':0,
        'projectSelection':'in-memory candidate .uproject fixture; real project/ledger remain Build122 until B3 preparation',
        'existingSafetyMatrix':'reused by impact; not rerun','scope':'synthetic CLI/native receipts, real bounded report files, actual official SDK stdio'})
    print(json.dumps({'status':'passed','sdkTools':17,'scope':'B2 adapter/stdio; UE and current Codex business notRun'}))

if __name__=='__main__':main()
