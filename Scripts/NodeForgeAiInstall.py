"""Install one matching AI addon and current-user client entry; projects start normally."""
import argparse
import hashlib
import json
import os
import platform
import re
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

from RunProcessGuard import installation_path, installation_json, installation_product, load_installation_binding, process_identity, installation_user_directory, read_ai_installation
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


def owned_legacy_client(entry):
    """Identify the shipped v0.1.0 entry from actual installed metadata, never its name."""
    try:
        if not isinstance(entry, dict) or not {'command','args'} <= set(entry) <= {'command','args','cwd','startup_timeout_sec','tool_timeout_sec'}:
            return False
        executable = installation_path(entry['command'])
        old_root = executable.parents[3]
        if executable != old_root / 'Tools/NodeForgeMcp/Runtime/python.exe' or not executable.is_file(): return False
        if entry['args'] != [str(old_root / 'Scripts/NodeForgeAiLaunch.py')]: return False
        if 'cwd' in entry and installation_path(entry['cwd']) != old_root: return False
        state = installation_json(old_root / 'Tools/NodeForgeMcp/ClientState.json')
        if set(state) != {'format','config'} or state['format'] != 'NodeForgeAiClientState1': return False
        path = installation_path(state['config'])
        if not path.is_relative_to(old_root / 'Reports/PluginControlStageTwo'): return False
        config = installation_json(path)
        project = installation_path(config['project'])
        binding = installation_json(project.parent / 'Config/NodeForgeInstallation.json')
        if binding.get('format') != 'NodeForgeInstallationBinding' or binding.get('version') != 1: return False
        _, _, _, _, product = installation_product(old_root, binding['engineRoot'])
        if (installation_json(old_root / 'NodeForge.uplugin').get('VersionName') != '0.1.0'
                or installation_path(config['pluginDirectory']) != old_root
                or config['binarySha256'] != product['binarySha256']
                or installation_path(binding['pluginDirectory']) != old_root
                or binding['binarySha256'] != product['binarySha256']): return False
        contract_path = old_root / 'Tools/NodeForgeMcp/ToolContract.json'
        contract = installation_json(contract_path, 1048576)
        docs = installation_path(old_root / contract['docsManifest'])
        if (not docs.is_relative_to(old_root / 'Docs/Versions')
                or digest(contract_path) != config['toolContractSha256']
                or digest(docs) != config['docsManifestSha256']): return False
        selected_project_closed(project)
        public = path.parent / 'SessionPublic.json'
        if public.exists() and Path(installation_json(public)['sessionFile']).exists():
            raise ValueError('previousSessionUnresolved: original registration preserved')
        return True
    except (ValueError, OSError, KeyError, IndexError, TypeError):
        return False


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
                if not owned_legacy_client(old): raise ValueError('clientEntryConflict: unknown registration preserved')
                section = re.search(r'(?m)^\[mcp_servers\.' + re.escape(name) + r'\]\s*\n(?P<body>(?:(?!^\[).|\n)*)', text)
                if section is None: raise ValueError('clientMigrationNeedsExplicitReview')
                body = section.group('body')
                for key, value in entry.items():
                    pattern = r'(?m)^' + re.escape(key) + r'\s*=\s*' + re.escape(json.dumps(old[key],ensure_ascii=False)) + r'(\s*(?:#.*)?)$'
                    body, changed = re.subn(pattern, lambda match: key+' = '+json.dumps(value,ensure_ascii=False)+match.group(1), body)
                    if changed != 1: raise ValueError('clientMigrationNeedsExplicitReview: nonstandard owned table preserved')
                after = (text[:section.start('body')]+body+text[section.end('body'):]).encode('utf-8')
                expected = {**data, 'mcp_servers':{**data['mcp_servers'],name:entry}}
                if tomllib.loads(after.decode('utf-8')) != expected: raise ValueError('clientMigrationInvalid')
                return path,before,after,entry
            else: return path, before, before, entry
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
                if not owned_legacy_client(servers[name]): raise ValueError('clientEntryConflict: unknown registration preserved')
            else: return path, before, before, entry
        servers[name] = entry
        after = encoded(data)
    return path, before, after, entry


