"""Stable installation entry; the existing bridge discovers exact editor generations."""
import sys
from pathlib import Path
from RunProcessGuard import installation_user_directory, load_ai_installation

root = Path(__file__).resolve().parents[1]
config = installation_user_directory(root) / 'Installation.json'
load_ai_installation(config, root)
from NodeForgeMcp import main
sys.argv = [str(root / 'Scripts/NodeForgeMcp.py'), '--config', str(config)]
raise SystemExit(main())
