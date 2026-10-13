"""Thin stdio-MCP business adapter. The existing CLI owns IPC/authentication/UE work."""
from __future__ import annotations
import copy
import ctypes
import hashlib
import json
import os
import msvcrt
import re
import subprocess
import sys
import threading
import uuid
from pathlib import Path
from jsonschema import Draft202012Validator
from NodeForgeEditorTransport import Windows, strict_json, TransportError
from RunProcessGuard import validate_isolated_project, process_identity, load_installation_binding, state_directory, load_ai_installation, installation_user_directory, discover_editor_sessions, installation_path, installation_product

ROOT = Path(__file__).resolve().parent.parent
HOME = ROOT / 'Tools/NodeForgeMcp'
REPORTS = ROOT / 'Reports/PluginControlStageTwo/McpDocs1'
CONTROL_REPORTS = ROOT / 'Reports/PluginControlStageTwo'
CLIENT_CONFIG = REPORTS / 'Client/Config.json'
VERSION = '0.4.0'
MAX_REPORT = 1048576
MAX_SELECTED = 32768
MAX_GRAPH = 65536
PUBLIC_CONFIG_FIELDS = {'format', 'version', 'bundleId', 'project', 'pluginDirectory',
    'binarySha256', 'sessionPublic', 'reportRoot', 'bridgeRunRoot',
    'toolContractSha256', 'docsManifestSha256'}

def encoded(value):
    return json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(',', ':'), allow_nan=False).encode('utf-8')

def sha(value): return hashlib.sha256(value).hexdigest()

def result(status, **values): return {'bridgeVersion': VERSION, 'status': status, **values}

class BridgeError(Exception):
    def __init__(self, code, status='rejected'):
        self.code, self.status = code, status
        super().__init__(code)

def safe_read(path, limit, expected=None, roots=(ROOT,)):
    path = Path(path)
    if (not path.is_absolute() or not any(path.resolve().is_relative_to(p.resolve()) for p in roots)
            or any(part.casefold() == 'plugincontrolsessions' for part in path.parts)
            or path.name.casefold() == 'session.json'):
        raise BridgeError('NFMCP_FILE_SCOPE', 'environmentBlocked')
    # Existing Windows helper verifies all reparse components and final opened handle.
    data = Windows().bounded_public_file(path, limit)
    if expected is not None and sha(data) != expected:
        raise BridgeError('NFMCP_FILE_SHA', 'environmentBlocked')
    return data

