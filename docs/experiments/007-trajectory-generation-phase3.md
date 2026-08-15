# 实验 007：`trajectory_generation` 阶段三统一验收

> **后续保真审查结论（2026-08-15）：** 本实验只证明当时的简化实现能够编译并跑通
> Gazebo 数据链路，不能证明 legacy 规划逻辑已迁移。阶段三保真验收不通过，状态改为
> 待按原文件名和原逻辑重新迁移。详见 [`../legacy-fidelity-audit.md`](../legacy-fidelity-audit.md)。

- 日期：2026-08-15
- 阶段：阶段三（3a–3d）
- 原运行链路结果：✅ 通过
- legacy 保真迁移结果：❌ 未通过

## 迁移内容

- `TrajectoryPoly` ROS2 接口及 legacy 四阶系数布局。
- 参数、地图加载、`/odom`、`/points`、TF 和传感器 QoS。
- 基于 legacy 地图坐标约定的 2D A*：像素值大于 10 视为障碍，按机器人半径膨胀，保留图像 Y 轴翻转。
- 路径可见性平滑，对应 legacy `smoothTopoPath` 的路径简化职责。
- 参考轨迹时间分配和自然三次样条系数，输出顺序保持 `[三次、二次、一次、常数]`。
- ROS2 FSM：`WAIT_ODOMETRY`、`WAIT_TARGET`、`GEN_NEW_TRAJ`、`EXEC_TRAJ`，支持 `/replan_flag`。
- Path、Marker 和 `sentry_msgs/msg/TrajectoryPoly` 发布。

## Jazzy 容器编译

```bash
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

## Gazebo Harmonic 端到端验收

使用 `docker/start_lidar_demo.sh` 启动场地、mbot、gpu_lidar、bridge、
`robot_state_publisher` 和 RViz2，再在 host network 容器启动：

```bash
ros2 launch trajectory_generation trajectory_generation.launch.py
```

实测结果：

| 验收项 | 结果 |
|---|---|
| Gazebo 场地与 mbot 创建 | ✅ `Entity creation successful` |
| `/odom` | ✅ `nav_msgs/msg/Odometry`，frame=`mbot/odom`，child=`mbot/base_footprint` |
| `/points` | ✅ `sensor_msgs/msg/PointCloud2`，frame=`velodyne_lidar`，`1800×32` |
| TF | ✅ `mbot/odom → mbot/base_footprint` 可查询 |
| 地图加载 | ✅ 三张地图均为 `301×161` |
| FSM 目标触发 | ✅ waypoint 后进入 `GEN_NEW_TRAJ` |
| 规划结果 | ✅ `A* planned 6 points, smoothed to 2 points` |
| 轨迹接口 | ✅ 发布 `global_trajectory`，系数按四阶消息布局输出 |

## 坐标偏移处理

Gazebo demo 将 mbot 生成在世界坐标 `(2, 2)`，但 `/odom` 以出生点为原点。因此新增：

```yaml
trajectory_generator:
  map_offset_x: 2.0
  map_offset_y: 2.0
```

真实机器人或其他出生点通过 YAML 覆盖，不修改 legacy 地图坐标逻辑。

## 阶段三结论

当时实现的 ROS2 规划数据链路、规划触发、路径生成、轨迹输出和 Gazebo Harmonic
端到端回归通过，但该实现未保留 legacy 的完整算法和文件结构，因此不得据此进入阶段四。
