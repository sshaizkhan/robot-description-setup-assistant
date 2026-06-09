from rdsa_web.ros_bridge import (
    JointStateRelay,
    MarkerRelay,
    TfRelay,
    joint_state_to_dict,
    marker_to_dict,
    prune_expired_markers,
    tf_message_to_transforms,
)


def test_joint_state_to_dict_zips_names_and_positions():
    assert joint_state_to_dict(["a", "b"], [0.5, 1.0]) == {"a": 0.5, "b": 1.0}


def test_joint_state_to_dict_truncates_to_shortest():
    assert joint_state_to_dict(["a", "b"], [0.5]) == {"a": 0.5}


def test_relay_latest_is_empty_before_start():
    relay = JointStateRelay()
    assert relay.latest() == {}


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


class _Marker:
    def __init__(self, ns="", id=0, action=0, marker_type=1, lifetime_sec=0.0):
        self.ns = ns
        self.id = id
        self.action = action
        self.type = marker_type
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
    d = marker_to_dict(_Marker(ns="a", id=3, marker_type=2))
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
