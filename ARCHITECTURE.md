# Architecture — Robot Description Setup Assistant

How the whole workspace fits together: who parses URDF, who publishes/consumes
`joint_states`, and how the C++, Python, and TypeScript code share one
robot-catalog domain.

> Web-stack-only detail lives in [`deps/app-robot-description-setup-assistant/ARCHITECTURE.md`](deps/app-robot-description-setup-assistant/ARCHITECTURE.md).
> This file is the workspace-wide map.
>
> **No Qt / RViz / MoveIt.** The original Qt desktop setup assistant (and its
> `robot_description_core_plugins` / `robot_description_setup_framework` /
> `robot_description_common` packages) was removed. The web UI is the only
> front-end; `robot_description_setup_assistant` is now a data-only package
> (catalog `config/`, robot `resources/`, web launch).

---

## 0. One front-end, one domain

The repo is a ROS 2 Humble workspace. The **web UI** is the only front-end:

| | Web UI |
|---|---|
| Entry | `./run.sh` → uvicorn |
| Render | three.js + `urdf-loader` in the browser |
| Catalog | C++ `robot_catalog_server` ROS service |
| GUI | React + Vite SPA |
| MoveIt / Qt / RViz | **Not used** |

Catalog data: `robot_description_setup_assistant/config/catalog/` (a directory of
`*.yml` merged by the loader) — 20 robots (9 Universal Robots + 11 KUKA), 3
categories (`franka` declared but empty). A legacy single-file `config/robots.yaml`
is kept only as a fallback.

---

## 1. The catalog domain is mirrored 3×

`RobotConfig` / `RobotSpecifications` / `CategoryInfo` / `RobotFilter` are
defined **three times**, all semantically identical (same fields, same filter
rules):

| Definition | File | Notes |
|---|---|---|
| C++ | `robot_catalog_core/include/robot_catalog_core/types.hpp` | source of truth |
| Python | `deps/app-robot-description-setup-assistant/backend/rdsa_deps/app-robot-description-setup-assistant/models.py` | Pydantic |
| TypeScript | `deps/app-robot-description-setup-assistant/frontend/src/api.ts` | interfaces |

Filter semantics identical everywhere (`matchesFilter` / `_matches_filter`):
type · category equality · payload/reach min–max · DOF exact · collaborative-only ·
required-tags **all** must match · case-insensitive substring search over
name/description/category/tags. Empty filter ⇒ return all.

---

## 2. Web UI — how it comes together

```
┌──────────────────────────────────────────────────────────────────────┐
│  Browser (React + three.js)            deps/app-robot-description-setup-assistant/frontend/src/              │
│                                                                        │
│   api.ts ──HTTP──┐    jointSocket.ts ──WS──┐   Viewer3D.tsx           │
│   robots/cats/   │    (/ws/joint_states)   │   urdf-loader parses XML │
│   filter/urdf    │                         │   package://→/meshes/... │
└──────────────────┼─────────────────────────┼──────────────────────────┘
                    │ HTTP                     │ WebSocket
                    ▼                          ▼
┌──────────────────────────────────────────────────────────────────────┐
│  FastAPI / uvicorn (Python — thin relay)  deps/app-robot-description-setup-assistant/backend/rdsa_deps/app-robot-description-setup-assistant/app.py  │
│                                                                        │
│  /api/robots /api/categories /api/robots/filter ─┐                    │
│  /api/robots/{id}/urdf      ── catalog.get_urdf() ┤ all rclpy          │
│  /meshes/{pkg}/{rel}        ── catalog.resolve_mesh() service          │
│  /api/robots/{id}/validate  ── catalog.validate()  ┘ calls            │
│  /api/robots/{id}/package   ── build_package_zip()  (templating, py)   │
│  /ws/joint_states           ── JointStateRelay (rclpy subscribe)      │
└────────────────────────────────────────────┼─────────────────────────┘
              rclpy: catalog/* services        │ rclpy: subscribe /joint_states
                                              ▼
                      ┌────────────────────────────────────┐
                      │ robot_catalog_server (C++)          │
                      │  RobotCatalogCore  yaml-cpp + JSON   │
                      │  get_urdf    → run xacro (fork/exec) │
                      │  resolve_mesh→ read file → bytes+mime │
                      │  validate    → getMissingPackages    │
                      └────────────────────────────────────┘
   (C++ reads the mesh file and returns bytes; Python touches no filesystem.)
```