def write_new(path, value):
    Windows().no_reparse(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('xb') as stream:
        stream.write(encoded(value) + b'\n'); stream.flush(); os.fsync(stream.fileno())

def replace_owned(path, value):
    Windows().no_reparse(path)
    pending = path.with_name(path.name + '.pending')
    write_new(pending, value)
    os.replace(pending, path)

def json_pointer(value, pointer):
    if pointer == '': return value
    if not pointer.startswith('/') or re.search(r'~(?![01])', pointer):
        raise BridgeError('NFMCP_JSON_POINTER')
    for token in pointer[1:].split('/'):
        token = token.replace('~1', '/').replace('~0', '~')
        if isinstance(value, dict) and token in value: value = value[token]
        elif isinstance(value, list) and re.fullmatch(r'0|[1-9][0-9]*', token) and int(token) < len(value):
            value = value[int(token)]
        else: raise BridgeError('NFMCP_JSON_POINTER')
    return value

def catalog_summary(native):
    catalog = native.get('catalog')
    if not isinstance(catalog, dict): return None
    return {'catalogDigest': native.get('catalogDigest'), 'engine': native.get('engine'),
        'adapter': native.get('adapter'), 'families': {name: {'nodes': len(family.get('nodes', [])),
            'supportStates': sorted({node.get('supportState', 'unknown') for node in family.get('nodes', [])})}
            for name, family in catalog.items() if isinstance(family, dict) and isinstance(family.get('nodes'), list)}}

def resource_payload(native, arguments, graph_schema=None):
    """Validate the bounded native read receipt; never reinterpret pending or failure."""
    read = native.get('resourceRead')
    kind = arguments.get('assetKind') or {'MaterialFunction':'MaterialFunction', 'Texture2D':'Texture2D', 'MaterialParameters':'Material'}.get(arguments.get('kind'))
    if (native.get('reportVersion') != 'ResultContractDraft1' or native.get('operationStatus') != 'passed'
            or native.get('overallStatus') != 'succeeded' or not isinstance(read, dict)
            or read.get('source') != arguments.get('source') or read.get('kind') != kind
            or read.get('originProof') != 'controlledSavedLoad' or read.get('sourceFileHeldReadOnly') is not True
            or any(not isinstance(read.get(k), str) or not re.fullmatch('[0-9a-f]{64}', read[k])
                   for k in ('savedSourceSha256', 'configurationDigest'))):
        raise BridgeError('NFMCP_RESOURCE_RESULT', 'unknown')
    if 'assetKind' in arguments:
        value=export_payload(native,graph_schema)
        identity=graph_identity(value[0])
        if (identity['requestId']!=arguments.get('requestId') or identity['assetPath']!=arguments.get('assetPath')
                or identity['assetKind']!=arguments.get('assetKind') or identity['version']!=arguments.get('formatVersion',2)):
            raise BridgeError('NFMCP_EXPORT_ARGUMENTS','unknown')
        return value
    if kind == 'Material':
        data = native.get('parameters')
        if (not isinstance(data, dict) or data.get('parent') != read['source'] or data.get('parentAssetSha256') != read['savedSourceSha256']
                or any(not isinstance(data.get(k), dict) for k in ('scalarParameters', 'vectorParameters', 'textureParameters'))
                or not isinstance(data.get('textureSamplerSignatures'), list)):
            raise BridgeError('NFMCP_RESOURCE_RESULT', 'unknown')
    elif kind == 'Texture2D':
        data = native.get('texture')
        if (not isinstance(data, dict) or data.get('source') != read['source']
                or data.get('savedSourceSha256') != read['savedSourceSha256']
                or data.get('class') != '/Script/Engine.Texture2D' or data.get('virtualTextureStreaming') is not False
                or type(data.get('sRGB')) is not bool or not isinstance(data.get('nativeSamplerType'), str)
                or not data['nativeSamplerType'] or not isinstance(data.get('sampleQualification'), str)
                or any(type(data.get(key)) is not int or data[key] < 0 for key in ('width', 'height'))
                or type(data.get('compressionSettings')) is not int
                or data.get('properties') != {'Texture':read['source'], 'SamplerType':data['nativeSamplerType']}):
            raise BridgeError('NFMCP_TEXTURE_RESULT', 'unknown')
    else:
        data, template = native.get('functionSignature'), native.get('nodeTemplate')
        if (not isinstance(data, dict) or not isinstance(data.get('properties'), dict)
                or data['properties'].get('MaterialFunction') != read['source']
                or data['properties'].get('FunctionAssetSha256') != read['savedSourceSha256']
                or any(not isinstance(data.get(k), list) for k in ('inputs', 'outputs'))
                or not isinstance(data.get('defaultPolicy'), str) or not isinstance(data.get('hashScope'), str)
                or not isinstance(template, dict) or template.get('class') != '/Script/Engine.MaterialExpressionMaterialFunctionCall'
                or template.get('properties') != data['properties']):
            raise BridgeError('NFMCP_RESOURCE_RESULT', 'unknown')
    return None

def resource_query_payload(native, arguments, schema, query_digest, binary_sha, binding_digest, catalog_digest):
    """Validate the authenticated metadata receipt; native code owns path admission."""
    data = native.get('resourceQuery')
    effective = dict(arguments)
    from jsonschema.exceptions import ValidationError
    try:
        Draft202012Validator(schema).validate(arguments)
    except ValidationError as error:
        raise BridgeError('NFMCP_RESOURCE_QUERY_ARGUMENTS', 'unknown') from error
    for key, definition in schema['properties'].items():
        if key not in effective and 'default' in definition:
            effective[key] = definition['default']
    if (native.get('reportVersion') != 'ResultContractDraft1' or native.get('overallStatus') != 'succeeded'
            or native.get('operationStatus') != 'passed' or not isinstance(data, dict)
            or data.get('filter') != effective or data.get('configurationDigest') != query_digest
            or data.get('binarySha256') != binary_sha or data.get('installationBindingDigest') != binding_digest
            or native.get('catalogDigest') != catalog_digest or data.get('catalogDigest') != catalog_digest
            or data.get('readPolicyRevision') != 1
            or any(type(data.get(key)) is not bool for key in ('complete', 'truncated', 'registryScanning'))
            or any(type(data.get(key)) is not int or data[key] < 0 for key in ('entriesVisited','directoriesVisited','matches','returned'))
            or not isinstance(data.get('rows'), list) or data['returned'] != len(data['rows'])
            or data['returned'] > effective['limit'] or not isinstance(data.get('nextAfter'), str)
            or not isinstance(data.get('incompleteReason'), str)):
        raise BridgeError('NFMCP_RESOURCE_QUERY_RESULT', 'unknown')
    complete, truncated, next_after = data['complete'], data['truncated'], data['nextAfter']
    if ((not complete and (not truncated or next_after or not data['incompleteReason']))
            or (complete and (data['registryScanning'] or data['incompleteReason']))
            or (complete and not truncated and next_after)):
        raise BridgeError('NFMCP_RESOURCE_QUERY_COMPLETENESS', 'unknown')
    previous = effective['after']
    for row in data['rows']:
        if (not isinstance(row, dict) or row.get('discovered') is not True or row.get('policyAllowed') is not True
                or row.get('inspected') is not False or row.get('requiresInspect') is not True
                or row.get('graphExportable') != 'unknown'
                or any(not isinstance(row.get(key), str) for key in ('objectPath','packagePath','class','name'))
                or row['class'] != '/Script/Engine.' + effective['kind'] or row['objectPath'] <= previous):
            raise BridgeError('NFMCP_RESOURCE_QUERY_ROW', 'unknown')
        package, separator, name = row['objectPath'].partition('.')
        if (not separator or name != row['name'] or package.rsplit('/',1)[-1] != name
                or package.rsplit('/',1)[0] != row['packagePath']
                or not package.startswith(effective['directory'] + '/')
                or (not effective['recursive'] and row['packagePath'] != effective['directory'])):
            raise BridgeError('NFMCP_RESOURCE_QUERY_ROW', 'unknown')
        previous = row['objectPath']
    if complete and truncated and (not data['rows'] or next_after != previous):
        raise BridgeError('NFMCP_RESOURCE_QUERY_CURSOR', 'unknown')
    return {key:data[key] for key in ('catalogDigest','filter','readPolicyRevision','registryScanning','complete','truncated',
            'nextAfter','incompleteReason','entriesVisited','directoriesVisited','matches','returned')}

def graph_identity(payload):
    """Read wire identity only; generated shared schema and native codec own fields."""
    if not isinstance(payload,dict) or payload.get('format')!='NodeForgeGraph' or type(payload.get('version')) is not int:
        raise BridgeError('NFMCP_GRAPH_IDENTITY','unknown')
    version=payload['version']
    if version==2:
        kind,target=payload.get('assetType'),payload.get('assetPath')
    elif version==3 and isinstance(payload.get('asset'),dict) and isinstance(payload.get('target'),dict):
        kind,target=payload['asset'].get('type'),payload['target'].get('assetPath')
    else:raise BridgeError('NFMCP_GRAPH_IDENTITY','unknown')
    if kind not in ('Material','Blueprint','MaterialInstance') or not isinstance(target,str) or not target:
        raise BridgeError('NFMCP_GRAPH_IDENTITY','unknown')
    return {'version':version,'assetKind':kind,'assetPath':target,'requestId':payload.get('requestId'),'catalogDigest':payload.get('catalogDigest')}

def export_payload(native,graph_schema=None):
    payload, text, digest = native.get('exportedRequest'), native.get('exportedRequestText'), native.get('exportedRequestSha256')
    read, evidence = native.get('resourceRead'), native.get('exportEvidence')
    kind = 'Material' if native.get('mode') == 'MaterialGraphExport' else 'Blueprint' if native.get('mode') == 'BlueprintGraphExport' else None
    if (kind is None or native.get('operationStatus') != 'passed' or native.get('overallStatus') != 'succeeded'
            or not isinstance(payload, dict) or not isinstance(text, str) or not 0 < len(text.encode('utf-8')) <= MAX_GRAPH
            or digest != sha(text.encode('utf-8')) or encoded(strict_json(text.encode('utf-8'))) != encoded(payload)
            or graph_identity(payload)['assetKind'] != kind
            or native.get('catalogDigest') != payload.get('catalogDigest')
            or not isinstance(read, dict) or read.get('kind') != kind or not isinstance(evidence, dict)
            or evidence.get('source') != read.get('source') or evidence.get('sourceAssetSha256') != read.get('savedSourceSha256')
            or evidence.get('sourceBytesUnchanged') is not True or evidence.get('sourceRemainsClean') is not True):
        raise BridgeError('NFMCP_EXPORT_PAYLOAD', 'unknown')
    if graph_schema is not None:
        from jsonschema.exceptions import ValidationError
        try:Draft202012Validator(graph_schema).validate(payload)
        except ValidationError as error:raise BridgeError('NFMCP_EXPORT_SCHEMA','unknown') from error
    elif graph_identity(payload)['version']==3:
        raise BridgeError('NFMCP_EXPORT_SCHEMA_UNAVAILABLE','unknown')
    return payload, text, digest

class Bridge:
    def __init__(self, config_path):
        self.config_path = Path(config_path)
        config_data = safe_read(self.config_path, 65536, roots=(self.config_path.parent,))
        self.config = config = strict_json(config_data)
        self.discovery = isinstance(config, dict) and config.get('format') == 'NodeForgeAiInstallation1'
        self.contexts = {}
        if self.discovery:
            self.config = config = load_ai_installation(self.config_path, ROOT)
        elif (not isinstance(config, dict) or set(config) != PUBLIC_CONFIG_FIELDS
                or config['format'] != 'NodeForgeMcpConfig' or type(config['version']) is not int
                or config['version'] != 1
                or any(not isinstance(v, str) for k, v in config.items() if k != 'version')):
            raise BridgeError('NFMCP_CONFIG', 'environmentBlocked')
        self.contract = strict_json(safe_read(HOME / 'ToolContract.json', MAX_REPORT, config['toolContractSha256'], (HOME,)))
        self.operations = strict_json(safe_read(ROOT / self.contract['operationsSnapshot'], MAX_REPORT,
            self.contract['operationsSnapshotSha256'], (CONTROL_REPORTS, ROOT / 'Metadata')))
        # Describe records the observing session beside the static definitions.
        # The current session is checked against its own public identity in _connect.
        self.operations.pop('identity', None)
        self.manifest_path = ROOT / self.contract['docsManifest']
        self.docs_root = self.manifest_path.parent.resolve()
        self.manifest = strict_json(safe_read(self.manifest_path, MAX_REPORT, config['docsManifestSha256'],
                                             (ROOT / 'Docs/Versions',)))
        self.bundle_id = self.contract['bundleId']
        if (self.bundle_id != config['bundleId'] or self.contract['bridgeVersion'] != VERSION
                or self.manifest['bundleId'] != self.bundle_id or self.manifest['bridgeVersion'] != VERSION
                or self.manifest['binarySha256'] != config['binarySha256']):
            raise BridgeError('NFMCP_VERSION', 'environmentBlocked')
        if self.discovery:
            self.state_root = installation_user_directory(ROOT)
            self.control_reports = self.state_root
            self.project = None
            _, self.engine, self.stage, self.binary, _ = installation_product(ROOT, config['engineRoot'])
            self.report_root = self.state_root / 'Runs'
            self.run_root = self.report_root / uuid.uuid4().hex
            self.public_path = None
            for path, expected in self.manifest['cliFiles'].items():
                safe_read(ROOT / path, MAX_REPORT, expected, (ROOT / 'Scripts',))
        else:
            self.project, self.stage, self.binary = validate_isolated_project(config['project'], config['pluginDirectory'], ROOT)
            self.state_root = state_directory(self.project)
            self.control_reports = self.state_root / 'Reports/PluginControlStageTwo'
            # The public usage manifest describes distributable product metadata.
            # The selected project AND actual stage are admitted by the private
            # binding + product InstallationManifest, not by a developer report path.
            if (self.manifest.get('installationSchemaVersion') != 1
                    or sha(safe_read(self.binary, 64 * MAX_REPORT, roots=(self.stage,))) != config['binarySha256']):
                raise BridgeError('NFMCP_STAGE', 'environmentBlocked')
            for path, expected in self.manifest['cliFiles'].items():
                safe_read(ROOT / path, MAX_REPORT, expected, (ROOT / 'Scripts',))
            self.report_root, self.run_root = Path(config['reportRoot']), Path(config['bridgeRunRoot'])
            self.public_path = Path(config['sessionPublic'])
            report_namespace = self.state_root / self.manifest['reportNamespace']
            if not report_namespace.resolve().is_relative_to(self.control_reports):
                raise BridgeError('NFMCP_RELEASE_REPORT_SCOPE', 'environmentBlocked')
            # Preserve the operator's already configured launcher across finite releases.
            # This admits one exact non-secret config, never its sibling reports.
            if (not Path(config_path).resolve().is_relative_to(HOME)
                    and not Path(config_path).resolve().is_relative_to(report_namespace)
                    and Path(config_path).resolve() != CLIENT_CONFIG.resolve()):
                raise BridgeError('NFMCP_CONFIG_RELEASE_SCOPE', 'environmentBlocked')
            if (not self.report_root.is_absolute() or not self.run_root.is_absolute()
                    or not self.report_root.resolve().is_relative_to(report_namespace)
                    or not self.run_root.resolve().is_relative_to(report_namespace)
                    or self.public_path != self.report_root / 'SessionPublic.json'):
                raise BridgeError('NFMCP_REPORT_ROOT', 'environmentBlocked')
        Windows().no_reparse(self.run_root)
        self.run_root.mkdir(parents=True, exist_ok=True)
        self.lease_path = self.run_root / 'Bridge.lock'
        self.lease = {'pid': os.getpid(), 'instance': uuid.uuid4().hex}
        self.owns_lease = False
        self.tools = {row['name']: row for row in self.contract['tools']}
        self.identity = None; self.handle = None
        self.reports = {}; self.calls = {}; self.unknown = set(); self.pending_cli = []
        self.business_lock = threading.Lock(); self.connections = threading.BoundedSemaphore(2)
        self.connection_lock = threading.Lock()
        self.metadata_lock = threading.RLock()
        # Bind the actually loaded application generation. A source hash alone
        # cannot load changed code; the server must construct a fresh generation.
        self.loaded_runtime_files = {path: sha(safe_read(ROOT / path, MAX_REPORT)) for path in (
            'Scripts/NodeForgeMcp.py', 'Scripts/NodeForgeMcpBridge.py',
            'Scripts/NodeForgeEditorTransport.py', 'Scripts/RunProcessGuard.py',
            'Tools/NodeForgeMcp/DependencyManifest.json', 'Tools/NodeForgeMcp/Requirements.lock')}
        runtime_bindings = {**self.manifest['cliFiles'], **{
            row['path']: row['sha256'] for row in self.manifest.get('bindings', [])}}
        if any(runtime_bindings.get(path) != digest for path, digest in self.loaded_runtime_files.items()):
            raise BridgeError('NFMCP_RUNTIME_BINDING', 'environmentBlocked')

    def configuration_changed(self):
        if self.discovery:
            return load_ai_installation(self.config_path, ROOT) != self.config
        current = strict_json(safe_read(self.config_path, 65536, roots=(HOME, self.control_reports)))
        if (not isinstance(current, dict) or set(current) != PUBLIC_CONFIG_FIELDS
                or current.get('format') != 'NodeForgeMcpConfig' or type(current.get('version')) is not int
                or current['version'] != 1 or any(not isinstance(v, str) for k, v in current.items() if k != 'version')):
            raise BridgeError('NFMCP_CONFIG', 'environmentBlocked')
        # Leave an approved same-version documentary refresh to _documents.
        return {k: v for k, v in current.items() if k != 'docsManifestSha256'} != {
            k: v for k, v in self.config.items() if k != 'docsManifestSha256'}

    def rebind(self, application_factory=None):
        """Admit a complete approved release, never a new value in an old handle.

        Called only by the existing SDK server with zero in-flight requests.
        Pending work and a live old editor keep their exact owner and records.
        """
        if self.discovery:
            if any(context.has_unresolved() for context in self.contexts.values()):
                raise BridgeError('NFMCP_REBIND_UNRESOLVED', 'environmentBlocked')
            candidate = application_factory(self) if application_factory else type(self)(self.config_path)
            for context in self.contexts.values(): context.close()
            self.contexts.clear()
            return candidate, {'oldBundleId':self.bundle_id, 'newBundleId':candidate.bundle_id,
                'oldHandleInvalidated':True, 'newConnectionRequired':True,
                'toolsChanged':self.contract['tools'] != candidate.contract['tools'],
                'applicationCodeReloaded':application_factory is not None,
                'loadedRuntimeFiles':candidate.loaded_runtime_files}
        if not self.connection_lock.acquire(blocking=False):
            raise BridgeError('NFMCP_REBIND_BUSY', 'environmentBlocked')
        try:
            if not self.business_lock.acquire(blocking=False):
                raise BridgeError('NFMCP_REBIND_BUSY', 'environmentBlocked')
            try:
                with self.metadata_lock:
                    return self._rebind_idle(application_factory)
            finally:
                self.business_lock.release()
        finally:
            self.connection_lock.release()

    def has_unresolved(self):
        if self.discovery: return any(context.has_unresolved() for context in self.contexts.values())
        if self.pending_cli or self.unknown:
            return True
        for path in (self.run_root / 'Calls').glob('*.json'):
            row = strict_json(safe_read(path, MAX_REPORT, roots=(self.run_root,)))
            if row.get('state') not in ('finished', 'failed', 'rejected', 'cancelled') or row.get('requiresReview'):
                return True
        return False

    def retained_query(self, name, params):
        """Only a recorded old call/report may bypass a pending config rebind."""
        if self.discovery:
            context = self.contexts.get(params.get('handle'))
            return bool(context and context.retained_query(name, params))
        if not self.identity or not self.handle or params.get('handle') != self.handle:
            return False
        if name == 'nodeforge_calls_get':
            arguments = params.get('arguments')
            return isinstance(arguments, dict) and arguments.get('callId') in self.calls
        if name == 'nodeforge_report_read':
            return params.get('reportId') in self.reports
        return False

    def _rebind_idle(self, application_factory=None):
        # An admitted adapter correction can recover the SAME native session.
        # Loading code is not permission to discard pending work or change its
        # native identity. Preserve the owner and let formal calls.get resolve it.
        candidate = application_factory(self) if application_factory else None
        if candidate is not None:
            receipt = candidate._adopt_compatible_owner(self)
            if receipt is not None:
                return candidate, receipt
        if self.has_unresolved():
            raise BridgeError('NFMCP_REBIND_UNRESOLVED', 'environmentBlocked')
        runtime_changed = any(sha(safe_read(ROOT / path, MAX_REPORT)) != digest
                              for path, digest in self.loaded_runtime_files.items())
        if runtime_changed and application_factory is None:
            raise BridgeError('NFMCP_RUNTIME_RESTART_REQUIRED', 'environmentBlocked')
        old_public = self.identity
        if old_public is None and self.public_path.exists():
            old_public = self._public()
        terminal = None
        if old_public is not None:
            terminal = process_identity(old_public['pid'])
            if (terminal['state'] == 'unknown' or terminal['state'] == 'running'
                    and terminal.get('createdTicks') == old_public['processStart']):
                raise BridgeError('NFMCP_REBIND_EDITOR_ACTIVE', 'environmentBlocked')
            private = self.state_root / 'Reports/PluginControlSessions' / old_public['controlSessionId'] / 'Session.json'
            if Path(old_public['sessionFile']).resolve() != private.resolve() or private.exists():
                raise BridgeError('NFMCP_REBIND_SESSION_NOT_CLOSED', 'environmentBlocked')
        if self.owns_lease and (not self.lease_path.exists() or strict_json(
                safe_read(self.lease_path, 65536, roots=(self.run_root,))) != self.lease):
            raise BridgeError('NFMCP_LEASE_CHANGED', 'environmentBlocked')
        config_data = safe_read(self.config_path, 65536, roots=(HOME, self.control_reports))
        candidate = candidate or type(self)(self.config_path)
        if (candidate.project != self.project or candidate.contract['editorApiVersion'] != self.contract['editorApiVersion']
                or candidate.contract['profile'] != self.contract['profile']):
            raise BridgeError('NFMCP_REBIND_INCOMPATIBLE', 'environmentBlocked')
        # Recheck the publication and loaded runtime before releasing this owner.
        safe_read(self.config_path, 65536, sha(config_data), (HOME, self.control_reports))
        safe_read(HOME / 'ToolContract.json', MAX_REPORT, candidate.config['toolContractSha256'], (HOME,))
        safe_read(candidate.manifest_path, MAX_REPORT, candidate.config['docsManifestSha256'], (ROOT / 'Docs/Versions',))
        if any(sha(safe_read(ROOT / path, MAX_REPORT)) != digest
               for path, digest in candidate.loaded_runtime_files.items()):
            raise BridgeError('NFMCP_FILE_SHA', 'environmentBlocked')
        if candidate.loaded_runtime_files != self.loaded_runtime_files and application_factory is None:
            raise BridgeError('NFMCP_RUNTIME_RESTART_REQUIRED', 'environmentBlocked')
        receipt = {'format': 'NodeForgeMcpRebind1', 'status': 'admitted',
            'oldBundleId': self.bundle_id, 'newBundleId': candidate.bundle_id,
            'oldBinarySha256': self.config['binarySha256'], 'newBinarySha256': candidate.config['binarySha256'],
            'oldDocsManifestSha256': self.config['docsManifestSha256'],
            'newDocsManifestSha256': candidate.config['docsManifestSha256'],
            'oldControlSessionId': old_public['controlSessionId'] if old_public else None,
            'bridgePid': os.getpid(),
            'oldProcessObservation': terminal, 'oldHandleInvalidated': self.handle is not None,
            'oldCallsPreserved': True, 'newConnectionRequired': True,
            'ueStarted': False, 'processTerminated': False, 'externalLeaseRemoved': False,
            'bridgeVersion': candidate.contract['bridgeVersion'],
            'applicationCodeReloaded': runtime_changed,
            'loadedRuntimeFiles': candidate.loaded_runtime_files,
            'oldRuntimeFiles': self.loaded_runtime_files,
            'toolsChanged': self.contract['tools'] != candidate.contract['tools']}
        journal = candidate.run_root / 'Rebindings' / ('Rebind' + uuid.uuid4().hex + '.json')
        write_new(journal, receipt)
        if self.owns_lease:
            # Release only this exact, idle owner's lease; never a stale/external lock.
            if strict_json(safe_read(self.lease_path, 65536, roots=(self.run_root,))) != self.lease:
                raise BridgeError('NFMCP_LEASE_CHANGED', 'environmentBlocked')
            self.lease_path.unlink()
        self.owns_lease = False
        self.handle = None; self.identity = None; self.reports.clear()
        receipt = {**receipt, 'status': 'passed', 'admissionPath': str(journal)}
        write_new(journal.with_name(journal.stem + 'Committed.json'), receipt)
        return candidate, receipt

    def _adopt_compatible_owner(self, old):
        """Transfer an idle SDK owner's objects, never its lock or native session."""
        native_fields = PUBLIC_CONFIG_FIELDS - {'bundleId', 'toolContractSha256', 'docsManifestSha256'}
        stable_runtime = set(old.loaded_runtime_files) - {
            'Scripts/NodeForgeMcp.py', 'Scripts/NodeForgeMcpBridge.py'}
        if (any(self.config[k] != old.config[k] for k in native_fields)
                or self.operations != {k: v for k, v in old.operations.items() if k != 'identity'}
                or any(self.manifest.get(k) != old.manifest.get(k) for k in (
                    'catalogDigest', 'engine', 'adapter', 'graphProtocolVersion',
                    'binarySha256', 'editorApiVersion', 'profile', 'installationSchemaVersion'))
                or self.contract['limits'] != old.contract['limits']
                or any(self.loaded_runtime_files[k] != old.loaded_runtime_files[k] for k in stable_runtime)
                or self.manifest['cliFiles'] != old.manifest['cliFiles']
                or self.contract['tools'] != old.contract['tools']
                or old.pending_cli):
            return None
        if old.identity is not None and self._public() != old.identity:
            raise BridgeError('NFMCP_SESSION_CHANGED', 'environmentBlocked')
        if old.owns_lease and strict_json(safe_read(old.lease_path, 65536, roots=(old.run_root,))) != old.lease:
            raise BridgeError('NFMCP_LEASE_CHANGED', 'environmentBlocked')
        # Constructor and loader validated the complete approved generation;
        # check it again before transferring the live state.
        config_data = safe_read(self.config_path, 65536, roots=(HOME, self.control_reports))
        if strict_json(config_data) != self.config:
            raise BridgeError('NFMCP_CONFIG_CHANGED', 'environmentBlocked')
        safe_read(HOME / 'ToolContract.json', MAX_REPORT, self.config['toolContractSha256'])
        safe_read(self.manifest_path, MAX_REPORT, self.config['docsManifestSha256'])
        for path, digest in self.loaded_runtime_files.items():
            safe_read(ROOT / path, MAX_REPORT, digest)
        receipt = {'format': 'NodeForgeMcpRebind1', 'status': 'admitted',
            'oldBundleId': old.bundle_id, 'newBundleId': self.bundle_id,
            'oldHandleInvalidated': False, 'newConnectionRequired': False,
            'toolsChanged': False, 'applicationCodeReloaded': True,
            'loadedRuntimeFiles': self.loaded_runtime_files,
            'oldRuntimeFiles': old.loaded_runtime_files, 'bridgePid': os.getpid(),
            'mode': 'sameNativeOwner', 'unresolvedCallsPreserved': sorted(old.unknown),
            'oldCallsPreserved': True, 'leaseBytesUnchanged': True,
            'ueStarted': False, 'processTerminated': False, 'externalLeaseRemoved': False}
        path = self.run_root / 'Rebindings' / ('Rebind' + uuid.uuid4().hex + '.json')
        write_new(path, receipt)
        safe_read(self.config_path, 65536, sha(config_data), (HOME, self.control_reports))
        self.identity = copy.deepcopy(old.identity); self.handle = old.handle
        self.lease = copy.deepcopy(old.lease); self.owns_lease = old.owns_lease
        self.calls = copy.deepcopy(old.calls); self.reports = copy.deepcopy(old.reports)
        self.unknown = set(old.unknown)
        old.owns_lease = False; old.identity = None; old.handle = None
        receipt = {**receipt, 'status': 'passed', 'admissionPath': str(path)}
        write_new(path.with_name(path.stem + 'Committed.json'), receipt)
        return receipt

    def _documents(self):
        if self.discovery:
            current = load_ai_installation(self.config_path, ROOT)
            if current != self.config:
                raise BridgeError('NFMCP_CONFIG_RELOAD_REQUIRED', 'environmentBlocked')
            safe_read(self.manifest_path, MAX_REPORT, current['docsManifestSha256'], (ROOT / 'Docs/Versions',))
            return self.manifest, current['docsManifestSha256']
        # Only the operator's fixed config can authorize a new docs fingerprint.
        # Session, executable and operation changes still require a new connection.
        with self.metadata_lock:
            config_data = safe_read(self.config_path, 65536, roots=(HOME, self.control_reports))
            current = strict_json(config_data)
            if (not isinstance(current, dict) or set(current) != PUBLIC_CONFIG_FIELDS
                    or any(type(current[key]) is not type(value) or current[key] != value
                           for key, value in self.config.items()
                           if key != 'docsManifestSha256')
                    or not isinstance(current['docsManifestSha256'], str)
                    or not re.fullmatch('[0-9a-f]{64}', current['docsManifestSha256'])):
                raise BridgeError('NFMCP_CONFIG_RELOAD_REQUIRED', 'environmentBlocked')
            manifest = strict_json(safe_read(self.manifest_path, MAX_REPORT,
                                            current['docsManifestSha256'], (ROOT / 'Docs/Versions',)))
            if not isinstance(manifest, dict):
                raise BridgeError('NFMCP_DOCUMENT_UNAVAILABLE', 'environmentBlocked')
            if current['docsManifestSha256'] != self.config['docsManifestSha256']:
                docs_fields = {'pages', 'artifacts', 'bindings', 'publicationStatus',
                    'fullUsagePackageComplete', 'releaseEvidence', 'frozenAt',
                    'actualCodexBusinessAcceptance', 'sourceRepair',
                    'firstProtocolObservationNotCurrentRevision',
                    'historicalEvidenceRetainsOriginalVersion',
                    'sourceContinuityDoesNotMeanUiRuntimeRetest',
                    'authoringSources', 'htmlArtifacts', 'publicationInputs'}
                if ({k: v for k, v in manifest.items() if k not in docs_fields}
                        != {k: v for k, v in self.manifest.items() if k not in docs_fields}):
                    raise BridgeError('NFMCP_CONFIG_RELOAD_REQUIRED', 'environmentBlocked')
                def bindings(value):
                    rows = value['bindings']
                    if len({row['path'] for row in rows}) != len(rows):
                        raise BridgeError('NFMCP_DOCUMENT_UNAVAILABLE', 'environmentBlocked')
                    return {row['path']: row for row in rows
                            if row['path'] != 'Scripts/BuildNodeForgeUsageDocs.py'}
                if bindings(manifest) != bindings(self.manifest):
                    raise BridgeError('NFMCP_CONFIG_RELOAD_REQUIRED', 'environmentBlocked')
                # Knowledge sources and generated HTML are documentary provenance.
                # Only those source fingerprints may change, never tool/core inputs.
                old_inputs = self.manifest.get('publicationInputs', {})
                new_inputs = manifest.get('publicationInputs', {})
                doc_inputs = {'pages', 'nodeDocumentation', 'controls', 'sourceNotice'}
                if (not isinstance(old_inputs, dict) or not isinstance(new_inputs, dict)
                        or set(old_inputs) != set(new_inputs)
                        or {k: v for k, v in old_inputs.items() if k not in doc_inputs}
                        != {k: v for k, v in new_inputs.items() if k not in doc_inputs}
                        or any(not isinstance(new_inputs[k], dict)
                               or old_inputs[k]['path'] != new_inputs[k].get('path')
                               for k in doc_inputs & set(old_inputs))):
                    raise BridgeError('NFMCP_CONFIG_RELOAD_REQUIRED', 'environmentBlocked')
                def document_ref(row, roots, limit):
                    if (not isinstance(row, dict) or not isinstance(row.get('path'), str)
                            or Path(row['path']).is_absolute()
                            or not any((ROOT / row['path']).resolve().is_relative_to(root.resolve()) for root in roots)
                            or Path(row['path']).name.casefold() == 'session.json'
                            or any(part.casefold() == 'plugincontrolsessions' for part in Path(row['path']).parts)
                            or not isinstance(row.get('sha256'), str)
                            or not re.fullmatch('[0-9a-f]{64}', row['sha256'])
                            or type(row.get('bytes')) is not int or not 0 <= row['bytes'] <= limit):
                        raise BridgeError('NFMCP_DOCUMENT_UNAVAILABLE', 'environmentBlocked')
                for name in doc_inputs & set(new_inputs):
                    roots = (ROOT / 'Docs', CONTROL_REPORTS) if name == 'sourceNotice' else (ROOT / 'Docs',)
                    document_ref(new_inputs[name], roots, MAX_REPORT)
                for name, roots in (('authoringSources', (ROOT / 'Docs',)),
                                    ('htmlArtifacts', (self.docs_root,))):
                    rows = manifest.get(name, [])
                    if not isinstance(rows, list):
                        raise BridgeError('NFMCP_DOCUMENT_UNAVAILABLE', 'environmentBlocked')
                    for row in rows: document_ref(row, roots, MAX_REPORT)
                    if len({row['path'] for row in rows}) != len(rows):
                        raise BridgeError('NFMCP_DOCUMENT_UNAVAILABLE', 'environmentBlocked')
                for name in ('Catalog.json', 'Operations.json'):
                    old = [row for row in self.manifest['artifacts'] if Path(row['path']).name == name]
                    new = [row for row in manifest['artifacts'] if Path(row['path']).name == name]
                    if old != new:
                        raise BridgeError('NFMCP_CONFIG_RELOAD_REQUIRED', 'environmentBlocked')
                for row in manifest['artifacts']:
                    if (Path(row['path']).is_absolute()
                            or not (ROOT / row['path']).resolve().is_relative_to(self.docs_root)):
                        raise BridgeError('NFMCP_DOCUMENT_UNAVAILABLE', 'environmentBlocked')
                page_ids = set()
                for row in manifest['pages']:
                    if (not isinstance(row['pageId'], str) or row['pageId'] in page_ids
                            or not 1 <= len(row['pageId']) <= 128
                            or Path(row['path']).is_absolute()
                            or not (ROOT / row['path']).resolve().is_relative_to(self.docs_root)
                            or not re.fullmatch('[0-9a-f]{64}', row['sha256'])
                            or type(row['bytes']) is not int or not 0 <= row['bytes'] <= 65536):
                        raise BridgeError('NFMCP_DOCUMENT_UNAVAILABLE', 'environmentBlocked')
                    page_ids.add(row['pageId'])
                # A publishing race must not commit a partly observed config/package.
                safe_read(self.config_path, 65536, sha(config_data), (HOME, self.control_reports))
                safe_read(self.manifest_path, MAX_REPORT, current['docsManifestSha256'], (ROOT / 'Docs/Versions',))
                self.manifest = manifest
                self.config = current
            return self.manifest, self.config['docsManifestSha256']

    def close(self):
        if self.discovery:
            for context in self.contexts.values(): context.close()
            return
        # A live timed-out CLI or unclosed business retains its lease/checkpoint.
        if not self.connection_lock.acquire(blocking=False): return
        try:
            if not self.business_lock.acquire(blocking=False): return
            try:
                if not self.owns_lease or self.has_unresolved(): return
                if self.lease_path.is_file() and strict_json(safe_read(self.lease_path, 65536, roots=(self.run_root,))) == self.lease:
                    self.lease_path.unlink()
                self.owns_lease = False
                self.identity = None; self.handle = None; self.reports.clear()
            finally:
                self.business_lock.release()
        finally:
            self.connection_lock.release()

    def _claim_business_lease(self):
        if self.owns_lease:
            if strict_json(safe_read(self.lease_path, 65536, roots=(self.run_root,))) != self.lease:
                raise BridgeError('NFMCP_LEASE_CHANGED', 'environmentBlocked')
            return
        try:
            write_new(self.lease_path, self.lease)
        except FileExistsError:
            self._recover_closed_owner()
        self.owns_lease = True

    def _recover_closed_owner(self):
        """Evidence-gated ownership handoff; keep the lease file and original calls."""
        win = Windows()
        win.no_reparse(self.lease_path)
        # Share mode zero excludes competing recovery/read/write/delete. Do not
        # unlink the lease or take it from a live/unknown owner.
        handle = win.k.CreateFileW(str(self.lease_path), 0xc0000000, 0, None, 3, 0x00200000, None)
        if handle == ctypes.c_void_p(-1).value:
            raise BridgeError('NFMCP_BUSY', 'environmentBlocked')
        try:
            final = ctypes.create_unicode_buffer(32768)
            n = win.k.GetFinalPathNameByHandleW(handle, final, len(final), 0)
            if not n or n >= len(final) or Path(final.value.removeprefix('\\\\?\\')).resolve() != self.lease_path.resolve():
                raise BridgeError('NFMCP_LEASE_CHANGED', 'environmentBlocked')
            fd = msvcrt.open_osfhandle(handle, os.O_RDWR | os.O_BINARY)
            handle = None
            with os.fdopen(fd, 'r+b') as stream:
                original = stream.read(65537)
                if not 0 < len(original) <= 65536:
                    raise BridgeError('NFMCP_LEASE_CHANGED', 'environmentBlocked')
                owner = strict_json(original)
                if (not isinstance(owner, dict) or set(owner) != {'pid', 'instance'}
                        or type(owner['pid']) is not int or owner['pid'] <= 0
                        or not isinstance(owner['instance'], str) or not re.fullmatch('[0-9a-f]{32}', owner['instance'])
                        or process_identity(owner['pid'])['state'] != 'absent'):
                    raise BridgeError('NFMCP_BUSY', 'environmentBlocked')
                public = self._public()
                prior_identity = self.identity
                evidence = []
                try:
                    self.identity = public
                    for path in sorted((self.run_root / 'Calls').glob('*.json')):
                        data = safe_read(path, MAX_REPORT, roots=(self.run_root,))
                        row = strict_json(data)
                        if (row.get('controlSessionId') != public['controlSessionId']
                                or path.name != row.get('callId', '') + '.json'):
                            raise BridgeError('NFMCP_OLD_CALL_SESSION', 'environmentBlocked')
                        self.calls[row['callId']] = row
                        summary = self._cli('editor-call', 'calls.get', {'callId': row['callId']})
                        value = self._project_result(summary)
                        native = summary.get('response', {}).get('result', {})
                        if (native.get('callId') != row['callId']
                                or native.get('controlSessionId') != public['controlSessionId']
                                or native.get('operation') != row.get('operation')
                                or native.get('expectedDraftRevision') != row.get('expectedDraftRevision')
                                or native.get('state') not in ('completed', 'rejected', 'cancelled')
                                or value.get('cliExitCode') not in (0, 2, 3)
                                or value['status'] in ('unknown', 'pending', 'stopping', 'environmentBlocked')):
                            raise BridgeError('NFMCP_REBIND_UNRESOLVED', 'environmentBlocked')
                        evidence.append({'callId': row['callId'], 'originalPath': str(path),
                            'originalSha256': sha(data), 'originalBytes': len(data), 'query': value})
                finally:
                    self.identity = prior_identity
                if self._public() != public or process_identity(owner['pid'])['state'] != 'absent':
                    raise BridgeError('NFMCP_LEASE_CHANGED', 'environmentBlocked')
                for item in evidence:
                    safe_read(Path(item['originalPath']), MAX_REPORT, item['originalSha256'], (self.run_root,))
                journal = self.run_root / 'Rebindings' / ('OwnerRecovery' + uuid.uuid4().hex + '.json')
                receipt = {'format': 'NodeForgeMcpOwnerRecovery1', 'status': 'admitted',
                    'oldLeaseText': original.decode('utf-8'), 'oldLeaseSha256': sha(original),
                    'newOwner': self.lease, 'identity': public, 'calls': evidence,
                    'originalCallsUnchanged': True, 'leaseFileDeleted': False,
                    'nativeBusinessResubmitted': False, 'ueStarted': False, 'processTerminated': False}
                write_new(journal, receipt)
                stream.seek(0); stream.write(encoded(self.lease) + b'\n'); stream.truncate()
                stream.flush(); os.fsync(stream.fileno())
                self.owns_lease = True
                write_new(journal.with_name(journal.stem + 'Committed.json'),
                          {**receipt, 'status': 'passed', 'admissionPath': str(journal)})
        finally:
            if handle is not None: win.k.CloseHandle(handle)

    def _public(self):
        identity = strict_json(safe_read(self.public_path, 65536, roots=(self.report_root,)))
        if (identity.get('project') != self.project.as_posix() or identity.get('binarySha256') != self.config['binarySha256']
                or Path(identity.get('module', '')) != self.binary
                or Path(identity.get('reportRoot', '')) != self.report_root
                or identity.get('editorApiVersion') != self.operations['editorApiVersion']
                or identity.get('profile') != self.operations['profile']
                or not re.fullmatch('[0-9a-f]{32}', identity.get('controlSessionId', ''))):
            raise BridgeError('NFMCP_PUBLIC_IDENTITY', 'environmentBlocked')
        return identity

    def _bound(self, handle, retained_recovery=False):
        if not self.handle or handle != self.handle: raise BridgeError('NFMCP_HANDLE', 'environmentBlocked')
        if retained_recovery:
            # Retained queries use the original loaded implementation and exact
            # public identity/report bytes. They never read the new tool contract.
            if any(sha(safe_read(ROOT / path, MAX_REPORT)) != digest
                   for path, digest in self.loaded_runtime_files.items()
                   if path not in ('Scripts/NodeForgeMcp.py', 'Scripts/NodeForgeMcpBridge.py')):
                raise BridgeError('NFMCP_RUNTIME_RESTART_REQUIRED', 'environmentBlocked')
        else:
            self._documents()
        current = self._public()
        observation = process_identity(current['pid'])
        if (observation.get('state') != 'running' or observation.get('createdTicks') != current['processStart']
                or not Path(current['sessionFile']).is_file()):
            raise BridgeError('NFMCP_SESSION_CLOSED', 'environmentBlocked')
        if current != self.identity:
            self.handle = None; self.reports.clear()
            raise BridgeError('NFMCP_SESSION_CHANGED', 'environmentBlocked')
        return current

    def _cli(self, action, operation=None, arguments=None, call_id=None, revision=None):
        identity = self._public()
        for path, expected in self.manifest['cliFiles'].items():
            safe_read(ROOT / path, MAX_REPORT, expected, (ROOT / 'Scripts',))
        run = self.run_root / 'Cli' / ('Run' + uuid.uuid4().hex)
        command = [sys.executable, str(ROOT / 'Scripts/NodeForgeControl.py'), action,
            '--session-file', identity['sessionFile'], '--project', str(self.project),
            '--plugin-directory', str(self.stage), '--run-directory', str(run), '--timeout-seconds', '180']
        if action != 'editor-describe': command += ['--call-id', call_id or ('Read-' + uuid.uuid4().hex)]
        if action == 'editor-call': command += ['--operation', operation]
        if revision is not None: command += ['--expected-revision', revision]
        if arguments:
            args_file = self.run_root / 'Arguments' / ('Args' + uuid.uuid4().hex + '.json')
            write_new(args_file, arguments); command += ['--arguments-file', str(args_file)]
        if not self.connections.acquire(blocking=False): raise BridgeError('NFMCP_CONNECTION_BUSY')
        try:
            process = subprocess.Popen(command, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                creationflags=subprocess.CREATE_NO_WINDOW)
            try:
                stdout, stderr = process.communicate(timeout=195)
            except subprocess.TimeoutExpired:
                cli_identity = process_identity(process.pid)
                self.pending_cli.append({'process': process, 'run': run, 'identity': cli_identity})
                write_new(self.run_root / 'LiveCli' / (run.name + '.json'),
                    {'cliProcess': cli_identity, 'run': str(run), 'callId': call_id,
                     'operation': operation, 'controlSessionId': identity['controlSessionId'],
                     'state': 'unknown', 'noTerminationRequested': True})
                return {'cliExitCode': 5, 'executionState': 'unknown', 'run': str(run),
                        'callId': call_id, 'operation': operation, 'controlSessionId': identity['controlSessionId'],
                        'cliProcess': cli_identity,
                        'errors': [{'code': 'NFMCP_CLI_DEADLINE', 'message': 'CLI preserved; query original native call. No replay.'}]}
            capture = self.run_root / 'CliCapture' / (run.name + '.json')
            value = strict_json(stdout) if 0 < len(stdout) <= MAX_REPORT else None
            if not isinstance(value, dict) or value.get('cliExitCode') != process.returncode:
                value = {'cliExitCode': 5, 'executionState': 'unknown', 'callId': call_id,
                         'operation': operation, 'run': str(run), 'controlSessionId': identity['controlSessionId'],
                         'errors': [{'code': 'NFMCP_CLI_OUTPUT'}]}
            write_new(capture, {'summary': value, 'stdoutSha256': sha(stdout), 'stdoutBytes': len(stdout),
                                'stderrSha256': sha(stderr), 'stderrBytes': len(stderr), 'exitCode': process.returncode})
            return value
        except (OSError, ValueError, TransportError):
            return {'cliExitCode': 5, 'executionState': 'unknown', 'callId': call_id,
                    'operation': operation, 'run': str(run), 'controlSessionId': identity['controlSessionId'],
                    'errors': [{'code': 'NFMCP_CLI_UNCONFIRMED'}]}
        finally: self.connections.release()

    def select_connection(self, params):
        self._documents()
        if not self.config['enabled']: return result('notConnected', reason='aiDisabled', candidates=[])
        candidates, rejected = discover_editor_sessions(ROOT, self.engine)
        visible = [{k:v for k,v in row.items() if k != 'config'} for row in candidates]
        selected = candidates
        if 'project' in params:
            project = installation_path(params['project']).as_posix()
            selected = [row for row in selected if row['project'].casefold() == project.casefold()]
        if 'controlSessionId' in params:
            selected = [row for row in selected if row['controlSessionId'] == params['controlSessionId']]
        if not selected: return result('notConnected', candidates=visible, rejectedCandidates=rejected,
            reason='selectedSessionUnavailable' if params else 'noActiveEditor')
        if len(selected) != 1: return result('selectionRequired', candidates=visible,
            selectionFields=['project','controlSessionId'], rejectedCandidates=rejected)
        row = selected[0]
        with self.connection_lock:
            context = next((c for c in self.contexts.values() if c.config_path == Path(row['config'])), None)
            context = context or type(self)(row['config'])
            connected = context.connect()
            if connected['status'] == 'connected': self.contexts[connected['handle']] = context
            return connected

    def connect(self):
        if not self.connection_lock.acquire(blocking=False):
            raise BridgeError('NFMCP_BUSY', 'environmentBlocked')
        try:
            return self._connect()
        finally:
            self.connection_lock.release()

    def _connect(self):
        public = self._public()
        if self.identity is not None and public != self.identity:
            raise BridgeError('NFMCP_RECONNECT_REQUIRES_NEW_CONFIG_PROCESS', 'environmentBlocked')
        self._claim_business_lease()
        summary = self._cli('editor-describe')
        if summary.get('cliExitCode') != 0: return self._project_result(summary)
        description = summary.get('response', {}).get('result', {})
        if (description.get('identity') != public
                or any(description.get(key) != self.operations[key] for key in self.operations)):
            raise BridgeError('NFMCP_OPERATION_VERSION_MISMATCH', 'environmentBlocked')
        self.identity = public
        if self.handle is None: self.handle = uuid.uuid4().hex
        for path in (self.run_root / 'Calls').glob('*.json'):
            row = strict_json(safe_read(path, MAX_REPORT, roots=(self.run_root,)))
            if row['controlSessionId'] != public['controlSessionId']:
                raise BridgeError('NFMCP_OLD_CALL_SESSION', 'environmentBlocked')
            self.calls[row['callId']] = row
            if row['state'] in ('submitted', 'unknown', 'pending') or row.get('requiresReview'):
                self.unknown.add(row['callId'])
        return result('connected', handle=self.handle, identity=public, operations=len(description['operations']),
                      unresolvedCalls=sorted(self.unknown), docsStatus=self.manifest.get('publicationStatus'))

    def _register_native(self, summary):
        call = summary.get('response', {}).get('result', {})
        payload = call.get('result', {})
        reference = payload.get('nativeReport')
        if not isinstance(reference, dict): return []
        if (call.get('controlSessionId') != self.identity['controlSessionId']
                or not isinstance(reference.get('sha256'), str) or not re.fullmatch('[0-9a-f]{64}', reference['sha256'])
                or type(reference.get('bytes')) is not int or not 0 < reference['bytes'] <= MAX_REPORT):
            raise BridgeError('NFMCP_REPORT_BINDING', 'unknown')
        data = safe_read(Path(reference['path']), MAX_REPORT, reference['sha256'], (self.report_root,))
        if len(data) != reference['bytes']: raise BridgeError('NFMCP_REPORT_BYTES', 'unknown')
        native = strict_json(data)
        sidecar = strict_json(safe_read(Path(payload['controlEvidencePath']), MAX_REPORT,
                                         payload['controlEvidenceSha256'], (self.report_root,)))
        for key in ('controlSessionId', 'binarySha256'):
            if sidecar.get(key) != self.identity[key]: raise BridgeError('NFMCP_REPORT_IDENTITY', 'unknown')
        if (sidecar.get('callId') != call.get('callId') or sidecar.get('operation') != call.get('operation')
                or sidecar.get('host') != 'activeEditor' or sidecar.get('inputMethod') != 'editorProgram'
                or sidecar.get('draftRevision') != call.get('expectedDraftRevision')
                or native.get('pid') != self.identity['pid'] or native.get('project') != self.identity['project']
                or native.get('sessionId') != reference.get('nativeSessionId')):
            raise BridgeError('NFMCP_REPORT_CALL', 'unknown')
        expected_modes = {'catalog.get': 'GraphCatalog', 'rules.query': 'GraphRules',
                          'resources.query': 'ResourceQuery',
                          'graphs.preflight': 'GraphPreflight', 'graphs.generate': 'Import',
                          'draft.prepareCopy': 'PrepareNewCopy', 'graphs.preview': 'GraphPreview'}
        query = call['operation'] in ('catalog.get', 'rules.query', 'resources.query', 'resources.inspect', 'graphs.export')
        if not query and native.get('mode') != expected_modes.get(call['operation']): raise BridgeError('NFMCP_REPORT_MODE', 'unknown')
        if query:
            # Query sidecars contain full identity/native reference; graph sidecars deliberately differ.
            for key in ('pid', 'processStart', 'project'):
                if sidecar.get(key) != self.identity[key]: raise BridgeError('NFMCP_REPORT_IDENTITY', 'unknown')
            if Path(sidecar.get('module', '')).resolve() != self.binary.resolve():
                raise BridgeError('NFMCP_REPORT_MODULE', 'unknown')
            if (sidecar.get('nativeReport') != reference or sidecar.get('querySnapshot') != payload.get('querySnapshot')
                    or sidecar.get('queryDigest') != payload.get('queryDigest')
                    or sidecar.get('catalogDigest') != payload.get('catalogDigest')):
                raise BridgeError('NFMCP_QUERY_BINDING', 'unknown')
            snapshot = strict_json(safe_read(Path(payload['querySnapshot']), 524288, payload['queryDigest'], (self.report_root,)))
            original = self.calls.get(call['callId'])
            if original and snapshot != original['arguments']: raise BridgeError('NFMCP_QUERY_ARGUMENTS', 'unknown')
            if call['operation'] == 'resources.inspect':
                mode = {'MaterialParameters': 'MaterialParameters', 'MaterialFunction': 'MaterialFunction', 'Texture2D':'TextureInspect'}.get(snapshot.get('kind'))
            elif call['operation'] == 'graphs.export':
                mode = {'Material': 'MaterialGraphExport', 'Blueprint': 'BlueprintGraphExport'}.get(snapshot.get('assetKind'))
            else: mode = expected_modes.get(call['operation'])
            if mode is None or native.get('mode') != mode: raise BridgeError('NFMCP_REPORT_MODE', 'unknown')
            catalog = native.get('catalogDigest', '') if call['operation'] == 'resources.inspect' else native.get('catalogDigest')
            if payload.get('cliExitCode') == 0 and (native.get('operationStatus') != 'passed'
                    or catalog != payload.get('catalogDigest')):
                raise BridgeError('NFMCP_QUERY_RESULT', 'unknown')
            if payload.get('cliExitCode') == 0 and call['operation'] in ('resources.inspect', 'graphs.export'):
                resource_payload(native, snapshot, self.contract.get('graphRequestSchema'))
            if payload.get('cliExitCode') == 0 and call['operation'] == 'resources.query':
                load_installation_binding(self.project, self.stage, ROOT)
                binding_bytes = safe_read(self.project.parent / 'Config/NodeForgeInstallation.json', 65536, roots=(self.project.parent,))
                schema = self.tools['nodeforge_resources_query']['nativeDefinition']['argumentsSchema']
                resource_query_payload(native, snapshot, schema, payload['queryDigest'], self.config['binarySha256'],
                                       sha(binding_bytes), self.manifest['catalogDigest'])
        else:
            if (sidecar.get('snapshotPath') != payload.get('inputSnapshot')
                    or sidecar.get('snapshotDigest') != payload.get('inputDigest')
                    or sidecar.get('nativeReport') != reference):
                raise BridgeError('NFMCP_GRAPH_INPUT_BINDING', 'unknown')
            safe_read(Path(payload['inputSnapshot']), 65536, payload['inputDigest'], (self.report_root,))
            # Preflight can reject during parsing, before a validated FRequest
            # publishes its inputDigest. The exact report and input bytes are
            # already bound by the authenticated call's immutable sidecar.
            # Only this explicit read-only, not-written terminal rejection may
            # omit the native digest; wrong digests and generation stay strict.
            early_preflight_rejection = (
                call['operation'] == 'graphs.preflight'
                and call.get('state') == 'rejected'
                and payload.get('cliExitCode') == 2
                and native.get('overallStatus') == 'rejected'
                and native.get('phase') == 'finished'
                and native.get('requestOutcome') == 'notBuilt'
                and native.get('failedStage') == 'validation'
                and native.get('checks', {}).get('validation', {}).get('status') == 'failed'
                and native.get('residue', {}).get('diskState') == 'notWritten'
                and native.get('residue', {}).get('saveAttempted') is False
                and native.get('residue', {}).get('targetFilePresent') is False
                and native.get('residue', {}).get('ownedObject') == 'None'
                and all(native.get('checks', {}).get(stage, {}).get('status') == 'notRun'
                        for stage in ('build', 'initialReadback', 'compile', 'finalReadback', 'publish', 'save', 'persistence'))
                and 'inputDigest' not in native)
            if not early_preflight_rejection and native.get('inputDigest') != payload.get('inputDigest'):
                raise BridgeError('NFMCP_GRAPH_INPUT_BINDING', 'unknown')
            if call['operation'] in ('draft.prepareCopy', 'graphs.preview'):
                draft = payload.get('draft', {})
                if (sidecar.get('nativeReport') != reference or sidecar.get('draft') != draft
                        or sidecar.get('newDraftRevision') != draft.get('draftRevision')):
                    raise BridgeError('NFMCP_COPY_DRAFT_BINDING', 'unknown')
                if call['operation'] == 'draft.prepareCopy':
                    applied = payload.get('draftApplied')
                    if (type(applied) is not bool or sidecar.get('draftApplied') is not applied
                            or payload.get('previousDraftRevision') != call['expectedDraftRevision']
                            or payload.get('newDraftRevision') != draft.get('draftRevision')):
                        raise BridgeError('NFMCP_COPY_COMMIT_BINDING', 'unknown')
                    preparation = native.get('copyPreparation')
                    if (applied or 'copyPreparation' in payload) and payload.get('copyPreparation') != preparation:
                        raise BridgeError('NFMCP_COPY_PREPARATION_BINDING', 'unknown')
                    if applied:
                        if (not isinstance(preparation, dict) or native.get('operationStatus') != 'passed'
                                or preparation.get('sourceInputDigest') != payload['inputDigest']
                                or preparation.get('targetReserved') is not False
                                or preparation.get('assetGenerated') is not False
                                or preparation.get('sourceInputUnchanged') is not True
                                or draft.get('digest') != preparation.get('preparedInputDigest')
                                or draft.get('copyPreparationReport') != reference['path']
                                or int(draft['draftRevision']) != int(call['expectedDraftRevision']) + 1):
                            raise BridgeError('NFMCP_COPY_APPLIED_BINDING', 'unknown')
                        # Existing product writes prepared text here; it is not a new arbitrary-file tool.
                        prepared = strict_json(safe_read(Path(preparation['preparedInputPath']), MAX_GRAPH,
                            preparation['preparedInputDigest'], (self.state_root / 'Reports/PreparedCopies',)))
                        original = strict_json(safe_read(Path(payload['inputSnapshot']), MAX_GRAPH,
                            payload['inputDigest'], (self.report_root,)))
                        prepared_identity,original_identity=graph_identity(prepared),graph_identity(original)
                        if (prepared_identity['requestId'] != preparation.get('newRequestId')
                                or prepared_identity['assetPath'] != preparation.get('newAssetPath')
                                or original_identity['requestId'] != preparation.get('originalRequestId')
                                or original_identity['assetPath'] != preparation.get('originalAssetPath')
                                or prepared_identity['version'] != original_identity['version']):
                            raise BridgeError('NFMCP_COPY_IDENTITY_BINDING', 'unknown')
                    elif draft.get('draftRevision') != call['expectedDraftRevision']:
                        raise BridgeError('NFMCP_COPY_NOT_APPLIED_BINDING', 'unknown')
                    if payload.get('cliExitCode') == 0 and not applied:
                        raise BridgeError('NFMCP_COPY_SUCCESS_WITHOUT_COMMIT', 'unknown')
                elif (draft.get('draftRevision') != call['expectedDraftRevision']
                        or draft.get('digest') != payload['inputDigest']
                        or (payload.get('cliExitCode') == 0 and (native.get('operationStatus') != 'passed'
                            or not isinstance(native.get('graphDifference'), dict)))):
                    raise BridgeError('NFMCP_PREVIEW_RESULT_BINDING', 'unknown')
        report_id = sha(encoded({'session': self.identity['controlSessionId'], 'callId': call['callId'], 'reference': reference}))
        with self.metadata_lock:
            if report_id not in self.reports and len(self.reports) >= 256: raise BridgeError('NFMCP_REPORT_CAPACITY', 'unknown')
            self.reports[report_id] = {'callId': call['callId'], 'reference': reference}
        info = {'reportId': report_id, **reference}
        summary_catalog = catalog_summary(native)
        if summary_catalog is not None: info['catalogSummary'] = summary_catalog
        if call['operation']=='resources.query' and payload.get('cliExitCode')==0:
            info['resourceQuerySummary'] = {key:native['resourceQuery'][key] for key in ('filter','complete','truncated','registryScanning','nextAfter','incompleteReason','returned')}
            info['rowsPointer']='/resourceQuery/rows'
        return [info]

    def _project_result(self, summary):
        code = summary.get('cliExitCode', 5)
        state = summary.get('executionState', 'unknown')
        value = result(state, cliExitCode=code, nativeSummary=summary)
        if self.identity is not None and 'response' in summary:
            try: value['reports'] = self._register_native(summary)
            except (BridgeError, TransportError, KeyError, ValueError, OSError) as error:
                value.update(status='unknown', cliExitCode=5, errors=[{'code': getattr(error, 'code', 'NFMCP_REPORT_UNKNOWN')}])
                self.unknown.add(summary.get('response', {}).get('result', {}).get('callId', 'reportUnknown'))
        return value

    def _record(self, call_id, row, new=False):
        path = self.run_root / 'Calls' / (call_id + '.json')
        with self.metadata_lock:
            (write_new if new else replace_owned)(path, row)
            self.calls[call_id] = row

    def business(self, tool, params, retained_recovery=False):
        self._bound(params['handle'], retained_recovery)
        definition = tool['nativeDefinition']; operation = tool['operation']
        arguments = params['arguments']; revision = params.get('expectedDraftRevision')
        gt = definition['expectedDraftRevisionRequired']
        if revision is not None and int(revision) > 18446744073709551615: raise BridgeError('NFMCP_REVISION_UINT64')
        if operation == 'draft.replace' and not 0 < len(arguments['text'].encode('utf-8')) <= 65536:
            raise BridgeError('NFMCP_GRAPH_BYTES')
        fingerprint = sha(encoded({'operation': operation, 'arguments': arguments, 'revision': revision}))
        call_id = params.get('callId')
        if gt:
            if not self.business_lock.acquire(blocking=False): raise BridgeError('NFMCP_BUSY')
            try:
                if call_id in self.calls:
                    if self.calls[call_id]['fingerprint'] != fingerprint: raise BridgeError('NFMCP_CALL_CONFLICT')
                    return self.business(self.tools['nodeforge_calls_get'],
                                         {'handle': params['handle'], 'arguments': {'callId': call_id}})
                if self.unknown: raise BridgeError('NFMCP_UNRESOLVED_CALL', 'unknown')
                row = {'callId': call_id, 'controlSessionId': self.identity['controlSessionId'],
                       'fingerprint': fingerprint, 'operation': operation, 'arguments': arguments,
                       'expectedDraftRevision': revision, 'state': 'submitted'}
                self._record(call_id, row, new=True)
                self.unknown.add(call_id)
                try:
                    summary = self._cli('editor-call', operation, arguments, call_id, revision)
                    value = self._project_result(summary)
                except (BridgeError, TransportError, OSError, ValueError, KeyError, TypeError) as error:
                    summary = {'cliExitCode': 5, 'executionState': 'unknown', 'callId': call_id,
                               'operation': operation, 'errors': [{'code': getattr(error, 'code', 'NFMCP_SUBMITTED_UNKNOWN')}]}
                    value = self._project_result(summary)
                row.update(state=value['status'], summary=summary)
                if value['status'] not in ('unknown', 'pending', 'stopping') and value.get('cliExitCode') in (0, 2, 3):
                    self.unknown.discard(call_id)
                row['requiresReview'] = call_id in self.unknown
                value['unresolvedCalls'] = sorted(self.unknown)
                self._record(call_id, row)
                return value
            finally: self.business_lock.release()
        summary = self._cli('editor-call', operation, arguments)
        value = self._project_result(summary)
        if operation == 'calls.get':
            original = arguments['callId']
            native_call = summary.get('response', {}).get('result', {})
            if native_call.get('callId') == original and native_call.get('state') in ('completed', 'rejected', 'cancelled') and value.get('cliExitCode') in (0, 2):
                self.unknown.discard(original)
                if original in self.calls:
                    row = copy.deepcopy(self.calls[original]); row.update(state=value['status'], summary=summary, requiresReview=False)
                    self._record(original, row)
            self._collect_late()
        return value

    def _collect_late(self):
        for pending in list(self.pending_cli):
            process = pending['process']
            if process.poll() is None: continue
            try:
                stdout, stderr = process.communicate(timeout=.05)
            except subprocess.TimeoutExpired: continue
            summary = None
            try:
                if 0 < len(stdout) <= MAX_REPORT: summary = strict_json(stdout)
            except (TransportError, ValueError, UnicodeError): pass
            write_new(self.run_root / 'LateCli' / (pending['run'].name + '.json'),
                {'cliProcess': pending['identity'], 'run': str(pending['run']), 'summary': summary,
                 'stdoutSha256': sha(stdout), 'stdoutBytes': len(stdout),
                 'stderrSha256': sha(stderr), 'stderrBytes': len(stderr), 'exitCode': process.returncode})
            self.pending_cli.remove(pending)

    def call(self, name, params, *, retained_recovery=False):
        try:
            if name not in self.tools: raise BridgeError('NFMCP_TOOL_UNAVAILABLE')
            tool = self.tools[name]
            Draft202012Validator(tool['inputSchema']).validate(params)
            if len(encoded(params)) > 524288: raise BridgeError('NFMCP_ARGUMENT_BYTES')
            strict_json(encoded(params))
            if retained_recovery and not self.retained_query(name, params):
                raise BridgeError('NFMCP_RETAINED_QUERY_UNAVAILABLE', 'environmentBlocked')
            if self.discovery and (tool['operation'] is not None or name == 'nodeforge_report_read'):
                context = self.contexts.get(params.get('handle'))
                if context is None: raise BridgeError('NFMCP_HANDLE', 'environmentBlocked')
                self.unknown = set().union(*(c.unknown for c in self.contexts.values()))
                if tool['operation'] not in (None, 'calls.get') and tool.get('nativeDefinition', {}).get('dispatch') == 'gameThread':
                    if any(c.has_unresolved() for c in self.contexts.values()):
                        raise BridgeError('NFMCP_UNRESOLVED_CALL', 'unknown')
                value = context.call(name, params, retained_recovery=retained_recovery)
                return value
            if name == 'nodeforge_connect' and self.discovery: return self.select_connection(params)
            if tool['operation'] is not None: return self.business(tool, params, retained_recovery)
            if name == 'nodeforge_connect':
                if ('project' in params and installation_path(params['project']) != self.project
                        or 'controlSessionId' in params and params['controlSessionId'] != self._public()['controlSessionId']):
                    raise BridgeError('NFMCP_SELECTED_SESSION_MISMATCH', 'environmentBlocked')
                return self.connect()
            documents, documents_sha = ((self.manifest, self.config['docsManifestSha256'])
                if retained_recovery else self._documents())
            if name == 'nodeforge_docs_index':
                # Keep discovery bounded; node details are reached through generated family indices.
                index_fields = ('format', 'bundleId', 'bridgeVersion', 'buildId', 'engine', 'adapter',
                    'editorApiVersion', 'profile', 'binarySha256', 'catalogDigest', 'installationSchemaVersion', 'stage',
                    'counts', 'reportNamespace', 'publicationStatus', 'fullUsagePackageComplete',
                    'readerEntrypoints', 'developerKnowledgeComplete', 'actualCodexBusinessAcceptance')
                manifest = {key: documents[key] for key in index_fields if key in documents}
                manifest['pages'] = [row for row in documents['pages'] if row.get('kind') != 'node']
                value = result('passed', manifest=manifest, totalPages=len(documents['pages']),
                              docsManifestSha256=documents_sha,
                              indexScope='guidesAndFamilyIndices', nodePagesListed=False)
                if len(encoded(value)) > MAX_SELECTED:
                    return result('resultTooLarge', selectionReturned=False, limitBytes=MAX_SELECTED,
                        docsManifestSha256=documents_sha,
                        errors=[{'code':'NFMCP_DISCOVERY_INDEX_TOO_LARGE'}])
                return value
            if name == 'nodeforge_docs_read':
                entries = [row for row in documents['pages'] if row['pageId'] == params['pageId']]
                if len(entries) != 1: raise BridgeError('NFMCP_DOCUMENT_UNAVAILABLE')
                entry = entries[0]
                text = safe_read(ROOT / entry['path'], 65536, entry['sha256']).decode('utf-8')
                if len(text.encode('utf-8')) != entry['bytes']: raise BridgeError('NFMCP_DOCUMENT_BYTES', 'environmentBlocked')
                return result('passed', bundleId=self.bundle_id, pageId=entry['pageId'], text=text,
                              sha256=entry['sha256'], docsManifestSha256=documents_sha)
            if name == 'nodeforge_report_read':
                self._bound(params['handle'], retained_recovery)
                entry = self.reports.get(params['reportId'])
                if entry is None: raise BridgeError('NFMCP_REPORT_UNAVAILABLE')
                reference = entry['reference']
                try:
                    native = strict_json(safe_read(Path(reference['path']), MAX_REPORT, reference['sha256'], (self.report_root,)))
                except (BridgeError, TransportError, OSError, ValueError) as error:
                    self.unknown.add(entry['callId'])
                    if entry['callId'] in self.calls:
                        row = copy.deepcopy(self.calls[entry['callId']]); row['state'] = 'unknown'
                        self._record(entry['callId'], row)
                    raise BridgeError(getattr(error, 'code', 'NFMCP_REPORT_UNKNOWN'), 'unknown') from error
                selected = json_pointer(native, params['pointer'])
                if params['pointer'] == '/exportedRequest':
                    try:
                        selected, text, digest = export_payload(native, self.contract.get('graphRequestSchema'))
                    except (BridgeError, TransportError, ValueError) as error:
                        self.unknown.add(entry['callId'])
                        raise BridgeError(getattr(error, 'code', 'NFMCP_EXPORT_PAYLOAD'), 'unknown') from error
                    read = native['resourceRead']
                    return result('passed', reportId=params['reportId'], callId=entry['callId'], pointer=params['pointer'],
                        sha256=reference['sha256'], value=selected, text=text, payloadSha256=digest,
                        sourceRef={'exportReportPath': reference['path'], 'exportReportSha256': reference['sha256'],
                            'source': read['source'], 'sourceAssetSha256': read['savedSourceSha256'], 'assetKind': read['kind'], 'payloadSha256': digest})
                if len(encoded(selected)) > MAX_SELECTED:
                    navigation = {'keys': list(selected)} if isinstance(selected, dict) else {'length': len(selected)} if isinstance(selected, list) else {'kind': type(selected).__name__}
                    return result('resultTooLarge', reportId=params['reportId'], pointer=params['pointer'],
                                  selectionReturned=False, limitBytes=MAX_SELECTED, navigation=navigation)
                return result('passed', reportId=params['reportId'], callId=entry['callId'], pointer=params['pointer'],
                              sha256=reference['sha256'], value=selected)
            raise BridgeError('NFMCP_TOOL_UNAVAILABLE')
        except Exception as error:
            from jsonschema.exceptions import ValidationError
            if isinstance(error, ValidationError): code, status = 'NFMCP_SCHEMA', 'rejected'
            elif isinstance(error, BridgeError): code, status = error.code, error.status
            elif isinstance(error, (TransportError, OSError, ValueError, KeyError, TypeError)):
                code, status = getattr(error, 'code', 'NFMCP_LOCAL_PRECONDITION'), 'environmentBlocked'
            else: raise
            return result(status, errors=[{'code': code}], unresolvedCalls=sorted(self.unknown))
