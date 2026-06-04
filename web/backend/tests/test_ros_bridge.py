from rdsa_web.ros_bridge import JointStateRelay, joint_state_to_dict


def test_joint_state_to_dict_zips_names_and_positions():
    assert joint_state_to_dict(["a", "b"], [0.5, 1.0]) == {"a": 0.5, "b": 1.0}


def test_joint_state_to_dict_truncates_to_shortest():
    assert joint_state_to_dict(["a", "b"], [0.5]) == {"a": 0.5}


def test_relay_latest_is_empty_before_start():
    relay = JointStateRelay()
    assert relay.latest() == {}
