#include "rclcpp/rclcpp.hpp"
#include "v4l2_camera_cpp/v4l2_yuyv_camera.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(v4l2_camera_cpp::make_v4l2_yuyv_camera());
  rclcpp::shutdown();
  return 0;
}
