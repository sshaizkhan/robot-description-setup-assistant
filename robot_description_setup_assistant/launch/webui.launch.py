"""Launch the RDSA web UI backend (uvicorn serving FastAPI)."""
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    share = get_package_share_directory("robot_description_setup_assistant")
    # Canonical multi-file catalog directory (falls back to robots.yaml if absent).
    catalog_dir = os.path.join(share, "config", "catalog")
    robots_yaml = catalog_dir if os.path.isdir(catalog_dir) else os.path.join(
        share, "config", "robots.yaml"
    )

    host = LaunchConfiguration("host")
    port = LaunchConfiguration("port")

    return LaunchDescription([
        DeclareLaunchArgument("host", default_value="127.0.0.1"),
        DeclareLaunchArgument("port", default_value="8000"),
        # C++ catalog service node — the source of truth for the catalog.
        Node(
            package="robot_catalog_server",
            executable="robot_catalog_server",
            name="robot_catalog_server",
            output="screen",
        ),
        ExecuteProcess(
            cmd=[
                "uvicorn", "rdsa_web.app:app",
                "--host", host,
                "--port", port,
            ],
            additional_env={
                "RDSA_ROBOTS_YAML": robots_yaml,
                "RDSA_CATALOG_BACKEND": "cpp",
                **(
                    {"RDSA_FRONTEND_DIST": os.environ["RDSA_FRONTEND_DIST"]}
                    if os.environ.get("RDSA_FRONTEND_DIST")
                    else {}
                ),
            },
            output="screen",
        ),
    ])
