#!/usr/bin/env bash
# 启动 RMUC 世界、mbot、gpu_lidar、ROS2 bridge 和 robot_state_publisher。
set -eo pipefail

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ -z "${DISPLAY:-}" ]]; then
  echo "DISPLAY 未设置；请在图形桌面终端中运行此脚本。" >&2
  exit 1
fi

if docker info >/dev/null 2>&1; then
  docker_cmd=(docker)
else
  docker_cmd=(sudo docker)
fi

xhost +local:docker

exec "${docker_cmd[@]}" compose -f "${project_dir}/docker/docker-compose.yml" \
  run --rm --no-TTY sentry bash -lc '
set -eo pipefail

cd /root/sentry_ws
colcon build --symlink-install --packages-select rmchangdi1230 mbot_description >/dev/null
source install/setup.bash

world_file="install/rmchangdi1230/share/rmchangdi1230/worlds/rmuc_static.world"
bridge_file="install/mbot_description/share/mbot_description/config/lidar_bridge.yaml"

xacro install/mbot_description/share/mbot_description/urdf/mbot.xacro > /tmp/mbot.urdf
gz sdf -p /tmp/mbot.urdf > /tmp/mbot.sdf

gz sim -r "$world_file" > /tmp/gz-sim.log 2>&1 &
gz_pid=$!
bridge_pid=""
rsp_pid=""
rviz_pid=""

cleanup() {
  [[ -n "$rsp_pid" ]] && kill "$rsp_pid" 2>/dev/null || true
  [[ -n "$rviz_pid" ]] && kill "$rviz_pid" 2>/dev/null || true
  [[ -n "$bridge_pid" ]] && kill "$bridge_pid" 2>/dev/null || true
  kill "$gz_pid" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

for _ in $(seq 1 30); do
  if gz service -l | grep -qx /world/default/create; then
    break
  fi
  sleep 1
done
gz service -l | grep -qx /world/default/create || {
  echo "Gazebo world 未在 30 秒内就绪。日志：/tmp/gz-sim.log" >&2
  exit 1
}

ros2 launch ros_gz_bridge ros_gz_bridge.launch.py \
  bridge_name:=sentry_bridge config_file:="$bridge_file" > /tmp/lidar-bridge.log 2>&1 &
bridge_pid=$!

ros2 launch mbot_description robot_state_publisher.launch.py \
  > /tmp/robot-state-publisher.log 2>&1 &
rsp_pid=$!

sleep 2
ros2 run ros_gz_sim create -world default -file /tmp/mbot.sdf -name mbot -x 2 -y 2 -z 0.3

rviz2 -d install/mbot_description/share/mbot_description/config/rviz/mbot_lidar.rviz \
  > /tmp/rviz2.log 2>&1 &
rviz_pid=$!

echo
echo "启动完成："
echo "  Gazebo:  场地、mbot 与雷达可视化"
echo "  ROS2:    /scan  /points  /odom  /tf"
echo "  RViz2:   已加载 mbot_lidar.rviz（Fixed Frame: base_footprint，显示 /points）"
echo "  验收:    另开终端运行 ros2 topic hz /scan 和 ros2 topic hz /points"
echo "  日志:    /tmp/gz-sim.log  /tmp/lidar-bridge.log  /tmp/robot-state-publisher.log  /tmp/rviz2.log"

wait "$gz_pid"
'
