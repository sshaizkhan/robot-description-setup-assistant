import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import generate_catalog as g


def test_vendors_and_paths():
    assert set(g.VENDORS) == {"kuka", "abb", "fanuc", "yaskawa"}
    assert g.VENDORS["abb"]["package"] == "abb_robot_descriptions"
    assert g.DEPS.is_dir()
    assert g.vendor_dir("abb").is_dir()
    assert (g.vendor_dir("abb") / "urdf").is_dir()
    assert g.ARMS_OUT.is_dir()
