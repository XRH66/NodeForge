"""Client's stable stdio entry; every editor generation gets an explicit preparation."""
import sys
from pathlib import Path
from RunProcessGuard import installation_json, installation_path

root = Path(__file__).resolve().parents[1]
state = installation_json(root / 'Tools/NodeForgeMcp/ClientState.json')
config = installation_path(state['config'])
if not config.is_relative_to(root / 'Reports/PluginControlStageTwo'):
    raise ValueError('Client configuration outside the installed product')
from NodeForgeMcp import main
sys.argv = [str(root / 'Scripts/NodeForgeMcp.py'), '--config', str(config)]
raise SystemExit(main())
