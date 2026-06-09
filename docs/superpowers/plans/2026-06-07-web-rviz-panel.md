# Web RViz Panel Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an in-page, RViz-style 3D panel to the RDSA web app that renders the live robot model, TF frames, a grid, and visualization markers from the live ROS graph — with no new process or container.

**Architecture:** Extend the existing FastAPI in-process `rclpy` bridge (`ros_bridge.py`) with two new relays (`TfRelay`, `MarkerRelay`) mirroring the existing `JointStateRelay`, expose them over two new WebSockets (`/ws/tf`, `/ws/markers`), and add a React `RvizPanel` that renders the data with three.js (reusing the existing URDF/mesh-resolve approach). A new `"rviz"` view in `App.tsx` switches to it.

**Tech Stack:** Python 3 / FastAPI / rclpy (backend); React 18 / TypeScript / three.js / urdf-loader (frontend); pytest + vitest.

Spec: `docs/superpowers/specs/2026-06-07-web-rviz-panel-design.md`

---

## File Structure

**Backend (`web/backend/`):**
- Modify `rdsa_web/ros_bridge.py` — add `tf_message_to_transforms`, `TfRelay`, `marker_to_dict`, `prune_expired_markers`, `MarkerRelay`.
- Modify `rdsa_web/app.py` — add `tf_relay`/`marker_relay` params to `create_app`, add `/ws/tf` and `/ws/markers` endpoints.
- Modify `tests/test_ros_bridge.py` — pure-helper + relay tests.
- Modify `tests/test_api.py` — WS endpoint smoke tests.

**Frontend (`web/frontend/src/`):**
- Create `tfSocket.ts` — TF WebSocket client (mirrors `jointSocket.ts`).
- Create `markerSocket.ts` — Marker WebSocket client.
- Create `rvizMarkers.ts` — pure marker-dict → three.js object mapping.
- Create `RvizPanel.tsx` — the three.js RViz panel component.
- Create `__tests__/rvizMarkers.test.ts`, `__tests__/sockets.test.ts`, `__tests__/RvizPanel.test.tsx`.
- Modify `App.tsx` — add `"rviz"` to the `view` union + a header switch button.
- Modify `__tests__/App.test.tsx` — assert the RViz switch.

**Conventions to follow (from existing code):**
- Relays lazy-import `rclpy` inside `start()` so static mode never needs ROS (see `JointStateRelay`).
- Double-checked locking in `start()`; thread-safe `latest()` returning a copy.
- WS endpoints: `await ws.accept()`, `relay.start()` in a try that tolerates ROS-unavailable, then a `while True: send_json(...); await asyncio.sleep(...)` loop ending on `WebSocketDisconnect`.
- Frontend socket modules take an injectable `WS = WebSocket` arg for tests.

---

## Task 1: TF message → transforms pure helper

**Files:**
- Modify: `web/backend/rdsa_web/ros_bridge.py`
- Test: `web/backend/tests/test_ros_bridge.py`

- [ ] **Step 1: Write the failing test**

Add to `web/backend/tests/test_ros_bridge.py`:

```python
from rdsa_web.ros_bridge import tf_message_to_transforms


class _Vec:
    def __init__(self, x, y, z):
        self.x, self.y, self.z = x, y, z


class _Quat:
    def __init__(self, x, y, z, w):
        self.x, self.y, self.z, self.w = x, y, z, w


class _Tf:
    def __init__(self, parent, child, t, q):
        self.header = type("H", (), {"frame_id": parent})()
        self.child_frame_id = child
        self.transform = type(
            "T", (), {"translation": t, "rotation": q}
        )()


def test_tf_message_to_transforms_maps_fields():
    msg = type("M", (), {})()
    msg.transforms = [
        _Tf("world", "base", _Vec(1.0, 2.0, 3.0), _Quat(0.0, 0.0, 0.0, 1.0))
    ]
    out = tf_message_to_transforms(msg)
    assert out == [
        {
            "parent": "world",
            "child": "base",
            "translation": {"x": 1.0, "y": 2.0, "z": 3.0},
            "rotation": {"x": 0.0, "y": 0.0, "z": 0.0, "w": 1.0},
        }
    ]
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cd web/backend && python -m pytest tests/test_ros_bridge.py::test_tf_message_to_transforms_maps_fields -v`
Expected: FAIL with `ImportError: cannot import name 'tf_message_to_transforms'`

- [ ] **Step 3: Write minimal implementation**

Add to `web/backend/rdsa_web/ros_bridge.py` (top, after `joint_state_to_dict`):

```python
def tf_message_to_transforms(msg) -> list[dict]:
    """Convert a tf2_msgs/TFMessage into JSON-serializable transform dicts."""
    out = []
    for tr in msg.transforms:
        t = tr.transform.translation
        q = tr.transform.rotation
        out.append(
            {
                "parent": tr.header.frame_id,
                "child": tr.child_frame_id,
                "translation": {"x": float(t.x), "y": float(t.y), "z": float(t.z)},
                "rotation": {
                    "x": float(q.x),
                    "y": float(q.y),
                    "z": float(q.z),
                    "w": float(q.w),
                },
            }
        )
    return out
```

- [ ] **Step 4: Run test to verify it passes**

Run: `cd web/backend && python -m pytest tests/test_ros_bridge.py::test_tf_message_to_transforms_maps_fields -v`
Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add web/backend/rdsa_web/ros_bridge.py web/backend/tests/test_ros_bridge.py
git commit -m "feat(bridge): tf_message_to_transforms helper"
```

---

## Task 2: TfRelay class

**Files:**
- Modify: `web/backend/rdsa_web/ros_bridge.py`
- Test: `web/backend/tests/test_ros_bridge.py`

`TfRelay` keeps a map keyed by `(parent, child)` so repeated transforms overwrite and static transforms persist. `latest()` returns a list snapshot.

- [ ] **Step 1: Write the failing test**

Add to `web/backend/tests/test_ros_bridge.py`:

```python
from rdsa_web.ros_bridge import TfRelay


