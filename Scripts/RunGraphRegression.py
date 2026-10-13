"""Replay data-driven graphs in an isolated project using one unchanged plugin build.

Project-owned extension of RunPrototypeRegression.py / ValidatePrototype.py.  Uses only
the Python standard library; never builds the plugin or installs dependencies.  Root
agent/user schedules actual Unreal execution.  --prepare-only writes inputs and the
isolated project without launching Unreal.  Reports and projects must be new.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import struct
import subprocess
import sys
import time
from pathlib import Path
from RunProcessGuard import run_guarded


ROOT = Path(__file__).resolve().parents[1]
ENGINE = Path('X:/UnrealEngine/UE_5.3/Engine/Binaries/Win64/UnrealEditor-Cmd.exe')
FIXTURES = ROOT / 'Tests/GraphV2'
REQUIRED_IMPORT_CHECKS = ('validation', 'build', 'initialReadback', 'compile',
                          'finalReadback', 'publish', 'save')
ASSET_SUFFIXES = {'.uasset', '.umap'}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def ref(path):
    return path.resolve().relative_to(ROOT).as_posix()


def read_json(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def write_new(path, value):
    with path.open('x', encoding='utf-8', newline='\n') as stream:
        json.dump(value, stream, ensure_ascii=False, indent=2, allow_nan=False)
        stream.write('\n')


def files_under(folder, suffixes=None):
    return {ref(path): sha(path) for path in sorted(folder.rglob('*'))
            if path.is_file() and (suffixes is None or path.suffix in suffixes)}


def build_snapshot(plugin_directory=None):
    snapshot = files_under(ROOT / 'Plugins/NodeForge/Source')
    for path in [ROOT / 'Plugins/NodeForge/NodeForge.uplugin',
                 ROOT / 'Plugins/NodeForge/Binaries/Win64/UnrealEditor-NodeForgeEditor.dll',
                 ROOT / 'Plugins/NodeForge/Binaries/Win64/UnrealEditor.modules',
                 ROOT / 'DevProject/Binaries/Win64/UnrealEditor-NodeForgeEditor.dll']:
        snapshot[ref(path)] = sha(path) if path.is_file() else None
    if plugin_directory:
        snapshot.update(files_under(plugin_directory))
    return snapshot


def protected_assets():
    snapshot = {}
    for folder in ['DevProject/Content', 'ValidationProject/Content']:
        snapshot.update(files_under(ROOT / folder, ASSET_SUFFIXES))
    return snapshot


def subset_errors(expected, actual, location='value', number_policy='double'):
    """Compare typed native values; extra fields are allowed, magnitude-sized gaps are not."""
    errors = []
    if isinstance(expected, dict):
        if not isinstance(actual, dict):
            return [location + ': expected object in actual readback']
        for key, value in expected.items():
            if key not in actual:
                errors.append(location + '.' + key + ': missing in actual readback')
            else:
                errors.extend(subset_errors(value, actual[key], location + '.' + key, number_policy))
    elif isinstance(expected, list):
        if not isinstance(actual, list) or len(expected) != len(actual):
            return [location + ': actual array length differs']
        for index, value in enumerate(expected):
            errors.extend(subset_errors(value, actual[index], f'{location}[{index}]', number_policy))
    elif isinstance(expected, bool):
        if expected is not actual:
            errors.append(location + ': boolean differs')
    elif isinstance(expected, (int, float)):
        good = not isinstance(actual, bool) and isinstance(actual, (int, float)) and math.isfinite(actual) and math.isfinite(expected)
        if good:
            if isinstance(expected, int) or number_policy == 'exact':
                good = expected == actual
            elif number_policy == 'float32':
                try:
                    wanted = struct.unpack('<f', struct.pack('<f', expected))[0]
                    # UE exports the stored float as a JSON double. Never requantize actual.
                    good = math.isfinite(wanted) and actual == wanted
                except (OverflowError, struct.error):
                    good = False
            elif number_policy == 'double':
                tolerance = min(1e-3, max(1e-6, abs(expected) * 1e-15))
                good = abs(expected - actual) <= tolerance
            else:
                raise ValueError('Unknown numeric comparison policy: ' + number_policy)
        if not good:
            errors.append(location + ': numeric value differs')
    elif expected != actual:
        errors.append(location + ': value differs')
    return errors


def automation_report_passed(data, test_name):
    """The native overall status and required protections, never a passed substep."""
    if data.get('test') != test_name or data.get('status') != 'passed':
        return False
    if test_name == 'NodeForge.MinimalPanel':
        return all(data.get(key) is True for key in ('tabOpened', 'tabClosed', 'preExistingProductionTabPreserved')) and data.get('widgetConstruction') == 'passed'
    if test_name == 'NodeForge.BlueprintRuntime':
        return all(data.get(key) is True for key in ('hasActorBegunPlay', 'pieStopped', 'loggerRemoved')) and data.get('actorCount') == 1 and data.get('attributedMessageCount', 0) >= 1
    return False


def readback_errors(request, actual, function_signatures=None):
    """Independent structural checks; never accept a success flag alone."""
    if not isinstance(actual, dict):
        return ['Actual graph readback is missing']
    errors = []
    number_policy = 'float32' if request.get('assetType') in ('Material', 'MaterialInstance') else 'double'
    nodes = actual.get('nodes')
    if not isinstance(nodes, list):
        return ['Actual graph readback has no nodes array']
    by_id = {node.get('id'): node for node in nodes if isinstance(node, dict)}
    expected_ids = {node['id'] for node in request['nodes']}
    if len(nodes) != len(by_id) or set(by_id) != expected_ids:
        errors.append('Actual logical node IDs or count differ from input')
    for expected in request['nodes']:
        found = by_id.get(expected['id'])
        if found is None:
            continue
        if expected['class'] == '/Script/Engine.MaterialExpressionMaterialFunctionCall':
            signature = (function_signatures or {}).get(expected['properties'].get('MaterialFunction'))
            if signature is None:
                errors.append(expected['id'] + ': independent native function signature missing')
            else:
                errors.extend(subset_errors(signature, found.get('functionSignature'), expected['id'] + '.functionSignature', 'float32'))
        for field in ['class', 'properties', 'defaults']:
            if field in expected:
                errors.extend(subset_errors(expected[field], found.get(field),
                                            expected['id'] + '.' + field, number_policy))
        if 'position' in expected and found.get('position') != expected['position']:
            errors.append(expected['id'] + '.position: explicit integer coordinates changed')
    def edge_key(edge):
        if not isinstance(edge, dict):
            return ('INVALID', '', '', '')
        return tuple(str(edge.get(key)) for key in ('from', 'output', 'to', 'input'))
    edges = actual.get('edges')
    if not isinstance(edges, list) or sorted(map(edge_key, edges)) != sorted(
            map(edge_key, request['edges'])):
        errors.append('Actual edge endpoints, ports or count differ from input')
    errors.extend(subset_errors(request['settings'], actual.get('settings'), 'settings', number_policy))
    if request['assetType'] == 'Material' and actual.get('expressionCount') != len(request['nodes']):
        errors.append('Actual material expressionCount differs from input')
    if request['assetType'] == 'MaterialInstance':
        if actual.get('nativeClass') != '/Script/Engine.MaterialInstanceConstant':
            errors.append('Actual native constant material instance class missing')
        for field in ('exactOverridesAndEffectiveValuesMatch', 'parentBytesUnchanged', 'matchesRequest'):
            if actual.get(field) is not True:
                errors.append('Independent instance readback flag not passed: ' + field)
        settings = actual.get('settings', {})
        for kind in ('scalarParameters', 'vectorParameters', 'textureParameters'):
            if set(settings.get(kind, {})) != set(request['settings'][kind]):
                errors.append('Extra or missing actual override keys: ' + kind)
    if 'layout' in actual:
        errors.extend(layout_errors(request, actual))
    return errors


def layout_positions(actual):
    """Persisted coordinates, independently keyed by logical ID (including root)."""
    layout = actual.get('layout', {}) if isinstance(actual, dict) else {}
    nodes = layout.get('nodes', []) if isinstance(layout, dict) else []
    return {node.get('id'): node.get('position') for node in nodes if isinstance(node, dict)}


def layout_errors(request, actual):
    """Check actual reported rectangles; do not reimplement the layout algorithm."""
    errors = []
    layout = actual.get('layout')
    if not isinstance(layout, dict) or not isinstance(layout.get('nodes'), list):
        return ['Layout report or layout nodes are missing']
    for field in ('algorithm', 'sizePolicy'):
        if not isinstance(layout.get(field), str) or not layout[field]:
            errors.append('layout.' + field + ': missing provenance')
    requested = {node['id']: node for node in request['nodes']}
    real = {node.get('id'): node for node in actual.get('nodes', []) if isinstance(node, dict)}
    entries = layout['nodes']
    by_id = {node.get('id'): node for node in entries if isinstance(node, dict)}
    expected_ids = set(requested) | ({'Material'} if request['assetType'] == 'Material' else set())
    if len(entries) != len(by_id) or set(by_id) != expected_ids:
        errors.append('Layout IDs/count differ from real nodes and declared virtual root')
    rectangles = []
    limit = 1000000 if request['assetType'] == 'Material' else 100000
    for node_id, node in by_id.items():
        position, size = node.get('position'), node.get('boundsSize')
        if not isinstance(position, list) or len(position) != 2 or any(
                isinstance(value, bool) or not isinstance(value, (int, float))
                or not math.isfinite(value) or value != int(value) or abs(value) > limit
                for value in position):
            errors.append(str(node_id) + ': layout position is not two bounded integers')
            continue
        if not isinstance(size, list) or len(size) != 2 or any(
                isinstance(value, bool) or not isinstance(value, (int, float))
                or not math.isfinite(value) or value <= 0 for value in size):
            errors.append(str(node_id) + ': layout boundsSize must be two positive finite dimensions')
            continue
        expected_explicit = node_id == 'Material' or 'position' in requested.get(node_id, {})
        if node.get('explicitPosition') is not expected_explicit:
            errors.append(str(node_id) + ': layout explicitPosition differs from input declaration')
        if node_id in requested and 'position' in requested[node_id] and position != requested[node_id]['position']:
            errors.append(str(node_id) + ': layout moved an explicit anchor')
        if node_id in real and real[node_id].get('position') != position:
            errors.append(str(node_id) + ': actual node position differs from layout report')
        if not isinstance(node.get('layer'), int) or isinstance(node.get('layer'), bool):
            errors.append(str(node_id) + ': layout layer must be integer')
        if not isinstance(node.get('sizeSource'), str) or not node['sizeSource']:
            errors.append(str(node_id) + ': layout sizeSource missing')
        rectangles.append((node_id, position, size, expected_explicit))
    anchor_overlaps = []
    for index, (left_id, left, left_size, left_explicit) in enumerate(rectangles):
        for right_id, right, right_size, right_explicit in rectangles[index + 1:]:
            overlap = (left[0] < right[0] + right_size[0] and right[0] < left[0] + left_size[0]
                       and left[1] < right[1] + right_size[1] and right[1] < left[1] + left_size[1])
            if overlap:
                if left_explicit and right_explicit:
                    anchor_overlaps.append((left_id, right_id))
                else:
                    errors.append(f'Automatic rectangle overlaps: {left_id} / {right_id}')
    warnings = layout.get('explicitAnchorOverlapWarnings')
    if not isinstance(warnings, list) or any(not isinstance(item, str) for item in warnings):
        errors.append('layout.explicitAnchorOverlapWarnings must be a string array')
    elif anchor_overlaps and not warnings:
        errors.append('Overlapping preserved explicit anchors lack an overlap warning')
    elif not anchor_overlaps and warnings:
        errors.append('Layout claims explicit anchor overlap absent from actual rectangles')
    return errors


def semantic_graph(request):
    """Identity/array-order-independent graph representation for variation evidence."""
    return {'assetType': request['assetType'], 'settings': request['settings'],
            'nodes': sorted([{key: value for key, value in node.items() if key != 'position'}
                             for node in request['nodes']], key=lambda node: node['id']),
            'edges': sorted(request['edges'], key=lambda edge: (
                edge['from'], edge['output'], edge['to'], edge['input']))}


def actual_graph_digest(actual):
    if not isinstance(actual, dict) or not isinstance(actual.get('nodes'), list):
        return None
    normalized = {
        'nodes': sorted([{key: node.get(key) for key in ('id', 'class', 'properties', 'defaults')}
                         for node in actual['nodes']], key=lambda node: node['id']),
        'edges': sorted([{key: edge.get(key) for key in ('from', 'output', 'to', 'input')}
                         for edge in actual.get('edges', [])], key=lambda edge: (
                             edge['from'], edge['output'], edge['to'], edge['input'])),
        'settings': actual.get('settings')}
    return hashlib.sha256(json.dumps(normalized, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def catalog_signature_errors(request, catalog, function_signatures=None):
    """Check positive fixtures against exported instance signatures, not guessed port names."""
    if request['assetType'] == 'MaterialInstance':
        signature = catalog.get('materialInstance', {})
        return [] if signature.get('assetType') == 'MaterialInstance' and signature.get(
            'nativeClass') == '/Script/Engine.MaterialInstanceConstant' and request['nodes'] == [] \
            and request['edges'] == [] else ['Native instance catalog signature or empty graph missing']
    domain = catalog.get('material' if request['assetType'] == 'Material' else 'blueprint', {})
    entries = domain.get('nodes', [])
    variables = {item['name']: item['type'] for item in request['settings'].get('variables', [])}
    signatures = {}
    errors = []
    for node in request['nodes']:
        candidates = [entry for entry in entries if entry.get('class') == node['class']]
        if request['assetType'] == 'Blueprint':
            variable = node['properties'].get('variable')
            if variable:
                candidates = [entry for entry in candidates if entry.get('variableType') == variables.get(variable)]
            elif any(entry.get('caseAdapter') for entry in candidates):
                # Native Switch probes provide base/hidden and exec case signatures.
                # User labels/counts are parameterized, then checked on the actual UE instance.
                candidates = [entry for entry in candidates if entry.get('caseAdapter')
                              and entry.get('properties', {}).get('hasDefault') is node['properties'].get('hasDefault')]
            else:
                candidates = [entry for entry in candidates if entry.get('properties') == node['properties']]
        if len(candidates) != 1 or candidates[0].get('constructible') is not True:
            errors.append(node['id'] + ': no unique constructible actual catalog signature')
            continue
        entry = candidates[0]
        adapter = entry.get('caseAdapter') if request['assetType'] == 'Blueprint' else None
        if adapter:
            props = node['properties']
            if adapter['kind'] == 'switchInteger':
                count, start = props.get('caseCount'), props.get('startIndex')
                if set(props) != {'startIndex', 'caseCount', 'hasDefault'} or type(count) is not int or type(start) is not int \
                        or not adapter['minimumCaseCount'] <= count <= adapter['maximumCaseCount'] \
                        or start < -2147483648 or start + count > 2147483647:
                    errors.append(node['id'] + ': invalid bounded native integer case configuration')
                    continue
                names = [str(start + i) for i in range(count)]
            elif adapter['kind'] == 'switchName':
                names = props.get('cases')
                if set(props) != {'cases', 'hasDefault'} or not isinstance(names, list) \
                        or not adapter['minimumCaseCount'] <= len(names) <= adapter['maximumCaseCount'] \
                        or any(not isinstance(name, str) or not re.fullmatch('[A-Za-z][A-Za-z0-9]{0,63}', name) for name in names) \
                        or len({name.casefold() for name in names}) != len(names) \
                        or any(name.casefold() in {'none', 'default', 'execute', 'selection', 'notequal_namename', 'notequal_intint'} for name in names):
                    errors.append(node['id'] + ': invalid bounded native name case configuration')
                    continue
            else:
                errors.append(node['id'] + ': unknown native case adapter')
                continue
            is_case = lambda pin: pin.get('direction') == 'output' and pin.get('category') == 'exec' and pin.get('name') != 'Default'
            prototypes = [pin for pin in entry.get('pins', []) if is_case(pin)]
            if not prototypes or any(pin.get('hidden') or pin.get('subcategory') != prototypes[0].get('subcategory') for pin in prototypes):
                errors.append(node['id'] + ': native visible case output prototype missing')
                continue
            entry = dict(entry, pins=[pin for pin in entry['pins'] if not is_case(pin)]
                         + [dict(prototypes[0], name=name) for name in names])
        if node['class'] == '/Script/Engine.MaterialExpressionMaterialFunctionCall':
            signature = (function_signatures or {}).get(node['properties'].get('MaterialFunction'))
            if signature is None or signature['properties'] != node['properties']:
                errors.append(node['id'] + ': no matching actual bound function signature/hash')
                continue
            entry = dict(entry, inputs=signature['inputs'], outputs=signature['outputs'])
        if request['assetType'] == 'Material':
            inputs = {port['name']: port for port in entry.get('inputs', [])}
            outputs = {port['index']: port for port in entry.get('outputs', [])}
            unknown = set(node['properties']) - set(entry.get('properties', {}))
            if unknown:
                errors.append(node['id'] + ': properties absent from actual catalog: ' + ', '.join(sorted(unknown)))
            for port in node.get('defaults', {}):
                if not inputs.get(port, {}).get('defaultProperty'):
                    errors.append(node['id'] + '.' + port + ': actual catalog has no default mapping')
        else:
            def port_name(port):
                return node['properties'].get('variable') if port['name'] == 'Value' \
                    and 'variable' in node['properties'] else port['name']
            inputs = {port_name(port): port for port in entry.get('pins', [])
                      if port.get('direction') == 'input' and not port.get('hidden')}
            outputs = {port_name(port): port for port in entry.get('pins', [])
                       if port.get('direction') == 'output' and not port.get('hidden')}
            for port in node.get('defaults', {}):
                if not inputs.get(port, {}).get('defaultWritable'):
                    errors.append(node['id'] + '.' + port + ': actual catalog has no writable default')
        signatures[node['id']] = {'inputs': inputs, 'outputs': outputs}
    for edge in request['edges']:
        if edge['output'] not in signatures.get(edge['from'], {}).get('outputs', {}):
            errors.append(edge['from'] + '.' + edge['output'] + ': output absent from actual catalog')
        if request['assetType'] == 'Material' and edge['to'] == 'Material':
            available = domain.get('materialInputs', [])
        else:
            available = signatures.get(edge['to'], {}).get('inputs', {})
        if edge['input'] not in available:
            errors.append(edge['to'] + '.' + edge['input'] + ': input absent from actual catalog')
        elif isinstance(available, dict) and available[edge['input']].get('connectible') is False:
            errors.append(edge['to'] + '.' + edge['input'] + ': actual native input is blocked by the adapter contract')
    return errors


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--run', required=True, help='Fresh ASCII alphanumeric run name')
    parser.add_argument('--catalog', required=True, help='Existing GraphCatalog report inside workspace')
    parser.add_argument('--prepare-only', action='store_true', help='Write inputs, launch no Unreal processes')
    parser.add_argument('--timeout', type=int, default=360)
    parser.add_argument('--case', action='append', help='Run selected manifest case(s); partial coverage is explicit')
    parser.add_argument('--fixtures', default='Tests/GraphV2', help='Fixture directory inside workspace; original 33-case matrix remains default')
    parser.add_argument('--plugin-directory', help='Explicit staged NodeForge folder under Reports; never replace the live Editor module')
    parser.add_argument('--function-signature', action='append', default=[], help='Actual read-only native function report with the selected DLL fingerprint')
    return parser.parse_args()


def main():
    args = parse_args()
    plugin_directory = (ROOT / args.plugin_directory).resolve() if args.plugin_directory else ROOT / 'Plugins/NodeForge'
    if args.plugin_directory and (not plugin_directory.is_relative_to(ROOT / 'Reports') or plugin_directory.name != 'NodeForge'):
        raise SystemExit('Staged plugin must be an explicit NodeForge directory under Reports')
    if not (plugin_directory / 'NodeForge.uplugin').is_file():
        raise SystemExit('Plugin descriptor missing')
    loaded_binary = plugin_directory / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'
    snapshot = lambda: build_snapshot(plugin_directory if args.plugin_directory else None)
    if not re.fullmatch(r'[A-Za-z0-9]{1,24}', args.run):
        raise SystemExit('Use a fresh ASCII alphanumeric run name, at most 24 characters')
    catalog_path = (ROOT / args.catalog).resolve()
    if not catalog_path.is_relative_to(ROOT) or not catalog_path.is_file():
        raise SystemExit('Catalog must be an existing file inside the workspace')
    catalog_report = read_json(catalog_path)
    digest = catalog_report.get('catalogDigest')
    function_signatures = {}
    signature_evidence = {}
    for name in args.function_signature:
        path = (ROOT / name).resolve()
        if not path.is_relative_to(ROOT / 'Reports') or not path.is_file():
            raise SystemExit('Function signature must be an existing actual report')
        data = read_json(path)
        signature = data.get('functionSignature', {})
        if data.get('mode') != 'MaterialFunction' or data.get('operationStatus') != 'passed' \
                or data.get('readOnlyExecutionFingerprint', {}).get('binarySha256') != sha(loaded_binary) \
                or not signature.get('properties', {}).get('FunctionAssetSha256'):
            raise SystemExit('Function report must be native, successful and from the selected DLL')
        key = signature['properties']['MaterialFunction']
        if key in function_signatures:
            raise SystemExit('Duplicate function signature')
        function_signatures[key] = signature
        signature_evidence[ref(path)] = sha(path)
    if not re.fullmatch(r'[0-9a-f]{64}', digest or '') or not isinstance(catalog_report.get('catalog'), dict):
        raise SystemExit('A real GraphCatalog report with catalog and SHA-256 catalogDigest is required')
    fixtures = (ROOT / args.fixtures).resolve()
    if not fixtures.is_relative_to(ROOT) or not (fixtures / 'Cases.json').is_file():
        raise SystemExit('Fixture directory must have Cases.json inside the workspace')
    manifest = read_json(fixtures / 'Cases.json')
    cases = manifest['cases']
    names = {case['name'] for case in cases}
    if args.case:
        unknown = set(args.case) - names
        if unknown:
            raise SystemExit('Unknown case(s): ' + ', '.join(sorted(unknown)))
        cases = [case for case in cases if case['name'] in args.case]
        if any(case.get('requires') not in (None, *args.case) for case in cases):
            raise SystemExit('A selected case needs its prerequisite case in --case arguments')
        if any(case.get('compareLayoutWith') not in (None, *args.case) for case in cases):
            raise SystemExit('A selected layout comparison needs its reference case in --case arguments')
    out = ROOT / 'Reports/20261008GraphV2' / args.run
    project_dir = ROOT / 'ValidationProject/RegressionRuns' / args.run
    if out.exists() or project_dir.exists():
        raise SystemExit('Use new report and project directories; old evidence/assets are preserved')
    if not args.prepare_only and not ENGINE.is_file():
        raise SystemExit('Verified UE executable is unavailable; do not install or switch engines')
    inputs = []
    for case in cases:
        path = (fixtures / case['input']).resolve()
        if not path.is_relative_to(ROOT) or not path.is_file():
            raise SystemExit('Fixture input must be an existing workspace file')
        request = read_json(path)
        if not case.get('preserveCatalogDigest', False):
            request['catalogDigest'] = digest
        if request['requestId'] != 'Graph' + case['name']:
            raise SystemExit('Fixture requestId must be Graph plus the case name: ' + str(path))
        if case['expected'] == 'succeeded':
            signature_errors = catalog_signature_errors(request, catalog_report['catalog'], function_signatures)
            if signature_errors:
                raise SystemExit('Actual catalog preflight failed for ' + case['name'] + ':\n'
                                 + '\n'.join(signature_errors))
        inputs.append((case, request, path))
    out.mkdir(parents=True, exist_ok=False)
    project_dir.mkdir(parents=True, exist_ok=False)
    (out / 'Inputs').mkdir()
    (out / 'InputTemplates').mkdir()
    project = project_dir / 'NodeForgeGraphRegression.uproject'
    write_new(project, {'FileVersion': 3, 'EngineAssociation': '5.3',
                       'AdditionalPluginDirectories': [str(plugin_directory.parent)],
                       'Plugins': [{'Name': 'NodeForge', 'Enabled': True}]})
    # The same existing regression renderer configuration; only this fresh project is written.
    (project_dir / 'Config').mkdir()
    (project_dir / 'Config/DefaultEngine.ini').write_text(
        '[/Script/Engine.RendererSettings]\nr.DynamicGlobalIlluminationMethod=0\n'
        'r.ReflectionMethod=0\nr.DefaultFeature.AutoExposure=False\n', encoding='utf-8')
    prepared = []
    for case, request, source in inputs:
        deferred = bool(case.get('parentHashFromCase'))
        target = out / ('InputTemplates' if deferred else 'Inputs') / (case['name'] + '.json')
        write_new(target, request)
        prepared.append({'name': case['name'], 'input': ref(target), 'source': ref(source),
                         'sourceSha256': sha(source), 'inputSha256': sha(target),
                         'nodeCount': len(request['nodes']), 'edgeCount': len(request['edges']),
                         'expected': case['expected'], 'category': case['category'],
                         'parentHashBinding': 'pending native saved-parent inspection' if deferred else 'notRequired',
                         'semanticGraphSha256': hashlib.sha256(json.dumps(
                             semantic_graph(request), sort_keys=True, separators=(',', ':')).encode()).hexdigest()})
    write_new(out / 'Preparation.json', {'project': ref(project), 'catalog': ref(catalog_path),
                                       'fixtures': ref(fixtures),
                                       'catalogSha256': sha(catalog_path), 'catalogDigest': digest,
                                       'positiveActualCatalogSignatureChecks': 'passed',
                                       'cases': prepared, 'unrealExecution': 'notRun'})
    if args.prepare_only:
        print('Prepared inputs only; Unreal not run: ' + ref(out), flush=True)
        return 0

    before_build = snapshot()
    before_protected = protected_assets()
    if not loaded_binary.is_file():
        raise SystemExit('No installed plugin DLL; build/synchronization must be scheduled separately')
    write_new(out / 'Baseline.json', {'sourceAndBinary': before_build, 'protectedAssets': before_protected})
    steps = []
    results = []
    graph_digests = {}
    graph_layouts = {}
    parent_inspections = {}

    def execute(name, options):
        report = out / (name + '.json')
        command = [str(ENGINE), str(project), '-run=NodeForge', *options,
                   '-NFReport=' + str(report), '-d3d11', '-RenderOffscreen',
                   '-AllowCommandletRendering', '-unattended', '-nop4', '-nosound',
                   '-NoLiveCoding', '-abslog=' + str(out / (name + '.log')),
                   '-stdout', '-FullStdOutLogOutput']
        write_new(out / (name + 'Command.json'), command)
        started = time.monotonic()
        exit_code = None
        error = None
        try:
            process = run_guarded(command, cwd=ROOT, console=out / (name + 'Console.txt'), timeout=args.timeout,
                expected_module=loaded_binary)
            exit_code = process.returncode
        except OSError as exc:
            error = str(exc)
        data = {}
        if report.exists():
            try:
                data = read_json(report)
            except (UnicodeError, json.JSONDecodeError) as exc:
                error = str(exc)
        step = {'step': name, 'command': command, 'exitCode': exit_code,
                'durationSeconds': round(time.monotonic() - started, 3), 'report': ref(report),
                'actual': data.get('overallStatus'), 'executionError': error}
        steps.append(step)
        print(name, exit_code, data.get('overallStatus'), error or '', flush=True)
        return data, step, report

    for case, request, _ in inputs:
        name = case['name']
        before_case = files_under(project_dir / 'Content', ASSET_SUFFIXES)
        previous_hashes = dict(before_case)
        errors = []
        prerequisite = case.get('requires')
        if prerequisite and not any(result['name'] == prerequisite and result['result'] == 'passed'
                                    for result in results):
            result = {'name': name, 'result': 'blocked',
                      'errors': ['Required successful case did not pass: ' + prerequisite]}
            results.append(result)
            write_new(out / (name + 'Result.json'), result)
            continue
        if snapshot() != before_build:
            # Continuing with a changed build would invalidate the central no-recompile claim.
            results.append({'name': name, 'result': 'blocked', 'errors': ['Plugin source or binary changed during matrix']})
            break
        source = out / 'Inputs' / (name + '.json')
        parent_case = case.get('parentHashFromCase')
        if parent_case:
            parent_request = next((r for c, r, _ in inputs if c['name'] == parent_case), None)
            parent_file = project_dir / 'Content' / (parent_request['assetPath'].removeprefix('/Game/') + '.uasset') if parent_request else None
            if not parent_request or not parent_file.is_file() or not any(
                    result['name'] == parent_case and result['result'] == 'passed' for result in results):
                results.append({'name': name, 'result': 'blocked', 'errors': ['Saved parent prerequisite not verified: ' + parent_case]})
                continue
            parent_path = parent_request['assetPath'] + '.' + parent_request['assetPath'].rsplit('/', 1)[-1]
            if parent_case not in parent_inspections:
                config = out / (parent_case + 'InspectInput.json')
                write_new(config, {'parent': parent_path})
                inspection, inspect_step, _ = execute(parent_case + 'Inspect', ['-NFMode=MaterialParameters', '-NFInput=' + str(config)])
                if inspect_step['exitCode'] != 0 or inspection.get('overallStatus') != 'succeeded' \
                        or inspection.get('parameters', {}).get('parent') != parent_path \
                        or inspection.get('parameters', {}).get('parentAssetSha256') != sha(parent_file) \
                        or files_under(project_dir / 'Content', ASSET_SUFFIXES) != before_case:
                    results.append({'name': name, 'result': 'blocked', 'errors': ['Native parent inspection failed or altered saved assets']})
                    continue
                parent_inspections[parent_case] = inspection['parameters']
            parameters = parent_inspections[parent_case]
            # Only derived execution input gets the real saved-parent hash. Original templates stay unchanged.
            request['settings']['parentAssetSha256'] = '0' * 64 if case.get('staleParentHash') else sha(parent_file)
            if case['expected'] == 'succeeded':
                for kind in ('scalarParameters', 'vectorParameters', 'textureParameters'):
                    if not set(request['settings'][kind]).issubset(parameters.get(kind, {})):
                        errors.append('Actual parent inspection lacks requested typed parameters: ' + kind)
            write_new(source, request)
            write_new(out / (name + 'InputBinding.json'), {
                'template': ref(out / 'InputTemplates' / (name + '.json')),
                'templateSha256': sha(out / 'InputTemplates' / (name + '.json')),
                'parentCase': parent_case, 'actualParent': parent_path, 'actualParentAssetSha256': sha(parent_file),
                'requestedHashIsIntentionallyStale': bool(case.get('staleParentHash')),
                'derivedInput': ref(source), 'derivedInputSha256': sha(source),
                'bindingOnly': 'parentAssetSha256; original request properties/parent path unchanged'})
        data, step, generation_report = execute(name + 'Import', ['-NFMode=Import', '-NFInput=' + str(source)])
        after_import = files_under(project_dir / 'Content', ASSET_SUFFIXES)
        if case['expected'] == 'rejected':
            if step['exitCode'] in (None, 0) or data.get('overallStatus') != 'rejected':
                errors.append('Invalid request did not explicitly reject with a nonzero exit code')
            if data.get('checks', {}).get('build', {}).get('status') != 'notRun':
                errors.append('Invalid request progressed to building')
            if data.get('residue', {}).get('diskState') != 'notWritten':
                errors.append('Rejected request reports a disk write/residue')
            if before_case != after_import:
                errors.append('Rejected request changed the isolated project asset set or bytes')
        elif case['expected'] in ('failed', 'cleanupRequired'):
            # A compile-time negative is distinct from a validation rejection.
            # The contract permits cleanupRequired when an owned candidate remains.
            # Require the exact fixture status and explicit no-save evidence in both cases.
            if step['exitCode'] in (None, 0):
                errors.append('Compile-negative request did not return a nonzero exit code')
            if data.get('overallStatus') != case['expected']:
                errors.append('Compile-negative overallStatus differs from the declared contract expectation')
            if data.get('checks', {}).get('build', {}).get('status') != 'passed' \
                    or data.get('checks', {}).get('compile', {}).get('status') != 'failed' \
                    or data.get('failedStage') != 'compile':
                errors.append('Compile-negative failure was not attributed to actual compilation')
            if data.get('executionFingerprint', {}).get('binarySha256') != sha(loaded_binary):
                errors.append('Compile-negative actual loaded binary differs')
            residue = data.get('residue', {})
            if residue.get('diskState') != 'notWritten' or before_case != after_import:
                errors.append('Compile-negative request changed saved assets or reports disk residue')
            if residue.get('saveAttempted') is not False or residue.get('targetFilePresent') is not False:
                errors.append('Compile-negative request lacks explicit no-save/no-target-file evidence')
            if any(data.get('checks', {}).get(stage, {}).get('status') != 'notRun'
                   for stage in ('finalReadback', 'publish', 'save', 'persistence', 'runtime')):
                errors.append('Compile-negative request progressed beyond the failed compilation')
            compilation = data.get('compilation', {})
            if compilation.get('compilationFinished') is not True or not compilation.get('errors'):
                errors.append('Compile-negative request lacks completed native compiler error evidence')
            if data.get('inputDigest') != sha(source) or data.get('requestId') != request['requestId'] \
                    or data.get('assetPath') != request['assetPath']:
                errors.append('Compile-negative receipt does not identify the exact input and target')
            if case['expected'] == 'cleanupRequired':
                candidate = residue.get('candidate', '')
                if not candidate.startswith('/Game/NodeForgeStaging/Session') \
                        or not residue.get('ownedObject', '').startswith(candidate + '.') \
                        or residue.get('target') != request['assetPath'] \
                        or not residue.get('cleanup') or residue.get('cleanup') == 'noneRequired':
                    errors.append('Cleanup-required failure lacks owned staging object and cleanup evidence')
        else:
            if data.get('executionFingerprint', {}).get('binarySha256') != sha(loaded_binary):
                errors.append('Actual loaded binary differs from the selected plugin build')
            if step['exitCode'] != 0 or data.get('overallStatus') != 'waitingPersistence':
                errors.append('Import did not complete compilation and save')
            if data.get('inputDigest') != sha(source) or data.get('requestId') != request['requestId'] \
                    or data.get('assetPath') != request['assetPath']:
                errors.append('Import receipt does not identify exact input bytes and target')
            for check in REQUIRED_IMPORT_CHECKS:
                if data.get('checks', {}).get(check, {}).get('status') != 'passed':
                    errors.append('Import check not passed: ' + check)
            if request['assetType'] == 'MaterialInstance':
                compilation = data.get('compilation', {})
                for key in ('inheritsParentShader', 'parentDirtyStatePreserved', 'resourcePresent', 'compilationFinished', 'shaderMapComplete'):
                    if compilation.get(key) is not True:
                        errors.append('Actual instance shader check failed: ' + key)
            for key in ['initialReadback', 'finalReadback', 'publishedReadback']:
                errors.extend(key + ': ' + message for message in readback_errors(request, data.get(key), function_signatures))
                readback = data.get(key)
                if case.get('requireLayout') and (not isinstance(readback, dict)
                                                 or not isinstance(readback.get('layout'), dict)):
                    errors.append(key + ': required layout report missing')
            graph_layouts[name] = layout_positions(data.get('publishedReadback'))
            if graph_layouts[name]:
                for key in ('initialReadback', 'finalReadback'):
                    if layout_positions(data.get(key)) != graph_layouts[name]:
                        errors.append(key + ': positions differ from published layout')
            layout_reference = case.get('compareLayoutWith')
            if layout_reference and (not graph_layouts.get(layout_reference)
                                     or graph_layouts[name] != graph_layouts[layout_reference]):
                errors.append('Shuffled node input changed per-ID layout: ' + layout_reference)
            graph_digests[name] = actual_graph_digest(data.get('publishedReadback'))
            equivalent = case.get('equivalentTo')
            if equivalent and (not graph_digests.get(equivalent)
                               or graph_digests[name] != graph_digests[equivalent]):
                errors.append('Shuffled nodes did not produce equivalent actual graph: ' + equivalent)
            target = project_dir / 'Content' / (request['assetPath'].removeprefix('/Game/') + '.uasset')
            if not target.is_file() or data.get('fileSha256') != sha(target):
                errors.append('Saved asset bytes missing or do not match receipt')
            if any(after_import.get(key) != value for key, value in previous_hashes.items()):
                errors.append('Import changed a previously existing test asset')
            if data.get('overallStatus') == 'waitingPersistence' and target.is_file():
                verify, reopen_step, _ = execute(name + 'Reopen', ['-NFMode=Verify', '-NFInput=' + str(source),
                                                                  '-NFPrevious=' + str(generation_report)])
                if reopen_step['exitCode'] != 0 or verify.get('overallStatus') != 'succeeded':
                    errors.append('Independent reopen did not succeed')
                if verify.get('checks', {}).get('persistence', {}).get('status') != 'passed':
                    errors.append('Independent persistence check did not pass')
                if data.get('pid') == verify.get('pid') or not data.get('pid') or not verify.get('pid'):
                    errors.append('Generation and reopen process identities are not independent')
                for key in ['reopenedReadback', 'reopenedFinalReadback']:
                    errors.extend(key + ': ' + message for message in readback_errors(request, verify.get(key), function_signatures))
                    if graph_layouts[name] and layout_positions(verify.get(key)) != graph_layouts[name]:
                        errors.append(key + ': independent reopen coordinates changed')
                if files_under(project_dir / 'Content', ASSET_SUFFIXES) != after_import:
                    errors.append('Verify changed saved asset bytes or set')
        result = {'name': name, 'category': case['category'], 'expected': case['expected'],
                  'result': 'passed' if not errors else 'failed', 'errors': errors,
                  'actualSemanticGraphSha256': graph_digests.get(name),
                  'actualLayoutPositions': graph_layouts.get(name),
                  'independentLayoutChecks': 'passed' if graph_layouts.get(name) and not errors else 'notPassedOrAbsent',
                  'assetsBefore': before_case, 'assetsAfter': files_under(project_dir / 'Content', ASSET_SUFFIXES),
                  'visual': 'notRun', 'runtime': 'notRun'}
        results.append(result)
        write_new(out / (name + 'Result.json'), result)
        print(name + ': ' + result['result'], flush=True)

    after_build = snapshot()
    after_protected = protected_assets()
    signature_reports_unchanged = all(sha(ROOT / name) == value for name, value in signature_evidence.items())
    all_passed = (len(results) == len(cases) and all(result['result'] == 'passed' for result in results)
                  and before_build == after_build and before_protected == after_protected and signature_reports_unchanged)
    summary = {'result': 'passed' if all_passed else 'failed',
               'scope': 'Selected graph import/compile/save/readback/reopen/rejection checks only',
               'fullMatrix': len(cases) == len(manifest['cases']),
               'project': ref(project), 'catalog': ref(catalog_path), 'catalogDigest': digest,
               'fixtures': ref(fixtures),
               'selectedPlugin': ref(plugin_directory), 'selectedBinarySha256': sha(loaded_binary),
               'cases': results, 'steps': steps, 'sourceAndBinaryBefore': before_build,
               'sourceAndBinaryAfter': after_build, 'oneUnchangedBuild': before_build == after_build,
               'protectedAssetsBefore': before_protected, 'protectedAssetsAfter': after_protected,
               'allProtectedAssetBytesUnchanged': before_protected == after_protected,
               'nativeParentInspections': parent_inspections,
               'nativeFunctionSignatureReports': signature_evidence,
               'nativeFunctionSignatureReportsUnchanged': signature_reports_unchanged,
               'legacyRegression': 'separate required run; not implied by this matrix',
               'visualAndAnimation': 'notRun', 'blueprintRuntime': 'notRun', 'externalAiUi': 'notRun'}
    write_new(out / 'Summary.json', summary)
    print('Summary: ' + ref(out / 'Summary.json'), flush=True)
    return 0 if all_passed else 1


if __name__ == '__main__':
    sys.exit(main())
