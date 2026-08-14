// 阶段 3a | ROS2 最小节点，对照 legacy/.../trajectory_generation/src/trajectory_generator_node.cpp

#include <rclcpp/rclcpp.hpp>

#include "trajectory_generation/grid_node.hpp"

namespace trajectory_generation
{

class TrajectoryGenerationNode : public rclcpp::Node
{
public:
  TrajectoryGenerationNode()
: Node("trajectory_generation")
  {
    RCLCPP_INFO(get_logger(), "trajectory_generation 3a skeleton is ready");
  }
};

}  // namespace trajectory_generation

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<trajectory_generation::TrajectoryGenerationNode>());
  rclcpp::shutdown();
  return 0;
}
