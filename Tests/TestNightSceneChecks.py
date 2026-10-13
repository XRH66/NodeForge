"""Exact single-message PIE gates: reuse preserved data, never launch UE."""
import ast, copy, hashlib, json, math, re, sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Scripts'))
from RunGraphRegression import read_json,write_new,sha,ref

source=ROOT/'Scripts/RunGraphSceneChecks.py'
tree=ast.parse(source.read_text('utf-8'))
function=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='blueprint_runtime_checks')
namespace={'Path':Path,'math':math,'re':re}
exec(compile(ast.Module(body=[function],type_ignores=[]),str(source),'exec'),namespace)
check=namespace['blueprint_runtime_checks']
prior=ROOT/'Reports/20261009UsableEditor/MemberRuntime1'
original=read_json(prior/'MemberCountRuntime.json')
config=read_json(prior/'MemberCountConfig.json')
config_path=prior/'MemberCountConfig.json'
log=(prior/'MemberCountRuntime.log').read_text('utf-8-sig',errors='replace')
out=ROOT/'Reports/20261010NightAudit/SceneExact1'
out.mkdir(exist_ok=False)
cases=[]

def run(name,change=lambda data:None,*,expected=True,log_text=log,prior_pids=()):
    data=copy.deepcopy(original);change(data)
    checks=check(data,original['pid'],config,config_path,log_text,prior_pids)
    accepted=all(checks.values())
    cases.append({'case':name,'expectedAccepted':expected,'actualAccepted':accepted,
                  'passed':accepted==expected,'checks':checks})

run('PreservedRealExactDataAccepted')
run('ExtraMessageRejected',lambda d:d['engineMessages'].append('[Other] unexpected'),expected=False)
run('RepeatedExpectedMessageRejected',lambda d:d.update(engineMessages=d['engineMessages']*2,messageCount=2,attributedMessageCount=2),expected=False)
run('WrongCountRejected',lambda d:d.update(messageCount=2),expected=False)
run('WrongPidRejected',lambda d:d.update(pid=d['pid']+1),expected=False)
run('ReusedUiPidRejected',expected=False,prior_pids=(original['pid'],))
run('WrongConfigRejected',lambda d:d.update(configPath=str(out/'Other.json')),expected=False)
run('WrongClassRejected',lambda d:d.update(expectedClass='/Game/Wrong.Wrong_C'),expected=False)
run('WrongActorRejected',lambda d:d.update(sourceActor='other'),expected=False)
run('WrongFunctionRejected',lambda d:d.update(sourceFunction='Other'),expected=False)
run('WrongWorldRejected',lambda d:d.update(sourceWorld='other'),expected=False)
run('NonPieRejected',lambda d:d.update(worldType='Editor'),expected=False)
run('NotStoppedRejected',lambda d:d.update(pieStopped=False),expected=False)
run('NaNObservedRejected',lambda d:d.update(observedSeconds=float('nan')),expected=False)
run('ExtraNativeLogRejected',expected=False,log_text=log+'\nLogBlueprintUserMessages: [Other] unexpected\n')
run('MissingNativeExitMarkerRejected',expected=False,log_text=log.replace('TEST COMPLETE. EXIT CODE: 0','missing marker'))

# Execute the actual final per-domain hash expressions: no conditional `sha` import.
summary=next(n for n in tree.body if isinstance(n,ast.Expr) and isinstance(n.value,ast.Call)
             and isinstance(n.value.func,ast.Name) and n.value.func.id=='write'
             and isinstance(n.value.args[-1],ast.Dict) and any(isinstance(k,ast.Constant) and k.value=='sourceBlueprintBytesUnchanged' for k in n.value.args[-1].keys))
fields={k.value:v for k,v in zip(summary.value.args[-1].keys,summary.value.args[-1].values)}
ctx={'hashlib':hashlib,'runtime_sources':{config_path:sha(config_path)},'material_sources':{}}
for field,wanted in [('sourceBlueprintBytesUnchanged',True),('sourceMaterialBytesUnchanged','notApplicable')]:
    actual=eval(compile(ast.Expression(fields[field]),'<actual scene summary>','eval'),ctx)
    cases.append({'case':field,'actual':actual,'expected':wanted,'passed':actual==wanted})
passed=all(c['passed'] for c in cases)
write_new(out/'Summary.json',{'result':'passed' if passed else 'failed','cases':cases,
    'runnerHashes':{ref(p):sha(p) for p in (source,Path(__file__))},
    'preservedInput':ref(prior/'MemberCountRuntime.json'),'preservedInputSha256':sha(prior/'MemberCountRuntime.json'),
    'scope':'Actual current pure function and summary AST with read-only old real data plus labelled negatives; no new UE or UI pass'})
print(json.dumps({'result':'passed' if passed else 'failed','cases':len(cases)}),flush=True)
raise SystemExit(0 if passed else 1)
