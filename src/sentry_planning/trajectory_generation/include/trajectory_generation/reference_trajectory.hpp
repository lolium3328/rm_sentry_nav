#pragma once

// 阶段 3c | 参考轨迹系数，对照 legacy/.../trajectory_generation/include/reference_path.h

#include <Eigen/Core>

#include <vector>

namespace trajectory_generation
{

struct PolynomialTrajectory
{
  std::vector<float> coef_x;
  std::vector<float> coef_y;
  std::vector<float> duration;
};

class ReferenceTrajectory
{
public:
  PolynomialTrajectory generate(
    const std::vector<Eigen::Vector3d> & path, double desired_speed) const;
};

}  // namespace trajectory_generation
