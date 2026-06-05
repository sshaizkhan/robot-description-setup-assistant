# RDSA Web UI — Architecture

How the pieces fit together: who parses URDF, who handles joint states, and how
the C++, Python, and TypeScript code come together to serve the web UI.

> **⚠️ Updated — C++ now owns all URDF/mesh/validation work.**
> This document predates the "C++ does the work, Python only relays" refactor.
> In production (`RDSA_CATALOG_BACKEND=cpp`):
> - xacro → URDF XML runs in **C++** (`robot_catalog_core/src/urdf.cpp`, service
>   `catalog/get_urdf`) — **not** Python `urdf.py`.
> - `package://` mesh resolution **and file reading** run in **C++**
>   (`mesh.cpp :: readMesh`, `catalog/resolve_mesh`) — the service returns raw
>   bytes + MIME type; Python wraps them in a `Response` and touches no filesystem.
> - validation routes through `catalog/validate_robot` (**six** services now, not four).
>
> The Python `urdf.py` / `meshes.py` / `validation.py` modules remain **only** as
> the YAML fallback used in no-ROS/test mode. See the repo-root
> [`ARCHITECTURE.md`](../ARCHITECTURE.md) §3 for the current canonical description;
> sections below describe the older Python-side flow (still accurate for fallback mode).

> **TL;DR responsibility map**
>
> | Concern | Owner | Language | Where |
> |---|---|---|---|
> | Robot catalog (list/filter/categories) | `robot_catalog_server` node + `RobotCatalogCore` | **C++** | `robot_catalog_core/`, `robot_catalog_server/` |
> | Catalog ↔ web bridge (ROS service client) | `CatalogClient` / `CppCatalog` | **Python (rclpy)** | `web/backend/rdsa_web/catalog_client.py` |
> | HTTP/WebSocket API | FastAPI app | **Python (uvicorn)** | `web/backend/rdsa_web/app.py` |
> | xacro → URDF XML expansion | `resolve_urdf` (shells out to `xacro` CLI) | **Python** | `web/backend/rdsa_web/urdf.py` |
> | URDF XML → 3D scene + joints | `urdf-loader` (three.js) | **TypeScript** | `web/frontend/src/Viewer3D.tsx` |
> | `package://` mesh file serving | `resolve_mesh_path` | **Python** | `web/backend/rdsa_web/meshes.py` |
> | `/joint_states` subscribe → WebSocket relay | `JointStateRelay` | **Python (rclpy)** | `web/backend/rdsa_web/ros_bridge.py` |
> | `/joint_states` **publishing** | NOT in this stack — external | — | generated bringup pkg / robot driver |
> | Generated bringup package (zip) | `build_package_zip` | **Python** | `web/backend/rdsa_web/package_gen.py` |

---

## Big picture

```
┌──────────────────────────────────────────────────────────────────────┐
│  Browser (React + three.js)            web/frontend/                   │
│                                                                        │
│   api.ts ───HTTP──┐     jointSocket.ts ───WS──┐   Viewer3D.tsx         │
│   (robots,        │     (/ws/joint_states)    │   • urdf-loader parses │
│    categories,    │                           │     URDF XML           │
│    filter, urdf)  │                           │   • rewrites package:// │
│                   │                           │     → /meshes/...       │
│                   │                           │   • sets joint values   │
└───────────────────┼───────────────────────────┼───────────────────────┘
                     │ HTTP                       │ WebSocket
                     ▼                            ▼
┌──────────────────────────────────────────────────────────────────────┐
│  FastAPI / uvicorn  (Python)           web/backend/rdsa_web/app.py     │
│                                                                        │
│  /api/robots, /api/categories, /api/robots/filter                     │
│        └── catalog (CppCatalog)  ──rclpy service call──┐               │
│  /api/robots/{id}/urdf  ── resolve_urdf() ── xacro CLI │               │
│  /meshes/{pkg}/{rel}    ── resolve_mesh_path()         │               │
│  /api/robots/{id}/package ── build_package_zip()       │               │
│  /ws/joint_states       ── JointStateRelay ──┐         │               │
└───────────────────────────────────────────────┼─────────┼─────────────┘
                                                 │ rclpy   │ rclpy service
                            subscribes /joint_states       │ (catalog/*)
                                                 │         ▼
                                                 │  ┌──────────────────────┐
                                                 │  │ robot_catalog_server  │
                                                 │  │      (C++ rclcpp)     │
                                                 │  │  RobotCatalogCore     │
                                                 │  │  parses robots.yaml   │
                                                 │  │  (yaml-cpp) → JSON    │
                                                 │  └──────────────────────┘
                                                 ▼
                              ROS graph topic /joint_states
                              (published by SOMETHING ELSE:
                               joint_state_publisher_gui, a
                               real robot driver, a bag, etc.)
```

