# 交接文档(截至 2026-08-14)

## 参考计划文档

- **主计划**:`docs/migration-plan.md`(分阶段迁移方案)
- **实验记录**:`docs/experiments/`(001 消息、002 场地、003 mbot)
- **开发规范**:`AGENTS.md`

## 项目概述

哨兵导航 **ROS1(Noetic)→ ROS2(Jazzy)** 迁移。
目标环境:Ubuntu 24.04 + ROS2 Jazzy + Gazebo Harmonic(gz-sim 8.11)。
环境由 Docker 管理,镜像已构建:`sentry:jazzy`。

## 已完成进度

| 阶段 | 内容 | 状态 |
|------|------|------|
| 阶段 0 | Docker 环境(ROS2 Jazzy + OCS2 + Gazebo Harmonic) | ✅ 完成 |
| 阶段 1 | `sentry_msgs` 消息包迁移 | ✅ 完成,已验证 |
| 阶段 2a | 场地世界(RMchangdi1230)启动 | ✅ 完成,已验证 |
| 阶段 2b | mbot 模型加载(6 link + 5 joint) | ✅ 完成,已验证 |
| 阶段 2c | mbot 差速驱动(DiffDrive 插件 + cmd_vel) | ✅ 完成,已验证(发 1m/s 前进 5s,车移动约 4m) |
| 阶段 2d | 雷达(gpu_lidar) | ⚠️ **部分完成,有审查点待修** |

## 阶段 2d 当前状态(雷达)

**已完成**:
- gpu_lidar 传感器已加到 mbot 模型(HDL-32E 规格:32 束、360°、垂直 +10.67°~-30.67°)
- 世界文件已显式声明 4 个系统插件(Physics / UserCommands / SceneBroadcaster / Sensors + ogre2)

**审查点(待修,按优先级)**:

1. **雷达姿态**:`<pose> 0 0 0.21` 原本放在 `<sensor>` 外部(已改为放内部,但**尚未验证生效**);
   - 位置:`src/sentry_gazebo_2024/m_bot/mbot_description/urdf/mbot_base.xacro`
2. **雷达 ROS2 数据链未闭环**:还需验证
   - `/model/mbot/lidar` 话题是否产生数据;
   - 消息类型(gz.msgs.LaserScan / gz.msgs.PointCloudPacked);
   - frame ID;
   - `ros_gz_bridge` 到 ROS2 点云话题(sensor_msgs/PointCloud2)的桥接。
3. **场地材质(低优先级)**:`rmuc_static.world` 场地视觉无显式材质,gz-sim 用亮白默认,
   后续补 diffuse/ambient 让坡道/障碍物轮廓更清楚。

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

## git 状态

- 已提交 14 个 commit(见 `git log`)
- **未提交改动**:
  - `src/.../mbot_base.xacro`(雷达 pose 移入 sensor)
  - `src/.../rmuc_static.world`(系统插件、视觉改回 STL、恢复 GUI 相机)

## 下一步建议

1. 用 GUI 模式验证雷达:`gz sim -r` + `ros_gz_sim create` 生成 mbot,检查 `/model/mbot/lidar` 话题;
2. 配 `ros_gz_bridge` 桥接点云到 ROS2;
3. 提交阶段 2d;
4. 进入阶段 3(`trajectory_generation` 全局规划)。
