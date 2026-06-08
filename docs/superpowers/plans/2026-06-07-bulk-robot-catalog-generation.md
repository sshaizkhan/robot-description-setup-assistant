# Bulk Robot Catalog Generation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add all ~480 vendor robots (kuka/abb/fanuc/yaskawa) to the RDSA catalog via a reusable generator script, so they appear in the browse grid and assembly arm picker.

**Architecture:** A pure-Python generator (`scripts/generate_catalog.py`) enumerates each vendor's standalone `urdf/<type>.urdf.xacro` files (those with a matching `config/<type>/` dir), scrapes DOF from `joint_limits.yaml`, and emits one auto-generated `config/catalog/arms/<vendor>.yml` per vendor (each carrying its own `categories:` block). The C++ catalog loader already merges every `*.yml` recursively; no C++ change. After generation, rebuild `robot_description_setup_assistant` to install the catalog.

**Tech Stack:** Python 3 + PyYAML, pytest, colcon (ROS 2). Interpreter for all script/test commands: `web/backend/.venv/bin/python` (has PyYAML 6.0.3 + pytest 9.0.3).

---

## Conventions used by every task

- **Repo root:** `/home/bot/rds_ws/src/robot-description-setup-assistant` — run all commands from here.
- **Python:** define once per shell: `PY=web/backend/.venv/bin/python`
- **Pytest invocation:** `$PY -m pytest scripts/test_generate_catalog.py -v`
- The generator is pure w.r.t. the checked-out `deps/` submodules; tests assert against the **real** submodule data (already present), no fixtures needed.

## File Structure

- Create: `scripts/generate_catalog.py` — the generator (pure functions + `main`).
- Create: `scripts/test_generate_catalog.py` — pytest unit tests for the generator.
- Create: `scripts/make_placeholder.py` — one-shot stdlib PNG writer for the card placeholder.
- Create: `robot_description_setup_assistant/resources/graphics/placeholder.png` — shared card image (produced by the script above).
- Modify: `robot_description_setup_assistant/config/catalog/categories.yml` — remove the now-duplicated `kuka` category block.
- Generated (overwritten by the script, committed): `robot_description_setup_assistant/config/catalog/arms/{kuka,abb,fanuc,yaskawa}.yml`.
  - Note: `kuka.yml` already exists (11 curated entries) and is **replaced** wholesale (spec Approach B). `universal_robots.yml` is left untouched.

---

### Task 1: Generator module skeleton (constants + paths)

**Files:**
- Create: `scripts/generate_catalog.py`
- Test: `scripts/test_generate_catalog.py`

- [ ] **Step 1: Write the failing test**

Create `scripts/test_generate_catalog.py`:

```python
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import generate_catalog as g


def test_vendors_and_paths():
    assert set(g.VENDORS) == {"kuka", "abb", "fanuc", "yaskawa"}
    assert g.VENDORS["abb"]["package"] == "abb_robot_descriptions"
    # deps dir and a vendor dir resolve to real directories
    assert g.DEPS.is_dir()
    assert g.vendor_dir("abb").is_dir()
    assert (g.vendor_dir("abb") / "urdf").is_dir()
    # output dir for arm catalogs exists
    assert g.ARMS_OUT.is_dir()
```

- [ ] **Step 2: Run test to verify it fails**

Run: `PY=web/backend/.venv/bin/python; $PY -m pytest scripts/test_generate_catalog.py::test_vendors_and_paths -v`
Expected: FAIL — `ModuleNotFoundError: No module named 'generate_catalog'`.

- [ ] **Step 3: Write minimal implementation**

Create `scripts/generate_catalog.py`:

