# 阶段二保真修补计划

> 本文是阶段二的独立修补计划，不修改 `docs/migration-plan.md`。修补依据为
> [`../AGENTS.md`](../AGENTS.md) 的保真迁移规则和
> [`legacy-fidelity-audit.md`](legacy-fidelity-audit.md) 的审查结论。

- 目标平台：ROS2 Jazzy + Gazebo Harmonic
- legacy 基线：`legacy/sentry_planning/src/sentry_gazebo_2024/`
- 修补范围：`RMchangdi1230` 和 `m_bot` 下的四个包
- 当前状态：单车场地、运动、雷达和 RViz2 演示链路可用，但阶段二保真迁移未完成

## 1. 修补目标

1. 保留已经验证的场地 STL、mbot 机械参数、gpu lidar、TF、odom 和 RViz2 数据链路。
2. 恢复 legacy 的文件名、相对目录、xacro 宏边界、包边界和 launch 职责。
3. 将 Classic Gazebo、roscpp、rospy 和 ros_control API 替换为 Jazzy/Harmonic 等价接口，
   不改变机械参数、控制公式、命名空间、生成位置、传感器规格和 topic 语义。
4. 补齐当前缺失的 `mbot_control`、`mbot_gazebo`、`mbot_teleop` 三个 ROS2 包。
5. 完成逐文件映射、容器编译、单车/多车控制、雷达、IMU、TF 和 RViz2 验收后，
   再将阶段二标记为完成。

## 2. 执行原则

- 功能实现保留 legacy 原文件名和相对层级；launch 仅按本项目 ROS2 规范增加 `.launch.py` 后缀。
- 不再把显示模型、仿真模型、驱动、雷达和 IMU 合并到一个 xacro。
- 原 world/xacro 超过 500 行时保持原结构，不为了行数拆分。
- 新增 bridge、Harmonic 传感器或兼容入口时明确标记为 ROS2 适配文件，不替代 legacy 模块。
- Classic Gazebo 插件必须在原职责位置做等价替换，不得以“能运动”为理由删除控制分支。
- legacy 自身存在的失效引用和代码缺陷先登记；修复会改变 legacy 行为时，实施前请求批准。
- 所有编译和运行验收都在项目 Docker 容器内执行。

## 3. 修补批次

| 批次 | 内容 | 主要产物 |
|---|---|---|
| 2R-0 | 冻结基线和建立映射 | legacy/src 文件矩阵、参数/topic 对照表 |
| 2R-1 | 修补场地包 | 完整 `rmchangdi1230` 目录、两个 world、launch 和 URDF |
| 2R-2 | 恢复 mbot 模型边界 | 原 xacro、IMU、HDL-32E、显示 launch 和 RViz 配置 |
| 2R-3 | 迁移控制包 | `mbot_control`、ROS2 controller 配置、轮速控制验证 |
| 2R-4 | 迁移 Gazebo 编排 | `mbot_gazebo`、六个 world、四个 launch、多车和 bridge |
| 2R-5 | 迁移 teleop | 原键位、加减速和左右轮计算逻辑的 rclpy 版本 |
| 2R-6 | 统一回归验收 | 单车、多车、cmd_vel、轮速、雷达、IMU、TF、RViz2 |

## 4. 文件改动计划

### 4.1 `rmchangdi1230`

目标目录：`src/sentry_gazebo_2024/rmchangdi1230/`。ROS2 包名按项目规范保持小写，
包内文件名和相对层级与 legacy 保持一致。

现有文件处理：

| 文件 | 操作 |
|---|---|
| `CMakeLists.txt` | 修改；安装 `config/`、`launch/`、`meshes/`、`urdf/`、`world/` |
| `package.xml` | 修改；补齐 launch、robot_state_publisher、rviz2、ros_gz_sim 等运行依赖 |
| `meshes/base_link.STL` | 原样保留；继续校验与 legacy SHA-256 一致 |
| `worlds/rmuc_static.world` | 先保留作兼容入口；主实现恢复到 `world/rmuc_static.world` 后，是否删除需另行批准 |

从 legacy 新增并适配：

- `config/joint_names_RMchangdi1230.yaml`
- `launch/display.launch.py`，对应 `launch/display.launch`
- `launch/gazebo.launch.py`，对应 `launch/gazebo.launch`
- `meshes/base_link2022.STL`
- `meshes/base_link2025.STL`
- `urdf/RMchangdi1230.csv`
- `urdf/RMchangdi1230.urdf`
- `urdf/RMchangdi12302025.urdf`
- `world/rmuc.world`
- `world/rmuc_static.world`

必要适配：

