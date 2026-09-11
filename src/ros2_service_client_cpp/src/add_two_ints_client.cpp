#include <chrono>
#include <cstdint>
#include <memory>

#include "example_interfaces/srv/add_two_ints.hpp"
#include "rclcpp/rclcpp.hpp"

using namespace std::chrono_literals;

class AddTwoIntsClient final : public rclcpp::Node {
 public:
  AddTwoIntsClient() : Node("add_two_ints_client") {
    a_ = declare_parameter<std::int64_t>("a", 2);
    b_ = declare_parameter<std::int64_t>("b", 3);
    client_ = create_client<example_interfaces::srv::AddTwoInts>("add_two_ints");
    timer_ = create_wall_timer(500ms, [this] { send_request(); });
  }

 private:
  void send_request() {
    if (request_sent_) {
      return;
    }
    if (!client_->service_is_ready()) {
      RCLCPP_INFO_THROTTLE(
        get_logger(), *get_clock(), 2000, "Waiting for /add_two_ints service...");
      return;
    }

    auto request = std::make_shared<example_interfaces::srv::AddTwoInts::Request>();
    request->a = a_;
    request->b = b_;
    request_sent_ = true;
    timer_->cancel();
    RCLCPP_INFO(get_logger(), "Sending request: %ld + %ld", static_cast<long>(a_), static_cast<long>(b_));

    client_->async_send_request(
      request,
      [this](rclcpp::Client<example_interfaces::srv::AddTwoInts>::SharedFuture future) {
        RCLCPP_INFO(get_logger(), "Response: %ld", static_cast<long>(future.get()->sum));
        rclcpp::shutdown();
      });
  }

  std::int64_t a_;
  std::int64_t b_;
  bool request_sent_{false};
  rclcpp::Client<example_interfaces::srv::AddTwoInts>::SharedPtr client_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<AddTwoIntsClient>());
  rclcpp::shutdown();
  return 0;
}