---

## 1. Who parses URDF?

URDF handling happens in **two stages, two different places**:

### Stage A — xacro → URDF XML (server-side, Python)

`web/backend/rdsa_web/urdf.py :: resolve_urdf(robot)`:

1. Resolves the robot's `urdf_package` share dir via `ament_index_python`.
2. Runs the **`xacro` CLI** as a subprocess on `share/<urdf_path>` with the
   robot's `xacro_args` (30s timeout, no stdin).
3. Returns the expanded **URDF XML** as a string.

Served at `GET /api/robots/{id}/urdf` → `{ urdf_xml, mesh_base: "/meshes",
missing_packages, error }`.

So macro expansion is done by the standard ROS `xacro` tool, invoked by Python.
The backend does **not** hand-roll any XML parsing here — it just captures
`xacro` stdout.

### Stage B — URDF XML → 3D scene graph (client-side, TypeScript)

`web/frontend/src/Viewer3D.tsx` uses **`urdf-loader`** (a three.js loader):

- `loader.parse(urdfXml)` builds the link/joint tree as three.js objects.
- `loader.parseCollision = true` so collision geometry loads too.
- Movable joints (`jointType !== "fixed"`) become slider controls.
- The URDF is Z-up, so the robot is rotated `-90°` about X for the Y-up viewer.

The browser is what actually interprets links, joints, and visual/collision
meshes for rendering.

### Mesh resolution (the handoff between B and the backend)

URDF mesh references look like `package://ur_description/meshes/base.stl`.
`urdf-loader` can't fetch those directly, so:

1. `Viewer3D` sets `loader.packages = pkg => resolvePackageUrl(meshBase, ...)`
   (`web/frontend/src/meshUrl.ts`), turning `package://PKG/rel` into
   `/meshes/PKG/rel`.
2. The browser requests `GET /meshes/{pkg}/{rel}`.
3. Backend `web/backend/rdsa_web/meshes.py :: resolve_mesh_path(pkg, rel)`
   resolves the package share dir (ament_index) and returns the file — with a
   **path-traversal guard** (`..` collapsed lexically, containment checked;
   symlinks intentionally not resolved so merged/symlink colcon installs work).

---

## 2. Who publishes joint_states? (Important nuance)

**Nobody in this web stack publishes `/joint_states`.** The web app only
**consumes** them. There are two distinct posing modes:

### Manual mode (default, no ROS needed)

The joint sliders in `Viewer3D` call `robot.setJointValue(name, value)` directly
on the three.js model. Pure client-side — no ROS, no topics. "Randomize pose"
also runs entirely in the browser (rejection-samples poses, picks the one with
the fewest self-collisions using shrunk AABB overlap tests).

### Live mode (mirrors a running ROS graph)

1. `web/backend/rdsa_web/ros_bridge.py :: JointStateRelay` is an **rclpy node**
   (`rdsa_joint_relay`) that **subscribes** to `/joint_states` (`sensor_msgs/
   JointState`) and keeps the latest `{joint: position}` map. It spins in a
   background thread; creation is double-checked-locked so concurrent WS clients
   don't spawn duplicate nodes.
2. `app.py :: /ws/joint_states` WebSocket pushes `relay.latest()` at ~10 Hz.
3. Frontend `jointSocket.ts :: connectJointStates` opens the socket;
   `liveJoints.ts` parses each message; `App.tsx` stores it; `Viewer3D` applies
   each value with `robot.setJointValue` (sliders hidden in live mode).

**The publisher is external.** Something else on the ROS graph must publish
`/joint_states`, e.g.:

- the generated bringup package (see §4) running `joint_state_publisher_gui`,
- a real robot driver,
- a `ros2 bag` replay, etc.

If nothing publishes, the WebSocket stays connected and simply streams an empty
joint map — the UI doesn't error.

---

## 3. Who owns the catalog? (The C++ part)

The robot catalog (the list of 20 robots, 3 categories, and filtering) is the
**C++ source of truth** when `RDSA_CATALOG_BACKEND=cpp` (set by the launch file).

### C++ side

