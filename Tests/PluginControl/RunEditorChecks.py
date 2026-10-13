"""Finite second-stage checks using the real plugin and existing process/asset guards."""
from __future__ import annotations
import argparse
import json
import re
import sys
import subprocess
import time
import os
import unicodedata
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'Scripts'))
from RunGraphRegression import ASSET_SUFFIXES, ENGINE, build_snapshot, files_under, ref, sha, write_new
from RunProcessGuard import run_guarded, validate_isolated_project

def allocate_control_batch(ledger, profile=None):
    """Pure allocation; persist once before any submission. Never rename a retry."""
    if ledger.get('version') != 1 or ledger.get('activeRun') is not None:
        raise ValueError('Unresolved/unknown run; inspect original evidence before reuse')
    number = ledger.get('nextBatch')
    if type(number) is not int or not 1 <= number <= 999999:
        raise ValueError('Invalid/exhausted six-digit batch counter')
    batch = f'B{number:06d}'
    if profile == 'BlueprintTick':
        return {'batch': batch, 'nextBatch': number + 1, 'identities': [
            {'case': 'Blueprint', 'requestId': f'NFA{number:06d}TickReq',
             'assetPath': f'/Game/NodeForgeTests/BlueprintTick/{batch}/BPTickMove'}],
            'auxiliaryMap': f'/Game/NodeForgeTests/BlueprintTick/{batch}/TickMap',
            'auxiliaryMapReason': 'Original passive PIE observer needs a saved map referencing the one saved Actor class'}
    if profile == 'AssetSettings':
        return {'batch': batch, 'nextBatch': number + 1, 'identities': [
            {'case': kind, 'requestId': f'NFA{number:06d}{short}Req',
             'assetPath': f'/Game/NodeForgeTests/AssetSettings/{batch}/{name}'}
            for kind, short, name in (('Material', 'Mat', 'MASMask'), ('Blueprint', 'Bp', 'BPASEdit'))]}
    if profile == 'PersonalCandidate':
        return {'batch': batch, 'nextBatch': number + 1, 'identities': [
            {'case': 'Material', 'requestId': f'NFA{number:06d}ResourceReq',
             'assetPath': f'/Game/NodeForgeTests/Candidate/{batch}/MResourceMask'}]}
    if profile is not None:
        raise ValueError('Unknown finite allocation profile')
    return {'batch': batch, 'nextBatch': number + 1, 'identities': [
        {'case': kind, 'requestId': f'NFA{number:06d}{short}Req',
         'assetPath': f'/Game/NodeForgeTests/{kind}/{batch}/Sample01'}
        for kind, short in (('Material', 'Mat'), ('Blueprint', 'Bp'))]}