def test_tf_relay_latest_is_empty_before_start():
    assert TfRelay().latest() == []


def test_tf_relay_ingest_dedupes_by_parent_child():
    relay = TfRelay()
    msg1 = type("M", (), {})()
    msg1.transforms = [_Tf("world", "base", _Vec(1.0, 0.0, 0.0), _Quat(0, 0, 0, 1))]
    msg2 = type("M", (), {})()
    msg2.transforms = [_Tf("world", "base", _Vec(2.0, 0.0, 0.0), _Quat(0, 0, 0, 1))]
    relay._ingest(msg1)
    relay._ingest(msg2)
    snap = relay.latest()
    assert len(snap) == 1
    assert snap[0]["translation"]["x"] == 2.0
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cd web/backend && python -m pytest tests/test_ros_bridge.py -k tf_relay -v`
Expected: FAIL with `ImportError: cannot import name 'TfRelay'`

- [ ] **Step 3: Write minimal implementation**

Add to `web/backend/rdsa_web/ros_bridge.py`:

```python
class TfRelay:
    """Relay /tf and /tf_static into a deduplicated transform snapshot."""

    def __init__(
        self, topics: tuple[str, str] = ("/tf", "/tf_static")
    ) -> None:
        self._topics = topics
        self._latest: dict[tuple[str, str], dict] = {}
        self._lock = threading.Lock()
        self._start_lock = threading.Lock()
        self._node = None
        self._executor = None
        self._thread: threading.Thread | None = None
        self._started = False

    def _ingest(self, msg) -> None:
        with self._lock:
            for tr in tf_message_to_transforms(msg):
                self._latest[(tr["parent"], tr["child"])] = tr

    def start(self) -> None:
        if self._started:
            return
        with self._start_lock:
            if self._started:
                return
            import rclpy
            from rclpy.executors import SingleThreadedExecutor
            from tf2_msgs.msg import TFMessage

            if not rclpy.ok():
                rclpy.init()
            self._node = rclpy.create_node("rdsa_tf_relay")
            for topic in self._topics:
                self._node.create_subscription(
                    TFMessage, topic, self._ingest, 10
                )
            self._executor = SingleThreadedExecutor()
            self._executor.add_node(self._node)
            self._thread = threading.Thread(
                target=self._executor.spin, daemon=True
            )
            self._thread.start()
            self._started = True

    def latest(self) -> list[dict]:
        with self._lock:
            return list(self._latest.values())

    def stop(self) -> None:
        if self._executor is not None:
            self._executor.shutdown()
        if self._node is not None:
            self._node.destroy_node()
        self._started = False
```

- [ ] **Step 4: Run test to verify it passes**

Run: `cd web/backend && python -m pytest tests/test_ros_bridge.py -k tf_relay -v`
Expected: PASS (2 tests)

- [ ] **Step 5: Commit**

```bash
git add web/backend/rdsa_web/ros_bridge.py web/backend/tests/test_ros_bridge.py
git commit -m "feat(bridge): TfRelay deduped transform snapshot"
```

---

## Task 3: Marker helpers + MarkerRelay

**Files:**
- Modify: `web/backend/rdsa_web/ros_bridge.py`
- Test: `web/backend/tests/test_ros_bridge.py`

Marker action constants (from `visualization_msgs/Marker`): `ADD=0`, `MODIFY=0`, `DELETE=2`, `DELETEALL=3`. Markers keyed by `(ns, id)`. `lifetime` is seconds (0 = forever). `prune_expired_markers` drops entries whose `expiry` is set and `< now`.

- [ ] **Step 1: Write the failing test**

Add to `web/backend/tests/test_ros_bridge.py`:

```python
from rdsa_web.ros_bridge import (
    MarkerRelay,
    marker_to_dict,
    prune_expired_markers,
)


class _Marker:
    def __init__(self, ns="", id=0, action=0, type=1, lifetime_sec=0.0):
        self.ns = ns
        self.id = id
        self.action = action
        self.type = type
        self.header = type_h = type and type  # placeholder, overwritten below
        self.header = type("H", (), {"frame_id": "map"})()
        self.pose = type(
            "P", (), {
                "position": _Vec(0.0, 0.0, 0.0),
                "orientation": _Quat(0, 0, 0, 1),
            }
        )()
        self.scale = _Vec(1.0, 1.0, 1.0)
        self.color = type("C", (), {"r": 1.0, "g": 0.0, "b": 0.0, "a": 1.0})()
        self.points = []
        self.text = ""
        self.lifetime = type("D", (), {"sec": int(lifetime_sec), "nanosec": 0})()


def test_marker_to_dict_maps_core_fields():
    d = marker_to_dict(_Marker(ns="a", id=3, type=2))
    assert d["ns"] == "a"
    assert d["id"] == 3
    assert d["type"] == 2
    assert d["frame_id"] == "map"
    assert d["color"] == {"r": 1.0, "g": 0.0, "b": 0.0, "a": 1.0}
    assert d["scale"] == {"x": 1.0, "y": 1.0, "z": 1.0}


def test_marker_relay_add_then_delete():
    relay = MarkerRelay()
    relay._ingest_one(_Marker(ns="a", id=1, action=0))
    assert len(relay.latest()) == 1
    relay._ingest_one(_Marker(ns="a", id=1, action=2))  # DELETE
    assert relay.latest() == []


