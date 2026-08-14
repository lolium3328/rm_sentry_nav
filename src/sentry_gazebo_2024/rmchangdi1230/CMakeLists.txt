cmake_minimum_required(VERSION 3.8)
project(rmchangdi1230)

find_package(ament_cmake REQUIRED)

# 安装 world 与 meshes 到 share/<pkg>/,供 gz-sim 与 launch 使用
install(
  DIRECTORY worlds meshes
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
