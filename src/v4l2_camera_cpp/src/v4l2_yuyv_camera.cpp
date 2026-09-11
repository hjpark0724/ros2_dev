#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/image_encodings.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "v4l2_camera_cpp/v4l2_yuyv_camera.hpp"

namespace
{

int xioctl(const int fd, const unsigned long request, void * argument)
{
  int result;
  do {
    result = ioctl(fd, request, argument);
  } while (result == -1 && errno == EINTR);
  return result;
}

struct MmapBuffer
{
  void * start{nullptr};
  size_t length{0};
};

}  // namespace

class V4l2YuyvCamera final : public rclcpp::Node
{
public:
  V4l2YuyvCamera()
  : Node("v4l2_yuyv_camera")
  {
    device_path_ = declare_parameter<std::string>("device", "/dev/video0");
    requested_width_ = declare_parameter<int>("width", 640);
    requested_height_ = declare_parameter<int>("height", 480);
    frame_id_ = declare_parameter<std::string>("frame_id", "camera_optical_frame");
    topic_name_ = declare_parameter<std::string>("topic", "camera/image_raw");

    publisher_ = create_publisher<sensor_msgs::msg::Image>(
      topic_name_, rclcpp::SensorDataQoS());

    try {
      configure_device();
    } catch (...) {
      close_device();
      throw;
    }

    poll_timer_ = create_wall_timer(std::chrono::milliseconds(1), [this] { capture_one(); });
    RCLCPP_INFO(
      get_logger(), "Capturing %ux%u YUYV from %s on %s", width_, height_, device_path_.c_str(),
      topic_name_.c_str());
  }

