# Architecture — Robot Description Setup Assistant

How the whole workspace fits together: the two front-ends, who parses URDF, who
publishes/consumes `joint_states`, and how the C++, Python, and TypeScript code
share one robot-catalog domain.

> Web-stack-only detail lives in [`web/ARCHITECTURE.md`](web/ARCHITECTURE.md).
> This file is the workspace-wide map.

---

## 0. Two front-ends, one domain

The repo is a ROS 2 Humble workspace. There are **two complete UIs** over the
same robot catalog:

| | Legacy Qt desktop app | Web UI (current) |
|---|---|---|
| Entry | `robot_description_setup_assistant` (`main.cpp`) | `web/run.sh` → uvicorn |
| Render | RViz (`RVizPanel` + MoveIt `RobotStateDisplay`) | three.js + `urdf-loader` in browser |
| Catalog | `RobotConfigManager` singleton (in-process) | C++ `robot_catalog_server` ROS service |
| GUI | Qt widgets (pluginlib setup steps) | React + Vite SPA |
| MoveIt | **Required** (URDF/SRDF config, RViz plugin) | **Not used** |
| Status | Being retired | Active |

Both read the **same** `robot_description_setup_assistant/config/robots.yaml`
(20 robots: 9 Universal Robots + 11 KUKA; 3 categories; `franka` declared but
empty).

---

## 1. The catalog domain is mirrored 4×

`RobotConfig` / `RobotSpecifications` / `CategoryInfo` / `RobotFilter` are
defined **four times**, all semantically identical (same fields, same filter
rules):

| Definition | File | Notes |
|---|---|---|
| C++ legacy | `robot_description_core_plugins/.../robot_config_manager.hpp` | Qt, `std::filesystem::path` fields |
| C++ new | `robot_catalog_core/include/robot_catalog_core/types.hpp` | Qt-free, MoveIt-free |
| Python | `web/backend/rdsa_web/models.py` | Pydantic |
| TypeScript | `web/frontend/src/api.ts` | interfaces |

Filter semantics identical everywhere (`matchesFilter` / `_matches_filter`):
category equality · payload/reach min–max · DOF exact · collaborative-only ·
required-tags **all** must match · case-insensitive substring search over
name/description/category/tags. Empty filter ⇒ return all.

The C++ `robot_catalog_core` is a deliberate **de-Qt'd, MoveIt-free copy** of
the legacy `RobotConfigManager` logic — built so the web backend can own the
catalog without dragging in Qt/MoveIt.

---

## 2. Web UI — how it comes together

