"""Resolve a robot's xacro into URDF XML via the xacro CLI."""
from __future__ import annotations

import shlex
import subprocess
from pathlib import Path

from ament_index_python.packages import (
    PackageNotFoundError,
    get_package_share_directory,
)

from .models import RobotConfig


class UrdfError(Exception):
    """Raised when a robot's URDF cannot be produced."""


def tokenize_xacro_args(args: str) -> list[str]:
    """Split a 'key:=value' arg string, preserving quoted values with spaces."""
    return shlex.split(args)


def resolve_urdf(robot: RobotConfig) -> str:
    try:
        share = Path(get_package_share_directory(robot.urdf_package))
    except PackageNotFoundError as exc:
        raise UrdfError(f"package not found: {robot.urdf_package}") from exc

    xacro_file = share / robot.urdf_path
    if not xacro_file.is_file():
        raise UrdfError(f"xacro file not found: {xacro_file}")

    cmd = ["xacro", str(xacro_file), *tokenize_xacro_args(robot.xacro_args)]
    try:
        proc = subprocess.run(
            cmd, capture_output=True, text=True, timeout=30, stdin=subprocess.DEVNULL
        )
    except subprocess.TimeoutExpired as exc:
        raise UrdfError(f"xacro timed out for {robot.urdf_package}") from exc
    if proc.returncode != 0:
        raise UrdfError(f"xacro failed: {proc.stderr.strip()}")
    return proc.stdout
