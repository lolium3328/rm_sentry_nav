# 实验 008：阶段二保真修补记录

- 日期：2026-08-15
- 依据：`docs/phase-2-remediation-plan.md`
- 状态：容器构建、单车/五车生成和控制器 smoke 已通过；完整 GUI/RViz 与持续数据验收仍待补齐

## 提交批次

| 批次 | 提交 | 结果 |
|---|---|---|
| 2R-1 场地结构 | `123a947` | 已提交 |
| 2R-2 mbot description | `54f6e88` | 已提交 |
| 2R-3 mbot control | `4615dbf` | 已提交 |
| 2R-4 mbot gazebo | `fb5645f` | 已提交 |
| 2R-5 mbot teleop | `6198bd8` | 已提交 |

## legacy/src 映射

- `RMchangdi1230` 的 `config/`、`launch/`、`meshes/`、`urdf/`、`world/` 均已迁移；
  `export.log` 是 CAD 导出日志，不参与构建。
- `mbot_description` 的显示 xacro、Gazebo xacro、传感器、网格、RViz 和配置均已恢复；
  ROS2 launch 使用 `.launch.py` 后缀。
- `mbot_control`、`mbot_gazebo`、`mbot_teleop` 的同名 ROS1 launch 由 `.launch.py` 等价入口
  映射。`arbotix`、`turtlebot_teleop` 和外部 velodyne 插件没有 Jazzy/Harmonic 直接包，
  已使用本地 HDL-32E、原生 gpu_lidar、ros_gz_bridge 和 rclpy 适配。

## 静态证据

已在宿主工作区完成：

```text
python3 -m py_compile 通过（所有 ROS2 Python 入口）
xmllint --noout 通过（package.xml、URDF、xacro）
Python XML 解析通过（RMchangdi1230 两个 world、mbot 六个 world）
Classic Gazebo/ROS1 API 搜索：无插件或 roscpp/rospy 残留
```

场地三个 STL 与 legacy SHA-256 一致：
`base_link.STL=87cde692fd24481381ecee5e26dfdb46fef75da051a5a873016f85186f4bd36b`。

## 初始位姿核对

`legacy/.../mbot_3d_lidar_gazebo.launch` 的实际生成点为：

| 模型 | x | y | z |
|---|---:|---:|---:|
| mbot | 5.0 | 8.0 | 0.1 |
| mbot2 | 15.0 | 8.0 | 0.1 |
| mbot3 | 15.0 | 10.0 | 0.1 |
| mbot4 | 5.0 | 12.0 | 1.1 |
| mbot5 | 5.0 | 6.0 | 0.1 |

修补前 ROS2 多车入口错误地把 `mbot` 放在 `(0, 0, 0.1)`，而场地模型原点处
已有障碍几何，截图中的重叠由此造成；`mbot2` 也不是 legacy 的 `(15, 8)`。
现已恢复上表，单车入口同样使用 legacy 首车点 `(5, 8, 0.1)`。`rmuc.world` 和
`rmuc_static.world` 的场地 mesh URI 也已从失效的 `../../RMchangdi1230/...` 修正为
相对当前 `world/` 目录有效的 `../meshes/...`。

## 容器验证

计划命令：

```bash
docker compose -f docker/docker-compose.yml run --rm sentry bash -lc \
  'cd /root/sentry_ws && source /opt/ros/jazzy/setup.bash && \
   colcon build --symlink-install --packages-select \
   rmchangdi1230 mbot_description mbot_control mbot_gazebo mbot_teleop'
```

本次执行在宿主机被 Docker daemon 权限拒绝：
`permission denied while trying to connect to the Docker API at unix:///var/run/docker.sock`。
随后通过 `sg docker -c ...` 刷新 `docker` 补充组后已恢复验证，结果如下：

- 五个包构建成功：`5 packages finished`。
- 单车正式入口启动成功，`ros_gz_sim create` 返回 `Entity creation successful`；
  bridge 创建 `/scan`、`/points`、`/imu/data`、`/odom` 和 `/cmd_vel` 映射。
- 五车入口启动成功，五个 create 均返回 `Entity creation successful`；五个命名空间
  的 scan、points、imu、odom、cmd_vel bridge 均创建。
- Docker 内 `gz model -m ... -p` 实测位姿与上表完全一致：
  `mbot=(5,8,0.1)`、`mbot2=(15,8,0.1)`、`mbot3=(15,10,0.1)`、
  `mbot4=(5,12,1.1)`、`mbot5=(5,6,0.1)`。
- `use_ros2_control:=true` 后 `gz_ros2_control` 成功初始化硬件；两个轮速控制器和
  `joint_state_broadcaster` 均输出 `Configured and activated`。默认路径不加载该插件，
  使用 Harmonic DiffDrive，两个后端不会同时抢占轮关节。
- `gz sdf -k` 对场地 world 返回 `Valid.`。
- 运行时曾发现 DiffDrive plugin name 与 Harmonic 注册名不一致，已改为
  `gz::sim::systems::DiffDrive` 并重新 smoke test；修复后无该插件加载错误。

仍未将阶段二标为最终完成：当前 smoke test 尚未替代完整 GUI/RViz、持续传感器频率、
cmd_vel/odom 位移和 teleop 键位验收；这些是下一步必须在同一 Docker 容器内补齐的证据。

## 尚待补齐的验收清单

1. 容器构建五个包并用 `ros2 pkg prefix` 逐一确认。
2. 展开所有 xacro，核对 6 link、5 joint、质量/惯量、wheel geometry、HDL-32E 32 束、
   IMU 1000 Hz 与零噪声配置。
3. 启动 `mbot_empty_world.launch.py`，验证 cmd_vel、odom、TF、`/scan`、`/points`、
   `/imu/data` 和 RViz2。
4. 启动 `mbot_3d_lidar_gazebo.launch.py`，验证 mbot～mbot5 位姿、命名空间、红色目标车
   和控制器不串线。
5. 启动 teleop，验证原键位、加减速、平滑停止和
   `left=50*speed+3*turn`、`right=50*speed-3*turn`。
6. 重跑实验 002、004，并启动 `trajectory_generation` 做 `/odom`、`/points`、TF 回归。
