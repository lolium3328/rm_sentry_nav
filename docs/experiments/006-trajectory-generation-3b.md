# 实验 006：`trajectory_generation` 阶段 3b 数据接入验证

- 日期：2026-08-15
- 阶段：阶段 3b（参数、地图、`/odom`、`/points`、TF）
- 结果：✅ 通过

## 实施内容

- 新增 `trajectory_generation.yaml`，将 topic、frame 和地图路径参数化。
- 使用 `ament_index_cpp` 解析包 share 目录，消除地图绝对路径依赖。
- 迁移并安装 legacy 的 `occ2024low.png`、`bev2024low.png`、`occtopo2024low.png`。
- 使用 OpenCV 加载三张地图并在节点启动时报告尺寸。
- 使用 `rclcpp::SensorDataQoS()` 订阅 `/odom` 和 `/points`。
- 使用 `tf2_ros::Buffer`/`TransformListener` 定时检查 `mbot/odom` 到
  `mbot/base_footprint` 的 TF。
- 新增 `trajectory_generation.launch.py`。

## Jazzy 容器验证

```bash
docker compose -f docker/docker-compose.yml run --rm sentry
source /opt/ros/jazzy/setup.bash
cd /root/sentry_ws
colcon build --symlink-install --packages-select sentry_msgs trajectory_generation
```

结果：

```text
Finished <<< sentry_msgs
Finished <<< trajectory_generation
Summary: 2 packages finished
```

launch 启动日志确认：

```text
maps loaded: occ=301x161, bev=301x161, distance=301x161
3b data layer ready: odom=/odom, point_cloud=/points,
tf=mbot/odom -> mbot/base_footprint
```

节点检查：

```text
/odom: nav_msgs/msg/Odometry, subscription QoS BEST_EFFORT/VOLATILE
/points: sensor_msgs/msg/PointCloud2, subscription QoS BEST_EFFORT/VOLATILE
```

该 QoS 与阶段二 `ros_gz_bridge` 的传感器数据流约定一致。实验环境本次未同时启动
Gazebo TF 发布端，因此 TF 检查器处于等待状态；TF listener 已正常创建，阶段二的
`mbot/odom -> mbot/base_footprint` 变换名称与配置一致。

## 阶段 3b 结论

阶段 3b 验收通过。后续阶段 3c 接入 FSM、A*、Topo、路径平滑和轨迹消息发布。
