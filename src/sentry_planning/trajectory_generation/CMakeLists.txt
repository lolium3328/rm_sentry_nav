# 阶段 3c | 对照 legacy/sentry_planning/src/sentry_planning/trajectory_generation/CMakeLists.txt
cmake_minimum_required(VERSION 3.8)
project(trajectory_generation)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(OpenCV REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(sentry_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(visualization_msgs REQUIRED)

add_library(trajectory_generation_core
  src/grid_node.cpp
  src/grid_planner.cpp
  src/reference_trajectory.cpp
)
target_include_directories(trajectory_generation_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(trajectory_generation_core Eigen3::Eigen)
target_include_directories(trajectory_generation_core PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(trajectory_generation_core ${OpenCV_LIBS})

add_executable(trajectory_generation
  src/trajectory_generation_node.cpp
)
target_link_libraries(trajectory_generation trajectory_generation_core)
ament_target_dependencies(
  trajectory_generation
  ament_index_cpp
  geometry_msgs
  nav_msgs
  rclcpp
  sensor_msgs
  sentry_msgs
  std_msgs
  tf2
  tf2_ros
  visualization_msgs
)
target_include_directories(trajectory_generation PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(trajectory_generation ${OpenCV_LIBS})

install(
  DIRECTORY include/
  DESTINATION include
)

install(
  DIRECTORY config launch map
  DESTINATION share/${PROJECT_NAME}
)

install(
  TARGETS trajectory_generation_core trajectory_generation
  EXPORT export_trajectory_generation
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

ament_export_targets(export_trajectory_generation HAS_LIBRARY_TARGET)
ament_export_dependencies(ament_index_cpp geometry_msgs nav_msgs rclcpp sensor_msgs sentry_msgs std_msgs tf2 tf2_ros visualization_msgs)

ament_package()
