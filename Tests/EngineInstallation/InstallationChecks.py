"""Targeted checks of the production installation admission seam using exact candidate files."""
import argparse, hashlib, json, shutil, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'Scripts'))
from RunProcessGuard import installation_layout, installation_product, load_installation_binding
from PrepareNodeForgeInstallation import encoded

def run(candidate,out,engine_root):
    out.mkdir(parents=True,exist_ok=False)
    engine=out/'UE532'; project=out/'Project/Check.uproject'
    project.parent.mkdir()
    project.write_bytes(encoded({'FileVersion':3,'Plugins':[{'Name':'NodeForge','Enabled':True}]}))
    for relative in ['Engine/Build/Build.version','Engine/Binaries/Win64/UnrealEditor.modules']:
        path=engine/relative;path.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(engine_root/relative,path)
    (engine/'Engine/Binaries/Win64/UnrealEditor-Cmd.exe').write_bytes(b'presence fixture; never executed')
    rows=[]
    def reject(name,fn):
        try:fn()
        except ValueError:rows.append({'case':name,'passed':True});return
        raise AssertionError(name+' unexpectedly admitted')
    manifest=json.loads((candidate/'InstallationManifest.json').read_bytes())
    paths=[('projectPlugin',project.parent/'Plugins/NodeForge'),('enginePlugin',engine/'Engine/Plugins/NodeForge'),
        ('engineMarketplace',engine/'Engine/Plugins/Marketplace/NodeForge')]
    config=project.parent/'Config/NodeForgeInstallation.json';config.parent.mkdir()
    for layout,plugin in paths:
        plugin.mkdir(parents=True,exist_ok=True)
        for rel in ['NodeForge.uplugin','InstallationManifest.json','Binaries/Win64/UnrealEditor-NodeForgeEditor.dll','Binaries/Win64/UnrealEditor.modules']:
            dest=plugin/rel;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(candidate/rel,dest)
        binding={'format':'NodeForgeInstallationBinding','version':1,'installationRoot':str(plugin),'project':str(project),
            'pluginDirectory':str(plugin),'binarySha256':manifest['binarySha256'],'engineRoot':str(engine),'readRoots':['/Game/NodeForgeTests']}
        config.write_bytes(encoded(binding))
        assert installation_layout(project,engine,plugin)==layout
        installation_product(plugin,engine);load_installation_binding(project,plugin,plugin)
        rows.append({'case':layout,'passed':True})
        dll=plugin/'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll';old=dll.read_bytes();dll.write_bytes(old+b'x')
        reject(layout+'WrongDll',lambda:installation_product(plugin,engine));dll.write_bytes(old)
        duplicate=paths[1 if layout=='projectPlugin' else 0][1]/'NodeForge.uplugin'
        duplicate.parent.mkdir(parents=True,exist_ok=True);duplicate.write_bytes(b'duplicate')
        reject(layout+'Duplicate',lambda:load_installation_binding(project,plugin,plugin));duplicate.unlink()
        # Only this fixture's files; relocate outside every supported plugin position.
        relocated=out/('Recovered'+layout);shutil.move(str(plugin),str(relocated))
    version=engine/'Engine/Build/Build.version';old=version.read_bytes();v=json.loads(old);v['PatchVersion']=1;version.write_bytes(encoded(v))
    reject('WrongUEPatch',lambda:installation_product(out/'RecoveredengineMarketplace',engine));version.write_bytes(old)
    modules=engine/'Engine/Binaries/Win64/UnrealEditor.modules';old=modules.read_bytes();v=json.loads(old);v['BuildId']='incorrect';modules.write_bytes(encoded(v))
    reject('WrongBuildId',lambda:installation_product(out/'RecoveredengineMarketplace',engine));modules.write_bytes(old)
    result={'status':'passed','checks':rows,'binarySha256':manifest['binarySha256'],'engineExecution':False}
    (out/'Result.json').write_bytes(encoded(result));print(json.dumps(result))

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--candidate',type=Path,required=True);p.add_argument('--out',type=Path,required=True);p.add_argument('--engine-root',type=Path,required=True)
    a=p.parse_args();run(a.candidate.resolve(),a.out.resolve(),a.engine_root.resolve())
