"""Mcp2 knowledge-metadata refresh only; isolated bounded files, no SDK/CLI/UE."""
import argparse,ast,copy,json,re,sys,threading,types
from pathlib import Path
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Reports/PluginControlStageTwo/McpMaintenance1/E3/KnowledgeRefresh'
sys.path.insert(0,str(ROOT/'Scripts'))
E3=OUT.parent

def read(path):return json.loads(path.read_text('utf-8'))
def module_at(path):
    module=types.ModuleType('NodeForgeStagedKnowledgeBridge')
    module.__file__=str(ROOT/'Scripts/NodeForgeMcpBridge.py')
    exec(compile(path.read_text('utf-8'),module.__file__,'exec'),module.__dict__)
    return module
def main():
    global OUT,E3
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report-name',default='KnowledgeRefresh')
    args=parser.parse_args()
    if not re.fullmatch('[A-Za-z][A-Za-z0-9]{1,47}',args.report_name):raise ValueError('Bounded English report name required')
    OUT=ROOT/'Reports/PluginControlStageTwo/McpMaintenance1/E3'/args.report_name
    E3=OUT.parent
    OUT.mkdir(exist_ok=False)
    old=module_at(E3/'BeforeKnowledgeRefresh/CandidateBridge.py.txt')
    new=module_at(E3.parent/'E2/PendingActivation/NodeForgeMcpBridge.py.txt')
    assert old.VERSION==new.VERSION=='0.2.0'
    fake=OUT/'Repository';home=fake/'Docs/Versions/Build122/Mcp2R1';home.mkdir(parents=True)
    page=home/'Readme.md';page.write_text('# 原版知识\n',encoding='utf-8')
    html=home/'Readme.html';html.write_text('<h1>原版知识</h1>',encoding='utf-8')
    author=fake/'Docs/Author/Readme.md';author.parent.mkdir(parents=True);author.write_bytes(page.read_bytes())
    notice=fake/'Docs/Author/Notice.md';notice.write_text('本机资料来源',encoding='utf-8')
    def ref(path):return {'path':path.relative_to(fake).as_posix(),'sha256':new.sha(path.read_bytes()),'bytes':path.stat().st_size}
    base=read(ROOT/'Docs/Versions/Build122/Mcp2R1/Manifest.json')
    base['pages']=[{'pageId':'Readme','kind':'guide',**ref(page)}]
    base['artifacts']=[r for r in base['artifacts'] if Path(r['path']).name in ('Catalog.json','Operations.json')]
    base['authoringSources']=[ref(author)];base['htmlArtifacts']=[ref(html)]
    for name in ('pages','nodeDocumentation','controls'):base['publicationInputs'][name]=ref(author)
    base['publicationInputs']['sourceNotice']=ref(notice)
    config=read(ROOT/'Reports/PluginControlStageTwo/McpDocs1/Client/Config.json')
    config['bundleId']=base['bundleId']
    manifest_path=home/'Manifest.json';config_path=OUT/'Config.json'
    def publish(value,fields=None):
        manifest_path.write_bytes(new.encoded(value)+b'\n')
        current={**config,'docsManifestSha256':new.sha(manifest_path.read_bytes()),**(fields or {})}
        config_path.write_bytes(new.encoded(current)+b'\n');return current
    initial=publish(base)
    def fixture(module):
        bridge=module.Bridge.__new__(module.Bridge)
        bridge.config_path,bridge.manifest_path,bridge.docs_root=config_path,manifest_path,home
        bridge.config,bridge.manifest=copy.deepcopy(initial),copy.deepcopy(base)
        bridge.metadata_lock=threading.RLock()
        bridge.tools={r['name']:r for r in read(ROOT/'Docs/Versions/Build122/Mcp2R1/ToolContract.json')['tools']}
        bridge.bundle_id=base['bundleId'];bridge.handle='1'*32;bridge.identity={'controlSessionId':'2'*32}
        bridge.reports={'OriginalReport':{'unchanged':True}};bridge.calls={'UnknownCall':{'state':'unknown'}}
        bridge.unknown={'UnknownCall'};bridge.pending_cli=['OriginalPending']
        return bridge
    legacy=fixture(old);bridge=fixture(new);checks=[]
    def record(name,condition,detail=None):
        row={'name':name,'status':'passed' if condition else 'failed','detail':detail}
        checks.append(row);(OUT/('Case'+str(len(checks))+'.json')).write_text(json.dumps(row,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        assert condition,name
    page.write_text('# 更新知识\n',encoding='utf-8');author.write_bytes(page.read_bytes());html.write_text('<h1>更新知识</h1>',encoding='utf-8')
    updated=copy.deepcopy(base);updated['pages'][0].update(ref(page));updated['authoringSources']=[ref(author)];updated['htmlArtifacts']=[ref(html)]
    for name in ('pages','nodeDocumentation','controls'):updated['publicationInputs'][name]=ref(author)
    approved=publish(updated)
    with patch.object(old,'ROOT',fake):
        value=legacy.call('nodeforge_docs_index',{})
    record('PreviousCandidateRejectsPureKnowledgeMetadata',value['status']=='environmentBlocked' and value['errors'][0]['code']=='NFMCP_CONFIG_RELOAD_REQUIRED',value['errors'])
    state=lambda:(bridge.handle,copy.deepcopy(bridge.identity),copy.deepcopy(bridge.reports),copy.deepcopy(bridge.calls),set(bridge.unknown),list(bridge.pending_cli))
    before=state()
    lease=OUT/'OriginalOwnerMarker.txt';lease.write_text('Original marker; not a business lease',encoding='utf-8');lease_bytes=lease.read_bytes()
    with patch.object(new,'ROOT',fake):
        index=bridge.call('nodeforge_docs_index',{});value=bridge.call('nodeforge_docs_read',{'pageId':'Readme'})
        record('ApprovedMcp2PageAuthorHtmlAndSourceShaRefresh',index['status']==value['status']=='passed' and value['text']==page.read_bytes().decode('utf-8')
            and value['docsManifestSha256']==approved['docsManifestSha256'],{'indexStatus':index['status'],'readStatus':value['status'],'errors':value.get('errors')})
        record('UnknownPendingCallReportHandleAndOwnerStateRetained',state()==before and lease.read_bytes()==lease_bytes)
        def reject(name,mutate,code):
            candidate=copy.deepcopy(updated);mutate(candidate);publish(candidate)
            value=bridge.call('nodeforge_docs_index',{})
            record(name,value['status']=='environmentBlocked' and value['errors'][0]['code']==code
                and bridge.manifest==updated and bridge.config==approved,value['errors'])
        reject('ToolMetadataInputStillRequiresReload',lambda x:x['publicationInputs']['toolMetadata'].update(sha256='0'*64),'NFMCP_CONFIG_RELOAD_REQUIRED')
        reject('CatalogArtifactStillRequiresReload',lambda x:x['artifacts'][0].update(sha256='0'*64),'NFMCP_CONFIG_RELOAD_REQUIRED')
        reject('RuntimeSourceBindingStillRequiresReload',lambda x:x['bindings'][0].update(sha256='0'*64),'NFMCP_CONFIG_RELOAD_REQUIRED')
        reject('HtmlOutsidePackageRejected',lambda x:x['htmlArtifacts'][0].update(path='Reports/Other.html'),'NFMCP_DOCUMENT_UNAVAILABLE')
        reject('AuthorOutsideDocsRejected',lambda x:x['authoringSources'][0].update(path='Tools/Private.txt'),'NFMCP_DOCUMENT_UNAVAILABLE')
        reject('MalformedDocumentInputRejected',lambda x:x['publicationInputs'].update(controls='invalid'),'NFMCP_CONFIG_RELOAD_REQUIRED')
        reject('DocumentInputPathChangeRequiresReload',lambda x:x['publicationInputs']['controls'].update(path='Docs/Author/Other.md'),'NFMCP_CONFIG_RELOAD_REQUIRED')
    # Full business functions must stay identical; only VERSION and _documents may differ.
    def normalized(path):
        tree=ast.parse(path.read_text('utf-8'))
        for item in tree.body:
            if isinstance(item,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='VERSION' for t in item.targets):item.value=ast.Constant('normalized')
            if isinstance(item,ast.ClassDef) and item.name=='Bridge':item.body=[n for n in item.body if not (isinstance(n,ast.FunctionDef) and n.name=='_documents')]
        return ast.dump(tree,include_attributes=False)
    before_source=E3/'BeforeKnowledgeRefresh/CandidateBridge.py.txt';after_source=E3.parent/'E2/PendingActivation/NodeForgeMcpBridge.py.txt'
    record('AllNonDocumentBridgeCodeUnchanged',normalized(before_source)==normalized(after_source))
    summary={'stage':'E3/knowledge-refresh','status':'passed','canAdvance':True,'appliesOnlyTo':'new documentary metadata admission; not full E3/client/final acceptance',
        'checks':checks,'candidateSource':{'path':after_source.relative_to(ROOT).as_posix(),'sha256':new.sha(after_source.read_bytes()),'bytes':after_source.stat().st_size},
        'currentRootBridgeNotChanged':True,'productionHTMLNotWritten':True,'testScope':'Isolated real bounded files; staged Bridge._documents; no SDK/CLI/UE/business submission',
        'sdk':'notRun','cli':'notRun','ue':'notRun','newAssets':False,'originalStateRetained':True}
    (OUT/'Review.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'knowledgeMetadataRefresh':'passed','checks':len(checks),'UEStarted':False,'productionHTMLWritten':False}))
if __name__=='__main__':main()
