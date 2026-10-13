"""Fixed S4 auxiliary GUI checks; no arbitrary window or business operations."""
from __future__ import annotations
import ctypes
from ctypes import wintypes
import hashlib
import json
import time
from pathlib import Path
from RunGraphRegression import sha, ref, write_new, REQUIRED_IMPORT_CHECKS, readback_errors
from RunProcessGuard import process_identity, observed_module
from EditorClipboardMemory import ClipboardMemory


def read_exact_text(path):
    # Match FFileHelper text exactly; read_text silently rewrites CRLF to LF.
    return Path(path).read_bytes().decode('utf-8-sig')


def run_cases(root, out, folder, identity, process, client, guard, asset_snapshot, allow_asset, freeze_asset):
    clipboard = ClipboardMemory(out, identity)
    try:
        return run_owned_cases(root, out, folder, identity, process, client, guard,
            asset_snapshot, allow_asset, freeze_asset, clipboard)
    finally:
        clipboard.finish()


def run_owned_cases(root, out, folder, identity, process, client, guard, asset_snapshot, allow_asset, freeze_asset, clipboard):
    rows, results = [], []
    revision = '0'
    read = lambda path: json.loads(Path(path).read_text('utf-8-sig'))
    text_digest = lambda text: hashlib.sha256(text.encode('utf-8')).hexdigest()

    def require(case, condition, evidence=None):
        row = {'case': case, 'passed': bool(condition), 'evidence': evidence or {}}
        rows.append(row)
        write_new(out / (case + 'AuxCheck.json'), row)
        if not condition:
            raise RuntimeError('Aux UI gate failed; preserve original evidence and stop writes: ' + case)

    def call(case, method, arguments=None, timeout=30):
        extra = ['--operation', method, '--call-id', case]
        if method != 'draft.get':
            extra += ['--expected-revision', revision]
        if arguments is not None:
            path = out / (case + 'Arguments.json')
            write_new(path, arguments)
            extra += ['--arguments-file', str(path)]
        return client(case, 'editor-call', extra, call_timeout=timeout)['response']['result']

    def draft(case):
        return call(case, 'draft.get')

    def wait_json(path, seconds, observe=None):
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            if (out / 'AuxUiFailure.json').is_file():
                raise RuntimeError('Native auxiliary failure observed; restore clipboard and do not repeat action')
            if observe:
                observe()
            try:
                return read(path)
            except (OSError, json.JSONDecodeError):
                if process.poll() is not None:
                    raise RuntimeError('Owned editor actually exited; preserve native failure, do not repeat action')
                time.sleep(.1)
        raise RuntimeError('Aux marker missing at bounded deadline; do not replay UI action')

    def dialog_cancel_observer():
        """One WM_CLOSE only on this PID's exact, owned, observed file dialog."""
        user = ctypes.WinDLL('user32', use_last_error=True)
        callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
        user.EnumWindows.argtypes = [callback_type, wintypes.LPARAM]
        user.EnumWindows.restype = wintypes.BOOL
        user.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
        user.GetWindowThreadProcessId.restype = wintypes.DWORD
        user.GetWindow.argtypes = [wintypes.HWND, wintypes.UINT]
        user.GetWindow.restype = wintypes.HWND
        for name in ('GetClassNameW', 'GetWindowTextW'):
            fn = getattr(user, name)
            fn.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
            fn.restype = ctypes.c_int
        user.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
        user.PostMessageW.restype = wintypes.BOOL
        started, posted = time.monotonic(), False

        def observe():
            nonlocal posted
            if posted:
                return
            if time.monotonic() - started > 15:
                raise RuntimeError('Exact owned file dialog not observed before deadline; no window message sent')
            ready_path = out / 'FileCancelDialogReady.json'
            if not ready_path.is_file():
                return
            ready = read(ready_path)
            expected_title = '载入 NodeForge JSON（不会生成资产）'
            if (ready['pid'] != process.pid or ready['controlSessionId'] != identity['controlSessionId']
                    or ready['title'] != expected_title or not str(ready['parentHwnd']).isdigit()):
                raise RuntimeError('Native dialog ownership marker mismatch; no action')
            parent = int(ready['parentHwnd'])
            if not parent:
                raise RuntimeError('Nonzero exact owner HWND required')
            matches = []

            @callback_type
            def enum(window, unused):
                pid = wintypes.DWORD()
                if not user.GetWindowThreadProcessId(window, ctypes.byref(pid)) or pid.value != process.pid:
                    return True  # No other process's title/class is read or retained.
                cls, title = ctypes.create_unicode_buffer(64), ctypes.create_unicode_buffer(256)
                user.GetClassNameW(window, cls, len(cls))
                if cls.value != '#32770' or user.GetWindow(window, 4) != parent:
                    return True
                user.GetWindowTextW(window, title, len(title))
                if title.value == expected_title:
                    matches.append(int(window))
                return True

            if not user.EnumWindows(enum, 0):
                raise RuntimeError('Owned dialog enumeration failed; no action')
            if not matches:
                return
            if len(matches) != 1:
                raise RuntimeError('Unique exact owned dialog required; no action')
            guard()
            actual = process_identity(process.pid)
            module = observed_module(process.pid, 'UnrealEditor-NodeForgeEditor.dll')
            window, owner_pid, window_pid = matches[0], wintypes.DWORD(), wintypes.DWORD()
            owner_tid = user.GetWindowThreadProcessId(parent, ctypes.byref(owner_pid))
            window_tid = user.GetWindowThreadProcessId(window, ctypes.byref(window_pid))
            if (actual.get('state') != 'running' or actual.get('createdTicks') != identity['processStart']
                    or module is None or module.resolve() != Path(identity['module']).resolve()
                    or sha(module) != identity['binarySha256'] or not owner_tid or not window_tid
                    or owner_pid.value != process.pid or window_pid.value != process.pid
                    or user.GetWindow(window, 4) != parent):
                raise RuntimeError('Live PID/start/module/owner changed; no action')
            posted = bool(user.PostMessageW(window, 0x0010, 0, 0))
            write_new(out / 'OwnedDialogCancellation.json', {'pid': process.pid,
                'processStart': identity['processStart'], 'moduleSha256': identity['binarySha256'],
                'dialogHwnd': str(window), 'parentHwnd': str(parent), 'class': '#32770',
                'title': expected_title, 'singleCloseMessagePosted': posted,
                'cancellationNotYetProven': True})
            if not posted:
                raise RuntimeError('Owned WM_CLOSE posting failed; no second attempt')
        return observe

    def gui(case):
        nonlocal revision
        guard()
        before_assets = asset_snapshot()
        current = draft(case + 'BeforeDraft')
        revision = current['draftRevision']
        known_clipboard = None
        if case == 'PasteValid':
            known_clipboard = read_exact_text(root/'Examples/GraphV2Samples/BlueprintConcat.json')
        elif case == 'PasteReject':
            known_clipboard = ''
        elif case in ('CopyReadOnly', 'CopyReport'):
            known_clipboard = read_exact_text(read(out/'RulesMaterialUiDone.json')['after']['nativeReport'])
        sequence = clipboard.before_write(known_clipboard) if known_clipboard is not None else None
        write_new(out / (case + 'UiReady.json'), {'controlSessionId': identity['controlSessionId'],
            'draftRevision': revision, 'inputDigest': current['digest'], 'clipboardSequence': sequence})
        observer = dialog_cancel_observer() if case == 'FileCancel' else None
        proof = wait_json(out / (case + 'UiDone.json'), 30, observer)
        before, after = proof['before'], proof['after']
        observed_only = case in ('BusyButtons', 'StaleAssets')
        require(case + 'ActualProductionProof', proof['operationStatus'] == 'passed'
            and proof['source'] == 'actualGui' and proof['actualVisibleProductionTab']
            and proof['actualDriverClickCompleted'] == (not observed_only)
            and proof['actionKind'] == ('observeDisabled' if observed_only else 'actualDriverClick')
            and proof['pid'] == process.pid and proof['controlSessionId'] == identity['controlSessionId']
            and before['draftRevision'] == revision and before['inputText'] == current['text']
            and before['inputDigest'] == current['digest'] and Path(proof['screenshot']).stat().st_size > 0)
        actual = draft(case + 'AfterDraft')
        require(case + 'FormalDraftEqualsActualUi', actual['text'] == after['inputText']
            and actual['draftRevision'] == after['draftRevision'] and actual['digest'] == after['inputDigest'])
        revision = actual['draftRevision']
        if known_clipboard is not None:
            clipboard.after_write(known_clipboard)
        require(case + 'NoAssetMutation', asset_snapshot() == before_assets)
        guard()
        return proof

    def preserved(case, proof, except_fields=('resultNotice',)):
        require(case + 'OriginalStatePreserved', {k:v for k,v in proof['before'].items() if k not in except_fields}
            == {k:v for k,v in proof['after'].items() if k not in except_fields})

    def ack(case, proof):
        path = proof['after']['nativeReport']
        value = {'operationStatus': 'passed', 'controlSessionId': identity['controlSessionId'],
            'nativeReportSha256': sha(Path(path)) if path else ''}
        write_new(out / (case + 'UiChecked.json'), value)
        require(case + 'NativeAckBeforeNextMutation', wait_json(out / (case + 'UiCheckedAccepted.json'), 10) == value)

    initial = call('InitialControlledDraft', 'draft.replace', {'text': '{"auxDraft":true}'})
    revision = initial['result']['draft']['draftRevision']
    source = root / 'Examples/GraphV2Samples/BlueprintConcat.json'
    proof = gui('PasteValid')
    require('PasteExactControlledFileAndOneRevision', proof['after']['inputText'] == read_exact_text(source)
        and proof['after']['inputDigest'] == sha(source)
        and int(proof['after']['draftRevision']) == int(proof['before']['draftRevision']) + 1)
    ack('PasteValid', proof)
    for case in ('PasteReject', 'FileCancel'):
        proof = gui(case)
        preserved(case, proof)
        require(case + 'ExplicitNotice', bool(proof['after']['resultNotice']))
        if case == 'FileCancel':
            observed = read(out / 'OwnedDialogCancellation.json')
            require('FileDialogActuallyCancelled', observed['singleCloseMessagePosted']
                and observed['pid'] == process.pid and observed['moduleSha256'] == identity['binarySha256']
                and proof['after']['resultNotice'] == '已取消载入；原输入、检查和结果保留。')
        ack(case, proof)
    proof = gui('RulesMaterial')
    native_path = Path(proof['after']['nativeReport'])
    require('RulesActualNativeReadonly', native_path.resolve().is_relative_to(root/'Reports')
        and read(native_path)['mode'] == 'GraphRules' and read(native_path)['operationStatus'] == 'passed'
        and read(native_path)['pid'] == process.pid and proof['after']['readOnlyLength'] > 0
        and proof['before']['inputText'] == proof['after']['inputText'])
    ack('RulesMaterial', proof)
    for case in ('CopyReadOnly', 'CopyReport'):
        proof = gui(case)
        expected = proof['before']['readOnlyDigest'] if case == 'CopyReadOnly' else text_digest(read_exact_text(proof['before']['nativeReport']))
        require(case + 'ActualClipboardExactControlledData', proof['observations']['clipboardMatchesExpected']
            and proof['observations']['controlledClipboardDigest'] == expected)
        preserved(case, proof, ('resultNotice', 'resultText') if case == 'CopyReport' else ('resultNotice',))
        ack(case, proof)
    proof = gui('BusyButtons')
    require('BusyActualButtonsDisabled', proof['observations'] and all(value is False for value in proof['observations'].values()))
    preserved('BusyButtons', proof)
    ack('BusyButtons', proof)

    before_assets = asset_snapshot()
    pref = call('AuxBaselinePreflight', 'graphs.preflight')['result']
    require('AuxBaselinePreflightNoSave', read(pref['nativeReport']['path'])['mode'] == 'GraphPreflight'
        and asset_snapshot() == before_assets)
    request = read(source)
    target = folder/'Content'/(request['assetPath'].removeprefix('/Game/')+'.uasset')
    allow_asset(target)
    generated = call('AuxBaselineGenerate', 'graphs.generate', timeout=180)['result']
    native_path, snapshot = Path(generated['nativeReport']['path']), Path(generated['inputSnapshot'])
    native = read(native_path)
    # Freeze confirmed save before checking structure or permitting another step.
    require('AuxBaselineActualSaveBytes', target.is_file() and native['checks']['save']['status'] == 'passed'
        and sha(target) == native['fileSha256'])
    freeze_asset(target, 'AuxBaseline')
    write_new(out/'SavedAssetProtection.json', {ref(target): sha(target)})
    errors = [key for key in REQUIRED_IMPORT_CHECKS if native['checks'][key]['status'] != 'passed']
    for key in ('initialReadback', 'finalReadback', 'publishedReadback'):
        errors += [key + ': ' + error for error in readback_errors(request, native.get(key))]
    require('AuxBaselineOriginalProgramSourceAndFixedReadbacks', not errors and native['mode'] == 'Import'
        and native['inputMethod'] == 'editorProgram' and 'uiInput' not in native
        and native['overallStatus'] == 'waitingPersistence' and native['pid'] == process.pid
        and sha(snapshot) == sha(source) == native['inputDigest']
        and generated['nativeReport']['sha256'] == sha(native_path)
        and native['executionFingerprint']['binarySha256'] == identity['binarySha256'], {'errors': errors})
    results.append({'case': 'AuxBaseline', 'status': 'passed', 'input': ref(snapshot),
        'inputDigest': sha(snapshot), 'pid': process.pid, 'asset': ref(target), 'assetSha256': sha(target),
        'nativeReport': ref(native_path), 'nativeSha256': sha(native_path), 'source': 'editorProgram'})
    for case, field in [('Locate', 'actualSelectedAssetMatchesCurrentResult'), ('OpenAsset', 'actualAssetEditorOpenWithoutDirtyPackage')]:
        proof = gui(case)
        require(case + 'ActualAssetAction', proof['observations'][field] is True
            and proof['after']['assetPath'] == request['assetPath'] and not proof['after']['resultStale'])
        preserved(case, proof)
        ack(case, proof)
    replaced = call('StaleResultControlledDraft', 'draft.replace', {'text': '{'})
    revision = replaced['result']['draft']['draftRevision']
    proof = gui('StaleAssets')
    require('StaleActualAssetButtonsDisabled', proof['after']['resultStale']
        and all(value is False for value in proof['observations'].values()))
    preserved('StaleAssets', proof)
    ack('StaleAssets', proof)
    guard()
    disabled = client('AuxDisable', 'editor-call', ['--operation', 'control.disable', '--call-id', 'AuxDisable'])
    require('FormalDisableBeforeNormalOwnedExit', disabled['executionState'] == 'stopped'
        and not Path(identity['sessionFile']).exists())
    write_new(out/'AuxChecks.json', {'operationStatus': 'passed', 'checks': rows, 'graphs': results,
        'source': 'actualGui; disabled observations separate from clicks; formal CLI only seeds owned asset',
        'clipboardBackup': 'helper memory only; original opaque formats verified in ClipboardMemoryRestore.json'})
    return results
