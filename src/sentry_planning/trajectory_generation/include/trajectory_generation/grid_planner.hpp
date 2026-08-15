#pragma once

// 阶段 3c | A* 核心，保留 legacy Astar_searcher 的地图坐标约定

#include <Eigen/Core>
#include <opencv2/core/mat.hpp>

#include <vector>

namespace trajectory_generation
{

class GridPlanner
{
public:
  void set_occupancy_map(
    const cv::Mat & occupancy_map, double resolution, double lower_x, double lower_y,
    double robot_radius, double offset_x, double offset_y);

  std::vector<Eigen::Vector3d> plan(
    const Eigen::Vector3d & start, const Eigen::Vector3d & goal) const;

  std::vector<Eigen::Vector3d> smooth_path(
    const std::vector<Eigen::Vector3d> & path) const;

private:
  Eigen::Vector2i coord_to_index(const Eigen::Vector3d & point) const;
  Eigen::Vector3d index_to_coord(const Eigen::Vector2i & index) const;
  bool is_free(const Eigen::Vector2i & index) const;
  bool line_is_free(const Eigen::Vector2i & start, const Eigen::Vector2i & goal) const;

  cv::Mat occupancy_map_;
  double resolution_{0.1};
  double lower_x_{0.0};
  double lower_y_{0.0};
  double offset_x_{0.0};
  double offset_y_{0.0};
  int inflate_cells_{0};
};

}  // namespace trajectory_generation
