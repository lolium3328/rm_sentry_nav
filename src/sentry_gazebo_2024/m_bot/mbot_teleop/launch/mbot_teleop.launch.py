from launch import LaunchDescription
from launch_ros.actions import Node
def generate_launch_description():
    return LaunchDescription([Node(package='mbot_teleop', executable='mbot_teleop.py', name='mbot_teleop', output='screen')])
