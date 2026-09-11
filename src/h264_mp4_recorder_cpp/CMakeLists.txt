cmake_minimum_required(VERSION 3.8)
project(h264_mp4_recorder_cpp)

find_package(ament_cmake REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(rclcpp REQUIRED)
pkg_check_modules(FFMPEG_RECORDER REQUIRED IMPORTED_TARGET libavcodec libavformat libavutil)

if(NOT TARGET h264_interfaces__rosidl_typesupport_cpp)
  find_package(h264_interfaces REQUIRED)
endif()

add_library(h264_mp4_recorder_node SHARED src/h264_mp4_recorder.cpp)
add_library(${PROJECT_NAME}::h264_mp4_recorder_node ALIAS h264_mp4_recorder_node)
target_include_directories(h264_mp4_recorder_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
ament_target_dependencies(h264_mp4_recorder_node rclcpp)
target_link_libraries(h264_mp4_recorder_node PkgConfig::FFMPEG_RECORDER)
if(TARGET h264_interfaces__rosidl_typesupport_cpp)
  target_link_libraries(h264_mp4_recorder_node h264_interfaces__rosidl_typesupport_cpp)
  add_dependencies(h264_mp4_recorder_node h264_interfaces__rosidl_typesupport_fastrtps_cpp)
else()
  ament_target_dependencies(h264_mp4_recorder_node h264_interfaces)
endif()

# The node implementation is a shared library. Do not require downstream users
# to recreate the build-only PkgConfig imported target.
set_property(TARGET h264_mp4_recorder_node PROPERTY INTERFACE_LINK_LIBRARIES "")

add_executable(h264_mp4_recorder src/main.cpp)
ament_target_dependencies(h264_mp4_recorder rclcpp)
target_link_libraries(h264_mp4_recorder h264_mp4_recorder_node)

install(TARGETS h264_mp4_recorder_node EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)
install(TARGETS h264_mp4_recorder DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY include/ DESTINATION include)
ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(h264_interfaces rclcpp)
ament_package()
