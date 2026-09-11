cmake_minimum_required(VERSION 3.8)
project(ros2_service_server_cpp)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(example_interfaces REQUIRED)
find_package(rclcpp REQUIRED)

# colcon resolves this package through its install prefix. The workspace root
# adds all packages as CMake subdirectories, where the generated target is
# already available instead.
if(NOT TARGET add_two_float_interfaces__rosidl_typesupport_cpp)
  find_package(add_two_float_interfaces REQUIRED)
endif()

add_executable(add_two_ints_server src/add_two_ints_server.cpp)
ament_target_dependencies(add_two_ints_server example_interfaces rclcpp)

add_executable(add_two_floats_server src/add_two_floats_server.cpp)
ament_target_dependencies(add_two_floats_server rclcpp)
if(TARGET add_two_float_interfaces__rosidl_typesupport_cpp)
  target_link_libraries(
    add_two_floats_server
    add_two_float_interfaces__rosidl_typesupport_cpp)

  # CLion runs this executable directly from its CMake build directory. Add
  # the generated Fast DDS type-support library directory to its runtime
  # search path, as it has not been installed into a sourced ROS prefix.
  add_dependencies(
    add_two_floats_server
    add_two_float_interfaces__rosidl_typesupport_fastrtps_cpp)
  set_property(
    TARGET add_two_floats_server
    APPEND PROPERTY BUILD_RPATH
    "$<TARGET_FILE_DIR:add_two_float_interfaces__rosidl_typesupport_fastrtps_cpp>")
  if(UNIX AND NOT APPLE)
    # Use DT_RPATH rather than DT_RUNPATH so rclcpp's dlopen() can also find
    # the generated type-support library.
    target_link_options(add_two_floats_server PRIVATE "-Wl,--disable-new-dtags")
  endif()
else()
  ament_target_dependencies(add_two_floats_server add_two_float_interfaces)
endif()

install(TARGETS add_two_ints_server add_two_floats_server DESTINATION lib/${PROJECT_NAME})

ament_package()
