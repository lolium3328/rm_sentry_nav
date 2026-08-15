#!/usr/bin/env bash
# Formal ROS2 entry point for the single-mbot Harmonic lidar demo.
set -euo pipefail

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ -z "${DISPLAY:-}" ]]; then
  echo "DISPLAY 未设置；请在图形桌面终端中运行此脚本。" >&2
  exit 1
fi
xhost +local:docker >/dev/null

# The login session may not have refreshed the docker supplementary group.
# sg preserves the project command while using the configured docker group.
sg docker -c "docker compose -f '${project_dir}/docker/docker-compose.yml' run --rm sentry bash -lc 'cd /root/sentry_ws && source /opt/ros/jazzy/setup.bash && colcon build --symlink-install --packages-select rmchangdi1230 mbot_description mbot_gazebo && source install/setup.bash && ros2 launch mbot_gazebo mbot_empty_world.launch.py'"
