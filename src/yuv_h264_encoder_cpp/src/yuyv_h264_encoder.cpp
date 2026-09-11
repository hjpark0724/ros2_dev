#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include "h264_interfaces/msg/h264_packet.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/image_encodings.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "yuv_h264_encoder_cpp/yuyv_h264_encoder.hpp"

namespace
{

std::string ffmpeg_error(const int error)
{
  char buffer[AV_ERROR_MAX_STRING_SIZE]{};
  av_strerror(error, buffer, sizeof(buffer));
  return buffer;
}

}  // namespace

class YuyvH264Encoder final : public rclcpp::Node
{
public:
  YuyvH264Encoder()
  : Node("yuyv_h264_encoder")
  {
    input_topic_ = declare_parameter<std::string>("input_topic", "camera/image_raw");
    output_topic_ = declare_parameter<std::string>("output_topic", "camera/h264");
    bitrate_ = declare_parameter<int>("bitrate", 4'000'000);
    fps_ = declare_parameter<int>("fps", 30);
    gop_size_ = declare_parameter<int>("gop_size", 30);
    if (bitrate_ <= 0 || fps_ <= 0 || gop_size_ <= 0) {
      throw std::invalid_argument("bitrate, fps, and gop_size must be positive");
    }
    frame_interval_ns_ = 1'000'000'000LL / fps_;

    publisher_ = create_publisher<h264_interfaces::msg::H264Packet>(output_topic_, rclcpp::SensorDataQoS());
    subscription_ = create_subscription<sensor_msgs::msg::Image>(
      input_topic_, rclcpp::SensorDataQoS(),
      [this](sensor_msgs::msg::Image::ConstSharedPtr image) { encode(*image); });
  }

