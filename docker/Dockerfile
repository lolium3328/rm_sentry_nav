# syntax=docker/dockerfile:1

# =============================================================================
# 哨兵导航 ROS1(Noetic) -> ROS2(Jazzy) 迁移 —— 开发环境镜像
# 目标: Ubuntu 24.04 + ROS 2 Jazzy (+ Gazebo Harmonic / ros_gz)
#
# 多阶段分层(便于缓存与复现, Dockerfile 即环境唯一事实源):
#   base  : 系统 + ROS2 基础包 + 编译工具链
#   deps  : Sophus 等第三方 C++ 库(源码编译)
#   ocs2  : OCS2 官方 ros2 分支(替代仓库内自带的 catkin 版 src/ocs2)
#   dev   : 最终开发环境(项目源码通过 bind mount 挂载, 增量迁移/编译)
# =============================================================================

ARG ROS_DISTRO=jazzy

# ------------------------------- base --------------------------------------
FROM osrf/ros:${ROS_DISTRO}-desktop-full AS base

SHELL ["/bin/bash", "-c"]
ENV DEBIAN_FRONTEND=noninteractive \
    LANG=C.UTF-8 \
    LC_ALL=C.UTF-8

# 换 ROS apt 源为清华镜像(国内加速; packages.ros.org 直连极慢)
# 注意: osrf 镜像的 ROS 源是 deb822 格式, 位于 ros2.sources(符号链接指向 /usr/share/ros-apt-source/ros2.sources),
# 需用 --follow-symlinks 避免 sed -i 破坏符号链接
RUN sed -i --follow-symlinks -E \
        -e 's|https?://packages\.ros\.org/ros2/ubuntu|https://mirrors.tuna.tsinghua.edu.cn/ros2/ubuntu|g' \
        -e 's|Types: deb deb-src|Types: deb|g' \
        /etc/apt/sources.list.d/ros2.sources \
    && grep -hE "URIs|Types" /etc/apt/sources.list.d/ros2.sources

# Gazebo Harmonic 官方 apt 源 (ROS2 Jazzy 用 ros_gz + gz-sim 替代经典 Gazebo)
RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates curl gnupg lsb-release wget \
    && curl -fsSL https://packages.osrfoundation.org/gazebo.gpg \
        -o /usr/share/keyrings/pkgs-osrf-archive-keyring.gpg \
    && echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/pkgs-osrf-archive-keyring.gpg] http://packages.osrfoundation.org/gazebo/ubuntu-stable $(lsb_release -cs) main" \
        > /etc/apt/sources.list.d/gazebo-stable.list \
    && rm -rf /var/lib/apt/lists/*

# ROS2 包(由 ros-noetic-* 逐项映射到 ros-jazzy-*)
#   经典 gazebo_ros / gazebo_ros_control 已废弃 -> 用 ros-gz / gz-ros2-control
RUN apt-get update && apt-get install -y --no-install-recommends \
        ros-${ROS_DISTRO}-xacro \
        ros-${ROS_DISTRO}-robot-state-publisher \
        ros-${ROS_DISTRO}-joint-state-publisher \
        ros-${ROS_DISTRO}-pcl-ros \
        ros-${ROS_DISTRO}-image-transport \
        ros-${ROS_DISTRO}-camera-info-manager \
        ros-${ROS_DISTRO}-ros2-control \
        ros-${ROS_DISTRO}-ros2-controllers \
        ros-${ROS_DISTRO}-ros-gz \
        ros-${ROS_DISTRO}-gz-ros2-control \
        ros-${ROS_DISTRO}-nav-msgs \
        ros-${ROS_DISTRO}-visualization-msgs \
    && rm -rf /var/lib/apt/lists/*

# 编译工具链 + 第三方系统库 (OCS2 / 项目依赖)
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake git \
        python3-colcon-common-extensions python3-rosdep python3-pip \
        libeigen3-dev libboost-all-dev libglpk-dev \
        libfmt-dev libspdlog-dev libyaml-cpp-dev \
        libopencv-dev libpcl-dev \
        pybind11-dev python3-pybind11 \
        qt6-base-dev qt6-base-dev-tools \
        doxygen \
    && rm -rf /var/lib/apt/lists/*

# rosdep 初始化(osrf 镜像可能已初始化, 失败也不中断)
RUN (rosdep init || true) && rosdep update

# ------------------------------- deps --------------------------------------
FROM base AS deps

# Sophus (Lie 群库; 项目直接 include 使用, Ubuntu 24.04 无 apt 包, 需源码安装)
# 注意: GCC13 + Eigen 的 SIMD 代码会在 Sophus 1.22.10 的单元测试里触发 -Werror=array-bounds,
# 因此关闭测试/示例, 并把 array-bounds 降级为警告(我们只用它的头文件库)
RUN git clone --depth 1 --branch 1.22.10 https://github.com/strasdat/Sophus.git /tmp/Sophus \
    && cmake -S /tmp/Sophus -B /tmp/Sophus/build \
        -DCMAKE_BUILD_TYPE=Release \
        -DSOPHUS_USE_BASIC_LOGGING=ON \
        -DSOPHUS_BUILD_TESTS=OFF \
        -DSOPHUS_BUILD_EXAMPLES=OFF \
        -DCMAKE_CXX_FLAGS="-Wno-error=array-bounds" \
    && cmake --build /tmp/Sophus/build --parallel \
    && cmake --install /tmp/Sophus/build \
    && rm -rf /tmp/Sophus

# ------------------------------- ocs2 --------------------------------------
# OCS2 官方 ros2 分支, 直接面向 Ubuntu 24.04 + ROS2 Jazzy (colcon/ament)
# 用它替换仓库内 src/ocs2 的 catkin 旧版
FROM deps AS ocs2

ARG ROS_DISTRO=jazzy

ENV OCS2_WS=/root/ocs2_ws
WORKDIR ${OCS2_WS}/src

# --recursive 拉取 blasfeo/hpipm 等子模块
RUN git clone --depth 1 --branch ros2 --recursive \
        https://github.com/leggedrobotics/ocs2.git

WORKDIR ${OCS2_WS}
# 只编译本项目所需的核心包(MPC/SQP/ROS 接口等), 跳过 legged/anymal/quadrotor 等示例机器人包
# (那些包的依赖 pinocchio/hpp-fcl/grid-map 等在 Jazzy 不可用, 且本项目用不到)
# 核心依赖(Eigen/Boost/glpk/fmt/spdlog/pybind11)已在 base 阶段装好, 无需 rosdep install 全量解析
# 注意: Dockerfile 的 RUN 不会自动 source ROS 环境, 必须手动 source /opt/ros/.../setup.bash
RUN source /opt/ros/${ROS_DISTRO}/setup.bash \
    && colcon build --symlink-install \
        --packages-up-to ocs2_mpc ocs2_sqp ocs2_robotic_tools ocs2_ros_interfaces \
        --cmake-args -DCMAKE_BUILD_TYPE=Release \
    && echo "source ${OCS2_WS}/install/setup.bash" >> /root/.bashrc

# ------------------------------- dev ---------------------------------------
FROM ocs2 AS dev

ARG ROS_DISTRO=jazzy
ENV ROS_DISTRO=${ROS_DISTRO} \
    SENTRY_WS=/root/sentry_ws

WORKDIR ${SENTRY_WS}

# 项目源码通过 bind mount 挂载到 ${SENTRY_WS}/src(见 docker-compose.yml)
# 这里不 COPY 源码 —— 便于增量迁移与实时编辑
RUN mkdir -p ${SENTRY_WS}/src

COPY docker/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
CMD ["bash"]
