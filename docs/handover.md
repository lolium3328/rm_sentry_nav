# 交接文档(截至 2026-08-14)

## 参考计划文档

- **主计划**:`docs/migration-plan.md`(分阶段迁移方案)
- **实验记录**:`docs/experiments/`(001 消息、002 场地、003 mbot)
- **开发规范**:`AGENTS.md`

## 项目概述

哨兵导航 **ROS1(Noetic)→ ROS2(Jazzy)** 迁移。
目标环境:Ubuntu 24.04 + ROS2 Jazzy + Gazebo Harmonic(gz-sim 8.11)。
环境由 Docker 管理,镜像已构建:`sentry:jazzy`。

## 当前进度（阶段二保真修补）

| 阶段 | 内容 | 状态 |
|------|------|------|
| 阶段 0 | Docker 环境(ROS2 Jazzy + OCS2 + Gazebo Harmonic) | ✅ 完成 |
| 阶段 1 | `sentry_msgs` 消息包迁移 | ✅ 完成,已验证 |
| 阶段 2R-1 | 恢复 RMchangdi1230 legacy 目录与资源 | ✅ 已提交，待容器复验 |
| 阶段 2R-2 | 恢复 mbot description 文件与 xacro 边界 | ✅ 已提交，待容器复验 |
| 阶段 2R-3 | mbot_control ROS2 控制器迁移 | ✅ 已提交，待容器复验 |
| 阶段 2R-4 | mbot_gazebo Harmonic 编排与五车入口 | ✅ 已提交，待容器复验 |
| 阶段 2R-5 | mbot_teleop rclpy 迁移 | ✅ 已提交，待容器复验 |
| 阶段 2R-6 | 统一回归验收 | ⏳ Docker 已恢复；build、单车/五车 smoke 通过，完整传感器/控制/RViz 仍待验收 |

## 阶段 2d 当前状态(雷达)

**已完成**:
- gpu_lidar 传感器已加到 mbot 模型(HDL-32E 规格:32 束、360°、垂直 +10.67°~-30.67°)
- 世界文件已显式声明 4 个系统插件(Physics / UserCommands / SceneBroadcaster / Sensors + ogre2)
- Gazebo `/lidar`、`/lidar/points` 和 ROS2 `/scan`、`/points` 已闭环，频率约 10 Hz
- `base_footprint → velodyne_lidar` TF、`/odom` 和机器人运动联动已通过验收
- RViz2 配置已安装：`config/rviz/mbot_lidar.rviz`，默认显示红色 `/points`

历史验收记录：`docs/experiments/004-gpu-lidar-acceptance.md`；本次修补记录：
`docs/experiments/008-phase-2-fidelity-remediation.md`。正式入口见：
`docker/start_lidar_demo.sh`。

**已知非阻塞提示**:

1. `gz_frame_id` 在 SDF schema 中产生兼容性 warning，但运行时 frame 正确为 `velodyne_lidar`。
2. EGL/Dri2 warning 不影响本次点云发布；如需优化渲染性能再单独处理。

## 已验证的关键事实(避免重复踩坑)

1. **场地显示正常**:GUI 里的灰色方格是世界参考网格,不是场地;场地 STL 范围 x=-0.989~28.989、
   y=-0.5~15.5,世界原点在场地一角(与 legacy 一致,不是错位)。场地"白成一片"是缺显式材质,不是法线问题。
2. **STL 碰撞正常**:dartsim + STL 碰撞能正常创建(那条 "Mesh construction not implemented" 是误导日志)。
3. **无头模式(-s)渲染卡死**:gpu_lidar 依赖 ogre2,无头模式渲染线程初始化后卡住;
   **必须用 GUI 模式**(需宿主机 `xhost +` 放行 X 授权)。
4. **gz sim 输出缓冲**:`timeout ... > file` 会丢日志,用 `| grep`/`| head` 管道。
5. **sdformat 固定关节合并**:base_link 会被合并进 base_footprint,传感器挂在 base_footprint 上,
   雷达高度 pose 要按绝对高度 0.21(0.13 + 0.08)写。

## 目录结构

```
src/
├── sentry_gazebo_2024/        # 仿真(对应 legacy)
│   ├── rmchangdi1230/         # 场地
│   └── m_bot/mbot_description/ # mbot 模型
├── sentry_planning/           # 规划(待迁移: trajectory_generation 等)
└── sentry_msgs/               # 消息
```

## 环境用法

```bash
# 宿主机需先放行 X(每开新终端一次)
xhost +

# 进入容器
docker compose -f docker/docker-compose.yml run --rm sentry

# 容器内编译
cd /root/sentry_ws && colcon build --symlink-install
```

## 本次修补提交

- `123a947` 场地结构
- `54f6e88` mbot description 边界
- `4615dbf` mbot_control
- `fb5645f` mbot_gazebo
- `6198bd8` mbot_teleop

## 下一步建议

1. 使用 `docker/start_lidar_demo.sh` 重跑阶段 2d 验收（如需复核）;
2. 进入阶段 3(`trajectory_generation` 全局规划)。