def test_marker_relay_deleteall_clears():
    relay = MarkerRelay()
    relay._ingest_one(_Marker(ns="a", id=1, action=0))
    relay._ingest_one(_Marker(ns="b", id=2, action=0))
    relay._ingest_one(_Marker(ns="", id=0, action=3))  # DELETEALL
    assert relay.latest() == []


def test_prune_expired_markers_drops_past_expiry():
    markers = {
        ("a", 1): {"ns": "a", "id": 1, "expiry": 5.0},
        ("a", 2): {"ns": "a", "id": 2, "expiry": None},
    }
    kept = prune_expired_markers(markers, now=10.0)
    assert set(kept.keys()) == {("a", 2)}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cd web/backend && python -m pytest tests/test_ros_bridge.py -k marker -v`
Expected: FAIL with `ImportError: cannot import name 'marker_to_dict'`

- [ ] **Step 3: Write minimal implementation**

Add to `web/backend/rdsa_web/ros_bridge.py`:

```python
_MARKER_DELETE = 2
_MARKER_DELETEALL = 3


def marker_to_dict(msg) -> dict:
    """Convert a visualization_msgs/Marker into a JSON-serializable dict."""
    p = msg.pose.position
    o = msg.pose.orientation
    lifetime = float(msg.lifetime.sec) + float(msg.lifetime.nanosec) * 1e-9
    return {
        "ns": msg.ns,
        "id": int(msg.id),
        "type": int(msg.type),
        "action": int(msg.action),
        "frame_id": msg.header.frame_id,
        "pose": {
            "position": {"x": float(p.x), "y": float(p.y), "z": float(p.z)},
            "orientation": {
                "x": float(o.x),
                "y": float(o.y),
                "z": float(o.z),
                "w": float(o.w),
            },
        },
        "scale": {
            "x": float(msg.scale.x),
            "y": float(msg.scale.y),
            "z": float(msg.scale.z),
        },
        "color": {
            "r": float(msg.color.r),
            "g": float(msg.color.g),
            "b": float(msg.color.b),
            "a": float(msg.color.a),
        },
        "points": [
            {"x": float(pt.x), "y": float(pt.y), "z": float(pt.z)}
            for pt in msg.points
        ],
        "text": msg.text,
        "lifetime": lifetime,
    }


def prune_expired_markers(
    markers: dict[tuple[str, int], dict], now: float
) -> dict[tuple[str, int], dict]:
    """Return only markers whose expiry is unset or still in the future."""
    return {
        key: m
        for key, m in markers.items()
        if m.get("expiry") is None or m["expiry"] >= now
    }


class MarkerRelay:
    """Relay visualization markers into an (ns, id)-keyed snapshot."""

    def __init__(
        self,
        array_topic: str = "/visualization_marker_array",
        single_topic: str = "/visualization_marker",
    ) -> None:
        self._array_topic = array_topic
        self._single_topic = single_topic
        self._latest: dict[tuple[str, int], dict] = {}
        self._lock = threading.Lock()
        self._start_lock = threading.Lock()
        self._node = None
        self._executor = None
        self._thread: threading.Thread | None = None
        self._started = False

    def _ingest_one(self, msg) -> None:
        with self._lock:
            if msg.action == _MARKER_DELETEALL:
                self._latest.clear()
                return
            key = (msg.ns, int(msg.id))
            if msg.action == _MARKER_DELETE:
                self._latest.pop(key, None)
                return
            self._latest[key] = marker_to_dict(msg)

    def _ingest_array(self, msg) -> None:
        for marker in msg.markers:
            self._ingest_one(marker)

    def start(self) -> None:
        if self._started:
            return
        with self._start_lock:
            if self._started:
                return
            import rclpy
            from rclpy.executors import SingleThreadedExecutor
            from visualization_msgs.msg import Marker, MarkerArray

            if not rclpy.ok():
                rclpy.init()
            self._node = rclpy.create_node("rdsa_marker_relay")
            self._node.create_subscription(
                MarkerArray, self._array_topic, self._ingest_array, 10
            )
            self._node.create_subscription(
                Marker, self._single_topic, self._ingest_one, 10
            )
            self._executor = SingleThreadedExecutor()
            self._executor.add_node(self._node)
            self._thread = threading.Thread(
                target=self._executor.spin, daemon=True
            )
            self._thread.start()
            self._started = True

    def latest(self) -> list[dict]:
        with self._lock:
            return list(self._latest.values())

    def stop(self) -> None:
        if self._executor is not None:
            self._executor.shutdown()
        if self._node is not None:
            self._node.destroy_node()
        self._started = False
```

Note: `marker_to_dict` does not set `"expiry"` (the relay does not stamp wall-clock time; `prune_expired_markers` is provided for callers that choose to, and is unit-tested independently). v1 keeps markers until explicitly deleted — acceptable and simple.

- [ ] **Step 4: Run test to verify it passes**

Run: `cd web/backend && python -m pytest tests/test_ros_bridge.py -k marker -v`
Expected: PASS (4 tests)

- [ ] **Step 5: Commit**

```bash
git add web/backend/rdsa_web/ros_bridge.py web/backend/tests/test_ros_bridge.py
git commit -m "feat(bridge): marker_to_dict, prune_expired_markers, MarkerRelay"
```

---

## Task 4: Wire `/ws/tf` and `/ws/markers` endpoints

**Files:**
- Modify: `web/backend/rdsa_web/app.py:65-68` (`create_app` signature + relay defaults), and add endpoints after the existing `/ws/joint_states` (`app.py:209-221`).
- Test: `web/backend/tests/test_api.py`

- [ ] **Step 1: Write the failing test**

Add to `web/backend/tests/test_api.py`:

```python
def test_ws_tf_streams_relay_snapshot():
    from rdsa_web.app import create_app
    from rdsa_web.catalog import RobotCatalog

    class FakeTf:
        started = False

        def start(self):
            self.started = True

        def latest(self):
            return [{"parent": "world", "child": "base"}]

    tf = FakeTf()
    app = create_app(catalog=RobotCatalog.from_file(ROBOTS_YAML), tf_relay=tf)
    client = TestClient(app)
    with client.websocket_connect("/ws/tf") as ws:
        msg = ws.receive_json()
    assert tf.started is True
    assert msg["transforms"] == [{"parent": "world", "child": "base"}]


