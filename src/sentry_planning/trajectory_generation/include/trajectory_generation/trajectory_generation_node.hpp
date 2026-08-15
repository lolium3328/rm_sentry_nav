#pragma once

// 阶段 3c | ROS2 规划适配层，对照 legacy/.../trajectory_generation/include/replan_fsm.h

#include <rclcpp/rclcpp.hpp>

#include <nav_msgs/msg/odometry.hpp>
#include <nav_msgs/msg/path.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sentry_msgs/msg/trajectory_poly.hpp>
#include <std_msgs/msg/bool.hpp>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>
#include <visualization_msgs/msg/marker.hpp>

#include <opencv2/core/mat.hpp>

#include <memory>
#include <string>

#include "trajectory_generation/grid_planner.hpp"
#include "trajectory_generation/reference_trajectory.hpp"

namespace trajectory_generation
{

class TrajectoryGenerationNode : public rclcpp::Node
{
public:
  TrajectoryGenerationNode();

private:
  void declare_parameters();
  void load_maps();
  std::string resolve_resource_path(const std::string & path) const;
  void odometry_callback(nav_msgs::msg::Odometry::ConstSharedPtr message);
  void point_cloud_callback(sensor_msgs::msg::PointCloud2::ConstSharedPtr message);
  void waypoint_callback(nav_msgs::msg::Path::ConstSharedPtr message);
  void replan_callback(std_msgs::msg::Bool::ConstSharedPtr message);
  void execute_fsm();
  void publish_plan(const std::vector<Eigen::Vector3d> & path);
  void check_transform();

  std::string odom_topic_;
  std::string point_cloud_topic_;
  std::string odom_frame_;
  std::string base_frame_;
  std::string map_directory_;
  std::string occ_file_path_;
  std::string bev_file_path_;
  std::string distance_map_file_path_;

  cv::Mat occupancy_map_;
  cv::Mat bev_map_;
  cv::Mat distance_map_;
  double map_resolution_{0.1};
  double map_lower_x_{0.0};
  double map_lower_y_{0.0};
  double map_offset_x_{0.0};
  double map_offset_y_{0.0};
  double robot_radius_{0.35};
  double reference_speed_{2.5};
  Eigen::Vector3d current_position_{Eigen::Vector3d::Zero()};
  Eigen::Vector3d target_position_{Eigen::Vector3d::Zero()};
  bool have_odometry_{false};
  bool have_target_{false};
  enum class PlanningState {WAIT_ODOMETRY, WAIT_TARGET, GEN_NEW_TRAJ, EXEC_TRAJ};
  PlanningState planning_state_{PlanningState::WAIT_ODOMETRY};
  GridPlanner grid_planner_;
  ReferenceTrajectory reference_trajectory_;

  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odometry_subscription_;
  rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr point_cloud_subscription_;
  rclcpp::Subscription<nav_msgs::msg::Path>::SharedPtr waypoint_subscription_;
  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr replan_subscription_;
  rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr global_path_publisher_;
  rclcpp::Publisher<visualization_msgs::msg::Marker>::SharedPtr path_marker_publisher_;
  rclcpp::Publisher<sentry_msgs::msg::TrajectoryPoly>::SharedPtr trajectory_publisher_;
  rclcpp::TimerBase::SharedPtr planning_timer_;
  rclcpp::TimerBase::SharedPtr transform_check_timer_;
  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  std::unique_ptr<tf2_ros::TransformListener> tf_listener_;
};

}  // namespace trajectory_generation
