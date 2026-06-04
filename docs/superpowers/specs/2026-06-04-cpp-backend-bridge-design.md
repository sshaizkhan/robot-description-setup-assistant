# Design: C++ catalog logic as the backend, bridged to the web UI

Date: 2026-06-04
Status: Approved design, pending spec review
Branch: `feat/cpp-backend-bridge` (off `master`; does NOT touch `feat/webui-rewrite`)

## Goal

Reuse the existing C++ catalog logic (`RobotConfigManager`) as the source of
truth for the web UI, exposed the idiomatic ROS 2 C++ way (a `rclcpp` service
node), with FastAPI/uvicorn as a thin forwarder and the React frontend
unchanged. The catalog is the first node; the same wiring is how future heavy
C++ (MoveIt, planners, controllers, live hardware) plugs into the web UI later.

## Why (motivation)

- Don't throw away the C++ that was written.
- ROS 2's C++ support is strong; keep growth on the C++/ROS side.

## Chosen approach (A): C++ ROS 2 service node + rclpy bridge

A C++ `rclcpp` node wraps the catalog logic and exposes ROS services. The
Python FastAPI layer calls those services with `rclpy` and forwards the result
over the existing HTTP API. Rejected alternatives:

- **B (pybind11, in-process):** simplest data flow but ignores the ROS C++
  ecosystem (a stated motivation) and recompiles C++ into the uvicorn process.
- **C (standalone C++ web/gRPC service):** least ROS-native; a second
  server + contract to maintain.

A wins because it reuses the C++, is the native ROS 2 pattern, isolates the C++
process from the web server, and generalizes to future C++ ROS nodes.

## Substrate (branch + bringing the web stack over)

`master` has the C++ but not the `web/` stack; `feat/webui-rewrite` has the web
stack but deleted the C++. The hybrid needs both, so on `feat/cpp-backend-bridge`:

1. Start from `master` (C++ Qt packages intact).
2. Bring the `web/` tree (FastAPI backend + React frontend) over from
   `feat/webui-rewrite` (e.g. `git checkout feat/webui-rewrite -- web` plus the
   `webui.launch.py`), committed as the starting point.
3. Keep the C++ packages. Only `RobotConfigManager` is needed; the Qt widgets
   and `RVizPanel` can remain unbuilt (or be trimmed in a later pass) — they are
   out of scope here.

The React frontend and the FastAPI HTTP/WS contract are unchanged; only the
backend's *data source* for catalog/validation moves from Python YAML parsing
to the C++ service.

## Components

### 1. `RobotCatalogCore` — pure C++ library (de-Qt)

`RobotConfigManager` mixes valuable logic with Qt infrastructure
(`QObject`, `QFileSystemWatcher`, `Q_SIGNALS`). Extract a pure-C++ library with
no Qt dependency:

- Structs already plain `std::`/value types: `RobotConfig`, `RobotSpecifications`,
  `CategoryInfo`, `RobotFilter` (move into the core, drop any Qt usage).
- Methods (ported verbatim, Qt removed): `loadFromFile`, `getAllRobots`,
  `getRobotById`, `getCategories`, `getCategoryInfo`, `getRobotsByCategory`,
  `filterRobots`, `searchRobots`, `validateRobotPackages`, `getMissingPackages`.
- Dropped for now: `QFileSystemWatcher` auto-reload and `Q_SIGNALS`
  (`configurationsReloaded`, etc.). Auto-reload, if wanted later, becomes an
  inotify watch or an explicit reload service — not in scope.

This makes the logic compile and unit-test without Qt or ROS.

### 2. `robot_catalog_msgs` — service definitions

To avoid a wall of nested `.msg` types, service responses carry a **JSON
string** (the catalog is data-shaped and the web layer forwards JSON anyway):

```
# GetRobots.srv
---
string robots_json          # JSON array of RobotConfig

# GetCategories.srv
---
string categories_json      # JSON array of CategoryInfo

# FilterRobots.srv
string filter_json          # JSON RobotFilter
---
string robots_json          # JSON array of RobotConfig

# ValidateRobot.srv
string robot_id
---
bool ok
string[] missing_packages
```

