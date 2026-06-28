#!/bin/bash
# =============================================================================
# Install dependencies for hero_localization — ROS 2 Humble (Ubuntu 22.04)
# =============================================================================
set -e

echo "=== Installing ROS 2 Humble system dependencies ==="

# Base ROS 2 packages
sudo apt install -y \
  ros-humble-rclcpp \
  ros-humble-rclcpp-components \
  ros-humble-tf2 \
  ros-humble-tf2-ros \
  ros-humble-tf2-geometry-msgs \
  ros-humble-tf2-eigen \
  ros-humble-nav-msgs \
  ros-humble-sensor-msgs \
  ros-humble-geometry-msgs \
  ros-humble-xacro \
  ros-humble-robot-state-publisher \
  ros-humble-rosidl-default-generators

# PCL (for relocalization)
sudo apt install -y \
  libpcl-dev \
  ros-humble-pcl-conversions \
  ros-humble-pcl-ros

# GTest (for hero_core unit tests)
sudo apt install -y \
  libgtest-dev

# small_gicp (header-only GICP library for relocalization)
# Option A: clone into workspace src/
if [ ! -d "src/small_gicp" ]; then
  echo "=== Cloning small_gicp ==="
  git clone https://github.com/koide3/small_gicp.git src/small_gicp
fi

echo ""
echo "=== Done. Now build with colcon: ==="
echo "  cd /path/to/hero_shoot_ws"
echo "  colcon build --symlink-install"
echo "  source install/setup.bash"
