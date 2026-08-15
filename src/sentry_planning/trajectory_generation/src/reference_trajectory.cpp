// 阶段 3c | 参考轨迹系数，对照 legacy/.../trajectory_generation/src/reference_path.cpp

#include "trajectory_generation/reference_trajectory.hpp"

#include <Eigen/LU>

#include <algorithm>
#include <cmath>

namespace trajectory_generation
{

PolynomialTrajectory ReferenceTrajectory::generate(
  const std::vector<Eigen::Vector3d> & path, double desired_speed) const
{
  PolynomialTrajectory result;
  if (path.size() < 2) {
    return result;
  }
  const std::size_t count = path.size() - 1;
  std::vector<double> time(count);
  for (std::size_t i = 0; i < count; ++i) {
    const auto distance = (path[i + 1] - path[i]).head<2>().norm();
    time[i] = std::max(0.01, distance / std::max(0.1, desired_speed));
    result.duration.push_back(static_cast<float>(time[i]));
  }

  // Natural cubic spline second derivatives, matching reference_path's
  // banded-system formulation while keeping the ROS2 adapter independent.
  Eigen::MatrixXd system = Eigen::MatrixXd::Zero(static_cast<int>(path.size()), path.size());
  Eigen::VectorXd rhs_x = Eigen::VectorXd::Zero(path.size());
  Eigen::VectorXd rhs_y = Eigen::VectorXd::Zero(path.size());
  system(0, 0) = 1.0;
  system(path.size() - 1, path.size() - 1) = 1.0;
  for (std::size_t i = 1; i + 1 < path.size(); ++i) {
    system(i, i - 1) = time[i - 1];
    system(i, i) = 2.0 * (time[i - 1] + time[i]);
    system(i, i + 1) = time[i];
    rhs_x(static_cast<int>(i)) = 6.0 *
      ((path[i + 1].x() - path[i].x()) / time[i] -
      (path[i].x() - path[i - 1].x()) / time[i - 1]);
    rhs_y(static_cast<int>(i)) = 6.0 *
      ((path[i + 1].y() - path[i].y()) / time[i] -
      (path[i].y() - path[i - 1].y()) / time[i - 1]);
  }
  const Eigen::VectorXd second_x = system.fullPivLu().solve(rhs_x);
  const Eigen::VectorXd second_y = system.fullPivLu().solve(rhs_y);
  for (std::size_t i = 0; i < count; ++i) {
    const double duration = time[i];
    result.coef_x.push_back(static_cast<float>(
      (second_x(i + 1) - second_x(i)) / (6.0 * duration)));
    result.coef_x.push_back(static_cast<float>(second_x(i) / 2.0));
    result.coef_x.push_back(static_cast<float>((path[i + 1].x() - path[i].x()) / duration -
      duration * (2.0 * second_x(i) + second_x(i + 1)) / 6.0));
    result.coef_x.push_back(static_cast<float>(path[i].x()));
    result.coef_y.push_back(static_cast<float>(
      (second_y(i + 1) - second_y(i)) / (6.0 * duration)));
    result.coef_y.push_back(static_cast<float>(second_y(i) / 2.0));
    result.coef_y.push_back(static_cast<float>((path[i + 1].y() - path[i].y()) / duration -
      duration * (2.0 * second_y(i) + second_y(i + 1)) / 6.0));
    result.coef_y.push_back(static_cast<float>(path[i].y()));
  }
  return result;
}

}  // namespace trajectory_generation
