# [迁移自 ROS1] legacy/sentry_planning/src/rm2023_sentry_msgs/msg/referee_system/RobotsHP.msg
# 阶段1 | 改动: Header → std_msgs/Header, 字段不变
# 机器人血量数据 0x0003 1Hz
std_msgs/Header header
# RED

uint16 red_1_hp
uint16 red_2_hp
uint16 red_3_hp
uint16 red_4_hp
uint16 red_5_hp
uint16 red_sentry_hp
uint16 red_outpost_hp
uint16 red_base_hp

# BLUE

uint16 blue_1_hp
uint16 blue_2_hp
uint16 blue_3_hp
uint16 blue_4_hp
uint16 blue_5_hp
uint16 blue_sentry_hp
uint16 blue_outpost_hp
uint16 blue_base_hp
