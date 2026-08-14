# [迁移自 ROS1] legacy/sentry_planning/src/rm2023_sentry_msgs/msg/referee_system/slaver_speed.msg
# 阶段1 | 改动: 文件名 slaver_speed → SlaverSpeed(ROS2 命名规范)
float32 line_speed
float32 angle_target
float32 angle_current
uint8 xtl_flag
uint8 in_bridge