  ~V4l2YuyvCamera() override
  {
    close_device();
  }

private:
  void configure_device()
  {
    if (requested_width_ <= 0 || requested_height_ <= 0) {
      throw std::invalid_argument("width and height must be positive");
    }

    fd_ = open(device_path_.c_str(), O_RDWR | O_NONBLOCK);
    if (fd_ == -1) {
      throw std::system_error(errno, std::generic_category(), "open " + device_path_);
    }

    v4l2_capability capability{};
    if (xioctl(fd_, VIDIOC_QUERYCAP, &capability) == -1) {
      throw std::system_error(errno, std::generic_category(), "VIDIOC_QUERYCAP");
    }
    if ((capability.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0 ||
      (capability.capabilities & V4L2_CAP_STREAMING) == 0)
    {
      throw std::runtime_error("device does not support single-planar V4L2 streaming capture");
    }

    v4l2_format format{};
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = static_cast<uint32_t>(requested_width_);
    format.fmt.pix.height = static_cast<uint32_t>(requested_height_);
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    format.fmt.pix.field = V4L2_FIELD_NONE;
    if (xioctl(fd_, VIDIOC_S_FMT, &format) == -1) {
      throw std::system_error(errno, std::generic_category(), "VIDIOC_S_FMT");
    }
    if (format.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV) {
      throw std::runtime_error("camera did not accept V4L2_PIX_FMT_YUYV");
    }

    width_ = format.fmt.pix.width;
    height_ = format.fmt.pix.height;
    stride_ = std::max(format.fmt.pix.bytesperline, width_ * 2U);
    frame_bytes_ = static_cast<size_t>(stride_) * height_;

    v4l2_requestbuffers request{};
    request.count = 4;
    request.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    request.memory = V4L2_MEMORY_MMAP;
    if (xioctl(fd_, VIDIOC_REQBUFS, &request) == -1 || request.count < 2) {
      throw std::system_error(errno, std::generic_category(), "VIDIOC_REQBUFS");
    }

    buffers_.resize(request.count);
    for (uint32_t index = 0; index < request.count; ++index) {
      v4l2_buffer buffer{};
      buffer.type = request.type;
      buffer.memory = request.memory;
      buffer.index = index;
      if (xioctl(fd_, VIDIOC_QUERYBUF, &buffer) == -1) {
        throw std::system_error(errno, std::generic_category(), "VIDIOC_QUERYBUF");
      }
      buffers_[index].length = buffer.length;
      buffers_[index].start = mmap(nullptr, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, buffer.m.offset);
      if (buffers_[index].start == MAP_FAILED) {
        buffers_[index].start = nullptr;
        throw std::system_error(errno, std::generic_category(), "mmap V4L2 buffer");
      }
      queue_buffer(index);
    }

    auto type = v4l2_buf_type{V4L2_BUF_TYPE_VIDEO_CAPTURE};
    if (xioctl(fd_, VIDIOC_STREAMON, &type) == -1) {
      throw std::system_error(errno, std::generic_category(), "VIDIOC_STREAMON");
    }
    streaming_ = true;
  }

  void queue_buffer(const uint32_t index)
  {
    v4l2_buffer buffer{};
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = index;
    if (xioctl(fd_, VIDIOC_QBUF, &buffer) == -1) {
      throw std::system_error(errno, std::generic_category(), "VIDIOC_QBUF");
    }
  }

  void capture_one()
  {
    v4l2_buffer buffer{};
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    if (xioctl(fd_, VIDIOC_DQBUF, &buffer) == -1) {
      if (errno != EAGAIN) {
        RCLCPP_ERROR_THROTTLE(get_logger(), *get_clock(), 2000, "VIDIOC_DQBUF: %s", std::strerror(errno));
      }
      return;
    }

    if (buffer.index >= buffers_.size()) {
      RCLCPP_ERROR(get_logger(), "driver returned invalid buffer index %u", buffer.index);
      return;
    }

    try {
      if (buffer.bytesused < frame_bytes_) {
        RCLCPP_WARN(get_logger(), "dropping short frame: %u bytes, need %zu", buffer.bytesused, frame_bytes_);
      } else {
        sensor_msgs::msg::Image image;
        image.header.stamp = now();
        image.header.frame_id = frame_id_;
        image.width = width_;
        image.height = height_;
        image.encoding = sensor_msgs::image_encodings::YUYV;
        image.is_bigendian = false;
        image.step = stride_;
        const auto * begin = static_cast<const uint8_t *>(buffers_[buffer.index].start);
        image.data.assign(begin, begin + frame_bytes_);
        publisher_->publish(std::move(image));
      }
      queue_buffer(buffer.index);
    } catch (const std::exception & error) {
      RCLCPP_ERROR(get_logger(), "capturing frame failed: %s", error.what());
      try {
        queue_buffer(buffer.index);
      } catch (const std::exception & queue_error) {
        RCLCPP_ERROR(get_logger(), "failed to requeue V4L2 buffer: %s", queue_error.what());
      }
    }
  }

  void close_device()
  {
    if (fd_ == -1) {
      return;
    }
    if (streaming_) {
      auto type = v4l2_buf_type{V4L2_BUF_TYPE_VIDEO_CAPTURE};
      (void)xioctl(fd_, VIDIOC_STREAMOFF, &type);
      streaming_ = false;
    }
    for (const auto & buffer : buffers_) {
      if (buffer.start != nullptr) {
        (void)munmap(buffer.start, buffer.length);
      }
    }
    buffers_.clear();
    (void)close(fd_);
    fd_ = -1;
  }

  int fd_{-1};
  bool streaming_{false};
  std::string device_path_;
  std::string frame_id_;
  std::string topic_name_;
  int requested_width_{0};
  int requested_height_{0};
  uint32_t width_{0};
  uint32_t height_{0};
  uint32_t stride_{0};
  size_t frame_bytes_{0};
  std::vector<MmapBuffer> buffers_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr poll_timer_;
};

namespace v4l2_camera_cpp
{

rclcpp::Node::SharedPtr make_v4l2_yuyv_camera()
{
  return std::make_shared<V4l2YuyvCamera>();
}

}  // namespace v4l2_camera_cpp
