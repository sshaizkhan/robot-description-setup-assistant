# Web RViz Panel — Design

**Date:** 2026-06-07
**Status:** Approved (brainstorming) — pending implementation plan
**Branch context:** `feat/cpp-backend-bridge`

## Goal

Add an in-page, RViz-style 3D visualization to the existing Robot Description
Setup Assistant (RDSA) web app. The user switches to an "RViz" panel inside the
current React page and sees a live ROS visualization (robot model posed by live
TF/JointStates, TF frames, a ground grid, and visualization markers) rendered in
the browser.

## Decisions and Constraints

These were settled during brainstorming and bound the design:

1. **In-page web-native viz, not the literal RViz binary.** RViz2 is a compiled
   Qt/C++ desktop application; a browser tab cannot execute it. Embedding it
   would require running RViz2 as a separate process and streaming its pixels
   (e.g. KasmVNC/noVNC in a container). The user explicitly rejected running any
   extra container or process. Therefore we build an **RViz-equivalent** web
   viewer that runs inside the existing page.
2. **No new process.** The browser cannot speak DDS, so live ROS data needs a
   WebSocket bridge. We fold that bridge into the **existing FastAPI backend**,
   which already runs an in-process `rclpy` node (`JointStateRelay`) and exposes
   `ws /ws/joint_states`. No separate `rosbridge_server`, no container.
3. **Live ROS graph as the data source.** The backend's in-process node shares
   the host ROS graph (same `ROS_DOMAIN_ID` / `RMW_IMPLEMENTATION`). The panel
   shows whatever is live (`robot_state_publisher`, TF, marker publishers).
4. **v1 displays:** RobotModel + TF + Grid + Markers. LaserScan, PointCloud2,
   and Image are explicitly **deferred** (they need binary WS frames and
   decimation; heavy on the target hardware).
5. **Hardware constraint.** Target is an old/low-RAM machine (see `CLAUDE.md`).
   WS push rates are capped and dense sensor data is out of scope for v1.
6. **Not 100% RViz parity.** No support for arbitrary RViz plugins. This is the
   accepted tradeoff for staying in-page with no extra process.
7. **No regression.** The existing `Viewer3D` (three.js) and all current
   backend routes remain untouched. The RViz panel is additive.

## Architecture

```
React page
 |-- [3D]   existing Viewer3D            (default, untouched)
 \-- [RViz] RvizPanel  -- reuses three.js scene
        |  WS /ws/tf            <- TfRelay      (rclpy: /tf, /tf_static)
        |  WS /ws/markers       <- MarkerRelay  (rclpy: marker topic)
        |  WS /ws/joint_states  <- existing JointStateRelay
        v
  FastAPI backend (the single process already run)
        v  rclpy in-process node, same ROS graph
  live ROS: robot_state_publisher, TF, marker publishers
```

The panel renders RViz-style displays in the browser using three.js. The backend
relays live ROS topics to the browser over WebSockets as rate-capped JSON
snapshots.

## Components

### Backend — `web/backend/rdsa_web/ros_bridge.py` (extend) + `app.py` (wire)

The existing `JointStateRelay` is the template: lazy `rclpy` import, a node on a
threaded `SingleThreadedExecutor`, double-checked-locking `start()`, a
thread-safe `latest()` snapshot, and `stop()`. New relays mirror this exactly.

- **`TfRelay`**
  - Subscribes `/tf` (`tf2_msgs/TFMessage`) and `/tf_static`.
  - Maintains a map keyed by `(parent_frame, child_frame)` -> latest transform
    (translation + rotation quaternion + stamp). Static transforms are stored
    once and persist.
  - `latest()` returns a JSON-serializable snapshot of all known transforms.
  - Pure helper `tf_message_to_transforms(msg) -> list[dict]` is unit-testable
    without ROS (mirrors `joint_state_to_dict`).

- **`MarkerRelay`**
  - Subscribes a configurable topic (default `/visualization_marker_array`,
    `visualization_msgs/MarkerArray`) and `/visualization_marker`
    (`visualization_msgs/Marker`).
  - Maintains an `(ns, id) -> marker` map. Honors marker `action`:
    `ADD/MODIFY` (upsert), `DELETE` (remove one), `DELETEALL` (clear).
  - Honors `lifetime` by dropping expired markers from the snapshot.
  - Pure helper `marker_to_dict(msg) -> dict` is unit-testable without ROS.