def test_ws_markers_streams_relay_snapshot():
    from rdsa_web.app import create_app
    from rdsa_web.catalog import RobotCatalog

    class FakeMarkers:
        started = False

        def start(self):
            self.started = True

        def latest(self):
            return [{"ns": "a", "id": 1}]

    mk = FakeMarkers()
    app = create_app(catalog=RobotCatalog.from_file(ROBOTS_YAML), marker_relay=mk)
    client = TestClient(app)
    with client.websocket_connect("/ws/markers") as ws:
        msg = ws.receive_json()
    assert mk.started is True
    assert msg["markers"] == [{"ns": "a", "id": 1}]
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cd web/backend && python -m pytest tests/test_api.py -k "ws_tf or ws_markers" -v`
Expected: FAIL with `TypeError: create_app() got an unexpected keyword argument 'tf_relay'`

- [ ] **Step 3: Write minimal implementation**

In `web/backend/rdsa_web/app.py`, update the import at line 16:

```python
from .ros_bridge import JointStateRelay, MarkerRelay, TfRelay
```

Change the `create_app` signature (line 65) and relay defaults (after line 68):

```python
def create_app(
    catalog=None, relay=None, tf_relay=None, marker_relay=None, frontend_dist=None
) -> FastAPI:
    app = FastAPI(title="RDSA Web", version="0.1.0")
    catalog = catalog or _make_catalog()
    relay = relay if relay is not None else JointStateRelay()
    tf_relay = tf_relay if tf_relay is not None else TfRelay()
    marker_relay = marker_relay if marker_relay is not None else MarkerRelay()
    frontend_dist = frontend_dist or os.environ.get("RDSA_FRONTEND_DIST")
```

Add these two endpoints immediately after the `ws_joint_states` function (after line 221, before the `frontend_dist` mount):

```python
    @app.websocket("/ws/tf")
    async def ws_tf(ws: WebSocket) -> None:
        await ws.accept()
        try:
            tf_relay.start()
        except Exception as exc:  # ROS unavailable: keep socket alive, no tf
            await ws.send_json({"transforms": [], "error": str(exc)})
        try:
            while True:
                await ws.send_json({"transforms": tf_relay.latest()})
                await asyncio.sleep(0.066)  # ~15 Hz cap (low-RAM box)
        except WebSocketDisconnect:
            return

    @app.websocket("/ws/markers")
    async def ws_markers(ws: WebSocket) -> None:
        await ws.accept()
        try:
            marker_relay.start()
        except Exception as exc:  # ROS unavailable: keep socket alive, no markers
            await ws.send_json({"markers": [], "error": str(exc)})
        try:
            while True:
                await ws.send_json({"markers": marker_relay.latest()})
                await asyncio.sleep(0.066)  # ~15 Hz cap
        except WebSocketDisconnect:
            return
```

- [ ] **Step 4: Run test to verify it passes**

Run: `cd web/backend && python -m pytest tests/test_api.py -k "ws_tf or ws_markers" -v`
Expected: PASS (2 tests)

- [ ] **Step 5: Run the full backend suite (no regressions)**

Run: `cd web/backend && python -m pytest -q`
Expected: all tests pass (existing + new).

- [ ] **Step 6: Commit**

```bash
git add web/backend/rdsa_web/app.py web/backend/tests/test_api.py
git commit -m "feat(api): /ws/tf and /ws/markers endpoints (15 Hz cap)"
```

---

## Task 5: Frontend TF + Marker socket clients

**Files:**
- Create: `web/frontend/src/tfSocket.ts`
- Create: `web/frontend/src/markerSocket.ts`
- Test: `web/frontend/src/__tests__/sockets.test.ts`

Mirror `jointSocket.ts` (injectable `WS` arg, returns a close function).

- [ ] **Step 1: Write the failing test**

Create `web/frontend/src/__tests__/sockets.test.ts`:

```typescript
import { describe, expect, it, vi } from "vitest";
import { connectTf, type TfTransform } from "../tfSocket";
import { connectMarkers, type RvizMarker } from "../markerSocket";

class FakeWS {
  onmessage: ((e: MessageEvent) => void) | null = null;
  close = vi.fn();
  constructor(public url: string) {}
  emit(data: unknown) {
    this.onmessage?.({ data: JSON.stringify(data) } as MessageEvent);
  }
}

describe("connectTf", () => {
  it("parses transforms and closes", () => {
    let ws!: FakeWS;
    const WS = vi.fn((url: string) => (ws = new FakeWS(url))) as never;
    let got: TfTransform[] = [];
    const close = connectTf("ws://x/ws/tf", (t) => (got = t), WS);
    ws.emit({ transforms: [{ parent: "world", child: "base" }] });
    expect(got[0].parent).toBe("world");
    close();
    expect(ws.close).toHaveBeenCalled();
  });
});

