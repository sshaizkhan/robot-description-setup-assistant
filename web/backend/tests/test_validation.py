import rdsa_web.validation as validation
from rdsa_web.models import RobotConfig


def _robot(required):
    return RobotConfig(
        id="x",
        display_name="X",
        urdf_package="x_description",
        required_packages=required,
    )


def test_no_missing_when_all_present(monkeypatch):
    monkeypatch.setattr(
        validation, "_package_exists", lambda name: True
    )
    robot = _robot(["a_pkg", "b_pkg"])
    assert validation.get_missing_packages(robot) == []
    assert validation.validate_robot_packages(robot) is True


def test_reports_missing(monkeypatch):
    present = {"a_pkg"}
    monkeypatch.setattr(
        validation, "_package_exists", lambda name: name in present
    )
    robot = _robot(["a_pkg", "b_pkg"])
    assert validation.get_missing_packages(robot) == ["b_pkg"]
    assert validation.validate_robot_packages(robot) is False


def test_empty_required_is_valid(monkeypatch):
    monkeypatch.setattr(validation, "_package_exists", lambda name: False)
    robot = _robot([])
    assert validation.get_missing_packages(robot) == []
    assert validation.validate_robot_packages(robot) is True