- **WebSocket endpoints in `app.py`**
  - `GET ws /ws/tf` — pushes the `TfRelay` snapshot.
  - `GET ws /ws/markers` — pushes the `MarkerRelay` snapshot.
  - Both reuse the existing `/ws/joint_states` send-loop shape (snapshot ->
    send JSON -> sleep), with a shared **rate cap** (default 15 Hz) to protect
    the low-RAM box. Disconnect handled via `WebSocketDisconnect`.
  - Relays lazy-init `rclpy` on first connection; static mode (no ROS) is
    unaffected, exactly like `JointStateRelay`.

### Frontend — `web/frontend/src/RvizPanel.tsx` (+ small render helpers)

- A new panel/tab beside the 3D viewer. A **"RViz"** control in the header
  switches the main view to the panel (and back).
- Reuses the existing three.js scene setup and URDF mesh-resolve path
  (`meshUrl.ts`, the `Viewer3D` loading approach) — no duplicate mesh logic.
- **RobotModel display:** loads the live `/robot_description` URDF, poses joints
  from the existing `/ws/joint_states` feed, and applies the robot root /
  fixed-frame transform from `/ws/tf`.
- **TF display:** renders frame axes (toggle show/hide) and a **fixed-frame
  selector** that sets the scene's reference frame.
- **Grid display:** three.js `GridHelper`.
- **Markers display:** maps supported marker types to three.js objects —
  arrow, cube, sphere, line strip/list, points, text (sprite). Updates from
  `/ws/markers`. Unsupported types are ignored (logged, not fatal).
- **Connection status indicator** per stream and a graceful "no ROS graph"
  empty state.

A small pure module (e.g. `rvizMarkers.ts`) converts a marker dict to a
three.js object so it can be unit-tested in isolation.

## Data Flow

1. Browser opens the RViz panel -> opens WS connections to `/ws/tf`,
   `/ws/markers`, `/ws/joint_states`.
2. On first connection, each relay lazily starts its `rclpy` subscription on the
   shared ROS graph.
3. Each relay keeps the latest snapshot; the WS loop pushes JSON at <= the rate
   cap.
4. The panel applies snapshots to the three.js scene: joint values pose the
   URDF, TF positions the fixed frame and axes, markers are upserted/removed.
5. Closing the panel closes the WS connections. Relays may stay warm (the
   `JointStateRelay` precedent) — lifecycle detail deferred to the plan.

## Error Handling

- **No ROS environment / static mode:** relays do not start; WS endpoints send a
  status frame indicating "ROS unavailable"; panel shows the empty state. No
  crash, matching the existing lazy-rclpy convention.
- **WS disconnect:** handled with `WebSocketDisconnect`; relays unaffected.
- **Malformed/unsupported markers:** skipped and logged; never crash the loop.
- **Backpressure:** fixed rate cap; only latest snapshot is sent (no queue
  growth).

## Testing Strategy

- **Backend (pytest, matches existing suite):**
  - `tf_message_to_transforms` and `marker_to_dict` pure-transform tests (no ROS).
  - `MarkerRelay` action semantics: ADD/MODIFY/DELETE/DELETEALL, lifetime expiry.
  - WS endpoint smoke tests with a mocked relay snapshot.
- **Frontend (vitest, matches existing suite):**
  - `RvizPanel` view states: connected, empty (no graph), error.
  - `rvizMarkers` mapping units: each supported marker type -> expected three.js
    object kind.
- **Manual:**
  - Playwright smoke: panel mounts, header switch works, no console errors.
  - Live check against a demo TF + marker publisher (e.g. a small ROS demo) to
    confirm robot posing and marker rendering.
- **CI:** no container or live-ROS dependency; all automated tests run without a
  ROS graph.

## Out of Scope (v1)

- LaserScan, PointCloud2, Image displays (binary WS frames + decimation).
- Arbitrary RViz plugin parity.
- Interactive markers (6-DOF drag / goal-pose interaction).
- Pixel-streamed real RViz (KasmVNC/WebRTC) — viable later if the project moves
  to a GPU render host; the in-page panel and its header control would remain.

## Build Note

Per `CLAUDE.md`, any `colcon build` for backend ROS message deps must use
`--parallel-workers 1` and prefer `--packages-select`. The web changes
themselves (FastAPI + React) do not require colcon.
