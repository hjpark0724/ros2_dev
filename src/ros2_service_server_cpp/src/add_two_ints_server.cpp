#include <memory>

#include "example_interfaces/srv/add_two_ints.hpp"
#include "rclcpp/rclcpp.hpp"

class AddTwoIntsServer final : public rclcpp::Node {
 public:
  AddTwoIntsServer() : Node("add_two_ints_server") {
    service_ = create_service<example_interfaces::srv::AddTwoInts>(
      "add_two_ints",
      [this](const std::shared_ptr<example_interfaces::srv::AddTwoInts::Request> request,
             std::shared_ptr<example_interfaces::srv::AddTwoInts::Response> response) {
        response->sum = request->a + request->b;
        RCLCPP_INFO(
          get_logger(), "Request: %ld + %ld = %ld", static_cast<long>(request->a),
          static_cast<long>(request->b), static_cast<long>(response->sum));
      });
  }

 private:
  rclcpp::Service<example_interfaces::srv::AddTwoInts>::SharedPtr service_;
};

int main(int argc, char * argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<AddTwoIntsServer>());
  rclcpp::shutdown();
  return 0;
}
