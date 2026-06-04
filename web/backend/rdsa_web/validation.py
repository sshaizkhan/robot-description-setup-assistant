"""Robot package validation via ament_index_python."""
from __future__ import annotations

from ament_index_python.packages import (
    PackageNotFoundError,
    get_package_share_directory,
)

from .models import RobotConfig


def _package_exists(name: str) -> bool:
    try:
        get_package_share_directory(name)
        return True
    except PackageNotFoundError:
        return False


def get_missing_packages(robot: RobotConfig) -> list[str]:
    return [p for p in robot.required_packages if not _package_exists(p)]


def validate_robot_packages(robot: RobotConfig) -> bool:
    return not get_missing_packages(robot)
