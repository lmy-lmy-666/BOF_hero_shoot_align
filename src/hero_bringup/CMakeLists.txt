cmake_minimum_required(VERSION 3.16)
project(hero_bringup VERSION 0.1.0 LANGUAGES CXX)

find_package(ament_cmake REQUIRED)

install(DIRECTORY launch pcd
  DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY config
  DESTINATION share/${PROJECT_NAME}
  OPTIONAL
)

install(PROGRAMS scripts/lidar_frame_relay.py
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY rviz maps
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
