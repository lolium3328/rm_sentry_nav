# 阶段 3b | 对照 legacy/.../trajectory_generation/launch/global_searcher_sim.launch

from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    package_share = Path(get_package_share_directory("trajectory_generation"))
    parameters = package_share / "config" / "trajectory_generation.yaml"
    return LaunchDescription(
        [
            Node(
                package="trajectory_generation",
                executable="trajectory_generation",
                name="trajectory_generation",
                output="screen",
                parameters=[str(parameters)],
            )
        ]
    )
