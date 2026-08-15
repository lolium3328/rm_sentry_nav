from pathlib import Path
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch.substitutions import Command
from launch_ros.parameter_descriptions import ParameterValue

def generate_launch_description():
    d = Path(get_package_share_directory('mbot_description'))
    world = Path(get_package_share_directory('mbot_gazebo')) / 'worlds' / 'playground.world'
    gz = Path(get_package_share_directory('ros_gz_sim')) / 'launch' / 'gz_sim.launch.py'
    model = d / 'urdf/xacro/gazebo/mbot_gazebo.xacro'
    description = ParameterValue(Command(['xacro ', str(model)]), value_type=str)
    return LaunchDescription([
        IncludeLaunchDescription(PythonLaunchDescriptionSource(str(gz)), launch_arguments={'gz_args': str(world)}.items()),
        Node(package='robot_state_publisher', executable='robot_state_publisher', parameters=[{'robot_description': description}]),
        Node(package='ros_gz_sim', executable='create', arguments=['-name', 'mbot', '-topic', 'robot_description'], output='screen'),
    ])
