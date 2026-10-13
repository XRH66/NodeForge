"""Owned pure-data negative tests; never launch UE or modify protected projects."""
import ast, copy, hashlib, json, sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Scripts'))
from RunProcessGuard import validate_isolated_project,validate_terminal_step
from RunGraphRegression import read_json,write_new,ref,sha

out=ROOT/'Reports/20261010NightAudit/CoordinatorFixed1'
out.mkdir(exist_ok=False)
folder=ROOT/'ValidationProject/RegressionRuns/CoordinatorGuards1'
folder.mkdir(exist_ok=False)
project=folder/'CoordinatorGuards1.uproject'
stage=ROOT/'Reports/20261010NightAudit/Build74/PluginStage/NodeForge'
binary=stage/'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'
descriptor={'FileVersion':3,'EngineAssociation':'5.3','AdditionalPluginDirectories':[str(stage.parent)],'Plugins':[{'Name':'NodeForge','Enabled':True}]}
write_new(project,descriptor)
cases=[]
def run(name,action,reject=False):
    try:
        action()
        actual='accepted'
    except (ValueError,FileNotFoundError,KeyError,TypeError):
        actual='rejected'
    expected='rejected' if reject else 'accepted'
    cases.append({'case':name,'actual':actual,'expected':expected,'passed':actual==expected})

run('ValidIsolatedSelection',lambda:validate_isolated_project(project,stage,ROOT))
run('RootProjectRejected',lambda:validate_isolated_project(ROOT/'ValidationProject/NodeForgeValidation.uproject',stage,ROOT),True)
run('FrozenRc1ProjectRejected',lambda:validate_isolated_project(ROOT/'ValidationProject/RegressionRuns/CandidateRC1/NFCandidateRC1.uproject',stage,ROOT),True)
run('RootPluginFallbackRejected',lambda:validate_isolated_project(project,ROOT/'Plugins/NodeForge',ROOT),True)
run('MissingStageRejected',lambda:validate_isolated_project(project,out/'Missing/NodeForge',ROOT),True)
def changed_descriptor(change):
    value=copy.deepcopy(descriptor);change(value)
    project.write_text(json.dumps(value),encoding='utf-8')
    try:validate_isolated_project(project,stage,ROOT)
    finally:project.write_text(json.dumps(descriptor),encoding='utf-8')
run('WrongPluginDirectory',lambda:changed_descriptor(lambda d:d.update(AdditionalPluginDirectories=[str(ROOT/'Plugins')])),True)
run('MultiplePluginDirectories',lambda:changed_descriptor(lambda d:d['AdditionalPluginDirectories'].append(str(ROOT/'Plugins'))),True)
run('DisabledPlugin',lambda:changed_descriptor(lambda d:d['Plugins'][0].update(Enabled=False)),True)
override=folder/'Plugins/NodeForge';override.mkdir(parents=True)
run('LocalOverrideRejected',lambda:validate_isolated_project(project,stage,ROOT),True)
# Keep the sentinel override as evidence; don't use this fixture project for UE.
prior=ROOT/'Reports/20261009UsableEditor/NightNegative74'
step=read_json(prior/'Progress1.json')['steps'][0]
old_project=ROOT/read_json(prior/'Preparation.json')['project']
run('RealPreserved74TerminalAccepted',lambda:validate_terminal_step(prior,'ValidationBatch',step,binary,old_project))
for name,change in [('FalseStep',lambda d:d.update(passed=False)),('NonzeroStep',lambda d:d.update(exitCode=3)),
                    ('WrongPid',lambda d:d.update(pid=d['pid']+1)),('MissingPid',lambda d:d.pop('pid')),
                    ('WrongStepName',lambda d:d.update(step='Other'))]:
    value=copy.deepcopy(step);change(value)
    run(name,lambda value=value:validate_terminal_step(prior,'ValidationBatch',value,binary,old_project),True)
def copied_terminal(name,change):
    target=out/name;target.mkdir()
    data={kind:read_json(prior/('ValidationBatch'+kind+'.json')) for kind in ('Started','Exit','LoadedModule')}
    data['Started']['command']=[('-NFReport='+str(target/'ValidationBatch.json')) if arg.startswith('-NFReport=') else arg for arg in data['Started']['command']]
    change(data)
    for kind,value in data.items():write_new(target/('ValidationBatch'+kind+'.json'),value)
    validate_terminal_step(target,'ValidationBatch',step,binary,old_project)
run('NonzeroExitRejected',lambda:copied_terminal('NonzeroExit',lambda d:d['Exit'].update(exitCode=4)),True)
run('ModuleMismatchRejected',lambda:copied_terminal('WrongModule',lambda d:d['LoadedModule'].update(diskFileSha256='0'*64)),True)
run('WrongProjectRejected',lambda:copied_terminal('WrongProject',lambda d:d['Started']['command'].__setitem__(1,str(ROOT/'ValidationProject/NodeForgeValidation.uproject'))),True)
run('MissingExitRejected',lambda:validate_terminal_step(out,'ValidationBatch',step,binary,old_project),True)
# Read the actual final predicate, without executing the script/importing its entry point.
tree=ast.parse((ROOT/'Scripts/RunPreflightChecks.py').read_text('utf-8'))
predicate=next(n.value for n in reversed(tree.body) if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='ok' for t in n.targets))
expression=compile(ast.Expression(predicate),'<actual preflight final predicate>','eval')
expected=['ValidationBatch',*[n+p for n in ('Material','Blueprint','Instance') for p in ('Import','Reopen')],'ResultView','PanelConstruction']
def final(steps):
    accepted=eval(expression,{'errors':[],'protection':{'owned':True},'expected_steps':expected,'steps':steps})
    if accepted is not True:raise ValueError('final state rejected')
complete=[{'step':n,'passed':True,'exitCode':0} for n in expected]
run('ExactNineStepsAccepted',lambda:final(complete))
run('EmptyStepsRejected',lambda:final([]),True)
run('IncompleteStepsRejected',lambda:final(complete[:-1]),True)
bad=copy.deepcopy(complete);bad[0]['passed']=False
run('FailedBatchCannotBecomePassed',lambda:final(bad),True)
bad_exit=copy.deepcopy(complete);bad_exit[0]['exitCode']=3
run('NonzeroBatchCannotBecomePassed',lambda:final(bad_exit),True)
good=all(c['passed'] for c in cases)
write_new(out/'Summary.json',{'result':'passed' if good else 'failed','cases':cases,
    'runnerHashes':{ref(p):sha(p) for p in (ROOT/'Scripts/RunProcessGuard.py',ROOT/'Scripts/RunGraphSceneChecks.py',ROOT/'Scripts/RunPreflightChecks.py',Path(__file__))},
    'scope':'Pure owned descriptor/terminal data and exact current final AST predicate; zero UE launches, no asset mutations; real prior 74 terminal only read, not relabelled as a new native run'})
print(json.dumps({'result':'passed' if good else 'failed','cases':len(cases)}),flush=True)
raise SystemExit(0 if good else 1)