Two OS processes (started by `webui.launch.py` or manually):
1. **`robot_catalog_server`** — C++ rclcpp node, catalog source of truth.
2. **uvicorn** (FastAPI, `RDSA_CATALOG_BACKEND=cpp`) — HTTP/WS + serves the SPA.

The FastAPI process embeds two rclpy nodes of its own: `rdsa_catalog_client`
(calls the C++ services) and `rdsa_joint_relay` (subscribes `/joint_states`).
So ROS runs on **both** sides of the bridge.

Fallback: unset `RDSA_CATALOG_BACKEND` ⇒ FastAPI uses the pure-Python
`RobotCatalog` (`catalog.py`, YAML) — no ROS needed (this is what tests/CI use).
If the C++ node is down in cpp mode, catalog endpoints return **HTTP 503**.

---

## 3. Who parses URDF?

**Core principle: C++ does all parsing/resolving; Python only relays.** In the
web stack's production (cpp) mode, Python never parses URDF, resolves
`package://`, or checks packages — every one of those is a C++ service call.

### Stage A — xacro → URDF XML (C++)
- **Web (cpp mode)**: `robot_catalog_core/src/urdf.cpp :: resolveUrdf()` resolves
  the package share dir (`ament_index_cpp`), builds the `xacro` argv, and runs
  the `xacro` process **from C++** (fork/exec, separate stdout/stderr pipes, 30s
  timeout). Exposed as the `catalog/get_urdf` service; `app.py` relays the result
  to `GET /api/robots/{id}/urdf` (and caches it).
- **YAML fallback**: `urdf.py :: resolve_urdf()` (Python xacro subprocess) —
  used **only** in no-ROS/test mode (`RDSA_CATALOG_BACKEND` unset).

### Stage B — URDF XML → 3D scene
- `Viewer3D.tsx` uses `urdf-loader` (`loader.parse(xml)`) **in the browser** to
  build the three.js link/joint tree (rotated −90° X for Z-up→Y-up). Visual
  meshes load first; collision geometry is loaded lazily only when the user
  toggles "Show collision". The robot is hidden until all meshes are in, then
  revealed at once (no link-by-link build).

### Mesh resolution (web)
URDF refs are `package://PKG/rel`. `Viewer3D` rewrites them via
`meshUrl.resolvePackageUrl` → `/meshes/PKG/rel`. The backend resolves that path:
- **cpp mode**: `robot_catalog_core/src/mesh.cpp :: readMesh()` does the
  `ament_index_cpp` lookup + **path-traversal guard** (lexical `..` collapse,
  containment check; symlinks not resolved so merged/symlink installs work),
  **reads the file** (if within the **5 MiB `kMeshSizeCap`** — checked via
  `file_size` *before* reading, so oversized files never load into memory), and
  returns the raw bytes + MIME type via the `catalog/resolve_mesh` service.
  Python (`app.py`) just wraps those bytes in a `Response` — **zero filesystem
  access**, no resolving, no reading. Over-cap files yield `too_large` →
  `MeshTooLarge` → **HTTP 413** with the offending size, so the user can shrink
  the asset.
- **YAML fallback**: `meshes.py :: resolve_mesh_path` (Python) — test/no-ROS only.

### Validation (missing packages)
`catalog/validate_robot` (C++ `getMissingPackages`, `ament_index_cpp`) backs
`GET /api/robots/{id}/validate` in cpp mode; `validation.py` is the YAML fallback.

---

## 4. Who publishes `joint_states`?

**The web app does not publish `/joint_states` itself.** Two posing modes:

