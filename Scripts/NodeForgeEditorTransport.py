"""Bounded Windows transport for the official active-editor client; no UE launch or graph rules."""
from __future__ import annotations
import ctypes
from ctypes import wintypes as w
import json
import math
import struct
import time
from pathlib import Path
from RunProcessGuard import observed_module, process_identity, validate_isolated_project, state_directory

MAX_FRAME = 524288
MAX_DESCRIPTOR = 65536
PROFILE = 'nodeforge.editor.v1'
API = '1.0'
INVALID = ctypes.c_void_p(-1).value

class TransportError(Exception):
    def __init__(self, code, exit_code=3):
        self.code, self.exit_code = code, exit_code
        super().__init__(code)  # No untrusted message, path or credential reflection.

def validate_arguments(arguments, schema):
    """Admit declared scalar arguments and the bounded conditional schema subset.

    The bound native description supplies this shape. Native dispatch still owns
    graph, source, target and revision semantics; no operation-specific rules live
    here. Optional properties must not be mistaken for forbidden extra arguments.
    """
    if not isinstance(arguments, dict):
        raise TransportError('NFEDITOR_ARGUMENTS', 2)
    properties = schema['properties']
    keys = set(arguments)
    if not set(schema['required']).issubset(keys) or not keys.issubset(properties):
        raise TransportError('NFEDITOR_ARGUMENTS', 2)
    scalar_types = {'string': str, 'boolean': bool, 'integer': int}
    if (any(spec.get('type') not in scalar_types for spec in properties.values())
            or any(type(value) is not scalar_types[properties[key]['type']]
                   for key, value in arguments.items())):
        raise TransportError('NFEDITOR_ARGUMENTS', 2)
    if any(key in keys and not set(dependencies).issubset(keys)
           for key, dependencies in schema.get('dependentRequired', {}).items()):
        raise TransportError('NFEDITOR_ARGUMENTS', 2)
    for rule in schema.get('allOf', []):
        # Consume the native declaration; do not duplicate operation field names.
        # Only const/required -> not(anyOf(required)) is needed by this contract.
        try:
            condition = rule['if']
            then = rule['then']
            excluded = then['not']
            alternatives = excluded['anyOf']
            declarations = condition.get('properties', {})
            if (set(rule) != {'if', 'then'}
                    or set(condition) - {'properties', 'required'}
                    or set(then) != {'not'} or set(excluded) != {'anyOf'}
                    or not isinstance(alternatives, list) or not alternatives
                    or any(set(spec) != {'const'} for spec in declarations.values())
                    or any(set(option) != {'required'} for option in alternatives)):
                raise TransportError('NFEDITOR_ARGUMENTS', 2)
            matches = (set(condition.get('required', [])).issubset(keys)
                and all(key not in arguments or
                    (type(arguments[key]) is type(spec['const'])
                     and arguments[key] == spec['const'])
                    for key, spec in declarations.items()))
            if matches and any(set(option['required']).issubset(keys)
                               for option in alternatives):
                raise TransportError('NFEDITOR_ARGUMENTS', 2)
        except (KeyError, TypeError, AttributeError) as error:
            raise TransportError('NFEDITOR_ARGUMENTS', 2) from error

class Overlapped(ctypes.Structure):
    _fields_ = [('Internal', ctypes.c_size_t), ('InternalHigh', ctypes.c_size_t),
        ('Offset', w.DWORD), ('OffsetHigh', w.DWORD), ('hEvent', w.HANDLE)]

class Acl(ctypes.Structure):
    _fields_ = [('revision', w.BYTE), ('zero', w.BYTE), ('size', w.WORD), ('count', w.WORD), ('zero2', w.WORD)]

