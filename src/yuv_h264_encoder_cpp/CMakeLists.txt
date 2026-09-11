cmake_minimum_required(VERSION 3.8)
project(yuv_h264_encoder_cpp)

find_package(ament_cmake REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
pkg_check_modules(FFMPEG_ENCODER REQUIRED IMPORTED_TARGET libavcodec libavutil libswscale)

if(NOT TARGET h264_interfaces__rosidl_typesupport_cpp)
  find_package(h264_interfaces REQUIRED)
endif()

add_library(yuyv_h264_encoder_node SHARED src/yuyv_h264_encoder.cpp)
add_library(${PROJECT_NAME}::yuyv_h264_encoder_node ALIAS yuyv_h264_encoder_node)
target_include_directories(yuyv_h264_encoder_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
ament_target_dependencies(yuyv_h264_encoder_node rclcpp sensor_msgs)
target_link_libraries(yuyv_h264_encoder_node PkgConfig::FFMPEG_ENCODER)
if(TARGET h264_interfaces__rosidl_typesupport_cpp)
  target_link_libraries(yuyv_h264_encoder_node h264_interfaces__rosidl_typesupport_cpp)
  add_dependencies(yuyv_h264_encoder_node h264_interfaces__rosidl_typesupport_fastrtps_cpp)
else()
  ament_target_dependencies(yuyv_h264_encoder_node h264_interfaces)
endif()

# The node implementation is a shared library. Do not require downstream users
# to recreate the build-only PkgConfig imported target.
set_property(TARGET yuyv_h264_encoder_node PROPERTY INTERFACE_LINK_LIBRARIES "")

add_executable(yuyv_h264_encoder src/main.cpp)
ament_target_dependencies(yuyv_h264_encoder rclcpp)
target_link_libraries(yuyv_h264_encoder yuyv_h264_encoder_node)

install(TARGETS yuyv_h264_encoder_node EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)
install(TARGETS yuyv_h264_encoder DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY include/ DESTINATION include)
ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(h264_interfaces rclcpp sensor_msgs)
ament_package()
