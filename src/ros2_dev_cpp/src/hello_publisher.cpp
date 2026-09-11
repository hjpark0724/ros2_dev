#include <chrono>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

class HelloPublisher final : public rclcpp::Node {
 public:
  HelloPublisher() : Node("hello_publisher"), count_(0) {
    publisher_ = create_publisher<std_msgs::msg::String>("hello", 10);
    timer_ = create_wall_timer(1s, [this] {
      std_msgs::msg::String message;
      message.data = "Hello from ROS 2 Jazzy #" + std::to_string(count_++);
      publisher_->publish(message);
      RCLCPP_INFO(get_logger(), "%s", message.data.c_str());
    });
  }

 private:
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
  std::size_t count_;
};

int main(int argc, char * argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<HelloPublisher>());
  rclcpp::shutdown();
  return 0;
}
