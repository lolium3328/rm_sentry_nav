from pathlib import Path
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import Command
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue

# Exact legacy mbot_3d_lidar_gazebo.launch positions.  The field origin is
# occupied by the RMchangdi1230 mesh, so (0, 0) is intentionally not used.
POSES = {'mbot': ('5.0','8.0','0.1'), 'mbot2': ('15.0','8.0','0.1'), 'mbot3': ('15.0','10.0','0.1'), 'mbot4': ('5.0','12.0','1.1'), 'mbot5': ('5.0','6.0','0.1')}

def description_node(name, model, target=False):
    share = Path(get_package_share_directory('mbot_description'))
    file = share / 'urdf/xacro/gazebo' / ('mbot_velodyne_target.xacro' if target else 'mbot_velodyne.xacro')
    desc = ParameterValue(Command(['xacro ', str(file)]), value_type=str)
    x, y, z = POSES[name]
    return [Node(package='robot_state_publisher', executable='robot_state_publisher', namespace=name, name='robot_state_publisher', parameters=[{'robot_description': desc}, {'publish_frequency': 100.0}]), Node(package='ros_gz_sim', executable='create', arguments=['-name', name, '-topic', f'/{name}/robot_description', '-x', x, '-y', y, '-z', z], output='screen')]

def generate_multi_world(world_name='rmuc_static.world'):
    share = Path(get_package_share_directory('mbot_gazebo'))
    field = Path(get_package_share_directory('rmchangdi1230'))
    gz = Path(get_package_share_directory('ros_gz_sim')) / 'launch/gz_sim.launch.py'
    bridge = share / 'config' / 'ros_gz_bridge_mbot1_to_5.yaml'
    world = field / 'worlds' / world_name if world_name == 'rmuc_static.world' else share / 'worlds' / world_name
    actions = [IncludeLaunchDescription(PythonLaunchDescriptionSource(str(gz)), launch_arguments={'gz_args': str(world)}.items())]
    actions.append(Node(package='ros_gz_bridge', executable='parameter_bridge', arguments=['--ros-args', '-p', f'config_file:={bridge}'], output='screen'))
    for name in POSES:
        actions.extend(description_node(name, None, name == 'mbot5'))
    return LaunchDescription(actions)

def generate_single(world_name='rmuc_static.world'):
    share = Path(get_package_share_directory('mbot_gazebo'))
    field = Path(get_package_share_directory('rmchangdi1230'))
    desc_share = Path(get_package_share_directory('mbot_description'))
    gz = Path(get_package_share_directory('ros_gz_sim')) / 'launch/gz_sim.launch.py'
    world = field / 'worlds' / world_name
    model = desc_share / 'urdf/xacro/gazebo/mbot_velodyne.xacro'
    desc = ParameterValue(Command(['xacro ', str(model)]), value_type=str)
    bridge = share / 'config' / 'ros_gz_bridge.yaml'
    return LaunchDescription([
        IncludeLaunchDescription(PythonLaunchDescriptionSource(str(gz)), launch_arguments={'gz_args': str(world)}.items()),
        Node(package='robot_state_publisher', executable='robot_state_publisher', parameters=[{'robot_description': desc}, {'publish_frequency': 50.0}]),
        Node(package='ros_gz_sim', executable='create', arguments=['-name', 'mbot', '-topic', 'robot_description', '-x', '5.0', '-y', '8.0', '-z', '0.1'], output='screen'),
        Node(package='ros_gz_bridge', executable='parameter_bridge', arguments=['--ros-args', '-p', f'config_file:={bridge}'], output='screen'),
    ])
