"""Install the matching AI addon into one project using the existing installation checks."""
import argparse
import hashlib
import json
import os
import platform
import shutil
import subprocess
import sys
import time
import tomllib
import urllib.request
import urllib.parse
import uuid
import zipfile
from pathlib import Path

from RunProcessGuard import installation_path, installation_json, installation_product, load_installation_binding, process_identity
from PrepareNodeForgeInstallation import selected_project_closed, replace_exact, encoded

ROOT = Path(__file__).resolve().parents[1]


def child_environment():
    return {key: value for key, value in os.environ.items() if key != 'NODEFORGE_GITHUB_TOKEN'}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def product_project(value):
    path = installation_path(value)
    if path.is_dir():
        projects = list(path.glob('*.uproject'))
        if len(projects) != 1:
            raise ValueError('projectAmbiguous: specify one uproject; no project selected automatically')
        path = projects[0]
    if path.suffix != '.uproject' or not path.is_file():
        raise ValueError('projectMissing: an existing UE project is required')
    return path


def product_engine(project, explicit):
    if explicit:
        return installation_path(explicit)
    binding = project.parent / 'Config/NodeForgeInstallation.json'
    if binding.is_file():
        return installation_path(installation_json(binding)['engineRoot'])
    import winreg
    association = installation_json(project).get('EngineAssociation')
    candidates = []
    try:
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, 'SOFTWARE\\EpicGames\\Unreal Engine\\' + str(association)) as key:
            candidates.append(winreg.QueryValueEx(key, 'InstalledDirectory')[0])
    except FileNotFoundError:
        pass
    try:
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r'SOFTWARE\Epic Games\Unreal Engine\Builds') as key:
            candidates.append(winreg.QueryValueEx(key, str(association))[0])
    except FileNotFoundError:
        pass
    candidates = list(dict.fromkeys(candidates))
    if len(candidates) != 1:
        raise ValueError('engineAmbiguous: exact engine root required')
    return installation_path(candidates[0])


def package():
    manifest = installation_json(ROOT / 'AiPackageManifest.json', 4 * 1048576)
    if os.name != 'nt' or platform.machine().lower() not in ('amd64', 'x86_64'):
        raise ValueError('platformUnsupported: Windows x64 required')
    paths = set()
    for row in manifest['files']:
        path = installation_path(ROOT / row['path'])
        if not path.is_relative_to(ROOT) or row['path'].casefold() in paths:
            raise ValueError('packagePathInvalid')
        paths.add(row['path'].casefold())
        if not path.is_file() or path.stat().st_size != row['bytes'] or digest(path) != row['sha256']:
            raise ValueError('packageHashMismatch: ' + row['path'])
    if 'Scripts/NodeForgeAiInstall.py'.casefold() not in paths:
        raise ValueError('packageInstallerMissing')
    return manifest


def invoke(root, arguments):
    interpreter = root / 'Tools/NodeForgeMcp/Runtime/python.exe'
    child = subprocess.run([str(interpreter), str(root / 'Scripts/PrepareNodeForgeInstallation.py'), *arguments],
        cwd=root, capture_output=True, text=True, encoding='utf-8', timeout=45,
        creationflags=subprocess.CREATE_NO_WINDOW, env=child_environment())
    value = json.loads(child.stdout)
    if child.returncode or value['status'] != 'passed':
        raise ValueError('installationValidationFailed: ' + json.dumps(value, ensure_ascii=False))
    return value


