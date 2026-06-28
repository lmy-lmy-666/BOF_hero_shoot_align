cmake_minimum_required(VERSION 3.16)
project(hero_description VERSION 0.1.0 LANGUAGES CXX)

find_package(ament_cmake REQUIRED)

# Install URDF files
install(DIRECTORY urdf
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
