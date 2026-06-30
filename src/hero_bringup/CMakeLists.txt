cmake_minimum_required(VERSION 3.16)
project(hero_bringup VERSION 0.1.0 LANGUAGES CXX)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)

# C++ relay: fast, no GIL bottleneck at 18Hz pointcloud rates
add_executable(lidar_frame_relay src/lidar_frame_relay.cpp)
ament_target_dependencies(lidar_frame_relay rclcpp sensor_msgs)
install(TARGETS lidar_frame_relay DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY launch pcd
  DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY config
  DESTINATION share/${PROJECT_NAME}
  OPTIONAL
)

# Keep the Python relay as fallback (optional)
install(PROGRAMS scripts/lidar_frame_relay.py
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY rviz
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