```python
#!/usr/bin/env python3
"""Generate the RDSA arm catalog YAML from vendor robot-description submodules.

Re-runnable. For each vendor, enumerates standalone urdf/<type>.urdf.xacro that
have a matching config/<type>/ dir, and writes
config/catalog/arms/<vendor>.yml (overwriting). Run from anywhere; paths are
resolved relative to this file.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

import yaml

REPO_ROOT = Path(__file__).resolve().parent.parent
DEPS = REPO_ROOT / "deps"
ARMS_OUT = (
    REPO_ROOT / "robot_description_setup_assistant" / "config" / "catalog" / "arms"
)
PLACEHOLDER_IMAGE = "resources/graphics/placeholder.png"

# vendor key -> metadata. "label" is the human prefix used in display names.
VENDORS = {
    "kuka": {
        "package": "kuka_robot_descriptions",
        "display_name": "KUKA Robots",
        "manufacturer": "KUKA Robotics",
        "website": "https://www.kuka.com",
        "label": "KUKA",
    },
    "abb": {
        "package": "abb_robot_descriptions",
        "display_name": "ABB Robots",
        "manufacturer": "ABB",
        "website": "https://new.abb.com/products/robotics",
        "label": "ABB",
    },
    "fanuc": {
        "package": "fanuc_robot_descriptions",
        "display_name": "FANUC Robots",
        "manufacturer": "FANUC",
        "website": "https://www.fanuc.com",
        "label": "FANUC",
    },
    "yaskawa": {
        "package": "yaskawa_robot_descriptions",
        "display_name": "Yaskawa Motoman",
        "manufacturer": "Yaskawa",
        "website": "https://www.motoman.com",
        "label": "Yaskawa",
    },
}


def vendor_dir(vendor: str) -> Path:
    return DEPS / VENDORS[vendor]["package"]
```

- [ ] **Step 4: Run test to verify it passes**

Run: `$PY -m pytest scripts/test_generate_catalog.py::test_vendors_and_paths -v`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add scripts/generate_catalog.py scripts/test_generate_catalog.py
git commit -m "feat(catalog): generator skeleton with vendor metadata and paths"
```

---

### Task 2: `discover_types` — enumerate renderable robot types

**Files:**
- Modify: `scripts/generate_catalog.py` (append function)
- Test: `scripts/test_generate_catalog.py` (append test)

- [ ] **Step 1: Write the failing test**

Append to `scripts/test_generate_catalog.py`:

```python
def test_discover_types_counts_and_excludes_dispatch():
    abb = g.discover_types("abb")
    assert len(abb) == 99
    assert "irb_120_3_0_6" in abb
    assert abb == sorted(abb)  # stable, sorted output

    # kuka dispatch file "kuka.urdf.xacro" has no config/kuka dir -> excluded
    kuka = g.discover_types("kuka")
    assert "kuka" not in kuka
    assert len(kuka) > 100  # the per-robot standalone families

    # every discovered type has a real standalone xacro on disk
    for t in g.discover_types("yaskawa"):
        assert (g.vendor_dir("yaskawa") / "urdf" / f"{t}.urdf.xacro").is_file()
        assert (g.vendor_dir("yaskawa") / "config" / t).is_dir()
```

- [ ] **Step 2: Run test to verify it fails**

Run: `$PY -m pytest scripts/test_generate_catalog.py::test_discover_types_counts_and_excludes_dispatch -v`
Expected: FAIL — `AttributeError: module 'generate_catalog' has no attribute 'discover_types'`.

- [ ] **Step 3: Write minimal implementation**

Append to `scripts/generate_catalog.py`:

```python
def discover_types(vendor: str) -> list[str]:
    """Robot types with a standalone urdf/<type>.urdf.xacro AND a config/<type>/ dir.

    The config-dir requirement naturally excludes a vendor dispatch file such as
    kuka.urdf.xacro (no config/kuka), keeping output to per-robot descriptions.
    """
    vdir = vendor_dir(vendor)
    urdf = vdir / "urdf"
    config = vdir / "config"
    types: list[str] = []
    suffix = ".urdf.xacro"
    for f in sorted(urdf.glob("*.urdf.xacro")):
        t = f.name[: -len(suffix)]
        if (config / t).is_dir():
            types.append(t)
    return types
```

- [ ] **Step 4: Run test to verify it passes**

Run: `$PY -m pytest scripts/test_generate_catalog.py::test_discover_types_counts_and_excludes_dispatch -v`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add scripts/generate_catalog.py scripts/test_generate_catalog.py
git commit -m "feat(catalog): discover_types enumerates renderable robot types"
```

---

### Task 3: `scrape_dof` — degrees of freedom from joint_limits.yaml

**Files:**
- Modify: `scripts/generate_catalog.py` (append function)
- Test: `scripts/test_generate_catalog.py` (append test)

- [ ] **Step 1: Write the failing test**

Append to `scripts/test_generate_catalog.py`:

```python
def test_scrape_dof():
    # irb_120 is a 6-axis arm
    assert g.scrape_dof("abb", "irb_120_3_0_6") == 6
    # m_410ic_185 is a 4-axis palletizer
    assert g.scrape_dof("fanuc", "m_410ic_185") == 4
    # missing type -> 0, never raises
    assert g.scrape_dof("abb", "does_not_exist") == 0
```