def selected_installation_closed(engine):
    from NodeForgeControl import engine_processes, command_arguments
    for item in engine_processes():
        arguments = command_arguments(item.get('CommandLine') or '')
        if not arguments:
            raise ValueError('editorProcessUnknown: no files replaced')
        executable = installation_path(arguments[0])
        if executable.is_relative_to(engine / 'Engine/Binaries'):
            raise ValueError('installationInUse: normally close editors using this engine before installing')


def install_verified(args, manifest, project, engine, target):
    previous = None
    if args.resume_receipt or args.upgrade_receipt:
        from NodeForgeEditorTransport import Windows, strict_json
        original = installation_path(args.resume_receipt or args.upgrade_receipt)
        scope = installation_user_directory(target) / 'InstallReceipts'
        if not original.is_relative_to(scope): raise ValueError('resumeReceiptScope')
        previous = strict_json(Windows().bounded_public_file(original, 4 * 1048576, require_user_owner=True))
        if (previous.get('format') != 'NodeForgeAiInstallReceipt1'
                or previous.get('status') != ('installed' if args.upgrade_receipt else 'failed')
                or previous.get('aiVersion') != manifest['aiVersion']
                or installation_path(previous['pluginDirectory']) != target
                or installation_path(previous['engineRoot']) != engine):
            raise ValueError('resumeReceiptIdentity')
        # A receipt can contain more than one committed write to one file.
        # The last committed fingerprint is the current state; earlier entries
        # remain the ordered audit history, not contradictory final assertions.
        for row in {w['path']:w for w in previous['completedWrites']}.values():
            if 'sha256' in row:
                path = installation_path(row['path'])
                user_config = installation_user_directory(target) / 'Installation.json'
                if (not (path.is_relative_to(target) or path == user_config)
                        or not path.is_file() or digest(path) != row['sha256']):
                    raise ValueError('resumePartialFileChanged: original receipt preserved')
        receipt_dir = original.parent
        receipt_path = receipt_dir / ('ReceiptResume' + uuid.uuid4().hex + '.json')
    else:
        receipt_dir = installation_user_directory(target) / 'InstallReceipts' / uuid.uuid4().hex
        receipt_dir.mkdir(parents=True, exist_ok=False)
        receipt_path = receipt_dir / 'Receipt.json'
    receipt = {'format': 'NodeForgeAiInstallReceipt1', 'aiVersion': manifest['aiVersion'],
        'project': str(project) if project else None, 'pluginDirectory': str(target), 'engineRoot': str(engine),
        'status': 'preparing', 'completedWrites': [], 'requiresReview': False}
    if previous:
        receipt.update(resumes=str(original), originalReceiptSha256=digest(original),
            completedWrites=list(previous['completedWrites']))
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
            selected_project_closed(project) if project else selected_installation_closed(engine)
            # User receipts can be on a different volume from the engine.
            # Copy exact verified bytes into a new same-volume staging directory,
            # then atomically publish it without replacing an existing plugin.
            pending_root = installation_path(target.parent / ('NodeForgeInstallPending' + uuid.uuid4().hex))
            pending_root.mkdir(exist_ok=False)
            pending = pending_root / 'NodeForge'
            receipt['completedWrites'].append({'kind':'pluginStagingDirectory', 'path':str(pending_root)})
            save()
            shutil.copytree(stage, pending)
            for source in stage.rglob('*'):
                if source.is_file() and digest(source) != digest(pending / source.relative_to(stage)):
                    raise ValueError('pluginStagingHashMismatch')
            installation_product(pending, engine)
            selected_project_closed(project) if project else selected_installation_closed(engine)
            if target.exists(): raise ValueError('pluginTargetOccupied: staging preserved for review')
            os.rename(pending, target)
            pending_root.rmdir()  # Exact owned, empty same-volume staging container only.
            receipt['completedWrites'].append({'kind': 'pluginInstalled', 'path': str(target)})
            save()
        _, _, _, _, product = installation_product(target, engine)
        if product['binarySha256'] != manifest['binarySha256'] or product != manifest['installationManifest']:
            raise ValueError('coreVersionMismatch: plugin and AI package versions must match')
        planned = []
        revised = []
        for row in manifest['files']:
            relative = row['path']
            source, destination = ROOT / relative, installation_path(target / relative)
            if destination.exists():
                if not destination.is_file() or digest(destination) != row['sha256']:
                    owned = previous and next((w for w in previous['completedWrites']
                        if w.get('path') == str(destination) and w.get('sha256') == digest(destination)), None)
                    if not owned: raise ValueError('addonFileConflict: ' + relative)
                    revised.append((source, destination, row, owned['sha256']))
            else:
                planned.append((source, destination, row))
        binding_path = project.parent / 'Config/NodeForgeInstallation.json' if project else None
        if binding_path and binding_path.exists():
            load_installation_binding(project, target, target)
        command = {'command': str(target / 'Tools/NodeForgeMcp/Runtime/python.exe'),
            'args': [str(target / 'Scripts/NodeForgeAiLaunch.py')], 'cwd': str(target)}
        if args.client_config:
            client_plan(args.client_config, args.client_format, args.server_name, command)
        selected_installation_closed(engine)
        for source, destination, row, before_sha in revised:
            replace_exact(destination, before_sha, source.read_bytes())
            receipt['completedWrites'] = [w for w in receipt['completedWrites'] if w.get('path') != str(destination)]
            receipt['completedWrites'].append({'path':str(destination), 'sha256':row['sha256'], 'beforeSha256':before_sha})
            save()
        for source, destination, row in planned:
            destination.parent.mkdir(parents=True, exist_ok=True)
            with destination.open('xb') as out:
                out.write(source.read_bytes())
            receipt['completedWrites'].append({'path': str(destination), 'sha256': row['sha256']})
            save()
        installed_manifest = target / 'AiPackageManifest.json'
        manifest_bytes = (ROOT / 'AiPackageManifest.json').read_bytes()
        if installed_manifest.exists() and installed_manifest.read_bytes() != manifest_bytes:
            recorded = previous and next((w for w in previous['completedWrites']
                if w.get('path') == str(installed_manifest) and w.get('sha256') == digest(installed_manifest)), None)
            if not recorded: raise ValueError('addonManifestConflict: require exact owned upgrade receipt')
            replace_exact(installed_manifest, recorded['sha256'], manifest_bytes)
        elif not installed_manifest.exists():
            replace_exact(installed_manifest, None, manifest_bytes)
        receipt['completedWrites'] = [w for w in receipt['completedWrites'] if w.get('path') != str(installed_manifest)]
        receipt['completedWrites'].append({'path':str(installed_manifest), 'sha256':digest(installed_manifest)})
        save()
        if binding_path and not binding_path.exists():
            receipt['binding'] = invoke(target, ['--action', 'bind', '--installation-root', str(target),
                '--project', str(project), '--engine-root', str(engine), '--read-root', '/Game/NodeForgeTests',
                '--read-root', '/Engine/EngineResources', '--read-root', '/Engine/Functions/Engine_MaterialFunctions02'])
            save()
        contract_path = target / 'Tools/NodeForgeMcp/ToolContract.json'
        contract = installation_json(contract_path, 1048576)
        docs_path = installation_path(target / contract['docsManifest'])
        if not docs_path.is_relative_to(target / 'Docs/Versions'):
            raise ValueError('documentationScopeInvalid')
        config = installation_user_directory(target) / 'Installation.json'
        desired = {'format':'NodeForgeAiInstallation1', 'version':1, 'enabled':True,
            'pluginDirectory':target.as_posix(), 'engineRoot':engine.as_posix(),
            'binarySha256':product['binarySha256'], 'bundleId':contract['bundleId'],
            'toolContractSha256':digest(contract_path), 'docsManifestSha256':digest(docs_path)}
        if config.exists():
            current = read_ai_installation(config, target)
            if current != desired:
                # Only a prior, completed product receipt owns this exact old state.
                # Unknown entries, residual credentials and partial writes stop activation.
                prior = list((config.parent / 'InstallReceipts').glob('*/Receipt*.json'))
                if len(prior) > 128: raise ValueError('installationReceiptLimit')
                owned = any(r.get('status') == 'installed' and r.get('requiresReview') is False
                    and any(w.get('path') == str(config) and w.get('sha256') == digest(config)
                            for w in r.get('completedWrites', []))
                    for r in (installation_json(p, 4 * 1048576) for p in prior))
                if not owned or current['engineRoot'] != engine.as_posix():
                    raise ValueError('aiInstallationUpgradeUnknown: original state preserved')
                indexes = list((config.parent / 'Sessions').glob('*.json'))
                if len(indexes) > 128: raise ValueError('sessionDiscoveryLimit')
                if any(Path(installation_json(p)['sessionFile']).exists() for p in indexes):
                    raise ValueError('previousSessionUnresolved: inspect original session and report')
                receipt['previousInstallation'] = current
                receipt['previousInstallationSha256'] = digest(config)
                save()
                replace_exact(config, receipt['previousInstallationSha256'], encoded(desired))
                receipt['completedWrites'].append({'path':str(config), 'sha256':digest(config), 'kind':'installationUpgraded'})
                save()
        else:
            replace_exact(config, None, encoded(desired))
            receipt['completedWrites'].append({'path':str(config), 'sha256':digest(config)})
            save()
        if args.client_config:
            path, before, after, entry = client_plan(args.client_config, args.client_format, args.server_name, command)
            if before != after:
                replace_exact(path, hashlib.sha256(before).hexdigest() if before is not None else None, after)
            receipt['client'] = {'path': str(path), 'format': args.client_format, 'name': args.server_name,
                'entry': entry, 'changed': before != after, 'afterSha256': digest(path),
                'beforeSha256':hashlib.sha256(before).hexdigest() if before is not None else None}
            save()
        checked = subprocess.run([command['command'], str(target / 'Scripts/NodeForgeAiCheck.py'),
            '--config', str(config)], cwd=target, capture_output=True, text=True, encoding='utf-8',
            timeout=45, creationflags=subprocess.CREATE_NO_WINDOW, env=child_environment())
        if checked.returncode:
            raise ValueError('stdioDiscoveryFailed: ' + checked.stderr[-1000:] + checked.stdout[-1000:])
        receipt.update(status='installed', requiresReview=False, config=str(config), mcp=command,
            connectionCheck=json.loads(checked.stdout), ueStarted=False,
            nextAction='Open any enabled project normally; confirm first-use binding if needed, then nodeforge_connect selects an exact session.')
        save()
        return {'status':receipt['status'], 'aiVersion':manifest['aiVersion'],
            'project':str(project) if project else None, 'pluginDirectory':str(target),
            'receipt':str(receipt_path), 'config':str(config), 'mcp':command,
            'clientConfigured':bool(args.client_config), 'discovery':receipt['connectionCheck'], 'ueStarted':False}
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
    parser.add_argument('--project', help='Exact project or a directory containing exactly one uproject')
    parser.add_argument('--engine-root', help='Defaults to the existing binding or exact registered engine association')
    parser.add_argument('--plugin-zip', help='Optional matching ordinary plugin ZIP; otherwise downloads the fixed Release asset')
    parser.add_argument('--client-config', help='Exact client file; no global configuration path is guessed')
    parser.add_argument('--client-format', choices=['mcp-json', 'codex-toml'], default='mcp-json')
    parser.add_argument('--server-name', default='nodeforge')
    parser.add_argument('--plugin-directory', help='Optional exact supported installed layout; defaults to this engine Marketplace')
    parser.add_argument('--resume-receipt', help='Exact original failed receipt; verify all partial bytes before continuing the same installation')
    parser.add_argument('--upgrade-receipt', help='Exact completed owned receipt for a recorded same-core addon update')
    args = parser.parse_args()
    try:
        # Prevalidate against ordinary identity metadata without assuming an addon contains the core.
        manifest = package()
        project = product_project(args.project) if args.project else None
        if not args.engine_root and project is None:
            raise ValueError('engineRequired: select the official UE5.3.2 engine directory once')
        engine = product_engine(project, args.engine_root)
        if project: selected_project_closed(project)
        selected_installation_closed(engine)
        locations = [engine / 'Engine/Plugins/Marketplace/NodeForge', engine / 'Engine/Plugins/NodeForge']
        if project: locations.append(project.parent / 'Plugins/NodeForge')
        present = [path for path in locations if (path / 'NodeForge.uplugin').exists()]
        if len(present) > 1: raise ValueError('installationAmbiguous: existing copies preserved')
        target = installation_path(args.plugin_directory) if args.plugin_directory else (present[0] if present else locations[0])
        if target not in locations: raise ValueError('installationLayoutUnsupported')
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
