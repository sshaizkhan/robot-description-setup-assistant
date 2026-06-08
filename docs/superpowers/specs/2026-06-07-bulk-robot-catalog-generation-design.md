# Bulk Robot Catalog Generation — Design

Date: 2026-06-07
Branch: feat/cpp-backend-bridge

## Problem

The robot-description submodules were expanded to expose the full vendor robot
families, but the RDSA catalog still lists only a curated handful. Current
catalog (`config/catalog/arms/`) contains 11 KUKA entries and a few Universal
Robots entries. The newly available standalone descriptions are:

| Vendor  | ROS package                  | Standalone `urdf/*.urdf.xacro` | DOF source            |
|---------|------------------------------|--------------------------------|-----------------------|
| KUKA    | `kuka_robot_descriptions`    | 170                            | `config/<t>/joint_limits.yaml` |
| ABB     | `abb_robot_descriptions`     | 99                             | same                  |
| FANUC   | `fanuc_robot_descriptions`   | 111                            | same                  |
| Yaskawa | `yaskawa_robot_descriptions` | 100                            | same                  |

All four packages are already colcon-built into the merged install at
`/home/bot/rds_ws/install/share/<pkg>/`, with installed `urdf/` counts matching
source. So once catalog entries exist, the C++ catalog server resolves and
renders them with no rebuild of the description packages.

Goal: add **all ~480** of these robots to the catalog so they appear in the
browse grid and the assembly builder's arm picker, via a **reusable generator**
that can be re-run after future submodule additions.

## Key facts (verified)

- **Loader** (`robot_catalog_core/src/catalog.cpp`): recursively merges every
  `*.yml`/`*.yaml` under the catalog dir. All `specifications` fields are
  optional (default 0/false). A robot entry needs only: map key (`id`),
  `urdf_package`, `urdf_path`, `xacro_args`, `category`, `type`, and is rendered
  via `attach.tool_frame`. `categories:` blocks may live in any file and are
  merged.
- **Standalone xacro shape** (abb/fanuc/yaskawa, and the 170 kuka per-robot
  files): self-contained, take `prefix` (default `""`), include their own
  `_macro.xacro`, attach to a `world` link. So `xacro_args: ""` is sufficient —
  no `*_type:=` argument needed (unlike the legacy kuka dispatch).
- **Flange link**: every vendor macro exposes `${prefix}tool0`. With the default
  empty prefix → `tool_frame: "tool0"`, identical to existing entries. End
  effectors attach in the assembly builder unchanged.
- **DOF**: count distinct `joint_N` keys under `joint_limits:` in
  `config/<type>/joint_limits.yaml` (6 for irb_120, 4 for the m_410 palletizer).
- **Catalog load path**: the server reads the **installed** copy at
  `install/share/robot_description_setup_assistant/config/catalog/`. Editing
  source ymls requires a rebuild of `robot_description_setup_assistant` to take
  effect.
- **Images**: served by `/api/robots/{id}/image`, which resolves `image_path`
  inside the `robot_description_setup_assistant` package; empty path → 404. No
  PNGs exist for abb/fanuc/yaskawa.

## Decisions (agreed)

1. **Coverage**: all ~480 robots, via a reusable generator script.
2. **KUKA reconciliation — Approach B (full regen)**: regenerate all 170 kuka
   entries from the standalone xacros, replacing the legacy 11 dispatch-based
   curated entries. Uniform with the other vendors, no near-duplicate models.
   Loses curated specs/images on those 11 (acceptable — the other ~470 have
   empty specs anyway).
3. **Images**: all generated entries reference one shared placeholder,
   `resources/graphics/placeholder.png`.
4. **Scope**: generator handles the four standalone-xacro vendors
   (kuka/abb/fanuc/yaskawa). Universal Robots stays as its existing curated,
   dispatch-based `universal_robots.yml` (different `ur_type:=` pattern); it can
   be folded in later if desired. (User was unsure about UR; left untouched.)

## Architecture

### Generator script — `scripts/generate_catalog.py`

A standalone Python 3 script (stdlib + PyYAML, already a backend dep). Pure
function of the submodule contents; safe to re-run (idempotent overwrite).

For each vendor:

1. List `deps/<vendor>_robot_descriptions/urdf/*.urdf.xacro`, **excluding**
   `*_macro.xacro` and the vendor dispatch file `<vendor>.urdf.xacro`
   (e.g. `kuka.urdf.xacro`).