- [ ] **Step 2: Run test to verify it fails**

Run: `$PY -m pytest scripts/test_generate_catalog.py::test_scrape_dof -v`
Expected: FAIL — `AttributeError: ... 'scrape_dof'`.

- [ ] **Step 3: Write minimal implementation**

Append to `scripts/generate_catalog.py`:

```python
def scrape_dof(vendor: str, type_: str) -> int:
    """Count joints in config/<type>/joint_limits.yaml; 0 if unavailable."""
    jl = vendor_dir(vendor) / "config" / type_ / "joint_limits.yaml"
    if not jl.is_file():
        return 0
    text = jl.read_text()
    try:
        data = yaml.safe_load(text) or {}
    except yaml.YAMLError:
        data = {}
    limits = data.get("joint_limits") if isinstance(data, dict) else None
    if isinstance(limits, dict):
        return len(limits)
    # Fallback: count distinct joint_N tokens in the raw file.
    return len(set(re.findall(r"joint_\d+", text)))
```

- [ ] **Step 4: Run test to verify it passes**

Run: `$PY -m pytest scripts/test_generate_catalog.py::test_scrape_dof -v`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add scripts/generate_catalog.py scripts/test_generate_catalog.py
git commit -m "feat(catalog): scrape_dof reads joint count from joint_limits.yaml"
```

---

### Task 4: `titleize` + `build_entry` — one catalog entry

**Files:**
- Modify: `scripts/generate_catalog.py` (append functions)
- Test: `scripts/test_generate_catalog.py` (append test)

- [ ] **Step 1: Write the failing test**

Append to `scripts/test_generate_catalog.py`:

```python
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
```

- [ ] **Step 2: Run test to verify it fails**

Run: `$PY -m pytest scripts/test_generate_catalog.py -k "titleize or build_entry" -v`
Expected: FAIL — `AttributeError: ... 'titleize'`.

- [ ] **Step 3: Write minimal implementation**

Append to `scripts/generate_catalog.py`:

```python
def titleize(type_: str) -> str:
    """irb_120_3_0_6 -> 'Irb 120 3 0 6' (underscores to spaces, capitalized)."""
    return " ".join(part.capitalize() for part in type_.split("_"))


def build_entry(vendor: str, type_: str) -> dict:
    meta = VENDORS[vendor]
    return {
        "type": "arm",
        "display_name": f"{meta['label']} {titleize(type_)}",
        "description": f"{meta['label']} industrial robot",
        "image_path": PLACEHOLDER_IMAGE,
        "urdf_package": meta["package"],
        "urdf_path": f"urdf/{type_}.urdf.xacro",
        "xacro_args": "",
        "category": vendor,
        "attach": {"tool_frame": "tool0"},
        "specifications": {"degrees_of_freedom": scrape_dof(vendor, type_)},
        "required_packages": [meta["package"]],
        "tags": ["industrial"],
    }
```

- [ ] **Step 4: Run test to verify it passes**

Run: `$PY -m pytest scripts/test_generate_catalog.py -k "titleize or build_entry" -v`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add scripts/generate_catalog.py scripts/test_generate_catalog.py
git commit -m "feat(catalog): build_entry produces a single arm catalog entry"
```

---

### Task 5: `build_vendor_doc` + `write_doc` — one vendor file

**Files:**
- Modify: `scripts/generate_catalog.py` (append functions)
- Test: `scripts/test_generate_catalog.py` (append test)

- [ ] **Step 1: Write the failing test**

Append to `scripts/test_generate_catalog.py`:

```python
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
    assert text.startswith("# ABB Robots")  # header comment present
    loaded = _yaml.safe_load(text)
    assert loaded["robots"]["irb_120_3_0_6"]["urdf_path"] == (
        "urdf/irb_120_3_0_6.urdf.xacro"
    )
    assert loaded["categories"]["abb"]["manufacturer"] == "ABB"
```

- [ ] **Step 2: Run test to verify it fails**

Run: `$PY -m pytest scripts/test_generate_catalog.py -k "vendor_doc or write_doc" -v`
Expected: FAIL — `AttributeError: ... 'build_vendor_doc'`.

- [ ] **Step 3: Write minimal implementation**

Append to `scripts/generate_catalog.py`:

