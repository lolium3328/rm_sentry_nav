"""ROS2 controller entry; the legacy controller node remains opt-in."""
from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
from pathlib import Path

def generate_launch_description():
    cfg = Path(get_package_share_directory('mbot_control')) / 'config' / 'mbot_control.yaml'
    controllers = ['joint_state_broadcaster', 'left_wheel_joint_controller', 'right_wheel_joint_controller']
    return LaunchDescription([Node(package='controller_manager', executable='spawner',
        arguments=[name, '--controller-manager', '/mbot/controller_manager'], output='screen') for name in controllers])
