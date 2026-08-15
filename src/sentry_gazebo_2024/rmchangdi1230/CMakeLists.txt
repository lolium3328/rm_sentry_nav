cmake_minimum_required(VERSION 3.8)
project(rmchangdi1230)

find_package(ament_cmake REQUIRED)

install(DIRECTORY config launch meshes urdf world worlds
  DESTINATION share/${PROJECT_NAME})

ament_package()