def strict_json(data):
    if not data or data.startswith(b'\xef\xbb\xbf') or b'\0' in data:
        raise TransportError('NFEDITOR_ENCODING', 2)
    def pairs(rows):
        value = {}
        for key, item in rows:
            if key in value:
                raise TransportError('NFEDITOR_DUPLICATE_KEY', 2)
            value[key] = item
        return value
    value = json.loads(data.decode('utf-8', errors='strict'), object_pairs_hook=pairs,
        parse_constant=lambda _: (_ for _ in ()).throw(TransportError('NFEDITOR_NONFINITE', 2)))
    def check(item, depth):
        if depth > 32:
            raise TransportError('NFEDITOR_DEPTH', 2)
        if isinstance(item, str):
            if '\0' in item or any(0xd800 <= ord(c) <= 0xdfff for c in item):
                raise TransportError('NFEDITOR_SCALAR', 2)
        elif isinstance(item, float) and not math.isfinite(item):
            raise TransportError('NFEDITOR_NONFINITE', 2)
        elif isinstance(item, dict):
            for key, child in item.items(): check(key, depth); check(child, depth + 1)
        elif isinstance(item, list):
            for child in item: check(child, depth + 1)
    check(value, 0)
    return value

class Windows:
    def __init__(self):
        self.k = ctypes.WinDLL('kernel32', use_last_error=True)
        self.a = ctypes.WinDLL('advapi32', use_last_error=True)
        signatures = {
            'CreateFileW': ([w.LPCWSTR,w.DWORD,w.DWORD,ctypes.c_void_p,w.DWORD,w.DWORD,w.HANDLE],w.HANDLE),
            'CloseHandle': ([w.HANDLE],w.BOOL),
            'CreateEventW': ([ctypes.c_void_p,w.BOOL,w.BOOL,w.LPCWSTR],w.HANDLE),
            'ReadFile': ([w.HANDLE,ctypes.c_void_p,w.DWORD,ctypes.POINTER(w.DWORD),ctypes.POINTER(Overlapped)],w.BOOL),
            'WriteFile': ([w.HANDLE,ctypes.c_void_p,w.DWORD,ctypes.POINTER(w.DWORD),ctypes.POINTER(Overlapped)],w.BOOL),
            'GetOverlappedResult': ([w.HANDLE,ctypes.POINTER(Overlapped),ctypes.POINTER(w.DWORD),w.BOOL],w.BOOL),
            'CancelIoEx': ([w.HANDLE,ctypes.POINTER(Overlapped)],w.BOOL),
            'WaitForSingleObject': ([w.HANDLE,w.DWORD],w.DWORD),
            'WaitNamedPipeW': ([w.LPCWSTR,w.DWORD],w.BOOL),
            'GetNamedPipeServerProcessId': ([w.HANDLE,ctypes.POINTER(w.ULONG)],w.BOOL),
            'GetFileSizeEx': ([w.HANDLE,ctypes.POINTER(ctypes.c_int64)],w.BOOL),
            'GetFinalPathNameByHandleW': ([w.HANDLE,w.LPWSTR,w.DWORD,w.DWORD],w.DWORD),
            'GetFileAttributesW': ([w.LPCWSTR],w.DWORD),
            'GetCurrentProcess': ([],w.HANDLE),
            'LocalFree': ([ctypes.c_void_p],ctypes.c_void_p),
        }
        for name,(args,result) in signatures.items():
            fn=getattr(self.k,name);fn.argtypes=args;fn.restype=result
        signatures = {
            'OpenProcessToken': ([w.HANDLE,w.DWORD,ctypes.POINTER(w.HANDLE)],w.BOOL),
            'GetTokenInformation': ([w.HANDLE,ctypes.c_int,ctypes.c_void_p,w.DWORD,ctypes.POINTER(w.DWORD)],w.BOOL),
            'ConvertSidToStringSidW': ([ctypes.c_void_p,ctypes.POINTER(w.LPWSTR)],w.BOOL),
            'GetSecurityInfo': ([w.HANDLE,ctypes.c_int,w.DWORD,ctypes.POINTER(ctypes.c_void_p),ctypes.c_void_p,ctypes.POINTER(ctypes.c_void_p),ctypes.c_void_p,ctypes.POINTER(ctypes.c_void_p)],w.DWORD),
            'GetSecurityDescriptorControl': ([ctypes.c_void_p,ctypes.POINTER(w.WORD),ctypes.POINTER(w.DWORD)],w.BOOL),
            'GetAce': ([ctypes.c_void_p,w.DWORD,ctypes.POINTER(ctypes.c_void_p)],w.BOOL),
        }
        for name,(args,result) in signatures.items():
            fn=getattr(self.a,name);fn.argtypes=args;fn.restype=result
        self.user, self.logon = self.token_sids()

    def sid_text(self, sid):
        text = w.LPWSTR()
        if not self.a.ConvertSidToStringSidW(sid,ctypes.byref(text)):
            raise TransportError('NFEDITOR_SID')
        try: return text.value
        finally: self.k.LocalFree(ctypes.cast(text,ctypes.c_void_p))

    def token_sids(self):
        token=w.HANDLE()
        if not self.a.OpenProcessToken(self.k.GetCurrentProcess(),8,ctypes.byref(token)):
            raise TransportError('NFEDITOR_TOKEN')
        def info(kind):
            needed=w.DWORD();self.a.GetTokenInformation(token,kind,None,0,ctypes.byref(needed))
            if not 0 < needed.value <= MAX_DESCRIPTOR: raise TransportError('NFEDITOR_TOKEN')
            data=ctypes.create_string_buffer(needed.value)
            if not self.a.GetTokenInformation(token,kind,data,len(data),ctypes.byref(needed)): raise TransportError('NFEDITOR_TOKEN')
            return data
        class SidAttributes(ctypes.Structure):
            _fields_=[('sid',ctypes.c_void_p),('attributes',w.DWORD)]
        class Groups(ctypes.Structure):
            _fields_=[('count',w.DWORD),('first',SidAttributes)]
        try:
            user=info(1);user_sid=ctypes.cast(user,ctypes.POINTER(SidAttributes)).contents.sid
            groups=info(2); count=ctypes.cast(groups,ctypes.POINTER(w.DWORD)).contents.value
            start=ctypes.addressof(groups)+Groups.first.offset
            rows=ctypes.cast(start,ctypes.POINTER(SidAttributes))
            logons=[self.sid_text(rows[i].sid) for i in range(count) if rows[i].attributes & 0xc0000000 == 0xc0000000]
            if len(logons)!=1: raise TransportError('NFEDITOR_LOGON_SID')
            return self.sid_text(user_sid),logons[0]
        finally:self.k.CloseHandle(token)

    def no_reparse(self,path):
        path=Path(path)
        if not path.is_absolute(): raise TransportError('NFEDITOR_PATH',2)
        for item in (path,*path.parents):
            attributes=self.k.GetFileAttributesW(str(item))
            if attributes!=0xffffffff and attributes & 0x400: raise TransportError('NFEDITOR_REPARSE',2)

    def security(self,handle,kind,mask):
        owner=ctypes.c_void_p();acl=ctypes.c_void_p();sd=ctypes.c_void_p()
        if self.a.GetSecurityInfo(handle,kind,5,ctypes.byref(owner),None,ctypes.byref(acl),None,ctypes.byref(sd)):
            raise TransportError('NFEDITOR_DACL')
        try:
            control=w.WORD();revision=w.DWORD()
            if not acl.value or not owner.value or not self.a.GetSecurityDescriptorControl(sd,ctypes.byref(control),ctypes.byref(revision)):
                raise TransportError('NFEDITOR_DACL')
            if not control.value & 0x1000 or self.sid_text(owner)!=self.user or ctypes.cast(acl,ctypes.POINTER(Acl)).contents.count!=2:
                raise TransportError('NFEDITOR_DACL')
            actual=[]
            for index in range(2):
                ace=ctypes.c_void_p()
                if not self.a.GetAce(acl,index,ctypes.byref(ace)):raise TransportError('NFEDITOR_DACL')
                raw=ctypes.string_at(ace,8);kind_ace,flags,_,rights=struct.unpack('<BBHI',raw)
                sid=self.sid_text(ctypes.c_void_p(ace.value+8))
                if kind_ace!=0 or flags & 0x10 or rights!=mask:raise TransportError('NFEDITOR_DACL')
                actual.append(sid)
            if set(actual)!={self.logon,'S-1-5-18'}:raise TransportError('NFEDITOR_DACL')
            return {'owner':self.user,'logonSid':self.logon,'protectedDacl':True,'mask':mask,'aces':actual}
        finally:self.k.LocalFree(sd)

    def descriptor(self,path):
        self.no_reparse(path)
        for directory in (path.parent,path.parent.parent):
            handle=self.k.CreateFileW(str(directory),0x20080,7,None,3,0x02200000,None)
            if handle==INVALID:raise TransportError('NFEDITOR_SESSION_DIRECTORY')
            try:self.security(handle,1,0x1f01ff)
            finally:self.k.CloseHandle(handle)
        handle=self.k.CreateFileW(str(path),0x120081,1,None,3,0x00200000,None)
        if handle==INVALID:raise TransportError('NFEDITOR_DESCRIPTOR')
        try:
            evidence=self.security(handle,1,0x1f01ff)
            final=ctypes.create_unicode_buffer(32768);n=self.k.GetFinalPathNameByHandleW(handle,final,len(final),0)
            if not n or n>=len(final) or Path(final.value.removeprefix('\\\\?\\')).resolve()!=path.resolve():raise TransportError('NFEDITOR_DESCRIPTOR_PATH')
            size=ctypes.c_int64()
            if not self.k.GetFileSizeEx(handle,ctypes.byref(size)) or not 0<size.value<=MAX_DESCRIPTOR:raise TransportError('NFEDITOR_DESCRIPTOR_SIZE')
            data=ctypes.create_string_buffer(size.value);count=w.DWORD()
            if not self.k.ReadFile(handle,data,len(data),ctypes.byref(count),None) or count.value!=len(data):raise TransportError('NFEDITOR_DESCRIPTOR_READ')
            return strict_json(data.raw),evidence
        finally:self.k.CloseHandle(handle)

    def bounded_public_file(self,path,limit,require_user_owner=False):
        self.no_reparse(path)
        handle=self.k.CreateFileW(str(path),0x100081 | (0x20000 if require_user_owner else 0),1,None,3,0x00200000,None)
        if handle==INVALID:raise TransportError('NFEDITOR_FILE_OPEN',5)
        try:
            if require_user_owner:
                owner=ctypes.c_void_p();descriptor=ctypes.c_void_p()
                if self.a.GetSecurityInfo(handle,1,1,ctypes.byref(owner),None,None,None,ctypes.byref(descriptor)):
                    raise TransportError('NFEDITOR_FILE_OWNER')
                try:
                    if not owner.value or self.sid_text(owner)!=self.user: raise TransportError('NFEDITOR_FILE_OWNER')
                finally:self.k.LocalFree(descriptor)
            final=ctypes.create_unicode_buffer(32768);n=self.k.GetFinalPathNameByHandleW(handle,final,len(final),0)
            if not n or n>=len(final) or Path(final.value.removeprefix('\\\\?\\')).resolve()!=path.resolve():raise TransportError('NFEDITOR_FILE_PATH',2)
            size=ctypes.c_int64()
            if not self.k.GetFileSizeEx(handle,ctypes.byref(size)) or not 0<size.value<=limit:raise TransportError('NFEDITOR_FILE_SIZE',2)
            data=ctypes.create_string_buffer(size.value);count=w.DWORD()
            if not self.k.ReadFile(handle,data,len(data),ctypes.byref(count),None) or count.value!=len(data):raise TransportError('NFEDITOR_FILE_READ',5)
            return data.raw
        finally:self.k.CloseHandle(handle)

