"""Internal finite PC delivery adapters used by the original copy/runtime scripts.

One verified Material, the unchanged BT Blueprint, and one copy-only map. This is
not a product control API, a replacement graph executor, or a matrix test runner.
"""
import argparse
import json
import re
from pathlib import Path
from RunGraphRegression import ROOT, read_json, write_new, sha, ref, files_under, ASSET_SUFFIXES
from RunProcessGuard import installation_path, installation_json

TICK = '/Game/NodeForgeTests/BlueprintTick/B000007/BPTickMove'
TICK_SHA = '6ae609358986d3456c64a46fec135028a191c6922f205db76731935eccfcd0db'
TICK_MESSAGE = 'NodeForge Tick NFA000007TickReq'

def owned_path(value, directory):
    path = installation_path(str(ROOT / value))
    if not path.is_relative_to(ROOT / directory):
        raise ValueError('Path outside the selected project-owned scope: ' + directory)
    return path

def package_path(value):
    if not isinstance(value, str) or not re.fullmatch(r'/Game/NodeForgeTests/(?:[A-Za-z0-9]{1,64}/)*[A-Za-z0-9]{1,64}', value):
        raise ValueError('Exact isolated package path required')
    return value

def selected_files(plugin):
    plugin = installation_path(str(ROOT / plugin))
    if not any(plugin.is_relative_to(ROOT / name) for name in ['Reports', 'Builds']) or plugin.name != 'NodeForge':
        raise ValueError('Explicit tested or unpacked candidate plugin required')
    return {ref(p): sha(p) for p in sorted(plugin.rglob('*')) if p.is_file()}

def load_input(path):
    path = owned_path(path, 'Reports')
    data = installation_json(path, 65536)
    if set(data) != {'format', 'version', 'sourceProject', 'selectedPlugin', 'material', 'tick', 'map'} \
            or data['format'] != 'NodeForgePersonalDeliveryInput' or type(data['version']) is not int or data['version'] != 1:
        raise ValueError('Exact personal delivery input v1 required')
    project = owned_path(data['sourceProject'], 'ValidationProject/RegressionRuns/NFCUiApiLab')
    if project.name != 'NFCUiApiLab.uproject' or not project.is_file():
        raise ValueError('Original selected NFCUiApiLab required')
    package_path(data['map'])
    if data['tick'] != {'path': TICK, 'sha256': TICK_SHA}:
        raise ValueError('Only unchanged frozen BT asset may be reused')
    material = data['material']
    required = {'path', 'sha256', 'input', 'inputSha256', 'generationReport', 'generationReportSha256', 'verifyReport', 'verifyReportSha256'}
    if not isinstance(material, dict) or set(material) != required:
        raise ValueError('Exact original generation/input/new-PID Verify references required')
    package_path(material['path'])
    records = {}
    for key in ['input', 'generationReport', 'verifyReport']:
        file = owned_path(material[key], 'Reports')
        if sha(file) != material[key + 'Sha256']:
            raise ValueError('Original input/receipt changed: ' + key)
        records[key] = installation_json(file, 65536 if key == 'input' else 1048576)
    generation, verified, request = records['generationReport'], records['verifyReport'], records['input']
    plugin_files = selected_files(data['selectedPlugin'])
    dll = ROOT / data['selectedPlugin'] / 'Binaries/Win64/UnrealEditor-NodeForgeEditor.dll'
    dll_sha = sha(dll)
    target = request.get('target', {}).get('assetPath') if request.get('version') == 3 else request.get('assetPath')
    asset_type = request.get('asset', {}).get('type') if request.get('version') == 3 else request.get('assetType')
    settings = request.get('asset', {}).get('settings') if request.get('version') == 3 else request.get('settings')
    fixture = installation_json(ROOT / 'Reports/PluginControlStageTwo/PersonalCandidate1/PC0/FixturePlan.json', 65536)
    for report in [generation, verified]:
        if report.get('assetPath') != material['path'] or report.get('fileSha256') != material['sha256'] \
                or report.get('inputDigest') != material['inputSha256'] \
                or report.get('executionFingerprint', {}).get('binarySha256') != dll_sha \
                or Path(report.get('project', '')).resolve() != project:
            raise ValueError('Material input/target/project/saved bytes/tested DLL identity differs')
    if target != material['path'] or asset_type != 'Material' or settings != fixture['material']['settings'] \
            or request.get('format') != 'NodeForgeGraph' or request.get('version') not in (2, 3) \
            or generation.get('overallStatus') != 'waitingPersistence' or verified.get('overallStatus') != 'succeeded' \
            or verified.get('mode') != 'Verify' or verified.get('pid') == generation.get('pid') \
            or any(type(report.get('pid')) is not int or report['pid'] <= 0 for report in [generation, verified]) \
            or verified.get('checks', {}).get('persistence', {}).get('status') != 'passed':
        raise ValueError('Actual original same-stage generation and new-PID no-resave Verify required')
    for item in [material, data['tick']]:
        file = project.parent / 'Content' / (item['path'][6:] + '.uasset')
        if sha(file) != item['sha256']:
            raise ValueError('Selected source asset changed')
    return data, plugin_files