class ReusableEditorProject:
    """One registered mutable project; old phase fresh-project guards stay intact."""
    def __init__(self, name, stage, out, resume_unsubmitted=None, allocation_profile=None):
        from NodeForgeControl import assert_exclusive
        from NodeForgeEditorTransport import Windows
        if (unicodedata.normalize('NFC', name) != name or not name.isascii()
                or not name.isalnum() or not name.startswith('NFC') or not 1 <= len(name) <= 20):
            raise ValueError('NFC-prefixed ASCII alphanumeric UE project name, maximum 20 characters required')
        self.folder = ROOT / 'ValidationProject/RegressionRuns' / name
        self.project = self.folder / (name + '.uproject')
        self.stage = stage.resolve(); self.out = out.resolve()
        if (not self.stage.is_relative_to(ROOT / 'Reports')
                or not self.out.is_relative_to(ROOT / 'Reports/PluginControlStageTwo') or self.out.exists()):
            raise ValueError('Explicit Reports stage and exclusive new control run required')
        self.ledger_path = self.folder / 'Saved/NodeForge/ControlRuns.json'
        self.lease_path = self.ledger_path.with_name('ControlRun.lock')
        windows = Windows()
        windows.no_reparse(self.folder); windows.no_reparse(self.ledger_path); windows.no_reparse(self.lease_path)
        if self.folder.exists():
            if not self.ledger_path.is_file():
                raise ValueError('Existing unregistered project protected; no adoption or cleanup')
            assert_exclusive(self.project)
            ledger = json.loads(self.ledger_path.read_text('utf-8'))
            if ledger.get('project') != self.project.as_posix() or ledger.get('projectName') != name:
                raise ValueError('Project ledger identity conflict')
            # PrepareNodeForgeInstallation owns the current explicit binding.
            # The ledger's old stage is historical; do not rewrite the descriptor here.
            validate_isolated_project(self.project, self.stage, ROOT)
            if ledger.get('assets') != files_under(self.folder / 'Content'):
                raise ValueError('Previously saved content changed or unaccounted assets exist')
            if ledger.get('requests') != files_under(self.folder / 'Saved/NodeForge/Requests'):
                raise ValueError('Previously saved request receipts changed; preserve them')
        else:
            if not (self.stage / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll').is_file():
                raise ValueError('Compiled candidate stage required before project initialization')
            self.folder.mkdir(parents=True)
            write_new(self.project, {'FileVersion': 3, 'EngineAssociation': '5.3',
                'AdditionalPluginDirectories': [str(self.stage.parent)],
                'Plugins': [{'Name': 'NodeForge', 'Enabled': True}]})
            self.ledger_path.parent.mkdir(parents=True, exist_ok=True)
            ledger = {'version': 1, 'project': self.project.as_posix(), 'projectName': name,
                'pluginDirectory': self.stage.as_posix(), 'nextBatch': 1, 'activeRun': None,
                'assets': {}, 'requests': {}, 'runs': []}
            write_new(self.ledger_path, ledger)
        recovery = None
        if resume_unsubmitted is not None:
            from RunProcessGuard import process_identity
            old = resume_unsubmitted.resolve()
            active = ledger.get('activeRun')
            if (not old.is_relative_to(ROOT / 'Reports/PluginControlStageTwo') or not active
                    or active.get('runDirectory') != old.as_posix()):
                raise ValueError('Explicit original allocated run required for recovery')
            diagnosis = json.loads((old / 'OwnershipDiagnosis.json').read_text('utf-8'))
            native = json.loads((old / 'Identity.json').read_text('utf-8-sig'))
            stop = json.loads((old / 'StopReceipt.json').read_text('utf-8-sig'))
            prep = json.loads((old / 'Preparation.json').read_text('utf-8'))
            if (diagnosis.get('businessSubmissions') != 0 or diagnosis.get('operationStatus') != 'confirmedFixtureError'
                    or process_identity(diagnosis['pid']).get('state') != 'absent'
                    or native.get('pid') != diagnosis['pid'] or native.get('exitRequest') != 'normalOwnedEditorAfterChannelStop'
                    or stop.get('state') != 'stopped' or not stop.get('descriptorDeleted')
                    or any(not x.get('threadDone') or x.get('pendingIo') for x in stop.get('workers', []))
                    or list((old / 'Calls').glob('*'))
                    or files_under(self.folder / 'Content') != prep['assetsBefore']
                    or files_under(self.folder / 'Saved/NodeForge/Requests') != prep['requestsBefore']):
                raise ValueError('Recovery cannot prove normal exit/no submissions/no writes; preserve unresolved run')
            lock = json.loads(self.lease_path.read_text('utf-8'))
            if lock.get('runDirectory') != old.as_posix():
                raise ValueError('Original run lease owner conflict')
            # Archive terminal failure; retain exactly the original allocated identities/counter.
            recovery = {'originalRun': old.as_posix(), 'diagnosis': diagnosis, 'lease': lock}
            allocation = {'batch': active['batch'], 'nextBatch': ledger['nextBatch'], 'identities': active['identities']}
        else:
            allocation = allocate_control_batch(ledger, allocation_profile)
        ledger_before = json.loads(json.dumps(ledger))
        assert_exclusive(self.project)
        self.out.mkdir(parents=True)
        if recovery is not None:
            write_new(self.out / 'UnsubmittedRecovery.json', recovery)
            ledger['runs'].append({**ledger['activeRun'], 'state': 'failedBeforeBusinessSubmission',
                'recoveryEvidence': (self.out / 'UnsubmittedRecovery.json').as_posix()})
            self.lease_path.unlink()  # Verified absent original process and this exact lease only.
        # O_EXCL is the serial run lease; do not delete an existing lock on failure.
        with self.lease_path.open('x', encoding='utf-8') as stream:
            json.dump({'runDirectory': self.out.as_posix(), 'runnerPid': os.getpid()}, stream)
        self.ledger = ledger
        write_new(self.out / 'LedgerBefore.json', ledger_before)
        write_new(self.out / 'ProjectBefore.json', json.loads(self.project.read_text('utf-8')))
        self.project, self.stage, self.binary = validate_isolated_project(self.project, self.stage, ROOT)
        self.allocation = allocation
        for item in allocation['identities']:
            target = self.folder / 'Content' / (item['assetPath'].removeprefix('/Game/') + '.uasset')
            receipt = self.folder / 'Saved/NodeForge/Requests' / (item['requestId'] + '.json')
            if target.exists() or receipt.exists() or len(item['requestId']) > 32:
                raise ValueError('Allocated identity already occupied; no alternate allocation')
        ledger.update(nextBatch=allocation['nextBatch'], pluginDirectory=self.stage.as_posix(),
            activeRun={'batch': allocation['batch'], 'runDirectory': self.out.as_posix(),
                'state': 'allocated', 'identities': allocation['identities']})
        self.save()
        write_new(self.out / 'IdentityAllocation.json', allocation)

    @staticmethod
    def _replace_json(path, data):
        pending = path.with_name(path.name + '.pending')
        with pending.open('x', encoding='utf-8', newline='\n') as stream:
            json.dump(data, stream, ensure_ascii=False, indent=2); stream.write('\n')
            stream.flush(); os.fsync(stream.fileno())
        os.replace(pending, path)

    def save(self):
        self._replace_json(self.ledger_path, self.ledger)

    def checkpoint(self, state, **evidence):
        self.ledger['activeRun'].update(state=state, **evidence); self.save()

    def register_prepared_identity(self, case, prepare_call, draft):
        """Record a committed product Prepare in our run; never rename or finish it.

        Both arguments are original nativeSummary.response.result values: the
        draft.prepareCopy call record and the complete subsequent draft.get.
        """
        from hashlib import sha256
        active = self.ledger.get('activeRun')
        if (not isinstance(active, dict) or active.get('runDirectory') != self.out.as_posix()
                or active.get('batch') != self.allocation['batch']
                or active.get('identities') != self.allocation['identities']
                or self.ledger.get('project') != self.project.as_posix()
                or self.ledger.get('pluginDirectory') != self.stage.as_posix()
                or json.loads(self.ledger_path.read_text('utf-8')) != self.ledger
                or json.loads(self.lease_path.read_text('utf-8')) !=
                    {'runDirectory': self.out.as_posix(), 'runnerPid': os.getpid()}):
            raise ValueError('Current allocated run and owned serial lease required')
        seeds = [x for x in active['identities'] if x.get('case') == case]
        recorded = active.get('preparedIdentities', [])
        if (case not in ('Material', 'Blueprint') or len(seeds) != 1
                or not isinstance(recorded, list) or any(x.get('case') == case for x in recorded)
                or not isinstance(prepare_call, dict) or not isinstance(draft, dict)):
            raise ValueError('One unrecorded Material or Blueprint preparation required')
        seed = seeds[0]
        payload = prepare_call.get('result', {})
        if not isinstance(payload, dict):
            raise ValueError('Original native preparation payload required')
        call_id = prepare_call.get('callId', '')
        previous = payload.get('previousDraftRevision', '')
        current = payload.get('newDraftRevision', '')
        if (prepare_call.get('operation') != 'draft.prepareCopy' or prepare_call.get('state') != 'completed'
                or payload.get('state') != 'completed'
                or type(payload.get('cliExitCode')) is not int or payload['cliExitCode'] != 0
                or payload.get('draftApplied') is not True
                or not isinstance(call_id, str) or not re.fullmatch(r'[A-Za-z0-9-]{1,64}', call_id)
                or not isinstance(previous, str) or not re.fullmatch(r'0|[1-9][0-9]*', previous)
                or not isinstance(current, str) or not re.fullmatch(r'0|[1-9][0-9]*', current)
                or not 0 <= int(previous) < 18446744073709551615
                or int(current) != int(previous) + 1
                or prepare_call.get('expectedDraftRevision') != previous
                or draft.get('draftReadable') is not True or draft.get('draftRevision') != current
                or not isinstance(draft.get('text'), str)):
            raise ValueError('Successful applied Prepare and complete current draft required')
        metadata = {k: v for k, v in draft.items() if k != 'text'}
        if payload.get('draft') != metadata:
            raise ValueError('Prepare metadata and current full draft conflict')
        preparation = payload.get('copyPreparation')
        reference = payload.get('nativeReport', {})
        if not isinstance(preparation, dict) or not isinstance(reference, dict):
            raise ValueError('Original preparation and native report reference required')
        native_path = self.out/'Calls'/(call_id+'Native.json')
        snapshot = self.out/'Calls'/(call_id+'Input.json')
        sidecar_path = self.out/'Calls'/(call_id+'Control.json')
        actual = {'requestId': preparation.get('newRequestId'), 'assetPath': preparation.get('newAssetPath')}
        if (not isinstance(actual['requestId'], str)
                or not re.fullmatch(r'[A-Za-z0-9]{1,64}', actual['requestId'])
                or not isinstance(actual['assetPath'], str)
                or actual['assetPath'].rsplit('/', 1)[0] != seed['assetPath'].rsplit('/', 1)[0]
                or not re.fullmatch(r'[A-Za-z0-9]{1,64}', actual['assetPath'].rsplit('/', 1)[-1])
                or actual['requestId'] == seed['requestId'] or actual['assetPath'] == seed['assetPath']
                or preparation.get('originalRequestId') != seed['requestId']
                or preparation.get('originalAssetPath') != seed['assetPath']
                or preparation.get('sourceInputUnchanged') is not True
                or preparation.get('targetReserved') is not False or preparation.get('assetGenerated') is not False
                or set(preparation.get('changedFields', [])) != {'assetPath', 'requestId'}):
            raise ValueError('Exact allocated-to-actual identity-only preparation required')
        prepared_path = ROOT/'Reports/PreparedCopies'/(actual['requestId']+'.json')
        paths = [(reference.get('path'), native_path), (payload.get('inputSnapshot'), snapshot),
            (preparation.get('sourceInput'), snapshot), (payload.get('controlEvidencePath'), sidecar_path),
            (preparation.get('preparedInputPath'), prepared_path), (draft.get('copyPreparationReport'), native_path)]
        if any(not isinstance(value, str) or Path(value).resolve() != expected.resolve() for value, expected in paths):
            raise ValueError('Original files must bind this run and product prepared input')

        def read_bound(path, digest, limit):
            raw = path.read_bytes()
            if (not isinstance(digest, str) or not re.fullmatch(r'[0-9a-f]{64}', digest)
                    or len(raw) > limit or sha256(raw).hexdigest() != digest):
                raise ValueError('Original preparation evidence SHA or size conflict')
            value = json.loads(raw.decode('utf-8-sig'))
            if not isinstance(value, dict):
                raise ValueError('Original preparation evidence must be an object')
            return value, raw

        native, native_raw = read_bound(native_path, reference.get('sha256'), 1048576)
        original, _ = read_bound(snapshot, payload.get('inputDigest'), 65536)
        prepared, prepared_raw = read_bound(prepared_path, preparation.get('preparedInputDigest'), 65536)
        sidecar, _ = read_bound(sidecar_path, payload.get('controlEvidenceSha256'), 1048576)
        identity = json.loads((self.out/'SessionPublic.json').read_text('utf-8-sig'))
        if (identity.get('project') != self.project.as_posix() or identity.get('binarySha256') != sha(self.binary)
                or native.get('project') != identity['project'] or native.get('pid') != identity.get('pid')
                or native.get('mode') != 'PrepareNewCopy' or native.get('inputMethod') != 'editorProgram'
                or native.get('operationStatus') != 'passed' or native.get('overallStatus') != 'succeeded'
                or native.get('copyPreparation') != preparation or native.get('inputDigest') != payload['inputDigest']
                or reference.get('bytes') != len(native_raw)
                or reference.get('nativeSessionId') != native.get('sessionId')
                or sidecar.get('controlSessionId') != identity.get('controlSessionId')
                or prepare_call.get('controlSessionId') != identity.get('controlSessionId')
                or sidecar.get('binarySha256') != identity['binarySha256']
                or sidecar.get('callId') != call_id or sidecar.get('operation') != 'draft.prepareCopy'
                or sidecar.get('draftRevision') != previous or sidecar.get('newDraftRevision') != current
                or sidecar.get('draftApplied') is not True or sidecar.get('draft') != metadata
                or sidecar.get('nativeReport') != reference
                or sidecar.get('snapshotPath') != payload['inputSnapshot']
                or sidecar.get('snapshotDigest') != payload['inputDigest']):
            raise ValueError('Prepare report, sidecar and current session identity conflict')
        if (preparation.get('sourceInputDigest') != payload['inputDigest']
                or draft.get('digest') != preparation['preparedInputDigest']
                or draft['text'].encode('utf-8') != prepared_raw
                or draft.get('bytes') != len(prepared_raw)
                or any(x.get('format') != 'NodeForgeGraph' or type(x.get('version')) is not int
                    or x['version'] != 2 or x.get('assetType') != case for x in (original, prepared))
                or original.get('requestId') != seed['requestId'] or original.get('assetPath') != seed['assetPath']
                or prepared.get('requestId') != actual['requestId'] or prepared.get('assetPath') != actual['assetPath']):
            raise ValueError('Original and complete prepared graph identity or bytes conflict')
        identity_only = {**prepared, 'requestId': seed['requestId'], 'assetPath': seed['assetPath']}
        # UE JSON numbers share one type; booleans must never compare equal to 0/1.
        def graph_value(value):
            normalized = json.loads(json.dumps(value, allow_nan=False), parse_int=float)
            return json.dumps(normalized, sort_keys=True, separators=(',', ':'), allow_nan=False)
        if graph_value(identity_only) != graph_value(original):
            raise ValueError('Prepare changed graph data beyond requestId and assetPath')
        if (any(x['actual']['requestId'] == actual['requestId'] or x['actual']['assetPath'] == actual['assetPath']
                for x in recorded)
                or any(x['case'] != case and (x['requestId'] == actual['requestId']
                    or x['assetPath'] == actual['assetPath']) for x in active['identities'])
                or (self.folder/'Content'/(actual['assetPath'].removeprefix('/Game/')+'.uasset')).exists()
                or (self.folder/'Saved/NodeForge/Requests'/(actual['requestId']+'.json')).exists()):
            raise ValueError('Prepared actual identity is duplicated or already occupied')
        row = {'case': case, 'allocated': dict(seed), 'actual': actual, 'prepareCallId': call_id,
            'controlSessionId': identity['controlSessionId'], 'originalInput': ref(snapshot),
            'originalInputSha256': preparation['sourceInputDigest'], 'preparedInput': ref(prepared_path),
            'preparedInputSha256': preparation['preparedInputDigest'], 'prepareReport': ref(native_path),
            'prepareReportSha256': reference['sha256'], 'controlEvidence': ref(sidecar_path),
            'controlEvidenceSha256': payload['controlEvidenceSha256'], 'previousDraftRevision': previous,
            'draftRevision': current, 'draftApplied': True, 'exportOrigin': draft.get('exportOrigin')}
        # Validate before the only write; preserve allocated seed, counter and finish semantics.
        updated = json.loads(json.dumps(self.ledger))
        updated['activeRun'].update(state='prepared', preparedIdentities=[*recorded, row])
        self._replace_json(self.ledger_path, updated)
        self.ledger = updated
        return json.loads(json.dumps(row))

    def finish(self, evidence):
        from NodeForgeControl import assert_exclusive
        assert_exclusive(self.project)
        run = dict(self.ledger['activeRun']); run.update(state='passed', evidence=evidence)
        self.ledger['runs'].append(run); self.ledger['activeRun'] = None
        self.ledger['assets'] = files_under(self.folder / 'Content')
        self.ledger['requests'] = files_under(self.folder / 'Saved/NodeForge/Requests')
        self.save()
        write_new(self.out / 'LedgerAfter.json', self.ledger)
        lock = json.loads(self.lease_path.read_text('utf-8'))
        if lock != {'runDirectory': self.out.as_posix(), 'runnerPid': os.getpid()}:
            raise ValueError('Serial lease owner changed; preserve lock')
        self.lease_path.unlink()  # Only our verified lease, never assets or request receipts.

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--phase', choices=['State', 'Ui', 'Protocol', 'Identity', 'Draft', 'ExternalUi', 'Boundary','LifeConnect','LifeRead','LifeWrite','LifeQueued','LifeRunning','DisabledUi','Preflight','Graph','AssetBoundary','GuiGraph','GuiBusiness','GuiAux','RulesCatalog'], required=True)
    parser.add_argument('--project-name', required=True)
    parser.add_argument('--plugin-directory', required=True)
    parser.add_argument('--report-directory', required=True)
    parser.add_argument('--resume-unsubmitted-run', help='RulesCatalog only: explicit no-submission terminal fixture recovery; identities preserved')
    parser.add_argument('--asset-protection', action='append', default=[],
        help='Additional saved-asset protection report; S3 saved assets are always protected')
    args = parser.parse_args()
    if args.resume_unsubmitted_run and args.phase != 'RulesCatalog':
        raise SystemExit('Recovery is limited to the finite RulesCatalog phase')
    if args.phase == 'RulesCatalog':
        from EditorQueryCases import run_stage
        return run_stage(args)
    if not args.project_name.isascii() or not args.project_name.isalnum():
        raise SystemExit('Fresh ASCII alphanumeric project name required')
    if len(args.project_name) > 20:
        raise SystemExit('UE project name must be at most 20 characters; nothing created')
    if args.phase in ('Ui', 'ExternalUi','DisabledUi') and not args.project_name.startswith(('NFCUi', 'ControlEditorUiState')):
        raise SystemExit('Owned NFCUi project name required for real UI checks')
    external_ui = args.phase in ('ExternalUi','DisabledUi')
    graph_phase = args.phase == 'Graph'
    boundary_phase = args.phase == 'AssetBoundary'
    gui_graph_phase = args.phase == 'GuiGraph'
    business_phase = args.phase == 'GuiBusiness'
    aux_phase = args.phase == 'GuiAux'
    asset_phase = graph_phase or boundary_phase or gui_graph_phase or business_phase or aux_phase
    channel_phase = args.phase in ('Identity','Draft','Boundary','LifeConnect','LifeRead','LifeWrite','LifeQueued','LifeRunning','Preflight','Graph','AssetBoundary','GuiGraph','GuiBusiness','GuiAux') or external_ui
    if channel_phase and not external_ui and not args.project_name.startswith(('NFC' + args.phase, 'ControlEditor' + args.phase)):
        raise SystemExit('Fresh registered NFC' + args.phase + ' project required')
    stage = (ROOT / args.plugin_directory).resolve()
    out = (ROOT / args.report_directory).resolve()
    if not out.is_relative_to(ROOT / 'Reports/PluginControlStageTwo'):
        raise SystemExit('New second-stage report directory required')
    folder = ROOT / 'ValidationProject/RegressionRuns' / args.project_name
    if out.exists() or folder.exists():
        raise SystemExit('Existing reports/projects protected; do not retry in place')
    if not stage.is_relative_to(ROOT / 'Reports') or not (stage / 'NodeForge.uplugin').is_file():
        raise SystemExit('Explicit already compiled Reports stage required')
    baseline_path = ROOT / 'Reports/PluginControlStageTwo/EditorControl1/S0/Baseline.json'
    protected = dict(json.loads(baseline_path.read_text('utf-8-sig'))['protectionBefore'])
    protection_paths = [ROOT / 'Reports/PluginControlStageTwo/EditorControl1/S3/SavedAssetProtection105.json',
        *((ROOT / path).resolve() for path in args.asset_protection)]
    for protection_path in protection_paths:
        if not protection_path.is_relative_to(ROOT / 'Reports/PluginControlStageTwo'):
            raise SystemExit('Saved-asset protection report must be inside second-stage Reports')
        assets = json.loads(protection_path.read_text('utf-8-sig'))
        if not isinstance(assets, dict) or not assets:
            raise SystemExit('Nonempty saved-asset protection map required')
        for path, digest in assets.items():
            asset = (ROOT / path).resolve()
            if (Path(path).is_absolute() or not asset.is_relative_to(ROOT / 'ValidationProject/RegressionRuns')
                    or asset.suffix not in ASSET_SUFFIXES or 'Content' not in asset.parts
                    or not isinstance(digest, str) or len(digest) != 64
                    or any(char not in '0123456789abcdef' for char in digest)):
                raise SystemExit('Invalid saved-asset protection entry')
            if path in protected and protected[path] != digest:
                raise SystemExit('Conflicting asset protection; preserve evidence and stop')
            protected[path] = digest
    if any(not (ROOT / path).is_file() or sha(ROOT / path) != digest for path, digest in protected.items()):
        raise SystemExit('Protected bytes changed; nothing created')
    out.mkdir(parents=True)
    folder.mkdir()
    project = folder / (args.project_name + '.uproject')
    write_new(project, {'FileVersion': 3, 'EngineAssociation': '5.3',
        'AdditionalPluginDirectories': [str(stage.parent)],
        'Plugins': [{'Name': 'NodeForge', 'Enabled': True}]})
    project, stage, binary = validate_isolated_project(project, stage, ROOT)
    frozen = build_snapshot(stage)
    def asset_snapshot():
        return files_under(folder / 'Content', None if asset_phase else ASSET_SUFFIXES)
    before_assets = asset_snapshot()
    owned_assets = dict(before_assets)
    allowed_asset = None
    allowed_relayout = False
    graph_results = []
    graph_attempted = []
    verify_results = []
    script_frozen = {path:sha(ROOT/path) for path in ('Scripts/NodeForgeControl.py','Scripts/NodeForgeEditorTransport.py',
        'Scripts/RunProcessGuard.py','Scripts/RunGraphRegression.py','Tests/PluginControl/RunCases.py','Tests/PluginControl/RunEditorChecks.py',
        'Tests/PluginControl/EditorGraphBoundaryCases.py','Tests/PluginControl/EditorGuiGraphCases.py',
        'Tests/PluginControl/EditorBusinessUiCases.py', 'Tests/PluginControl/EditorAuxUiCases.py',
        'Tests/PluginControl/EditorClipboardMemory.py')}
    write_new(out / 'Preparation.json', {'phase': args.phase, 'project': ref(project),
        'stage': ref(stage), 'moduleSha256': sha(binary), 'sourceAndBinary': frozen,
        'clientAndRunnerSources':script_frozen,
        'assetsBefore': before_assets, 'protectionBaseline': ref(baseline_path),
        'savedAssetProtectionReports': [ref(path) for path in protection_paths],
        'projectName': args.project_name, 'projectNameLength': len(args.project_name),
        'scope': ('Fixed actual production UI copy/export/preview/relayout and representative refusals; normal exit/three original CLI Verify' if business_phase
            else 'Formal load/preflight then actual production GUI Import of two graphs; normal exit/original CLI Verify' if gui_graph_phase
            else 'Real production graph refusals/accepted Import disconnect and original CLI new-PID Verify; no GUI proof' if boundary_phase
            else 'Formal editor CLI two-graph generation/actual production UI; normal exit then original CLI Verify' if graph_phase
            else 'Real production tab and Automation Driver; no IPC or asset generation' if args.phase == 'Ui'
            else 'Native protocol/state checks; no real UI/IPC or asset generation')})

    def guard():
        changed = [path for path, digest in protected.items()
            if not (ROOT / path).is_file() or sha(ROOT / path) != digest]
        changed += [path for path,digest in script_frozen.items() if not (ROOT/path).is_file() or sha(ROOT/path)!=digest]
        actual = asset_snapshot()
        extra_assets = set(actual) - set(owned_assets)
        relayout_allowed = set()
        if allowed_relayout and len(extra_assets) == 1:
            pattern = re.escape(ref(folder / 'Content/NodeForgeTests')) + r'/GraphLayout[0-9A-Fa-f]{32}\.uasset'
            relayout_allowed = {path for path in extra_assets if re.fullmatch(pattern, path)}
        asset_changed = (any(actual.get(path) != digest for path, digest in owned_assets.items())
            or bool(extra_assets - ({allowed_asset} if allowed_asset else set()) - relayout_allowed)) if asset_phase else actual != before_assets
        if changed or build_snapshot(stage) != frozen or asset_changed:
            raise SystemExit('Protection/build/asset changed; stop before next UE process: ' + repr(changed))

    cases = [('SharedState', 'NodeForge.EditorSharedState', 'NFEditorStateReport'),
        ('PanelInput', 'NodeForge.PanelInputGuards', 'NFPanelInputReport'),
        ('ResultView', 'NodeForge.ResultView', 'NFReport'),
        ('CompactResults', 'NodeForge.CompactResults', 'NFReport')]
    if args.phase == 'State':
        cases.append(('NamingCopies', 'NodeForge.NamingCopies', 'NFNamingReport'))
    if args.phase in ('Ui', 'ExternalUi','DisabledUi'):
        cases = [('ProductionUi', 'NodeForge.EditorProductionUi', 'NFEditorUiReport')]
    elif args.phase == 'Protocol':
        cases = [('Protocol', 'NodeForge.EditorControlProtocol', 'NFEditorProtocolReport'),
            ('SharedState', 'NodeForge.EditorSharedState', 'NFEditorStateReport')]
    elif channel_phase:
        cases = [('Identity', 'NodeForge.EditorControlIdentity', 'NFEditorControlReport')]
    if channel_phase:
        write_new(out / 'Registration.json', {'project': project.as_posix(), 'pluginDirectory': stage.as_posix(),
            'binarySha256': sha(binary), 'reportDirectory': out.as_posix()})
    steps = []
    for name, test, report_arg in cases:
        guard()
        report = out / (name + '.json')
        engine = ENGINE.with_name('UnrealEditor.exe') if args.phase == 'Ui' or channel_phase else ENGINE
        rendering = ['-windowed', '-ResX=1280', '-ResY=900', '-NFEditorUiChecks'] if args.phase == 'Ui' else ['-RenderOffscreen']
        if channel_phase:
            rendering = ['-windowed', '-ResX=1280', '-ResY=900', '-NFControlEnable', '-NFControlRegistration=' + str(out / 'Registration.json')]
            if external_ui:
                rendering += ['-NFEditorUiChecks', '-NFEditorControlUiChecks']
                if args.phase=='DisabledUi':rendering+=['-NFEditorControlDisableUiChecks']
            if args.phase in ('LifeQueued','LifeRunning'):
                rendering+=['-NFEditorControl'+('Queued' if args.phase=='LifeQueued' else 'Running')+'StopChecks']
            if graph_phase:
                rendering += ['-NFEditorControlGraphUiChecks']
            if gui_graph_phase:
                rendering += ['-NFEditorControlGuiGenerateChecks']
            if business_phase:
                rendering += ['-NFEditorControlBusinessUiChecks']
            if aux_phase:
                rendering += ['-NFEditorControlAuxUiChecks']
            if boundary_phase:
                rendering += ['-NFEditorControlAssetBoundaryChecks']
        command = [str(engine), str(project), '-ExecCmds=Automation RunTests ' + test + '; Quit',
            '-TestExit=Automation Test Queue Empty', '-' + report_arg + '=' + str(report),
            '-d3d11', *rendering, '-unattended', '-nop4', '-nosound', '-NoLiveCoding',
            '-abslog=' + str(out / (name + '.log')), '-stdout', '-FullStdOutLogOutput']
        if channel_phase:
            command[2] = '-ExecCmds=Automation RunTests ' + test
            command.remove('-TestExit=Automation Test Queue Empty')
        observed_clients = False
        def observe_identity(process):
            nonlocal observed_clients, allowed_asset
            ready_path = out / 'ClientReady.json'
            if observed_clients or not ready_path.is_file(): return
            try:
                ready = json.loads(ready_path.read_text('utf-8-sig'))
            except (OSError, json.JSONDecodeError):
                return  # Observe the same owned publication; never dispatch from a partial marker.
            observed_clients = True
            from NodeForgeEditorTransport import Session, Windows, TransportError
            import ctypes
            from types import SimpleNamespace
            identity = ready['identity']
            rows = []
            def row(case, passed, observed=None):
                rows.append({'case':case,'passed':bool(passed),'observed':observed})
            def client(case, action, extra=None, expected=0, selected_stage=stage, call_timeout=30):
                run = out / ('Client' + case)
                cmd = [sys.executable,str(ROOT / 'Scripts/NodeForgeControl.py'),action,'--session-file',identity['sessionFile'],
                    '--project',str(project),'--plugin-directory',str(selected_stage),'--run-directory',str(run),'--timeout-seconds',str(call_timeout),*(extra or [])]
                write_new(out / ('Client' + case + 'Command.json'), cmd)
                if asset_phase:
                    from RunCases import run_client
                    result, value = run_client(cmd[2:], out/(case+'ClientConsole.txt'), call_timeout)
                else:
                    result = subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,encoding='utf-8',timeout=call_timeout+15,creationflags=subprocess.CREATE_NO_WINDOW)
                    value = json.loads(result.stdout)
                write_new(out / ('Client' + case + 'Result.json'),value)
                allowed=expected if isinstance(expected,tuple) else (expected,)
                matched = result.returncode in allowed and value.get('cliExitCode')==result.returncode
                row(case,matched, {'exitCode':result.returncode})
                if asset_phase and not matched:
                    raise RuntimeError('Formal client failed; no subsequent graph write: '+case)
                return value
            try:
                description = client('Describe','editor-describe')
                rows_meta = description['response']['result']['operations']
                query_names = {x['name'] for x in rows_meta} & {'rules.query', 'catalog.get'}
                row('OnlyImplementedMethodsAdvertised',len(rows_meta)==(13 if query_names else 11)
                    and len(query_names) in (0, 2) and len([x for x in rows_meta if x['name'].startswith('graphs.')])==2)
                state = client('State','editor-call',['--operation','editor.getState','--call-id','State-1'])
                draft = client('Draft','editor-call',['--operation','draft.get','--call-id','Draft-1'])
                row('FormalClientSameNativePidAndState',state['pid']==draft['pid']==process.pid==identity['pid']
                    and state['response']['result']['identity']==identity and draft['response']['result']['draftRevision']=='0')
                client('ArbitraryOperation','editor-call',['--operation','console.execute','--call-id','Arbitrary-1'],2)
                old_stage=ROOT / 'Reports/PluginControlStageTwo/EditorControl1/Build88/PluginStage/NodeForge'
                client('WrongStage','editor-describe',expected=3,selected_stage=old_stage)
                win=Windows();private,acl=win.descriptor(Path(identity['sessionFile']))
                credential=private['credential']
                def raw_connection():
                    bridge=Session.__new__(Session);bridge.win=win;bridge.deadline=time.monotonic()+5;bridge.handle=None
                    while True:
                        handle=win.k.CreateFileW(identity['pipe'],0x120003,0,None,3,0x40110000,None)
                        if handle!=ctypes.c_void_p(-1).value:break
                        code=ctypes.get_last_error();left=bridge.deadline-time.monotonic()
                        if code!=231 or left<=0 or not win.k.WaitNamedPipeW(identity['pipe'],max(1,int(left*1000))):raise TransportError('testConnect'+str(code))
                    bridge.handle=handle;return bridge
                params={'editorApiVersion':identity['editorApiVersion'],'profile':identity['profile'],'controlSessionId':identity['controlSessionId'],
                    'expectedProject':project.as_posix(),'expectedPid':process.pid,'expectedProcessStart':identity['processStart'],
                    'expectedBinarySha256':identity['binarySha256'],'credential':credential}
                for key in params:
                    bad=dict(params)
                    bad[key]=process.pid+1 if key=='expectedPid' else '0'*64 if key in ('credential','expectedBinarySha256') else '0'*32 if key=='controlSessionId' else '1' if key=='expectedProcessStart' else 'public-wrong-fixture'
                    bridge=raw_connection()
                    try:
                        reply=bridge.exchange({'jsonrpc':'2.0','id':'Wrong-1','method':'session.hello','params':bad},bridge.deadline)
                        row('WrongHello'+key,reply.get('error',{}).get('code')==-32010 and 'result' not in reply)
                        write_new(out / ('WrongHello'+key+'.json'),reply)
                    finally:bridge.close()
                raw=raw_connection()
                try:
                    request={'jsonrpc':'2.0','id':'Unauthed-1','method':'editor.getState','params':{'context':{'controlSessionId':identity['controlSessionId'],'editorApiVersion':'1.0','profile':'nodeforge.editor.v1'},'arguments':{}}}
                    reply=raw.exchange(request,raw.deadline);row('UnauthenticatedStateNotDisclosed',reply.get('error',{}).get('code')==-32010 and 'result' not in reply)
                    write_new(out/'Unauthenticated.json',reply)
                finally:raw.close()
                if boundary_phase:
                    from EditorGraphBoundaryCases import run_cases
                    if not all(x['passed'] for x in rows):
                        raise RuntimeError('Identity gate failed before asset boundary')
                    def allow_boundary_asset(target):
                        nonlocal allowed_asset
                        if not target.is_relative_to(folder/'Content') or target.exists():
                            raise RuntimeError('Fresh owned target required before Import')
                        allowed_asset=ref(target)
                    def freeze_boundary_asset(target):
                        nonlocal allowed_asset
                        guard()
                        owned_assets[ref(target)]=sha(target);allowed_asset=None
                        write_new(out/'SavedAssetProtection.json',dict(owned_assets))
                    graph_attempted.append('Disconnect')
                    graph_results.extend(run_cases(ROOT,out,folder,identity,process,client,guard,
                        asset_snapshot,allow_boundary_asset,freeze_boundary_asset))
                if gui_graph_phase:
                    from EditorGuiGraphCases import run_cases
                    if not all(x['passed'] for x in rows):
                        raise RuntimeError('Identity gate failed before GUI asset generation')
                    def allow_gui_asset(target):
                        nonlocal allowed_asset
                        guard()
                        if not target.is_relative_to(folder/'Content') or target.exists():
                            raise RuntimeError('Only one fresh fixed owned GUI target may be written')
                        allowed_asset=ref(target)
                    def freeze_gui_asset(target, case):
                        nonlocal allowed_asset
                        guard();owned_assets[ref(target)]=sha(target);allowed_asset=None
                        write_new(out/(case+'GuiSavedProtection.json'),dict(owned_assets))
                    graph_results.extend(run_cases(ROOT,out,folder,identity,process,client,guard,
                        asset_snapshot,allow_gui_asset,freeze_gui_asset))
                    write_new(out/'SavedAssetProtection.json',dict(owned_assets))
                if business_phase:
                    from EditorBusinessUiCases import run_cases
                    if not all(x['passed'] for x in rows):
                        raise RuntimeError('Identity gate failed before UI business actions')
                    def allow_business_asset(target):
                        nonlocal allowed_asset
                        guard()
                        if not target.is_relative_to(folder/'Content') or target.exists():
                            raise RuntimeError('Only one exact fresh owned business target may be written')
                        allowed_asset=ref(target)
                    def allow_business_relayout():
                        nonlocal allowed_relayout
                        guard()
                        if allowed_asset is not None or allowed_relayout:
                            raise RuntimeError('Relayout permission may not overlap another write')
                        allowed_relayout=True
                    def freeze_business_asset(target, case):
                        nonlocal allowed_asset, allowed_relayout
                        guard()
                        owned_assets[ref(target)]=sha(target)
                        allowed_asset=None;allowed_relayout=False
                        write_new(out/(case+'BusinessSavedProtection.json'),dict(owned_assets))
                    graph_results.extend(run_cases(ROOT,out,folder,identity,process,client,guard,
                        asset_snapshot,allow_business_asset,allow_business_relayout,freeze_business_asset))
                    write_new(out/'SavedAssetProtection.json',dict(owned_assets))
                if aux_phase:
                    from EditorAuxUiCases import run_cases
                    if not all(x['passed'] for x in rows):
                        raise RuntimeError('Identity gate failed before auxiliary UI actions')
                    def allow_aux_asset(target):
                        nonlocal allowed_asset
                        guard()
                        if not target.is_relative_to(folder/'Content') or target.exists():
                            raise RuntimeError('Only exact fresh owned auxiliary target may be written')
                        allowed_asset=ref(target)
                    def freeze_aux_asset(target, case):
                        nonlocal allowed_asset
                        guard();owned_assets[ref(target)]=sha(target);allowed_asset=None
                        write_new(out/(case+'AuxSavedProtection.json'),dict(owned_assets))
                    graph_results.extend(run_cases(ROOT,out,folder,identity,process,client,guard,
                        asset_snapshot,allow_aux_asset,freeze_aux_asset))
                if graph_phase:
                    from RunGraphRegression import readback_errors, REQUIRED_IMPORT_CHECKS
                    def graph_check(case, passed, observed=None):
                        row(case, passed, observed)
                        if not passed:
                            raise RuntimeError('Graph gate failed; preserve original call/assets: '+case)
                    def wait_owned_json(path, seconds):
                        deadline = time.monotonic()+seconds
                        while time.monotonic()<deadline:
                            try:
                                return json.loads(path.read_text('utf-8-sig'))
                            except (OSError,json.JSONDecodeError):
                                time.sleep(.1)
                        raise RuntimeError('Owned UI proof unavailable before original deadline')
                    graph_check('IdentityGatesBeforeFirstAsset',all(x['passed'] for x in rows))
                    revision='0'
                    for case,file_name in (('Blueprint','Examples/GraphV2Samples/BlueprintConcat.json'),('Material','Examples/GraphV2Samples/StaticColor.json')):
                        guard()
                        graph_attempted.append(case)
                        source=ROOT/file_name;request=json.loads(source.read_text('utf-8-sig'));source_digest=sha(source)
                        target=(folder/'Content'/(request['assetPath'].removeprefix('/Game/')+'.uasset')).resolve()
                        graph_check(case+'FreshControlledTarget',request['assetPath'].startswith('/Game/NodeForgeTests/')
                            and target.is_relative_to(folder/'Content') and not target.exists())
                        arguments_path=out/(case+'LoadArguments.json');write_new(arguments_path,{'path':source.as_posix()})
                        loaded=client(case+'Load','editor-call',['--operation','draft.loadFile','--call-id',case+'-Load','--expected-revision',revision,'--arguments-file',str(arguments_path)])
                        revision=loaded['response']['result']['result']['draft']['draftRevision']
                        draft=client(case+'Draft','editor-call',['--operation','draft.get','--call-id',case+'-Draft'])['response']['result']
                        graph_check(case+'ExactDraftBeforeGraph',draft['draftRevision']==revision and draft['digest']==source_digest and draft['text']==source.read_text('utf-8'))
                        before_preflight=asset_snapshot()
                        for phase,operation,mode in (('Preflight','graphs.preflight','GraphPreflight'),('Generate','graphs.generate','Import')):
                            guard()
                            if phase=='Generate':allowed_asset=ref(target)
                            value=client(case+phase,'editor-call',['--operation',operation,'--call-id',case+'-'+phase,'--expected-revision',revision],call_timeout=180 if phase=='Generate' else 30)
                            record=value['response']['result'];result=record['result'];native_path=Path(result['nativeReport']['path']);native=json.loads(native_path.read_text('utf-8-sig'))
                            snapshot=Path(result['inputSnapshot']);sidecar_path=Path(result['controlEvidencePath']);sidecar=json.loads(sidecar_path.read_text('utf-8-sig'))
                            graph_check(case+phase+'BoundOriginalReceipt',value['pid']==native['pid']==process.pid and value['moduleSha256']==identity['binarySha256']
                                and native['mode']==mode and native['sessionId']==result['nativeReport']['nativeSessionId'] and native['requestId']==request['requestId']
                                and native['inputMethod']=='editorProgram' and 'uiInput' not in native and native['inputDigest']==source_digest
                                and sha(snapshot)==source_digest==result['inputDigest']==sidecar['snapshotDigest'] and sidecar['draftRevision']==revision
                                and sidecar['controlSessionId']==identity['controlSessionId'] and sidecar['callId']==case+'-'+phase
                                and sidecar['binarySha256']==identity['binarySha256'] and sidecar['nativeReport']==result['nativeReport']
                                and sha(native_path)==result['nativeReport']['sha256'] and sha(sidecar_path)==result['controlEvidenceSha256'])
                            if phase=='Preflight':
                                graph_check(case+'PreflightNoAssetWrite',native['operationStatus']=='passed' and native['overallStatus']=='succeeded' and asset_snapshot()==before_preflight)
                            else:
                                errors=[check+': not passed' for check in REQUIRED_IMPORT_CHECKS if native.get('checks',{}).get(check,{}).get('status')!='passed']
                                for key in ('initialReadback','finalReadback','publishedReadback'):
                                    errors += [key+': '+error for error in readback_errors(request,native.get(key))]
                                graph_check(case+'IndependentStructureCompileAndSave',not errors and native['overallStatus']=='waitingPersistence'
                                    and target.is_file() and sha(target)==native['fileSha256'],{'errors':errors})
                                guard()
                                owned_assets[ref(target)]=sha(target);allowed_asset=None
                                write_new(out/(case+'SavedProtection.json'),dict(owned_assets))
                                status=client(case+'Status','editor-status',['--call-id',case+'-Generate'])
                                graph_check(case+'OriginalCallQueriedWithoutReplay',status['response']['result']==record)
                                state=client(case+'State','editor-call',['--operation','editor.getState','--call-id',case+'-State'])['response']['result']
                                graph_check(case+'CurrentResultAndLastSuccess',not state['resultStale'] and state['draftRevision']==revision
                                    and state['reportPath']==native_path.as_posix() and state['assetPath']==request['assetPath']
                                    and state['lastSuccess']['reportPath']==native_path.as_posix() and state['lastSuccess']['assetPath']==request['assetPath'])
                                write_new(out/(case+'Complete.json'),{'controlSessionId':identity['controlSessionId'],'callId':case+'-Generate',
                                    'inputDigest':source_digest,'nativeReport':native_path.as_posix(),'assetPath':request['assetPath']})
                                proof=wait_owned_json(out/(case+'Ui.json'),10)
                                graph_check(case+'ActualProductionUiProof',proof['operationStatus']=='passed' and proof['pid']==process.pid
                                    and proof['controlSessionId']==identity['controlSessionId'] and proof['draftRevision']==revision
                                    and proof['inputDigest']==source_digest and proof['nativeReport']==native_path.as_posix()
                                    and proof['assetPath']==request['assetPath'] and proof['resultSummary']==state['resultSummary']
                                    and proof['actualVisibleProductionTab'] and Path(proof['screenshot']).stat().st_size>0)
                                graph_results.append({'case':case,'status':'passed','input':ref(snapshot),'inputDigest':source_digest,
                                    'nativeReport':ref(native_path),'nativeReportSha256':sha(native_path),'pid':process.pid,
                                    'asset':ref(target),'assetSha256':sha(target),'uiProof':ref(out/(case+'Ui.json'))})
                        guard()
                    client('Disable','editor-call',['--operation','control.disable','--call-id','Disable-1'])
                    guard()
                if args.phase=='Preflight':
                    from RunGraphRegression import readback_errors
                    revision='0'
                    for case,file_name,expected in (('Blueprint','Examples/GraphV2Samples/BlueprintConcat.json',0),
                        ('Material','Examples/GraphV2Samples/StaticColor.json',0),('InvalidJson','Tests/PluginControl/InvalidJson.json',2),
                        ('UnknownPin','Tests/Invalid/UnknownPin.json',2)):
                        source=ROOT/file_name;arguments_path=out/(case+'LoadArguments.json');write_new(arguments_path,{'path':source.as_posix()})
                        loaded=client(case+'Load','editor-call',['--operation','draft.loadFile','--call-id',case+'-Load','--expected-revision',revision,'--arguments-file',str(arguments_path)])
                        revision=loaded['response']['result']['result']['draft']['draftRevision']
                        preflight=client(case+'Preflight','editor-call',['--operation','graphs.preflight','--call-id',case+'-Preflight','--expected-revision',revision],expected)
                        result=preflight['response']['result']['result'];native_path=Path(result['nativeReport']['path']);native=json.loads(native_path.read_text('utf-8-sig'))
                        sidecar_path=Path(result['controlEvidencePath']);sidecar=json.loads(sidecar_path.read_text('utf-8-sig'));snapshot=Path(result['inputSnapshot'])
                        row(case+'ProgramSourceAndNativeIdentity',native['pid']==process.pid and native['inputMethod']=='editorProgram' and 'uiInput' not in native
                            and native['overallStatus']==('succeeded' if expected==0 else 'rejected') and sha(native_path)==result['nativeReport']['sha256']
                            and sha(snapshot)==sha(source)==result['inputDigest']==sidecar['snapshotDigest'] and sidecar['draftRevision']==revision
                            and sidecar['callId']==case+'-Preflight' and sidecar['controlSessionId']==identity['controlSessionId']
                            and sidecar['nativeReport']['nativeSessionId']==native['sessionId'] and sidecar['binarySha256']==identity['binarySha256']
                            and sha(sidecar_path)==result['controlEvidenceSha256'])
                        state=client(case+'State','editor-call',['--operation','editor.getState','--call-id',case+'-State'])['response']['result']
                        row(case+'CurrentResultBoundAndNoAsset',state['draftRevision']==revision and not state['resultStale'] and state['reportPath']==native_path.as_posix()
                            and not state['assetPath'] and not files_under(project.parent/'Content',ASSET_SUFFIXES))
                if args.phase.startswith('Life'):
                    import struct
                    from NodeForgeControl import engine_processes,process_uses_project,digest
                    session_args=SimpleNamespace(session_file=identity['sessionFile'],project=str(project),plugin_directory=str(stage),timeout_seconds=30)
                    old=ROOT/'Reports/PluginControlStageTwo/EditorControl1/S2/Draft99/ClientReady.json'
                    prior=json.loads(old.read_text('utf-8-sig'))['identity']
                    # Original descriptor is gone and cannot bind to this new registered PID/session.
                    cmd=[sys.executable,str(ROOT/'Scripts/NodeForgeControl.py'),'editor-describe','--session-file',prior['sessionFile'],
                        '--project',str(project),'--plugin-directory',str(stage),'--run-directory',str(out/'OldDescriptor'), '--timeout-seconds','5']
                    result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,encoding='utf-8',timeout=10,creationflags=subprocess.CREATE_NO_WINDOW)
                    old_reply=json.loads(result.stdout);write_new(out/'OldDescriptorResult.json',old_reply)
                    row('NewInstanceRejectsOldDescriptorAndUsesNewIdentity',result.returncode==3 and identity['pid']!=prior['pid']
                        and identity['controlSessionId']!=prior['controlSessionId'] and identity['processStart']!=prior['processStart'])
                    old_status=client('OldCallInNewSession','editor-status',['--call-id','Load-1'],5)
                    row('OldBusinessCallDoesNotCrossSession',old_status['response']['error']['data']['code']=='unknownCall')
                    if args.phase in ('LifeQueued','LifeRunning'):
                        argument_path=out/'MechanismArguments.json';write_new(argument_path,{'text':'{"mechanism":true}'})
                        client('MechanismStop','editor-call',['--operation','draft.replace','--call-id','Mechanism-1','--expected-revision','0',
                            '--arguments-file',str(argument_path)],(2,5) if args.phase=='LifeQueued' else (0,5))
                        deadline=time.monotonic()+5;finished_path=out/'Calls/Mechanism-1Finished.json'
                        while not finished_path.is_file() and time.monotonic()<deadline:time.sleep(.1)
                        finished=json.loads(finished_path.read_text('utf-8-sig'))
                        row('NativeInjectedStopOnRealCallKeepsCorrectTerminalEvidence',finished['state']==('cancelled' if args.phase=='LifeQueued' else 'completed')
                            and finished['cliExitCode']==(2 if args.phase=='LifeQueued' else 0)
                            and len(list((out/'Calls').glob('*Accepted.json')))==1 and len(list((out/'Calls').glob('*Finished.json')))==1)
                        write_new(out/'StopMechanism.json',{'scope':'Native GT stop/reentry injection on real admitted draft call; not a runtime Import/disconnect test',
                            'finished':finished})
                    bridge=None
                    try:
                        if args.phase=='LifeWrite':
                            argument_path=out/'LargeDraftArguments.json';write_new(argument_path,{'text':'x\n'*30000})
                            client('LargeDraft','editor-call',['--operation','draft.replace','--call-id','LargeWrite-1','--expected-revision','0',
                                '--arguments-file',str(argument_path)])
                        if args.phase in ('LifeRead','LifeWrite'):
                            bridge=Session(session_args,ROOT,digest,engine_processes,process_uses_project)
                            if args.phase=='LifeRead':
                                bridge.io(True,b'\x7b',1,time.monotonic()+5) # Partial length header, authenticated; no business is dispatched.
                                write_new(out/'WaitingPoint.json',{'point':'partialRead','bytesSent':1,'responseRead':False})
                            else:
                                request={'jsonrpc':'2.0','id':'UnreadDraft','method':'draft.get','params':{'context':{
                                    'controlSessionId':identity['controlSessionId'],'editorApiVersion':'1.0','profile':'nodeforge.editor.v1'},'arguments':{}}}
                                body=json.dumps(request,separators=(',',':')).encode('utf-8');frame=struct.pack('<I',len(body))+body
                                offset=0
                                while offset<len(frame):offset+=bridge.io(True,frame[offset:],len(frame)-offset,time.monotonic()+5)
                                write_new(out/'WaitingPoint.json',{'point':'unreadWrite','requestBytesSent':len(frame),'responseRead':False,'draftUtf8Bytes':60000})
                            time.sleep(.25)
                        if args.phase in ('LifeConnect','LifeRead','LifeWrite'):
                            disabled=client('Disable','editor-call',['--operation','control.disable','--call-id','Disable-1'])
                            stop=json.loads((out/'StopReceipt.json').read_text('utf-8-sig'))
                            kind={'LifeConnect':1,'LifeRead':2,'LifeWrite':3}[args.phase]
                            row('ActualPendingPointCancelledAndCompletionObserved',any(x.get('lastCancelledKind')==kind and x.get('lastCancelledWindowsCode')!=996
                                and x.get('cancelCompleted',0)>0 for x in stop['workers']) and stop['transportHandlesReleased']
                                and disabled.get('executionState')=='stopped' and not Path(identity['sessionFile']).exists())
                        old_query=client('AfterDisabled','editor-describe',expected=3)
                    finally:
                        if bridge:bridge.close()
                if args.phase == 'Boundary':
                    import struct
                    from NodeForgeEditorTransport import strict_json
                    from NodeForgeControl import engine_processes,process_uses_project,digest
                    session_args=SimpleNamespace(session_file=identity['sessionFile'],project=str(project),plugin_directory=str(stage),timeout_seconds=30)
                    first=Session(session_args,ROOT,digest,engine_processes,process_uses_project)
                    second=Session(session_args,ROOT,digest,engine_processes,process_uses_project)
                    try:
                        # Two already authenticated connections, no new connection delay between A/B.
                        admitted=first.request('draft.replace',{'text':'x'*64000},'TwoSlotA','0')
                        rejected=second.request('draft.restore',{},'TwoSlotB','0')
                        row('TwoRealConnectionsSingleReservedBusinessSlot',admitted.get('result',{}).get('state')=='queued'
                            and rejected.get('error',{}).get('data',{}).get('code')=='busy'
                            and not (out/'Calls/TwoSlotBAccepted.json').exists())
                        while admitted.get('result',{}).get('state') in ('accepted','queued','running'):
                            time.sleep(0.25);admitted=second.request('calls.get',{'callId':'TwoSlotA'},'TwoSlotQuery')
                        row('OnlyOriginalSlotExecuted',admitted.get('result',{}).get('state')=='completed'
                            and second.request('draft.get',{},'AfterTwoSlot')['result']['draftRevision']=='1')
                        request={'jsonrpc':'2.0','id':'Partial-1','method':'editor.getState','params':{'context':{
                            'controlSessionId':identity['controlSessionId'],'editorApiVersion':'1.0','profile':'nodeforge.editor.v1'},'arguments':{}}}
                        body=json.dumps(request,separators=(',',':')).encode('utf-8');frame=struct.pack('<I',len(body))+body
                        deadline=time.monotonic()+5
                        for chunk in (frame[:1],frame[1:3],frame[3:9],frame[9:]):
                            offset=0
                            while offset<len(chunk):offset+=first.io(True,chunk[offset:],len(chunk)-offset,deadline)
                        size=struct.unpack('<I',first.read_exact(4,deadline))[0]
                        partial=strict_json(first.read_exact(size,deadline))
                        row('RealPartialHeaderAndBodyReassembled',partial.get('id')=='Partial-1' and partial.get('result',{}).get('draftRevision')=='1')
                        write_new(out/'TwoConnections.json',{'accepted':admitted,'secondResponse':rejected,'partialResponse':partial})
                    finally:first.close();second.close()
                    truncated=raw_connection()
                    try:truncated.io(True,struct.pack('<I',100)+b'{',5,time.monotonic()+5)
                    finally:truncated.close()
                    # Deliberate fresh sentinel at the exact journal path; protected thereafter.
                    blocked=out/'Calls/RegistrationBlockedAccepted.json';blocked.write_bytes(b'owned-negative-sentinel')
                    before_block=sha(blocked)
                    args_path=out/'BlockedArguments.json';write_new(args_path,{'text':'{}'})
                    failure=client('RegistrationBlocked','editor-call',['--operation','draft.replace','--call-id','RegistrationBlocked',
                        '--expected-revision','1','--arguments-file',str(args_path)],5)
                    query=client('RegistrationBlockedStatus','editor-status',['--call-id','RegistrationBlocked'],5)
                    state=client('AfterRegistrationFailure','editor-call',['--operation','editor.getState','--call-id','FailureState'])
                    current=client('AfterRegistrationFailureDraft','editor-call',['--operation','draft.get','--call-id','FailureDraft'])
                    row('FailedRegistrationNeverExecutesAndKeepsQueryableUnknown',failure['response']['result']['state']=='unknown'
                        and query['response']['result']['code']=='registrationFailed' and state['response']['result']['state']=='stopping'
                        and current['response']['result']['draftRevision']=='1' and sha(blocked)==before_block
                        and not (out/'Calls/RegistrationBlockedFinished.json').exists())
                if args.phase == 'Draft':
                    def draft_read(case):
                        return client(case,'editor-call',['--operation','draft.get','--call-id',case+'-Read'])['response']['result']
                    fixed=ROOT/'Examples/GraphV2Samples/BlueprintConcat.json';original=fixed.read_text('utf-8')
                    def mutate(case,operation,call_id,revision,arguments=None,expected=0):
                        extra=['--operation',operation,'--call-id',call_id,'--expected-revision',str(revision)]
                        if arguments is not None:
                            path=out/(case+'Arguments.json');write_new(path,arguments);extra+=['--arguments-file',str(path)]
                        return client(case,'editor-call',extra,expected)
                    loaded=mutate('Load','draft.loadFile','Load-1',0,{'path':fixed.as_posix()})
                    actual=draft_read('LoadedDraft')
                    row('FormalLoadExactTextAndRevision',actual.get('text')==original and actual.get('digest')==sha(fixed) and actual.get('draftRevision')=='1')
                    finished_path=out/'Calls/Load-1Finished.json';finished_sha=sha(finished_path)
                    repeated=mutate('Repeat','draft.loadFile','Load-1',0,{'path':fixed.as_posix()})
                    # Query-1 correlates the first calls.get; Load-1 correlates a duplicate request.
                    # Compare the immutable business record, while Session.exchange validates each RPC id.
                    row('SameCallReturnsOriginalRecordWithoutReexecution',loaded['response']['result']==repeated['response']['result'] and sha(finished_path)==finished_sha
                        and draft_read('AfterRepeat').get('draftRevision')=='1')
                    mutate('Conflict','draft.loadFile','Load-1',0,{'path':(ROOT/'Examples/GraphV2Samples/StaticColor.json').as_posix()},2)
                    stale=mutate('Stale','draft.restore','Stale-1',0,expected=2)
                    row('RevisionRecheckedAtDispatch',stale['response']['result']['result']['code']=='staleRevision' and draft_read('AfterStale').get('text')==original)
                    mutate('Missing','draft.loadFile','Missing-1',1,{'path':(ROOT/'Tests/MissingEditorFile.json').as_posix()},2)
                    mutate('Replace','draft.replace','Replace-1',1,{'text':original+' '})
                    changed=draft_read('ReplacedDraft')
                    row('ProgramReplaceChangesOnceAndMarksOrigin',changed.get('text')==original+' ' and changed.get('draftRevision')=='2' and changed.get('origin')=='program')
                    mutate('Restore','draft.restore','Restore-1',2)
                    restored=draft_read('RestoredDraft')
                    row('FormalRestoreExactPreviousOnce',restored.get('text')==original and restored.get('draftRevision')=='3' and not restored.get('canRestore'))
                    mutate('NoRestore','draft.restore','NoRestore-1',3,expected=2)
                    mutate('SameLoad','draft.loadFile','SameLoad-1',3,{'path':fixed.as_posix()})
                    row('SameLoadAndRejectedRestoreDoNotIncrement',draft_read('AfterSameLoad').get('draftRevision')=='3')
                    mutate('PrivateInput','draft.loadFile','Private-1',3,{'path':identity['sessionFile']},2)
                    row('CredentialFileCannotEnterDraft',draft_read('AfterPrivateInput').get('text')==original)
                    status=client('OldCallStatus','editor-status',['--call-id','Load-1'])
                    row('FormalOldCallStatusKeepsOriginalEvidence',status['response']['result']==loaded['response']['result'])
                    client('UnknownCall','editor-status',['--call-id','NeverAccepted-1'],5)
                    from NodeForgeControl import engine_processes,process_uses_project,digest
                    session_args=SimpleNamespace(session_file=identity['sessionFile'],project=str(project),plugin_directory=str(stage),timeout_seconds=90)
                    bridge=Session(session_args,ROOT,digest,engine_processes,process_uses_project)
                    try:
                        before=bridge.request('draft.get',{},'BeforeDisconnect')['result']['draftRevision']
                        accepted=bridge.request('draft.replace',{'text':original+' '},'Disconnect-1',before)
                        write_new(out/'DisconnectedAccepted.json',accepted)
                        row('DisconnectAfterRealAcceptedResponse',accepted.get('result',{}).get('state') in ('accepted','queued','running'))
                    finally:bridge.close()
                    disconnected=client('DisconnectedStatus','editor-status',['--call-id','Disconnect-1'])
                    after_disconnect=draft_read('AfterDisconnect')
                    row('DisconnectedCallQueriedOnceWithoutReplay',disconnected['response']['result']['state']=='completed'
                        and after_disconnect['draftRevision']==str(int(before)+1) and after_disconnect['text']==original+' '
                        and len(list((out/'Calls').glob('Disconnect-1Accepted.json')))==1
                        and len(list((out/'Calls').glob('Disconnect-1Finished.json')))==1)
                    bridge=Session(session_args,ROOT,digest,engine_processes,process_uses_project)
                    try:
                        accepted_count=len(list((out/'Calls').glob('*Accepted.json')))
                        current=bridge.request('draft.get',{},'CapacityDraft')['result']['draftRevision']
                        capacity_ok=True
                        for index in range(128-accepted_count):
                            call_id='Capacity-'+str(index)
                            reply=bridge.request('draft.replace',{'text':'{}'},call_id,current)
                            while reply.get('result',{}).get('state') in ('accepted','queued','running'):
                                time.sleep(0.25);reply=bridge.request('calls.get',{'callId':call_id},'CapacityQuery-'+str(index))
                            if reply.get('result',{}).get('state')!='completed':capacity_ok=False;break
                            current=reply['result']['result']['draft']['draftRevision']
                        denied=bridge.request('draft.replace',{'text':'{}'},'CapacityOverflow',current)
                        row('ActualSessionCapacity128RetainsIdsAndRejects129',capacity_ok and len(list((out/'Calls').glob('*Accepted.json')))==128
                            and denied.get('error',{}).get('data',{}).get('code')=='callCapacity')
                        old=bridge.request('calls.get',{'callId':'Load-1'},'AfterCapacityOld')
                        row('CapacityDoesNotEvictOldCall',old['result']==loaded['response']['result'] and sha(finished_path)==finished_sha)
                        write_new(out/'CapacityReview.json',{'acceptedCalls':accepted_count,'totalCalls':128,'passed':capacity_ok,'overflow':denied,'oldCallUnchanged':old['result']==loaded['response']['result']})
                    finally:bridge.close()
                leaked = [p.name for p in out.rglob('*') if p.is_file() and credential.encode('ascii') in p.read_bytes()]
                row('CredentialAbsentFromEvidenceAndLogs',not leaked,{'leakedFiles':leaked})
                del private,credential
            except Exception as error:
                row('ExternalIdentityBatchException',False,{'type':type(error).__name__,'safeCode':error.code if isinstance(error,TransportError) else None,
                    'safeReason':str(error) if isinstance(error,RuntimeError) else None})
            passed=bool(rows) and all(x['passed'] for x in rows)
            write_new(out/'ClientChecks.json',{'operationStatus':'passed' if passed else 'failed','checks':rows,'pid':process.pid,'controlSessionId':identity['controlSessionId']})
            write_new(out/'ClientDone.json',{'operationStatus':'passed' if passed else 'failed','controlSessionId':identity['controlSessionId']})
        ui_observed = set()
        ui_rows = []
        def observe_ui(process):
            from hashlib import sha256
            for marker in (('UiReady','UiTyped','UiBusy','UiIdle') if args.phase=='DisabledUi' else ('UiReady','UiTyped','UiBusy')):
                path=out/(marker+'.json')
                if marker in ui_observed or not path.is_file(): continue
                ui_observed.add(marker)
                value=json.loads(path.read_text('utf-8-sig'))
                identity=json.loads((out/'UiReady.json').read_text('utf-8-sig'))['identity']
                def client(case,operation,revision=None,arguments=None,expected=0):
                    extra=['--operation',operation,'--call-id',case+'-1']
                    if revision is not None:extra+=['--expected-revision',revision]
                    if arguments is not None:
                        argument_path=out/(case+'Arguments.json');write_new(argument_path,arguments)
                        extra+=['--arguments-file',str(argument_path)]
                    command=[sys.executable,str(ROOT/'Scripts/NodeForgeControl.py'),'editor-call',
                        '--session-file',identity['sessionFile'],'--project',str(project),'--plugin-directory',str(stage),
                        '--run-directory',str(out/('Client'+case)),'--timeout-seconds','20',*extra]
                    write_new(out/(case+'Command.json'),command)
                    result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,encoding='utf-8',timeout=30,creationflags=subprocess.CREATE_NO_WINDOW)
                    reply=json.loads(result.stdout);write_new(out/(case+'Result.json'),reply)
                    ui_rows.append({'case':case,'passed':result.returncode==expected and reply.get('cliExitCode')==expected,
                        'exitCode':result.returncode})
                    return reply
                try:
                    if marker=='UiReady':
                        reply=client('ExternalLoad','draft.loadFile','0',{'path':value['inputPath']})
                        ui_rows.append({'case':'FormalClientToProductionSamePidDraft', 'passed':reply.get('pid')==process.pid==identity['pid']
                            and reply['response']['result']['result']['draft']['digest']==value['digest']})
                        done='UiLoaded'
                    elif marker=='UiTyped':
                        draft=client('ActualUiRead','draft.get')['response']['result']
                        ui_rows.append({'case':'ActualDriverToFormalClientExactTextRevisionDigest','passed':draft['text']==value['expectedText']
                            and draft['draftRevision']==value['expectedRevision']
                            and draft['digest']==sha256(value['expectedText'].encode('utf-8')).hexdigest()})
                        done='UiRead'
                    elif marker=='UiBusy':
                        state=client('ActualGuiBusyState','editor.getState')['response']['result']
                        rejected=client('GuiBusyMutation','draft.replace',value['draftRevision'],{'text':'{}'},2)
                        draft=client('GuiBusyDraft','draft.get')['response']['result']
                        ui_rows.append({'case':'ActualGuiBusyBlocksFormalMutationBeforeAcceptance','passed':state['state']=='busy'
                            and rejected['response']['error']['data']['code']=='busy' and draft['draftRevision']==value['draftRevision']
                            and not (out/'Calls/GuiBusyMutation-1Accepted.json').exists()})
                        done='UiBusyRead'
                    else:
                        disabled=client('DisableFromActualUi','control.disable')
                        ui_rows.append({'case':'FormalDisableCompletedBeforeFurtherGuiActions','passed':disabled.get('executionState')=='stopped'
                            and not Path(identity['sessionFile']).exists()})
                        done='UiDisabled'
                    write_new(out/(done+'.json'),{'controlSessionId':identity['controlSessionId'],'checksPassed':all(x['passed'] for x in ui_rows)})
                except Exception as error:
                    ui_rows.append({'case':marker+'Exception','passed':False,'type':type(error).__name__})
                write_new(out/(marker+'ClientChecks.json'),{'checks':ui_rows.copy(),'pid':process.pid})
        process = run_guarded(command, cwd=ROOT, console=out / (name + 'Console.txt'),
            timeout=720 if asset_phase else 180, expected_module=binary, observer=observe_ui if external_ui else observe_identity if channel_phase else None)
        data = json.loads(report.read_text('utf-8-sig')) if report.is_file() else {}
        ended = json.loads((out / (name + 'Exit.json')).read_text('utf-8-sig'))
        loaded = ended.get('actualLoadedModule', {})
        native_sha = data.get('moduleSha256', data.get('moduleFileSha256'))
        errors = []
        if process.returncode != 0 or data.get('operationStatus', data.get('status')) != 'passed':
            errors.append('Native test did not pass')
        if data.get('pid') != process.pid or loaded.get('pid') != process.pid or not loaded.get('matchesExpected'):
            errors.append('Actual PID/module differs or unavailable')
        if native_sha and native_sha.lower() != sha(binary):
            errors.append('Native module SHA differs')
        if ended.get('terminatedByRunner') is not False or ended.get('exitCode') != 0:
            errors.append('Native process lacks normal successful termination')
        for row in data.get('checks', data.get('cases', [])):
            if row.get('passed') is False:
                errors.append('Failed native row: ' + str(row.get('case')))
        if channel_phase:
            if external_ui:
                passed=len(ui_observed)==(4 if args.phase=='DisabledUi' else 3) and bool(ui_rows) and all(x['passed'] for x in ui_rows)
                client_checks={'operationStatus':'passed' if passed else 'failed','checks':ui_rows,'pid':process.pid}
                write_new(out/'ClientChecks.json',client_checks)
            else:
                client_checks=json.loads((out/'ClientChecks.json').read_text('utf-8-sig')) if (out/'ClientChecks.json').is_file() else {}
            stop=json.loads((out/'StopReceipt.json').read_text('utf-8-sig')) if (out/'StopReceipt.json').is_file() else {}
            if client_checks.get('operationStatus')!='passed':errors.append('External formal CLI/identity checks failed or absent')
            if (stop.get('state')!='stopped' or not stop.get('descriptorDeleted') or not stop.get('workers')
                or any(x.get('pendingIo') or not x.get('threadDone') or x.get('cancelRequested')!=x.get('cancelCompleted') for x in stop['workers'])):
                errors.append('Actual shutdown/credential/I/O completion receipt missing')
            if args.phase.startswith('Life') or args.phase=='DisabledUi' or asset_phase:
                module=json.loads((out/'ModuleShutdown.json').read_text('utf-8-sig')) if (out/'ModuleShutdown.json').is_file() else {}
                released=json.loads((out/'ControlDisabled.json').read_text('utf-8-sig')) if (out/'ControlDisabled.json').is_file() else {}
                if module.get('state')!='released' or not module.get('tickerHandleCleared') or not module.get('channelModuleReferenceReleased'):
                    errors.append('Actual module release receipt missing')
                if released.get('state')!='disabled' or not released.get('tickerSelfRemovalRequested') or not released.get('productServiceRetained'):
                    errors.append('Actual disable owner receipt missing')
        guard()
        step = {'step': name, 'test': test, 'pid': process.pid, 'exitCode': process.returncode,
            'report': ref(report), 'errors': errors, 'passed': not errors}
        write_new(out / (name + 'Review.json'), step)
        steps.append(step)
        print(name, 'passed' if not errors else 'failed', errors, flush=True)
        if errors:
            break
    guard()
    good = len(steps) == len(cases) and all(row['passed'] for row in steps)
    if asset_phase and good:
        from RunCases import run_client
        from RunProcessGuard import validate_terminal_step, LiveProcessPending
        from RunGraphRegression import readback_errors
        common=['--project',str(project),'--plugin-directory',str(stage),'--timeout-seconds','180']
        describe_run=out/'CommandletDescribe'
        try:
            child,description=run_client(['describe',*common,'--run-directory',str(describe_run)],out/'CommandletDescribeConsole.txt',180)
            if child.returncode!=0:raise RuntimeError('Original CLI runtime describe failed')
            validate_terminal_step(describe_run,'Native',{'step':'Native','passed':True,'pid':description['pid'],'exitCode':0},binary,project)
            guard()
            for item in graph_results:
                run=out/(item['case']+'Verify');snapshot=ROOT/item['input'];native_path=ROOT/item['nativeReport']
                guard()
                child,value=run_client(['execute','--operation','graphs.verifySaved',*common,'--input',str(snapshot),
                    '--generation-report',str(native_path),'--description-file',str(describe_run/'Native.json'),
                    '--run-directory',str(run)],out/(item['case']+'VerifyConsole.txt'),180)
                if child.returncode!=0:raise RuntimeError('Original CLI Verify failed/pending; do not re-import')
                validate_terminal_step(run,'Native',{'step':'Native','passed':True,'pid':value['pid'],'exitCode':0},binary,project)
                native=json.loads((run/'Native.json').read_text('utf-8-sig'));request=json.loads(snapshot.read_text('utf-8-sig'))
                errors=[]
                for key in ('reopenedReadback','reopenedFinalReadback'):
                    errors += [key+': '+error for error in readback_errors(request,native.get(key))]
                passed=not errors and native['pid']!=item['pid'] and native['checks']['persistence']['status']=='passed'
                passed=passed and native['inputDigest']==item['inputDigest'] and value['inputDigest']==item['inputDigest'] and native['overallStatus']=='succeeded'
                guard()
                verify_results.append({'case':item['case'],'status':'passed' if passed else 'failed','pid':native['pid'],
                    'nativeReport':ref(run/'Native.json'),'sha256':sha(run/'Native.json'),'errors':errors})
                if not passed:raise RuntimeError('Independent Verify did not satisfy fixed expectation')
        except LiveProcessPending as error:
            verify_results.append({'status':'pending','pid':error.process.pid,'evidence':str(error.evidence)});good=False
        except Exception as error:
            verify_results.append({'status':'failed','reason':str(error)});good=False
        good=good and len(graph_results)==len(verify_results)==(1 if boundary_phase or aux_phase else 3 if business_phase else 2) and all(x['status']=='passed' for x in verify_results)
    write_new(out / 'Summary.json', {'result': 'passed' if good else 'failed',
        'phase': args.phase, 'steps': steps, 'notRun': [name for name, _, _ in cases[len(steps):]],
        'project': ref(project), 'stage': ref(stage), 'moduleSha256': sha(binary),
        'protectedFiles': len(protected), 'protectionChanged': [], 'assetBytesUnchanged': not asset_phase,
        'generatedAssets':graph_results,'independentVerify':verify_results,'savedAssetProtection':owned_assets,
        'graphCases':[{'case':case,'status':'passed' if any(x['case']==case for x in graph_results)
            else 'failed' if case in graph_attempted else 'notRun'} for case in ('Blueprint','Material')] if graph_phase else [],
        'actualAssetsAfter':asset_snapshot(),'unconfirmedAssetWrites':{p:h for p,h in asset_snapshot().items() if p not in owned_assets},
        'UI': ('passed' if good else 'failed') if args.phase in ('Ui','ExternalUi','DisabledUi','Graph','GuiGraph','GuiBusiness','GuiAux') else 'notRun',
        'IPC': ('passed' if good else 'failed') if channel_phase else 'notRun',
        'ACL': ('passed' if good else 'failed') if channel_phase else 'notRun'})
    return 0 if good else 1

if __name__ == '__main__':
    raise SystemExit(main())