```
┌──────────────────────────────────────────────────────────────────────┐
│  Browser (React + three.js)            web/frontend/src/              │
│                                                                        │
│   api.ts ──HTTP──┐    jointSocket.ts ──WS──┐   Viewer3D.tsx           │
│   robots/cats/   │    (/ws/joint_states)   │   urdf-loader parses XML │
│   filter/urdf    │                         │   package://→/meshes/... │
└──────────────────┼─────────────────────────┼──────────────────────────┘
                    │ HTTP                     │ WebSocket
                    ▼                          ▼
┌──────────────────────────────────────────────────────────────────────┐
│  FastAPI / uvicorn (Python — thin relay)  web/backend/rdsa_web/app.py  │
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
  to `GET /api/robots/{id}/urdf`. (Running the `xacro` binary is the same thing
  MoveIt's `URDFConfig` does — orchestration is C++, not Python.)
- **Web (YAML fallback)**: `urdf.py :: resolve_urdf()` (Python xacro subprocess) —
  used **only** in no-ROS/test mode (`RDSA_CATALOG_BACKEND` unset).
- **Legacy Qt**: `RobotSelection::loadURDFFile()` → MoveIt `URDFConfig::loadFromPath()`.

### Stage B — URDF XML → 3D scene
- **Web**: `Viewer3D.tsx` uses `urdf-loader` (`loader.parse(xml)`) **in the
  browser** to build the three.js link/joint tree (`parseCollision=true`,
  rotated −90° X for Z-up→Y-up).
- **Legacy Qt**: MoveIt `RobotModel` + RViz `RobotStateDisplay` render it.

### Mesh resolution (web)
URDF refs are `package://PKG/rel`. `Viewer3D` rewrites them via
`meshUrl.resolvePackageUrl` → `/meshes/PKG/rel`. The backend resolves that path:
- **cpp mode**: `robot_catalog_core/src/mesh.cpp :: readMesh()` does the
  `ament_index_cpp` lookup + **path-traversal guard** (lexical `..` collapse,
  containment check; symlinks not resolved so merged/symlink installs work),
  **reads the file** (if within the **2 MiB `kMeshSizeCap`** — checked via
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

**Neither UI publishes `/joint_states` itself.** Two posing modes:

### Manual (default, no ROS)
- **Web**: sliders in `Viewer3D` call `robot.setJointValue()` directly on the
  three.js model. "Randomize" rejection-samples a self-collision-free pose
  (shrunk-AABB overlap test) — all client-side.
- **Legacy Qt**: RViz shows the static model.

### Live (mirror a running ROS graph)
- **Web**: `ros_bridge.py :: JointStateRelay` is an rclpy node
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

## 5. Legacy Qt path (retiring — still MoveIt-coupled)

```
main.cpp
  └─ SetupRobotDescriptionAssistantWidget        (setup_assistant pkg)
       ├─ pluginlib loads SetupStepWidget plugins (core_plugins pkg):
       │    ├─ StartScreenWidget    (new vs. existing package)
       │    └─ RobotSelectionWidget (catalog grid + filter + spec + RViz)
       │         └─ RobotConfigManager  (singleton, QFileSystemWatcher auto-reload)
       │         └─ FilterWidget / RobotSpecificationWidget
       └─ RVizPanel  (RViz VisualizationManager + MoveIt RobotStateDisplay)
```

Despite memory notes about "MoveIt removed," **this path is still hard-coupled to
MoveIt + RViz**: `rviz_panel.cpp` uses `moveit_rviz_plugin::RobotStateDisplay`
and `moveit_setup::URDFConfig/SRDFConfig`; `setup_step.hpp` and
`robot_selection.hpp` include `moveit_setup_framework`; the CMakeLists require
`moveit_setup_framework`, `moveit_ros_visualization`, `rviz_common`. The MoveIt
removal applied to the **new catalog/web path only**. `RobotSelectionWidget`
loads its catalog from the `robot_description_setup_assistant` package's
`robots.yaml`; the second copy under `robot_description_core_plugins/config/` is
stale and unused.

---

## 6. Package map

| Package | Role | Heavy deps |
|---|---|---|
| `robot_catalog_core` | Qt/MoveIt-free catalog lib (yaml-cpp + JSON writer) | yaml-cpp, ament_index_cpp |
| `robot_catalog_msgs` | `.srv` defs (GetRobots/GetCategories/FilterRobots/ValidateRobot) | rosidl |
| `robot_catalog_server` | rclcpp node advertising `catalog/*` services | rclcpp + the two above |
| `web/backend` (`rdsa_web`) | FastAPI app, rclpy bridge, urdf/mesh/package gen | fastapi, rclpy, xacro |
| `web/frontend` | React + three.js SPA | vite, urdf-loader |
| `robot_description_core_plugins` | Legacy Qt setup-step plugins + `RobotConfigManager` | Qt5, MoveIt, RViz |
| `robot_description_setup_framework` | Legacy Qt framework (`SetupStep`, `RVizPanel`, helpers) | Qt5, MoveIt, RViz |
| `robot_description_setup_assistant` | Legacy Qt main app + `robots.yaml` + launch | Qt5, RViz |
| `robot_description_common` | `robot_description_package()` cmake macro (warnings, C++17) | ament_cmake |
| `deps/ur5_description`, `deps/kuka_robot_descriptions` | Vendored robot description packages (xacro + meshes) | — |

**Build note:** low-RAM machine — always `colcon build --parallel-workers 1`
(max 2), prefer `--packages-select` one at a time. The new catalog packages are
light; the legacy Qt packages (Qt5 + MoveIt + RViz) are the expensive ones.

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