- world 保留模型、碰撞、摩擦、光照和相机参数，仅替换 Harmonic 系统插件和资源 URI。
- `RMchangdi1230.urdf` 中失效的旧包 URI改为当前包 URI，不改变 mesh、origin 和 inertia。
- 为 display launch 新增 `config/rviz/rmchangdi1230.rviz`；legacy 引用的 `urdf.rviz`
  不在仓库中，新增配置需在验收记录中说明。
- `export.log` 是 CAD 导出日志，不参与构建和运行，默认不迁移；在映射表中保留该理由。

### 4.2 `mbot_description`

目标目录：`src/sentry_gazebo_2024/m_bot/mbot_description/`。

现有文件处理：

| 文件 | 操作 |
|---|---|
| `CMakeLists.txt` | 修改；安装原 `config/launch/meshes/sensors/urdf` 层级和新增适配资源 |
| `package.xml` | 修改；补齐 xacro、joint state publisher、rviz2、ros_gz 和控制后端依赖 |
| `urdf/mbot.xacro` | 暂留作兼容入口；不得继续承载主实现 |
| `urdf/mbot_base.xacro` | 暂留作兼容入口；主实现恢复后是否删除需另行批准 |
| `config/lidar_bridge.yaml` | 修改；增加 IMU 和 ROS→Gazebo cmd_vel 映射，或由 `mbot_gazebo` 配置引用 |
| `launch/robot_state_publisher.launch.py` | 保留为 ROS2 辅助入口，改为引用恢复后的原路径 xacro |
| `config/rviz/mbot_lidar.rviz` | 保留为新增 ROS2 雷达验收配置 |
| `meshes/HDL32E_base.dae` | 保留；作为 ROS1 外部 `velodyne_description` 的本地适配资源 |
| `meshes/HDL32E_scan.dae` | 保留；作为 ROS1 外部 `velodyne_description` 的本地适配资源 |

从 legacy 新增并适配：

- `config/fake_mbot_arbotix.yaml`
- `config/rviz/mbot.rviz`
- `config/rviz/mbot_arbotix.rviz`
- `config/rviz/mbot_urdf.rviz`
- `launch/arbotix_mbot_xacro.launch.py`
- `launch/display_mbot_with_camera_xacro.launch.py`
- `launch/display_mbot_xacro.launch.py`
- `meshes/kinect.dae`
- `sensors/imu.xacro`
- `urdf/xacro/mbot.xacro`
- `urdf/xacro/mbot_base.xacro`
- `urdf/xacro/gazebo/mbot_base_gazebo.xacro`
- `urdf/xacro/gazebo/mbot_gazebo.xacro`
- `urdf/xacro/gazebo/mbot_velodyne.xacro`
- `urdf/xacro/gazebo/mbot_velodyne_target.xacro`

新增 ROS2 适配文件：

- `sensors/hdl32e.xacro`：封装当前已验证的 HDL-32E 外形和 Harmonic gpu lidar；由原
  `mbot_velodyne*.xacro` 引用，避免继续把雷达写入 `mbot_base_gazebo.xacro`。
- `config/rviz/mbot_lidar.rviz` 已存在，继续作为新增验收资源，不冒充 legacy RViz 配置。

xacro 职责恢复：

- `mbot_base.xacro` 只保留 legacy 显示模型的 6 link/5 joint 结构和原尺寸。
- `mbot_base_gazebo.xacro` 保留仿真碰撞、惯量、颜色参数、左右轮控制接口和差速驱动。
- `mbot_gazebo.xacro` 继续作为基础仿真模型入口。
- `mbot_velodyne.xacro` 和 `mbot_velodyne_target.xacro` 分别保留蓝车/红车、HDL-32E、
  IMU 和控制插件入口。
- `imu.xacro` 保留原 link、joint、质量、尺寸、1000 Hz 和零噪声配置，仅把
  `libgazebo_ros_imu_sensor.so` 替换为 Harmonic 原生 IMU 和 bridge。

### 4.3 `mbot_control`

新增目标目录：`src/sentry_gazebo_2024/m_bot/mbot_control/`。

从 legacy 新增并迁移：

- `CMakeLists.txt`
- `package.xml`
- `config/mbot_control.yaml`
- `launch/mbot_control.launch.py`，对应 `launch/mbot_control.launch`
- `src/mbot_controller.cpp`

控制迁移要求：

- `mbot_controller.cpp` 保留节点名、1000 Hz 循环、左右轮固定 `10.0` 速度和原 topic
  职责，只替换 roscpp/std_msgs API；不借机重写成 cmd_vel 控制器。
- `mbot_control.yaml` 保留 `mbot` 至 `mbot5` 五组命名空间、左右轮控制器、1000 Hz
  joint state 发布频率和 PID 数值。
