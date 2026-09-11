#pragma once

#include <atomic>
#include <memory>

#include "rclcpp/node.hpp"

namespace h264_mp4_recorder_cpp
{

class RecordingControl
{
public:
  explicit RecordingControl(const bool recording_enabled = true)
  : recording_enabled_(recording_enabled)
  {
  }

  void start()
  {
    recording_enabled_.store(true);
  }

  void stop()
  {
    recording_enabled_.store(false);
  }

  bool recording_enabled() const
  {
    return recording_enabled_.load();
  }

private:
  std::atomic_bool recording_enabled_;
};

rclcpp::Node::SharedPtr make_h264_mp4_recorder(
  std::shared_ptr<RecordingControl> recording_control = nullptr);

}  // namespace h264_mp4_recorder_cpp
