# 哨兵导航 ROS1(Noetic) → ROS2(Jazzy) 迁移方案

> 目标环境:**Ubuntu 24.04 + ROS 2 Jazzy**(LTS, 支持至 2029)
> 环境管理:**Dockerfile 作为唯一事实源**,所有开发/队友用同一镜像,消除"在我机器上能跑"问题。

---

## 0. 现状盘点

| 模块 | ROS1 现状 | 迁移到 ROS2 Jazzy 后 |
|------|-----------|---------------------|
| `sentry_msgs`(rm2023_sentry_msgs) | catkin + `.msg/.srv` | ament_cmake + 接口文件(几乎可直接复用) |
| `trajectory_generation` | roscpp + A*/Topo 全局规划, OpenCV/PCL/Eigen | rclcpp 改写 |
| `tracking_node`(trajectory_tracking) | roscpp + 内嵌 catkin OCS2(MPC) | rclcpp + **OCS2 官方 `ros2` 分支**(正好面向 Jazzy) |
| `waypoint_generator` | roscpp + tf | rclcpp + tf2 |
| `rviz_plugins` | Qt5 的 rviz 插件 | **Qt6 + rviz2 新插件 API**(工作量最大之一) |
| `sentry_gazebo_2024` | 经典 Gazebo(gazebo_ros) | **Gazebo Harmonic + ros_gz**(经典 Gazebo 已 EOL) |
| `src/ocs2`(内嵌) | catkin 版 | 删除,改用官方 `ros2` 分支(镜像内已编译) |
| Sophus / fmt / spdlog | 源码安装 | Sophus 源码安装、fmt/spdlog 用 apt |

---

## 1. 总体策略

**环境先行,代码分阶段增量迁移。**

1. 先用 Dockerfile 把"干净、可复现"的 ROS2 Jazzy 环境跑起来(本文第 2 节);
2. 从**依赖最少的包**开始逐个迁移,每迁移一个就 `colcon build --packages-select` 验证一个;
3. 仿真与规划解耦:规划模块(纯算法)先迁,仿真(模型/插件)后迁,两者通过 topic 对接。

> 关键决策(已查证):
> - **OCS2**:官方仓库存在 `ros2` 分支,明确面向 Ubuntu 24.04 + Jazzy(colcon/ament),直接替代内嵌 catkin 版。
> - **Gazebo**:经典 Gazebo 于 2025-01 EOL,Jazzy 不再发布 `gazebo_ros`,统一用 **Gazebo Harmonic + `ros_gz`**(本镜像已配好 apt 源与包)。

---

## 2. Docker 环境(先做这一步)

### 2.1 宿主机前置(你的机器是 Ubuntu 22.04 + RTX 5070 Ti,已装 ROS2 Humble)

```bash
# 1) 安装 Docker Engine(未装)
sudo apt-get update
sudo apt-get install -y ca-certificates curl
sudo install -m 0755 -d /etc/apt/keyrings
sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu $(. /etc/os-release && echo $VERSION_CODENAME) stable" \
  | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
sudo apt-get update
sudo apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
sudo usermod -aG docker $USER   # 之后重新登录生效

# 2) 安装 NVIDIA Container Toolkit(GPU 直通 + GUI 加速)
curl -fsSL https://nvidia.github.io/libnvidia-container/gpgkey | sudo gpg --dearmor -o /usr/share/keyrings/nvidia-container-toolkit-keyring.gpg
curl -s -L https://nvidia.github.io/libnvidia-container/stable/deb/nvidia-container-toolkit.list \
  | sed 's#deb https://#deb [signed-by=/usr/share/keyrings/nvidia-container-toolkit-keyring.gpg] https://#g' \
  | sudo tee /etc/apt/sources.list.d/nvidia-container-toolkit.list
sudo apt-get update && sudo apt-get install -y nvidia-container-toolkit
sudo nvidia-ctk runtime configure --runtime=docker && sudo systemctl restart docker
```

### 2.2 构建镜像

```bash
cd /home/anilam/rm_sentry_nav
docker compose -f docker/docker-compose.yml build   # 首次会编译 ROS 包 + OCS2, 较久(约 30~60min)
```

### 2.3 进入开发环境

```bash
# 允许容器访问 X server(每开一个终端执行一次)
xhost +local:docker

# 交互式进入(新 ROS2 工作区挂载到 /root/sentry_ws/src, 原 ROS1 代码挂载到 /root/legacy_ros1)
docker compose -f docker/docker-compose.yml run --rm sentry

# 容器内验证环境
printenv ROS_DISTRO          # jazzy
ros2 pkg list | head         # 能看到 ros-gz / ocs2_* 等
gz sim --versions            # Gazebo Harmonic
```

### 2.4 日常工作流

```bash
# 迁移一个包后在容器内增量编译
cd /root/sentry_ws
colcon build --symlink-install --packages-select sentry_msgs --cmake-args -DCMAKE_BUILD_TYPE=RelWithDebInfo
source install/setup.bash
```

---

## 3. ROS1 → ROS2 技术映射速查表

