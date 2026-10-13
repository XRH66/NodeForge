"""Build the bounded Build122/Mcp1 usage package from frozen native exports.

No UE calls, project allocation, asset mutation, or second node allowlist.
Candidate generation is explicit; published versions must not be regenerated.
"""
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HOME = ROOT / 'Docs/Versions/Build122/Mcp1'
REPORT = ROOT / 'Reports/PluginControlStageTwo/McpDocs1/P2'
CATALOG = ROOT / 'Reports/PluginControlStageTwo/RulesCatalog1/Run2/Calls/CatalogNative.json'
OPERATIONS = ROOT / 'Reports/PluginControlStageTwo/McpDocs1/P0/OperationsSnapshot.json'
VERIFICATIONS = [
    'UE532GraphVerification', 'UE532BlueprintMotionVerification',
    'UE532TextureFunctionsVerification', 'UE532MaterialSpatialVerification',
    'UE532BlueprintFamiliesVerification', 'UE532BlueprintComponentsVerification',
    'UE532BlueprintMemberScalarsVerification', 'UE532BlueprintSequenceVerification',
    'UE532BlueprintRotatorsVerification', 'UE532MaterialColorSelectionVerification',
    'UE532BlueprintMathInterpolationVerification', 'UE532BlueprintSwitchesVerification',
    'UE532BlueprintStringIdentityVerification',
]

def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def load(path): return json.loads(path.read_text('utf-8'))
def dump(value): return json.dumps(value, ensure_ascii=False, indent=2, allow_nan=False) + '\n'
def ref(path): return {'path': path.relative_to(ROOT).as_posix(), 'sha256': digest(path), 'bytes': path.stat().st_size}
def save(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding='utf-8', newline='\n')
def fence(value): return '```json\n' + dump(value) + '```\n'
def declared_bridge_version(path):
    """Read a literal source identity without importing/executing bridge code."""
    import ast
    matches=[]
    for item in ast.parse(path.read_text('utf-8')).body:
        if isinstance(item,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='VERSION' for t in item.targets):
            matches.append(ast.literal_eval(item.value))
    if len(matches)!=1 or not isinstance(matches[0],str):raise ValueError('One literal bridge VERSION required')
    return matches[0]
def layout_publication_rows(output,bundle_id):
    """Bind the existing renderer's local theme sources and generated resources."""
    from NodeForgeEditorTransport import Windows
    layout_path=output/'LayoutManifest.json'
    layout=load(layout_path)
    if layout.get('format')!='NodeForgeDocsLayout1' or layout.get('bundleId')!=bundle_id:
        raise ValueError('Layout identity mismatch')
    def checked(rows,source=False):
        if not isinstance(rows,list) or len({r['path'] for r in rows})!=len(rows):raise ValueError('Invalid/duplicate layout references')
        result=[]
        for row in rows:
            path=(ROOT/row['path']).resolve()
            allowed=(path in (ROOT/'Scripts/RenderNodeForgeDocs.mjs',ROOT/'Scripts/RenderNodeForgeDocsTheme2.mjs',ROOT/'Scripts/RenderNodeForgeDocsHero.mjs') or path.is_relative_to(ROOT/'Tools/NodeForgeDocs')) if source else path.is_relative_to(output)
            if Path(row['path']).is_absolute() or not allowed or not path.is_file():raise ValueError('Layout reference outside its scope')
            Windows().no_reparse(path)
            actual=ref(path)
            if actual!=row:raise ValueError('Layout reference SHA/bytes changed')
            result.append(actual)
        return result
    return {'sources':checked(layout['sourceFiles'],True),'html':checked(layout['htmlArtifacts']),
        'markdown':checked(layout['markdown']),'artifacts':[ref(layout_path)]+checked(layout['assets'])}
def row_binding(row):
    binding = row.get('properties') or row.get('catalogBinding') or row.get('binding') or {}
    if isinstance(binding, list): binding = {}
    return {k: binding[k] for k in ('owner', 'member') if k in binding}
def identity(kind, row):
    answer = {'assetKind': kind, 'class': row['class']}
    if row_binding(row): answer['binding'] = row_binding(row)
    if row.get('variableType'): answer['variableType'] = row['variableType']
    if row['class'].endswith('ExecutionSequence'): answer['configuration'] = row.get('properties', {})
    if 'K2Node_Switch' in row['class']: answer['configuration'] = row.get('properties', {})
    return answer
def matches(kind, node, row):
    if row.get('class') != node['class']: return False
    if row.get('assetType', kind) != kind: return False
    if node['class'].endswith(('CallFunction', 'K2Node_Event')):
        return bool(row_binding(row)) and row_binding(row) == row_binding(node)
    if node['class'].endswith(('VariableGet', 'VariableSet')):
        return bool(row.get('variableType')) and row['variableType'] == node['variableType']
    if node['class'].endswith('ExecutionSequence'):
        return row.get('properties') == node.get('properties')
    return True
def paths_in(value):
    if isinstance(value, dict):
        for v in value.values(): yield from paths_in(v)
    elif isinstance(value, list):
        for v in value: yield from paths_in(v)
    elif isinstance(value, str) and re.match(r'^(\.\./)?Reports/.+\.(json|md)$', value, re.I):
        yield value[3:] if value.startswith('../') else value
def public_evidence(path):
    p = (ROOT / path).resolve()
    if not p.is_relative_to(ROOT / 'Reports'): raise ValueError('Evidence outside Reports')
    if not p.is_file(): return {'path': path, 'availability': 'missing; no inferred pass'}
    answer = ref(p)
    if p.suffix.lower() == '.json':
        d = load(p)
        if isinstance(d, dict):
            answer['actualSummary'] = {k: d[k] for k in
                ('mode', 'overallStatus', 'operationStatus', 'matrixResult', 'result', 'status', 'pid',
                 'engine', 'adapter', 'project', 'inputDigest', 'binarySha256', 'requestId', 'assetPath')
                if k in d and isinstance(d[k], (str, int, bool, type(None)))}
            if isinstance(d.get('checks'), dict):
                answer['actualCheckStatuses'] = {k: v.get('status') if isinstance(v, dict) else v
                    for k, v in d['checks'].items()}
    return answer

def build_tool_contract(base, operations, metadata, bundle_id, bridge_version, manifest_path):
    """Explicit positive AI policy; native definitions and argument schemas stay authoritative."""
    import copy
    native = {row['name']: row for row in operations['operations']}
    if metadata.get('defaultAiExposure') != 'deny': raise ValueError('AI policy must default deny')
    contract = copy.deepcopy(base)
    rows = []
    for name, help_row in metadata['tools'].items():
        operation = help_row['operation']
        original = next((t for t in base['tools'] if t['name']==name and t['operation']==operation), None)
        if original is None:
            if (name!='nodeforge_resources_query' or operation!='resources.query'
                    or native.get(operation,{}).get('dispatch')!='gameThread'
                    or native[operation].get('expectedDraftRevisionRequired') is not True
                    or native[operation].get('sideEffect')!='metadataQueryReportOnly'):
                raise ValueError('New tool needs an explicit reviewed adapter: '+name)
            original=copy.deepcopy(next(t for t in base['tools'] if t['name']=='nodeforge_rules_query'))
            original.update(name=name,operation=operation,nativeDefinition=copy.deepcopy(native[operation]))
            original['inputSchema']['properties']['arguments']=copy.deepcopy(native[operation]['argumentsSchema'])
        if operation in ('session.hello','control.disable'): raise ValueError('Management operation is not AI open')
        if operation is not None and native.get(operation) != original['nativeDefinition']:
            expected=copy.deepcopy(original['nativeDefinition'])
            if operation=='graphs.export':
                expected['argumentsSchema']['properties']['formatVersion']={'type':'integer','enum':[2,3],'default':2}
            if operation=='resources.inspect':
                expected['argumentsSchema']['properties']['kind']['enum']=['MaterialParameters','MaterialFunction','Texture2D']
            if native.get(operation)!=expected:
                raise ValueError('Native definition changed outside reviewed export/Texture additions: '+operation)
        for field in ('purpose','preconditions','effects','success','guides'):
            if not help_row.get(field): raise ValueError('Incomplete help: '+name+'/'+field)
        row = copy.deepcopy(original)
        if name == 'nodeforge_connect':
            row['inputSchema']['properties'].update(project={'type':'string','minLength':1,
                'description':'Optional exact uproject; unique qualified session auto-selects if omitted'},
                controlSessionId={'type':'string','pattern':'^[0-9a-f]{32}$',
                'description':'Select one exact instance when multiple sessions are present'})
        if operation in ('graphs.export','resources.inspect'):
            row['nativeDefinition']=copy.deepcopy(native[operation])
            row['inputSchema']['properties']['arguments']=copy.deepcopy(native[operation]['argumentsSchema'])
        row['help'] = copy.deepcopy(help_row)
        field_help = '; '.join(k+': '+metadata['fields'][k] for k in row['inputSchema']['properties'])
        row['description'] = (help_row['purpose']+' 前提：'+help_row['preconditions']+' 参数：'+field_help+
            ' 效果：'+help_row['effects']+' 判据：'+help_row['success']+
            ' 资料pageId：'+', '.join(help_row['guides'])+'。当前版从docs_index确认；unknown/pending停写查原call。')
        schema = {'type':'object','required':['bridgeVersion','status'], 'additionalProperties':True,
            'properties':{'bridgeVersion':{'type':'string','pattern':r'^\d+\.\d+\.\d+$',
                'description':'Actually loaded bridge release; exact identity is bound by the approved contract and manifest'},'status':{'type':'string',
                'description':'Bridge/CLI state; never replaces native overallStatus or persistence verdict'},
                'cliExitCode':{'type':'integer'},'nativeSummary':{'type':'object','additionalProperties':True},
                'errors':{'type':'array','items':{'type':'object','properties':{'code':{'type':'string'}},'additionalProperties':True}},
                'unresolvedCalls':{'type':'array','items':{'type':'string'}},
                'reports':{'type':'array','items':{'type':'object','properties':{'reportId':{'type':'string'},
                    'sha256':{'type':'string'},'bytes':{'type':'integer'},'overallStatus':{'type':'string'}},'additionalProperties':True}}}}
        variants = {
            'nodeforge_connect':('connected',{'handle':{'type':'string'},'identity':{'type':'object'},'operations':{'type':'integer'},'docsStatus':{'type':'string'}},['handle','identity','operations','unresolvedCalls']),
            'nodeforge_docs_index':('passed',{'manifest':{'type':'object'},'totalPages':{'type':'integer'},'indexScope':{'type':'string'},'nodePagesListed':{'type':'boolean'},'docsManifestSha256':{'type':'string'}},['manifest','totalPages','docsManifestSha256','indexScope','nodePagesListed']),
            'nodeforge_docs_read':('passed',{'bundleId':{'type':'string'},'pageId':{'type':'string'},'text':{'type':'string'},'sha256':{'type':'string'},'docsManifestSha256':{'type':'string'}},['bundleId','pageId','text','sha256','docsManifestSha256']),
            'nodeforge_report_read':('passed',{'reportId':{'type':'string'},'callId':{'type':'string'},'pointer':{'type':'string'},'sha256':{'type':'string'},'value':{},'selectionReturned':{'type':'boolean'},'limitBytes':{'type':'integer'},'navigation':{'type':'object'},
                'text':{'type':'string','description':'For /exportedRequest only: exact complete native serialized Graph, at most 65536 UTF-8 bytes'},
                'payloadSha256':{'type':'string','pattern':'^[0-9a-f]{64}$'},
                'sourceRef':{'type':'object','required':['exportReportPath','exportReportSha256','source','sourceAssetSha256','assetKind','payloadSha256'],
                    'properties':{'exportReportPath':{'type':'string'},'exportReportSha256':{'type':'string','pattern':'^[0-9a-f]{64}$'},
                        'source':{'type':'string'},'sourceAssetSha256':{'type':'string','pattern':'^[0-9a-f]{64}$'},
                        'assetKind':{'enum':['Material','Blueprint']},'payloadSha256':{'type':'string','pattern':'^[0-9a-f]{64}$'}},'additionalProperties':False}},['reportId','callId','pointer','sha256','value'])}
        if name in variants:
            state, properties, required = variants[name]; schema['properties'].update(properties)
            schema['allOf']=[{'if':{'properties':{'status':{'const':state}}},'then':{'required':required}}]
            if name=='nodeforge_connect':
                schema['properties']['candidates']={'type':'array','items':{'type':'object','additionalProperties':True}}
                schema['allOf'].append({'if':{'properties':{'status':{'enum':['notConnected','selectionRequired']}}},
                    'then':{'required':['candidates']}})
            if name=='nodeforge_report_read':
                schema['allOf'].append({'if':{'required':['status','pointer'],'properties':{'status':{'const':'passed'},'pointer':{'const':'/exportedRequest'}}},
                    'then':{'required':['text','payloadSha256','sourceRef']}})
                schema['allOf'].append({'if':{'properties':{'status':{'const':'resultTooLarge'}}},
                    'then':{'required':['reportId','pointer','selectionReturned','limitBytes','navigation'],
                    'properties':{'selectionReturned':{'const':False}}}})
        if name in ('nodeforge_draft_prepareCopy','nodeforge_graphs_preview'):
            payload={'type':'object','additionalProperties':True,'properties':{
                'state':{'type':'string'},'code':{'type':'string'},'cliExitCode':{'type':'integer'},
                'draft':{'type':'object','additionalProperties':True,'properties':{
                    'draftRevision':{'type':'string'},'digest':{'type':'string'},
                    'copyPreparationReport':{'type':'string'},'canRestore':{'type':'boolean'}}},
                'nativeReport':{'type':'object'},'inputSnapshot':{'type':'string'},'inputDigest':{'type':'string'},
                'controlEvidencePath':{'type':'string'},'controlEvidenceSha256':{'type':'string'}}}
            required=['state','code','cliExitCode','draft','nativeReport','inputSnapshot','inputDigest',
                'controlEvidencePath','controlEvidenceSha256']
            if name=='nodeforge_draft_prepareCopy':
                payload['properties'].update(draftApplied={'type':'boolean'},previousDraftRevision={'type':'string'},
                    newDraftRevision={'type':'string'},copyPreparation={'type':'object','additionalProperties':True,
                        'properties':{'preparedInputPath':{'type':'string'},'preparedInputDigest':{'type':'string'},
                            'originalRequestId':{'type':'string'},'originalAssetPath':{'type':'string'},
                            'newRequestId':{'type':'string'},'newAssetPath':{'type':'string'},
                            'targetReserved':{'type':'boolean'},'assetGenerated':{'type':'boolean'}}})
                required+=['draftApplied','previousDraftRevision','newDraftRevision','copyPreparation']
            payload['allOf']=[{'if':{'required':['state'],'properties':{'state':{'const':'completed'}}},
                'then':{'required':required}}]
            schema['properties']['nativeSummary']={'type':'object','additionalProperties':True,
                'description':'原CLI摘要；response.result是原call，其result才是业务结果，unknown保留真实部分写入/提交事实。',
                'properties':{'response':{'type':'object','additionalProperties':True,'properties':{
                    'result':{'type':'object','additionalProperties':True,'properties':{'result':payload}}}}}}
        row['outputSchema'] = schema; rows.append(row)
    contract.update(bundleId=bundle_id,bridgeVersion=bridge_version,tools=rows,
        instructions=metadata['instructions'], docsManifest=manifest_path,
        aiExposure={'default':'deny','approved':[t['operation'] for t in rows if t['operation']],
            'notProjected':[n for n in native if n not in {t['operation'] for t in rows}],
            'source':'Tools/NodeForgeMcp/ToolMetadata.json'})
    return contract

