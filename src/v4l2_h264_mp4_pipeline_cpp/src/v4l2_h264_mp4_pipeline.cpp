#include <chrono>
#include <memory>
#include <stdexcept>

#include "h264_mp4_recorder_cpp/h264_mp4_recorder.hpp"
#include "rclcpp/executors/multi_threaded_executor.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_srvs/srv/trigger.hpp"
#include "v4l2_camera_cpp/v4l2_yuyv_camera.hpp"
#include "yuv_h264_encoder_cpp/yuyv_h264_encoder.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);

  rclcpp::executors::MultiThreadedExecutor executor;
  auto controller = std::make_shared<rclcpp::Node>("v4l2_h264_mp4_pipeline");
  const auto recording_duration_sec = controller->declare_parameter<int>("recording_duration_sec", 60);
  if (recording_duration_sec < 0) {
    throw std::invalid_argument("recording_duration_sec must be zero or positive");
  }

  auto camera = v4l2_camera_cpp::make_v4l2_yuyv_camera();
  auto encoder = yuv_h264_encoder_cpp::make_yuyv_h264_encoder();
  auto recording_control = std::make_shared<h264_mp4_recorder_cpp::RecordingControl>(false);
  auto recorder = h264_mp4_recorder_cpp::make_h264_mp4_recorder(recording_control);
  executor.add_node(controller);
  executor.add_node(camera);
  executor.add_node(encoder);
  executor.add_node(recorder);

  rclcpp::TimerBase::SharedPtr stop_timer;
  auto stop_timer_ref = std::make_shared<rclcpp::TimerBase::WeakPtr>();
  if (recording_duration_sec > 0) {
    stop_timer = controller->create_wall_timer(
      std::chrono::seconds(recording_duration_sec),
      [recording_control, stop_timer_ref, logger = controller->get_logger()] {
        recording_control->stop();
        if (const auto timer = stop_timer_ref->lock()) {
          timer->cancel();
        }
        RCLCPP_INFO(logger, "Recording duration elapsed; recording stopped");
      });
    *stop_timer_ref = stop_timer;
    stop_timer->cancel();
  }

  auto start_service = controller->create_service<std_srvs::srv::Trigger>(
    "start_recording",
    [recording_control, stop_timer](
      const std_srvs::srv::Trigger::Request::SharedPtr,
      std_srvs::srv::Trigger::Response::SharedPtr response) {
      const bool was_recording = recording_control->recording_enabled();
      recording_control->start();
      if (stop_timer != nullptr) {
        stop_timer->reset();
      }
      response->success = true;
      response->message = was_recording ? "recording is already active" :
        "recording started";
    });
  auto stop_service = controller->create_service<std_srvs::srv::Trigger>(
    "stop_recording",
    [recording_control, stop_timer](
      const std_srvs::srv::Trigger::Request::SharedPtr,
      std_srvs::srv::Trigger::Response::SharedPtr response) {
      const bool was_recording = recording_control->recording_enabled();
      recording_control->stop();
      if (stop_timer != nullptr) {
        stop_timer->cancel();
      }
      response->success = true;
      response->message = was_recording ? "recording stopped" :
        "recording is already stopped";
    });
  RCLCPP_INFO(
    controller->get_logger(),
    "Ready: call /start_recording and /stop_recording services%s",
    recording_duration_sec > 0 ? "; each start stops after the configured duration" : "");

  executor.spin();

  executor.remove_node(recorder);
  executor.remove_node(encoder);
  executor.remove_node(camera);
  executor.remove_node(controller);
  stop_timer.reset();
  stop_service.reset();
  start_service.reset();
  recorder.reset();  // Writes the MP4 trailer before ROS is shut down.
  encoder.reset();
  camera.reset();
  controller.reset();
  rclcpp::shutdown();
  return 0;
}