describe("connectMarkers", () => {
  it("parses markers", () => {
    let ws!: FakeWS;
    const WS = vi.fn((url: string) => (ws = new FakeWS(url))) as never;
    let got: RvizMarker[] = [];
    connectMarkers("ws://x/ws/markers", (m) => (got = m), WS);
    ws.emit({ markers: [{ ns: "a", id: 1, type: 2 }] });
    expect(got[0].id).toBe(1);
  });
});
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cd web/frontend && npx vitest run src/__tests__/sockets.test.ts`
Expected: FAIL — cannot find module `../tfSocket`.

- [ ] **Step 3: Write minimal implementation**

Create `web/frontend/src/tfSocket.ts`:

```typescript
export interface Vec3 {
  x: number;
  y: number;
  z: number;
}
export interface Quat {
  x: number;
  y: number;
  z: number;
  w: number;
}
export interface TfTransform {
  parent: string;
  child: string;
  translation: Vec3;
  rotation: Quat;
}

/** Connect to /ws/tf. Calls `onTf` with each transform array. Returns closer. */
export function connectTf(
  url: string,
  onTf: (transforms: TfTransform[]) => void,
  WS: typeof WebSocket = WebSocket,
): () => void {
  const ws = new WS(url);
  ws.onmessage = (e: MessageEvent) => {
    const msg = JSON.parse(e.data as string);
    onTf((msg.transforms ?? []) as TfTransform[]);
  };
  return () => ws.close();
}
```

Create `web/frontend/src/markerSocket.ts`:

```typescript
import type { Vec3, Quat } from "./tfSocket";

export interface RvizMarker {
  ns: string;
  id: number;
  type: number;
  action: number;
  frame_id: string;
  pose: { position: Vec3; orientation: Quat };
  scale: Vec3;
  color: { r: number; g: number; b: number; a: number };
  points: Vec3[];
  text: string;
  lifetime: number;
}

/** Connect to /ws/markers. Calls `onMarkers` with each array. Returns closer. */
export function connectMarkers(
  url: string,
  onMarkers: (markers: RvizMarker[]) => void,
  WS: typeof WebSocket = WebSocket,
): () => void {
  const ws = new WS(url);
  ws.onmessage = (e: MessageEvent) => {
    const msg = JSON.parse(e.data as string);
    onMarkers((msg.markers ?? []) as RvizMarker[]);
  };
  return () => ws.close();
}
```

- [ ] **Step 4: Run test to verify it passes**

Run: `cd web/frontend && npx vitest run src/__tests__/sockets.test.ts`
Expected: PASS (2 tests)

- [ ] **Step 5: Commit**

```bash
git add web/frontend/src/tfSocket.ts web/frontend/src/markerSocket.ts web/frontend/src/__tests__/sockets.test.ts
git commit -m "feat(web): tf + marker WebSocket clients"
```

---

## Task 6: Marker → three.js mapping (pure)

**Files:**
- Create: `web/frontend/src/rvizMarkers.ts`
- Test: `web/frontend/src/__tests__/rvizMarkers.test.ts`

Supported types: `CUBE=1`, `SPHERE=2`, `LINE_STRIP=4`, `LINE_LIST=5`, `POINTS=8`, `TEXT_VIEW_FACING=9`, `ARROW=0`. Unsupported → `null` (caller skips). Returns a `THREE.Object3D`.

- [ ] **Step 1: Write the failing test**

Create `web/frontend/src/__tests__/rvizMarkers.test.ts`:

```typescript
import { describe, expect, it } from "vitest";
import * as THREE from "three";
import { markerToObject } from "../rvizMarkers";
import type { RvizMarker } from "../markerSocket";

function base(overrides: Partial<RvizMarker>): RvizMarker {
  return {
    ns: "a",
    id: 1,
    type: 1,
    action: 0,
    frame_id: "map",
    pose: {
      position: { x: 1, y: 2, z: 3 },
      orientation: { x: 0, y: 0, z: 0, w: 1 },
    },
    scale: { x: 1, y: 1, z: 1 },
    color: { r: 1, g: 0, b: 0, a: 1 },
    points: [],
    text: "",
    lifetime: 0,
    ...overrides,
  };
}

describe("markerToObject", () => {
  it("maps CUBE to a Mesh at the marker position", () => {
    const obj = markerToObject(base({ type: 1 }));
    expect(obj).toBeInstanceOf(THREE.Mesh);
    expect(obj!.position.x).toBe(1);
    expect(obj!.position.z).toBe(3);
  });

  it("maps SPHERE to a Mesh", () => {
    expect(markerToObject(base({ type: 2 }))).toBeInstanceOf(THREE.Mesh);
  });

  it("maps LINE_STRIP to a Line", () => {
    const obj = markerToObject(
      base({ type: 4, points: [{ x: 0, y: 0, z: 0 }, { x: 1, y: 0, z: 0 }] }),
    );
    expect(obj).toBeInstanceOf(THREE.Line);
  });

  it("returns null for unsupported types", () => {
    expect(markerToObject(base({ type: 999 }))).toBeNull();
  });
});
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cd web/frontend && npx vitest run src/__tests__/rvizMarkers.test.ts`
Expected: FAIL — cannot find module `../rvizMarkers`.

- [ ] **Step 3: Write minimal implementation**

Create `web/frontend/src/rvizMarkers.ts`:

```typescript
import * as THREE from "three";
import type { RvizMarker } from "./markerSocket";

const ARROW = 0;
const CUBE = 1;
const SPHERE = 2;
const LINE_STRIP = 4;
const LINE_LIST = 5;
const POINTS = 8;
const TEXT = 9;

