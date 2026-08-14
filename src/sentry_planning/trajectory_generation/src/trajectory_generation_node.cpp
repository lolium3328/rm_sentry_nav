// 阶段 3b | ROS2 数据接入层，对照 legacy/.../trajectory_generation/src/trajectory_generator_node.cpp

#include "trajectory_generation/trajectory_generation_node.hpp"

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <opencv2/imgcodecs.hpp>
#include <tf2/exceptions.h>
#include <tf2/time.hpp>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <stdexcept>

namespace trajectory_generation
{

TrajectoryGenerationNode::TrajectoryGenerationNode()
: Node("trajectory_generation")
{
  declare_parameters();
  load_maps();

  tf_buffer_ = std::make_unique<tf2_ros::Buffer>(get_clock());
  tf_listener_ = std::make_unique<tf2_ros::TransformListener>(*tf_buffer_);

  odometry_subscription_ = create_subscription<nav_msgs::msg::Odometry>(
    odom_topic_, rclcpp::SensorDataQoS(),
    std::bind(&TrajectoryGenerationNode::odometry_callback, this, std::placeholders::_1));
  point_cloud_subscription_ = create_subscription<sensor_msgs::msg::PointCloud2>(
    point_cloud_topic_, rclcpp::SensorDataQoS(),
    std::bind(&TrajectoryGenerationNode::point_cloud_callback, this, std::placeholders::_1));
  transform_check_timer_ = create_wall_timer(
    std::chrono::milliseconds(500),
    std::bind(&TrajectoryGenerationNode::check_transform, this));

  RCLCPP_INFO(
    get_logger(), "3b data layer ready: odom=%s, point_cloud=%s, tf=%s -> %s",
    odom_topic_.c_str(), point_cloud_topic_.c_str(), odom_frame_.c_str(), base_frame_.c_str());
}

void TrajectoryGenerationNode::declare_parameters()
{
  odom_topic_ = declare_parameter<std::string>("topics.odom", "/odom");
  point_cloud_topic_ = declare_parameter<std::string>("topics.point_cloud", "/points");
  odom_frame_ = declare_parameter<std::string>("frames.odom", "mbot/odom");
  base_frame_ = declare_parameter<std::string>("frames.base", "mbot/base_footprint");
  map_directory_ = declare_parameter<std::string>("trajectory_generator.map_directory", "map");
  occ_file_path_ = declare_parameter<std::string>(
    "trajectory_generator.occ_file_path", "occ2024low.png");
  bev_file_path_ = declare_parameter<std::string>(
    "trajectory_generator.bev_file_path", "bev2024low.png");
  distance_map_file_path_ = declare_parameter<std::string>(
    "trajectory_generator.distance_map_file_path", "occtopo2024low.png");
}

std::string TrajectoryGenerationNode::resolve_resource_path(const std::string & path) const
{
  const std::filesystem::path configured_path(path);
  if (configured_path.is_absolute()) {
    return configured_path.string();
  }

  const auto share_directory =
    std::filesystem::path(ament_index_cpp::get_package_share_directory("trajectory_generation"));
  return (share_directory / map_directory_ / configured_path).lexically_normal().string();
}

void TrajectoryGenerationNode::load_maps()
{
  const auto occ_path = resolve_resource_path(occ_file_path_);
  const auto bev_path = resolve_resource_path(bev_file_path_);
  const auto distance_path = resolve_resource_path(distance_map_file_path_);

  occupancy_map_ = cv::imread(occ_path, cv::IMREAD_UNCHANGED);
  bev_map_ = cv::imread(bev_path, cv::IMREAD_UNCHANGED);
  distance_map_ = cv::imread(distance_path, cv::IMREAD_UNCHANGED);
  if (occupancy_map_.empty() || bev_map_.empty() || distance_map_.empty()) {
    throw std::runtime_error(
      "failed to load trajectory maps: " + occ_path + ", " + bev_path + ", " + distance_path);
  }

  RCLCPP_INFO(
    get_logger(), "maps loaded: occ=%dx%d, bev=%dx%d, distance=%dx%d",
    occupancy_map_.cols, occupancy_map_.rows, bev_map_.cols, bev_map_.rows,
    distance_map_.cols, distance_map_.rows);
}

void TrajectoryGenerationNode::odometry_callback(nav_msgs::msg::Odometry::ConstSharedPtr message)
{
  RCLCPP_INFO_ONCE(
    get_logger(), "received odometry: frame=%s child_frame=%s",
    message->header.frame_id.c_str(), message->child_frame_id.c_str());
}

void TrajectoryGenerationNode::point_cloud_callback(
  sensor_msgs::msg::PointCloud2::ConstSharedPtr message)
{
  RCLCPP_INFO_ONCE(
    get_logger(), "received point cloud: frame=%s width=%u height=%u",
    message->header.frame_id.c_str(), message->width, message->height);
}

void TrajectoryGenerationNode::check_transform()
{
  try {
    const auto transform = tf_buffer_->lookupTransform(
      odom_frame_, base_frame_, tf2::TimePointZero, tf2::durationFromSec(0.1));
    RCLCPP_INFO_ONCE(
      get_logger(), "TF ready: %s -> %s (x=%.3f, y=%.3f)", odom_frame_.c_str(),
      base_frame_.c_str(), transform.transform.translation.x, transform.transform.translation.y);
  } catch (const tf2::TransformException & exception) {
    RCLCPP_WARN_THROTTLE(
      get_logger(), *get_clock(), 5000, "waiting for TF %s -> %s: %s",
      odom_frame_.c_str(), base_frame_.c_str(), exception.what());
  }
}

}  // namespace trajectory_generation

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  try {
    rclcpp::spin(std::make_shared<trajectory_generation::TrajectoryGenerationNode>());
  } catch (const std::exception & exception) {
    fprintf(stderr, "trajectory_generation failed: %s\n", exception.what());
    rclcpp::shutdown();
    return 1;
  }
  rclcpp::shutdown();
  return 0;
}