  ~YuyvH264Encoder() override
  {
    flush();
    if (frame_ != nullptr) {
      av_frame_free(&frame_);
    }
    if (codec_context_ != nullptr) {
      avcodec_free_context(&codec_context_);
    }
    if (sws_context_ != nullptr) {
      sws_freeContext(sws_context_);
    }
  }

private:
  void configure(const sensor_msgs::msg::Image & image)
  {
    if (image.width == 0 || image.height == 0 || image.step == 0) {
      throw std::invalid_argument("image dimensions and step must be non-zero");
    }
    if (image.encoding != sensor_msgs::image_encodings::YUYV &&
      image.encoding != sensor_msgs::image_encodings::UYVY)
    {
      throw std::invalid_argument("only yuyv and uyvy input encodings are supported");
    }

    input_format_ = image.encoding == sensor_msgs::image_encodings::YUYV ?
      AV_PIX_FMT_YUYV422 : AV_PIX_FMT_UYVY422;
    width_ = image.width;
    height_ = image.height;

    sws_context_ = sws_getContext(
      static_cast<int>(width_), static_cast<int>(height_), input_format_,
      static_cast<int>(width_), static_cast<int>(height_), AV_PIX_FMT_YUV420P,
      SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (sws_context_ == nullptr) {
      throw std::runtime_error("sws_getContext failed");
    }

    const AVCodec * codec = avcodec_find_encoder_by_name("libx264");
    if (codec == nullptr) {
      codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    }
    if (codec == nullptr) {
      throw std::runtime_error("no H.264 encoder is available in this FFmpeg installation");
    }
    codec_context_ = avcodec_alloc_context3(codec);
    if (codec_context_ == nullptr) {
      throw std::bad_alloc();
    }
    codec_context_->width = static_cast<int>(width_);
    codec_context_->height = static_cast<int>(height_);
    codec_context_->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_context_->time_base = AVRational{1, 1'000'000'000};
    codec_context_->framerate = AVRational{fps_, 1};
    codec_context_->bit_rate = bitrate_;
    codec_context_->gop_size = gop_size_;
    codec_context_->max_b_frames = 0;
    codec_context_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    av_opt_set(codec_context_->priv_data, "preset", "ultrafast", 0);
    av_opt_set(codec_context_->priv_data, "tune", "zerolatency", 0);
    const int open_result = avcodec_open2(codec_context_, codec, nullptr);
    if (open_result < 0) {
      throw std::runtime_error("avcodec_open2: " + ffmpeg_error(open_result));
    }

    codec_config_.assign(
      codec_context_->extradata, codec_context_->extradata + codec_context_->extradata_size);
    if (codec_config_.empty()) {
      throw std::runtime_error("H.264 encoder did not provide codec configuration");
    }

    frame_ = av_frame_alloc();
    if (frame_ == nullptr) {
      throw std::bad_alloc();
    }
    frame_->format = codec_context_->pix_fmt;
    frame_->width = codec_context_->width;
    frame_->height = codec_context_->height;
    const int buffer_result = av_frame_get_buffer(frame_, 32);
    if (buffer_result < 0) {
      throw std::runtime_error("av_frame_get_buffer: " + ffmpeg_error(buffer_result));
    }
    configured_ = true;
    RCLCPP_INFO(
      get_logger(), "Encoding %ux%u %s to H.264 at %d bit/s", width_, height_,
      image.encoding.c_str(), bitrate_);
  }

  void encode(const sensor_msgs::msg::Image & image)
  {
    try {
      if (!configured_) {
        configure(image);
      }
      const bool same_encoding =
        (input_format_ == AV_PIX_FMT_YUYV422 && image.encoding == sensor_msgs::image_encodings::YUYV) ||
        (input_format_ == AV_PIX_FMT_UYVY422 && image.encoding == sensor_msgs::image_encodings::UYVY);
      if (image.width != width_ || image.height != height_ || !same_encoding)
      {
        RCLCPP_ERROR(get_logger(), "image geometry or encoding changed; restart encoder to reconfigure H.264");
        return;
      }
      const size_t required_bytes = static_cast<size_t>(image.step) * image.height;
      if (image.data.size() < required_bytes) {
        RCLCPP_WARN(get_logger(), "dropping incomplete image: %zu bytes, need %zu", image.data.size(), required_bytes);
        return;
      }

      const int writable_result = av_frame_make_writable(frame_);
      if (writable_result < 0) {
        throw std::runtime_error("av_frame_make_writable: " + ffmpeg_error(writable_result));
      }
      const uint8_t * source_data[] = {image.data.data(), nullptr, nullptr, nullptr};
      const int source_stride[] = {static_cast<int>(image.step), 0, 0, 0};
      sws_scale(
        sws_context_, source_data, source_stride, 0, static_cast<int>(height_), frame_->data,
        frame_->linesize);

      const int64_t pts = rclcpp::Time(image.header.stamp).nanoseconds();
      frame_->pts = last_pts_ < 0 ? pts : std::max(pts, last_pts_ + frame_interval_ns_);
      last_pts_ = frame_->pts;
      const int send_result = avcodec_send_frame(codec_context_, frame_);
      if (send_result < 0) {
        throw std::runtime_error("avcodec_send_frame: " + ffmpeg_error(send_result));
      }
      drain_packets(image.header);
    } catch (const std::exception & error) {
      RCLCPP_ERROR(get_logger(), "H.264 encoding failed: %s", error.what());
    }
  }

  void drain_packets(const std_msgs::msg::Header & header)
  {
    AVPacket * packet = av_packet_alloc();
    if (packet == nullptr) {
      throw std::bad_alloc();
    }
    while (true) {
      const int receive_result = avcodec_receive_packet(codec_context_, packet);
      if (receive_result == AVERROR(EAGAIN) || receive_result == AVERROR_EOF) {
        break;
      }
      if (receive_result < 0) {
        av_packet_free(&packet);
        throw std::runtime_error("avcodec_receive_packet: " + ffmpeg_error(receive_result));
      }

      h264_interfaces::msg::H264Packet output;
      output.header = header;
      output.width = width_;
      output.height = height_;
      output.pts_ns = static_cast<uint64_t>(packet->pts);
      output.duration_ns = static_cast<uint64_t>(frame_interval_ns_);
      output.keyframe = (packet->flags & AV_PKT_FLAG_KEY) != 0;
      output.codec_config = codec_config_;
      output.data.assign(packet->data, packet->data + packet->size);
      publisher_->publish(std::move(output));
      av_packet_unref(packet);
    }
    av_packet_free(&packet);
  }

  void flush()
  {
    if (codec_context_ == nullptr || publisher_ == nullptr) {
      return;
    }
    if (avcodec_send_frame(codec_context_, nullptr) >= 0) {
      std_msgs::msg::Header header;
      header.stamp = now();
      try {
        drain_packets(header);
      } catch (const std::exception &) {
        // Destructors cannot report encoder flush errors safely.
      }
    }
  }

  std::string input_topic_;
  std::string output_topic_;
  int bitrate_{0};
  int fps_{0};
  int gop_size_{0};
  uint32_t width_{0};
  uint32_t height_{0};
  int64_t last_pts_{-1};
  int64_t frame_interval_ns_{0};
  AVPixelFormat input_format_{AV_PIX_FMT_NONE};
  bool configured_{false};
  SwsContext * sws_context_{nullptr};
  AVCodecContext * codec_context_{nullptr};
  AVFrame * frame_{nullptr};
  std::vector<uint8_t> codec_config_;
  rclcpp::Publisher<h264_interfaces::msg::H264Packet>::SharedPtr publisher_;
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr subscription_;
};

namespace yuv_h264_encoder_cpp
{

rclcpp::Node::SharedPtr make_yuyv_h264_encoder()
{
  return std::make_shared<YuyvH264Encoder>();
}

}  // namespace yuv_h264_encoder_cpp
