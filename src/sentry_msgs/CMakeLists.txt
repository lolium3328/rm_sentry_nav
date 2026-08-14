# [迁移自 ROS1] legacy/sentry_planning/src/rm2023_sentry_msgs/CMakeLists.txt
# 阶段1 | 改动: catkin/add_message_files → ament_cmake/rosidl_generate_interfaces
cmake_minimum_required(VERSION 3.8)
project(sentry_msgs)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(std_msgs REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/RobotsHP.msg"
  "msg/RobotStatus.msg"
  "msg/SlaverSpeed.msg"
  "msg/TrajectoryPoly.msg"
  "srv/GoTarget.srv"
  DEPENDENCIES builtin_interfaces std_msgs
)

ament_export_dependencies(rosidl_default_runtime)

ament_package()
