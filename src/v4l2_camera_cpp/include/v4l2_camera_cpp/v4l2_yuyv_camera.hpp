#pragma once

#include "rclcpp/node.hpp"

namespace v4l2_camera_cpp
{

rclcpp::Node::SharedPtr make_v4l2_yuyv_camera();

}  // namespace v4l2_camera_cpp
