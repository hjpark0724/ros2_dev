#!/usr/bin/env bash
# Build the complete workspace from CLion or a terminal.
set -eo pipefail

workspace_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${workspace_root}/scripts/setup_ros2.sh"
cd "${workspace_root}"
colcon build --base-paths src --symlink-install "$@"
