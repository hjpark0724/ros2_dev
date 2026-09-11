#!/usr/bin/env bash
# Source this file from the workspace root after ROS 2 Jazzy is installed.
# Do not enable `set -e` here: sourced files share the caller's shell options.

if [[ -n "${ZSH_VERSION:-}" ]]; then
  setup_extension="zsh"
elif [[ -n "${BASH_VERSION:-}" ]]; then
  setup_extension="bash"
else
  setup_extension="sh"
fi

ros_setup="/opt/ros/jazzy/setup.${setup_extension}"
if [[ ! -f "${ros_setup}" ]]; then
  echo "ROS 2 Jazzy is not installed. Run ./bootstrap_ros2_jazzy.sh first." >&2
  return 1
fi

source "${ros_setup}"
# This script is documented to be sourced from the workspace root.  PWD works
# in both Bash and zsh, whereas BASH_SOURCE is Bash-specific.
workspace_root="$PWD"
workspace_setup="${workspace_root}/install/setup.${setup_extension}"
if [[ -f "${workspace_setup}" ]]; then
  source "${workspace_setup}"
fi

export ROS_DOMAIN_ID="${ROS_DOMAIN_ID:-0}"
export RMW_IMPLEMENTATION="${RMW_IMPLEMENTATION:-rmw_fastrtps_cpp}"
