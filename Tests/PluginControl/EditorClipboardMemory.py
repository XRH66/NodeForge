"""Bounded opaque Win32 clipboard preservation for the fixed auxiliary UI test."""
from __future__ import annotations
import ctypes
from ctypes import wintypes
import json
import hashlib
from pathlib import Path
from RunProcessGuard import process_identity, observed_module


class ClipboardMemory:
    def __init__(self, out, identity):
        self.out, self.identity = Path(out), identity
        self.items, self.known_texts = [], set()
        self.finished = False
        self.user = ctypes.WinDLL('user32', use_last_error=True)
        self.kernel = ctypes.WinDLL('kernel32', use_last_error=True)
        signatures = {
            'OpenClipboard': ([wintypes.HWND], wintypes.BOOL),
            'CloseClipboard': ([], wintypes.BOOL),
            'EnumClipboardFormats': ([wintypes.UINT], wintypes.UINT),
            'GetClipboardData': ([wintypes.UINT], wintypes.HANDLE),
            'EmptyClipboard': ([], wintypes.BOOL),
            'SetClipboardData': ([wintypes.UINT, wintypes.HANDLE], wintypes.HANDLE),
            'GetClipboardSequenceNumber': ([], wintypes.DWORD),
            'GetClipboardOwner': ([], wintypes.HWND),
            'GetWindowThreadProcessId': ([wintypes.HWND, ctypes.POINTER(wintypes.DWORD)], wintypes.DWORD),
            'GetClipboardFormatNameW': ([wintypes.UINT, wintypes.LPWSTR, ctypes.c_int], ctypes.c_int),
            'CreateWindowExW': ([wintypes.DWORD, wintypes.LPCWSTR, wintypes.LPCWSTR, wintypes.DWORD,
                ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, wintypes.HWND,
                wintypes.HMENU, wintypes.HINSTANCE, wintypes.LPVOID], wintypes.HWND),
            'DestroyWindow': ([wintypes.HWND], wintypes.BOOL),
        }
        for name, (args, result) in signatures.items():
            function = getattr(self.user, name)
            function.argtypes, function.restype = args, result
        for name, args, result in [
            ('GlobalSize', [wintypes.HGLOBAL], ctypes.c_size_t),
            ('GlobalLock', [wintypes.HGLOBAL], wintypes.LPVOID),
            ('GlobalUnlock', [wintypes.HGLOBAL], wintypes.BOOL),
            ('GlobalAlloc', [wintypes.UINT, ctypes.c_size_t], wintypes.HGLOBAL),
            ('GlobalFree', [wintypes.HGLOBAL], wintypes.HGLOBAL),
            ('GetModuleHandleW', [wintypes.LPCWSTR], wintypes.HMODULE),
        ]:
            function = getattr(self.kernel, name)
            function.argtypes, function.restype = args, result
        if not self.user.OpenClipboard(None):
            raise RuntimeError('Clipboard backup unavailable; no UI or clipboard write authorized')
        try:
            self.original_sequence = self.user.GetClipboardSequenceNumber()
            total, seen, fmt = 0, set(), 0
            while True:
                ctypes.set_last_error(0)
                fmt = self.user.EnumClipboardFormats(fmt)
                if not fmt:
                    if ctypes.get_last_error():
                        raise RuntimeError('Clipboard format enumeration failed; no modification')
                    break
                if fmt in seen or len(seen) >= 64 or not (fmt in (1, 7, 13, 16) or fmt >= 0xC000):
                    raise RuntimeError('Unsupported clipboard format; original clipboard remains unchanged')
                seen.add(fmt)
                name = ctypes.create_unicode_buffer(128)
                self.user.GetClipboardFormatNameW(fmt, name, len(name))
                if name.value.casefold() in ('dataobject', 'ole private data', 'oleprivatedata', 'objectlink', 'ownerlink'):
                    raise RuntimeError('Reference-based clipboard format cannot be safely copied; no modification')
                payload = self._bytes(fmt)
                total += len(payload)
                if total > 16 * 1024 * 1024:
                    raise RuntimeError('Clipboard backup memory limit exceeded; no modification')
                self.items.append((fmt, payload))  # Private opaque bytes remain in memory only.
            self.last_sequence = self.user.GetClipboardSequenceNumber()
        finally:
            self.user.CloseClipboard()

    def _bytes(self, fmt):
        handle = self.user.GetClipboardData(fmt)
        size = self.kernel.GlobalSize(handle) if handle else 0
        if not size or size > 16 * 1024 * 1024:
            raise RuntimeError('Bounded HGLOBAL clipboard data required; no private payload recorded')
        pointer = self.kernel.GlobalLock(handle)
        if not pointer:
            raise RuntimeError('Clipboard memory lock failed; private payload not recorded')
        try:
            return ctypes.string_at(pointer, size)
        finally:
            self.kernel.GlobalUnlock(handle)

    def before_write(self, known_text):
        if self.user.GetClipboardSequenceNumber() != self.last_sequence:
            raise RuntimeError('External clipboard change detected; stop before next clipboard write')
        self.known_texts.add(known_text)
        self.pending_text = known_text
        return str(self.last_sequence)

    def _current_text(self):
        try:
            value = self._bytes(13).decode('utf-16-le')
            return value.split('\x00', 1)[0]
        except (RuntimeError, UnicodeError):
            return None

    def after_write(self, known_text):
        if not self.user.OpenClipboard(None):
            raise RuntimeError('Clipboard ownership observation unavailable')
        try:
            if self._current_text() != known_text:
                raise RuntimeError('Actual clipboard does not match controlled UI data; no raw text recorded')
            self.last_sequence = self.user.GetClipboardSequenceNumber()
            self.pending_text = None
        finally:
            self.user.CloseClipboard()

    def _owned_editor_alive(self, owner_pid):
        if owner_pid != self.identity['pid']:
            return False
        actual = process_identity(owner_pid)
        if actual.get('state') == 'unknown':
            raise RuntimeError('Clipboard owner identity cannot be verified; do not overwrite current value')
        if actual.get('state') != 'running' or actual.get('createdTicks') != self.identity['processStart']:
            return False
        module = observed_module(owner_pid, 'UnrealEditor-NodeForgeEditor.dll')
        if module is None:
            raise RuntimeError('Clipboard owner module cannot be verified; do not overwrite current value')
        return (module.resolve() == Path(self.identity['module']).resolve()
            and hashlib.sha256(module.read_bytes()).hexdigest() == self.identity['binarySha256'])

    def finish(self):
        if self.finished:
            return
        self.finished = True
        window = self.user.CreateWindowExW(0, 'STATIC', 'NodeForgeClipboardRestore', 0,
            0, 0, 1, 1, wintypes.HWND(-3), None, self.kernel.GetModuleHandleW(None), None)
        if not window:
            raise RuntimeError('Hidden owned clipboard restoration window unavailable')
        handles, mode, verified, controlled_owner = [], 'failed', False, False
        try:
            if not self.user.OpenClipboard(window):
                raise RuntimeError('Clipboard restoration lock unavailable; preserve current value')
            try:
                sequence = self.user.GetClipboardSequenceNumber()
                owner = self.user.GetClipboardOwner()
                owner_pid = wintypes.DWORD()
                owned = owner and self.user.GetWindowThreadProcessId(owner, ctypes.byref(owner_pid))
                controlled_owner = bool(owned and self._owned_editor_alive(owner_pid.value))
                # Restore our own pending write even when its content assertion failed.
                # Exact live PID/start/module proves ownership independently of expected text.
                task_write = controlled_owner and (getattr(self, 'pending_text', None) is not None
                    or (sequence == self.last_sequence and self._current_text() in self.known_texts))
                if sequence == self.original_sequence:
                    verified = all(self._bytes(fmt) == payload for fmt, payload in self.items)
                    mode = 'originalUnchanged' if verified else 'failed'
                elif not task_write:
                    mode, verified = 'externalChangePreserved', True
                else:
                    # Allocate every replacement before changing any clipboard data.
                    for fmt, payload in self.items:
                        handle = self.kernel.GlobalAlloc(0x0002, len(payload))  # GMEM_MOVEABLE
                        if not handle:
                            raise RuntimeError('Restore allocation failed before clearing clipboard')
                        handles.append([fmt, handle])
                        pointer = self.kernel.GlobalLock(handle)
                        if not pointer:
                            raise RuntimeError('Restore memory lock failed before clearing clipboard')
                        try:
                            ctypes.memmove(pointer, payload, len(payload))
                        finally:
                            self.kernel.GlobalUnlock(handle)
                    if not self.user.EmptyClipboard():
                        raise RuntimeError('Owned clipboard clearing failed')
                    for entry in handles:
                        fmt, handle = entry
                        if not self.user.SetClipboardData(fmt, handle):
                            raise RuntimeError('Original format restoration failed; preserve failure evidence')
                        entry[1] = None  # Ownership transferred to Windows; never free it here.
                    verified = all(self._bytes(fmt) == payload for fmt, payload in self.items)
                    mode = 'originalFormatsRestored' if verified else 'failed'
            finally:
                self.user.CloseClipboard()
        finally:
            for fmt, handle in handles:
                if handle:
                    self.kernel.GlobalFree(handle)
            self.user.DestroyWindow(window)
            evidence = {'operationStatus': 'passed' if verified else 'failed', 'mode': mode,
                'pid': self.identity['pid'], 'controlSessionId': self.identity['controlSessionId'],
                'originalFormats': [fmt for fmt, payload in self.items],
                'originalPayloads': 'memory only; no text, bytes, lengths or digest recorded',
                'verifiedAllOriginalPayloadBytes': mode == 'originalFormatsRestored' and verified,
                'externalChangePreserved': mode == 'externalChangePreserved',
                'controlledOwnerVerified': controlled_owner}
            path = self.out / 'ClipboardMemoryRestore.json'
            with path.open('x', encoding='utf-8') as stream:
                json.dump(evidence, stream, ensure_ascii=False, indent=2)
            if verified:
                self.items.clear()
        if not verified:
            raise RuntimeError('Clipboard cleanup did not pass; auxiliary UI cannot be accepted')
