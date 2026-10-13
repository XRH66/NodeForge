"""Bounded explicit native blueprint runtime cases, shared by copy and game checks."""
import math
import re
from RunGraphRegression import ROOT, read_json, ref, sha

def load_ui_cases(path):
    """Separate strict contract for representative UI assets; never weaken the location manifest."""
    path = path.resolve()
    if not path.is_relative_to(ROOT / 'Reports') or not path.is_file() or path.stat().st_size > 65536:
        raise ValueError('Bounded existing Reports UI case manifest required')
    data = read_json(path)
    if not isinstance(data, dict) or set(data) != {'format', 'version', 'cases'} or data['format'] != 'NodeForgeUiCandidateCases' \
            or type(data['version']) is not int or data['version'] != 1:
        raise ValueError('Exact NodeForgeUiCandidateCases v1 manifest required')
    cases = data['cases']
    if not isinstance(cases, list) or len(cases) != 2:
        raise ValueError('Exactly one Blueprint and one Material representative required')
    names, maps, kinds = set(), set(), set()
    for case in cases:
        if not isinstance(case, dict) or set(case) != {'case', 'map', 'assetType', 'assetPath', 'expectedMessage'}:
            raise ValueError('Unknown or missing UI case field')
        name, kind = case['case'], case['assetType']
        if not isinstance(name, str) or not re.fullmatch(r'[A-Za-z0-9]{1,24}', name) or name in names:
            raise ValueError('Unique bounded UI case name required')
        if not test_asset(case['map']) or not test_asset(case['assetPath']) or case['map'] in maps:
            raise ValueError('Unique exact test asset/map paths required')
        if kind not in ('Blueprint', 'Material') or kind in kinds:
            raise ValueError('Exactly one of each native asset kind required')
        message = case['expectedMessage']
        if kind == 'Blueprint' and (not isinstance(message, str) or not message or len(message) > 256 or '\n' in message or '\r' in message):
            raise ValueError('Explicit single-line Blueprint message required')
        if kind == 'Material' and message is not None:
            raise ValueError('Material screenshot case must not claim Blueprint runtime')
        names.add(name); maps.add(case['map']); kinds.add(kind)
    return cases

def candidate_files(plugin):
    plugin = plugin.resolve()
    if not plugin.is_relative_to(ROOT / 'Reports') or not plugin.as_posix().endswith('/PluginStage/NodeForge'):
        raise ValueError('Frozen candidate PluginStage required')
    return {ref(p): sha(p) for p in plugin.rglob('*') if p.is_file()}

def validate_ui_delivery(preparation, project, manifest, cases):
    """Recheck provenance and saved bytes before native game/package execution."""
    if preparation.get('kind') != 'NodeForgeUiCandidateDelivery1' or preparation.get('result') != 'prepared' \
            or preparation.get('project') != ref(project) or preparation.get('caseManifest') != ref(manifest) \
            or preparation.get('caseManifestSha256') != sha(manifest) \
            or preparation.get('maps') != [c['map'] for c in cases]:
        raise ValueError('Actual hash-locked UI delivery preparation required')
    evidence = (ROOT / preparation['uiEvidence']).resolve()
    if not evidence.is_relative_to(ROOT / 'Reports') or sha(evidence) != preparation['uiEvidenceSha256']:
        raise ValueError('UI evidence changed or outside Reports')
    summary = read_json(evidence)
    if summary.get('result') != 'passed' or summary.get('oneUnchangedBuild') is not True:
        raise ValueError('Passing actual UI receipt/reopen/PIE/GPU evidence required')
    original_project = (ROOT / summary['uiProject']).resolve()
    verified_project = (ROOT / summary['verificationCopy']).resolve()
    for p in (original_project, verified_project):
        if not p.is_relative_to(ROOT / 'ValidationProject/RegressionRuns') or not p.is_file():
            raise ValueError('UI and verification projects must stay in RegressionRuns')
    expected_steps = {'BlueprintReopen', 'MaterialReopen', 'BlueprintMapCreate', 'BlueprintRuntime',
                      'MaterialSceneCreate', 'MaterialVisualCheck'}
    steps = summary.get('steps', [])
    if len(steps) != 6 or {s['step'] for s in steps} != expected_steps or any(s.get('passed') is not True for s in steps):
        raise ValueError('Exactly six completed native UI-asset checks required')
    for kind, method in [('Blueprint', 'file'), ('Material', 'paste')]:
        item = summary['receipts'][kind]
        receipt_file = (ROOT / item['report']).resolve()
        input_file = (ROOT / item['input']).resolve()
        original = (ROOT / item['originalInput']).resolve()
        if not receipt_file.is_relative_to(ROOT / 'Reports/UiImports') or not input_file.is_relative_to(ROOT) \
                or not original.is_relative_to(ROOT / 'SourceData/ExternalGraphAi2'):
            raise ValueError('Actual UI receipt/raw input path required')
        if sha(receipt_file) != item['reportSha256'] or sha(input_file) != item['inputSha256'] or sha(original) != item['originalInputSha256']:
            raise ValueError('Actual UI receipt or input changed')
        receipt = read_json(receipt_file)
        case = next(c for c in cases if c['assetType'] == kind)
        if receipt.get('inputMethod') != method or receipt.get('overallStatus') != 'waitingPersistence' \
                or receipt.get('assetPath') != case['assetPath'] or receipt.get('inputDigest') != item['inputSha256'] \
                or receipt.get('project', '').replace('\\', '/').lower() != original_project.as_posix().lower() \
                or receipt.get('executionFingerprint', {}).get('binarySha256') != preparation['dllSha256']:
            raise ValueError('UI receipt method/project/asset/input/DLL differs')
        asset = original_project.parent / 'Content' / (case['assetPath'][6:] + '.uasset')
        verified = verified_project.parent / 'Content' / (case['assetPath'][6:] + '.uasset')
        copied = project.parent / 'Content' / (case['assetPath'][6:] + '.uasset')
        if any(sha(p) != item['assetSha256'] for p in (asset, verified, copied)):
            raise ValueError('UI source, verification or delivery asset differs')
        reopened = read_json(ROOT / next(s['report'] for s in steps if s['step'] == kind + 'Reopen'))
        if reopened.get('overallStatus') != 'succeeded' or reopened.get('fileSha256') != item['assetSha256'] \
                or reopened.get('checks', {}).get('persistence', {}).get('status') != 'passed' \
                or reopened.get('pid') == item['pid']:
            raise ValueError('Independent same-byte reopen not proven')
    for item in preparation['assets']:
        source, copied = (ROOT / item['source']).resolve(), (ROOT / item['copy']).resolve()
        if not source.is_relative_to(verified_project.parent / 'Content') or not copied.is_relative_to(project.parent / 'Content') \
                or sha(source) != item['sha256'] or sha(copied) != item['sha256']:
            raise ValueError('Representative source/copy bytes changed')
    plugin = ROOT / preparation['selectedPlugin']
    if candidate_files(plugin) != preparation['validatedBuild']:
        raise ValueError('Candidate files differ from frozen delivery evidence')
    return plugin

