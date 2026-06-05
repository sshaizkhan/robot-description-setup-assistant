import os
from pathlib import Path

import pytest

# web/backend/tests/conftest.py -> repo root is 3 levels up.
REPO_ROOT = Path(__file__).resolve().parents[3]
# Canonical multi-file catalog (directory of *.yml). RobotCatalog.from_file
# detects a directory and merges every file under it.
ROBOTS_YAML = (
    REPO_ROOT
    / "robot_description_setup_assistant"
    / "config"
    / "catalog"
)

# Ensure the module-level `app = create_app()` in rdsa_web.app can locate the
# catalog at import time without requiring an installed ROS package.
os.environ.setdefault("RDSA_ROBOTS_YAML", str(ROBOTS_YAML))


@pytest.fixture
def robots_yaml_path() -> Path:
    assert ROBOTS_YAML.exists(), f"missing catalog dir: {ROBOTS_YAML}"
    return ROBOTS_YAML
