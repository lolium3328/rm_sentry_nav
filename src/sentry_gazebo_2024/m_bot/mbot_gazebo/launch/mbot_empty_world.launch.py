from pathlib import Path
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch.substitutions import Command
from launch_ros.parameter_descriptions import ParameterValue
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration

def generate_launch_description():
    d = Path(get_package_share_directory('mbot_description'))
    world = Path(get_package_share_directory('rmchangdi1230')) / 'worlds' / 'rmuc_static.world'
    bridge = Path(get_package_share_directory('mbot_gazebo')) / 'config' / 'ros_gz_bridge.yaml'
    gz = Path(get_package_share_directory('ros_gz_sim')) / 'launch' / 'gz_sim.launch.py'
    model = d / 'urdf/xacro/gazebo/mbot_gazebo.xacro'
    use_control = LaunchConfiguration('use_ros2_control')
    description = ParameterValue(Command(['xacro ', str(model), ' use_ros2_control:=', use_control]), value_type=str)
    return LaunchDescription([
        DeclareLaunchArgument('use_ros2_control', default_value='false'),
        IncludeLaunchDescription(PythonLaunchDescriptionSource(str(gz)), launch_arguments={'gz_args': str(world)}.items()),
        Node(package='robot_state_publisher', executable='robot_state_publisher', namespace='mbot', parameters=[{'robot_description': description}]),
        Node(package='ros_gz_sim', executable='create', arguments=['-name', 'mbot', '-topic', '/mbot/robot_description', '-x', '5.0', '-y', '8.0', '-z', '0.1'], output='screen'),
        Node(package='ros_gz_bridge', executable='parameter_bridge', arguments=['--ros-args', '-p', f'config_file:={bridge}'], output='screen'),
    ])