```python
def build_vendor_doc(vendor: str, types: list[str]) -> dict:
    meta = VENDORS[vendor]
    return {
        "categories": {
            vendor: {
                "display_name": meta["display_name"],
                "description": f"Industrial robots from {meta['manufacturer']}",
                "manufacturer": meta["manufacturer"],
                "website": meta["website"],
            }
        },
        "robots": {t: build_entry(vendor, t) for t in types},
    }


def write_doc(vendor: str, doc: dict) -> Path:
    out = ARMS_OUT / f"{vendor}.yml"
    header = (
        f"# {VENDORS[vendor]['display_name']} — AUTO-GENERATED by "
        f"scripts/generate_catalog.py.\n# Do not edit by hand; re-run the "
        f"generator instead.\n"
    )
    with out.open("w") as fh:
        fh.write(header)
        yaml.safe_dump(doc, fh, default_flow_style=False, sort_keys=False)
    return out
```

- [ ] **Step 4: Run test to verify it passes**

Run: `$PY -m pytest scripts/test_generate_catalog.py -k "vendor_doc or write_doc" -v`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add scripts/generate_catalog.py scripts/test_generate_catalog.py
git commit -m "feat(catalog): build_vendor_doc and write_doc emit a vendor YAML"
```

---

### Task 6: `generate` + `main` — orchestrate, dedupe, `--check`

**Files:**
- Modify: `scripts/generate_catalog.py` (append functions)
- Test: `scripts/test_generate_catalog.py` (append test)

- [ ] **Step 1: Write the failing test**

Append to `scripts/test_generate_catalog.py`:

```python
def test_generate_check_mode_validates_all(capsys):
    # --check must pass against the real submodules and write nothing new
    rc = g.generate(check=True)
    out = capsys.readouterr().out
    assert rc == 0
    assert "total:" in out
    # totals line reflects ~480 robots across the four vendors
    total = int(out.strip().splitlines()[-1].split("total:")[1])
    assert 450 <= total <= 520


def test_generate_ids_globally_unique():
    seen = set()
    for vendor in g.VENDORS:
        for t in g.discover_types(vendor):
            assert t not in seen, f"duplicate id {t}"
            seen.add(t)
```

- [ ] **Step 2: Run test to verify it fails**

Run: `$PY -m pytest scripts/test_generate_catalog.py -k "check_mode or globally_unique" -v`
Expected: FAIL — `AttributeError: ... 'generate'`.

- [ ] **Step 3: Write minimal implementation**

Append to `scripts/generate_catalog.py`:

```python
def generate(check: bool = False) -> int:
    """Build all vendor docs. Returns 0 on success, 1 on a validation error.

    When check=True, validate only (no files written).
    """
    all_ids: dict[str, str] = {}
    docs: dict[str, dict] = {}
    counts: list[tuple[str, int]] = []
    for vendor in VENDORS:
        types = discover_types(vendor)
        for t in types:
            prev = all_ids.get(t)
            if prev is not None:
                print(
                    f"ERROR: duplicate robot id '{t}' in '{vendor}' and '{prev}'",
                    file=sys.stderr,
                )
                return 1
            all_ids[t] = vendor
            urdf = vendor_dir(vendor) / "urdf" / f"{t}.urdf.xacro"
            if not urdf.is_file():
                print(f"ERROR: missing urdf file {urdf}", file=sys.stderr)
                return 1
        docs[vendor] = build_vendor_doc(vendor, types)
        counts.append((vendor, len(types)))

    if not check:
        ARMS_OUT.mkdir(parents=True, exist_ok=True)
        for vendor, doc in docs.items():
            write_doc(vendor, doc)

    for vendor, n in counts:
        print(f"{vendor}: {n}")
    print(f"total: {sum(n for _, n in counts)}")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--check",
        action="store_true",
        help="validate only; do not write catalog files",
    )
    args = ap.parse_args()
    return generate(check=args.check)


if __name__ == "__main__":
    raise SystemExit(main())
```

- [ ] **Step 4: Run test to verify it passes**

Run: `$PY -m pytest scripts/test_generate_catalog.py -v`
Expected: PASS (all tests).

- [ ] **Step 5: Commit**

```bash
git add scripts/generate_catalog.py scripts/test_generate_catalog.py
git commit -m "feat(catalog): generate orchestration with dedupe and --check"
```

---

### Task 7: Placeholder card image

**Files:**
- Create: `scripts/make_placeholder.py`
- Create: `robot_description_setup_assistant/resources/graphics/placeholder.png`

- [ ] **Step 1: Write the placeholder generator (stdlib only)**

Create `scripts/make_placeholder.py`:

```python
#!/usr/bin/env python3
"""Write a solid-color 400x300 PNG card placeholder using only the stdlib."""
import struct
import sys
import zlib
from pathlib import Path

