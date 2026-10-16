"""Read-only Windows x64 process discovery, isolated by the caller's time budget.

Toolhelp enumerates candidates; winternl PEB/RTL_USER_PROCESS_PARAMETERS supplies
the actual command line. No WMI provider, shell, process termination, or retries.
The x64 layouts follow the Windows SDK winternl.h, not a scan of process memory.
"""
import ctypes
from ctypes import wintypes as w
import json
import sys
import time


class QueryError(Exception):
    def __init__(self, stage, reason, pid=None):
        self.detail = dict(stage=stage, reason=reason)
        if pid is not None:
            self.detail['pid'] = pid
        super().__init__(reason)


class ProcessEntry(ctypes.Structure):
    _fields_ = [('dwSize', w.DWORD), ('cntUsage', w.DWORD), ('th32ProcessID', w.DWORD),
                ('th32DefaultHeapID', ctypes.c_size_t), ('th32ModuleID', w.DWORD),
                ('cntThreads', w.DWORD), ('th32ParentProcessID', w.DWORD),
                ('pcPriClassBase', w.LONG), ('dwFlags', w.DWORD), ('szExeFile', w.WCHAR * 260)]


class UnicodeString(ctypes.Structure):
    _fields_ = [('Length', w.USHORT), ('MaximumLength', w.USHORT), ('Buffer', ctypes.c_void_p)]


class BasicInformation(ctypes.Structure):
    _fields_ = [('ExitStatus', w.LONG), ('PebBaseAddress', ctypes.c_void_p),
                ('AffinityMask', ctypes.c_size_t), ('BasePriority', w.LONG),
                ('UniqueProcessId', ctypes.c_size_t), ('ParentId', ctypes.c_size_t)]


class Peb(ctypes.Structure):
    _fields_ = [('Reserved1', w.BYTE * 2), ('BeingDebugged', w.BYTE), ('Reserved2', w.BYTE),
                ('Reserved3', ctypes.c_void_p * 2), ('Ldr', ctypes.c_void_p),
                ('ProcessParameters', ctypes.c_void_p)]


class Parameters(ctypes.Structure):
    _fields_ = [('Reserved1', w.BYTE * 16), ('Reserved2', ctypes.c_void_p * 10),
                ('ImagePathName', UnicodeString), ('CommandLine', UnicodeString)]


