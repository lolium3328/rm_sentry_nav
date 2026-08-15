// 阶段 3c | A* 核心，保留 legacy Astar_searcher 的地图坐标约定

#include "trajectory_generation/grid_planner.hpp"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <unordered_map>

namespace trajectory_generation
{

namespace
{
struct QueueItem
{
  double score;
  Eigen::Vector2i index;
  bool operator<(const QueueItem & other) const {return score > other.score;}
};

int key(const Eigen::Vector2i & index, int width)
{
  return index.y() * width + index.x();
}
}  // namespace

void GridPlanner::set_occupancy_map(
  const cv::Mat & occupancy_map, double resolution, double lower_x, double lower_y,
  double robot_radius, double offset_x, double offset_y)
{
  cv::Mat gray;
  if (occupancy_map.channels() == 1) {
    gray = occupancy_map.clone();
  } else {
    cv::cvtColor(occupancy_map, gray, cv::COLOR_BGR2GRAY);
  }
  // legacy: pixel value > 10 means occupied, then inflate by robot radius.
  cv::threshold(gray, occupancy_map_, 10, 255, cv::THRESH_BINARY);
  resolution_ = resolution;
  lower_x_ = lower_x;
  lower_y_ = lower_y;
  offset_x_ = offset_x;
  offset_y_ = offset_y;
  inflate_cells_ = std::max(0, static_cast<int>(std::ceil(robot_radius / resolution_)));
  if (inflate_cells_ > 0) {
    const auto diameter = 2 * inflate_cells_ + 1;
    const auto kernel = cv::getStructuringElement(
      cv::MORPH_ELLIPSE, cv::Size(diameter, diameter));
    cv::dilate(occupancy_map_, occupancy_map_, kernel);
  }
}

Eigen::Vector2i GridPlanner::coord_to_index(const Eigen::Vector3d & point) const
{
  // legacy image rows count from the top while world y counts from the bottom.
  const int x = static_cast<int>(std::floor((point.x() + offset_x_ - lower_x_) / resolution_));
  const int image_y = static_cast<int>(
    std::floor((point.y() + offset_y_ - lower_y_) / resolution_));
  return {x, occupancy_map_.rows - 1 - image_y};
}

Eigen::Vector3d GridPlanner::index_to_coord(const Eigen::Vector2i & index) const
{
  const double x = lower_x_ + (static_cast<double>(index.x()) + 0.5) * resolution_ - offset_x_;
  const double y = lower_y_ +
    (static_cast<double>(occupancy_map_.rows - 1 - index.y()) + 0.5) * resolution_;
  return {x - 0.0, y - offset_y_, 0.0};
}

bool GridPlanner::is_free(const Eigen::Vector2i & index) const
{
  if (index.x() < 0 || index.y() < 0 ||
    index.x() >= occupancy_map_.cols || index.y() >= occupancy_map_.rows)
  {
    return false;
  }
  return occupancy_map_.at<unsigned char>(index.y(), index.x()) == 0;
}

bool GridPlanner::line_is_free(const Eigen::Vector2i & start, const Eigen::Vector2i & goal) const
{
  const int dx = std::abs(goal.x() - start.x());
  const int dy = std::abs(goal.y() - start.y());
  const int steps = std::max(dx, dy);
  for (int step = 0; step <= steps; ++step) {
    const double ratio = steps == 0 ? 0.0 : static_cast<double>(step) / steps;
    const Eigen::Vector2i point(
      static_cast<int>(std::lround(start.x() + ratio * (goal.x() - start.x()))),
      static_cast<int>(std::lround(start.y() + ratio * (goal.y() - start.y()))));
    if (!is_free(point)) {
      return false;
    }
  }
  return true;
}

std::vector<Eigen::Vector3d> GridPlanner::plan(
  const Eigen::Vector3d & start, const Eigen::Vector3d & goal) const
{
  if (occupancy_map_.empty()) {
    return {};
  }
  const auto start_index = coord_to_index(start);
  const auto goal_index = coord_to_index(goal);
  if (!is_free(start_index) || !is_free(goal_index)) {
    return {};
  }

  const int width = occupancy_map_.cols;
  const int height = occupancy_map_.rows;
  const int cell_count = width * height;
  const double infinity = std::numeric_limits<double>::infinity();
  std::vector<double> costs(cell_count, infinity);
  std::vector<int> parents(cell_count, -1);
  std::priority_queue<QueueItem> open;
  const auto heuristic = [&goal_index](const Eigen::Vector2i & p) {
      return (p.cast<double>() - goal_index.cast<double>()).norm();
    };

  costs[key(start_index, width)] = 0.0;
  open.push({heuristic(start_index), start_index});
  constexpr int directions[8][2] = {
    {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
  while (!open.empty()) {
    const auto current = open.top().index;
    open.pop();
    if (current == goal_index) {
      break;
    }
    const double current_cost = costs[key(current, width)];
    for (const auto & direction : directions) {
      const Eigen::Vector2i next = current + Eigen::Vector2i(direction[0], direction[1]);
      if (!is_free(next)) {
        continue;
      }
      const double step = std::hypot(direction[0], direction[1]);
      const double next_cost = current_cost + step;
      const auto next_key = key(next, width);
      if (next_cost < costs[next_key]) {
        costs[next_key] = next_cost;
        parents[next_key] = key(current, width);
        open.push({next_cost + heuristic(next), next});
      }
    }
  }

  if (parents[key(goal_index, width)] < 0 && start_index != goal_index) {
    return {};
  }
  std::vector<Eigen::Vector3d> path;
  for (int current_key = key(goal_index, width); current_key >= 0;
    current_key = parents[current_key])
  {
    const Eigen::Vector2i current(current_key % width, current_key / width);
    path.push_back(index_to_coord(current));
    if (current == start_index) {
      break;
    }
  }
  std::reverse(path.begin(), path.end());
  return path;
}

std::vector<Eigen::Vector3d> GridPlanner::smooth_path(
  const std::vector<Eigen::Vector3d> & path) const
{
  if (path.size() < 3) {
    return path;
  }
  std::vector<Eigen::Vector3d> result;
  result.push_back(path.front());
  std::size_t anchor = 0;
  while (anchor + 1 < path.size()) {
    std::size_t furthest = anchor + 1;
    const auto anchor_index = coord_to_index(path[anchor]);
    for (std::size_t candidate = furthest + 1; candidate < path.size(); ++candidate) {
      if (line_is_free(anchor_index, coord_to_index(path[candidate]))) {
        furthest = candidate;
      } else {
        break;
      }
    }
    result.push_back(path[furthest]);
    anchor = furthest;
  }
  return result;
}

}  // namespace trajectory_generation
