cmake_minimum_required(VERSION 3.22)
project(ros2_dev_workspace NONE)

# This file is for CLion's project model.  ROS 2 packages remain independently
# buildable with colcon from this workspace root.
file(GLOB workspace_package_cmake_lists
  RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*/CMakeLists.txt"
)
list(SORT workspace_package_cmake_lists)

# The integrated pipeline depends on the camera, encoder, and recorder packages.
# Keep it last so the CLion project model can resolve their in-workspace targets.
set(pipeline_package_cmake_list "src/v4l2_h264_mp4_pipeline_cpp/CMakeLists.txt")
list(REMOVE_ITEM workspace_package_cmake_lists "${pipeline_package_cmake_list}")

if(NOT workspace_package_cmake_lists)
  message(FATAL_ERROR "No ROS 2 packages were found under ${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()

foreach(package_cmake_list IN LISTS workspace_package_cmake_lists)
  get_filename_component(package_directory "${package_cmake_list}" DIRECTORY)
  add_subdirectory("${package_directory}")
endforeach()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${pipeline_package_cmake_list}")
  get_filename_component(pipeline_package_directory "${pipeline_package_cmake_list}" DIRECTORY)
  add_subdirectory("${pipeline_package_directory}")
endif()

# Select this target in CLion when a normal ROS 2 workspace build is required.
add_custom_target(colcon_build
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/colcon_build.sh"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  USES_TERMINAL
  COMMENT "Building ROS 2 workspace with colcon"
)
