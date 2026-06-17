# robot-description-setup-assistant

Web UI to browse a catalog of robotic arms (and end-effectors), preview them in
3D, and generate a custom robot description (URDF) package.

The catalog and all heavy work (YAML parsing, xacro→URDF expansion, `package://`
mesh resolution, package validation) run in **C++** ROS 2 nodes; a thin Python
**FastAPI** server relays them over HTTP/WebSocket to a **React + three.js**
single-page UI. See [`ARCHITECTURE.md`](ARCHITECTURE.md).

> The original Qt/RViz/MoveIt desktop setup assistant has been removed in favour
> of this web UI.

## Run

```bash
./run.sh            # http://127.0.0.1:8000  (use ./run.sh 0.0.0.0 8000 for LAN)
```

It sources ROS + the workspace, builds the frontend, starts the C++ catalog node,
and serves the API + SPA from one uvicorn process. See [`deps/app-robot-description-setup-assistant/QUICKSTART.md`](deps/app-robot-description-setup-assistant/QUICKSTART.md).

## Build (low-RAM machine)

```bash
colcon build --merge-install --parallel-workers 1 \
  --packages-select robot_catalog_msgs robot_catalog_core robot_catalog_server \
                    robot_description_setup_assistant
```

## Packages

| Package | Role |
|---|---|
| `robot_catalog_core` | Catalog library: YAML/dir load, filter, URDF (xacro), mesh read, JSON |
| `robot_catalog_msgs` | `.srv` definitions for the catalog services |
| `robot_catalog_server` | rclcpp node exposing the catalog over services |
| `robot_description_setup_assistant` | Data-only: robot catalog (`config/`), images (`resources/`), web launch |
| `deps/app-robot-description-setup-assistant/` | FastAPI backend (rclpy bridge) + React/three.js frontend |
| `deps/` | Vendored robot descriptions (UR, KUKA) |
