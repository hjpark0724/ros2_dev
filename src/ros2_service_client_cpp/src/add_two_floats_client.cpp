#include <chrono>
#include <memory>

#include "add_two_float_interfaces/srv/add_two_floats.hpp"
#include "rclcpp/rclcpp.hpp"

using namespace std::chrono_literals;

class AddTwoFloatsClient final : public rclcpp::Node {
 public:
  AddTwoFloatsClient() : Node("add_two_floats_client") {
    a_ = declare_parameter<double>("a", 1.5);
    b_ = declare_parameter<double>("b", 2.25);
    client_ = create_client<add_two_float_interfaces::srv::AddTwoFloats>("add_two_floats");
    timer_ = create_wall_timer(500ms, [this] { send_request(); });
  }

 private:
  void send_request() {
    if (request_sent_) {
      return;
    }
    if (!client_->service_is_ready()) {
      RCLCPP_INFO_THROTTLE(
        get_logger(), *get_clock(), 2000, "Waiting for /add_two_floats service...");
      return;
    }

    auto request = std::make_shared<add_two_float_interfaces::srv::AddTwoFloats::Request>();
    request->a = a_;
    request->b = b_;
    request_sent_ = true;
    timer_->cancel();
    RCLCPP_INFO(get_logger(), "Sending request: %.6f + %.6f", a_, b_);

    client_->async_send_request(
      request,
      [this](rclcpp::Client<add_two_float_interfaces::srv::AddTwoFloats>::SharedFuture future) {
        RCLCPP_INFO(get_logger(), "Response: %.6f", future.get()->sum);
        rclcpp::shutdown();
      });
  }

  double a_;
  double b_;
  bool request_sent_{false};
  rclcpp::Client<add_two_float_interfaces::srv::AddTwoFloats>::SharedPtr client_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<AddTwoFloatsClient>());
  rclcpp::shutdown();
  return 0;
}
