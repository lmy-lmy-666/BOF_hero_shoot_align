#!/bin/bash
# =============================================================================
# Install dependencies for hero_localization — ROS 2 Jazzy (Ubuntu 24.04)
# =============================================================================
# Differences from Humble:
#   - Package prefix changes from ros-humble-* to ros-jazzy-*
#   - tf2_eigen may ship as part of tf2 (no separate package)
#   - PCL and GTest package names may differ slightly
set -e

echo "=== Installing ROS 2 Jazzy system dependencies ==="

# Base ROS 2 packages
sudo apt install -y \
  ros-jazzy-rclcpp \
  ros-jazzy-rclcpp-components \
  ros-jazzy-tf2 \
  ros-jazzy-tf2-ros \
  ros-jazzy-tf2-geometry-msgs \
  ros-jazzy-nav-msgs \
  ros-jazzy-sensor-msgs \
  ros-jazzy-geometry-msgs \
  ros-jazzy-xacro \
  ros-jazzy-robot-state-publisher \
  ros-jazzy-rosidl-default-generators

# tf2_eigen: in Jazzy this is typically bundled with tf2_ros or tf2,
# but check if a standalone package exists.
sudo apt install -y ros-jazzy-tf2-eigen 2>/dev/null || \
  echo "Note: ros-jazzy-tf2-eigen not found as standalone (may be bundled)"

# PCL (for relocalization)
sudo apt install -y \
  libpcl-dev

# GTest
sudo apt install -y \
  libgtest-dev

# small_gicp
if [ ! -d "small_gicp" ]; then
  echo "=== Cloning small_gicp ==="
  git clone https://github.com/koide3/small_gicp.git
fi

echo ""
echo "=== Done. Now build with colcon: ==="
echo "  cd /path/to/hero_shoot_ws"
echo "  colcon build --symlink-install"
echo "  source install/setup.bash"
