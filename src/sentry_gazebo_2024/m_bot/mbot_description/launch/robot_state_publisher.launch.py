"""Publish the mbot URDF and its fixed-link TF tree."""

from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.substitutions import Command
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    package_share = Path(get_package_share_directory("mbot_description"))
    xacro_file = package_share / "urdf" / "mbot.xacro"

    robot_description = Command(["xacro ", str(xacro_file)])
    return LaunchDescription(
        [
            Node(
                package="robot_state_publisher",
                executable="robot_state_publisher",
                name="robot_state_publisher",
                output="screen",
                parameters=[
                    {"robot_description": ParameterValue(robot_description, value_type=str)}
                ],
            )
        ]
    )
