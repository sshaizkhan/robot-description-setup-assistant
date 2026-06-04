from rdsa_web.models import (
    CategoryInfo,
    RobotConfig,
    RobotFilter,
    RobotSpecifications,
)


def test_specifications_defaults():
    spec = RobotSpecifications()
    assert spec.degrees_of_freedom == 0
    assert spec.payload_kg == 0.0
    assert spec.mounting_options == []
    assert spec.collaborative is False


def test_robot_config_minimal():
    robot = RobotConfig(id="ur3", display_name="UR3", urdf_package="ur_description")
    assert robot.category == ""
    assert robot.tags == []
    assert isinstance(robot.specifications, RobotSpecifications)


def test_filter_is_empty_by_default():
    f = RobotFilter()
    assert f.is_empty() is True
    assert RobotFilter(category="kuka").is_empty() is False