def main():
    old = load(HOME / 'Manifest.json')
    if old.get('publicationStatus') == 'frozen': raise RuntimeError('Published package is immutable')
    REPORT.mkdir(parents=True, exist_ok=True)
    prior = REPORT / 'ManifestBeforeP2.json'
    if not prior.exists(): prior.write_bytes((HOME / 'Manifest.json').read_bytes())
    native, operations = load(CATALOG), load(OPERATIONS)
    contract = load(ROOT / 'Tools/NodeForgeMcp/ToolContract.json')
    delivery = load(ROOT / 'Reports/PluginControlStageTwo/RulesCatalog1/P3/DeliveryManifest.json')
    sources = []
    all_evidence_paths = set()
    for name in VERIFICATIONS:
        p = ROOT / 'Catalog' / (name + '.json'); d = load(p)
        rows = d.get('nodes', d.get('entries', []))
        if 'material' in d: rows = [{**n, 'assetType': 'Material'} for n in d['material']] + [{**n, 'assetType': 'Blueprint'} for n in d['blueprint']]
        source = {'source': ref(p), 'originalBuild': d.get('build', 'notExplicitInVerification'),
            'originalBinarySha256': d.get('selectedBinarySha256', d.get('binarySha256')),
            'originalCatalogDigest': d.get('catalogDigest'), 'limitations': d.get('limitations', []),
            'laterStatusCorrections': d.get('laterStatusCorrections', []), 'rows': rows,
            'combinationContext': d.get('cases', d.get('representativeCases', d.get('reports', {})))}
        sources.append(source); all_evidence_paths.update(paths_in(d))
    continuity = []
    for p, expected in delivery['sourceFiles'].items():
        actual = digest(ROOT / p)
        if actual != expected: raise RuntimeError('Build122 source conflict: ' + p)
        staged = ROOT / delivery['stage'] / Path(p).relative_to('Plugins/NodeForge') if Path(p).is_relative_to('Plugins/NodeForge') else ROOT / p
        if not staged.is_file() or digest(staged) != expected: raise RuntimeError('Stage source conflict: ' + p)
        continuity.append({'path': p, 'stagePath': staged.relative_to(ROOT).as_posix(), 'sha256': actual,
                           'meaning': 'Current source equals A Build122 source/stage; no full UI retest implied'})
    evidence = {'format': 'NodeForgeEvidenceIndex1', 'bundleId': contract['bundleId'],
        'historicalEvidenceOnly': True, 'catalogStateUnmodified': True,
        'meaning': 'Exact native identities and representative configurations; original version/layers retained. No universal per-node or Build122 runtime pass.',
        'sources': sources, 'referencedReports': [public_evidence(p) for p in sorted(all_evidence_paths)],
        'build122SourceContinuity': continuity, 'nodes': []}
    pages = []
    def page(page_id, title, text, filename=None, kind='guide', topics=None):
        path = HOME / (filename or page_id + '.md')
        if page_id in ('Readme', 'Installation', 'ResultsAndRecovery'):
            text += '\n\n仅同版资料更新可由固定配置docsManifestSha256批准后在同一连接刷新；业务/执行绑定、工程或会话变化仍须显式MCP服务重连。刷新核版本目录、SHA、唯一pageId和字节界限，保留原handle/call/report、unknown/pending及lease，不重连重发。返回docsManifestSha256可核实际资料。代码/SDK/工具定义更新仍须真实载入新代码，不以重启整个Codex为默认步骤；磁盘更新不冒充客户端加载成功。'
        save(path, '# ' + title + '\n\n' + text.strip() + '\n')
        item = {'pageId': page_id, 'title': title, 'kind': kind, 'topics': topics or [], **ref(path)}
        if item['bytes'] > 65536: raise RuntimeError('Oversized page: ' + page_id)
        pages.append(item)
        return item
    page('Readme', 'NodeForge Build122 · MCP1 使用手册', '''
本包对应准确 Build122 stage 与桥 0.1.0。P2 是完整候选使用资料；P3 真实 Codex/UE 业务与 P4 冻结仍须各自证据。根安装和 RC1 未升级。

先读 [快速开始](QuickStart.md)、[安装与前置](Installation.md)、[全部业务入口](FeatureMap.md)，再按任务读 [图协议](GraphAuthoring.md)、[材质](Material.md)、[蓝图](Blueprint.md)、[实例](MaterialInstance.md)、[API](ApiReference.md)。

[UI 指南](UiGuide.md)、[结果与恢复](ResultsAndRecovery.md)、[分层验收](Validation.md)、[扩展与迁移](ExtensionAndMigration.md)、[来源与许可](SourcesAndLicenses.md)保留真实边界。

节点逐项资料由实际目录生成：[材质索引](MaterialNodeIndex.md)、[蓝图索引](BlueprintNodeIndex.md)。没有手写第二套节点允许表，也没有每节点 MCP 工具。机器资料：[Manifest](Manifest.json)、[目录](Catalog.json)、[操作](Operations.json)、[证据](EvidenceIndex.json)、[文档索引](DocumentationIndex.json)、[覆盖表](Coverage.json)。完整目录与证据索引可以大于单次 MCP 读取限额，AI 应读索引和节点页，活动目录按受控报告 pointer 取得。

发现、能创建目录实例、实验支持、具体组合编译/保存/重开、运行、画面、打包分别解释；完整手册不等于全部组合通过。原节点目录全部保持 experimental；每页关联历史证据原版本，不把旧报告改成 Build122 实测。
''')
    page('Installation', '安装、版本身份与本地前置', f'''
环境是 Windows x64、UE `{native['engine']}`、适配器 `{native['adapter']}`；Graph v2，Editor API 1.0/profile nodeforge.editor.v1。插件显示的 0.1.0-prototype 不足以识别交付，必须核 Manifest 的 stage、实际 DLL SHA、完整 catalogDigest 和工具/资料指纹。

stage：`{old['stage']}`；DLL SHA：`{old['binarySha256']}`。默认登记工程 `{old['project']}`（NFCUiApiLab，11 字符）；不复制插件、不逐用例新建工程、不替换运行 DLL。历史工程不自动删除。根/RC1 属于其它版本，不能套用本包配置。

本阶段 Python 3.12 专用环境 `Tools/NodeForgeMcp/Environment`，官方 SDK mcp 2.3.0。依赖锁与许可见 [来源](SourcesAndLicenses.md)；无需 OpenAI API key。MCP 只用 stdio，不开放公网或 HTTP/OAuth。启动命令：

```text
{ROOT.as_posix()}/Tools/NodeForgeMcp/Environment/Scripts/python.exe {ROOT.as_posix()}/Scripts/NodeForgeMcp.py --config <本地操作者提供的绝对配置路径>
```

操作者先正常打开已登记工程，核独占、PID/创建时间、实际加载模块/SHA，再通过现有注册机制启用控制会话，取得当前公开 SessionPublic 路径。配置精确字段见 [McpProtocol](../../../../Contracts/McpProtocol.md)；sessionPublic、reportRoot 必须同会话，bridgeRunRoot 是新独有目录；内部 Session.json 不交给 AI，不猜历史 PID。

Codex 与其它客户端均按 stdio 配置 command=上述 python、args=[上述脚本,"--config",准确配置]、cwd={ROOT.as_posix()}。Codex 支持项目 .codex/config.toml，但 P3 实际验证配方和协商版本须以本阶段报告为准；P0 只完成 Codex 工具发现，不能声称业务通过。客户端超时须能容纳原业务截止，不自动取消或重发。桥不自动启动/停止 UE，不封装 Verify 调度。

配置任一指纹、工程归属或会话不匹配即停止。每个 GT 写操作先核 revision/busy 和新 callId；未知结果先查询原调用。该包不授权生产工程、升级安装、自动重置或新会话委派。
''')
    page('QuickStart', 'AI 快速开始：发现到新资产', '''
1. 调 `nodeforge_docs_index {}`，读取 Readme、Installation、GraphAuthoring、ResultsAndRecovery。确认同版 bundle、stage/DLL/协议/目录身份。文档能脱离 UE 读取；业务需当前会话。
2. 调 `nodeforge_connect {}` 得到 opaque handle；再调 operations_describe、editor_getState、draft_get。所有业务提供 `handle`、`arguments:{}`；以真实描述/状态为准，不能猜字段。
3. 先从 draft_get 取得当前字符串 draftRevision；目录和规则查询也走GT、产生报告，均需显式新 callId/expectedDraftRevision。调 `nodeforge_catalog_get {handle,arguments:{},callId:"首次分配的目录调用ID",expectedDraftRevision:"当前修订"}` 和两族 `nodeforge_rules_query {handle,arguments:{assetKind:"Material"或"Blueprint",query:""或有限精确查询词},callId:"首次分配的规则调用ID",expectedDraftRevision:"当前修订"}`。query 大小写无关子串 OR 搜索，不是生成配方。取得原报告 ID/完整 catalogDigest；按 JSON pointer 阅读实际节点和引脚，例如 `/catalog/material/nodes/0`。先读对应节点页，但活动报告才说明当前实例。
4. 台账首次分配短批次、新 assetPath/requestId；固定样例见 [JSON 例子](Examples.md)。只改明确身份和来自真实查询的 catalogDigest，原图语义及原文件保留。不能把示例固定名字再次提交。
5. 用 draft_get 的准确字符串 draftRevision 调 draft_replace（arguments.text=完整 UTF-8 JSON、显式新 callId、expectedDraftRevision）。或用 draft_loadFile 指定已准入项目输入文件。载入/替换只改草稿，不生成；读取新 revision，不将 JSON 图请求 ID 当 RPC callId。
6. 同修订调 graphs_preflight（arguments:{}），读原结果/报告。预检通过不预留资产、不编译保存。输入改变后重新读取状态；busy 或 staleRevision 不能自动改 revision 重试。
7. 明确对预检的同一草稿调 graphs_generate，使用另一新 callId 和准确 revision。检查 CLI/call 原状态、nativeSummary/checks/资产路径与输入 SHA，读取原 reportId。生成保存成功通常 waitingPersistence，不能改写成最终资产通过。
8. 调 calls_get（arguments.callId=原生成 callId），读取本会话 reportId 的 `/initialReadback`、`/finalReadback`、`/publishedReadback` 和 `/checks`，与原固定输入及独立期望比较；报告过大用 pointer，不截断。
9. 操作者在自有编辑器正常退出、工程独占后，用原正式 CLI verifySaved 在新 PID、同 stage、原输入和原生成收据核验。MCP 本期不调度进程。运行、材质画面与打包分别未由此验证。

精确工具/schema 见 [API](ApiReference.md)。任何 pending/unknown、部分写入或证据冲突先停下一次 GT，只有限查询原 call/CLI run/report；禁止换 ID 重复生成。完整结果大于界限明确 resultTooLarge，并返回可导航信息。
''')
    features = [
        ('发现操作、状态、草稿、调用', 'UI 状态', 'operations.describe/editor.getState/draft.get/calls.get', '11 业务投影内', 'A/S 正式证据；MCP P3 待实测', '无资产写入'),
        ('载入、替换、恢复草稿', '载入文件/粘贴/恢复', 'draft.loadFile/replace/restore', '正式开放', '共享服务；P1 桥检查；P3 待实测', '修改草稿、修订；不自动生成'),
        ('规则与目录查询', '两族查询/目录', 'rules.query/catalog.get；CLI getGraphCatalog', '正式开放', 'A Build122 两 UI 查询有限实测', '报告只读；Material/Blueprint query，不含实例参数查询'),
        ('预检与生成 Material/Actor Blueprint', '检查/生成新资产', 'graphs.preflight/generate；CLI preflightGraph/generateGraph', '正式开放', 'A/S 有限完成；本期只补 MCP 双图', '新资产 create-only；生成后待新 PID Verify'),
        ('普通 MaterialInstanceConstant', 'Graph 输入生成', '通用 graphs 管线接受同一 v2；无独立实例查询工具', '通用图入口可达；本期实例 MCP 实测 notRun', '定点核心/共享调用静态核查＋历史实例报告', '新实例；先取得已保存干净父材质参数/直接文件 SHA'),
        ('参数与材质函数签名读取', '检查一个选中资源', '内部 MaterialParameters/MaterialFunction', '未开放独立正式业务 API/MCP', '原版本历史只读证据；122 UI 全量重测 notRun', '普通材质/函数；源保护，真实签名/GUID/SHA'),
        ('普通材质/Actor 图导出', '导出选中图到草稿', '内部 MaterialGraphExport/BlueprintGraphExport', '未开放正式导出 API/MCP', '原版本导出/复制重开证据；122 UI 全量重测 notRun', '只读源、新目标/request、限定完整结构，不静默丢节点'),
        ('准备新副本', '准备新副本/恢复', '内部 PrepareNewCopy', '未开放正式 API/MCP', 'Build78 实际材质按钮/新 PID Verify，蓝图后台有限证据', '只更改两身份；不生成/预留；保留父目录'),
        ('差异预览', '预览当前完整草稿', '内部 GraphPreview', '未开放正式 API/MCP', '历史有限证据；新122 UI 未重复取证', '同 DLL 导出基线＋源 SHA/干净状态，新草稿关联；只读'),
        ('布局与重排副本', '生成时布局；按上次成功输入重排新副本', '共享布局；内部 RelayoutCopy', '生成布局复用；无独立重排 MCP', '历史布局/widget/保存重开有限证据', '显式锚点保持；尺寸 estimate；重排不含后来手工修改'),
        ('独立保存核验', '结果显示待独立重开', '原正式 CLI verifySaved', '未封装进程调度工具', 'M/A 各自新 PID Verify 可靠证据；P3 补当前新资产', '原输入/收据/SHA/同 DLL，新 PID；不重 Import'),
        ('定位、打开资产/完整报告、复制', '结果按钮及剪贴板', 'UI 服务/有界结果；MCP report_read 是受控报告读', '纯 UI 视图操作未 API 化', 'S/夜间各限定证据；本期不剪贴板/文件框大测', '不关闭无关窗口；原报告 ≤1MiB，MCP 选择 ≤32KiB'),
        ('版本资料、来源和许可', '本地资料', 'MCP docs_index/docs_read', '正式桥功能', 'P2 静态/SDK 检查；实际 Codex P3 待', 'manifest 指定 SHA/bytes/pageId，有界非秘密内容'),
        ('运行、画面、地图/布局探针、打包试用', '历史辅助/试用', '内部测试模式/原报告', '非 AI 任意 NFMode 工具', '历史各原版本证据；本期 runtime/visual/package notRun', '生成/Verify 不继承这些结论；RC1 冻结'),
    ]
    page('FeatureMap', '全部现有业务与开放层级', '''
“全部”覆盖当前插件现有业务和限制，不是全 UE API。下面 UI/后台存在由 Build122 源码及 stage SHA 对照核实；历史使用证据保留原版本，不等于 Build122 各按钮全部重新实测。

| 业务 | UI | 正式入口/内部核心 | MCP | 证据口径 | 副作用/边界 |
| --- | --- | --- | --- | --- | --- |
''' + '\n'.join('| ' + ' | '.join(row) + ' |' for row in features) + '''

原正式 CLI 四操作和 Editor 13 方法继续复用，MCP 开放 11 用户业务＋4 本地辅助。session.hello 内部使用，control.disable 归操作者。没有任意方法执行、逐节点工具、旧候选 O01–O44 全实现或第二套构图器。

当前节点与实例详细边界读各对象页。源文件与精确 SHA 对应见 EvidenceIndex.build122SourceContinuity；历史 UI/后台专题链接见 [UI 指南](UiGuide.md)。
''')
    api_text = '''
本页逐项从冻结 ToolContract/原 Operations 生成；11 个业务 inputSchema 的 arguments 直接等于原 argumentsSchema。所有业务含 handle/arguments；GT 另需 callId/规范 uint64 字符串 expectedDraftRevision。下面 schema 完整，不凭表中简写补字段。

原 Editor API 1.0、profile nodeforge.editor.v1，共13方法。内部 session.hello、管理 control.disable 不向 AI 默认开放。业务结果保留原 CLI/response/nativeSummary/reportIdentity；不能把 status=passed 理解为运行或独立重开通过。恢复读 [结果](ResultsAndRecovery.md)。
'''
    for tool in contract['tools']:
        api_text += '\n## ' + tool['name'] + '\n\n原操作：`' + str(tool['operation']) + '`。\n\n'
        if tool.get('nativeDefinition'):
            api_text += fence({k: v for k, v in tool['nativeDefinition'].items() if k != 'argumentsSchema'}) + '\n'
        api_text += fence({'inputSchema': tool['inputSchema'], 'outputSchema': tool['outputSchema']})
    api_text += '\n原 native 限额：\n\n' + fence(operations['limits']) + '''
桥另外：注册报告最多256条；JSON 选择32768字节；单页65536字节；manifest/原报告1MiB。calls 原128条保留与桥256报告是两个不同界限。读报告需要本会话 reportId，不接受任意路径。stdout 只传 SDK MCP；诊断不泄露凭据。
'''
    page('ApiReference', '正式 MCP 工具与精确 schema', api_text)
    page('GraphAuthoring', 'JSON 构图、路径与身份', '''
NodeForgeGraph v2 根必须恰有 format、version、requestId、assetType、assetPath、catalogDigest、nodes、edges、settings 九字段。format="NodeForgeGraph"、version=2；catalogDigest 来自当前实际目录内容摘要，不能用目录文件 SHA 替代。v1 旧固定样例保持兼容子集，不隐式迁移。

完整无 BOM UTF-8 1–65536 字节、深度≤32；拒绝重复键、NUL、注释、尾逗号、未知字段。节点必需 id/class/properties，可有 defaults/position。节点 ID 1–64 ASCII 字母数字，Material 是材质输出保留身份；显式 position 为两整数，abs≤100000。数组顺序不决定执行/构图，不能靠改排序修复连线。

每条边恰为 from/output/to/input 字符串。端口/类/属性/绑定大小写规范、目标输入单连接、拒绝环、自连、隐式转换或替线；默认值与同输入连接互斥。各对象真实签名见目录与节点页，不能猜输出别名。布局在实际构图后统一处理，保留显式锚点；报告 size estimate 不证明所有屏幕无重叠。

目标仅 `/Game/NodeForgeTests/` 下0/1/2级子目录，各段1–16 ASCII字母数字，叶名1–64；无空格/下划线/后缀/点段/扩展名/反斜线。磁盘路径核 ProjectContent/reparse/UE包名与总长≤min(260,平台上限)。测试工具分配叶名/request建议≤32，工程≤20建议≤12，不收紧原公开64上限。

同完整包路径或项目级 requestId 已用即拒绝。不覆盖、不静默加后缀；两个不同目录 Sample01 可共存。新批次由现有 ControlRuns 台账独占、串行首次分配并累计保护；失败后不换批次重试。准备副本仅 UI 明确操作，不预留，后续生成再核冲突。

正式组织目录名 NodeFroge 是用户指定拼写，不代表 UE mount；生产完整路径以后落实。现有 NodeForge 工作区/插件/模块不改名，不能向任意生产目录生成。

公共原契约 [GraphProtocol](../../../../Contracts/GraphProtocol.md)保留历史快照；本页已按 Build122 的子目录规则整理，不能按旧正文“平铺/24节点”解释新版。对象限制仍以当前真实目录与核心为准。
''')
    page('Material', '普通 Material：48 类与组合边界', '''
48 类，全部 experimental/discovered/constructible 目录实例；不代表48类每种配置都编译。最多256节点/1024边、至少1节点/1输出边；是输入上限，未作为最大复杂图性能实测。所有表达式必须到达有效 Material 输出；孤立/无输出支链明确拒绝，不删除或补连线。

Surface、Opaque/Translucent、Unlit/DefaultLit、twoSided boolean；Translucent 需显式 SurfaceForwardShading。输出仅当前模式实际 active 的 BaseColor/EmissiveColor/Metallic/Normal/Opacity/Roughness/Specular/WorldPositionOffset。Unlit 通常写 EmissiveColor；Opaque 不接 Opacity。真实编译继续判断维度、采样器和着色阶段。

output 是规范数字字符串索引，例如 "0"，不能 RGB/00/number。input 等原生实际 GetInputName。properties 仅目录允许字段；省略用记录的原生默认，不插入常量。defaults 只映射唯一原生 numeric OverridingInputProperty，无唯一映射拒绝。参数名必须独立，当前不允许同名共享。

贴图引用仅已存 `/Engine/` 或测试根精确普通 UTexture2D、非虚拟；不导入新资源。缺省 sampler 原生推断并回读，显式不匹配拒绝。TextureObjectParameter 继承的 UV/mip 输入并不被消费，不能照继承显示误连接。

MaterialFunctionCall 要绑定真实已保存干净普通函数，实际 direct file SHA、输入/输出 GUID/signature、原生必需性；未绑定目录空签名不是规则。资源检查 UI/后台取得真实签名；无独立函数查询 MCP，函数默认值/实例/层未适配。任意 Custom HLSL、Material Attributes、未知表达式/函数、任意对象导入都未开放。

Spatial world/object origins 使用精确原生枚举，方向/法线为世界空间，不自动转切线。PixelNormalWS 对 Normal/WPO 不适用；具体着色阶段由实际 shader 判定。颜色/距离/UV等族在同一适配器中组合，无水面专用生成器。

读 [48节点索引](MaterialNodeIndex.md)、对应精确属性/引脚页和历史 EvidenceIndex。该页不继承历史画面验证到新材质，也不保证任意大图/数值范围正确。
''')
    page('Blueprint', 'Actor Blueprint：91 签名、8 类与执行边界', '''
目录91签名/8类：64 CallFunction、1 Event、7 Sequence（outputCount2–8）、1 Branch、SwitchInteger/Name各2种 hasDefault探针、bool/float/string/vector/int/name/rotator 各Get/Set。状态experimental；变量探针 Value 必须换成输入中真实声明名，不把它当固定端口。

普通 Actor、一个 EventGraph；UE默认 SceneRoot/空 ConstructionScript 是脚手架，另列回读。最多128节点/256边/64成员；数组顺序无执行意义。允许无事件的纯数据图，但运行另验。事件仅 Actor.ReceiveBeginPlay最多1个；函数 owner/member 必须在目录且精确绑定，不允许任意 UFunction。

settings 恰为 parentClass=/Script/Engine.Actor、variables 数组（可空），成员 name ASCII字母开头、字母数字、不能大小写折叠重复或继承碰撞。float 为原生 real/double；vector=[x,y,z]；rotator={pitch,yaw,roll} 三角度数且各有限abs≤1e12；int32 整数范围；name 非空且小于NAME_SIZE，用规范 None 表示空身份；string≤4096。编译后读取真实 CDO，不依赖清空的编辑器声明 DefaultValue。

defaults 仅未连接、可写、可见的精确原生输入；bool/real/string/vector/int/name/rotator按类型，不用字符串替代数字/数组替代Rotator，不任意对象/容器。连接仅原生 MAKE；禁止自动转换/替换/环、split/orphan pins。

自身 Actor 函数用隐式 self；SceneComponent 成员只有本图 Actor.K2_GetRootComponent.ReturnValue 直接连 component.self，拒绝外部Actor/组件、任意对象默认/隐式cast/reroute。碰撞/物理/网络或任意调度正确性不由构图成功证明。

Sequence明确 outputCount2–8，端口逐字 Then_0…；历史具体输出数配置的编译/运行不能推广到其它数。SwitchInteger限定startIndex/caseCount1–8/hasDefault、int32溢出保护；SwitchName有限唯一cases/hasDefault。原生name相等大小写无关，但端口显示身份精确，无Default时不匹配结束，不补出口。

不支持自定义父类、任意多图、宏/函数图、自定义事件、timeline/delegate/interface、额外组件、循环、数组/Map/Set/enum、任意结构体或未知函数。图规模上限不等于可生成任何复杂业务；复杂度受这些执行/对象/类型限制。

读 [91签名索引](BlueprintNodeIndex.md)和对应节点/历史组合证据。目录扩展沿相同 CallFunction/反射/Spawner/配置路径，MCP 工具本身无需按函数追加。
''')
    page('MaterialInstance', '普通材质实例与资源依赖', '''
assetType=MaterialInstance，新普通 MaterialInstanceConstant；nodes/edges必须空，settings恰为parent、parentAssetSha256、scalarParameters、vectorParameters、textureParameters。父是已保存、干净普通 Surface Material，非实例父、layer或函数实例。实际父精确路径/直接磁盘SHA保护；生成不修改父。

参数必须来自该父实际 MaterialParameters（检查选中资源 UI/内部后台），只覆盖 global scalar/vector/nonvirtual Texture2D；未知名称/类型、静态switch、层参数、虚拟纹理、不匹配sampler拒绝。override maps 与真实值、父哈希、shader继承、保存/独立重开分别回读，UE setter bool 不能充当本机成功判据。

通用 graphs.preflight/generate 从共享 Execute 管线静态可达实例，没有独立实例 query/MCP资源检查；rules.query只两族。已有 UI/后台实例样例和历史验收保留，本期 P3 只材质/蓝图，实例 MCP 实测 notRun。不要为了本期完整文档新增实例测试批次。

目录内 materialInstance 原文与 [实例示例](Examples.md)说明配置；先由操作者取得实际父参数/sha，缺依赖时停止，不能虚构参数名或把样例父路径当已存在资源。
''' + '\n\n当前实例目录：\n\n' + fence(native['catalog']['materialInstance']))
    page('UiGuide', 'UI 全部业务与实际限制', '''
NodeForge面板输入区是可恢复草稿，载入JSON/整框粘贴只替换，不自动生成。Ctrl+V/右键仍原生光标/选区编辑；每次编辑增加revision，使旧检查/预览失效。检查与生成使用不可变快照/摘要，源文件路径不能代替实际提交字节。恢复取上一稿，不能将旧结果归给新草稿。

“检查输入”只预检；“生成新资产”明确新建、编译、保存，结果等待独立Verify。“准备新副本”用同解析器与原生唯一名建议，仅更改assetPath/requestId，显示新身份，可恢复；不写资产/预留，连续未生成可能同建议名。同名冲突真实显示，旧常驻通用文字已在107移除，不能把提示误当持续冲突。

规则两族查询/目录与正式API复用；大小写无关有限OR子串，空查询返回全族。查询报告与草稿生成分开，不将搜索词自动转整图。

“检查选中资源”仅一个已保存、干净普通Material/MaterialFunction，读取实际参数或动态函数签名及源SHA，不开放实例/layer。失败清旧复制/结果缓存，源不修改。详 [ResourceSelection](../../../ResourceSelection.md)。

“导出选中图”只普通Material或测试根普通Actor Blueprint、支持的完整结构。明确新request/目标，导出结果载入草稿/保存基线；需另点生成，失败清旧基线。Actor须单EventGraph、允许成员/默认脚手架；多图/宏/未知或不可表达属性/pins明确拒绝，不静默删节点。材质实例/MaterialFunction本体不能图导出。详 [MaterialGraphExport](../../../MaterialGraphExport.md)、[BlueprintGraphExport](../../../BlueprintGraphExport.md)。

“差异预览”比较当前完整v2输入与同DLL导出的源，核源干净/字节SHA/基线，节点逻辑ID、属性/连接及位置差异分开报告，只读且不生成。编辑使旧预览失效。详 [GraphPreview](../../../GraphPreview.md)。

“按上次输入重排新副本”读取上次成功生成的不可变输入/摘要，移除position、分配新身份、调用同Import；不会包含此后手工编辑资产，不能取无关当前草稿。已有源/输入保留，独立Verify使用派生输入和对应新报告。尺寸是estimate，锚点冲突保留warning，不等于实际Slate/人工美观全验。详 [GraphLayout](../../../GraphLayout.md)。

结果区显示有界摘要，完整诊断/来源/分层检查看原报告。“定位/打开资产”“打开/复制报告”是视图动作，不提升资产验证层；只读review面板不能生成，关面板不能丢执行归属。MCP报告工具读取相同原生内容但不操作剪贴板/窗口。

上述业务在Build122源码/stage中核实存在，历史使用证据各自原Build保留；A只补两查询UI，S/夜间按钮证据有明确版本，不能说122所有UI完整重测。本期不补剪贴板/文件框/全部UI大测。
''')
    page('ResultsAndRecovery', '结果状态、报告与有界恢复', '''
三个不同身份：MCP协议请求、Editor controlSession/callId、Graph requestId/assetPath。不能互换。桥返回结构化结果并保留原CLI/response、workflow/native状态、原报告 SHA/bytes/reportId/证据目录，不合成业务succeeded。

生成/Import资产：validation/build/initialReadback/compile/finalReadback/publish/save均真实passed、独立persistence未执行时 waitingPersistence。新PID Verify核原保存后才可满足八门槛；runtime/visual/package另列。辅助 GraphPreflight overallStatus=succeeded 只代表预检，GraphRules看operationStatus、Catalog看本操作完成，不套资产八门槛。准备副本 requestOutcome=notBuilt，资产/名称未预留。

GT串行、busy拒绝；expectedDraftRevision准确uint64字符串。staleRevision时先读原草稿/状态，不能自动改修订重发。callId持久记录先于提交：同ID同参数只查calls.get；同ID不同操作/参数/修订拒绝。新报告/输入/原资产不覆盖。控制保留128调用、桥256报告，容量不足停止。

pending/unknown、截止/取消、残留或报表指纹冲突立即停止后续GT。保存原callId、原CLI run、PID/创建身份、input snapshot、原收据/资产与部分写入；有限调calls.get查询原调用。CLI仍运行时不强杀，保留LiveCli。晚到收据只收集同进程原结果，不重发。明确失败仍需residue复核；桥 requiresReview不能通过重新连会话绕过。

只读恢复不能证明没有部分写入；不同会话旧handle/reportId失效，不猜PID/自动重连重放。报告篡改/越界/reparse/凭据文件/版本不符拒绝；原CLI可用于操作者精确状态查询。未知不能换request/asset重新生成，停止说明证据缺口。

报告读需本会话已登记reportId和RFC6901 pointer。原报告≤1MiB，所选≤32768字节；超限resultTooLarge返回keys/arrayLength等导航且selectionReturned=false，不截断。按 `/catalog/material/nodes/0`、`/checks`、`/initialReadback`、`/finalReadback`、`/publishedReadback` 等真实对象逐次读。文档pageId≤128、每页≤65536字节，只manifest原SHA/bytes，不任意路径。

原 [EditorControlProtocol](../../../../Contracts/EditorControlProtocol.md)、[ControlProtocol](../../../../Contracts/ControlProtocol.md)、[ResultContractDraft](../../../../Contracts/ResultContractDraft.md)是不同层契约。ResultContractDraft早期完整phase/取消/幂等原子锁等候选未全实现，不因这份手册宣称已实现；使用实际原生字段和本轮有界策略。
''')
    page('Validation', '验收层级、已有证据与最小复核', '''
本包P2静态覆盖/目录适配证明文档和薄桥读数据，SDK工具检查不等于真实Codex调用或UE运行。P3只一次同工程新身份Material+Blueprint批次，查询/草稿/preflight/generate/call/report及真实结构/编译/保存；随后自有编辑器正常退出，同stage新PID各一次Verify。P4只读复核与冻结，缺任一必需证据不得Goal complete。

| 层 | 完成判据 | 本期口径 |
| --- | --- | --- |
| 资料/SDK/静态 | schema/指纹/覆盖/链接、真实SDK封帧 | P0/P1可靠记录＋P2必要检查 |
| 真实客户端 | 真实Codex调用本桥、协商/发现及结构化业务收据 | P3待实测，不能用SDK冒称 |
| 构造/编译/保存 | 独立原输入对照实际readback；checks及磁盘/收据 | P3两图一次批次 |
| 独立Verify | 同stage、原输入/生成receipt、独占、新PID、语义/身份/字节 | P3两图各一次，不Import |
| 实际运行 | 声明执行/消息/变量/位置真实归属 | 本期notRun；历史逐组合另列 |
| 材质画面 | 真正渲染/图像或像素证据 | 本期notRun；历史原版本另列 |
| 打包/无插件交付 | 真正包和声明环境验证 | 本期notRun；RC1冻结独立 |

M0–M4/AC038、S0–S5/AC039/T023、A/AC040/T024已限定完成，不重新立项。S完整同PID双图实测105，相关核心源码连续性118；118没有重新全实测。A Build122有限两查询/子目录两图/新PID Verify已通过。未改CLI/C++/DLL/UI复用这些可靠证据，不编译UE、不跑139全节点/旧11负例/UI剪贴板/打包。真实缺陷仅补受影响用例并保留失败证据。

EvidenceIndex保留13个Verification来源、原Build/DLL/catalogDigest、精确节点binding或变量/Sequence配置、限定组合/层级和原报告SHA/实际摘要。class-level Switch代表配置记录不能变成每种caseCount都通过；无明确绑定的CallFunction证据不自动关联全部函数。目录状态不改成verified。源文件相等只关联当前组件，不把旧运行/画面升级成122重测。

旧 UE532GraphVerification 存在历史中文乱码；本包保留原SHA和机器字段，标记文字不可依赖，不靠猜文字补齐事实。缺原报告/实际层级未知保持missing/notEstablished，不重测凑完整。
''')
    example_text = '''
下面是原固定样例完整语义模板。requestId/assetPath只作占位，不允许直接复用；首次提交由 ControlRuns 台账分配新批次/两身份，catalogDigest来自当前formal query。原fixture SHA记录见 P0 FixtureManifest。输入原样保留，衍生仅改三字段，不能为绕失败改属性或连接。
'''
    for kind, filename in [('Material', 'StaticColor'), ('Blueprint', 'BlueprintConcat')]:
        d = load(ROOT / f'Examples/GraphV2Samples/{filename}.json')
        d['requestId'] = 'ReplaceWithAllocatedRequest'; d['assetPath'] = '/Game/NodeForgeTests/' + kind + '/B000000/ReplaceWithAllocatedName'
        d['catalogDigest'] = native['catalogDigest']
        example_text += '\n## ' + kind + '\n\n' + fence(d)
    example_text += '''
## MaterialInstance 模板（依赖未填，不是可直接生成样例）

先操作者实际读取已保存干净父 Material 的参数及直接文件 SHA。真实父/参数名称不可虚构；准备如下结构但替换真实数据后才可预检。此例本期MCP实例实测notRun：

```json
{"format":"NodeForgeGraph","version":2,"requestId":"ReplaceWithAllocatedRequest","assetType":"MaterialInstance","assetPath":"/Game/NodeForgeTests/Instance/B000000/ReplaceWithAllocatedName","catalogDigest":"ReplaceWithCurrentCatalogDigest","nodes":[],"edges":[],"settings":{"parent":"ReplaceWithInspectedParentObjectPath","parentAssetSha256":"ReplaceWithActualFileSha256","scalarParameters":{},"vectorParameters":{},"textureParameters":{}}}
```

## 查询与恢复参数

`nodeforge_rules_query` arguments={"assetKind":"Material","query":"ScalarParameter VectorParameter"}；Blueprint可query="Concat PrintString"。外层必须同时提供 handle、新 callId 和当前字符串 expectedDraftRevision；catalog_get 同样需要这些GT字段。空query是全族，结果报告可能需pointer读取。查询不改变草稿，query大小写规则与图精确绑定不同。

未知生成只调用 `nodeforge_calls_get` arguments={"callId":"原生成callId"}，不换ID重发。报告拿到reportId再 `nodeforge_report_read` pointer="/checks" 或真实readback子树。必须将原状态/收据与输入对照，不能把查询成功当资产通过。
'''
    page('Examples', '最小 JSON 样例与查询、恢复', example_text)
    page('ExtensionAndMigration', '节点扩展、缓存与版本同步', '''
单一来源链：真实核心实现/严格配置＋统一Operation定义→同版GraphCatalog/TaskRules/Operations→薄MCP schema投影＋节点/文档生成→必要受影响验收→新版本manifest/差异。UI和API复用同核心，不能在每个节点添加一套MCP代码或生成配方。

当前NodeForgeMcpBridge没有节点名/引脚允许表；从已绑定原生catalog报告返回数量、digest和受控reportId，AI读取真实节点/规则。新增普通CallFunction/Material类沿原反射/Spawner/Specs路径，operation/schema不变时15工具不用新增。新类型/动态签名/业务副作用或契约变化必须先审影响并同步真实操作/规则/错误/例子/文档；MCP不是自动获得全UE支持。

缓存键至少bundle＋engine/adapter＋实际DLL＋catalogDigest＋操作schema/文档manifestSHA。活动查询返回新摘要后旧缓存失效、重新读精确签名；不同session旧handle/report失效，不重放。未来新DLL应新manifest/config；不要把文件SHA、catalogDigest、桥version混用。旧目录的准入按其原子集/revision保留，旧JSON不静默迁移或替换digest。

本期仅两个已有可靠目录快照在相同桥/同ToolContract下检查44/76→48/91数据变化、新条目发现和缓存失效；这是读数据适配证明，不是新增节点在UE编译/运行验证。历史支持证据继续带原版本与具体组合，不能升级目录全部verified。

发布前由 Scripts/BuildNodeForgeUsageDocs.py 从正式导出和Verification生成候选。Manifest/DocumentationIndex列pageId/path/SHA/bytes、Nodes页原目录签名、EvidenceIndex原报告与源连续性。frozen包禁止再生成；下一版本另建目录、差异说明、迁移/兼容和必要受影响例子。版本说明应回答新增什么、AI哪里发现、怎样用、结果怎样判、未验什么。

本期不扩节点、函数参数查询/图导出API、副本/差异/重排API、生产路径、HTTP/并行调度。文档完整不是全API开放，后续有限业务须新授权。详细机制：[NodeExtensionWorkflow](../../../PluginControl/NodeExtensionWorkflow.md)、[版本规范](../../../PluginControl/VersionDocumentationPlan.md)。
''')
    page('SourcesAndLicenses', '固定来源、采用代码与许可', '''
MCP采用官方 [Python SDK v2.3.0](https://github.com/modelcontextprotocol/python-sdk/releases/tag/v2.3.0)，固定提交2118f14f8a19bc158d8a1cf90af58d85d187f849、MIT。本地完整SDK/29依赖版本、wheel SHA、许可证文件SHA见 [DependencyManifest](../../../../Tools/NodeForgeMcp/DependencyManifest.json)、[Requirements.lock](../../../../Tools/NodeForgeMcp/Requirements.lock)、[MCP通知](../../../../Tools/NodeForgeMcp/ThirdPartyNotices.md)。不将研究候选列为实际采用，未声称完整供应链安全审计。

协议封帧/协商由SDK负责：[MCP规范](https://modelcontextprotocol.io/specification/2026-07-28)、[stdio](https://modelcontextprotocol.io/specification/2026-07-28/basic/transports)、[tools](https://modelcontextprotocol.io/specification/2026-07-28/server/tools)。SDK实际probe协商2026-07-28；Codex协商修订只按实际观察记录解释；ProtocolSeen保留首次原时点，不能代表后续实例，不能据标准网页猜客户端。

Codex实际0.162.0-alpha.17.2：[官方MCP说明](https://developers.openai.com/codex/mcp)、[官方app-server](https://developers.openai.com/codex/app-server)；P0定点核本机生成schema/实际发现，不调用计费API，不创建额外模型对话。通用stdio配方不绑定Codex账号。

UE调用已安装官方5.3.2原生接口，不复制引擎实现：[Material Expressions](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-material-expressions-reference?application_version=5.3)、[Material Functions](https://dev.epicgames.com/documentation/en-us/unreal-engine/material-functions-in-unreal-engine?application_version=5.3)。具体pin/原生类型以内置真实目录和本机头文件为准，论坛/官网说明不替代本工程实测。

原插件保留固定MIT来源：ChiR24 591d3b0ea3c46a2a1926dbcafc225afecb6a75b3（精确连接窄提取）、ZiggyMar d1a85396d9c62328521256005fe7c420725651cc（函数过滤/变量绑定/原生工厂）、Autonomix 47a46ae8cc3f00230edceeb33f0d4f9d80cb185b（布局骨架）。实际stage全文：[ThirdPartyNotices](../../../../Reports/PluginControlStageTwo/RulesCatalog1/Build122/PluginStage/NodeForge/ThirdPartyNotices.md)、[Sources](../../../../References/SourceIndex.md)。GraphDeck/未明Epic材质文件/MPL等未复制，不能因GitHub有代码就越许可搬入。

原CLI/Win32安全传输/核心/UI未改变；桥只复用原正式CLI和现有有界安全读取，SDK只管MCP。新增自写范围是窄桥、版本绑定、持久调用门槛、有界报告/文档与生成索引；没有另造图引擎或Win32客户端。
''')
    for kind, field in [('Material', 'material'), ('Blueprint', 'blueprint')]:
        index_lines = []
        for i, node in enumerate(native['catalog'][field]['nodes'], 1):
            node_id = f'{kind}Node{i:03d}'
            ident = identity(kind, node)
            linked = []
            for source in sources:
                for row in source['rows']:
                    if matches(kind, node, row):
                        linked.append({'verificationSource': source['source']['path'],
                            'originalBuild': source['originalBuild'], 'originalBinarySha256': source['originalBinarySha256'],
                            'originalCatalogDigest': source['originalCatalogDigest'],
                            'matchKind': 'representativeClassConfiguration' if 'K2Node_Switch' in node['class'] else 'nativeIdentity',
                            'record': row})
            record = {'pageId': node_id, 'identity': ident, 'catalogPointer': f'/catalog/{field}/nodes/{i-1}',
                'catalogSupportState': node['supportState'], 'historicalRecords': linked,
                'missingHistoricalIdentityEvidence': not bool(linked),
                'allConfigurationsVerified': False, 'build122RuntimeRetested': False}
            evidence['nodes'].append(record)
            title = node['class'].split('.')[-1]
            if row_binding(node): title += ' ' + row_binding(node).get('member', '')
            if node.get('variableType'): title += ' ' + node['variableType']
            if ident.get('configuration'): title += ' ' + json.dumps(ident['configuration'], ensure_ascii=False, separators=(',', ':'))
            body = f'目录位置 `{record["catalogPointer"]}`；目录状态 experimental，实例能发现/构造，不据此推导编译、运行或画面。精确身份：\n\n' + fence(ident)
            body += '\n完整当前节点签名、默认/允许属性和引脚：\n\n' + fence(node)
            body += '\n变量节点 Value 是真实声明名占位；动态函数/Sequence/Switch必须按对象页规则和实际配置重新核签名，不能将探针参数强制为唯一输入。\n\n'
            body += f'历史对应记录 {len(linked)} 条。下列来源只关联上述原生身份/明确配置及原组合；完整层级、限制、原报告SHA和实际摘要见 EvidenceIndex。没有条目时保持notEstablished，不补成通过。\n\n'
            for rec in linked:
                s = next(s for s in sources if s['source']['path'] == rec['verificationSource'])
                body += '- `' + rec['verificationSource'] + '`，原Build=`' + str(rec['originalBuild']) + '`，原DLL=`' + str(rec['originalBinarySha256']) + '`，关联=`' + rec['matchKind'] + '`；记录的support/verification=`' + str(rec['record'].get('supportState', rec['record'].get('verification', 'seeOriginalLayerFields'))) + '`。\n'
                body += '\n' + fence(rec['record']) + '\n'
            body += f'\n返回 [{kind}索引](../{kind}NodeIndex.md)及[验证层](../Validation.md)。本页由原目录生成，不手工维护节点允许表。\n'
            page(node_id, title, body, 'Nodes/' + node_id + '.md', 'node', [kind, node['class'], node.get('family', '')])
            index_lines.append(f'| {node_id} | [{title}](Nodes/{node_id}.md) | {node.get("family", "")} | experimental | {len(linked)} |')
        page(kind + 'NodeIndex', kind + ' 精确节点索引', f'共 {len(index_lines)} 条当前目录签名。目录摘要 `{native["catalogDigest"]}`。历史记录数量不是测试成功次数；逐项看原身份/配置/层级/Build，目录状态全部保持experimental。\n\n| pageId | 原生节点/绑定或配置 | family | 目录状态 | 历史关联记录 |\n| --- | --- | --- | --- | --- |\n' + '\n'.join(index_lines), topics=[kind])
    save(HOME / 'Catalog.json', dump(native))
    save(HOME / 'Operations.json', dump(operations))
    save(HOME / 'EvidenceIndex.json', dump(evidence))
    coverage = {'format': 'NodeForgeUsageCoverage1', 'features': [dict(zip(
        ['business', 'ui', 'entry', 'mcp', 'evidenceMeaning', 'boundary'], row)) for row in features],
        'requiredGuideIds': ['Readme', 'Installation', 'QuickStart', 'FeatureMap', 'ApiReference', 'GraphAuthoring',
            'Material', 'Blueprint', 'MaterialInstance', 'UiGuide', 'ResultsAndRecovery', 'Validation', 'Examples',
            'ExtensionAndMigration', 'SourcesAndLicenses', 'MaterialNodeIndex', 'BlueprintNodeIndex'],
        'nativeOperations': [x['name'] for x in operations['operations']],
        'businessTools': [x['name'] for x in contract['tools'] if x['operation']],
        'counts': {k: len(native['catalog'][k]['nodes']) for k in ('material', 'blueprint')},
        'evidenceSourceCount': len(sources), 'noFullNodeRetest': True, 'ueTestsExecutedByGenerator': False}
    save(HOME / 'Coverage.json', dump(coverage))
    index = {'format': 'NodeForgeDocumentationIndex1', 'bundleId': old['bundleId'], 'pages': pages}
    save(HOME / 'DocumentationIndex.json', dump(index))
    manifest = {k: v for k, v in old.items() if k not in ('pages', 'publicationStatus', 'fullUsagePackageComplete')}
    manifest.update({'format': 'NodeForgeUsageManifest1', 'engine': native['engine'], 'adapter': native['adapter'],
        'graphProtocolVersion': 2, 'catalogDigest': native['catalogDigest'],
        'publicationStatus': 'candidateCompleteP2', 'fullUsagePackageComplete': True,
        'actualCodexBusinessAcceptance': 'notRun; P3', 'runtime': 'notRunThisStage', 'visual': 'notRunThisStage', 'package': 'notRunThisStage',
        'pages': pages, 'counts': coverage['counts'],
        'artifacts': [ref(HOME / f) for f in ('Catalog.json', 'Operations.json', 'EvidenceIndex.json', 'Coverage.json', 'DocumentationIndex.json')],
        'bindings': [ref(ROOT / p) for p in ('Tools/NodeForgeMcp/ToolContract.json', 'Tools/NodeForgeMcp/DependencyManifest.json',
            'Tools/NodeForgeMcp/Requirements.lock', 'Tools/NodeForgeMcp/ThirdPartyNotices.md', 'Scripts/NodeForgeMcp.py',
            'Scripts/NodeForgeMcpBridge.py', 'Scripts/BuildNodeForgeUsageDocs.py')],
        'catalogSource': ref(CATALOG), 'operationsSource': ref(OPERATIONS),
        'historicalEvidenceRetainsOriginalVersion': True, 'sourceContinuityDoesNotMeanUiRuntimeRetest': True})
    save(HOME / 'Manifest.json', dump(manifest))
    save(REPORT / 'Generation.json', dump({'stepId': 'P2', 'generator': ref(Path(__file__)),
        'manifest': ref(HOME / 'Manifest.json'), 'guideCount': 17, 'nodePageCount': len(evidence['nodes']),
        'verificationSources': len(sources), 'referencedReports': len(evidence['referencedReports']),
        'missingReports': [r['path'] for r in evidence['referencedReports'] if 'availability' in r],
        'sourceContinuityCount': len(continuity), 'ueStarted': False, 'projectAllocated': False,
        'canAdvance': False, 'next': 'Review full package coverage/links/SDK data adaptation before P3'}))
    print(json.dumps({'pages': len(pages), 'nodes': len(evidence['nodes']), 'references': len(evidence['referencedReports']),
                      'missingReports': sum('availability' in r for r in evidence['referencedReports'])}))