MAP_AUTHOR = r'''# Fixed copy-only UE5.3.2 authoring; no NodeForge import or module.
import json
from pathlib import Path
import unreal
c=json.loads(Path(CONFIG_PATH).read_text(encoding='utf-8'))
output=Path(c['report'])
if output.exists(): raise RuntimeError('Original authoring report protected')
r={'result':'failed','map':c['map'],'mapSaveAttempted':False}
try:
    if Path(c['mapFile']).exists():
        raise RuntimeError('Map already exists; refusing overwrite')
    world=unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    if world is None: raise RuntimeError('New blank map failed')
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    plane=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,0,0))
    plane.set_actor_scale3d(unreal.Vector(5,5,5))
    component=plane.get_component_by_class(unreal.StaticMeshComponent)
    component.set_static_mesh(unreal.load_object(None,'/Engine/BasicShapes/Plane.Plane'))
    material=unreal.load_object(None,c['material'])
    if material is None: raise RuntimeError('Verified copied material missing')
    component.set_material(0,material)
    camera=actors.spawn_actor_from_class(unreal.CameraActor,unreal.Vector(0,0,1000),unreal.Rotator(-90,-90,0))
    camera.set_editor_property('auto_activate_for_player',unreal.AutoReceiveInput.PLAYER0)
    cc=camera.get_component_by_class(unreal.CameraComponent)
    cc.set_editor_property('projection_mode',unreal.CameraProjectionMode.ORTHOGRAPHIC)
    cc.set_editor_property('ortho_width',600.0)
    cc.set_editor_property('aspect_ratio',1.0)
    cc.set_editor_property('constrain_aspect_ratio',True)
    pp=cc.get_editor_property('post_process_settings')
    pp.set_editor_property('override_auto_exposure_method',True)
    pp.set_editor_property('auto_exposure_method',unreal.AutoExposureMethod.AEM_MANUAL)
    pp.set_editor_property('override_auto_exposure_apply_physical_camera_exposure',True)
    pp.set_editor_property('auto_exposure_apply_physical_camera_exposure',False)
    pp.set_editor_property('override_bloom_intensity',True);pp.set_editor_property('bloom_intensity',0.0)
    pp.set_editor_property('override_motion_blur_amount',True);pp.set_editor_property('motion_blur_amount',0.0)
    cc.set_editor_property('post_process_settings',pp)
    bp=unreal.load_class(None,c['blueprint']+'_C')
    if bp is None: raise RuntimeError('Unchanged copied Tick class unavailable')
    tick=actors.spawn_actor_from_class(bp,unreal.Vector(0,0,-1000))
    if tick is None: raise RuntimeError('Native Tick actor spawn failed')
    r['tickActorName']=tick.get_name()
    r['mapSaveAttempted']=True
    if not unreal.EditorLoadingAndSavingUtils.save_map(world,c['map']): raise RuntimeError('Map save failed; inspect partial copy')
    registry=unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous(['/Game/NodeForgeTests'],True)
    pending=[c['map']];seen=set();edges=[]
    options=unreal.AssetRegistryDependencyOptions()
    while pending:
        path=pending.pop()
        if path in seen: continue
        if len(seen)>=1024: raise RuntimeError('Dependency closure exceeds fixed bound')
        if path.startswith('/Game/') and path not in c['allowedGamePackages']: raise RuntimeError('Unexpected Game dependency: '+path)
        if not path.startswith(('/Game/','/Engine/','/Script/')) or '/Script/NodeForge' in path: raise RuntimeError('Unexpected mount/generator dependency: '+path)
        seen.add(path)
        dependencies=[str(p) for p in registry.get_dependencies(path,options)]
        edges.append({'package':path,'dependencies':dependencies});pending.extend(dependencies)
    if registry.is_loading_assets(): raise RuntimeError('Dependency registry still scanning')
    r.update(result='passed',dependencies=sorted(seen),dependencyEdges=edges,actorCount=len(actors.get_all_level_actors()))
except Exception as e:
    r['error']=str(e)
    raise
finally:
    with output.open('x',encoding='utf-8') as f: json.dump(r,f,ensure_ascii=False,indent=2)
'''

