#include "h264_mp4_recorder_cpp/h264_mp4_recorder.hpp"
#include "rclcpp/rclcpp.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(h264_mp4_recorder_cpp::make_h264_mp4_recorder());
  rclcpp::shutdown();
  return 0;
}
