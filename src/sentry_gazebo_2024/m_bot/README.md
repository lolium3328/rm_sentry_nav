# mbot 仿真与控制（ROS 2 Jazzy）

本目录对应 `legacy/sentry_planning/src/sentry_gazebo_2024/m_bot/`，保留
`mbot_description`、`mbot_control`、`mbot_gazebo` 和 `mbot_teleop` 四个包的职责边界。

```bash
# 单车 Harmonic 世界、模型、TF、gpu lidar 和 bridge
ros2 launch mbot_gazebo mbot_empty_world.launch.py

# RMUC 多车（mbot 至 mbot5，mbot5 使用红色目标模型）
ros2 launch mbot_gazebo mbot_3d_lidar_gazebo.launch.py

# 选择 ros2_control 后端（默认入口使用 Harmonic DiffDrive）
ros2 launch mbot_gazebo mbot_empty_world.launch.py use_ros2_control:=true

# 控制器和键盘 teleop（在上面的 control backend 启动后）
ros2 launch mbot_control mbot_control.launch.py
ros2 launch mbot_teleop mbot_teleop.launch.py
```

legacy 的 `arbotix`、`turtlebot_teleop` 和外部 `velodyne_description` 在 Jazzy/Harmonic
没有直接可用的等价包；本迁移使用本地 HDL-32E 资源、原生 `gpu_lidar`、`ros_gz_bridge`
和 `rclpy` 入口，并在实验 008 中记录映射与验证边界。