2. For each `<type>`:
   - `id = <type>` (xacro basename, e.g. `irb_120_3_0_6`)
   - `display_name = "<VENDOR> " + titleized(type)` (underscores → spaces,
     uppercased tokens; e.g. `"ABB Irb 120 3 0 6"`)
   - `description = "<Vendor> industrial robot"` (generic)
   - `image_path = "resources/graphics/placeholder.png"`
   - `urdf_package = "<vendor>_robot_descriptions"`
   - `urdf_path = "urdf/<type>.urdf.xacro"`
   - `xacro_args = ""`
   - `category = "<vendor>"`
   - `type = "arm"`
   - `attach.tool_frame = "tool0"`
   - `specifications.degrees_of_freedom` = scraped joint count
     (`config/<type>/joint_limits.yaml`; 0 if file/keys absent). Other spec
     fields omitted (loader defaults them).
   - `required_packages = ["<vendor>_robot_descriptions"]`
   - `tags = ["industrial"]`
3. Emit `config/catalog/arms/<vendor>.yml` containing a `categories:` block for
   that vendor plus the `robots:` map. **Overwrites** the existing file.

**Cross-vendor id collisions**: ids are kept bare (matching existing `ur3`,
`kr6_r700_sixx` style). The generator asserts global uniqueness across all four
vendors and **fails loudly** if a collision is found (none expected). The map
keys are also sorted for stable, diff-friendly output.

### `categories.yml`

Each generated vendor file owns its own `categories:` entry (kuka/abb/fanuc/
yaskawa). The `kuka` entry is therefore removed from the shared `categories.yml`
to avoid a stale duplicate; `categories.yml` keeps the shared/non-generated
categories (universal_robots, franka, grippers, bases).

Category metadata:

| id      | display_name        | manufacturer  | website                          |
|---------|---------------------|---------------|----------------------------------|
| kuka    | KUKA Robots         | KUKA Robotics | https://www.kuka.com             |
| abb     | ABB Robots          | ABB           | https://new.abb.com/products/robotics |
| fanuc   | FANUC Robots        | FANUC         | https://www.fanuc.com            |
| yaskawa | Yaskawa Motoman     | Yaskawa       | https://www.motoman.com          |

### Placeholder asset

Add `robot_description_setup_assistant/resources/graphics/placeholder.png` — a
neutral robot-arm silhouette card image. Installed with the package so
`/api/robots/{id}/image` resolves it for every generated robot.

## Data flow

```
deps/<vendor>_.../urdf/*.urdf.xacro  ──┐
deps/<vendor>_.../config/<t>/joint_limits.yaml ──> generate_catalog.py
                                          │
                                          ▼
        config/catalog/arms/{kuka,abb,fanuc,yaskawa}.yml  (+ categories.yml edit)
                                          │  colcon build robot_description_setup_assistant
                                          ▼
   install/share/.../config/catalog/  ──> C++ catalog server ──> FastAPI ──> React grid + assembly arm picker
```

## Build / deploy steps (post-generation)

Per CLAUDE.md (low-RAM machine, one package at a time):

```bash
python3 scripts/generate_catalog.py
colcon build --parallel-workers 1 --packages-select robot_description_setup_assistant
```

Then restart the catalog server / app to load ~480 robots.

## Testing

- **Generator unit check**: a small pytest (or `--check` mode in the script)
  asserting (a) no id collisions, (b) every generated entry's `urdf_path` exists
  on disk, (c) total count ≈ 480.
- **Loader smoke**: after build, `GET /api/robots?limit=1` returns
  `X-Total-Count` ≈ 480; spot-check one abb/fanuc/yaskawa id resolves a URDF via
  the existing `/api/robots/{id}/urdf` path.
- Existing `robot_catalog_core` gtests remain green (no C++ change).

## Out of scope / deferred

- Real per-robot specs (payload/reach/weight) and per-robot images — not
  available in the submodules; left as placeholders.
- Universal Robots bulk expansion (dispatch-based; different pattern).
- Curated KUKA specs/images (dropped per Approach B; remappable later).

## Risks

- **Stale install**: forgetting the rebuild → server keeps old catalog. Mitigated
  by documenting the build step and the smoke test.
- **A few config dirs may lack `joint_limits.yaml`** → DOF 0 for those entries.
  Acceptable; does not break rendering.
- **UI scale**: ~480 cards. Frontend grid is already virtualized + paginated for
  500+, so no change required.
