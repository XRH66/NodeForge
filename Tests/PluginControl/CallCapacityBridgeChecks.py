"""Guard the real management-call admission before any native/CLI submission."""
import argparse,json,sys,threading
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--plugin-directory',required=True);p.add_argument('--out',required=True);a=p.parse_args()
home=Path(a.plugin_directory);out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=False)
sys.path.insert(0,str(home/'Scripts'))
from NodeForgeMcpBridge import Bridge,BridgeError
contract=json.loads((home/'Tools/NodeForgeMcp/ToolContract.json').read_text('utf-8-sig'))
tool=next(t for t in contract['tools'] if t['name']=='nodeforge_calls_releaseCompleted')
rows=[]
for case in ('UnknownOriginal','PendingCli','PendingOriginalJournal','BusinessBusy'):
    b=Bridge.__new__(Bridge);b.discovery=False;b.run_root=out/case;b.run_root.mkdir();b.calls={};b.unknown=set();b.pending_cli=[];b.business_lock=threading.Lock()
    b._bound=lambda *args:None # Isolate admission; real handle ownership is exercised by the SDK test.
    b._cli=lambda *args: (_ for _ in ()).throw(AssertionError('Guard incorrectly submitted CLI'))
    if case=='UnknownOriginal':b.unknown.add('Original')
    if case=='PendingCli':b.pending_cli.append({'fixture':'unconfirmed exit'})
    if case=='PendingOriginalJournal':
        (b.run_root/'Calls').mkdir();(b.run_root/'Calls/Original.json').write_text(json.dumps({'callId':'Original','state':'pending'}),encoding='utf-8')
    if case=='BusinessBusy':b.business_lock.acquire()
    try:
        b.business(tool,{'handle':'fixture','callId':'Cleanup','arguments':{'maxCount':50}})
        raise AssertionError('Unresolved cleanup was admitted')
    except BridgeError as e:
        assert e.code==('NFMCP_BUSY' if case=='BusinessBusy' else 'NFMCP_UNRESOLVED_CALL')
        assert not (b.run_root/'Calls/Cleanup.json').exists()
        rows.append({'case':case,'passed':True,'code':e.code,'cliSubmitted':False})
    finally:
        if case=='BusinessBusy':b.business_lock.release()
(out/'Summary.json').write_text(json.dumps({'status':'passed','checks':rows,'scope':'isolated real Bridge.business guards'},indent=2)+'\n',encoding='utf-8')
print(json.dumps({'status':'passed','checks':len(rows)}))