- 实施时在容器内查询 Jazzy 已安装 controller 类型，再选择能保持轮速 topic 和 PID
  语义的 ros2_control 控制器；不能等价时先记录并请求批准。
- `mbot_base_gazebo.xacro` 中的 ROS1 transmission/gazebo_ros_control 改为
  `<ros2_control>` 和 `gz_ros2_control`，左右轮 joint/interface 名称保持不变。
- 保留原 launch 中 `mbot` 至 `mbot5` 的 controller spawner；原本被注释的
  `mbot_controller` 节点继续保持默认不启动。

### 4.4 `mbot_gazebo`

新增目标目录：`src/sentry_gazebo_2024/m_bot/mbot_gazebo/`。

从 legacy 新增并适配：

- `CMakeLists.txt`
- `package.xml`
- `launch/mbot.launch.py`
- `launch/mbot_3d_lidar_gazebo.launch.py`
- `launch/mbot_empty_world.launch.py`
- `launch/mbot_rmuc_lidar_gazebo.launch.py`
- `worlds/playground.world`
- `worlds/rmuc_mbot_HDL32.world`
- `worlds/rmuc_mbot_VLP16.world`
- `worlds/rmuc_std_mbot_HDL32.world`
- `worlds/rmuc_std_mbot_HDL32_2.world`
- `worlds/room.world`

新增 ROS2 适配配置：

- `config/ros_gz_bridge.yaml`：集中保存 `/scan`、`/points`、`/imu/data`、`/tf`、
  `/odom` 和 ROS→Gazebo `/cmd_vel` 映射。
- 如多车 bridge 无法通过一个 YAML 参数化，则按原命名空间新增
  `config/ros_gz_bridge_mbot1_to_5.yaml`，不得只支持 `mbot` 后忽略 `mbot2`～`mbot5`。

launch/world 迁移要求：

- 四个 launch 保留原文件职责、参数、默认模型、生成位置、机器人名称和命名空间。
- `mbot_3d_lidar_gazebo.launch.py` 保留五车生成位置以及 `mbot5` 红色目标车模型。
- Classic Gazebo 启动和 spawn_model 替换为 `ros_gz_sim` 等价动作。
- 六个 world 保留模型、pose、物理参数、场地和传感器配置，仅替换不兼容插件与 URI。
- 原 `libgazebo_ros_diff_drive.so` 在 `mbot_base_gazebo.xacro` 原位置替换为 Harmonic
  `DiffDrive`，优先保持 100 Hz、wheel separation/radius、cmd_vel、odom 和 TF 语义。
- 同时保留可选 `gz_ros2_control` 轮速控制路径；如两个后端会争用 wheel joint，必须通过
  launch 选择后端并记录与 legacy 的差异，不能静默删除其中之一。

### 4.5 `mbot_teleop`

新增目标目录：`src/sentry_gazebo_2024/m_bot/mbot_teleop/`。

从 legacy 新增并迁移：

- `CMakeLists.txt`
- `package.xml`
- `launch/logitech.launch.py`
- `launch/mbot_teleop.launch.py`
- `scripts/mbot_teleop.py`

新增 Python 管理文件：

- `pyproject.toml`：仅管理格式检查和 Python 测试工具，使用 `uv`。
- `uv.lock`：由 `uv` 生成；ROS2 的 `rclpy` 和消息包仍由容器 apt/ROS 环境提供。

teleop 迁移要求：

- 保留原键位映射、速度倍率、超时减速、线速度/转向平滑和左右轮公式：
  `left=50*speed+3*turn`、`right=50*speed-3*turn`。
- 保留默认发布到 `mbot5` 左右轮控制器的语义，仅按 ROS2 控制器接口做必要消息适配。
- `rospy` 改为 `rclpy`，终端 raw mode 和 0.1 秒按键轮询逻辑保持不变。
- `logitech.launch.py` 优先寻找 ROS2 等价 joystick/velocity smoother；依赖或 topic
  无法一一映射时先列出差异并请求批准。

### 4.6 现有辅助文件和文档

| 文件 | 计划改动 |
|---|---|
| `src/sentry_gazebo_2024/m_bot/README.md` | 从 legacy 迁移并改写 ROS2 等价启动命令，保留原功能说明 |
| `docker/start_lidar_demo.sh` | 改为调用 `mbot_gazebo` 正式 launch，不再手工拼接被合并的 xacro |
| `docs/experiments/002-rmchangdi1230-world-migration.md` | 保留历史数据，增加修补后的状态引用 |
| `docs/experiments/003-mbot-description-migration.md` | 保留历史数据，说明原“合并模型”不再作为最终结构 |
| `docs/experiments/004-gpu-lidar-acceptance.md` | 保留原链路证据，增加修补后回归结果引用 |
| `docs/handover.md` | 修正阶段二状态和正式启动方式 |
| `docs/experiments/008-phase-2-fidelity-remediation.md` | 新增统一修补验收记录 |