# In the exceptional cancellation-unknown branch, retain ctypes storage/handles for the process lifetime.
# The CLI exits with unknown and never claims resources were released or the call was cancelled.
PENDING_IO=[]
class Session:
    def __init__(self,args,root,digest,engine_processes,process_uses_project):
        self.deadline=time.monotonic()+args.timeout_seconds;self.win=Windows();self.handle=None;self.evidence={}
        state_root=state_directory(args.project)
        path=Path(args.session_file);self.win.no_reparse(path)
        if not path.resolve().is_relative_to(state_root/'Reports/PluginControlSessions') or path.name!='Session.json':raise TransportError('NFEDITOR_SESSION_PATH',2)
        data,acl=self.win.descriptor(path)
        expected={'editorApiVersion','profile','controlSessionId','pid','processStart','project','module','binarySha256','engine','pipe','sessionFile','reportRoot','limits','credential'}
        if not isinstance(data,dict) or set(data)!=expected or data['profile']!=PROFILE or data['editorApiVersion']!=API:raise TransportError('NFEDITOR_DESCRIPTOR_FIELDS',2)
        project,stage,binary=validate_isolated_project(Path(args.project),Path(args.plugin_directory),root)
        session=data['controlSessionId'];pid=data['pid']
        if not isinstance(session,str) or len(session)!=32 or any(c not in '0123456789abcdef' for c in session) or type(pid)!=int or not 1<=pid<=0xffffffff:
            raise TransportError('NFEDITOR_IDENTITY',2)
        if path.resolve()!=state_root/'Reports/PluginControlSessions'/session/'Session.json' or Path(data['sessionFile']).resolve()!=path.resolve():raise TransportError('NFEDITOR_SESSION_PATH',2)
        if Path(data['project']).resolve()!=project or Path(data['module']).resolve()!=binary or data['binarySha256']!=digest(binary):raise TransportError('NFEDITOR_IDENTITY',3)
        if data['pipe']!='\\\\.\\pipe\\NodeForge-'+str(pid)+'-'+session:raise TransportError('NFEDITOR_PIPE_IDENTITY',3)
        identity=process_identity(pid);module=observed_module(pid,binary.name)
        processes=[p for p in engine_processes() if p.get('ProcessId')==pid]
        if (identity.get('state')!='running' or str(identity.get('createdTicks'))!=data['processStart'] or not module or module.resolve()!=binary
            or len(processes)!=1 or not process_uses_project(processes[0],project)):raise TransportError('NFEDITOR_OS_IDENTITY',3)
        self.data=data;self.evidence={'descriptorDacl':acl,'pid':pid,'processStart':data['processStart'],'module':str(binary),'binarySha256':digest(binary),'osIdentity':identity}
        hello_deadline=min(self.deadline,time.monotonic()+5)
        while True:
            # READ_CONTROL is needed to verify the pipe DACL. No generic-write or instance-creation bit.
            handle=self.win.k.CreateFileW(data['pipe'],0x120003,0,None,3,0x40110000,None)
            if handle!=INVALID:break
            error=ctypes.get_last_error()
            if error!=231:raise TransportError('NFEDITOR_PIPE_CONNECT',3)
            left=hello_deadline-time.monotonic()
            if left<=0 or not self.win.k.WaitNamedPipeW(data['pipe'],max(1,int(left*1000))):raise TransportError('NFEDITOR_CONNECT_DEADLINE',5)
        self.handle=handle
        try:
            observed=w.ULONG()
            if not self.win.k.GetNamedPipeServerProcessId(handle,ctypes.byref(observed)) or observed.value!=pid:raise TransportError('NFEDITOR_PIPE_SERVER_PID',3)
            self.evidence['pipeDacl']=self.win.security(handle,6,0x12019f)
            credential=data['credential']
            if not isinstance(credential,str) or len(credential)!=64 or any(c not in '0123456789abcdef' for c in credential):raise TransportError('NFEDITOR_CREDENTIAL_FORMAT',2)
            params={'editorApiVersion':API,'profile':PROFILE,'controlSessionId':session,'expectedProject':project.as_posix(),'expectedPid':pid,
                'expectedProcessStart':data['processStart'],'expectedBinarySha256':data['binarySha256'],'credential':credential}
            response=self.exchange({'jsonrpc':'2.0','id':'Hello-1','method':'session.hello','params':params},hello_deadline)
            result=response.get('result')
            public={key:value for key,value in data.items() if key!='credential'}
            if result!=public:raise TransportError('NFEDITOR_HELLO_IDENTITY',3)
            self.data=public;self.evidence['helloMatched']=True
        except BaseException:self.close();raise

    def close(self):
        if self.handle is not None:self.win.k.CloseHandle(self.handle);self.handle=None
    def io(self,write,data,size,deadline):
        if time.monotonic()>=deadline:raise TransportError('NFEDITOR_DEADLINE',5)
        event=self.win.k.CreateEventW(None,True,False,None)
        if not event:raise TransportError('NFEDITOR_EVENT')
        overlap=Overlapped();overlap.hEvent=event
        buffer=ctypes.create_string_buffer(data,size) if write else ctypes.create_string_buffer(size)
        count=w.DWORD();ret=self.win.k.WriteFile(self.handle,buffer,size,ctypes.byref(count),ctypes.byref(overlap)) if write else self.win.k.ReadFile(self.handle,buffer,size,ctypes.byref(count),ctypes.byref(overlap))
        retained=False
        try:
            if not ret:
                if ctypes.get_last_error()!=997:raise TransportError('NFEDITOR_PIPE_IO',5)
                left=max(0,deadline-time.monotonic())
                if self.win.k.WaitForSingleObject(event,int(left*1000))!=0:
                    self.win.k.CancelIoEx(self.handle,ctypes.byref(overlap))
                    if self.win.k.WaitForSingleObject(event,10000)!=0:
                        PENDING_IO.append((self.handle,event,overlap,buffer));self.handle=None;retained=True
                        raise TransportError('NFEDITOR_CANCELLATION_UNKNOWN',5)
                    result=self.win.k.GetOverlappedResult(self.handle,ctypes.byref(overlap),ctypes.byref(count),False)
                    if not result and ctypes.get_last_error()==996:
                        PENDING_IO.append((self.handle,event,overlap,buffer));self.handle=None;retained=True
                        raise TransportError('NFEDITOR_CANCELLATION_UNKNOWN',5)
                    raise TransportError('NFEDITOR_DEADLINE',5)
                if not self.win.k.GetOverlappedResult(self.handle,ctypes.byref(overlap),ctypes.byref(count),False):
                    if ctypes.get_last_error()==996:
                        PENDING_IO.append((self.handle,event,overlap,buffer));self.handle=None;retained=True
                        raise TransportError('NFEDITOR_CANCELLATION_UNKNOWN',5)
                    raise TransportError('NFEDITOR_PIPE_IO',5)
            if not count.value:raise TransportError('NFEDITOR_PIPE_DISCONNECTED',5)
            return count.value if write else buffer.raw[:count.value]
        finally:
            if not retained:self.win.k.CloseHandle(event)
    def read_exact(self,count,deadline):
        result=bytearray()
        while len(result)<count:result.extend(self.io(False,b'',count-len(result),deadline))
        return bytes(result)
    def exchange(self,request,deadline):
        deadline=min(deadline,self.deadline)
        body=json.dumps(request,ensure_ascii=False,separators=(',',':'),allow_nan=False).encode('utf-8')
        if not 0<len(body)<=MAX_FRAME:raise TransportError('NFEDITOR_FRAME_SIZE',2)
        framed=struct.pack('<I',len(body))+body;offset=0
        while offset<len(framed):offset+=self.io(True,framed[offset:],len(framed)-offset,deadline)
        size=struct.unpack('<I',self.read_exact(4,deadline))[0]
        if not 0<size<=MAX_FRAME:raise TransportError('NFEDITOR_FRAME_SIZE',5)
        response=strict_json(self.read_exact(size,deadline))
        if (not isinstance(response,dict) or set(response) not in ({'jsonrpc','id','result'},{'jsonrpc','id','error'})
            or response['jsonrpc']!='2.0' or response['id']!=request['id']):raise TransportError('NFEDITOR_RESPONSE',5)
        return response
    def request(self,method,arguments,call_id,revision=None):
        context={'controlSessionId':self.data['controlSessionId'],'editorApiVersion':API,'profile':PROFILE}
        if revision is not None:context['expectedDraftRevision']=revision
        return self.exchange({'jsonrpc':'2.0','id':call_id,'method':method,'params':{'context':context,'arguments':arguments}},min(self.deadline,time.monotonic()+(5 if method=='calls.get' else 10)))