def node_key(kind, row):
    return json.dumps(identity(kind,row),sort_keys=True,ensure_ascii=False,separators=(',',':'))

def catalog_diff(previous, current):
    def rows(c):
        return {node_key(kind,n):n for kind,field in [('Material','material'),('Blueprint','blueprint')]
            for n in c['catalog'][field]['nodes']}
    old,new=rows(previous),rows(current)
    return {'added':[json.loads(k) for k in sorted(new.keys()-old.keys())],
        'removed':[json.loads(k) for k in sorted(old.keys()-new.keys())],
        'changed':[json.loads(k) for k in sorted(new.keys()&old.keys()) if old[k]!=new[k]],
        'previousDigest':previous['catalogDigest'],'currentDigest':current['catalogDigest'],
        'matching':'exact native identity, never ordinal pageId','meaning':'data/documentation difference, not new UE verification'}

def ordinary_node_knowledge(kind, node):
    """Derive ordinary-node knowledge from the qualified production catalog, never admit nodes here."""
    family = node.get('family')
    ordinary = ((kind == 'Blueprint' and family == 'OrdinaryPureComputation'
                 and node.get('minimumBlueprintRevision') == 10)
                or (kind == 'Material' and family == 'OrdinarySurfaceMath'
                    and node.get('minimumMaterialRevision') == 6))
    if not ordinary or node.get('constructible') is not True or node.get('supportState') != 'experimental':
        return None
    binding = node.get('properties', {})
    if kind == 'Blueprint':
        if not isinstance(node.get('pins'), list) or not binding.get('owner') or not binding.get('member'):
            raise ValueError('Qualified ordinary function lacks exact binding/signature')
        label = binding['member']
        description = node.get('nativeDescription', '').strip()
        purpose = '原生纯计算：' + label
        preconditions = ('读取本页准确 owner/member 和 pins，使用本包完整 catalogDigest。'
                         '输入、输出及 float/double 以原生签名为准，不能隐式转换；函数本身不推动执行流，'
                         '结果需接入消费它的成员赋值、打印或其它计算。')
        limits = ('限两个受控原生库的已准入静态纯函数；不扩对象、数组、int64、FText、随机状态、隐藏上下文或 mutable ref。'
                  '只给可写、未连接的可见输入设置准确类型默认值。原生用途：' + (description or label)
                  + '；原生分类：' + node.get('nativeCategory', '未声明') + '。目录构造不等于所有组合运行通过。')
        fragment = {'id': 'ExampleNode', 'class': node['class'], 'properties': dict(binding)}
    else:
        if not isinstance(node.get('inputs'), list) or not node.get('outputs') or not isinstance(binding, dict):
            raise ValueError('Qualified ordinary expression lacks native ports/properties')
        label = node['class'].split('MaterialExpression', 1)[-1]
        purpose = '材质数学：' + label
        preconditions = ('普通 Surface 材质；按本页原生 inputs 名和 outputs 数字索引接线。'
                         '必需输入要连接，每个表达式必须到达活动材质输出，未知配置不能省略成另一语义。')
        limits = ('只可写本页 properties 中的标量/bool，未声明字段保留原生默认；参数范围与引脚 defaultProperty '
                  '按原生目录和协议，Period 必须大于零。不提供资源/GUID/动态端口适配；Shader 编译按实际组合，'
                  '材质画面和运行另列，不能从构造推断。')
        fragment = {'id': 'ExampleNode', 'class': node['class'], 'properties': {}}
    return {'identity': identity(kind, node), 'purpose': purpose, 'preconditions': preconditions,
            'limits': limits, 'exampleFragment': fragment,
            'exampleMeaning': '这是准确单节点声明片段，使用原生默认；不是完整可提交图。完整配方见 AiNodeBulk。'
                              '先按真实目录补必需连接，再用实际分配的 requestId/assetPath 和本版完整摘要预检。',
            'source': 'Plugins/NodeForge/Source/NodeForgeEditor/Private/NodeForgeNodeDefinitions.cpp'}


