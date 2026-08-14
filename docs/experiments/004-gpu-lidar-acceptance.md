# 实验 004：mbot GPU 雷达端到端验收（阶段 2d）

- 日期：2026-08-15
- 阶段：阶段 2d（gpu_lidar、Gazebo 可视化、ROS 2 bridge、TF）
- 前提：宿主机已构建镜像 `sentry:jazzy`，且可访问桌面 X11。

## 验收目标

确认以下链路完整可用：

```text
RMUC 场地 + mbot + HDL-32E 外形
          ↓
gz-sim gpu_lidar
          ↓
/lidar、/lidar/points（Gazebo Transport）
          ↓
ros_gz_bridge
          ↓
/scan、/points、/odom、/tf（ROS 2）
          ↓
RViz2 中显示点云
```

## 宿主机准备

从宿主机任意终端（例如提示符 `anilam@anilam:~$`）执行：

```bash
cd /home/anilam/rm_sentry_nav
xhost +local:docker
```

首次验收或修改了模型文件时，先增量构建：

```bash
docker compose -f docker/docker-compose.yml run --rm sentry
```

在容器内执行：

```bash
source /opt/ros/jazzy/setup.bash
cd /root/sentry_ws
colcon build --symlink-install --packages-select rmchangdi1230 mbot_description
```

退出容器后，按下列五个终端分别执行。所有容器使用 `network_mode: host`，不需要额外设置 ROS Domain。

## 终端 1：启动带 GUI 的 Gazebo 世界

```bash
cd /home/anilam/rm_sentry_nav
docker compose -f docker/docker-compose.yml run --rm sentry
```

容器内：

```bash
source /opt/ros/jazzy/setup.bash
cd /root/sentry_ws
source install/setup.bash
gz sim -r install/rmchangdi1230/share/rmchangdi1230/worlds/rmuc_static.world
```

预期：Gazebo GUI 打开并显示灰色 RMUC 场地。

> 必须使用 GUI 模式；`gz sim -s` 无头模式无法作为 gpu_lidar 渲染验收。

## 终端 2：生成并插入 mbot

```bash
cd /home/anilam/rm_sentry_nav
docker compose -f docker/docker-compose.yml run --rm sentry
```

容器内：

```bash
source /opt/ros/jazzy/setup.bash
cd /root/sentry_ws
source install/setup.bash

xacro install/mbot_description/share/mbot_description/urdf/mbot.xacro > /tmp/mbot.urdf
gz sdf -p /tmp/mbot.urdf > /tmp/mbot.sdf
ros2 run ros_gz_sim create -world default -file /tmp/mbot.sdf -name mbot -x 2 -y 2 -z 0.3
```

预期：命令输出 `Entity creation successful.`。Gazebo 中出现蓝色 mbot，车顶有 HDL-32E 雷达外壳。

## 终端 3：启动 ROS 2 bridge

```bash
cd /home/anilam/rm_sentry_nav
docker compose -f docker/docker-compose.yml run --rm sentry
```

容器内：

```bash
source /opt/ros/jazzy/setup.bash
cd /root/sentry_ws
source install/setup.bash

ros2 launch ros_gz_bridge ros_gz_bridge.launch.py \
  bridge_name:=sentry_bridge \
  config_file:=$(ros2 pkg prefix mbot_description)/share/mbot_description/config/lidar_bridge.yaml
```

预期日志包含 4 条 `Creating GZ->ROS Bridge`，对应 `/scan`、`/points`、`/tf`、`/odom`。

## 终端 4：发布机器人静态 TF

```bash
cd /home/anilam/rm_sentry_nav
docker compose -f docker/docker-compose.yml run --rm sentry
```

容器内启动 robot state publisher，并保持此终端运行：

```bash
source /opt/ros/jazzy/setup.bash
cd /root/sentry_ws
source install/setup.bash
ros2 launch mbot_description robot_state_publisher.launch.py
```

## 终端 5：检查数据

启动一个同样已 source 的容器终端，运行以下检查：

```bash
ros2 topic hz /scan
ros2 topic hz /points
ros2 topic echo /scan --once
ros2 topic info -v /points
ros2 run tf2_ros tf2_echo base_footprint velodyne_lidar
```

预期：

- `/scan` 频率约 10 Hz；
- `/points` 持续有 `sensor_msgs/msg/PointCloud2` 数据；
- `/scan` 的 `frame_id` 为 `velodyne_lidar`；
- `tf2_echo` 能持续输出 `base_footprint -> velodyne_lidar` 变换。

## RViz2 点云验收

在一个已 source 的容器终端运行：

```bash
rviz2
```

在 RViz2 中：

1. 将 **Fixed Frame** 设为 `base_footprint`；
2. 添加 **PointCloud2** display；
3. Topic 选择 `/points`；
4. 可选添加 **LaserScan** display，Topic 选择 `/scan`。

预期：可看到与场地障碍物、坡道相对应的点云；移动 mbot 后，点云视角随车体变化。

## 通过标准

以下项目全部成立即阶段 2d 验收通过：

- Gazebo 中场地、mbot 与 HDL-32E 外壳均可见；
- Gazebo 雷达可视化可见；
- `/scan` 与 `/points` 均持续发布；
- ROS 2 中消息类型、频率和 `velodyne_lidar` frame 正确；
- RViz2 能显示点云且 TF 无报错。

## 常见问题

| 现象 | 排查方式 |
|---|---|
| GUI 无法打开 | 在宿主机重新执行 `xhost +local:docker`，确认 `DISPLAY` 已导出。 |
| Gazebo 中没有 mbot | 确认终端 1 尚在运行，且终端 2 的 create 命令返回成功。 |
| 车顶没有雷达外壳 | 重新执行增量构建；检查 `install/mbot_description/share/mbot_description/meshes/` 中两个 `HDL32E_*.dae` 文件。 |
| ROS 2 中没有 `/points` | 确认 bridge 日志含 PointCloudPacked 到 PointCloud2 的映射，并确认 mbot 已生成。 |
| RViz2 报 TF 错误 | 确认 `robot_state_publisher.launch.py` 仍在运行，Fixed Frame 使用 `base_footprint`。 |
