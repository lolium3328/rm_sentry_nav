"""ROS2/Harmonic equivalent of the legacy Gazebo launcher."""
from pathlib import Path
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node

def generate_launch_description():
    share = Path(get_package_share_directory('rmchangdi1230'))
    gz = Path(get_package_share_directory('ros_gz_sim')) / 'launch' / 'gz_sim.launch.py'
    return LaunchDescription([
        IncludeLaunchDescription(PythonLaunchDescriptionSource(str(gz)),
                                 launch_arguments={'gz_args': str(share / 'world' / 'rmuc.world')}.items()),
        Node(package='ros_gz_sim', executable='create', arguments=['-file', str(share / 'urdf' / 'RMchangdi1230.urdf'), '-name', 'RMchangdi1230'], output='screen'),
    ])
