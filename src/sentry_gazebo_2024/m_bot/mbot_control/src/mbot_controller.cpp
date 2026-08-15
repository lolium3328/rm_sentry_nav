#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/float64_multi_array.hpp>

/* mbot速度消息发布器 */
int main (int argc, char** argv) {
                        /** 初始化 **/
    /* 注册节点 */
    rclcpp::init(argc, argv);
    auto node = rclcpp::Node::make_shared("mbot_controller");

    /* 消息发布器初始化 */
    auto left = node->create_publisher<std_msgs::msg::Float64MultiArray>("/mbot/left_wheel_joint_controller/commands", 1);
    auto right = node->create_publisher<std_msgs::msg::Float64MultiArray>("/mbot/right_wheel_joint_controller/commands", 1);

    /* 目标速度更新周期设置为1ms*/
    rclcpp::WallRate loop_rate(1000);
    while (rclcpp::ok()) {
        std_msgs::msg::Float64MultiArray left_speed; std_msgs::msg::Float64MultiArray right_speed;
        left_speed.data = {10.0}; right_speed.data = {10.0};
        left->publish(left_speed); right->publish(right_speed);
        rclcpp::spin_some(node); loop_rate.sleep();
    }
    rclcpp::shutdown();
}
