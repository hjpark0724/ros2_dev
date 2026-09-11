#include <memory>

#include "add_two_float_interfaces/srv/add_two_floats.hpp"
#include "rclcpp/rclcpp.hpp"

class AddTwoFloatsServer final : public rclcpp::Node {
 public:
  AddTwoFloatsServer() : Node("add_two_floats_server") {
    service_ = create_service<add_two_float_interfaces::srv::AddTwoFloats>(
      "add_two_floats",
      [this](const std::shared_ptr<add_two_float_interfaces::srv::AddTwoFloats::Request> request,
             std::shared_ptr<add_two_float_interfaces::srv::AddTwoFloats::Response> response) {
        response->sum = request->a + request->b;
        RCLCPP_INFO(
          get_logger(), "Request: %.6f + %.6f = %.6f", request->a, request->b, response->sum);
      });
  }

 private:
  rclcpp::Service<add_two_float_interfaces::srv::AddTwoFloats>::SharedPtr service_;
};

int main(int argc, char * argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<AddTwoFloatsServer>());
  rclcpp::shutdown();
  return 0;
}