### Manual (default, no ROS)
- Sliders in `Viewer3D` call `robot.setJointValue()` directly on the three.js
  model. "Randomize" rejection-samples a self-collision-free pose (shrunk-AABB
  overlap test) — all client-side.

### Live (mirror a running ROS graph)
- `ros_bridge.py :: JointStateRelay` is an rclpy node
  (`rdsa_joint_relay`) that **subscribes** `/joint_states`
  (`sensor_msgs/JointState`), caches the latest map (double-checked-locked
  thread), and `/ws/joint_states` streams it at ~10 Hz → `Viewer3D.setJointValue`.

**The publisher is external** — e.g. the **generated bringup package** (below),
a real robot driver, or `ros2 bag`. With no publisher, the socket streams an
empty map; the UI doesn't error.

### The generated bringup package closes the loop
`GET /api/robots/{id}/package` → `package_gen.py :: build_package_zip` emits an
ament_cmake `<id>_bringup` package whose `view.launch.py` starts
`robot_state_publisher` + `joint_state_publisher_gui`. Built and launched, that
**becomes** the `/joint_states` publisher live mode subscribes to. All catalog
values are JSON-encoded into the launch file (injection-safe).

---

## 5. Removed: the legacy Qt/RViz/MoveIt desktop app

The original desktop setup assistant — a Qt UI with an embedded RViz panel and
MoveIt-backed URDF/SRDF handling — has been **deleted**. Removed packages:
`robot_description_core_plugins` (Qt setup-step widgets + `RobotConfigManager`),
`robot_description_setup_framework` (Qt framework + `RVizPanel`), and
`robot_description_common` (its cmake macro). `robot_description_setup_assistant`
remains as a **data-only** package (no Qt/MoveIt): it ships the catalog
(`config/`), robot images (`resources/`), and the web launch file. The workspace
no longer depends on Qt5, RViz, or MoveIt.

---

## 6. Package map

| Package | Role | Deps |
|---|---|---|
| `robot_catalog_core` | Catalog lib: YAML/dir load, filter, URDF (xacro), mesh read, JSON writer | yaml-cpp, ament_index_cpp |
| `robot_catalog_msgs` | `.srv` defs (GetRobots/GetCategories/FilterRobots/ValidateRobot/GetUrdf/ResolveMesh) | rosidl |
| `robot_catalog_server` | rclcpp node advertising `catalog/*` services | rclcpp + the two above |
| `robot_description_setup_assistant` | **Data-only**: catalog `config/`, robot `resources/`, web launch | ament_cmake |
| `deps/app-robot-description-setup-assistant/backend` (`rdsa_web`) | FastAPI app + rclpy bridge (relay) + package gen | fastapi, rclpy |
| `deps/app-robot-description-setup-assistant/frontend` | React + three.js SPA | vite, urdf-loader, @tanstack/react-virtual |
| `deps/ur5_description`, `deps/kuka_robot_descriptions` | Vendored robot description packages (xacro + meshes) | — |

**Build note:** low-RAM machine — always `colcon build --merge-install
--parallel-workers 1` (max 2), prefer `--packages-select`. All remaining
packages are light (no Qt/MoveIt).

---

## 7. End-to-end flows (web)

- **Grid load**: `fetchRobots()` → `/api/robots` → `CppCatalog.get_all_robots()`
  → rclpy `catalog/get_robots` → C++ `robots_to_json(getAllRobots())` → parsed to
  Pydantic → JSON to browser.
- **Open viewer**: `fetchUrdf(id)` → `/api/robots/{id}/urdf` →
  `catalog.get_urdf()` → rclpy `catalog/get_urdf` → C++ runs `xacro` → XML
  relayed back → `Viewer3D` parses with urdf-loader → mesh GETs hit `/meshes/...`
  → `catalog/resolve_mesh` (C++) returns path → Python streams the file.
- **Live pose**: external publisher → `/joint_states` → `JointStateRelay` caches
  → `/ws/joint_states` 10 Hz → `Viewer3D.setJointValue`.
- **Export**: `/api/robots/{id}/package` → zip with `robot_state_publisher` +
  `joint_state_publisher_gui` launch.