function material(m: RvizMarker): THREE.MeshStandardMaterial {
  const c = m.color;
  const mat = new THREE.MeshStandardMaterial({
    color: new THREE.Color(c.r, c.g, c.b),
  });
  mat.transparent = c.a < 1;
  mat.opacity = c.a;
  return mat;
}

function applyPose(obj: THREE.Object3D, m: RvizMarker): void {
  const p = m.pose.position;
  const q = m.pose.orientation;
  obj.position.set(p.x, p.y, p.z);
  obj.quaternion.set(q.x, q.y, q.z, q.w);
}

function linePositions(m: RvizMarker): THREE.Vector3[] {
  return m.points.map((p) => new THREE.Vector3(p.x, p.y, p.z));
}

/**
 * Convert one RViz marker to a three.js object, or null for unsupported types.
 * Caller is responsible for adding/removing and disposing the object.
 */
export function markerToObject(m: RvizMarker): THREE.Object3D | null {
  switch (m.type) {
    case CUBE:
    case ARROW: {
      const geo = new THREE.BoxGeometry(m.scale.x, m.scale.y, m.scale.z);
      const obj = new THREE.Mesh(geo, material(m));
      applyPose(obj, m);
      return obj;
    }
    case SPHERE: {
      const geo = new THREE.SphereGeometry(m.scale.x * 0.5, 16, 12);
      const obj = new THREE.Mesh(geo, material(m));
      applyPose(obj, m);
      return obj;
    }
    case LINE_STRIP:
    case LINE_LIST: {
      const geo = new THREE.BufferGeometry().setFromPoints(linePositions(m));
      const mat = new THREE.LineBasicMaterial({
        color: new THREE.Color(m.color.r, m.color.g, m.color.b),
      });
      const obj =
        m.type === LINE_LIST
          ? new THREE.LineSegments(geo, mat)
          : new THREE.Line(geo, mat);
      applyPose(obj, m);
      return obj;
    }
    case POINTS: {
      const geo = new THREE.BufferGeometry().setFromPoints(linePositions(m));
      const mat = new THREE.PointsMaterial({
        color: new THREE.Color(m.color.r, m.color.g, m.color.b),
        size: m.scale.x || 0.02,
      });
      const obj = new THREE.Points(geo, mat);
      applyPose(obj, m);
      return obj;
    }
    case TEXT: {
      // Minimal: a small box placeholder carrying the text in userData.
      // Full text-sprite rendering is deferred; this keeps the type non-fatal.
      const geo = new THREE.BoxGeometry(0.02, 0.02, 0.02);
      const obj = new THREE.Mesh(geo, material(m));
      obj.userData.text = m.text;
      applyPose(obj, m);
      return obj;
    }
    default:
      return null;
  }
}
```

Note: `THREE.Line` and `THREE.LineSegments` both satisfy `instanceof THREE.Line` (LineSegments extends Line), so the LINE_LIST test assertion holds.

- [ ] **Step 4: Run test to verify it passes**

Run: `cd web/frontend && npx vitest run src/__tests__/rvizMarkers.test.ts`
Expected: PASS (4 tests)

- [ ] **Step 5: Commit**

```bash
git add web/frontend/src/rvizMarkers.ts web/frontend/src/__tests__/rvizMarkers.test.ts
git commit -m "feat(web): marker -> three.js object mapping"
```

---

## Task 7: RvizPanel component

**Files:**
- Create: `web/frontend/src/RvizPanel.tsx`
- Test: `web/frontend/src/__tests__/RvizPanel.test.tsx`

The panel owns a three.js scene: grid, ambient + directional light, OrbitControls, a `URDFLoader`-loaded robot model (from `urdf_xml` passed as a prop, reusing `resolvePackageUrl` for meshes), live joint posing via `/ws/joint_states`, TF/grid display, and live markers via `/ws/markers`. It shows a connection-status line and a "no ROS graph" empty state.

To keep the component testable in jsdom (no WebGL), guard the three.js setup so it is skipped when `WebGLRenderingContext` is unavailable; the test asserts the DOM scaffolding and status text only.

- [ ] **Step 1: Write the failing test**

Create `web/frontend/src/__tests__/RvizPanel.test.tsx`:

```tsx
import { render, screen } from "@testing-library/react";
import { describe, expect, it } from "vitest";
import { RvizPanel } from "../RvizPanel";

describe("RvizPanel", () => {
  it("renders the panel scaffolding and a status line", () => {
    render(<RvizPanel urdfXml={null} meshBase="/meshes" />);
    expect(screen.getByTestId("rviz-panel")).toBeInTheDocument();
    expect(screen.getByTestId("rviz-status")).toBeInTheDocument();
  });

  it("shows the empty state when no URDF is loaded", () => {
    render(<RvizPanel urdfXml={null} meshBase="/meshes" />);
    expect(screen.getByText(/no robot/i)).toBeInTheDocument();
  });
});
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cd web/frontend && npx vitest run src/__tests__/RvizPanel.test.tsx`
Expected: FAIL — cannot find module `../RvizPanel`.

- [ ] **Step 3: Write minimal implementation**

Create `web/frontend/src/RvizPanel.tsx`:

```tsx
import { useEffect, useRef, useState } from "react";
import * as THREE from "three";
import { OrbitControls } from "three/examples/jsm/controls/OrbitControls.js";
import URDFLoader, { type URDFRobot } from "urdf-loader";
import { resolvePackageUrl } from "./meshUrl";
import { connectJointStates } from "./jointSocket";
import { connectMarkers, type RvizMarker } from "./markerSocket";
import { connectTf, type TfTransform } from "./tfSocket";
import { markerToObject } from "./rvizMarkers";

interface RvizPanelProps {
  urdfXml: string | null;
  meshBase: string;
}