def snapshot():
    if sys.platform != 'win32' or ctypes.sizeof(ctypes.c_void_p) != 8:
        raise QueryError('platform', 'Windows x64 Python is required for UE5.3 Win64 discovery')
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    ntdll = ctypes.WinDLL('ntdll')
    kernel.CreateToolhelp32Snapshot.argtypes = [w.DWORD, w.DWORD]
    kernel.CreateToolhelp32Snapshot.restype = w.HANDLE
    for name in ('Process32FirstW', 'Process32NextW'):
        call = getattr(kernel, name)
        call.argtypes = [w.HANDLE, ctypes.POINTER(ProcessEntry)]
        call.restype = w.BOOL
    kernel.OpenProcess.argtypes = [w.DWORD, w.BOOL, w.DWORD]
    kernel.OpenProcess.restype = w.HANDLE
    kernel.CloseHandle.argtypes = [w.HANDLE]
    kernel.CloseHandle.restype = w.BOOL
    kernel.ReadProcessMemory.argtypes = [w.HANDLE, ctypes.c_void_p, ctypes.c_void_p,
                                       ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
    kernel.ReadProcessMemory.restype = w.BOOL
    kernel.IsWow64Process.argtypes = [w.HANDLE, ctypes.POINTER(w.BOOL)]
    kernel.IsWow64Process.restype = w.BOOL
    kernel.QueryFullProcessImageNameW.argtypes = [w.HANDLE, w.DWORD, w.LPWSTR, ctypes.POINTER(w.DWORD)]
    kernel.QueryFullProcessImageNameW.restype = w.BOOL
    ntdll.NtQueryInformationProcess.argtypes = [w.HANDLE, w.ULONG, ctypes.c_void_p,
                                             w.ULONG, ctypes.POINTER(w.ULONG)]
    ntdll.NtQueryInformationProcess.restype = w.LONG

    def fail(stage, pid=None):
        raise QueryError(stage, 'WinError ' + str(ctypes.get_last_error()), pid)

    def read(handle, address, value, stage, pid):
        count = ctypes.c_size_t()
        if not address or not kernel.ReadProcessMemory(handle, address, ctypes.byref(value),
                                                       ctypes.sizeof(value), ctypes.byref(count)):
            fail(stage, pid)
        if count.value != ctypes.sizeof(value):
            raise QueryError(stage, 'Incomplete process memory read', pid)
        return value

    handle = kernel.CreateToolhelp32Snapshot(0x2, 0)  # TH32CS_SNAPPROCESS
    if handle == ctypes.c_void_p(-1).value:
        fail('snapshot')
    candidates = []
    try:
        entry = ProcessEntry()
        entry.dwSize = ctypes.sizeof(entry)
        valid = kernel.Process32FirstW(handle, ctypes.byref(entry))
        while valid:
            if entry.szExeFile.lower() in ('unrealeditor.exe', 'unrealeditor-cmd.exe'):
                candidates.append((entry.th32ProcessID, entry.szExeFile))
            valid = kernel.Process32NextW(handle, ctypes.byref(entry))
        if ctypes.get_last_error() != 18:  # ERROR_NO_MORE_FILES is the only complete snapshot.
            fail('enumeration')
    finally:
        kernel.CloseHandle(handle)

    result = []
    for pid, name in candidates:
        handle = kernel.OpenProcess(0x400 | 0x10, False, pid)  # QUERY_INFORMATION | VM_READ
        if not handle:
            # A candidate disappearing during discovery is inconclusive. Do not call it idle.
            fail('openProcess', pid)
        try:
            wow64 = w.BOOL()
            if not kernel.IsWow64Process(handle, ctypes.byref(wow64)):
                fail('architecture', pid)
            if wow64.value:
                raise QueryError('architecture', 'Expected native Win64 UE process', pid)
            image = ctypes.create_unicode_buffer(32768)
            size = w.DWORD(len(image))
            if not kernel.QueryFullProcessImageNameW(handle, 0, image, ctypes.byref(size)):
                fail('imagePath', pid)
            if image.value.replace('\\', '/').rsplit('/', 1)[-1].lower() != name.lower():
                raise QueryError('imagePath', 'Process identity changed during snapshot', pid)
            basic = BasicInformation()
            returned = w.ULONG()
            status = ntdll.NtQueryInformationProcess(handle, 0, ctypes.byref(basic),
                                                    ctypes.sizeof(basic), ctypes.byref(returned))
            if status < 0:
                raise QueryError('basicInformation', 'NTSTATUS 0x%08x' % (status & 0xffffffff), pid)
            if basic.UniqueProcessId != pid:
                raise QueryError('basicInformation', 'PID changed during discovery', pid)
            peb = read(handle, basic.PebBaseAddress, Peb(), 'peb', pid)
            params = read(handle, peb.ProcessParameters, Parameters(), 'parameters', pid)
            value = params.CommandLine
            if not value.Buffer or not 0 < value.Length <= value.MaximumLength <= 65534 or value.Length % 2:
                raise QueryError('commandLine', 'Missing or invalid bounded UTF-16 command line', pid)
            raw = read(handle, value.Buffer, (ctypes.c_ubyte * value.Length)(), 'commandLine', pid)
            line = bytes(raw).decode('utf-16-le', errors='strict')
            if '\0' in line:
                raise QueryError('commandLine', 'Embedded NUL in command line', pid)
            result.append(dict(ProcessId=pid, CommandLine=line, ExecutablePath=image.value))
        finally:
            kernel.CloseHandle(handle)
    return result


def main():
    started = time.monotonic()
    try:
        result = dict(status='succeeded', processes=snapshot(), method='Toolhelp32+winternl-x64')
    except QueryError as error:
        result = dict(status='failed', error=error.detail)
    except (OSError, ValueError, AttributeError, UnicodeError) as error:
        result = dict(status='failed', error=dict(stage='nativeQuery', reason=str(error)))
    result['elapsedSeconds'] = time.monotonic() - started
    print(json.dumps(result, ensure_ascii=True))
    return 0 if result['status'] == 'succeeded' else 3


if __name__ == '__main__':
    sys.exit(main())