(`SearchRobots` is covered by `FilterRobots` with a `search_text` field, matching
the existing Python/HTTP behavior.)

JSON (de)serialization in C++ uses a small header-only lib (e.g.
`nlohmann/json`) or hand-written writers; the exact lib is decided in the plan.

### 3. `robot_catalog_server` — C++ `rclcpp` node

- Constructs one `RobotCatalogCore`, loaded from
  `get_package_share_directory("robot_description_setup_assistant")/config/robots.yaml`
  (or a parameter override).
- Advertises the four services, each calling the core and serializing to JSON.
- Launched alongside the web backend (added to `webui.launch.py`).

### 4. FastAPI bridge (`rclpy` client)

A new module `web/backend/rdsa_web/catalog_client.py`:

- Initializes `rclpy` once and creates a node + service clients at app startup.
- `get_all_robots()`, `get_categories()`, `filter_robots(filter)`,
  `validate(robot_id)` call the corresponding service (with a timeout) and
  parse the returned JSON into the existing pydantic models.
- `catalog.py`/`validation.py` are replaced (or thinly wrap this client) so the
  endpoint handlers in `app.py` are largely unchanged.

The `/urdf`, `/meshes`, `/ws/joint_states`, `/image`, `/package` endpoints stay
Python for now (they already work; each could move to C++ later via the same
service pattern).

## Data flow

```
Browser (React)
   │ GET /api/robots, POST /api/robots/filter, GET /api/robots/{id}/validate
   ▼
FastAPI (uvicorn, Python)  ── rclpy service call ──►  robot_catalog_server (C++)
   │  parse JSON response into pydantic                 │ RobotCatalogCore
   ◄───────────────────────────────────────────────────┘ (load/filter/search/validate)
   │ JSON over HTTP (unchanged contract)
   ▼
Browser renders catalog / detail
```

## Error handling

- `rclpy` client created at startup; if the C++ node is not up, service calls
  time out. Endpoints return **HTTP 503** with a clear "catalog service
  unavailable" message instead of hanging.
- Per-call timeout (e.g. 5 s). Malformed JSON from the service → 502.
- Launch starts the C++ node before/with uvicorn; docs note both must run.

## Testing

- **C++ gtest** on `RobotCatalogCore`: port the existing parity cases — 20
  robots, 3 categories, `universal_robots` = 9, filter by category/payload/
  collaborative/tags, case-insensitive search, missing-package validation.
- **ROS service test** (launch_testing or a small rclpy test): bring up
  `robot_catalog_server`, call each service, assert JSON shapes/counts.
- **Python bridge test**: a fake service (rclpy test node, or monkeypatched
  client) so the FastAPI layer is testable without building C++ in CI.
- The existing frontend tests are unaffected (HTTP contract unchanged).

## Phasing

- **P1 — Substrate.** Branch off master; bring `web/` over; build green
  (Python backend still serving from YAML at this point). Commit.
- **P2 — `RobotCatalogCore`.** Extract pure-C++ core from `RobotConfigManager`
  (de-Qt) + gtest parity suite.
- **P3 — Service interface + node.** `robot_catalog_msgs` (.srv) +
  `robot_catalog_server` `rclcpp` node; service-level test.
- **P4 — Bridge.** `catalog_client.py` rclpy clients; rewire `app.py`
  catalog/filter/validate endpoints; 503/502 handling; bridge tests; launch
  starts the node.

## Open items

- JSON library choice for C++ (`nlohmann/json` vendored vs. hand-written) —
  decided in the P3 plan.
- Whether to also move `/urdf` resolution into C++ (MoveIt URDF loading existed
  in the old code) — deferred; Python `xacro` path stays for now.
- Auto-reload (the dropped `QFileSystemWatcher`) — deferred; explicit reload
  service if needed.

## Out of scope

- The Qt widgets and `RVizPanel` (UI; replaced by React/three.js already).
- Changing the React frontend or the HTTP/WS contract.
- Any change to `feat/webui-rewrite`.
