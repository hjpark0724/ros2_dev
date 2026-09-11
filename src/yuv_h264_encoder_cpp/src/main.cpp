#include "rclcpp/rclcpp.hpp"
#include "yuv_h264_encoder_cpp/yuyv_h264_encoder.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(yuv_h264_encoder_cpp::make_yuyv_h264_encoder());
  rclcpp::shutdown();
  return 0;
}