def prepare_main(argv):
    parser = argparse.ArgumentParser(description='Finite PC copy or finalize; never launches UE')
    parser.add_argument('--personal-manifest', required=True)
    parser.add_argument('--run', required=True)
    parser.add_argument('--finalize-map', action='store_true')
    args = parser.parse_args(argv)
    if not re.fullmatch(r'NFC[A-Za-z0-9]{1,17}', args.run):
        raise ValueError('ASCII NFC project name <=20 required before creating files')
    manifest = owned_path(args.personal_manifest, 'Reports')
    data, validated = load_input(manifest)
    source = ROOT / data['sourceProject']
    target = ROOT / 'ValidationProject/StandaloneChecks' / args.run
    out = ROOT / 'Reports/PluginControlStageTwo/PersonalCandidate1/PC5/Standalone' / args.run
    project = target / (args.run + '.uproject')
    if args.finalize_map:
        from NodeForgeControl import assert_exclusive
        assert_exclusive(project)
        pending = installation_json(out / 'CopyPreparation.json', 1048576)
        if pending.get('inputSha256') != sha(manifest) or pending.get('validatedBuild') != validated \
                or pending.get('projectDescriptorSha256') != sha(project):
            raise ValueError('Original copy manifest/candidate changed')
        report = installation_json(out / 'MapAuthorReport.json', 1048576)
        map_file = target / 'Content' / (data['map'][6:] + '.umap')
        if report.get('result') != 'passed' or report.get('map') != data['map'] or not map_file.is_file() \
                or report.get('mapSaveAttempted') is not True or not report.get('tickActorName'):
            raise ValueError('Actual single saved native map and dependency closure required')
        for item in pending['assets']:
            if sha(ROOT / item['source']) != item['sha256'] or sha(ROOT / item['copy']) != item['sha256']:
                raise ValueError('Copy or source changed during map authoring; stop')
        expected = {row['copy'] for row in pending['assets']} | {ref(map_file)}
        if set(files_under(target / 'Content', ASSET_SUFFIXES)) != expected:
            raise ValueError('Unexpected asset/map in finite copy')
        descriptor = installation_json(project, 65536)
        if descriptor != pending['descriptor']:
            raise ValueError('Copy project configuration changed')
        descriptor['Plugins'] = [{'Name': 'NodeForge', 'Enabled': False}, {'Name': 'PythonScriptPlugin', 'Enabled': False}]
        with (out / 'MapFinalizeProjectBefore.uproject').open('xb') as stream:
            stream.write(project.read_bytes())
        project.write_text(json.dumps(descriptor, indent=2) + '\n', encoding='utf-8')
        case = {'case': 'MaskedTick', 'map': data['map'], 'assetType': 'Material', 'assetPath': data['material']['path'],
                'expectedMessage': TICK_MESSAGE, 'actorName': report['tickActorName'], 'expectedLocation': None}
        cases = out / 'RuntimeCases.json'
        write_new(cases, {'format': 'NodeForgePersonalDeliveryCases', 'version': 1, 'cases': [case]})
        write_new(out / 'Preparation.json', {**pending, 'kind': 'NodeForgePersonalCandidateDelivery1', 'result': 'prepared',
                  'descriptor': descriptor, 'mapSha256': sha(map_file), 'mapAuthorReportSha256': sha(out / 'MapAuthorReport.json'),
                  'projectDescriptorSha256': sha(project),
                  'caseManifest': ref(cases), 'caseManifestSha256': sha(cases)})
        print('Prepared one finite copy map; source assets unchanged; game/cook/runtime not run')
        return 0
    from NodeForgeControl import assert_exclusive
    assert_exclusive(source)
    if target.exists() or out.exists():
        raise ValueError('Copy/report already exists; inspect original partial state instead of repeating')
    target.mkdir(parents=True);out.mkdir(parents=True)
    assets = []
    write_new(out / 'CopyIntent.json', {'input': ref(manifest), 'inputSha256': sha(manifest), 'project': ref(project), 'map': data['map']})
    for item in [data['material'], data['tick']]:
        original = source.parent / 'Content' / (item['path'][6:] + '.uasset')
        copied = target / 'Content' / (item['path'][6:] + '.uasset')
        copied.parent.mkdir(parents=True, exist_ok=True)
        with copied.open('xb') as stream:
            stream.write(original.read_bytes())
        if sha(copied) != item['sha256'] or sha(original) != item['sha256']:
            raise ValueError('Selected copy bytes changed; inspect partial copy')
        assets.append({'source': ref(original), 'copy': ref(copied), 'sha256': item['sha256']})
    descriptor = {'FileVersion': 3, 'EngineAssociation': '5.3', 'Description': 'Fixed native PC delivery copy',
                  'Plugins': [{'Name': 'NodeForge', 'Enabled': False}, {'Name': 'PythonScriptPlugin', 'Enabled': True}]}
    write_new(project, descriptor)
    config = target / 'Config';config.mkdir()
    (config / 'DefaultEngine.ini').write_text('[/Script/EngineSettings.GameMapsSettings]\nGameDefaultMap='+data['map']+'\nEditorStartupMap='+data['map']+'\n\n[/Script/WindowsTargetPlatform.WindowsTargetSettings]\nDefaultGraphicsRHI=DefaultGraphicsRHI_DX11\n+D3D11TargetedShaderFormats=PCD3D_SM5\n\n[/Script/Engine.RendererSettings]\nr.DynamicGlobalIlluminationMethod=0\nr.ReflectionMethod=0\nr.RayTracing=False\nr.DefaultFeature.AutoExposure=False\nr.AntiAliasingMethod=0\n', encoding='utf-8')
    (config / 'DefaultGame.ini').write_text('[/Script/UnrealEd.ProjectPackagingSettings]\nBuildConfiguration=PPBC_Development\nbCookAll=False\nbUsePakFile=True\nbUseIoStore=False\n+MapsToCook=(FilePath="'+data['map']+'")\n', encoding='utf-8')
    author_config = out / 'MapAuthorInput.json'
    write_new(author_config, {'map': data['map'], 'material': data['material']['path']+'.'+data['material']['path'].split('/')[-1],
              'blueprint': TICK+'.BPTickMove', 'report': str(out / 'MapAuthorReport.json'),
              'mapFile': str(target / 'Content' / (data['map'][6:] + '.umap')),
              'allowedGamePackages': [data['map'], data['material']['path'], TICK]})
    (out / 'AuthorMap.py').write_text(MAP_AUTHOR.replace('CONFIG_PATH', repr(str(author_config))), encoding='utf-8')
    write_new(out / 'CopyPreparation.json', {'kind': 'NodeForgePersonalCandidateDelivery1', 'result': 'copiedAwaitingMap',
              'input': ref(manifest), 'inputSha256': sha(manifest), 'project': ref(project), 'descriptor': descriptor,
              'maps': [data['map']], 'assets': assets, 'selectedPlugin': data['selectedPlugin'], 'validatedBuild': validated,
              'projectDescriptorSha256': sha(project),
              'pluginEnabled': False, 'additionalPluginDirectories': [], 'pluginFilesPresent': False})
    print('Copied exactly two selected assets; one map authoring remains; UE not launched')
    return 0

