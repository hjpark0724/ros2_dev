#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/mem.h>
}

#include "h264_interfaces/msg/h264_packet.hpp"
#include "rclcpp/rclcpp.hpp"
#include "h264_mp4_recorder_cpp/h264_mp4_recorder.hpp"

namespace
{

std::string ffmpeg_error(const int error)
{
  char buffer[AV_ERROR_MAX_STRING_SIZE]{};
  av_strerror(error, buffer, sizeof(buffer));
  return buffer;
}

}  // namespace

class H264Mp4Recorder final : public rclcpp::Node
{
public:
  explicit H264Mp4Recorder(std::shared_ptr<h264_mp4_recorder_cpp::RecordingControl> recording_control)
  : Node("h264_mp4_recorder")
  , recording_control_(std::move(recording_control))
  {
    input_topic_ = declare_parameter<std::string>("input_topic", "camera/h264");
    output_path_ = declare_parameter<std::string>("output", "recording.mp4");
    subscription_ = create_subscription<h264_interfaces::msg::H264Packet>(
      input_topic_, rclcpp::SensorDataQoS(),
      [this](h264_interfaces::msg::H264Packet::ConstSharedPtr packet) { write_packet(*packet); });
    control_timer_ = create_wall_timer(
      std::chrono::milliseconds(20), [this] { update_recording_state(); });
  }

  ~H264Mp4Recorder() override
  {
    close_output();
  }

private:
  void update_recording_state()
  {
    const bool should_record = recording_control_->recording_enabled();
    if (!should_record && opened_) {
      close_output();
      RCLCPP_INFO(get_logger(), "Recording stopped");
    }
  }

  void open_output(const h264_interfaces::msg::H264Packet & input)
  {
    if (input.width == 0 || input.height == 0 || input.codec_config.empty()) {
      throw std::invalid_argument("first packet needs dimensions and H.264 codec configuration");
    }
    int result = avformat_alloc_output_context2(&format_context_, nullptr, nullptr, output_path_.c_str());
    if (result < 0 || format_context_ == nullptr) {
      throw std::runtime_error("avformat_alloc_output_context2: " + ffmpeg_error(result));
    }
    stream_ = avformat_new_stream(format_context_, nullptr);
    if (stream_ == nullptr) {
      throw std::runtime_error("avformat_new_stream failed");
    }
    stream_->time_base = AVRational{1, 1'000'000'000};
    stream_->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    stream_->codecpar->codec_id = AV_CODEC_ID_H264;
    stream_->codecpar->codec_tag = 0;
    stream_->codecpar->width = static_cast<int>(input.width);
    stream_->codecpar->height = static_cast<int>(input.height);
    stream_->codecpar->format = AV_PIX_FMT_YUV420P;
    stream_->codecpar->extradata_size = static_cast<int>(input.codec_config.size());
    stream_->codecpar->extradata = static_cast<uint8_t *>(
      av_mallocz(input.codec_config.size() + AV_INPUT_BUFFER_PADDING_SIZE));
    if (stream_->codecpar->extradata == nullptr) {
      throw std::bad_alloc();
    }
    std::memcpy(stream_->codecpar->extradata, input.codec_config.data(), input.codec_config.size());

    if ((format_context_->oformat->flags & AVFMT_NOFILE) == 0) {
      result = avio_open(&format_context_->pb, output_path_.c_str(), AVIO_FLAG_WRITE);
      if (result < 0) {
        throw std::runtime_error("avio_open: " + ffmpeg_error(result));
      }
    }
    result = avformat_write_header(format_context_, nullptr);
    if (result < 0) {
      throw std::runtime_error("avformat_write_header: " + ffmpeg_error(result));
    }
    width_ = input.width;
    height_ = input.height;
    first_pts_ns_ = input.pts_ns;
    opened_ = true;
    RCLCPP_INFO(get_logger(), "Writing %ux%u H.264 stream to %s", width_, height_, output_path_.c_str());
  }

  void write_packet(const h264_interfaces::msg::H264Packet & input)
  {
    try {
      if (!recording_control_->recording_enabled()) {
        return;
      }
      if (input.data.empty()) {
        return;
      }
      if (!opened_ && !input.keyframe) {
        return;
      }
      if (!opened_) {
        open_output(input);
      }
      if (input.width != width_ || input.height != height_) {
        RCLCPP_ERROR(get_logger(), "dropping packet after video dimensions changed from %ux%u to %ux%u", width_, height_, input.width, input.height);
        return;
      }

      AVPacket packet{};
      const int allocate_result = av_new_packet(&packet, static_cast<int>(input.data.size()));
      if (allocate_result < 0) {
        throw std::runtime_error("av_new_packet: " + ffmpeg_error(allocate_result));
      }
      std::memcpy(packet.data, input.data.data(), input.data.size());
      packet.stream_index = stream_->index;
      packet.pts = static_cast<int64_t>(input.pts_ns - first_pts_ns_);
      packet.dts = packet.pts;
      packet.duration = static_cast<int64_t>(input.duration_ns);
      if (input.keyframe) {
        packet.flags |= AV_PKT_FLAG_KEY;
      }
      const int write_result = av_interleaved_write_frame(format_context_, &packet);
      av_packet_unref(&packet);
      if (write_result < 0) {
        throw std::runtime_error("av_interleaved_write_frame: " + ffmpeg_error(write_result));
      }
    } catch (const std::exception & error) {
      RCLCPP_ERROR(get_logger(), "MP4 write failed: %s", error.what());
      close_output();
    }
  }

  void close_output()
  {
    if (format_context_ == nullptr) {
      return;
    }
    if (opened_) {
      (void)av_write_trailer(format_context_);
    }
    if ((format_context_->oformat->flags & AVFMT_NOFILE) == 0 && format_context_->pb != nullptr) {
      (void)avio_closep(&format_context_->pb);
    }
    avformat_free_context(format_context_);
    format_context_ = nullptr;
    stream_ = nullptr;
    opened_ = false;
  }

  std::string input_topic_;
  std::string output_path_;
  uint32_t width_{0};
  uint32_t height_{0};
  uint64_t first_pts_ns_{0};
  bool opened_{false};
  AVFormatContext * format_context_{nullptr};
  AVStream * stream_{nullptr};
  std::shared_ptr<h264_mp4_recorder_cpp::RecordingControl> recording_control_;
  rclcpp::Subscription<h264_interfaces::msg::H264Packet>::SharedPtr subscription_;
  rclcpp::TimerBase::SharedPtr control_timer_;
};

namespace h264_mp4_recorder_cpp
{

rclcpp::Node::SharedPtr make_h264_mp4_recorder(std::shared_ptr<RecordingControl> recording_control)
{
  if (recording_control == nullptr) {
    recording_control = std::make_shared<RecordingControl>();
  }
  return std::make_shared<H264Mp4Recorder>(std::move(recording_control));
}

}  // namespace h264_mp4_recorder_cpp