## 5. legacy 已知缺陷和审批点

以下问题来自 legacy 本身。实施时先保持可追溯，不擅自修复：

1. `display_mbot_with_camera_xacro.launch` 引用仓库中不存在的
   `urdf/xacro/mbot_with_camera.xacro`。
2. `mbot_gazebo/launch/mbot.launch` 默认引用不存在的
   `worlds/rmuc_mbot_lidar.world`。
3. `RMchangdi1230/launch/display.launch` 引用不存在的 `urdf.rviz`。
4. 两个场地 URDF 使用旧包名 `RM2024changdi` 和 `uc2025_asm`，与当前包名不一致。
5. `mbot_teleop.py` 的异常处理引用未定义的 `e`，finally 分支引用未定义的 `pub`。
6. `arbotix_python`、`turtlebot_teleop` 和原 velodyne Gazebo 插件在 Jazzy/Harmonic
   中不一定有直接对应包。

处理规则：ROS2 语法或资源路径导致的必需修复可在原位置完成并记录；改变默认功能、
控制公式、模型选择或依赖方案的修复，必须先获得用户批准。

## 6. 容器验证计划

### 6.1 静态和构建验证

```bash
cd /root/sentry_ws
colcon build --symlink-install --packages-select \
  rmchangdi1230 mbot_description mbot_control mbot_gazebo mbot_teleop
```

- 五个包均由 `ros2 pkg prefix` 找到。
- 所有 xacro 能展开，且无 Classic Gazebo 插件残留。
- legacy/src 文件映射无未解释缺项。
- 对比 mbot 的 link、joint、mass、inertia、wheel geometry 和 sensor 参数。
- 对比两个场地 world 及六个 mbot world 的模型、pose、物理和资源引用。

### 6.2 单车功能验证

- `mbot_gazebo.launch.py` 一键启动场地、单车、bridge、TF 和控制器。
- ROS2 `/cmd_vel` 能驱动 mbot，odom、TF、wheel TF/joint states 与运动一致。
- 左右轮控制器可独立接收原语义的速度命令。
- `/scan`、`/points`、`/imu/data` 持续发布，frame、频率、范围和束数符合 legacy 参数。
- RViz2 同时显示模型、TF、里程计、点云和 IMU frame。

### 6.3 多车和 teleop 验证

- `mbot_3d_lidar_gazebo.launch.py` 生成 `mbot`～`mbot5`，名称和初始位置与 legacy 一致。
- 五组 controller manager/controller 能按各自命名空间启动，不串 topic。
- `mbot_teleop.py` 的键位、加减速、平滑和停止行为与 legacy 一致，默认控制 `mbot5`。
- 蓝车和红色目标车外观、雷达、IMU、TF 和 odom 命名空间正确。

### 6.4 回归验证

- 重跑实验 002 的场地碰撞测试。
- 重跑实验 004 的 gpu lidar、bridge、TF、odom、运动和 RViz2 验收。
- 再启动 `trajectory_generation`，确认 `/odom`、`/points` 和 TF 接口没有被阶段二修补破坏。

## 7. 完成标准

阶段二只有同时满足以下条件才能重新标记完成：

- `RMchangdi1230` 与 `m_bot` 的 legacy/src 文件映射完整，缺项均有已批准说明。
- 原 xacro、world、launch、控制和 teleop 模块边界恢复，没有合并式替代实现。
- 单车和五车路径均可启动，ROS2 cmd_vel 和左右轮控制路径均有等价验证。
- 场地碰撞、mbot 机械参数、HDL-32E、IMU、TF、odom 和 RViz2 验收通过。
- 所有验证在 Docker 容器内完成并写入实验 008。
- 审查确认只有 ROS1→ROS2/Harmonic 必需适配，没有未经批准的功能删减。

## 8. 建议提交拆分

1. `docs: add phase two fidelity remediation plan`
2. `fix: restore rmchangdi1230 legacy structure`
3. `fix: restore mbot description module boundaries`
4. `feat: migrate mbot control to ros2 control`
5. `feat: migrate mbot gazebo launch and worlds`
6. `feat: migrate mbot teleop to rclpy`
7. `fix: complete phase two ros gz integration`
8. `docs: record phase two fidelity acceptance`

每个代码提交单独在容器内编译对应包；最终验收通过前，阶段二保持“修补中”状态。