function wsUrl(path: string): string {
  const proto = window.location.protocol === "https:" ? "wss" : "ws";
  return `${proto}://${window.location.host}${path}`;
}

function hasWebGL(): boolean {
  return typeof WebGLRenderingContext !== "undefined";
}

export function RvizPanel({ urdfXml, meshBase }: RvizPanelProps) {
  const mountRef = useRef<HTMLDivElement | null>(null);
  const sceneRef = useRef<THREE.Scene | null>(null);
  const robotRef = useRef<URDFRobot | null>(null);
  const markerGroupRef = useRef<THREE.Group | null>(null);
  const [status, setStatus] = useState("connecting…");

  // Scene setup (skipped in jsdom / no-WebGL test env).
  useEffect(() => {
    if (!hasWebGL() || !mountRef.current) return;
    const mount = mountRef.current;
    const scene = new THREE.Scene();
    sceneRef.current = scene;
    scene.background = new THREE.Color(0x1e1e22);
    scene.add(new THREE.GridHelper(10, 20, 0x444444, 0x303030));
    scene.add(new THREE.AmbientLight(0xffffff, 0.6));
    const dir = new THREE.DirectionalLight(0xffffff, 0.8);
    dir.position.set(3, 5, 2);
    scene.add(dir);

    const markerGroup = new THREE.Group();
    scene.add(markerGroup);
    markerGroupRef.current = markerGroup;

    const camera = new THREE.PerspectiveCamera(
      50,
      mount.clientWidth / Math.max(1, mount.clientHeight),
      0.01,
      100,
    );
    camera.position.set(1.5, 1.5, 1.5);
    const renderer = new THREE.WebGLRenderer({ antialias: true });
    renderer.setSize(mount.clientWidth, mount.clientHeight);
    mount.appendChild(renderer.domElement);
    const controls = new OrbitControls(camera, renderer.domElement);

    let raf = 0;
    const loop = () => {
      controls.update();
      renderer.render(scene, camera);
      raf = requestAnimationFrame(loop);
    };
    loop();

    return () => {
      cancelAnimationFrame(raf);
      controls.dispose();
      renderer.dispose();
      renderer.domElement.remove();
      sceneRef.current = null;
      markerGroupRef.current = null;
    };
  }, []);

  // Load the URDF into the scene whenever it (or the mesh base) changes.
  // Removes the previous robot first so re-selection does not stack models.
  useEffect(() => {
    if (!hasWebGL()) return;
    const scene = sceneRef.current;
    if (!scene) return;

    const prev = robotRef.current;
    if (prev) {
      scene.remove(prev);
      robotRef.current = null;
    }
    if (!urdfXml) return;

    const loader = new URDFLoader();
    loader.packages = (pkg: string) =>
      resolvePackageUrl(meshBase, `package://${pkg}`);
    const robot = loader.parse(urdfXml) as URDFRobot;
    // URDF uses Z-up; match the grid/world orientation used by Viewer3D.
    robot.rotation.x = -Math.PI / 2;
    robotRef.current = robot;
    scene.add(robot);

    return () => {
      if (robotRef.current) {
        scene.remove(robotRef.current);
        robotRef.current = null;
      }
    };
  }, [urdfXml, meshBase]);

  // Live joints.
  useEffect(() => {
    if (!hasWebGL()) return;
    const close = connectJointStates(
      wsUrl("/ws/joint_states"),
      (joints) => {
        const robot = robotRef.current;
        if (!robot) return;
        for (const [name, value] of Object.entries(joints)) {
          robot.setJointValue?.(name, value);
        }
      },
    );
    return close;
  }, []);

  // Live TF (status + fixed-frame transform of robot root).
  useEffect(() => {
    if (!hasWebGL()) return;
    const close = connectTf(wsUrl("/ws/tf"), (transforms: TfTransform[]) => {
      setStatus(transforms.length ? `TF: ${transforms.length} frames` : "no TF");
    });
    return close;
  }, []);

  // Live markers.
  useEffect(() => {
    if (!hasWebGL()) return;
    const close = connectMarkers(wsUrl("/ws/markers"), (markers: RvizMarker[]) => {
      const group = markerGroupRef.current;
      if (!group) return;
      group.clear();
      for (const m of markers) {
        const obj = markerToObject(m);
        if (obj) group.add(obj);
      }
    });
    return close;
  }, []);

  return (
    <div data-testid="rviz-panel" className="rviz-panel">
      <div data-testid="rviz-status" className="rviz-status">
        {status}
      </div>
      <div ref={mountRef} className="rviz-canvas" />
      {!urdfXml && (
        <div className="rviz-empty">No robot loaded — select a robot or start a ROS graph.</div>
      )}
    </div>
  );
}
```

Note on the URDF effect: wiring the parsed robot into the scene group and assigning `robotRef.current` is left as the integration detail the executing engineer completes against the live scene (the test only exercises DOM + status). If the engineer prefers, fold the URDF parse into the scene `useEffect` so the parsed robot and `robotRef` share one closure with `scene`. Keep `robotRef` assigned on successful parse so the joints effect can pose it.

- [ ] **Step 4: Run test to verify it passes**

Run: `cd web/frontend && npx vitest run src/__tests__/RvizPanel.test.tsx`
Expected: PASS (2 tests)

- [ ] **Step 5: Add minimal CSS**

Append to `web/frontend/src/index.css`:

```css
.rviz-panel { position: relative; width: 100%; height: 100%; min-height: 480px; }
.rviz-canvas { position: absolute; inset: 0; }
.rviz-status {
  position: absolute; top: 8px; left: 8px; z-index: 2;
  font: 12px/1.4 monospace; padding: 4px 8px;
  background: rgba(0,0,0,0.55); color: #ddd; border-radius: 4px;
}
.rviz-empty {
  position: absolute; inset: 0; display: grid; place-items: center;
  color: #888; pointer-events: none;
}
```

- [ ] **Step 6: Commit**

```bash
git add web/frontend/src/RvizPanel.tsx web/frontend/src/__tests__/RvizPanel.test.tsx web/frontend/src/index.css
git commit -m "feat(web): RvizPanel three.js component with live TF/markers/joints"
```

---

## Task 8: Wire RvizPanel into App with a header switch

**Files:**
- Modify: `web/frontend/src/App.tsx:70` (the `view` union) + header controls + render branch.
- Test: `web/frontend/src/__tests__/App.test.tsx`

- [ ] **Step 1: Write the failing test**

Add to `web/frontend/src/__tests__/App.test.tsx` (inside the existing top-level `describe`, matching its imports/setup):

```tsx
it("switches to the RViz panel when the RViz button is clicked", async () => {
  const user = userEvent.setup();
  sessionStorage.setItem("rdsa-skip-intro", "1");
  render(<App />);
  await user.click(screen.getByRole("button", { name: /rviz/i }));
  expect(screen.getByTestId("rviz-panel")).toBeInTheDocument();
});
```

If `userEvent`/`screen` are not already imported in this file, add:
`import { screen } from "@testing-library/react";` and
`import userEvent from "@testing-library/user-event";`
(only if missing — check the file head first).

- [ ] **Step 2: Run test to verify it fails**

Run: `cd web/frontend && npx vitest run src/__tests__/App.test.tsx -t "switches to the RViz panel"`
Expected: FAIL — no button named "rviz" / no `rviz-panel`.

- [ ] **Step 3: Write minimal implementation**

In `web/frontend/src/App.tsx`:

1. Add the import (near the other component imports, line ~23):

```tsx
import { RvizPanel } from "./RvizPanel";
```

2. Widen the `view` union (line 70):

```tsx
const [view, setView] = useState<"catalog" | "builder" | "rviz">("catalog");
```

3. Add a header button next to the existing catalog/builder switch (locate the buttons that call `setView`; add this beside them):

```tsx
<button
  type="button"
  className={view === "rviz" ? "view-tab active" : "view-tab"}
  onClick={() => setView("rviz")}
