cmake_minimum_required(VERSION 3.8)
project(v4l2_camera_cpp)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)

add_library(v4l2_yuyv_camera_node SHARED src/v4l2_yuyv_camera.cpp)
add_library(${PROJECT_NAME}::v4l2_yuyv_camera_node ALIAS v4l2_yuyv_camera_node)
target_include_directories(v4l2_yuyv_camera_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
ament_target_dependencies(v4l2_yuyv_camera_node rclcpp sensor_msgs)

add_executable(v4l2_yuyv_camera src/main.cpp)
target_link_libraries(v4l2_yuyv_camera v4l2_yuyv_camera_node)

install(TARGETS v4l2_yuyv_camera_node EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)
install(TARGETS v4l2_yuyv_camera DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY include/ DESTINATION include)
ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(rclcpp sensor_msgs)
ament_package()
