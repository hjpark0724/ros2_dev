cmake_minimum_required(VERSION 3.8)
project(ros2_service_client_cpp)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(example_interfaces REQUIRED)
find_package(rclcpp REQUIRED)

if(NOT TARGET add_two_float_interfaces__rosidl_typesupport_cpp)
  find_package(add_two_float_interfaces REQUIRED)
endif()

add_executable(add_two_ints_client src/add_two_ints_client.cpp)
ament_target_dependencies(add_two_ints_client example_interfaces rclcpp)

add_executable(add_two_floats_client src/add_two_floats_client.cpp)
ament_target_dependencies(add_two_floats_client rclcpp)
if(TARGET add_two_float_interfaces__rosidl_typesupport_cpp)
  target_link_libraries(
    add_two_floats_client
    add_two_float_interfaces__rosidl_typesupport_cpp)
  add_dependencies(
    add_two_floats_client
    add_two_float_interfaces__rosidl_typesupport_fastrtps_cpp)
  set_property(
    TARGET add_two_floats_client
    APPEND PROPERTY BUILD_RPATH
    "$<TARGET_FILE_DIR:add_two_float_interfaces__rosidl_typesupport_fastrtps_cpp>")
  if(UNIX AND NOT APPLE)
    target_link_options(add_two_floats_client PRIVATE "-Wl,--disable-new-dtags")
  endif()
else()
  ament_target_dependencies(add_two_floats_client add_two_float_interfaces)
endif()

install(TARGETS add_two_ints_client add_two_floats_client DESTINATION lib/${PROJECT_NAME})

ament_package()
