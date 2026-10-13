"""P2 only: complete package integrity/coverage and two saved catalogs through the same bridge.

No UE, project allocation, business submission, or repeated P1 state/fault matrix.
"""
import argparse
import asyncio
import copy
import json
import re
from pathlib import Path
from mcp import StdioServerParameters
from mcp.client import Client
from jsonschema import Draft202012Validator
from McpBridgeChecks import ROOT, Fixture, adapt_historical_report, config, read, save
from NodeForgeMcpBridge import Bridge, sha, encoded

HOME = ROOT / 'Docs/Versions/Build122/Mcp1'

def main():
    parser = argparse.ArgumentParser(); parser.add_argument('--out', required=True)
    parser.add_argument('--reuse-static', help='Preserved previous successful static cases, except changed pages/fingerprints/links')
    args = parser.parse_args()
    out = Path(args.out).resolve()
    assert out.is_relative_to(ROOT / 'Reports/PluginControlStageTwo/McpDocs1/P2') and not out.exists()
    out.mkdir(parents=True); rows = []
    def check(name, passed, detail=None):
        if args.reuse_static and name in ('CompleteCandidateCoverage', 'ExactNativeDataAndOperationProjection',
            '139ExactNodeEntriesWithHistoricalLayerBoundaries', 'SequenceConfigurationEvidenceNotPromoted'):
            original = Path(args.reuse_static).resolve() / (name + '.json')
            assert original.is_relative_to(ROOT / 'Reports/PluginControlStageTwo/McpDocs1/P2')
            old = read(original); assert old['passed']
            rows.append({'case': name, 'passed': True, 'evidenceReused': original.relative_to(ROOT).as_posix(),
                         'reason': 'Native data, coverage, evidence matching unchanged; only query example wording/fixture required fields changed'})
            return
        row = {'case': name, 'passed': bool(passed), 'detail': detail}
        rows.append(row); save(out / (name + '.json'), row)
        if not passed: raise AssertionError(name)
    manifest = read(HOME / 'Manifest.json'); index = read(HOME / 'DocumentationIndex.json')
    coverage = read(HOME / 'Coverage.json'); evidence = read(HOME / 'EvidenceIndex.json')
    catalog = read(HOME / 'Catalog.json'); ops = read(HOME / 'Operations.json')
    contract = read(ROOT / 'Tools/NodeForgeMcp/ToolContract.json')
    check('CompleteCandidateCoverage', manifest['fullUsagePackageComplete'] is True
        and manifest['publicationStatus'] == 'candidateCompleteP2'
        and manifest['actualCodexBusinessAcceptance'] == 'notRun; P3'
        and len(coverage['features']) == 14 and len(manifest['pages']) == 156
        and index['pages'] == manifest['pages']
        and set(coverage['requiredGuideIds']) == {p['pageId'] for p in manifest['pages'] if p['kind'] != 'node'})
    check('ExactNativeDataAndOperationProjection',
        catalog == read(ROOT / manifest['catalogSource']['path'])
        and ops == read(ROOT / manifest['operationsSource']['path'])
        and all(t['nativeDefinition']['argumentsSchema'] == t['inputSchema']['properties']['arguments']
            for t in contract['tools'] if t['operation'])
        and len(ops['operations']) == 13 and len(coverage['businessTools']) == 11)
    bindings = manifest['pages'] + manifest['artifacts'] + manifest['bindings'] + [manifest['catalogSource'], manifest['operationsSource']]
    mismatches = [p['path'] for p in bindings if sha((ROOT / p['path']).read_bytes()) != p['sha256'] or (ROOT / p['path']).stat().st_size != p['bytes']]
    check('AllManifestFingerprintsAndBoundedPages', not mismatches
        and len(set(p['pageId'] for p in manifest['pages'])) == len(manifest['pages'])
        and all(p['bytes'] <= 65536 for p in manifest['pages']), {'mismatches': mismatches})
    links = []; broken = []
    for p in manifest['pages']:
        path = ROOT / p['path']; text = path.read_text('utf-8')
        assert not text.startswith('\ufeff')
        for target in re.findall(r'\[[^\]]+\]\(([^)]+)\)', text):
            if re.match(r'^[A-Za-z]+://', target): continue
            dest = (path.parent / target.split('#')[0]).resolve()
            links.append({'page': p['pageId'], 'target': target})
            if not dest.is_relative_to(ROOT) or not dest.is_file(): broken.append(links[-1])
    check('LocalLinksAndUtf8', not broken, {'linksChecked': len(links), 'broken': broken})
    check('139ExactNodeEntriesWithHistoricalLayerBoundaries', len(evidence['nodes']) == 139
        and len(evidence['sources']) == 13
        and all(not n['allConfigurationsVerified'] and not n['build122RuntimeRetested']
            and n['catalogSupportState'] == 'experimental' for n in evidence['nodes'])
        and all('originalBinarySha256' in s and s['source']['sha256'] == sha((ROOT / s['source']['path']).read_bytes())
            for s in evidence['sources'])
        and all('availability' not in r and sha((ROOT / r['path']).read_bytes()) == r['sha256']
            for r in evidence['referencedReports']), {'references': len(evidence['referencedReports'])})
    # Existing untested Sequence counts must not inherit other counts' saved/runtime proof.
    untested = [n for n in evidence['nodes'] if n['identity']['class'].endswith('ExecutionSequence')
        and n['identity']['configuration']['outputCount'] in (4, 5, 6, 7)]
    check('SequenceConfigurationEvidenceNotPromoted', len(untested) == 4 and all(
        not any(r['record'].get('compileSaveIndependentReopen') or r['record'].get('runtimeSampleEvidence')
            for r in n['historicalRecords']) for n in untested))
    fixture = Fixture(out / 'Catalogs')
    adapted = []
    try:
        snapshots = read(ROOT / 'Reports/PluginControlStageTwo/McpDocs1/P0/FixtureManifest.json')['historicalCatalogs']
        prior = None
        for i, snapshot in enumerate(snapshots):
            original = ROOT / snapshot['path']; assert sha(original.read_bytes()) == snapshot['sha256']
            old = read(original)
            summary = adapt_historical_report(fixture, 'ClientCatalog', f'P2Catalog{i+1}')
            payload = summary['response']['result']['result']; path = Path(payload['nativeReport']['path'])
            native = read(path); native['catalog'] = old['catalog']; native['catalogDigest'] = old['catalogDigest']
            path.write_bytes(encoded(native)); payload['nativeReport'].update(sha256=sha(path.read_bytes()), bytes=path.stat().st_size)
            payload['catalogDigest'] = old['catalogDigest']
            sidecar_path = Path(payload['controlEvidencePath']); sidecar = read(sidecar_path)
            sidecar['nativeReport'] = copy.deepcopy(payload['nativeReport']); sidecar['catalogDigest'] = payload['catalogDigest']
            sidecar_path.write_bytes(encoded(sidecar)); payload['controlEvidenceSha256'] = sha(sidecar_path.read_bytes())
            fixture.bridge._cli = lambda *a, **kw: summary
            value = fixture.gt(f'P2Catalog{i+1}', operation='catalog.get', revision='0')
            save(out / f'Catalog{i+1}Projection.json', value)
            assert value['status'] == 'finished' and len(value['reports']) == 1
            selected = fixture.bridge.call('nodeforge_report_read', {'handle': fixture.handle,
                'reportId': value['reports'][0]['reportId'], 'pointer': '/catalog/blueprint/nodes/0'})
            save(out / f'Catalog{i+1}NodeSelection.json', selected)
            assert selected['status'] == 'passed' and selected['value'] == old['catalog']['blueprint']['nodes'][0]
            key = (manifest['bundleId'], manifest['binarySha256'], old['catalogDigest'])
            adapted.append({'source': snapshot, 'cacheKey': key, 'counts': {
                k: len(old['catalog'][k]['nodes']) for k in ('material', 'blueprint')},
                'toolContractSha256': sha((ROOT / 'Tools/NodeForgeMcp/ToolContract.json').read_bytes()),
                'reportedCatalogSummary': value['reports'][0]['catalogSummary'],
                'reportId': value['reports'][0]['reportId'], 'cacheInvalidated': prior is not None and prior != key})
            prior = key
        before, after = (read(ROOT / s['path']) for s in snapshots)
        def native_keys(d):
            return {json.dumps({'class': n['class'], 'properties': n.get('properties'), 'variableType': n.get('variableType')}, sort_keys=True)
                for family in ('material', 'blueprint') for n in d['catalog'][family]['nodes']}
        new = sorted(native_keys(after) - native_keys(before))
        check('SameBridgeToolsDiscoverChangedCatalogAndInvalidateCache', adapted[0]['counts'] == {'material': 44, 'blueprint': 76}
            and adapted[1]['counts'] == {'material': 48, 'blueprint': 91} and adapted[1]['cacheInvalidated']
            and adapted[0]['toolContractSha256'] == adapted[1]['toolContractSha256']
            and adapted[0]['reportId'] != adapted[1]['reportId'] and bool(new),
            {'adaptation': adapted, 'newNativeEntries': [json.loads(n) for n in new],
             'scope': 'Copied saved snapshots through actual bridge; no future UE-node/runtime claim'})
    finally: fixture.close()
    async def sdk():
        conf = config(out / 'Sdk', out / 'NoLiveSession')
        async with Client(StdioServerParameters(command=__import__('sys').executable,
            args=[str(ROOT / 'Scripts/NodeForgeMcp.py'), '--config', str(conf)], cwd=str(ROOT)), read_timeout_seconds=30) as client:
            result = await client.call_tool('nodeforge_docs_index', {})
            value = result.structured_content
            assert not result.is_error and value['totalPages'] == 156 and not value['nodePagesListed']
            assert len(value['manifest']['pages']) == 17 and len(encoded(value)) < 32768
            loaded = []
            for entry in value['manifest']['pages']:
                response = await client.call_tool('nodeforge_docs_read', {'pageId': entry['pageId']})
                data = response.structured_content
                assert not response.is_error and sha(data['text'].encode('utf-8')) == entry['sha256']
                loaded.append({'pageId': entry['pageId'], 'sha256': data['sha256']})
            node = await client.call_tool('nodeforge_docs_read', {'pageId': 'BlueprintNode001'})
            assert not node.is_error and 'K2Node_CallFunction' in node.structured_content['text']
            save(out / 'SdkDocuments.json', {'protocolVersion': client.protocol_version,
                'index': value, 'guidesLoaded': loaded, 'selectedNodePage': node.structured_content,
                'scope': 'Official SDK stdio documentation calls, not actual Codex/UE business'})
    asyncio.run(sdk())
    check('BoundedSdkIndexAllGuidesAndNodePage', True, 'SdkDocuments.json')
    baseline = read(ROOT / 'Reports/PluginControlStageTwo/McpDocs1/P0/Baseline.json')
    changed = [p for p, expected in baseline['protectedFiles'].items() if sha((ROOT / p).read_bytes()) != expected]
    check('OriginalProtectedFilesUnchanged', not changed, {'count': len(baseline['protectedFiles']), 'changed': changed})
    save(out / 'Review.json', {'stepId': 'P2', 'status': 'passed', 'canAdvance': True,
        'planSha256': baseline['planSha256'], 'checks': rows, 'manifestSha256': sha((HOME / 'Manifest.json').read_bytes()),
        'changedBridgeScope': 'docs_index returns bounded guides/family indices; business logic unchanged',
        'ue': 'notRun', 'projectAllocated': False, 'actualCodexBusiness': 'notRun; P3'})
    print(json.dumps({'stepId': 'P2', 'status': 'passed', 'checks': len(rows), 'ue': 'notRun'}))

if __name__ == '__main__': main()
