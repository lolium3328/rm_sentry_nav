# 迁移自 ROS1: legacy/sentry_planning/src/sentry_planning/trajectory_generation/msg/trajectoryPoly.msg
# 阶段 3a | 改动: ROS1 time → ROS2 builtin_interfaces/Time, 消息名按 ROS2 规范改为 PascalCase
builtin_interfaces/Time start_time
uint8 motion_mode
float32[] coef_x
float32[] coef_y
float32[] duration
