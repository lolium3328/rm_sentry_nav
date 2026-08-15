// 阶段 3c | ROS2 规划适配层，对照 legacy/.../trajectory_generation/src/replan_fsm.cpp

#include "trajectory_generation/trajectory_generation_node.hpp"

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <geometry_msgs/msg/point.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <opencv2/imgcodecs.hpp>
#include <tf2/exceptions.h>
#include <tf2/time.hpp>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <cmath>
#include <stdexcept>

namespace trajectory_generation
{

TrajectoryGenerationNode::TrajectoryGenerationNode()
: Node("trajectory_generation")
{
  declare_parameters();
  load_maps();
  grid_planner_.set_occupancy_map(
    occupancy_map_, map_resolution_, map_lower_x_, map_lower_y_, robot_radius_,
    map_offset_x_, map_offset_y_);

  tf_buffer_ = std::make_unique<tf2_ros::Buffer>(get_clock());
  tf_listener_ = std::make_unique<tf2_ros::TransformListener>(*tf_buffer_);

  odometry_subscription_ = create_subscription<nav_msgs::msg::Odometry>(
    odom_topic_, rclcpp::SensorDataQoS(),
    std::bind(&TrajectoryGenerationNode::odometry_callback, this, std::placeholders::_1));
  point_cloud_subscription_ = create_subscription<sensor_msgs::msg::PointCloud2>(
    point_cloud_topic_, rclcpp::SensorDataQoS(),
    std::bind(&TrajectoryGenerationNode::point_cloud_callback, this, std::placeholders::_1));
  waypoint_subscription_ = create_subscription<nav_msgs::msg::Path>(
    "/waypoint_generator/waypoints", rclcpp::QoS(10).reliable(),
    std::bind(&TrajectoryGenerationNode::waypoint_callback, this, std::placeholders::_1));
  replan_subscription_ = create_subscription<std_msgs::msg::Bool>(
    "/replan_flag", rclcpp::QoS(10).reliable(),
    std::bind(&TrajectoryGenerationNode::replan_callback, this, std::placeholders::_1));
  global_path_publisher_ = create_publisher<nav_msgs::msg::Path>("global_path", 10);
  path_marker_publisher_ = create_publisher<visualization_msgs::msg::Marker>("astar_path_vis", 10);
  trajectory_publisher_ = create_publisher<sentry_msgs::msg::TrajectoryPoly>("global_trajectory", 10);
  planning_timer_ = create_wall_timer(
    std::chrono::milliseconds(30), std::bind(&TrajectoryGenerationNode::execute_fsm, this));
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
  map_resolution_ = declare_parameter<double>("trajectory_generator.map_resolution", 0.1);
  map_lower_x_ = declare_parameter<double>("trajectory_generator.map_lower_point_x", 0.0);
  map_lower_y_ = declare_parameter<double>("trajectory_generator.map_lower_point_y", 0.0);
  map_offset_x_ = declare_parameter<double>("trajectory_generator.map_offset_x", 0.0);
  map_offset_y_ = declare_parameter<double>("trajectory_generator.map_offset_y", 0.0);
  robot_radius_ = declare_parameter<double>("trajectory_generator.robot_radius", 0.35);
  reference_speed_ = declare_parameter<double>(
    "trajectory_generator.reference_desire_speed", 2.5);
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
  current_position_.x() = message->pose.pose.position.x;
  current_position_.y() = message->pose.pose.position.y;
  current_position_.z() = message->pose.pose.position.z;
  have_odometry_ = true;
  if (planning_state_ == PlanningState::WAIT_ODOMETRY) {
    planning_state_ = have_target_ ? PlanningState::GEN_NEW_TRAJ : PlanningState::WAIT_TARGET;
  }
  RCLCPP_INFO_ONCE(
    get_logger(), "received odometry: frame=%s child_frame=%s",
    message->header.frame_id.c_str(), message->child_frame_id.c_str());
}

void TrajectoryGenerationNode::waypoint_callback(nav_msgs::msg::Path::ConstSharedPtr message)
{
  if (message->poses.empty()) {
    RCLCPP_WARN(get_logger(), "received an empty waypoint path");
    return;
  }
  const auto & target_pose = message->poses.back().pose.position;
  target_position_ = Eigen::Vector3d(target_pose.x, target_pose.y, target_pose.z);
  have_target_ = true;
  planning_state_ = have_odometry_ ? PlanningState::GEN_NEW_TRAJ : PlanningState::WAIT_ODOMETRY;
  RCLCPP_INFO(
    get_logger(), "FSM received target=(%.2f, %.2f), state=%s", target_position_.x(),
    target_position_.y(), have_odometry_ ? "GEN_NEW_TRAJ" : "WAIT_ODOMETRY");
}

void TrajectoryGenerationNode::replan_callback(std_msgs::msg::Bool::ConstSharedPtr message)
{
  if (message->data && have_target_ && have_odometry_) {
    planning_state_ = PlanningState::GEN_NEW_TRAJ;
    RCLCPP_INFO(get_logger(), "FSM replan requested");
  }
}

void TrajectoryGenerationNode::execute_fsm()
{
  if (planning_state_ != PlanningState::GEN_NEW_TRAJ) {
    return;
  }
  planning_state_ = PlanningState::EXEC_TRAJ;
  const auto raw_path = grid_planner_.plan(current_position_, target_position_);
  const auto path = grid_planner_.smooth_path(raw_path);
  if (path.empty()) {
    RCLCPP_ERROR(
      get_logger(), "A* failed: start=(%.2f, %.2f), goal=(%.2f, %.2f)",
      current_position_.x(), current_position_.y(), target_position_.x(), target_position_.y());
    planning_state_ = PlanningState::WAIT_TARGET;
    return;
  }
  publish_plan(path);
  RCLCPP_INFO(
    get_logger(), "A* planned %zu points, smoothed to %zu points", raw_path.size(), path.size());
}

void TrajectoryGenerationNode::publish_plan(const std::vector<Eigen::Vector3d> & path)
{
  const auto stamp = now();
  nav_msgs::msg::Path path_message;
  path_message.header.stamp = stamp;
  path_message.header.frame_id = odom_frame_;
  visualization_msgs::msg::Marker marker;
  marker.header = path_message.header;
  marker.ns = "trajectory_generation";
  marker.id = 0;
  marker.type = visualization_msgs::msg::Marker::LINE_STRIP;
  marker.action = visualization_msgs::msg::Marker::ADD;
  marker.scale.x = 0.08;
  marker.color.r = 0.1F;
  marker.color.g = 0.8F;
  marker.color.b = 0.2F;
  marker.color.a = 1.0F;

  sentry_msgs::msg::TrajectoryPoly trajectory;
  const auto stamp_nanoseconds = stamp.nanoseconds();
  trajectory.start_time.sec = static_cast<int32_t>(stamp_nanoseconds / 1000000000LL);
  trajectory.start_time.nanosec = static_cast<uint32_t>(stamp_nanoseconds % 1000000000LL);
  trajectory.motion_mode = 1;
  for (const auto & point : path) {
    geometry_msgs::msg::PoseStamped pose;
    pose.header = path_message.header;
    pose.pose.position.x = point.x();
    pose.pose.position.y = point.y();
    pose.pose.position.z = point.z();
    pose.pose.orientation.w = 1.0;
    path_message.poses.push_back(pose);
    geometry_msgs::msg::Point marker_point;
    marker_point.x = point.x();
    marker_point.y = point.y();
    marker_point.z = point.z();
    marker.points.push_back(marker_point);
  }

  const auto coefficients = reference_trajectory_.generate(path, reference_speed_);
  trajectory.coef_x = coefficients.coef_x;
  trajectory.coef_y = coefficients.coef_y;
  trajectory.duration = coefficients.duration;
  global_path_publisher_->publish(path_message);
  path_marker_publisher_->publish(marker);
  trajectory_publisher_->publish(trajectory);
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
