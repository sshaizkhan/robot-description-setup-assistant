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
    assert len(abb) >= 99
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


def test_build_vendor_doc():
    doc = g.build_vendor_doc("fanuc", ["m_10ia", "m_20ia"])
    assert doc["categories"]["fanuc"]["manufacturer"] == "FANUC"
    assert set(doc["robots"]) == {"m_10ia", "m_20ia"}
    assert doc["robots"]["m_10ia"]["category"] == "fanuc"


def test_write_doc_roundtrip(tmp_path, monkeypatch):
    import yaml as _yaml
    monkeypatch.setattr(g, "ARMS_OUT", tmp_path)
    doc = g.build_vendor_doc("abb", ["irb_120_3_0_6"])
    out = g.write_doc("abb", doc)
    assert out == tmp_path / "abb.yml"
    text = out.read_text()
    assert text.startswith("# ABB Robots")
    loaded = _yaml.safe_load(text)
    assert loaded["robots"]["irb_120_3_0_6"]["urdf_path"] == (
        "urdf/irb_120_3_0_6.urdf.xacro"
    )
    assert loaded["categories"]["abb"]["manufacturer"] == "ABB"


def test_generate_check_mode_validates_all(capsys):
    rc = g.generate(check=True)
    out = capsys.readouterr().out
    assert rc == 0
    assert "total:" in out
    total = int(out.strip().splitlines()[-1].split("total:")[1])
    assert 450 <= total <= 520


def test_generate_ids_globally_unique():
    seen = set()
    for vendor in g.VENDORS:
        for t in g.discover_types(vendor):
            assert t not in seen, f"duplicate id {t}"
            seen.add(t)


def test_generate_write_mode_creates_files(tmp_path, monkeypatch):
    import yaml as _yaml
    monkeypatch.setattr(g, "ARMS_OUT", tmp_path)
    rc = g.generate(check=False)
    assert rc == 0
    for vendor in g.VENDORS:
        out = tmp_path / f"{vendor}.yml"
        assert out.is_file()
    # spot-check round-trip of a real entry
    abb = _yaml.safe_load((tmp_path / "abb.yml").read_text())
    assert abb["robots"]["irb_120_3_0_6"]["urdf_path"] == (
        "urdf/irb_120_3_0_6.urdf.xacro"
    )


def test_scrape_dof_regex_fallback(tmp_path, monkeypatch):
    # YAML with no joint_limits mapping, but joint_N tokens in the body ->
    # exercises the regex fallback branch.
    vdir = tmp_path / "pkg"
    (vdir / "config" / "fake").mkdir(parents=True)
    (vdir / "config" / "fake" / "joint_limits.yaml").write_text(
        "some_other_key:\n  joint_1: 1\n  joint_2: 2\n  joint_3: 3\n"
    )
    monkeypatch.setitem(g.VENDORS, "fakevendor", {"package": "pkg"})
    monkeypatch.setattr(g, "DEPS", tmp_path)
    assert g.scrape_dof("fakevendor", "fake") == 3
