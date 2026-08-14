#pragma once

// 阶段 3a | 纯算法数据结构，对照 legacy/.../trajectory_generation/include/node.h

#include <Eigen/Core>

#include <limits>
#include <map>

namespace trajectory_generation
{

struct GridNode;
using GridNodePtr = GridNode *;

struct GridNode
{
  GridNode() = default;

  GridNode(const Eigen::Vector3i &grid_index, const Eigen::Vector3d &grid_coord)
: coord(grid_coord), index(grid_index)
  {
  }

  int id{0};
  double height{-1.0};
  Eigen::Vector3d coord = Eigen::Vector3d::Zero();
  Eigen::Vector3i dir = Eigen::Vector3i::Zero();
  Eigen::Vector3i index = Eigen::Vector3i::Zero();
  double g_score{std::numeric_limits<double>::infinity()};
  double f_score{std::numeric_limits<double>::infinity()};
  GridNodePtr came_from{nullptr};
  bool exist_second_height{false};
  double second_height{0.08};
  bool second_local_occupancy{false};
  bool second_local_swell{false};
  double visibility{0.0};
};

}  // namespace trajectory_generation
