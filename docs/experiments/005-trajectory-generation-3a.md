# 实验 005：`trajectory_generation` 阶段 3a 骨架验证

- 日期：2026-08-15
- 阶段：阶段 3a（接口、包骨架、纯算法数据结构、最小节点）
- 结果：✅ 通过

## 实施内容

- 在 `sentry_msgs` 中加入 `TrajectoryPoly.msg`，将 ROS1 `time` 映射为
  `builtin_interfaces/Time`，并注册到 `rosidl_generate_interfaces`。
- 创建 `src/sentry_planning/trajectory_generation` ROS2 `ament_cmake` 包。
- 创建不依赖 ROS1 的 `GridNode` 纯 Eigen 数据结构。
- 创建 `trajectory_generation` 最小 `rclcpp` 节点。
- 保留每个迁移文件的 legacy 对照路径；未复制 legacy 的 `.git` 或业务大文件。

## 容器验证

宿主机 Docker 服务正常，但当前 shell 未刷新 `docker` supplementary group；使用
临时 `sg docker -c` 运行项目规定的 Jazzy 容器命令：

```bash
sg docker -c 'docker compose -f docker/docker-compose.yml run --rm sentry bash -lc \
  "source /opt/ros/jazzy/setup.bash && cd /root/sentry_ws && \
   colcon build --symlink-install --packages-select sentry_msgs trajectory_generation"'
```

结果：

```text
Finished <<< sentry_msgs
Finished <<< trajectory_generation
Summary: 2 packages finished
```

接口检查：

```bash
ros2 interface show sentry_msgs/msg/TrajectoryPoly
```

结果包含 `builtin_interfaces/Time start_time`、`motion_mode`、`coef_x`、`coef_y` 和
`duration` 字段。

包与节点检查：

```bash
ros2 pkg prefix trajectory_generation
timeout 2s ros2 run trajectory_generation trajectory_generation
```

结果：包前缀为 `/root/sentry_ws/install/trajectory_generation`，节点输出
`trajectory_generation 3a skeleton is ready`，并能正常响应终止信号。

## 阶段 3a 结论

阶段 3a 验收通过。后续阶段 3b 再接入 `/odom`、`/points`、TF、参数和地图加载；阶段
3c 再迁移 FSM、A*、Topo、路径平滑和轨迹发布逻辑。