def closed_group_schema(definitions, group):
    """Project the registered native field facts; do not maintain another allowlist."""
    fields=definitions['fieldGroups'][group]
    properties={f['name']:({} if f['type']=='value' else {'type':f['type']}) for f in fields}
    for f in fields:
        if f.get('enum'):properties[f['name']]['enum']=f['enum']
    return {'type':'object','additionalProperties':False,
            'required':[f['name'] for f in fields if f['required']],
            'properties':properties}

def graph_request_schema(catalog_report):
    """Generate the current catalog's two wire shapes from native shared facts.

    This schema describes syntax/values. Native resource, pin, provenance and
    graph legality checks remain the existing product codec/core's responsibility.
    """
    import copy
    catalog=catalog_report['catalog'];d=catalog['assetDefinitions']
    group=lambda name:closed_group_schema(d,name)
    empty={'type':'object','additionalProperties':False}
    bindings={r['class']:r['kind'] for r in d['nodeBindings']}
    member_schemas={r['name']:r['valueSchema'] for r in d['memberTypes']}
    def variables(version):
        choices=[]
        for name,value in member_schemas.items():
            row=group('VariableV3' if version==3 else 'VariableV2')
            row['properties']['name']={'type':'string','pattern':'^[A-Za-z][A-Za-z0-9]{0,63}$'}
            row['properties']['default']=copy.deepcopy(value)
            if 'memberAttributesSchema' in d: row['properties']['attributes']=copy.deepcopy(d['memberAttributesSchema'])
            if version==3:
                typ=group('ValueType');typ['properties'].update(kind={'const':'Value'},name={'const':name})
                row['properties']['type']=typ
            else:row['properties']['type']={'const':name}
            choices.append(row)
        return {'type':'array','maxItems':d['maxVariables'],'items':{'oneOf':choices}}
    def node_schema(kind,version):
        variants=[];seen=set()
        for node in catalog['material' if kind=='Material' else 'blueprint']['nodes']:
            cls=node['class']
            if cls in seen:continue
            seen.add(cls);schema=group('NodeV3' if version==3 else 'NodeV2');p=schema['properties']
            p['id']={'type':'string','pattern':'^[A-Za-z0-9]{1,64}$'};p['class']={'const':cls}
            if kind=='Material':
                defaults={i['name']:copy.deepcopy(node['properties'][i['defaultProperty']]['valueSchema']) for i in node.get('inputs',[]) if i.get('defaultProperty')}
                p['defaults']={'type':'object','additionalProperties':False,'properties':defaults}
            else:p['defaults']={'type':'object','additionalProperties':copy.deepcopy(d['pinDefaultSchema'])}
            limit=d['materialPositionLimit' if kind=='Material' else 'blueprintPositionLimit']
            p['position']={'type':'array','minItems':2,'maxItems':2,'items':{'type':'integer','minimum':-limit,'maximum':limit}}
            bound=bindings.get(cls,'None')
            if bound!='None':
                binding_group={'Event':'FunctionBinding','Function':'FunctionBinding','Member':'MemberBinding','MaterialFunction':'MaterialFunctionBinding'}[bound]
                if version==3:
                    p['binding']=group(binding_group);p['binding']['properties']['kind']={'const':bound};schema['required'].append('binding');p['properties']=empty
                else:p['properties']=group({'Event':'FunctionPropertiesV2','Function':'FunctionPropertiesV2','Member':'MemberPropertiesV2','MaterialFunction':'MaterialFunctionPropertiesV2'}[bound])
            else:
                p.pop('binding',None)
                if kind=='Blueprint':p['properties']=copy.deepcopy(d['nodeConfigurationSchemas'][cls])
                else:
                    properties={k:copy.deepcopy(v['valueSchema']) for k,v in node.get('properties',{}).items()}
                    p['properties']={'type':'object','additionalProperties':False,'properties':properties}
            variants.append(schema)
        return {'oneOf':variants}
    def family(kind,version,a):
        root=group('RootV3' if version==3 else 'RootV2');p=root['properties']
        p.update(format={'const':'NodeForgeGraph'},version={'const':version},catalogDigest={'const':catalog_report['catalogDigest']},requestId={'type':'string','pattern':'^[A-Za-z0-9]{1,64}$'})
        if kind=='Material':settings=copy.deepcopy(d['materialSettingsSchema'])
        elif kind=='Blueprint':
            settings=group('BlueprintSettingsV2');settings['properties'].update(parentClass={'const':'/Script/Engine.Actor'},variables=variables(version))
        else:
            settings=group('InstanceSettingsV2')
            settings['properties']['parentAssetSha256']={'type':'string','pattern':'^[0-9a-f]{64}$'}
            settings['properties']['scalarParameters']={'type':'object','additionalProperties':copy.deepcopy(d['instanceScalarSchema'])}
            settings['properties']['vectorParameters']={'type':'object','additionalProperties':copy.deepcopy(d['instanceVectorSchema'])}
            settings['properties']['textureParameters']={'type':'object','additionalProperties':{'type':'string'}}
        nodes={'type':'array','minItems':a['minimumNodes'],'maxItems':a['maximumNodes']}
        edges={'type':'array','minItems':a['minimumEdges'],'maxItems':a['maximumEdges'],'items':group('Edge')}
        if a['graphCount']:nodes['items']=node_schema(kind,version)
        if version==2:
            p.update(assetType={'const':kind},settings=settings,nodes=nodes,edges=edges)
        else:
            p['target']=group('Target');asset=group({'Material':'MaterialAsset','Blueprint':'BlueprintAsset','MaterialInstance':'InstanceAsset'}[kind]);asset['properties']['type']={'const':kind}
            if kind=='Material':asset['properties']['settings']=settings
            elif kind=='Blueprint':
                parent=group('BlueprintParent');parent['properties']['class']={'const':'/Script/Engine.Actor'}
                members=group('Members');members['properties']['variables']=variables(version);asset['properties'].update(parent=parent,members=members)
            else:
                parent=group('InstanceParent');parent['properties']['assetSha256']=settings['properties']['parentAssetSha256']
                parameters=group('Parameters')
                for name,old in [('scalar','scalarParameters'),('vector','vectorParameters'),('texture','textureParameters')]:parameters['properties'][name]=settings['properties'][old]
                asset['properties'].update(parent=parent,parameters=parameters)
            p['asset']=asset;p['graphs']={'type':'array','minItems':a['graphCount'],'maxItems':a['graphCount']}
            if a['graphCount']:
                graph=group('Graph');graph['properties'].update(id={'const':a['graphId']},kind={'const':a['graphKind']},nodes=nodes,edges=edges);p['graphs']['items']=graph
        return root
    return {'$schema':'https://json-schema.org/draft/2020-12/schema','title':'Current NodeForgeGraph wire request',
            'x-nativeChecks':'catalog capability archives, canonical names, target occupancy, resource SHA/signatures, native pin legality, default/property exclusivity and graph legality remain native checks',
            'oneOf':[family(a['type'],v,a) for v in d['acceptedGraphVersions'] for a in d['assetTypes']]}

