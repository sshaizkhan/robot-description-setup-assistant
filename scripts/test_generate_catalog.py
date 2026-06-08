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


def test_scrape_dof():
    assert g.scrape_dof("abb", "irb_120_3_0_6") == 6
    assert g.scrape_dof("fanuc", "m_410ic_185") == 4
    assert g.scrape_dof("abb", "does_not_exist") == 0


def test_titleize():
    assert g.titleize("irb_120_3_0_6") == "Irb 120 3 0 6"
    assert g.titleize("crx_10ia_l") == "Crx 10ia L"


def test_build_entry_shape():
    e = g.build_entry("abb", "irb_120_3_0_6")
    assert e["type"] == "arm"
    assert e["display_name"] == "ABB Irb 120 3 0 6"
    assert e["image_path"] == g.PLACEHOLDER_IMAGE
    assert e["urdf_package"] == "abb_robot_descriptions"
    assert e["urdf_path"] == "urdf/irb_120_3_0_6.urdf.xacro"
    assert e["xacro_args"] == ""
    assert e["category"] == "abb"
    assert e["attach"]["tool_frame"] == "tool0"
    assert e["specifications"]["degrees_of_freedom"] == 6
    assert e["required_packages"] == ["abb_robot_descriptions"]
    assert e["tags"] == ["industrial"]
