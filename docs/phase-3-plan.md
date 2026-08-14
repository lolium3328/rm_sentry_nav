# 阶段三迁移规划：`trajectory_generation`

> 本文是阶段三的详细规划。总阶段顺序以 [`migration-plan.md`](migration-plan.md) 为准。

## 1. 目标与边界

将 `legacy/sentry_planning/src/sentry_planning/trajectory_generation` 迁移为
`src/sentry_planning/trajectory_generation` 下的 ROS2 `ament_cmake` 包，保留 A*、
Topo 搜索、路径平滑、重规划 FSM 和 RViz Marker 可视化能力。

阶段三不迁移 OCS2 MPC、航点生成器或 RViz 自定义插件；优先使用阶段二已验收的
`/odom`、`/points` 和 TF 数据流。`gz_ros2_control` 运动控制闭环另行处理。

## 2. 迁移顺序

### 2.1 接口先行

- 将 legacy 的 `msg/trajectoryPoly.msg` 迁移为
  `src/sentry_msgs/msg/TrajectoryPoly.msg`，字段保持兼容；`time` 使用
  `builtin_interfaces/Time start_time`。
- 在 `sentry_msgs/CMakeLists.txt` 中注册接口，验证：
  `ros2 interface show sentry_msgs/msg/TrajectoryPoly`。
- 规划器统一使用 `sentry_msgs/msg/trajectory_poly.hpp`，不在可执行包内重复生成消息。

### 2.2 创建包骨架

- 创建 `src/sentry_planning/trajectory_generation/`，与 legacy 分组层级保持一致。
- 新建 `package.xml`、`CMakeLists.txt`、`config/`、`launch/`。
- 包名保持 `trajectory_generation`，可执行文件保持 `trajectory_generation`。
- 每个迁移到 `src/` 的文件顶部注明对应的 legacy 原路径。
- 只迁移实际需要的纯算法代码，不复制 legacy 的 `.git` 或无关资源。

### 2.3 ROS2 接口层

- `ros::NodeHandle`、Publisher、Subscriber、Timer 改为 `rclcpp` 对应接口，日志改为
  `RCLCPP_*`。
- `ConstPtr` 回调改为 `std::shared_ptr<const Msg>`，时间使用节点 clock。
- 不迁移 `gazebo_msgs/ModelStates`；Gazebo Harmonic 使用 `/odom`
  (`nav_msgs/msg/Odometry`)。
- 点云默认使用 `/points` (`sensor_msgs/msg/PointCloud2`)，topic 名称必须参数化，
  以兼容真实传感器输入。
- 显式配置 `/odom`、`/points`、waypoint、裁判系统消息和重规划标志的 QoS；传感器流
  使用 sensor-data QoS，状态/控制消息按 reliable 语义验证。

### 2.4 参数、地图和启动文件

- 将 `nh.param(...)` 改为 `declare_parameter` 与类型化读取。
- 默认值集中到 `config/trajectory_generation.yaml`，保留 `trajectory_generator.*`
  参数语义。
- 地图路径不得使用 legacy 绝对路径；通过参数或 `ament_index_cpp` 获取包 share 路径。
- 将 `global_searcher*.launch` 和 `rviz.launch` 改为 Python launch，统一处理参数、
  remapping、`use_sim_time` 和节点启动顺序。

### 2.5 文件拆分

legacy 的 `RM_GridMap.cpp`、`TopoSearch.cpp`、`visualization_utils.cpp` 较大，迁移时
按“地图/点云回调”“搜索算法”“Marker 发布”“FSM/状态管理”拆分，单文件不得超过
500 行。纯算法类使用 Eigen/标准容器，ROS2 订阅、参数与发布集中在适配层。

## 3. 分批实施与验证

| 批次 | 内容 | 验证 |
|------|------|------|
| 3a | 接口、包骨架、纯算法和最小节点 | `colcon build --packages-select sentry_msgs trajectory_generation` |
| 3b | `/odom`、`/points`、TF、参数和地图加载 | `ros2 topic echo`、`tf2_echo`、路径检查 |
| 3c | FSM、A*、Topo、平滑和轨迹消息发布 | RViz2 显示路径/Marker，目标点可触发规划 |
| 3d | Gazebo Harmonic 端到端回归 | 场地、mbot、gpu_lidar 启动后持续输出轨迹 |

## 4. 完成标准与记录

- `colcon build --symlink-install --packages-select sentry_msgs trajectory_generation` 通过。
- `ros2 launch trajectory_generation <file>.launch.py` 无 ROS1/Gazebo Classic 依赖报错。
- 规划器能接收 `/points`、`/odom`、TF，且 QoS 匹配。
- 有效目标点能触发规划并发布 `sentry_msgs/msg/TrajectoryPoly`。
- RViz2 能显示搜索路径、平滑路径和规划 Marker。
- 将结果记录到 `docs/experiments/005-trajectory-generation-migration.md`。
- 代码改动按逻辑拆分 commit，并使用 `feat:`、`fix:`、`refactor:` 或 `docs:` 前缀。