def execute(args,summary,root,digest,write_new,engine_processes,process_uses_project):
    if not math.isfinite(args.timeout_seconds) or args.timeout_seconds<=0:raise TransportError('NFEDITOR_TIMEOUT',2)
    out=Path(args.run_directory)
    state_root=state_directory(args.project)
    if not out.is_absolute() or not out.resolve().is_relative_to(state_root/'Reports') or out.exists():raise TransportError('NFEDITOR_RUN_DIRECTORY',2)
    out.mkdir(parents=True);summary.update(run=str(out),project=str(Path(args.project).resolve()),_publish=True,host='activeEditor',editorApiVersion=API,profile=PROFILE)
    session=None
    try:
        session=Session(args,root,digest,engine_processes,process_uses_project)
        write_new(out/'Identity.json',session.evidence)
        description=session.request('operations.describe',{},'Describe-1')
        if 'error' in description:raise TransportError('NFEDITOR_DESCRIPTION',3)
        runtime=description['result'];write_new(out/'Description.json',runtime)
        if runtime.get('host')!='activeEditor' or runtime.get('identity')!=session.data:raise TransportError('NFEDITOR_DESCRIPTION_IDENTITY',3)
        if args.action=='editor-describe':response=description;method='operations.describe'
        else:
            method='calls.get' if args.action=='editor-status' else args.operation
            definitions=[row for row in runtime['operations'] if row.get('name')==method]
            if len(definitions)!=1 or definitions[0].get('dispatch')=='handshake':raise TransportError('NFEDITOR_OPERATION_UNAVAILABLE',2)
            arguments={'callId':args.call_id} if args.action=='editor-status' else {}
            if getattr(args,'arguments_file',None):
                path=Path(args.arguments_file);session.win.no_reparse(path)
                if not any(path.resolve().is_relative_to(scope) for scope in (root/'Examples',root/'Tests',state_root/'Reports')) or path.resolve().is_relative_to(state_root/'Reports/PluginControlSessions') or not path.is_file() or not 0<path.stat().st_size<=MAX_FRAME:
                    raise TransportError('NFEDITOR_ARGUMENTS_PATH',2)
                arguments=strict_json(session.win.bounded_public_file(path,MAX_FRAME))
            validate_arguments(arguments,definitions[0]['argumentsSchema'])
            revision=getattr(args,'expected_revision',None)
            if definitions[0]['expectedDraftRevisionRequired']!=(revision is not None):raise TransportError('NFEDITOR_REVISION',2)
            call_id='Status-1' if args.action=='editor-status' else args.call_id
            if not isinstance(call_id,str) or not 1<=len(call_id)<=64 or any(not(c.isascii() and (c.isalnum() or c=='-')) for c in call_id):raise TransportError('NFEDITOR_CALL_ID',2)
            request={'method':method,'callId':call_id,'expectedDraftRevision':revision,'argumentsDigest':__import__('hashlib').sha256(json.dumps(arguments,ensure_ascii=False,sort_keys=True).encode('utf-8')).hexdigest()}
            write_new(out/'Request.json',request)
            response=session.request(method,arguments,call_id,revision)
            if definitions[0]['dispatch']=='gameThread':
                summary.update(callId=call_id,pid=session.data['pid'],controlSessionId=session.data['controlSessionId'],executionState='unknown')
                write_new(out/'FirstResponse.json',response)
                query=0;delay=0.25
                while 'result' in response and response['result'].get('state') in ('accepted','queued','running'):
                    left=session.deadline-time.monotonic()
                    if left<=0:
                        summary.update(response=response,executionState='pending');return 5
                    time.sleep(min(delay,left));delay=min(delay*2,1)
                    if time.monotonic()>=session.deadline:
                        summary.update(response=response,executionState='pending');return 5
                    query+=1
                    response=session.request('calls.get',{'callId':call_id},'Query-'+str(query))
                    write_new(out/('Query'+str(query)+'.json'),response)
        write_new(out/'Response.json',response)
        code=0;execution='finished'
        if method=='control.disable' and 'result' in response:
            # Let the server observe the response was consumed and its peer closed.
            # Receipt observation uses the original total deadline; no reconnect/replay.
            session.close()
            receipt_path=Path(session.data['reportRoot'])/'StopReceipt.json'
            receipt=None
            while time.monotonic()<session.deadline:
                if receipt_path.is_file():
                    try:
                        receipt=strict_json(session.win.bounded_public_file(receipt_path,1048576));break
                    except (TransportError,ValueError,UnicodeError):pass # Only observe a file still being closed, never replay disable.
                time.sleep(min(.1,max(0,session.deadline-time.monotonic())))
            if receipt is None:
                summary.update(operation=method,pid=session.data['pid'],controlSessionId=session.data['controlSessionId'],response=response,executionState='stopping');return 5
            write_new(out/'StopReceipt.json',receipt)
            probe=session.win.k.CreateFileW(session.data['pipe'],0x120003,0,None,3,0x40110000,None)
            pipe_gone=probe==INVALID and ctypes.get_last_error()==2
            if probe!=INVALID:session.win.k.CloseHandle(probe)
            write_new(out/'PipeClosed.json',{'pipeAbsent':pipe_gone})
            stopped=(response['result'].get('state')=='stoppingRequested' and receipt.get('controlSessionId')==session.data['controlSessionId']
                and receipt.get('state')=='stopped' and receipt.get('descriptorDeleted') and receipt.get('transportHandlesReleased')
                and pipe_gone and not Path(session.data['sessionFile']).exists() and bool(receipt.get('workers'))
                and all(x.get('threadDone') and not x.get('pendingIo') and x.get('cancelRequested')==x.get('cancelCompleted') for x in receipt['workers']))
            summary.update(operation=method,pid=session.data['pid'],controlSessionId=session.data['controlSessionId'],response=response,executionState='stopped' if stopped else 'unknown')
            return 0 if stopped else 5
        if 'error' in response:
            rpc=response['error'].get('code');detail=response['error'].get('data',{}).get('code')
            code=3 if rpc==-32010 else 5 if detail in ('unknownCall','registrationPending') or rpc==-32603 else 2
            execution='unknown' if code==5 else 'environmentBlocked' if code==3 else 'rejected'
        elif method=='calls.get' or (args.action=='editor-call' and definitions[0]['dispatch']=='gameThread'):
            state=response['result'].get('state')
            if state in ('accepted','queued','running'):code=5;execution='pending'
            elif state=='unknown':code=5;execution='unknown'
            elif state not in ('completed','rejected','failed','cancelled'):code=5;execution='unknown'
            else:code=int(response['result'].get('cliExitCode',5));execution='finished' if code==0 else 'rejected' if code==2 else 'failed' if code==4 else 'unknown'
        summary.update(operation=method,pid=session.data['pid'],controlSessionId=session.data['controlSessionId'],moduleSha256=session.data['binarySha256'],response=response,executionState=execution)
        return code
    finally:
        if session:session.close()