def node_fragment_v3(fragment, definitions):
    """Encode an already qualified documentation fragment; not an execution path."""
    import copy
    node=copy.deepcopy(fragment)
    kind=next((r['kind'] for r in definitions['nodeBindings'] if r['class']==node['class']),'None')
    if kind!='None':
        props=node['properties'];binding={'kind':kind}
        if kind in ('Event','Function'):binding.update(owner=props['owner'],member=props['member'])
        elif kind=='Member':binding['name']=props['variable']
        else:binding.update(path=props['MaterialFunction'],assetSha256=props['FunctionAssetSha256'])
        node.update(properties={},binding=binding)
    return node


def validate_node_fragment(kind,node,explain,definitions=None):
    """Validate literal examples with schemas emitted by the native value definitions."""
    from jsonschema import Draft202012Validator
    fragment=explain['exampleFragment']
    if not isinstance(fragment,dict) or fragment.get('class')!=node['class']:
        raise ValueError('Example class differs from exact catalog identity')
    if kind!='Material':return
    properties=fragment.get('properties',{})
    if definitions is not None:
        binding=next((row['kind'] for row in definitions['nodeBindings'] if row['class']==node['class']),'None')
        if binding!='None':
            group={'Event':'FunctionPropertiesV2','Function':'FunctionPropertiesV2','Member':'MemberPropertiesV2','MaterialFunction':'MaterialFunctionPropertiesV2'}[binding]
            Draft202012Validator(closed_group_schema(definitions,group)).validate(properties)
            return
    allowed=node.get('properties',{})
    if not isinstance(properties,dict):raise ValueError('Example properties must be a literal map')
    for name,value in properties.items():
        if name not in allowed:raise ValueError('Example property not admitted: '+name)
        schema=allowed[name].get('valueSchema')
        if not isinstance(schema,dict):
            raise ValueError('Current candidate must supply its native value schema: '+name)
        Draft202012Validator(schema).validate(value)
    missing=[name for name,schema in allowed.items() if (schema.get('required') or name=='Texture' and schema.get('default') is None) and name not in properties]
    if missing and not all(name in explain['exampleMeaning'] for name in missing):
        raise ValueError('Example must explain missing real dependency: '+','.join(missing))

def publication_evidence(base,release,validation_only,cli_files,operations_sha):
    """A documentation publication cannot establish or silently inherit business verification."""
    return {'actualCodexBusinessAcceptance':
        'notApplicable; offline snapshot preview, no UE/client acceptance' if validation_only else
        'notEstablished by publisher; historical base evidence requires an explicit current impact review',
        'businessEvidenceReuse':{'automaticallyInherited':False,
            'coreIdentityUnchanged':all(base.get(k)==release.get(k) for k in ('binarySha256','engine','adapter','project','stage')),
            'cliBytesUnchanged':base.get('cliFiles')==cli_files,
            'operationsBytesUnchanged':base.get('operationsSource',{}).get('sha256')==operations_sha,
            'bridgeBusinessRequiresImpactReview':True}}


def public_catalog_record(report, binary_sha, qualification):
    """Product facts only; observing PID/project/session stay in the source report."""
    return {name: report[name] for name in ('engine', 'adapter', 'catalogDigest', 'catalog')} | {
        'binarySha256': binary_sha, 'qualification': qualification}


def public_release_record(release):
    """Documentation is not an installation permission for a developer project."""
    fields = ('bundleId', 'buildId', 'bridgeVersion', 'binarySha256', 'engine', 'adapter',
              'graphProtocolVersion', 'acceptedGraphVersions', 'installationSchemaVersion')
    value = {name: release[name] for name in fields}
    value.update(stage='PluginStage/NodeForge', reportNamespace='Reports/PluginControlStageTwo')
    return value

def public_evidence_record(value):
    """Keep product qualification facts; raw execution identities stay internal."""
    private = {'project', 'sessionId', 'controlSessionId', 'processStart', 'runDirectory',
               'reportDirectory', 'sessionFile', 'pipe', 'handle', 'token', 'callId',
               'requestId', 'reportId', 'assetPath', 'installationRoot', 'engineRoot'}
    if isinstance(value, dict):
        return {key: public_evidence_record(item) for key, item in value.items()
                if key not in private and not key.lower().endswith(('pid', 'pids'))}
    if isinstance(value, list):
        return [public_evidence_record(item) for item in value]
    if isinstance(value, str) and (re.search(r'(?i)\b[a-z]:[\\/]', value)
                                 or value.startswith(('Reports/', 'ValidationProject/', 'DevProject/'))):
        return 'internalEvidenceRetained'
    if isinstance(value, str):
        return re.sub(r'B\d{6}', 'HistoricalRun', value)
    return value


def public_installation_text(text):
    """Public recipes retain syntax, not the original operator's live identities."""
    def example(value, key=''):
        if isinstance(value, dict):
            return {name: example(item, name) for name, item in value.items()}
        if isinstance(value, list):
            return [example(item, key) for item in value]
        if key.lower().endswith(('pid', 'pids')) and isinstance(value, int):
            return 1
        if not isinstance(value, str):
            return value
        if key in ('handle', 'controlSessionId', 'sessionId'):
            return '0' * 32
        if key in ('requestId', 'originalRequestId', 'newRequestId') and not value.startswith(('Replace', 'Allocated', 'Example')):
            return 'ExampleRequest' + hashlib.sha256(value.encode()).hexdigest()[:8]
        if key in ('callId', 'prepareCallId') and not value.startswith(('Replace', 'Allocated', 'Example')):
            return 'ExampleCall' + hashlib.sha256(value.encode()).hexdigest()[:8]
        if key in ('reportId', 'exportReportSha256', 'reportSha256', 'payloadSha256', 'sourceAssetSha256'):
            return '0' * 64
        if key in ('exportReportPath', 'reportPath'):
            return 'ReplaceWithCurrentOriginalExportReportPath'
        if key == 'project':
            return 'ReplaceWithSelectedProject.uproject'
        return re.sub(r'B\d{6}', 'ExampleRun', value)
    def block(match):
        try:
            value = json.loads(match[1])
        except json.JSONDecodeError:
            return match.group(0)
        return '```json\n' + json.dumps(example(value), ensure_ascii=False, indent=2) + '\n```'
    original = text
    text = re.sub(r'```json\s*\n(.*?)\n```', block, text, flags=re.S)
    text = re.sub(r'X:(?:\\+|/)+NodeForge', 'ReplaceWithInstallationRoot', text, flags=re.I)
    text = text.replace('NFCUiApiLab', '专用验证工程')
    text = re.sub(r'PID\d+(?:/\d+)*', '历史进程', text)
    text = re.sub(r'B\d{6}', '历史批次', text)
    text = re.sub(r'(?<![A-Za-z0-9])(?:NFA\d{6}[A-Za-z0-9]*|ASB\d{6}[A-Za-z0-9]*|BTB\d{6}[A-Za-z0-9]*)', 'ExampleIdentity', text)
    text = re.sub(r'\[([^\]]+)\]\(([^)]+)\)',
                  lambda m: '[内部历史证据说明]({{page:SourcesAndLicenses}})'
                  if 'Reports/' in m[2] or 'Reports\\' in m[2] else m[0], text)
    def old_release_link(match):
        label, link = match[1], match[2]
        if re.search(r'(?:\.\./)+Build\d+/Mcp[^/]+/', link):
            if link.endswith('/Ai/JsonArchitectureExamples.md'):
                return '[当前完整JSON例子；历史原件内部保留]({{page:AiJsonArchitectureExamples}})'
            if link.endswith('/Ai/AssetSettings.md'):
                return '[当前资产设置说明；历史原件内部保留]({{page:AiAssetSettings}})'
            if link.endswith('/Rollback.json'):
                return '[当前安装回退；历史原件内部保留]({{page:VersionCompatibilityAndMigration}})'
            return '[内部保留的历史证据]({{page:SourcesAndLicenses}})'
        return match[0]
    text = re.sub(r'\[([^\]]+)\]\(([^)]+)\)', old_release_link, text)
    text = re.sub(r'(?<![A-Za-z0-9_/])Reports/PluginControlStageTwo/[^\s；，。、：;,)`]+',
                  '内部保留的历史证据', text)
    if text != original:
        heading, body = text.split('\n', 1)
        text = heading + '\n\n公开副本中的请求、目标、进程、会话及来源报告身份为虚构示例；历史产品结论保持，原始证明由维护者内部保留。实际调用须使用本次分配和返回的完整原 text/sourceRef，示例身份与零摘要不能作为来源证明。\n' + body
    if re.search(r'(?i)\b[a-z]:[\\/]', text):
        raise ValueError('Public installation page retains an absolute local path')
    return text