def load_runtime(manifest, project):
    manifest = owned_path(manifest, 'Reports/PluginControlStageTwo/PersonalCandidate1/PC5/Standalone')
    cases = installation_json(manifest, 65536)
    prep = installation_json(manifest.parent / 'Preparation.json', 1048576)
    data, validated = load_input(prep['input'])
    if cases.get('format') != 'NodeForgePersonalDeliveryCases' or cases.get('version') != 1 or len(cases.get('cases', [])) != 1 \
            or prep.get('result') != 'prepared' or prep.get('project') != ref(project) \
            or prep.get('caseManifest') != ref(manifest) or prep.get('caseManifestSha256') != sha(manifest) \
            or prep.get('inputSha256') != sha(ROOT / prep['input']) or prep.get('validatedBuild') != validated \
            or prep.get('projectDescriptorSha256') != sha(project) \
            or prep.get('mapAuthorReportSha256') != sha(manifest.parent / 'MapAuthorReport.json') \
            or sha(project.parent / 'Content' / (data['map'][6:] + '.umap')) != prep.get('mapSha256') \
            or read_json(project) != prep['descriptor']:
        raise ValueError('Actual same-byte finite preparation/map/config/candidate required')
    for item in prep['assets']:
        if sha(ROOT / item['source']) != item['sha256'] or sha(ROOT / item['copy']) != item['sha256']:
            raise ValueError('Source/copy changed before runtime')
    if cases['cases'][0]['map'] != data['map'] or cases['cases'][0]['expectedMessage'] != TICK_MESSAGE:
        raise ValueError('Frozen map/message expectation changed')
    return cases['cases'], prep, ROOT / prep['selectedPlugin'], validated

def masked_picture(path):
    # Existing local Pillow, as in CheckDeliveryEvidence; no new dependency install.
    from PIL import Image
    with Image.open(path) as opened:
        if opened.size != (256, 256):
            raise ValueError('Frozen actual screenshot size must be 256 square')
        image = opened.convert('RGB')
    rows = []
    for name, x, y, width, height, on in [('off',64,96,32,64,False), ('on',160,96,32,64,True), ('outside',4,4,16,16,False)]:
        pixels = list(image.crop((x,y,x+width,y+height)).getdata())
        matching = sum(r >= 32 and g <= .7*r and b <= .7*r if on else max(r,g,b) <= 12 for r,g,b in pixels)
        rows.append({'region': name, 'matchingPixels': matching, 'pixels': len(pixels), 'passed': matching*10 >= len(pixels)*9})
    return {'passed': all(row['passed'] for row in rows), 'image': ref(path), 'imageSha256': sha(path), 'regions': rows,
            'criterion': 'Frozen PC0 RGB8 on/off/outside ROIs >=90%; no image mutation or automatic threshold adjustment'}
