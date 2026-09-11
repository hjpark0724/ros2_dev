#!/usr/bin/env bash
# Installs ROS 2 Jazzy and the standard C++ development tools on Ubuntu 24.04.
set -euo pipefail

if [[ "${EUID}" -eq 0 ]]; then
  echo "Run this script as a regular user; it requests sudo only when needed." >&2
  exit 1
fi

if ! . /etc/os-release || [[ "${ID:-}" != "ubuntu" || "${VERSION_ID:-}" != "24.04" ]]; then
  echo "This bootstrap script supports Ubuntu 24.04 only." >&2
  exit 1
fi

sudo apt-get update
sudo apt-get install -y curl ca-certificates software-properties-common
sudo add-apt-repository -y universe

sudo curl -fsSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key \
  -o /usr/share/keyrings/ros-archive-keyring.gpg
echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu $(. /etc/os-release && echo "$UBUNTU_CODENAME") main" \
  | sudo tee /etc/apt/sources.list.d/ros2.list >/dev/null

sudo apt-get update
sudo apt-get upgrade -y
sudo apt-get install -y \
  ros-jazzy-desktop \
  ros-dev-tools \
  python3-argcomplete \
  python3-rosdep \
  python3-vcstool \
  build-essential \
  cmake \
  gdb

if [[ ! -e /etc/ros/rosdep/sources.list.d/20-default.list ]]; then
  sudo rosdep init
fi
rosdep update

echo
echo "ROS 2 Jazzy installation complete. Open a new shell, then run:"
echo "  cd $(pwd)"
echo "  source scripts/setup_ros2.sh"
echo "  colcon build --symlink-install"
