from pathlib import Path
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import Command
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue

POSES = {'mbot': ('0.0','0.0','0.1'), 'mbot2': ('10.0','10.0','0.1'), 'mbot3': ('15.0','10.0','0.1'), 'mbot4': ('5.0','12.0','1.1'), 'mbot5': ('5.0','6.0','0.1')}

def description_node(name, model, target=False):
    share = Path(get_package_share_directory('mbot_description'))
    file = share / 'urdf/xacro/gazebo' / ('mbot_velodyne_target.xacro' if target else 'mbot_velodyne.xacro')
    desc = ParameterValue(Command(['xacro ', str(file)]), value_type=str)
    x, y, z = POSES[name]
    return [Node(package='robot_state_publisher', executable='robot_state_publisher', namespace=name, name='robot_state_publisher', parameters=[{'robot_description': desc}, {'publish_frequency': 100.0}]), Node(package='ros_gz_sim', executable='create', arguments=['-name', name, '-topic', f'/{name}/robot_description', '-x', x, '-y', y, '-z', z], output='screen')]

def generate_multi_world(world_name='rmuc_std_mbot_HDL32_2.world'):
    share = Path(get_package_share_directory('mbot_gazebo'))
    gz = Path(get_package_share_directory('ros_gz_sim')) / 'launch/gz_sim.launch.py'
    actions = [IncludeLaunchDescription(PythonLaunchDescriptionSource(str(gz)), launch_arguments={'gz_args': str(share / 'worlds' / world_name)}.items())]
    for name in POSES:
        actions.extend(description_node(name, None, name == 'mbot5'))
    return LaunchDescription(actions)
