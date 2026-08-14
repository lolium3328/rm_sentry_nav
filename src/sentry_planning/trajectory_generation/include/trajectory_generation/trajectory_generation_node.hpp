#pragma once

// 阶段 3b | ROS2 数据接入层，对照 legacy/.../trajectory_generation/include/RM_GridMap.h

#include <rclcpp/rclcpp.hpp>

#include <nav_msgs/msg/odometry.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>

#include <opencv2/core/mat.hpp>

#include <memory>
#include <string>

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

  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odometry_subscription_;
  rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr point_cloud_subscription_;
  rclcpp::TimerBase::SharedPtr transform_check_timer_;
  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  std::unique_ptr<tf2_ros::TransformListener> tf_listener_;
};

}  // namespace trajectory_generation