- `robot_catalog_core/` — a pure C++ library (`RobotCatalogCore`) with **no Qt
  and no ROS** beyond `ament_index_cpp`. It parses `robots.yaml` with
  **yaml-cpp**, and serializes results to JSON with a **hand-written writer**
  (`json.cpp` — deliberately no `nlohmann/json` dependency). Implements
  `getAllRobots / getRobotById / getCategories / filterRobots / searchRobots /
  getMissingPackages`.
- `robot_catalog_msgs/` — `.srv` definitions: `GetRobots`, `GetCategories`,
  `FilterRobots` (typed input fields + `has_*` flags so C++ never has to parse
  JSON input), `ValidateRobot`.
- `robot_catalog_server/` — `server_node.cpp`, an `rclcpp::Node` named
  `robot_catalog_server` that loads `robots.yaml` and advertises four services:
  `catalog/get_robots`, `catalog/get_categories`, `catalog/filter_robots`,
  `catalog/validate_robot`. Catalog responses are JSON strings.

### Python bridge

`web/backend/rdsa_web/catalog_client.py`:

- `CatalogClient` — a thin **rclpy** client (`rdsa_catalog_client` node) that
  calls the services concurrently (a background executor spins the node so many
  `call_async` futures are in flight at once), with a timeout, and caches
  resolved mesh bytes. Raises
  `CatalogServiceUnavailable` if the node is down.
- `CppCatalog` — an adapter exposing the same interface FastAPI expects
  (`get_all_robots`, `get_categories`, `filter_robots`, `get_robot_by_id`),
  backed by the client. JSON strings from C++ are parsed into the same Pydantic
  models the rest of the app uses.

### Wiring + fallback

`app.py :: _make_catalog()` picks `CppCatalog` when `RDSA_CATALOG_BACKEND=cpp`,
otherwise a pure-Python YAML catalog (used by tests/CI so they need no ROS).
The catalog endpoints (`/api/robots`, `/api/categories`, `/api/robots/filter`)
catch `CatalogServiceUnavailable` and return **HTTP 503** if the C++ node is
unreachable.

**Note on validation:** `GET /api/robots/{id}/validate` currently uses the
Python `validation.py :: get_missing_packages` (ament_index) on the HTTP path.
The C++ node also exposes a `validate_robot` service, but the HTTP validate
endpoint does not route through it today.

---

## 4. Generated bringup package (the "export" button)

`GET /api/robots/{id}/package` → `package_gen.py :: build_package_zip` produces a
downloadable ROS 2 ament_cmake package (`<id>_bringup`) containing
`package.xml`, `CMakeLists.txt`, `README.md`, and a `view.launch.py` that starts
`robot_state_publisher` (loading the robot's xacro) + `joint_state_publisher_gui`.

This is the artifact that, when built and launched, **becomes a `/joint_states`
publisher** — closing the loop with live mode in §2. All interpolated catalog
values are JSON-encoded into the launch file so catalog data can't inject code.

---

## 5. End-to-end request flows

**Load the grid:** browser `fetchRobots()` → `GET /api/robots` → FastAPI →
`CppCatalog.get_all_robots()` → rclpy `catalog/get_robots` → C++ node →
`robots_to_json(core_.getAllRobots())` → parsed to models → JSON to browser.

**Open the 3D viewer:** browser `fetchUrdf(id)` → `GET /api/robots/{id}/urdf` →
`resolve_urdf()` runs `xacro` → URDF XML returned → `Viewer3D` parses with
`urdf-loader` → mesh requests go to `/meshes/...` → `resolve_mesh_path` serves
files.

**Live posing:** external node publishes `/joint_states` → `JointStateRelay`
(rclpy subscribe) caches latest → `/ws/joint_states` streams at 10 Hz →
`jointSocket` → `Viewer3D.setJointValue` updates the model.

---

## 6. Process model at runtime

Two processes run together (started by `robot_description_setup_assistant/launch/
webui.launch.py`, or manually — see `web/QUICKSTART.md`):

1. **`robot_catalog_server`** (C++ rclcpp node) — the catalog source of truth.
2. **uvicorn** running FastAPI with `RDSA_CATALOG_BACKEND=cpp` — serves the HTTP
   API, the WebSocket relay, and the built frontend (`RDSA_FRONTEND_DIST`).

The FastAPI process additionally creates its own rclpy nodes inside itself:
`rdsa_catalog_client` (catalog calls) and `rdsa_joint_relay` (joint-state
subscription). So ROS lives on both sides of the bridge: a dedicated C++ node
for the catalog, and lightweight rclpy nodes embedded in the Python server.