| 项目 | ROS1(Noetic) | ROS2(Jazzy) |
|------|--------------|-------------|
| 构建系统 | catkin / `catkin_make` | ament_cmake / `colcon build` |
| C++ API | `roscpp`(`ros::NodeHandle`, `Publisher`, `Subscriber`) | `rclcpp`(`rclcpp::Node`, `create_publisher`) |
| Python API | `rospy` | `rclpy` |
| 消息生成 | `add_message_files` + `generate_messages` | 接口文件放 `msg/` 目录, `rosidl_generate_interfaces` |
| launch | `.launch`(XML) | `.launch.py`(Python) |
| 参数 | `<rosparam>` / `ros::param` | `declare_parameter` / YAML |
| TF | `tf`(广播/监听) | `tf2`(`tf2_ros`) |
| 服务 | `rosservice` / `.srv` | `rclcpp::Service` / `.srv` |
| 自定义 rviz 插件 | Qt5 + `rviz::Display` | Qt6 + `rviz_common::Display` |
| Gazebo 接口 | `gazebo_ros` / `gazebo_plugins` | `ros_gz`(`ros_gz_bridge`/`ros_gz_sim`) |
| 控制器 | `gazebo_ros_control` | `gz_ros2_control` |
| 编译并行 | `catkin_make -j12` | `colcon build --parallel-workers 12` |
| 查询包 | `rospack find` | `ros2 pkg prefix` |

---

## 4. 分阶段迁移清单

### 阶段 0:环境(Docker)——已完成 ✅
`docker/Dockerfile` + `docker/docker-compose.yml` + `docker/entrypoint.sh` 已就位, 镜像 `sentry:jazzy` 已构建并验证。

### 阶段 1:`sentry_msgs` —— 已完成 ✅(见 `docs/experiments/001-sentry-msgs-migration.md`)
- `package.xml` → `ament_cmake` 格式;
- `CMakeLists.txt` 用 `rosidl_generate_interfaces` 替换 `add_message_files/generate_messages`;
- `.msg/.srv` 字段基本复用,`Header` 改为 `std_msgs/Header` 限定名;
- ⚠️ 消息文件必须平铺在 `msg/` 下,不能放子目录(实测 `msg/referee_system/` 会导致 `ros2 interface show` 失败);
- `slaver_speed.msg` 按 ROS2 规范改名为 `SlaverSpeed.msg`。

### 阶段 2:仿真 → Gazebo Harmonic(可选但建议先打通,供规划调试)
- `velodyne_simulator` → 用 `ros_gz` 的 `gpu_lidar`/`Lidar` 传感器替换经典 gazebo velodyne 插件;
- `.world` 文件多数可复用(SDF 格式基本兼容),插件标签 `<plugin>` 由 `libgazebo_ros_*` 换成 `ros_gz_*`;
- 控制器由 `gazebo_ros_control` 迁移到 `gz_ros2_control`;
- `joint-state-publisher` / `robot-state-publisher` 用法基本一致,xacro 直接可用。

### 阶段 3:`trajectory_generation`(全局规划,纯算法)
- `roscpp` → `rclcpp`(Node、Publisher/Subscriber、Timer);
- `nav_msgs::Path`、`visualization_msgs` 在 ROS2 接口名不变;
- OpenCV/PCL/Eigen 代码基本不动,只需改 include 与 CMake `find_package`;
- 参数改 `declare_parameter`,地图路径等仍走 YAML。

### 阶段 4:`tracking_node`(OCS2 MPC,最重)
- 删除依赖内嵌 `src/ocs2` 的引用,改为 `find_package(ocs2_core ocs2_mpc ocs2_sqp ...)`;
- 镜像内已按 OCS2 `ros2` 分支编译好,`ament` 导出后可直接 `find_package`;
- 业务代码按 OCS2 ros2 分支自带的示例(如 `ocs2_ros_interfaces` / `ocs2_mpc`)改写;
- 自定义代价/约束/动力学类(`SentryRobotInterface` 等)结构基本沿用,只替换 ROS 接口层。

### 阶段 5:`waypoint_generator` + `rviz_plugins`
- `waypoint_generator`:rclcpp + tf2,较简单;
- `rviz_plugins`(Qt5→Qt6):按 rviz2 官方插件迁移指南改 `rviz_common::Display`/`rviz_common::Tool`,plugin 描述文件与 `pluginlib` 导出规则更新。

---

## 5. 风险与注意事项

1. **rviz_plugins 与 velodyne 插件工作量最大**,建议放到最后,先用现成 rviz2 原生工具(如 `2D Goal Pose`)临时替代。
2. **OCS2 `ros2` 分支仍在演进**,若镜像内 `colcon build` 失败,以 `https://github.com/leggedrobotics/ocs2/tree/ros2/installation.md` 为准微调 Dockerfile 的 ocs2 阶段。
3. **ROS2 默认 QoS** 与 ROS1 不同,订阅/发布可靠性、durability 需显式指定,否则 topic 可能收不到数据(迁移时最常见的坑)。
4. **`ros_gz` 的 bridge** 需要显式写消息映射(在 launch 中配置 `ros_gz_bridge`),不能像经典 gazebo 那样隐式出 topic。
5. 代码里多处硬编码路径(`/media/dragon/EXTERNAL_USB/...`、`/usr/lib/x86_64-linux-gnu/cmake/opencv4`)需清理。

---

## 6. 验证命令(每阶段结束后)

```bash
colcon build --symlink-install --packages-select <pkg>
source install/setup.bash
ros2 pkg list | grep <pkg>          # 包被识别
ros2 launch <pkg> <file>.launch.py  # 启动无报错
ros2 topic list && ros2 topic echo /<topic>  # 数据流通
```
