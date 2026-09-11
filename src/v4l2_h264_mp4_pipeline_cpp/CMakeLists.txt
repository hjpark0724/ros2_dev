cmake_minimum_required(VERSION 3.8)
project(v4l2_h264_mp4_pipeline_cpp)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_srvs REQUIRED)
if(NOT TARGET v4l2_camera_cpp::v4l2_yuyv_camera_node)
  find_package(v4l2_camera_cpp REQUIRED)
endif()
if(NOT TARGET yuv_h264_encoder_cpp::yuyv_h264_encoder_node)
  find_package(yuv_h264_encoder_cpp REQUIRED)
endif()
if(NOT TARGET h264_mp4_recorder_cpp::h264_mp4_recorder_node)
  find_package(h264_mp4_recorder_cpp REQUIRED)
endif()

add_executable(v4l2_h264_mp4_pipeline src/v4l2_h264_mp4_pipeline.cpp)
ament_target_dependencies(v4l2_h264_mp4_pipeline rclcpp std_srvs)
target_link_libraries(v4l2_h264_mp4_pipeline
  v4l2_camera_cpp::v4l2_yuyv_camera_node
  yuv_h264_encoder_cpp::yuyv_h264_encoder_node
  h264_mp4_recorder_cpp::h264_mp4_recorder_node)

# When CLion builds the workspace root it runs this target directly from the
# CMake build tree. rclcpp loads both generated Fast DDS and introspection
# type support with dlopen(), so build both libraries and add their directory
# through DT_RPATH.
if(TARGET h264_interfaces__rosidl_typesupport_fastrtps_cpp)
  add_dependencies(
    v4l2_h264_mp4_pipeline
    h264_interfaces__rosidl_typesupport_fastrtps_cpp
    h264_interfaces__rosidl_typesupport_introspection_cpp)
  set_property(
    TARGET v4l2_h264_mp4_pipeline
    APPEND PROPERTY BUILD_RPATH
    "$<TARGET_FILE_DIR:h264_interfaces__rosidl_typesupport_fastrtps_cpp>")
  if(UNIX AND NOT APPLE)
    target_link_options(v4l2_h264_mp4_pipeline PRIVATE "-Wl,--disable-new-dtags")
  endif()
endif()

install(TARGETS v4l2_h264_mp4_pipeline DESTINATION lib/${PROJECT_NAME})
ament_package()
