# 实验 001:sentry_msgs 迁移(阶段 1)

- 日期:2026-08-14
- 阶段:迁移计划阶段 1(自定义消息包)
- 结果:✅ 通过

## 目标

将原 ROS1 包 `rm2023_sentry_msgs`(包名 `sentry_msgs`)迁移到 ROS2 Jazzy,
使自定义消息/服务能被后续规划包引用。

## 改动内容

| 文件 | 改动 |
|------|------|
| `package.xml` | catkin 格式 → ament_cmake 格式,新增 `rosidl_default_generators/runtime` |
| `CMakeLists.txt` | `add_message_files`+`generate_messages` → `rosidl_generate_interfaces` |
| `RobotsHP.msg` | `Header` → `std_msgs/Header`(限定名) |
| `RobotStatus.msg` | `Header` → `std_msgs/Header` |
| `SlaverSpeed.msg` | 原 `slaver_speed.msg` 改名(ROS2 规范 PascalCase),字段不变 |
| `GoTarget.srv` | 无字段改动(ROS1/ROS2 语法兼容) |

## 迁移要点与踩坑

1. **Header 必须用限定名**:ROS2 中 `Header` 需写成 `std_msgs/Header`,并在
   `rosidl_generate_interfaces` 的 `DEPENDENCIES` 声明 `std_msgs`。
2. **消息文件不能放子目录**:最初放在 `msg/referee_system/`,编译能过、
   `ros2 interface list` 也注册,但 `ros2 interface show` 会失败——因为 ROS2
   接口类型是扁平的 `pkg/msg/Type`,定义文件必须平铺在 `msg/` 下。
3. **命名规范**:`slaver_speed.msg` 按 ROS2 惯例改为 `SlaverSpeed.msg`,
   生成类型为 `sentry_msgs::msg::SlaverSpeed`(头文件名仍为 snake_case)。

## 验证结果

```bash
# 编译
colcon build --symlink-install --packages-select sentry_msgs
# => Summary: 1 package finished [3.62s]

# 接口列表
ros2 interface list | grep sentry
# sentry_msgs/msg/RobotStatus
# sentry_msgs/msg/RobotsHP
# sentry_msgs/msg/SlaverSpeed
# sentry_msgs/srv/GoTarget

# 接口定义抽查
ros2 interface show sentry_msgs/msg/RobotsHP   # ✅ 正常
ros2 interface show sentry_msgs/srv/GoTarget   # ✅ 正常
```

## 环境备注

- 构建产物持久化:`docker-compose.yml` 已为 `build/install/log` 增加命名卷,
  否则 `docker compose run --rm` 后 `install/` 会丢失。