def publish_release(input_path, resume_render=False):
    """Finite parameterized release on the existing publisher; never regenerates a published directory."""
    import os,copy,subprocess
    from NodeForgeEditorTransport import Windows
    input_path=Path(input_path).resolve()
    if not input_path.is_relative_to(ROOT):raise ValueError('Release input outside workspace')
    cfg=load(input_path)
    if cfg.get('format')!='NodeForgeDocumentationRelease1':raise ValueError('Explicit release input required')
    theme=cfg.get('layoutTheme','classic')
    if theme not in ('classic','brand2','hero'):raise ValueError('Unknown explicit layout theme')
    renderer={'classic':'Scripts/RenderNodeForgeDocs.mjs','brand2':'Scripts/RenderNodeForgeDocsTheme2.mjs','hero':'Scripts/RenderNodeForgeDocsHero.mjs'}[theme]
    output=(ROOT/cfg['output']).resolve()
    resume_input = ROOT/cfg['privatePublicationInput'] if cfg['release'].get('installationSchemaVersion') == 1 else output/'ReleaseInput.json'
    allowed=ROOT/('Reports/PluginControlStageTwo' if cfg.get('validationOnly') is True else 'Docs/Versions')
    Windows().no_reparse(output)
    if not output.is_relative_to(allowed) or output==allowed:raise ValueError('Bounded output required')
    if output.exists():
        if (not resume_render or (output/'Manifest.json').exists() or not resume_input.is_file()
                or load(resume_input)!=cfg):
            raise ValueError('Existing/frozen package protected; only exact unfinished rendering may resume')
    elif resume_render:raise ValueError('No original candidate to resume')
    inputs={name:(ROOT/path).resolve() for name,path in cfg['inputs'].items()}
    for name,p in inputs.items():
        if not p.is_relative_to(ROOT) or not p.is_file():raise ValueError('Invalid exact input: '+name)
        Windows().no_reparse(p)
        if cfg.get('expectedInputSha256',{}).get(name) not in (None,digest(p)):raise ValueError('Input SHA mismatch: '+name)
    base,cat,ops=load(inputs['baseManifest']),load(inputs['catalog']),load(inputs['operations'])
    # Keep the original describe report as source evidence, not as a future PID requirement.
    ops.pop('identity', None)
    evidence,metadata=load(inputs['evidence']),load(inputs['toolMetadata'])
    author_pages,node_docs,controls=load(inputs['pages']),load(inputs['nodeDocumentation']),load(inputs['controls'])
    release=cfg['release']; stage=(ROOT/release['stage']).resolve()
    installation_release = release.get('installationSchemaVersion') == 1
    if installation_release:
        evidence = public_evidence_record(evidence)
    public_release = public_release_record(release) if installation_release else release
    binary=stage/'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'
    Windows().no_reparse(binary)
    if digest(binary)!=release['binarySha256'] or cfg['reviewedCoreBinarySha256']!=release['binarySha256']:
        raise ValueError('DLL or author-reviewed core binding mismatch')
    if cat['engine']!=release['engine'] or cat['adapter']!=release['adapter']:raise ValueError('Catalog/engine/adapter mismatch')
    if declared_bridge_version(ROOT/'Scripts/NodeForgeMcpBridge.py')!=release['bridgeVersion']:
        raise ValueError('Actual bridge source VERSION differs from requested release; preserve current client and prepare a grouped maintenance activation')
    ops_contract=load(inputs['baseToolContract'])
    contract=build_tool_contract(ops_contract,ops,metadata,release['bundleId'],release['bridgeVersion'],
        (output/'Manifest.json').relative_to(ROOT).as_posix())
    contract['graphRequestSchema']=graph_request_schema(cat)
    from jsonschema import Draft202012Validator
    Draft202012Validator.check_schema(contract['graphRequestSchema'])
    contract.update(operationsSnapshot='Metadata/Operations.json' if installation_release else inputs['operations'].relative_to(ROOT).as_posix(),
        operationsSnapshotSha256=hashlib.sha256(dump(ops).encode('utf-8')).hexdigest() if installation_release else digest(inputs['operations']))
    # All knowledge is prepared/validated in memory before creating the new candidate.
    counts={field:len(cat['catalog'][field]['nodes']) for field in ('material','blueprint')}
    facts={**public_release,**counts,'workspaceRoot':'ReplaceWithInstallationRoot' if installation_release else ROOT.as_posix(),
        'documentationRoot':output.relative_to(ROOT).as_posix(),
        'projectName':'SelectedProject' if installation_release else Path(release['project']).stem,
        **({'project':'ReplaceWithSelectedProject.uproject','sessionDirectory':'Reports/PluginControlStageTwo/ReplaceWithCurrentRegistration'} if installation_release else {}),
        'materialCount':counts['material'],'blueprintCount':counts['blueprint'],
        'blueprintClassCount':len({n['class'] for n in cat['catalog']['blueprint']['nodes']}),
        'toolCount':len(contract['tools']),'businessToolCount':sum(t['operation'] is not None for t in contract['tools']),
        'editorApiVersion':ops['editorApiVersion'],'profile':ops['profile'],'catalogDigest':cat['catalogDigest'],
        'publicationStatus':'发布状态以Manifest.publicationStatus为准；根/RC1未升级'}
    for name,path in cfg.get('exampleFiles', {}).items():
        example=(ROOT/path).resolve()
        if not example.is_relative_to(ROOT) or not example.is_file():
            raise ValueError('Exact bounded authored example input required')
        payload=load(example)
        Draft202012Validator(contract['graphRequestSchema']).validate(payload)
        facts[name]=dump(payload).rstrip()
    prepared={}; authors=[]
    for item in author_pages:
        p=(ROOT/item['source']).resolve()
        if not p.is_relative_to(ROOT/'Docs') or not p.is_file():raise ValueError('Invalid authored page')
        text=p.read_text('utf-8')
        if len(text.strip())<180 or not item.get('requiredTopics'):raise ValueError('Missing substantive required knowledge: '+item['pageId'])
        if item['pageId'] in prepared:raise ValueError('Duplicate pageId')
        prepared[item['pageId']]={'output':item['output'],'text':text,'kind':'guide','topics':item['requiredTopics']}
        authors.append(ref(p))
    # Reuse original exact reference text and historical evidence, with correct relocated links.
    old_root=inputs['baseManifest'].parent
    reference_names=('GraphAuthoring','Material','Blueprint','MaterialInstance','Examples','ResultsAndRecovery')
    base_pages={p['pageId']:p for p in base['pages']}
    def original_page(name):
        row=base_pages[name];path=(ROOT/row['path']).resolve()
        if not path.is_relative_to(old_root) or digest(path)!=row['sha256']:
            raise ValueError('Historical reference path/SHA differs: '+name)
        return path
    old_map={original_page(n):'Reference/'+n+'.md' for n in reference_names}
    old_map[original_page('MaterialNodeIndex')]='Reference/MaterialNodeIndex.md'
    old_map[original_page('BlueprintNodeIndex')]='Reference/BlueprintNodeIndex.md'
    for name in reference_names:
        source=original_page(name); target=output/'Reference'/(name+'.md')
        text=source.read_text('utf-8')
        text=text.replace(base['catalogDigest'],cat['catalogDigest'])
        text=text.replace('keys/arrayLength','navigation.keys/navigation.length')
        text=re.sub(r'\[([^\]]+)\]\(([^)]+)\)',lambda m: m.group(0) if re.match(r'^[a-z]+:',m[2],re.I) else
            '['+m[1]+']('+os.path.relpath(output/old_map.get((source.parent/m[2]).resolve(),(source.parent/m[2]).resolve()),target.parent).replace('\\','/')+')',text)
        if name=='Material':
            text=text.replace('48',str(counts['material']))
            if 'Masked' in cat['catalog']['assetDefinitions']['materialSettingsSchema']['properties']['blend']['enum']:
                text=text.replace('Surface、Opaque/Translucent、', 'Surface、Opaque/Translucent/Masked、')
                text=text.replace('输出仅当前模式实际 active 的 BaseColor/EmissiveColor/Metallic/Normal/Opacity/Roughness/Specular/WorldPositionOffset。', '输出仅当前模式实际 active 的 BaseColor/EmissiveColor/Metallic/Normal/Opacity/OpacityMask/Roughness/Specular/WorldPositionOffset。')
                text+='\n\n本包新增 Masked/OpacityMask 和四项材质细节；Masked 显式提供 opacityMaskClipValue，allowNegativeEmissiveColor=true 仅适用于 Unlit。省略、inactive 存储、原生回读及完整 v2/v3 输入见[统一资产设置]({{page:AiAssetSettings}})。\n'
            if any(t['operation']=='resources.inspect' for t in contract['tools']):
                text=text.replace('资源检查 UI/后台取得真实签名；无独立函数查询 MCP，函数默认值/实例/层未适配。',
                    '资源检查UI及正式 nodeforge_resources_inspect(kind=MaterialFunction, source=准确路径)取得真实签名/模板/源SHA；没有专用函数工具，函数本体创建/图导出、函数默认值/实例/层未适配。')
        if name=='MaterialInstance' and any(t['operation']=='resources.inspect' for t in contract['tools']):
            text=text.replace('（检查选中资源 UI/内部后台）','（检查选中资源UI，或正式 resources.inspect(kind=MaterialParameters)读取普通父材质）')
            text=text.replace('没有独立实例 query/MCP资源检查','没有实例对象专用query；普通父材质参数可由正式MCP resources.inspect读取')
            text=text.replace('先由操作者取得实际父参数/sha','操作者准备/注册工程及真实普通父材质后，AI可通过正式资源读取工具取得父参数/sha')
        if name=='Blueprint':
            if 'memberAttributesSchema' in cat['catalog']['assetDefinitions']:
                text+='\n\n已有七类 Value 变量可显式提供 attributes 三字段：instanceEditable、category、tooltip。整组省略保留原 factory 行为，完整规则和原 MCP 配方见[统一资产设置]({{page:AiAssetSettings}})。\n'
            events=[n['properties']['member'] for n in cat['catalog']['blueprint']['nodes'] if n['class']=='/Script/BlueprintGraph.K2Node_Event']
            text=text.replace('事件仅 Actor.ReceiveBeginPlay最多1个；', '事件按本包目录：'+ '、'.join('Actor.'+e for e in events)+'，每个精确事件最多1个、可共存；')
            text=text.replace('91',str(counts['blueprint']))
            # The immediately preceding frozen release may already have a larger count.
            # Refresh count labels from that exact base, rather than the original 91-only text.
            text=re.sub(r'(?<!\d)'+str(base['counts']['blueprint'])+r'(?=\s*签名)',str(counts['blueprint']),text)
            text=re.sub(r'\b\d+ Event\b',str(len(events))+' Event',text)
            calls=sum(node['class']=='/Script/BlueprintGraph.K2Node_CallFunction' for node in cat['catalog']['blueprint']['nodes'])
            text=re.sub(r'\b64 CallFunction\b',str(calls)+' CallFunction',text)
            text=text.replace('数组顺序无执行意义。','nodes/edges条目顺序不表示执行顺序；cases等配置数组保持语义顺序，original来源比较仍核原节点数组顺序。')
        if name=='Examples' and installation_release:
            text='# 最小完整JSON配方与结果恢复\n\n完整当前v2材质/Actor蓝图及依赖明确的实例结构如下。图语义沿用原固定样例，并核当前schema；示例身份不预留，实际提交前按[AiPreparation]({{page:AiPreparation}})核新目标/request与真实目录。维护验收台账不是普通安装前置。\n'
            for kind, filename in [('Material','StaticColor'),('Blueprint','BlueprintConcat')]:
                payload=load(ROOT/f'Examples/GraphV2Samples/{filename}.json')
                payload.update(requestId='Example'+kind,assetPath='/Game/NodeForgeTests/Examples/'+filename,catalogDigest=cat['catalogDigest'])
                Draft202012Validator(contract['graphRequestSchema']).validate(payload)
                text+='\n## '+kind+' v2\n\n'+fence(payload)
            instance={'format':'NodeForgeGraph','version':2,'requestId':'ExampleInstance','assetType':'MaterialInstance','assetPath':'/Game/NodeForgeTests/Examples/Instance','catalogDigest':cat['catalogDigest'],'nodes':[],'edges':[],'settings':{'parent':'/Game/NodeForgeTests/Examples/Parent.Parent','parentAssetSha256':'0'*64,'scalarParameters':{},'vectorParameters':{},'textureParameters':{}}}
            Draft202012Validator(contract['graphRequestSchema']).validate(instance)
            text+='\n## MaterialInstance：填写实际依赖后才可执行\n\nParent路径和全零parentAssetSha256是明确未取得依赖证明的占位值，不是有效父引用。先精确读取本次已保存、clean普通父材质真实参数/文件SHA；空覆盖映射只说明不覆写参数，不证明任何参数覆盖验收。实例MCP未做本期实测。\n\n'+fence(instance)
            text+='\n\n完整v3见[AiJsonArchitectureExamples]({{page:AiJsonArchitectureExamples}})，纹理/函数引用见[AiResources]({{page:AiResources}})。connect后先draft_get；rules_query/catalog_get使用本次handle、新callId、真实expectedDraftRevision。draft_replace/loadFile载入完整输入后再读修订，各新call预检和生成；calls_get查询原callId，report_read按原reportId取checks/readback。unknown停止依赖写入，waitingPersistence保留原输入/收据、正常退出自有UE、同stage新PID原CLI Verify。详细恢复见[AiResultsAndRecovery]({{page:AiResultsAndRecovery}})。\n'
        if name in ('Material','Blueprint','MaterialInstance'):
            heading,body=text.split('\n',1)
            text=heading+'\n\n当前v3结构见[GraphAuthoring]({{page:GraphAuthoring}})，三族完整v3输入见[AiJsonArchitectureExamples]({{page:AiJsonArchitectureExamples}})。下方settings/nodes/edges和绑定properties是仍使用的v2详细表示；v3在asset/graphs/binding中明确分区，解码到同一生产描述。\n\n'+body
        if name=='Naming' and any(t['operation']=='draft.prepareCopy' for t in contract['tools']):
            text=text.replace('准备副本仅 UI 明确操作','准备副本可由UI或正式 draft.prepareCopy明确操作')
        prepared[name]={'output':'Reference/'+name+'.md','text':text,'kind':'guide','topics':[name]};authors.append(ref(source))
    definitions=cat['catalog']['assetDefinitions']
    # The base page already contains generated modern metadata. Reuse only its
    # original detailed wire rules; emitting current facts once prevents every
    # future release from recursively duplicating all preceding definitions.
    legacy_rules = prepared['GraphAuthoring']['text'].rsplit('## 既有v2详细规则与兼容例子', 1)[-1].lstrip()
    prepared['GraphAuthoring']['text'] = legacy_rules
    prepared['GraphAuthoring']['text']='# 当前v3与v2兼容编写\n\n本包推荐NodeForgeGraph v3，七根字段、明确asset/graphs/binding/Value类型。旧v2解码同一模型；旧档案按原wire能力准入：134保持2/3及已验Tick，133保持2/3但无Tick，132保持2且无Tick；三者均不开放本期资产设置。graphs.export显式formatVersion=3；遗漏仍2。准备、预览、重排保持输入版本。raw input/report/payload SHA分别保留；来源original保留数组顺序/presence/目标，retain按原编辑规则。\n\n以下结构和值事实由本包native AssetDefinitions生成；资源/动态pin/编译/保存/Verify仍需正式核心验收。完整当前schema见[GraphRequestSchema.json](../GraphRequestSchema.json)。\n\n'+fence(definitions)+'\n\n## 既有v2详细规则与兼容例子\n\n'+prepared['GraphAuthoring']['text']
    if 'AiJsonArchitectureExamples' in prepared:
        prepared['GraphAuthoring']['text'] += '\n\n三族当前完整v3输入与已使用身份/实际父SHA说明见[完整例子]({{page:AiJsonArchitectureExamples}})。'
    node_explanations={json.dumps(x['identity'],sort_keys=True,ensure_ascii=False,separators=(',',':')):x for x in node_docs}
    if len(node_explanations)!=len(node_docs):raise ValueError('Duplicate authored node identity')
    old_evidence={json.dumps(n['identity'],sort_keys=True,ensure_ascii=False,separators=(',',':')):n for n in evidence['nodes']}
    node_records=[]
    for kind,field in [('Material','material'),('Blueprint','blueprint')]:
        lines=[]
        for i,node in enumerate(cat['catalog'][field]['nodes']):
            key=node_key(kind,node); explain=node_explanations.get(key) or ordinary_node_knowledge(kind,node)
            if not explain or any(not explain.get(k) for k in ('purpose','preconditions','limits','exampleFragment','exampleMeaning','source')):
                raise ValueError('New node lacks actual authored knowledge: '+key)
            validate_node_fragment(kind,node,explain,definitions)
            ident=identity(kind,node); page_id=kind+'Node'+hashlib.sha256(key.encode()).hexdigest()[:12]
            historic=old_evidence.get(key,{}).get('historicalRecords',[])
            record={'pageId':page_id,'identity':ident,'catalogPointer':f'/catalog/{field}/nodes/{i}',
                'catalogSupportState':node['supportState'],'historicalRecords':historic,
                'missingHistoricalIdentityEvidence':not bool(historic),'allConfigurationsVerified':False,'runtimeRetestedThisRelease':False,
                'releaseRecords':old_evidence.get(key,{}).get('releaseRecords',[])}
            node_records.append(record)
            text='# '+explain['purpose']+'\n\n用途：'+explain['purpose']+'。\n\n前提：'+explain['preconditions']+'\n\n限制：'+explain['limits']
            text+='\n\n精确身份（绑定本包；不能凭序号跨版匹配）：\n\n'+fence(ident)
            text+='\n当前原目录属性/允许默认/引脚与类型：\n\n'+fence(node)
            text+='\n当前v3声明片段（v2使用下方历史绑定表示）：\n\n'+fence(node_fragment_v3(explain['exampleFragment'],definitions))+'\n'+explain['exampleMeaning']
            text+='\n\n来源：'+explain['source']+'。\n历史记录保留原身份/配置/Build/层级，缺证据保持notEstablished：\n\n'+fence(historic)
            text+='\n本期资料引用的实际证据（无条目时 notEstablished；签名证据不能代替组合编译/保存/Verify）：\n\n'+fence(record['releaseRecords'])
            text+='\n返回[族索引]({{page:'+kind+'NodeIndex}})、[族限制]({{page:'+kind+'}})和[验证说明]({{page:DeveloperTestingAndEvidence}})。\n'
            prepared[page_id]={'output':'Reference/Nodes/'+page_id+'.md','text':text,'kind':'node','topics':[kind,node.get('family','')],'identity':ident}
            lines.append(f'| [{explain["purpose"]}]({{{{page:{page_id}}}}}) | {page_id} | {node.get("family","")} | {node["supportState"]} |')
        prepared[kind+'NodeIndex']={'output':'Reference/'+kind+'NodeIndex.md','text':
            f'# {kind} 精确节点索引\n\n当前{len(lines)}条；digest '+cat['catalogDigest']+'。页ID按精确身份hash，Manifest再绑定版本；目录可构造不代表所有配置实测。\n\n| 用途 | pageId | family | 状态 |\n| --- | --- | --- | --- |\n'+'\n'.join(lines),'kind':'guide','topics':[kind,'NodeIndex']}
    # Generate reference API from the same contract/help, never a second schema authority.
    api='# 正式MCP工具与原结果\n\n'+metadata['instructions']+'\n\n'
    for tool in contract['tools']:
        api+='## '+tool['name']+'\n\n'+tool['description']+'\n\n输入schema：\n\n'+fence(tool['inputSchema'])+'\n原结果schema：\n\n'+fence(tool['outputSchema'])+'\n'
    # Keep bounded pages without weakening or omitting schemas.
    api_sections=re.split(r'(?m)^## ',api); api_intro=api_sections[0]; section_pages=[]
    for index in range(0,len(api_sections)-1,5):
        page_id='AiApiTools'+str(index//5+1); content=api_intro+'\n'+''.join('## '+s for s in api_sections[1+index:1+index+5])
        prepared[page_id]={'output':'Ai/'+page_id+'.md','text':content,'kind':'guide','topics':['ToolSchema']};section_pages.append(page_id)
    prepared['AiApiReference']={'output':'Ai/ApiReference.md','text':'# API参考入口\n\n原工具/输入/结果schema来自本包ToolContract，帮助来自ToolMetadata；字段原生定义来自Operations。Graph请求、RPC call、报告与handle不可互换。\n\n'+
        '\n'.join('- [工具分卷 '+p+']({{page:'+p+'}})' for p in section_pages)+
        '\n\n稳定公共字段：bridgeVersion/status；业务保留cliExitCode/nativeSummary/reports/unresolvedCalls；错误保留errors.code。docs/connect/report成功分支分别有准确字段；report_read超限给navigation且selectionReturned=false，docs_index发现摘要超32768字节时明确拒绝、不截断冒称完整。unknown/pending/stopping停后续GT；waitingPersistence在原Import报告内，不能改成资产最终通过。额外原生/兼容字段保持，schema不会转换返回值。完整序列见[AiTaskRecipes]({{page:AiTaskRecipes}})和[结果恢复]({{page:AiResultsAndRecovery}})。','kind':'guide','topics':['ApiReference']}
    if 'AiAssetRead' in prepared:
        prepared['AiApiReference']['text'] += '\n\n/exportedRequest是完整导出专用pointer：原Graph最多65536 UTF-8字节，一次返回text/value/payloadSha256/sourceRef，不截断。其它子树仍限32768字节。draft.replace/loadFile只传完整text或原文件path，以及sourceRef中的exportReportPath/exportReportSha256两项；不能把整个sourceRef塞入arguments。详[AiAssetRead]({{page:AiAssetRead}})。'
    # Update the relocated current reference once; AI guidance derives from it.
    prepared['ResultsAndRecovery']['text'] = prepared['ResultsAndRecovery']['text'].replace(
        '工程/会话/执行或业务定义变化为NFMCP_CONFIG_RELOAD_REQUIRED，需显式重连服务。',
        '工程/会话/执行或业务定义变化必须成套批准；兼容更新在原SDK内自动接续，正常新会话重新connect取得handle，不重放旧call。').replace(
        '服务代码升级须真实载入新进程，实际原Codex加载证据以P4为准',
        '原P4服务级载入是历史边界；本版兼容应用代码按批准配置可在同一SDK进程实际载入，新代次以serviceRuntime和当前接续回执为准')
    prepared['AiResultsAndRecovery']={'output':'Ai/ResultsAndRecovery.md','text':prepared['ResultsAndRecovery']['text'].replace('本轮','原有限阶段')+
        '\n\n本版批准的纯资料纠错可同步pages/authoringSources/htmlArtifacts及固定路径文档输入SHA。仅资料刷新保持Catalog/Operations、工具元数据、CLI/SDK/执行绑定和会话；其它变化须成套批准。已载入0.3.2且应用ABI兼容时沿原SDK连接自动接续并重取handle，SDK/解释器/连接核心不兼容才需服务级载入。两条路径均不清unknown/pending。发现索引只返回版本/能力/读者与指南导航；整包维护依据留在本机Manifest及来源/源码页。\n\n原材料完整知识和Verify实际命令均在[AiPreparation]({{page:AiPreparation}})；所有必需资料均可docs_read，外部契约仅补充来源。','kind':'guide','topics':['Results','Recovery']}
    if 'AiAssetRead' in prepared:
        prepared['AiResultsAndRecovery']['text'] += '\n\n新的显式读取/导出与来源绑定恢复详[AiAssetRead]({{page:AiAssetRead}})。sourceMemoryUnproven不自动reload，sourceNotReady不等于不存在；原报告/SHA/源冲突或unknown/pending先查原call和原报告，停止依赖载入/生成，不换目标或身份重发。'
    features=load(inputs['baseCoverage'])['features']
    if definitions['acceptedGraphVersions']==[2,3]:
        for feature in features:
            if feature['business']=='普通 MaterialInstanceConstant':
                feature['entry']=feature['entry'].replace('同一 v2','共同描述，输入v2/v3')
            if feature['business']=='显式保留来源编辑与正式准备副本':
                feature['boundary']=feature['boundary'].replace('Graph v2两族','Graph v2/v3两族')
    exposed={tool['operation'] for tool in contract['tools'] if tool['operation']}
    for feature in features:
        if feature.get('mcp') == '当前15业务投影中；原版本证据保持其范围':
            feature['mcp'] = f'当前{len(exposed)}业务投影中；原版本证据保持其范围'
    feature_text='# 已有功能与入口矩阵\n\n本表说明当前声明的业务和开放边界。基础包P3固定两图和新PID Verify属于其原版本历史证据；本版是否复用须有影响Review，发布器不建立UE验收结论。目录不是全UE准入。\n\n| 功能 | UI | 正式入口/MCP | 保护与限制 |\n| --- | --- | --- | --- |\n'
    if any(n['properties'].get('member')=='ReceiveTick' for n in cat['catalog']['blueprint']['nodes']) and not any(f['business']=='普通Actor ReceiveTick与持续样例' for f in features):
        features.append({'business':'普通Actor ReceiveTick与持续样例','ui':'同一受控图输入与原生成按钮','entry':'原catalog/preflight/generate/export工具；没有逐事件或公开PIE工具','boundary':'每个精确事件最多1个；有限X轴100厘米/秒样例的实际Delta/三时刻/两区间证据见AiBlueprintTick；其它组合仍experimental'})
    if any(f['name']=='opacityMaskClipValue' for f in definitions['fieldGroups']['MaterialSettings']):
        features.append({'business':'Surface材质细节与七类变量编辑属性','ui':'材质/变量原生Details；同一输入与原生成按钮','entry':'原catalog、preflight、generate、export、prepare/preview工具；工具数以本包契约为准','boundary':'Masked/OpacityMask、四项材质设置、instanceEditable/category/tooltip；字段条件与完整v2/v3见AiAssetSettings；当前候选实际画面/Win64有限范围见VersionReleaseNotes，TAA/LOD仍未重测'})
    if 'resources.query' in exposed:
        features.append({'business':'有界资源发现、精确检查与引用','ui':'原资源读取与图输入区域；无新增浏览器面板','entry':'resources.query / resources.inspect / 原graphs入口','boundary':'Texture2D、MaterialFunction、Material元数据列表；发现/政策/检查/图导出分开，实际来源准入与恢复见AiResources'})
    for f in features:feature_text+='| '+' | '.join(str(f[k]).replace('P3待实测','P3原有限实测通过').replace('本期只补 MCP 双图','原P3固定两图通过') for k in ('business','ui','entry','boundary'))+' |\n'
    feature_text+='\n\n当前{{materialCount}}材质类/{{blueprintCount}}蓝图签名；普通实例沿共同描述由原生核心构造，当前实际证据层见本包发行说明；没有实例专用MCP或实例图导出。规则查询仅Material/Blueprint。'
    feature_text+=('resources.inspect/graphs.export已正式投影，准确对象/完整payload/来源绑定见[AiAssetRead]({{page:AiAssetRead}})。' if {'resources.inspect','graphs.export'}.issubset(exposed) and 'AiAssetRead' in prepared else '手动读取/导出尚无独立MCP。')
    feature_text+=('draft.prepareCopy/graphs.preview沿共享Service正式投影，显式retainExportOrigin编辑和真实准备身份/完整差异见[AiCopyPreview]({{page:AiCopyPreview}})；准确验证层以本版发行说明为准。重排和纯视图仍无独立MCP。' if {'draft.prepareCopy','graphs.preview'}.issubset(exposed) and 'AiCopyPreview' in prepared else '副本预览、重排和纯视图仍无独立MCP。')
    feature_text+='受支持种类/复杂度见[Material]({{page:Material}})、[Blueprint]({{page:Blueprint}})、[实例]({{page:MaterialInstance}})。\n'
    prepared['FeatureMap']={'output':'Reference/FeatureMap.md','text':feature_text,'kind':'guide','topics':['FeatureMap']}
    source_map=cfg['sourceMap']
    map_text='# 实际源码地图\n\n文件/关键入口按职责定位，读取实际当前源码和本版stage，不凭旧聊天猜文件。根源码用于继续维护；本版stage与原交付的连续性须按实际Review核验，新改动须按影响构建。\n\n| 职责/输入输出 | 文件与关键入口 | 调用关系/契约 |\n| --- | --- | --- |\n'
    if installation_release:
        map_text='# 实际源码地图\n\n交付包的原生受测源码在PluginStage/NodeForge/Source，正式工具在Scripts。先核InstallationManifest、ReleaseManifest和同版资料Manifest的产品字节/目录/工具身份，再沿[Architecture]({{page:DeveloperArchitecture}})读相关模块。下面原生路径已映射交付包；维护库源码相应位于Plugins/NodeForge/Source。标注“维护库专用”的测试台账、原报告、文稿维护源和渲染工具不随安装包交付，不能作为安装运行依赖。继续完整产品开发须取得准确维护库及新授权，不靠旧聊天或公开历史示例恢复执行权。\n\n| 职责/输入输出 | 文件与关键入口 | 调用关系/契约 |\n| --- | --- | --- |\n'
    for row in source_map:
        for path in row['files']:
            if not (ROOT/path).is_file():raise ValueError('Source map file does not exist: '+path)
        display=[]
        for path in row['files']:
            if installation_release and path.startswith('Plugins/NodeForge/Source/'):
                path=path.replace('Plugins/NodeForge/Source/','PluginStage/NodeForge/Source/',1)
            elif installation_release and (path.startswith(('Tests/','Reports/','Docs/PluginControl/','Tools/NodeForgeDocs/')) or path=='Scripts/RenderNodeForgeDocsHero.mjs'):
                path+='（维护库专用，不随安装包交付）'
            display.append(path)
        map_text+='| '+row['role']+' | '+'; '.join(display)+'；'+row['entry']+' | '+row['relation']+' |\n'
    if installation_release:
        map_text+='| 有界查询/精确纹理检查/引用资格 | PluginStage/NodeForge/Source/NodeForgeEditor/Private/NodeForgeResourceSource.h、.cpp；QueryArgumentsSchema/EffectiveQueryArguments/Query/InspectTexture/QualifyTexture/FCapture | 原安装读取根与AssetRegistry元数据查询不加载对象；精确检查建立当前saved/clean证明；普通Texture引用沿MaterialGraph同一生产资格，列表不升级来源 |\n'
    map_text+='\n\n本表不是新API承诺。原理见[Architecture]({{page:DeveloperArchitecture}})；三类扩展及发布见DeveloperNodeExtension/OperationExtension/VersionRelease；现行进度见根五文件，原报告提供限定历史证据。\n'
    prepared['DeveloperSourceMap']={'output':'Developer/SourceMap.md','text':map_text,'kind':'guide','topics':['SourceMap']}
    notices=inputs['sourceNotice'].read_text('utf-8')
    prepared['SourcesAndLicenses']={'output':'Reference/SourcesAndLicenses.md','text':notices,'kind':'guide','topics':['Sources','Licenses']}
    for required in cfg['requiredPageIds']:
        if required not in prepared:raise ValueError('Missing reader foundation: '+required)
    if not all(t in prepared for row in contract['tools'] for t in row['help']['guides']):raise ValueError('Tool guide page not readable by pageId')
    paths={k:(output/v['output']).resolve() for k,v in prepared.items()}
    if len(set(paths.values()))!=len(paths) or any(not p.is_relative_to(output) for p in paths.values()):raise ValueError('Page output collision/outside scope')
    pages=[]; rendered=[]
    for page_id,row in prepared.items():
        text=row['text']
        for k,v in facts.items():text=text.replace('{{'+k+'}}',str(v))
        if installation_release:
            text=public_installation_text(text)
        def page_link(match):
            dest=paths.get(match[1])
            if dest is None:raise ValueError('Missing page link: '+match[1])
            return os.path.relpath(dest,paths[page_id].parent).replace('\\','/')
        text=re.sub(r'\{\{page:([^}]+)\}\}',page_link,text)
        if '{{' in text or len(text.encode('utf-8'))>65536:raise ValueError('Unresolved/big page: '+page_id)
        if installation_release:
            for raw in re.findall(r'```json\s*\n(.*?)\n```',text,re.S):
                payload=json.loads(raw)
                if isinstance(payload,dict) and payload.get('format')=='NodeForgeGraph':
                    Draft202012Validator(contract['graphRequestSchema']).validate(payload)
        title=text.splitlines()[0].lstrip('# ').strip()
        rendered.append({'pageId':page_id,'output':row['output'],'title':title,'text':text})
    def save_candidate(path,text):
        if path.exists():
            if path.read_bytes()!=text.encode('utf-8'):raise ValueError('Partial candidate changed: '+str(path))
        else:save(path,text)
    output.mkdir(parents=True,exist_ok=resume_render)
    for page in rendered:
        save_candidate(output/page['output'],page['text'])
        row=prepared[page['pageId']]
        entry={'pageId':page['pageId'],'title':page['title'],**ref(output/page['output']),'kind':row['kind'],'topics':row['topics']}
        if 'identity' in row:entry['identity']=row['identity']
        pages.append(entry)
    new_evidence=copy.deepcopy(evidence);new_evidence['nodes']=node_records
    diff=catalog_diff(load(inputs['previousCatalog']),cat)
    previous_contract=load(inputs.get('previousToolContract',inputs['baseToolContract']))
    previous_tools={x['name']:x for x in previous_contract['tools']}
    diff.update(operationNames={'previous':[r['name'] for r in load(inputs['previousOperations'])['operations']],
        'current':[r['name'] for r in ops['operations']]},
        newTools=[t['name'] for t in contract['tools'] if t['name'] not in previous_tools],
        toolInputChanged=[t['name'] for t in contract['tools'] if t['name'] in previous_tools and t['inputSchema']!=previous_tools[t['name']]['inputSchema']])
    coverage={'format':'NodeForgeUsageCoverage2','counts':counts,'features':features,'controls':controls,
        'requiredGuideIds':cfg['requiredPageIds'],'nativeOperations':[x['name'] for x in ops['operations']],
        'businessTools':[x['name'] for x in contract['tools'] if x['operation']],
        'knowledgeCategories':['User','Ai','Developer','Version','Reference'],'nodeExplanations':len(node_records),
        'noFullNodeRetest':True,'ueTestsExecutedByGenerator':False}
    index={'format':'NodeForgeDocumentationIndex1','bundleId':release['bundleId'],'pages':pages}
    rollback={'format':'NodeForgeRollback1','restoreTogether':cfg['rollbackFiles'],
        'sdkUnchanged':True,'oldBundle':cfg.get('rollbackBundle',base['bundleId']),'preserveNewHistory':True,
        'stopIfActiveUnknown':True,'requiresServiceReconnect':True}
    publication_input = cfg
    publication_input_name = 'ReleaseInput.json'
    catalog_output = cat
    if installation_release:
        # Keep the exact source input internally. The public derivative contains
        # no renderer executable, selected project or session/rollback identity.
        private_input = (ROOT / cfg['privatePublicationInput']).resolve()
        if not private_input.is_relative_to(ROOT / 'Reports/PluginControlStageTwo'):
            raise ValueError('Private publication input must stay in the internal report namespace')
        save_candidate(private_input, dump(cfg))
        publication_input_name = 'PublicRelease.json'
        publication_input = {'format':'NodeForgePublicRelease1', 'release':public_release,
            'counts':counts, 'catalogDigest':cat['catalogDigest'], 'tools':len(contract['tools']),
            'qualification':cfg['catalogQualification'], 'fullPublicContentAudit':'pendingPC6PC7',
            'privateInstallation':'PrepareNodeForgeInstallation generates local bindings/commands; not included in public release'}
        catalog_output = public_catalog_record(cat, release['binarySha256'], cfg['catalogQualification'])
        rollback = {'format':'NodeForgeInstallationRollback1','action':'Original explicit bind receipt + unchanged after SHA via PrepareNodeForgeInstallation --action rollback',
            'preserveUnrelatedEdits':True,'preserveCallsReports':True,'stopIfActiveUnknown':True,
            'sdkUnchanged':True,'applicationAbi':1,'privateReceiptIncluded':False}
    for filename,value in [('Catalog.json',cat),('Operations.json',ops),('EvidenceIndex.json',new_evidence),
        ('Coverage.json',coverage),('DocumentationIndex.json',index),('ToolContract.json',contract),
        (publication_input_name,publication_input),('Diff.json',diff),('Rollback.json',rollback),('GraphRequestSchema.json',contract['graphRequestSchema'])]:
        save_candidate(output/filename,dump(catalog_output if filename=='Catalog.json' else value))
    rendered_result=subprocess.run([cfg['nodeExe'],str(ROOT/renderer)],
        input=json.dumps({'root':str(output),'bundleId':release['bundleId'],'pages':rendered},ensure_ascii=False),
        encoding='utf-8',stdout=subprocess.PIPE,stderr=subprocess.PIPE,creationflags=subprocess.CREATE_NO_WINDOW)
    if rendered_result.returncode:
        failure={'stage':'render','status':'failed','input':ref(input_path),'returncode':rendered_result.returncode,
            'stderrTail':rendered_result.stderr[-1200:],'ueStarted':False,'assetsGenerated':False}
        save_candidate(output/'GenerationFailure.json',dump(failure))
        raise ValueError('Renderer failed; preserve partial candidate: '+rendered_result.stderr[-500:])
    layout_rows=layout_publication_rows(output,release['bundleId'])
    bindings={p:ref(ROOT/p) for p in cfg['bindingFiles']}
    for row in layout_rows['sources']:bindings[row['path']]=row
    manifest={k:v for k,v in base.items() if k not in ('pages','artifacts','bindings','publicationStatus',
        'releaseEvidence','frozenAt','sourceRepair','sourceDocsRefresh','firstProtocolObservationNotCurrentRevision')}
    manifest['historicalBaseRelease']=ref(inputs['baseManifest'])
    cli_files={}
    for name in base['cliFiles']:
        p=(ROOT/name).resolve()
        if Path(name).is_absolute() or not p.is_relative_to(ROOT/'Scripts') or not p.is_file():
            raise ValueError('CLI binding outside actual scripts')
        Windows().no_reparse(p)
        cli_files[name]=digest(p)
    manifest['cliFiles']=cli_files
    if installation_release:
        # Runtime bindings stay exact; historical raw report references remain
        # internal instead of being inherited into distributable metadata.
        manifest = {'cliFiles':cli_files}
        for path in cfg.get('additionalCliFiles', []):
            source=(ROOT/path).resolve()
            if not source.is_relative_to(ROOT/'Scripts') or not source.is_file():
                raise ValueError('Additional CLI binding must be a real product script')
            cli_files[path]=digest(source)
    manifest.update(public_release,format='NodeForgeUsageManifest1',catalogDigest=cat['catalogDigest'],counts=counts,
        publicationStatus='offlineSnapshotPreview' if cfg.get('validationOnly') else cfg.get('publicationStatus','candidateCompleteE2'),fullUsagePackageComplete=True,pages=pages,
        validationOnly=bool(cfg.get('validationOnly')),
        artifacts=[ref(output/f) for f in ('Catalog.json','Operations.json','EvidenceIndex.json','Coverage.json',
            'DocumentationIndex.json','ToolContract.json',publication_input_name,'Diff.json','Rollback.json','GraphRequestSchema.json')]+layout_rows['artifacts'],
        bindings=list(bindings.values()),authoringSources=authors,
        htmlArtifacts=layout_rows['html'],
        readerEntrypoints={'user':'UserReadme','ai':'AiReadme','developer':'DeveloperReadme','version':'VersionReadme'},
        developerKnowledgeComplete=True)
    if not installation_release:
        manifest.update(catalogSource=ref(inputs['catalog']),operationsSource=ref(inputs['operations']),
            publicationInputs={k:ref(p) for k,p in inputs.items()})
    else:
        manifest.update(catalogQualification=cfg['catalogQualification'],fullPublicContentAudit='pendingPC6PC7')
    manifest.update(publication_evidence(base,release,bool(cfg.get('validationOnly')),cli_files,digest(inputs['operations'])))
    save(output/'Manifest.json',dump(manifest))
    receipt={'generator':ref(Path(__file__)),'input':ref(input_path),'manifest':ref(output/'Manifest.json'),
        'render':json.loads(rendered_result.stdout),'pages':len(pages),'nodePages':len(node_records),
        'tools':len(contract['tools']),'ueStarted':False,'assetsGenerated':False,'status':'candidateRequiresReview'}
    save(output/'Generation.json',dump(receipt))
    print(json.dumps({'bundleId':release['bundleId'],'pages':len(pages),'nodes':len(node_records),'candidate':str(output),'ueStarted':False}))
    return manifest

def publish_installation_edition(input_path):
    """Reuse frozen public knowledge for one bounded installation release.

    Unchanged node pages retain their IDs and content, not a new qualification.
    Current installation guides are explicit inputs; old manifests are immutable.
    """
    import copy
    cfg=load(Path(input_path))
    if cfg.get('format')!='NodeForgeInstallationDocumentationEdition1':raise ValueError('Exact edition input required')
    version=cfg['version']
    if not re.fullmatch(r'\d+\.\d+\.\d+',version):raise ValueError('Release version required')
    base_path=(ROOT/cfg['baseManifest']).resolve();base=load(base_path);old=base_path.parent
    output=(ROOT/cfg['output']).resolve()
    if output!=ROOT/'Docs/Versions'/('v'+version) or output.exists():raise ValueError('Fresh exact version output required')
    if digest(base_path)!=cfg['baseManifestSha256']:raise ValueError('Frozen base changed')
    stage=(ROOT/cfg['stage']).resolve();installation=load(ROOT/'Releases'/version/'InstallationManifest.json')
    if digest(stage/'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll')!=installation['binarySha256']:raise ValueError('Tested core mismatch')
    cat=load(stage/'Metadata/Catalog.json');ops=load(ROOT/'Metadata/Operations.json')
    metadata=load(ROOT/'Tools/NodeForgeMcp/ToolMetadata.json')
    contract=build_tool_contract(load(ROOT/cfg['baseToolContract']),ops,metadata,'NodeForge-'+version,
        declared_bridge_version(ROOT/'Scripts/NodeForgeMcpBridge.py'),(output/'Manifest.json').relative_to(ROOT).as_posix())
    contract['graphRequestSchema']=graph_request_schema(cat)
    contract.update(operationsSnapshot='Metadata/Operations.json',operationsSnapshotSha256=digest(ROOT/'Metadata/Operations.json'))
    from jsonschema import Draft202012Validator
    Draft202012Validator.check_schema(contract['graphRequestSchema'])
    old_version=base['aiVersion'];pages=[];prepared=[]
    for row in base['pages']:
        source=(ROOT/row['path']).resolve()
        if not source.is_relative_to(old) or digest(source)!=row['sha256']:raise ValueError('Frozen page changed: '+row['pageId'])
        override=cfg.get('guideOverrides',{}).get(row['pageId'])
        if override:
            source=(ROOT/override).resolve()
            if not source.is_relative_to(ROOT/'Docs') or not source.is_file():raise ValueError('Invalid public guide input')
        text=source.read_text('utf-8').replace('v'+old_version,'v'+version).replace('NodeForge-'+old_version,'NodeForge-'+version).replace('NodeForge-AI-'+old_version,'NodeForge-AI-'+version)
        text=text.replace('桥0.3.6','桥0.4.0')
        if row['pageId']=='VersionReadme':
            text=text.replace('这是 NodeForge 的第一个可用版本，','本版增加引擎级共享安装和 AI 一次接入，')
        if row['pageId']=='DeveloperVersionRelease':
            text=text.replace('v'+version+' 是第一个可用版本。','v0.1.0 是首个可用版本；本版增加引擎级安装与 AI 一次接入。')
        destination=output/(ROOT/row['path']).relative_to(old)
        # Canonical repository guides become same-version local links.
        text=text.replace('(Versions/v'+version+'/', '(../')
        if row['pageId']=='UserGettingStarted':
            text=text.replace('(Installation.md)','(PluginInstallation.md)')
        prepared.append((destination,text,{**row,'title':re.search(r'^#\s+(.+)$',text,re.M)[1]}))
    api_intro='# MCP 工具与返回 schema\n\n有 handle 的工具使用 connected 返回的本工程/本会话句柄。pending/unknown 时查询原 callId，停止依赖写入。\n\n'
    for number in range(1,5):
        page_id='AiApiTools'+str(number)
        body=api_intro+''.join('## '+t['name']+'\n\n'+t['description']+'\n\n输入schema：\n\n'+fence(t['inputSchema'])+'\n原结果schema：\n\n'+fence(t['outputSchema'])+'\n' for t in contract['tools'][(number-1)*5:number*5])
        prepared=[(p,body,{**r,'title':'MCP 工具与返回 schema'}) if r['pageId']==page_id else (p,t,r) for p,t,r in prepared]
    output.mkdir(parents=True)
    for destination,text,row in prepared:
        save(destination,text);pages.append({**row,**ref(destination)})
    for name,value in [('Catalog.json',cat),('Operations.json',ops),('GraphRequestSchema.json',contract['graphRequestSchema']),('ToolContract.json',contract)]:save(output/name,dump(value))
    save(ROOT/'Releases'/version/'ToolContract.json',dump(contract))
    manifest=copy.deepcopy(base)
    cli_names=['NodeForgeControl.py','NodeForgeEditorTransport.py','RunProcessGuard.py','PrepareNodeForgeInstallation.py']
    manifest.update(bundleId=contract['bundleId'],buildId=version,aiVersion=version,bridgeVersion=contract['bridgeVersion'],
        binarySha256=installation['binarySha256'],stage='.',reportNamespace='Reports/PluginControlStageTwo/NodeForge',
        publicationStatus='candidate',pages=pages,counts={k:len(cat['catalog'][k]['nodes']) for k in ('material','blueprint')},
        catalogDigest=cat['catalogDigest'],cliFiles={'Scripts/'+n:digest(ROOT/'Scripts'/n) for n in cli_names},
        artifacts=[ref(output/n) for n in ['Catalog.json','Operations.json','GraphRequestSchema.json','ToolContract.json']],
        bindings=[ref(ROOT/'Scripts'/n) for n in ['NodeForgeMcp.py','NodeForgeMcpBridge.py','NodeForgeAiInstall.py','NodeForgeAiCheck.py','NodeForgeAiLaunch.py']]
            +[ref(ROOT/'Tools/NodeForgeMcp'/n) for n in ['DependencyManifest.json','Requirements.lock','ThirdPartyNotices.md']])
    save(output/'Manifest.json',dump(manifest))
    receipt={'status':'candidateRequiresReview','pages':len(pages),'tools':len(contract['tools']),
        'generator':ref(Path(__file__).resolve()),'input':ref(Path(input_path).resolve()),'manifest':ref(output/'Manifest.json'),
        'qualification':'Unchanged node knowledge reused; new installation/lifecycle evidence in E0-E4; no new node qualification'}
    save(ROOT/cfg['receipt'],dump(receipt));print(dump({k:receipt[k] for k in ['status','pages','tools']}))
    return manifest

if __name__ == '__main__':
    import argparse
    parser=argparse.ArgumentParser(description='Existing NodeForge documentation publisher; exact finite release input required')
    parser.add_argument('--release-input')
    parser.add_argument('--edition-input',help='Exact bounded installation edition of existing public knowledge')
    parser.add_argument('--resume-render',action='store_true',help='Only resume the exact unfinished candidate after preserved diagnosis; no manifest may exist')
    args=parser.parse_args()
    if args.edition_input:publish_installation_edition(args.edition_input)
    elif args.release_input:publish_release(args.release_input,args.resume_render)
    else:main() # historical path retains its immutable/frozen guard
