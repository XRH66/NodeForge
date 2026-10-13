"""Read-only report presentation and actual panel construction; no UI click claim."""
import argparse, subprocess, time
from RunGraphRegression import ROOT, ENGINE, read_json, write_new, build_snapshot, sha, ref

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--run',required=True)
parser.add_argument('--plugin-directory',required=True)
args=parser.parse_args()
if not args.run.isascii() or not args.run.isalnum():raise SystemExit('Fresh ASCII alphanumeric run name required')
plugin=(ROOT/args.plugin_directory).resolve()
if not plugin.is_relative_to(ROOT/'Reports') or plugin.name!='NodeForge':raise SystemExit('Explicit staged NodeForge build required')
out=ROOT/'Reports/20261009UsableEditor'/args.run
folder=ROOT/'ValidationProject/RegressionRuns'/args.run
if out.exists() or folder.exists():raise SystemExit('Existing reports/projects protected')
baseline=build_snapshot(plugin)
out.mkdir();folder.mkdir()
project=folder/(args.run+'.uproject')
write_new(project,{'FileVersion':3,'EngineAssociation':'5.3','AdditionalPluginDirectories':[str(plugin.parent)],'Plugins':[{'Name':'NodeForge','Enabled':True}]})
steps=[]
for name,test,report_arg in [('ResultView','NodeForge.ResultView','NFReport'),('PanelConstruction','NodeForge.MinimalPanel','NFPanelReport')]:
    report=out/(name+'.json')
    cmd=[str(ENGINE),str(project),'-ExecCmds=Automation RunTests '+test+'; Quit','-TestExit=Automation Test Queue Empty',
         '-'+report_arg+'='+str(report),'-d3d11','-RenderOffscreen','-AllowCommandletRendering','-unattended','-nop4','-nosound','-NoLiveCoding',
         '-abslog='+str(out/(name+'.log')),'-stdout','-FullStdOutLogOutput']
    write_new(out/(name+'Command.json'),cmd)
    with (out/(name+'Console.txt')).open('xb') as stream:
        process=subprocess.Popen(cmd,cwd=ROOT,stdout=stream,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
        started=time.monotonic()
        while process.poll() is None and time.monotonic()-started<180:time.sleep(.2)
        if process.poll() is None:
            write_new(out/'LiveProcess.json',{'pid':process.pid,'command':cmd})
            raise SystemExit('Native process still live; retain and observe, do not restart')
    data=read_json(report) if report.exists() else {}
    passed=process.returncode==0 and data.get('status')=='passed'
    steps.append({'step':name,'passed':passed,'exitCode':process.returncode,'report':ref(report)})
    print(name,steps[-1],flush=True)
after=build_snapshot(plugin)
good=all(s['passed'] for s in steps) and baseline==after
write_new(out/'Summary.json',{'result':'passed' if good else 'failed','steps':steps,'project':ref(project),'selectedPlugin':ref(plugin),
    'dllSha256':sha(plugin/'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'),'sourceAndBinaryBefore':baseline,'sourceAndBinaryAfter':after,
    'oneUnchangedBuild':baseline==after,'scope':'Actual frozen-report summary/copy payload and same production panel factory construction; no asset graph, real clipboard button or UI import acceptance'})
raise SystemExit(0 if good else 1)
