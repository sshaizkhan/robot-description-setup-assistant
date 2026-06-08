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


def test_discover_types_counts_and_excludes_dispatch():
    abb = g.discover_types("abb")
    assert len(abb) == 99
    assert "irb_120_3_0_6" in abb
    assert abb == sorted(abb)

    kuka = g.discover_types("kuka")
    assert "kuka" not in kuka
    assert len(kuka) > 100

    for t in g.discover_types("yaskawa"):
        assert (g.vendor_dir("yaskawa") / "urdf" / f"{t}.urdf.xacro").is_file()
        assert (g.vendor_dir("yaskawa") / "config" / t).is_dir()
