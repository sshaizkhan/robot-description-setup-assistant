"""Relay ROS /joint_states to the web layer via rclpy.

rclpy is imported lazily inside start() so that importing this module (and
running static-mode code) never requires a ROS environment.
"""
from __future__ import annotations

import threading
from typing import Iterable


def joint_state_to_dict(
    names: Iterable[str], positions: Iterable[float]
) -> dict[str, float]:
    return {name: float(pos) for name, pos in zip(names, positions)}


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


class JointStateRelay:
    def __init__(self, topic: str = "/joint_states") -> None:
        self._topic = topic
        self._latest: dict[str, float] = {}
        self._lock = threading.Lock()
        self._start_lock = threading.Lock()
        self._node = None
        self._executor = None
        self._thread: threading.Thread | None = None
        self._started = False

    def start(self) -> None:
        # Double-checked locking: concurrent first WebSocket connections must not
        # each create a duplicate rclpy node/executor.
        if self._started:
            return
        with self._start_lock:
            if self._started:
                return
            import rclpy
            from rclpy.executors import SingleThreadedExecutor
            from sensor_msgs.msg import JointState

            if not rclpy.ok():
                rclpy.init()
            self._node = rclpy.create_node("rdsa_joint_relay")

            def _cb(msg) -> None:
                with self._lock:
                    self._latest = joint_state_to_dict(msg.name, msg.position)

            self._node.create_subscription(JointState, self._topic, _cb, 10)
            self._executor = SingleThreadedExecutor()
            self._executor.add_node(self._node)
            self._thread = threading.Thread(
                target=self._executor.spin, daemon=True
            )
            self._thread.start()
            self._started = True

    def latest(self) -> dict[str, float]:
        with self._lock:
            return dict(self._latest)

    def stop(self) -> None:
        if self._executor is not None:
            self._executor.shutdown()
        if self._node is not None:
            self._node.destroy_node()
        self._started = False


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
