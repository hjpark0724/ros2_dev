cmake_minimum_required(VERSION 3.8)
project(ros2_dev_cpp)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(hello_publisher src/hello_publisher.cpp)
ament_target_dependencies(hello_publisher rclcpp std_msgs)

install(TARGETS hello_publisher DESTINATION lib/${PROJECT_NAME})
ament_package()