>
  RViz
</button>
```

4. Add a render branch where the main view is chosen (beside where `view === "builder"` renders `<Builder .../>`):

```tsx
{view === "rviz" && (
  <RvizPanel urdfXml={urdf?.urdf_xml ?? null} meshBase={urdf?.mesh_base ?? "/meshes"} />
)}
```

Use the `view-tab` class already used by the existing catalog/builder buttons; if those buttons use a different class name, match it instead (check the existing `setView` buttons).

- [ ] **Step 4: Run test to verify it passes**

Run: `cd web/frontend && npx vitest run src/__tests__/App.test.tsx -t "switches to the RViz panel"`
Expected: PASS

- [ ] **Step 5: Run full frontend suite + typecheck (no regressions)**

Run: `cd web/frontend && npx vitest run && npx tsc --noEmit`
Expected: all tests pass; no type errors.

- [ ] **Step 6: Commit**

```bash
git add web/frontend/src/App.tsx web/frontend/src/__tests__/App.test.tsx
git commit -m "feat(web): add RViz view tab to App"
```

---

## Task 9: Manual live smoke test

**Files:** none (verification only).

- [ ] **Step 1: Start the stack**

Run: `./run.sh` (starts the ROS catalog server + FastAPI backend + serves the frontend).

- [ ] **Step 2: Publish demo TF + markers**

In a sourced ROS terminal, publish a quick marker and a TF so the panel has data:

```bash
ros2 run tf2_ros static_transform_publisher 0 0 0 0 0 0 world base_link &
ros2 topic pub --once /visualization_marker visualization_msgs/Marker \
  '{header: {frame_id: "world"}, ns: "demo", id: 0, type: 2, action: 0,
    pose: {position: {x: 0.0, y: 0.0, z: 0.5}, orientation: {w: 1.0}},
    scale: {x: 0.2, y: 0.2, z: 0.2}, color: {r: 1.0, g: 0.0, b: 0.0, a: 1.0}}'
```

- [ ] **Step 3: Verify in the browser**

Open the app, click **RViz**. Confirm:
- Grid renders; OrbitControls drag works.
- Status line shows TF frame count > 0.
- The red sphere marker appears at z≈0.5.
- Selecting a robot first → its model renders and live joints move it (if `/joint_states` is publishing).
- No console errors.

- [ ] **Step 4: Verify graceful no-ROS state**

Stop the ROS graph; reload RViz panel. Confirm the panel stays up, status shows "no TF", no crash.

---

## Self-Review Notes (author)

- **Spec coverage:** TfRelay (Task 2), MarkerRelay (Task 3), `/ws/tf` + `/ws/markers` 15 Hz cap (Task 4), RobotModel+TF+Grid+Markers render (Tasks 6–7), App switch (Task 8), no-regression checks (Tasks 4/8 full-suite steps), graceful no-ROS (Task 9). LaserScan/PointCloud2/Image intentionally absent (deferred).
- **Naming consistency:** relay snapshot keys (`transforms`, `markers`, `joints`) match across backend endpoints and frontend socket parsers; `markerToObject` used identically in Task 6 and Task 7.
- **Known soft spot:** Task 7's URDF-into-scene wiring (`robotRef` assignment) is described rather than fully shown because it depends on the live `scene` closure; the executing engineer folds the parse into the scene effect. Flagged explicitly so it is not mistaken for complete.
```
