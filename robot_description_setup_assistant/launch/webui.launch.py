"""Launch the RDSA web UI backend (uvicorn serving FastAPI)."""
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    share = get_package_share_directory("robot_description_setup_assistant")
    robots_yaml = os.path.join(share, "config", "robots.yaml")

    host = LaunchConfiguration("host")
    port = LaunchConfiguration("port")

    return LaunchDescription([
        DeclareLaunchArgument("host", default_value="127.0.0.1"),
        DeclareLaunchArgument("port", default_value="8000"),
        ExecuteProcess(
            cmd=[
                "uvicorn", "rdsa_web.app:app",
                "--host", host,
                "--port", port,
            ],
            additional_env={
                "RDSA_ROBOTS_YAML": robots_yaml,
                **(
                    {"RDSA_FRONTEND_DIST": os.environ["RDSA_FRONTEND_DIST"]}
                    if os.environ.get("RDSA_FRONTEND_DIST")
                    else {}
                ),
            },
            output="screen",
        ),
    ])
