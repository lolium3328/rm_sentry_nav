#!/usr/bin/env bash
set -e

ROS_DISTRO="${ROS_DISTRO:-jazzy}"
OCS2_WS="${OCS2_WS:-/root/ocs2_ws}"
SENTRY_WS="${SENTRY_WS:-/root/sentry_ws}"

# 1) ROS2 环境
if [ -f "/opt/ros/${ROS_DISTRO}/setup.bash" ]; then
  source "/opt/ros/${ROS_DISTRO}/setup.bash"
fi

# 2) OCS2(如果已在镜像内编译)
if [ -f "${OCS2_WS}/install/setup.bash" ]; then
  source "${OCS2_WS}/install/setup.bash"
fi

# 3) 本项目(如果已用 colcon 编译)
if [ -f "${SENTRY_WS}/install/setup.bash" ]; then
  source "${SENTRY_WS}/install/setup.bash"
fi

exec "$@"