def client_plan(path, kind, name, command):
    path = installation_path(path)
    before = path.read_bytes() if path.exists() else None
    if before is not None and len(before) > 1048576:
        raise ValueError('clientConfigTooLarge')
    if not name or not all(c.isalnum() or c in '-_' for c in name) or len(name) > 64:
        raise ValueError('clientNameInvalid')
    entry = {'command': command['command'], 'args': command['args']}
    if kind == 'codex-toml':
        entry.update(cwd=command['cwd'], startup_timeout_sec=30, tool_timeout_sec=200)
        text = before.decode('utf-8-sig') if before else ''
        data = tomllib.loads(text)
        old = data.get('mcp_servers', {}).get(name)
        if old is not None:
            if old != entry:
                raise ValueError('clientEntryConflict: use a different server name or explicitly remove the old registration')
            return path, before, before, entry
        section = '\n[mcp_servers.' + name + ']\n' + '\n'.join(
            key + ' = ' + json.dumps(value, ensure_ascii=False) for key, value in entry.items()) + '\n'
        after = (text + section).encode('utf-8')
        if tomllib.loads(after.decode('utf-8'))['mcp_servers'][name] != entry:
            raise ValueError('clientConfigInvalid')
    else:
        data = installation_json(path, 1048576) if before else {}
        servers = data.setdefault('mcpServers', {})
        if not isinstance(servers, dict):
            raise ValueError('clientConfigInvalid')
        if name in servers:
            if servers[name] != entry:
                raise ValueError('clientEntryConflict: use a different server name or explicitly remove the old registration')
            return path, before, before, entry
        servers[name] = entry
        after = encoded(data)
    return path, before, after, entry


