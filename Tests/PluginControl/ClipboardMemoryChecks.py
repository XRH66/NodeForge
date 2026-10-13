"""Failure cleanup regression with controlled fake Win32 memory; no desktop writes."""
from __future__ import annotations
import ctypes
from ctypes import wintypes
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'Scripts'))
from EditorClipboardMemory import ClipboardMemory
from EditorAuxUiCases import read_exact_text


class FakeKernel:
    def __init__(self, fail_at=0):
        self.buffers, self.freed, self.fail_at = {}, [], fail_at
        self.allocations = 0

    def GetModuleHandleW(self, unused):
        return 1

    def GlobalAlloc(self, flags, size):
        self.allocations += 1
        if self.allocations == self.fail_at:
            return None
        handle = self.allocations
        self.buffers[handle] = ctypes.create_string_buffer(size)
        return handle

    def GlobalLock(self, handle):
        return ctypes.addressof(self.buffers[handle])

    def GlobalUnlock(self, handle):
        return True

    def GlobalFree(self, handle):
        self.freed.append(handle)
        return None


class FakeUser:
    def __init__(self, kernel, owner_pid=123, sequence=2, text='report\r\n'):
        self.kernel, self.owner_pid, self.sequence, self.text = kernel, owner_pid, sequence, text
        self.formats, self.transferred = {13: (text+'\0').encode('utf-16-le')}, []
        self.emptied = self.destroyed = 0

    def CreateWindowExW(self, *args):
        return 42

    def DestroyWindow(self, window):
        self.destroyed += 1
        return True

    def OpenClipboard(self, window):
        return True

    def CloseClipboard(self):
        return True

    def GetClipboardSequenceNumber(self):
        return self.sequence

    def GetClipboardOwner(self):
        return 7

    def GetWindowThreadProcessId(self, window, pid_pointer):
        ctypes.cast(pid_pointer, ctypes.POINTER(wintypes.DWORD))[0] = self.owner_pid
        return 1

    def EmptyClipboard(self):
        self.emptied += 1
        self.formats.clear()
        self.sequence += 1
        return True

    def SetClipboardData(self, fmt, handle):
        self.formats[fmt] = self.kernel.buffers[handle].raw
        self.transferred.append(handle)
        self.sequence += 1
        return handle


class FakeClipboard(ClipboardMemory):
    def __init__(self, out, owner_pid=123, fail_at=0, pending='report\n'):
        self.out = out
        self.identity = {'pid': 123, 'processStart': '456', 'controlSessionId': 'ControlledFixture',
            'module': str(Path(__file__).resolve()), 'binarySha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
        self.items = [(13, 'ControlledOriginal\0'.encode('utf-16-le')), (50000, b'ControlledHTML\0')]
        self.original_sequence = self.last_sequence = 1
        self.known_texts = {'report\n'}
        self.pending_text, self.finished = pending, False
        self.kernel = FakeKernel(fail_at)
        self.user = FakeUser(self.kernel, owner_pid)

    def _current_text(self):
        return self.user.text

    def _bytes(self, fmt):
        return self.user.formats[fmt]


class ClipboardFailureChecks(unittest.TestCase):
    def exercise(self, assertion, owner_pid=123, fail_at=0, actual_state=None, module=None):
        with tempfile.TemporaryDirectory(prefix='NFClipboard', dir=ROOT/'Reports/PluginControlStageTwo') as tmp:
            clipboard = FakeClipboard(Path(tmp), owner_pid, fail_at)
            state = actual_state or {'state': 'running', 'createdTicks': '456'}
            selected_module = Path(__file__).resolve() if module is None else module
            with patch('EditorClipboardMemory.process_identity', return_value=state), \
                    patch('EditorClipboardMemory.observed_module', return_value=selected_module):
                assertion(clipboard)

    def test_failed_content_assertion_restores_all_original_formats(self):
        def check(c):
            original = dict(c.items)
            with self.assertRaisesRegex(RuntimeError, 'does not match'):
                c.after_write('report\n')
            c.finish()
            self.assertEqual(c.user.formats, original)
            self.assertEqual(c.kernel.freed, [])
            witness = json.loads((c.out/'ClipboardMemoryRestore.json').read_text('utf-8'))
            self.assertTrue(witness['verifiedAllOriginalPayloadBytes'])
            self.assertTrue(witness['controlledOwnerVerified'])
            self.assertNotIn('ControlledOriginal', json.dumps(witness))
            c.finish()
            self.assertEqual(c.user.emptied, 1)
        self.exercise(check)

    def test_external_owner_is_preserved(self):
        def check(c):
            before = dict(c.user.formats)
            c.finish()
            self.assertEqual(c.user.formats, before)
            self.assertEqual(c.user.emptied, 0)
            witness = json.loads((c.out/'ClipboardMemoryRestore.json').read_text('utf-8'))
            self.assertEqual(witness['mode'], 'externalChangePreserved')
            self.assertFalse(witness['verifiedAllOriginalPayloadBytes'])
        self.exercise(check, owner_pid=999)

    def test_reused_pid_is_preserved(self):
        self.exercise(lambda c: (c.finish(), self.assertEqual(c.user.emptied, 0)),
            actual_state={'state': 'running', 'createdTicks': 'DifferentCreation'})

    def test_unknown_owner_identity_fails_without_overwrite(self):
        def check(c):
            with self.assertRaisesRegex(RuntimeError, 'cannot be verified'):
                c.finish()
            self.assertEqual(c.user.emptied, 0)
            self.assertTrue(c.items)
            witness = json.loads((c.out/'ClipboardMemoryRestore.json').read_text('utf-8'))
            self.assertEqual(witness['operationStatus'], 'failed')
        self.exercise(check, actual_state={'state': 'unknown'})

    def test_allocation_failure_keeps_current_clipboard(self):
        def check(c):
            with self.assertRaisesRegex(RuntimeError, 'allocation failed'):
                c.finish()
            self.assertEqual(c.user.emptied, 0)
            self.assertEqual(c.kernel.freed, [1])
            self.assertEqual(c.user.destroyed, 1)
            self.assertTrue(c.items)
        self.exercise(check, fail_at=2)

    def test_raw_text_preserves_crlf_and_bom_handling(self):
        with tempfile.TemporaryDirectory(prefix='NFText', dir=ROOT/'Reports/PluginControlStageTwo') as tmp:
            path = Path(tmp)/'Controlled.txt'
            path.write_bytes(b'\xef\xbb\xbfreport\r\n')
            self.assertEqual(read_exact_text(path), 'report\r\n')
            self.assertNotEqual(path.read_text('utf-8-sig'), read_exact_text(path))


if __name__ == '__main__':
    unittest.main(verbosity=2)
