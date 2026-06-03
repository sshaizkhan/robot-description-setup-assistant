from launch import LaunchDescription

from launch.actions import (
    OpaqueFunction,
    DeclareLaunchArgument,
)
from launch_ros.actions import Node
from launch.substitutions import (
    LaunchConfiguration,
)
from distutils.util import strtobool


def launch_setup(context, *args, **kwargs):
    debug = LaunchConfiguration('debug', default=False).perform(context)

    if strtobool(debug):
        print("Running robot_description_setup_assistant with gdb")
        prefix = ['gdb -ex run --args']
    else:
        print("Running robot_description_setup_assistant")
        prefix = []

    robot_description_setup_assistant = Node(
        package="robot_description_setup_assistant",
        executable="robot_description_setup_assistant",
        output="screen",
        prefix=prefix,
    )

    return [robot_description_setup_assistant]


def generate_launch_description():
    debug = DeclareLaunchArgument(
        'debug',
        default_value='False',
        description='When True, gbd debugger will execute.',
        choices=['True', 'False']
    )

    return LaunchDescription([debug, OpaqueFunction(function=launch_setup)])