def install_verified(args, manifest, project, engine, target):
    receipt_dir = ROOT / 'InstallReceipts' / uuid.uuid4().hex
    receipt_dir.mkdir(parents=True, exist_ok=False)
    receipt_path = receipt_dir / 'Receipt.json'
    receipt = {'format': 'NodeForgeAiInstallReceipt1', 'aiVersion': manifest['aiVersion'],
        'project': str(project), 'pluginDirectory': str(target), 'engineRoot': str(engine),
        'status': 'preparing', 'completedWrites': [], 'requiresReview': False}
    def save():
        receipt_path.write_bytes(encoded(receipt))
    save()
    try:
        if not target.exists():
            plugin_zip = installation_path(args.plugin_zip) if args.plugin_zip else receipt_dir / manifest['pluginAsset']['name']
            if not args.plugin_zip:
                request = urllib.request.Request(manifest['pluginAsset']['url'])
                # Private repositories require an explicit token in the process environment.
                token = os.environ.get('NODEFORGE_GITHUB_TOKEN')
                if token:
                    request = urllib.request.Request(manifest['pluginAsset']['apiUrl'],
                        headers={'Authorization': 'Bearer ' + token, 'Accept': 'application/octet-stream'})
                opener = urllib.request.build_opener(SafeRedirect())
                with opener.open(request, timeout=45) as stream, plugin_zip.open('xb') as out:
                    shutil.copyfileobj(stream, out)
            if digest(plugin_zip) != manifest['pluginAsset']['sha256']:
                raise ValueError('pluginArchiveHashMismatch')
            extracted = receipt_dir / 'PluginExtract'
            extracted.mkdir()
            with zipfile.ZipFile(plugin_zip) as archive:
                seen = set()
                for item in archive.infolist():
                    destination = installation_path(extracted / item.filename)
                    if (not destination.is_relative_to(extracted) or item.filename.casefold() in seen
                            or item.filename.endswith('/') or item.external_attr >> 16 & 0o170000 == 0o120000):
                        raise ValueError('pluginArchivePathInvalid')
                    seen.add(item.filename.casefold())
                    destination.parent.mkdir(parents=True, exist_ok=True)
                    with destination.open('xb') as out:
                        out.write(archive.read(item))
            stage = extracted / 'NodeForge'
            installation_product(stage, engine)
            target.parent.mkdir(parents=True, exist_ok=True)
            selected_project_closed(project)
            os.rename(stage, target)
            receipt['completedWrites'].append({'kind': 'pluginInstalled', 'path': str(target)})
            save()
        _, _, _, _, product = installation_product(target, engine)
        if product['binarySha256'] != manifest['binarySha256'] or product != manifest['installationManifest']:
            raise ValueError('coreVersionMismatch: plugin and AI package versions must match')
        planned = []
        for row in manifest['files']:
            relative = row['path']
            source, destination = ROOT / relative, installation_path(target / relative)
            if destination.exists():
                if not destination.is_file() or digest(destination) != row['sha256']:
                    raise ValueError('addonFileConflict: ' + relative)
            else:
                planned.append((source, destination, row))
        binding_path = project.parent / 'Config/NodeForgeInstallation.json'
        if binding_path.exists():
            load_installation_binding(project, target, target)
        # Validate the client's schema before changing the project/configuration.
        if args.client_config:
            client_plan(args.client_config, args.client_format, args.server_name,
                {'command': str(target / 'Tools/NodeForgeMcp/Runtime/python.exe'),
                 'args': [str(target / 'Scripts/NodeForgeAiLaunch.py')], 'cwd': str(target)})
        selected_project_closed(project)
        for source, destination, row in planned:
            destination.parent.mkdir(parents=True, exist_ok=True)
            with destination.open('xb') as out:
                out.write(source.read_bytes())
            receipt['completedWrites'].append({'path': str(destination), 'sha256': row['sha256']})
            save()
        if not binding_path.exists():
            receipt['binding'] = invoke(target, ['--action', 'bind', '--installation-root', str(target),
                '--project', str(project), '--engine-root', str(engine), '--read-root', '/Game/NodeForgeTests',
                '--read-root', '/Engine/EngineResources', '--read-root', '/Engine/Functions/Engine_MaterialFunctions02'])
            save()
        stable = target / 'Tools/NodeForgeMcp/ClientState.json'
        reuse = stable.exists()
        if reuse:
            state = installation_json(stable)
            config = installation_path(state['config'])
            if not config.is_relative_to(target / 'Reports/PluginControlStageTwo'):
                raise ValueError('clientStateInvalid')
            commands = installation_json(config.parent / 'Commands.json')
            public = config.parent / 'SessionPublic.json'
            if public.exists():
                identity = installation_json(public)
                private = installation_path(identity['sessionFile'])
                if private.exists() or process_identity(identity['pid']).get('state') == 'unknown':
                    raise ValueError('previousSessionUnresolved: preserve the original session and receipt')
                reuse = False
        if not reuse:
            prepared = invoke(target, ['--action', 'prepare-editor', '--installation-root', str(target),
                '--project', str(project), '--engine-root', str(engine),
                '--python', str(target / 'Tools/NodeForgeMcp/Runtime/python.exe')])
            commands = prepared['commands']
            config = Path(prepared['config'])
            replace_exact(stable, digest(stable) if stable.exists() else None,
                encoded({'format': 'NodeForgeAiClientState1', 'config': str(config)}))
            receipt['completedWrites'].append({'path': str(stable), 'sha256': digest(stable)})
            save()
        command = {'command': str(target / 'Tools/NodeForgeMcp/Runtime/python.exe'),
            'args': [str(target / 'Scripts/NodeForgeAiLaunch.py')], 'cwd': str(target)}
        if args.client_config:
            path, before, after, entry = client_plan(args.client_config, args.client_format, args.server_name, command)
            if before != after:
                replace_exact(path, hashlib.sha256(before).hexdigest() if before is not None else None, after)
            receipt['client'] = {'path': str(path), 'format': args.client_format, 'name': args.server_name,
                'entry': entry, 'changed': before != after, 'afterSha256': digest(path)}
            save()
        checked = subprocess.run([command['command'], str(target / 'Scripts/NodeForgeAiCheck.py'),
            '--config', str(config)], cwd=target, capture_output=True, text=True, encoding='utf-8',
            timeout=45, creationflags=subprocess.CREATE_NO_WINDOW, env=child_environment())
        if checked.returncode:
            raise ValueError('stdioDiscoveryFailed: ' + checked.stderr[-1000:] + checked.stdout[-1000:])
        receipt.update(status='installed', requiresReview=False, config=str(config), mcp=command,
            editor=commands['editor'], stop=commands['afterStartup'], close=commands['normalCloseAfterChannelStop'],
            connectionCheck=json.loads(checked.stdout), ueStarted=False,
            nextAction='Start the returned editor command, then run NodeForgeAiCheck.py --config <config> --connect')
        save()
        if args.connect:
            editor = commands['editor']
            editor_args = editor['args']
            selected_project_closed(project)
            startup = subprocess.STARTUPINFO()
            startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
            startup.wShowWindow = 1
            with (receipt_dir / 'EditorLaunchConsole.txt').open('xb') as console:
                child = subprocess.Popen([editor['command'], *editor_args], cwd=target, stdout=console,
                    stderr=console, startupinfo=startup, creationflags=subprocess.CREATE_NO_WINDOW,
                    env=child_environment())
            receipt.update(ueStarted=True, ownedEditorPid=child.pid, status='startingEditor')
            save()
            deadline = time.monotonic() + 180
            while not (config.parent / 'SessionPublic.json').exists():
                if child.poll() is not None:
                    raise ValueError('editorStartupFailed: inspect the original launch log')
                if time.monotonic() >= deadline:
                    raise ValueError('editorStartupPending: original process preserved; inspect it before retry')
                time.sleep(0.25)
            checked = subprocess.run([command['command'], str(target / 'Scripts/NodeForgeAiCheck.py'),
                '--config', str(config), '--connect'], cwd=target, capture_output=True, text=True,
                encoding='utf-8', timeout=45, creationflags=subprocess.CREATE_NO_WINDOW, env=child_environment())
            if checked.returncode:
                (receipt_dir / 'ConnectionCheckStdout.txt').write_text(checked.stdout, encoding='utf-8')
                (receipt_dir / 'ConnectionCheckStderr.txt').write_text(checked.stderr, encoding='utf-8')
                raise ValueError('editorConnectionFailed: ' + checked.stderr[-1000:] + checked.stdout[-1000:])
            receipt.update(status='connected', connectionCheck=json.loads(checked.stdout))
            save()
        return {'status': receipt['status'], 'aiVersion': manifest['aiVersion'], 'project': str(project),
            'pluginDirectory': str(target), 'receipt': str(receipt_path), 'config': str(config),
            'mcp': command, 'editor': commands['editor'], 'connection': receipt['connectionCheck']['connection'],
            'clientConfigured': bool(args.client_config), 'discovery': receipt['connectionCheck']}
    except BaseException as error:
        receipt.update(status='failed', errorType=type(error).__name__, message=str(error),
            requiresReview=bool(receipt['completedWrites'] or receipt.get('binding') or receipt.get('client')))
        save()
        error.install_receipt = str(receipt_path)
        raise


class SafeRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, request, response, code, message, headers, new_url):
        result = super().redirect_request(request, response, code, message, headers, new_url)
        if result is not None and urllib.parse.urlparse(request.full_url).netloc != urllib.parse.urlparse(new_url).netloc:
            result.remove_header('Authorization')
        return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project', required=True, help='Exact project or a directory containing exactly one uproject')
    parser.add_argument('--engine-root', help='Defaults to the existing binding or exact registered engine association')
    parser.add_argument('--plugin-zip', help='Optional matching ordinary plugin ZIP; otherwise downloads the fixed Release asset')
    parser.add_argument('--client-config', help='Exact client file; no global configuration path is guessed')
    parser.add_argument('--client-format', choices=['mcp-json', 'codex-toml'], default='mcp-json')
    parser.add_argument('--server-name', default='nodeforge')
    parser.add_argument('--connect', action='store_true', help='Launch one owned UE editor and verify the real read-only connection')
    args = parser.parse_args()
    try:
        # Prevalidate against ordinary identity metadata without assuming an addon contains the core.
        manifest = package()
        project = product_project(args.project)
        selected_project_closed(project)
        engine = product_engine(project, args.engine_root)
        target = installation_path(project.parent / 'Plugins/NodeForge')
        if target == ROOT or ROOT.is_relative_to(target):
            raise ValueError('installSourceConflict: extract addon outside the installed plugin')
        result = install_verified(args, manifest, project, engine, target)
    except Exception as error:
        result = {'status': 'rejected', 'errorType': type(error).__name__, 'message': str(error),
            'receipt': getattr(error, 'install_receipt', None)}
        print(json.dumps(result, ensure_ascii=False))
        return 2
    print(json.dumps(result, ensure_ascii=False))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