def test_asset(path):
    return isinstance(path, str) and re.fullmatch(r'/Game/NodeForgeTests/[A-Za-z0-9]{1,64}', path) is not None

def load_cases(path):
    path = path.resolve()
    if not path.is_relative_to(ROOT) or not path.is_file() or path.stat().st_size > 65536:
        raise ValueError('Bounded existing workspace case manifest required')
    data = read_json(path)
    if not isinstance(data, dict) or data.get('format') != 'NodeForgeStandaloneCases' or type(data.get('version')) is not int or data.get('version') != 1 or set(data) != {'format', 'version', 'cases'}:
        raise ValueError('Exact NodeForgeStandaloneCases v1 manifest required')
    cases = data.get('cases')
    if not isinstance(cases, list) or not 1 <= len(cases) <= 32:
        raise ValueError('One to 32 explicit blueprint runtime cases required')
    names, maps, blueprints = set(), set(), set()
    for case in cases:
        if not isinstance(case, dict) or set(case) != {'case', 'map', 'blueprint', 'expectedMessage', 'expectedLocation'}:
            raise ValueError('Unknown or missing explicit case field')
        name = case['case']
        if not isinstance(name, str) or not re.fullmatch(r'[A-Za-z0-9]{1,24}', name) or name in names:
            raise ValueError('Unique bounded ASCII case names required')
        if not test_asset(case['map']) or not test_asset(case['blueprint']) or case['map'] in maps or case['blueprint'] in blueprints:
            raise ValueError('Unique exact isolated saved blueprint/map paths required')
        message = case['expectedMessage']
        location = case['expectedLocation']
        if not isinstance(message, str) or not message or len(message) > 256 or '\n' in message or '\r' in message:
            raise ValueError('Single-line nonempty runtime expectation required')
        if not isinstance(location, list) or len(location) != 3 or any(type(v) not in (int, float) or not math.isfinite(v) or abs(v) > 1e12 for v in location):
            raise ValueError('Three bounded finite expected coordinates required')
        if message != 'X=%.3f Y=%.3f Z=%.3f' % tuple(location):
            raise ValueError('Position message must use native FVector three-decimal format')
        names.add(name); maps.add(case['map']); blueprints.add(case['blueprint'])
    return cases

def verify_location_probe(request, case):
    """Require real native dataflow in the already independently verified saved graph."""
    if request.get('assetType') != 'Blueprint' or request.get('assetPath') != case['blueprint']:
        raise ValueError('Case references a different generated blueprint')
    def one(owner, member):
        matches = [n for n in request['nodes'] if n.get('class') == '/Script/BlueprintGraph.K2Node_CallFunction'
            and n.get('properties', {}).get('owner') == owner and n.get('properties', {}).get('member') == member]
        if len(matches) != 1:
            raise ValueError('Exactly one native readback/print/move instance required: ' + member)
        return matches[0]
    get = one('/Script/Engine.Actor', 'K2_GetActorLocation')
    move = one('/Script/Engine.Actor', 'K2_SetActorLocation')
    convert = one('/Script/Engine.KismetStringLibrary', 'Conv_VectorToString')
    printer = one('/Script/Engine.KismetSystemLibrary', 'PrintString')
    if 'InString' in printer.get('defaults', {}) or printer.get('defaults', {}).get('bPrintToLog') is not True:
        raise ValueError('Position must come from live data, not a literal default or disabled log')
    needed = [{'from': get['id'], 'output': 'ReturnValue', 'to': convert['id'], 'input': 'InVec'},
              {'from': convert['id'], 'output': 'ReturnValue', 'to': printer['id'], 'input': 'InString'},
              {'from': move['id'], 'output': 'then', 'to': printer['id'], 'input': 'execute'}]
    if any(edge not in request['edges'] for edge in needed):
        raise ValueError('Native self readback after movement is not wired to PrintString')
    return {'source': 'GetActorLocation -> Conv_VectorToString -> PrintString after SetActorLocation',
            'getNode': get['id'], 'convertNode': convert['id'], 'printNode': printer['id'],
            'literalPositionDefault': False, 'runtimeTextPrecision': 'native FVector ToString: three decimals'}