WIDTH, HEIGHT = 400, 300
RGB = (42, 42, 42)  # #2a2a2a, matches the dark theme card background


def _chunk(tag: bytes, data: bytes) -> bytes:
    return (
        struct.pack(">I", len(data))
        + tag
        + data
        + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
    )


def main() -> int:
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("placeholder.png")
    row = b"\x00" + bytes(RGB) * WIDTH  # filter byte 0 + RGB pixels
    raw = row * HEIGHT
    png = (
        b"\x89PNG\r\n\x1a\n"
        + _chunk(b"IHDR", struct.pack(">IIBBBBB", WIDTH, HEIGHT, 8, 2, 0, 0, 0))
        + _chunk(b"IDAT", zlib.compress(raw, 9))
        + _chunk(b"IEND", b"")
    )
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(png)
    print(f"wrote {out} ({len(png)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
```

- [ ] **Step 2: Generate the PNG**

Run:
```bash
PY=web/backend/.venv/bin/python
$PY scripts/make_placeholder.py robot_description_setup_assistant/resources/graphics/placeholder.png
```
Expected: prints `wrote robot_description_setup_assistant/resources/graphics/placeholder.png (...)`.

- [ ] **Step 3: Verify it is a valid PNG**

Run: `file robot_description_setup_assistant/resources/graphics/placeholder.png`
Expected: `PNG image data, 400 x 300, 8-bit/color RGB, non-interlaced`.

- [ ] **Step 4: Commit**

```bash
git add scripts/make_placeholder.py robot_description_setup_assistant/resources/graphics/placeholder.png
git commit -m "feat(catalog): add shared placeholder card image"
```

---

### Task 8: Remove duplicated `kuka` category from shared categories.yml

**Files:**
- Modify: `robot_description_setup_assistant/config/catalog/categories.yml`

Each generated vendor file owns its `categories:` block (incl. `kuka`), so the shared file must not also define `kuka` (avoid a stale duplicate; loader merge order is non-authoritative for content).

- [ ] **Step 1: Delete the `kuka` block**

In `robot_description_setup_assistant/config/catalog/categories.yml`, remove exactly these lines:

```yaml
  kuka:
    display_name: "KUKA Robots"
    description: "Industrial robots for various applications"
    manufacturer: "KUKA Robotics"
    website: "https://www.kuka.com"
```

Leave the `universal_robots`, `franka`, `grippers`, and `bases` blocks intact.

- [ ] **Step 2: Verify YAML still parses and kuka is gone**

Run:
```bash
PY=web/backend/.venv/bin/python
$PY -c "import yaml; d=yaml.safe_load(open('robot_description_setup_assistant/config/catalog/categories.yml')); print(sorted(d['categories']))"
```
Expected: `['bases', 'franka', 'grippers', 'universal_robots']` (no `kuka`).

- [ ] **Step 3: Commit**

```bash
git add robot_description_setup_assistant/config/catalog/categories.yml
git commit -m "chore(catalog): drop kuka category from shared file (now generator-owned)"
```

---

### Task 9: Run generator, commit generated catalog

**Files:**
- Overwrite (committed): `robot_description_setup_assistant/config/catalog/arms/{kuka,abb,fanuc,yaskawa}.yml`

- [ ] **Step 1: Validate with --check first**

Run:
```bash
PY=web/backend/.venv/bin/python
$PY scripts/generate_catalog.py --check
```
Expected: per-vendor counts then `total: <~480>`, exit 0. If a duplicate-id or missing-urdf error prints, STOP and fix before writing.

- [ ] **Step 2: Generate the files**

Run: `$PY scripts/generate_catalog.py`
Expected: same counts; files written.

- [ ] **Step 3: Spot-check generated output**

Run:
```bash
PY=web/backend/.venv/bin/python
$PY -c "import yaml; d=yaml.safe_load(open('robot_description_setup_assistant/config/catalog/arms/abb.yml')); r=d['robots']['irb_120_3_0_6']; print(r['urdf_path'], r['attach']['tool_frame'], r['specifications']['degrees_of_freedom'])"
```
Expected: `urdf/irb_120_3_0_6.urdf.xacro tool0 6`.

- [ ] **Step 4: Confirm full test suite still green**

Run: `$PY -m pytest scripts/test_generate_catalog.py -v`
Expected: PASS.

- [ ] **Step 5: Commit generated catalog**

```bash
git add robot_description_setup_assistant/config/catalog/arms/kuka.yml \
        robot_description_setup_assistant/config/catalog/arms/abb.yml \
        robot_description_setup_assistant/config/catalog/arms/fanuc.yml \
        robot_description_setup_assistant/config/catalog/arms/yaskawa.yml
git commit -m "feat(catalog): generate ~480 arm entries for kuka/abb/fanuc/yaskawa"
```

---

### Task 10: Build + integration smoke test

**Files:** none (build + verification only).

Per CLAUDE.md: low-RAM machine — build one package, `--parallel-workers 1`.

- [ ] **Step 1: Rebuild the catalog-owning package**

Run:
```bash
cd /home/bot/rds_ws
colcon build --parallel-workers 1 --packages-select robot_description_setup_assistant
cd src/robot-description-setup-assistant
```
Expected: `Finished <<< robot_description_setup_assistant`, no errors.

- [ ] **Step 2: Verify installed catalog + placeholder**

Run:
```bash
ls /home/bot/rds_ws/install/share/robot_description_setup_assistant/config/catalog/arms/
ls /home/bot/rds_ws/install/share/robot_description_setup_assistant/resources/graphics/placeholder.png
```
Expected: `abb.yml fanuc.yml kuka.yml universal_robots.yml yaskawa.yml` and the placeholder path listed.

- [ ] **Step 3: Launch the app and check the catalog total**

Run (background the app, then probe):
```bash
cd /home/bot/rds_ws && source install/setup.bash
./src/robot-description-setup-assistant/run.sh > /tmp/rdsa.log 2>&1 &
sleep 25
curl -si "http://localhost:8000/api/robots?limit=1" | grep -i x-total-count
```
Expected: `x-total-count:` header showing ~480+ (the new arms plus existing UR/EE/base entries).

- [ ] **Step 4: Verify a new robot's URDF resolves end-to-end**

Run:
```bash
curl -s "http://localhost:8000/api/robots/irb_120_3_0_6/urdf" | head -c 200
curl -si "http://localhost:8000/api/robots/irb_120_3_0_6/image" | head -n 1
```
Expected: first prints URDF XML (a `<robot ...>` opening); second prints `HTTP/1.1 200 OK` (placeholder served).

- [ ] **Step 5: Stop the app**

Run: `pkill -f run.sh; pkill -f uvicorn; pkill -f robot_catalog_server`
Expected: app processes terminated.

- [ ] **Step 6: Final confirmation note**

No commit (build artifacts are git-ignored). Record the observed `x-total-count` and that `irb_120_3_0_6` rendered URDF + image, confirming the new vendors are live in the catalog and assembly arm picker.

---

## Self-Review

**Spec coverage:**
- Reusable generator → Tasks 1-6. ✓
- All ~480, kuka full-regen (B) → Task 9 overwrites kuka.yml + Task 6 total assertion. ✓
- Standalone xacro path, `xacro_args:""`, `tool0` → Task 4 `build_entry` + test. ✓
- DOF scraped, other specs empty → Task 3 + Task 4. ✓
- Placeholder image → Task 7; referenced in Task 4. ✓
- Per-vendor `categories:` blocks; kuka removed from shared file → Task 5 `build_vendor_doc` + Task 8. ✓
- Catalog loaded from install → rebuild → Task 10. ✓
- Smoke test (X-Total-Count ~480, URDF + image resolve) → Task 10. ✓
- UR untouched → no task modifies `universal_robots.yml`. ✓
- No deps edits needed (validated in spec) → no task touches `deps/`. ✓

**Placeholder scan:** No TBD/TODO; every code step shows full code; commands have expected output. ✓

**Type consistency:** Function names consistent across tasks — `vendor_dir`, `discover_types`, `scrape_dof`, `titleize`, `build_entry`, `build_vendor_doc`, `write_doc`, `generate`, `main`. `PLACEHOLDER_IMAGE`/`ARMS_OUT`/`DEPS`/`VENDORS` referenced consistently. Entry dict keys used in Task 4 test match `build_entry` output and downstream `write_doc` roundtrip (Task 5). ✓
