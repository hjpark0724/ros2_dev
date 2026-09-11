#pragma once

#include "rclcpp/node.hpp"

namespace yuv_h264_encoder_cpp
{

rclcpp::Node::SharedPtr make_yuyv_h264_encoder();

}  // namespace yuv_h264_encoder_cpp
