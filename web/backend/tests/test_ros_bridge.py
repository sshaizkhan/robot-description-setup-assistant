from rdsa_web.ros_bridge import JointStateRelay, joint_state_to_dict, tf_message_to_transforms


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
