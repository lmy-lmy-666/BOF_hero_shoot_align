cmake_minimum_required(VERSION 3.16)
project(hero_localization VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Dependencies ---
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(tf2_eigen REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(visualization_msgs REQUIRED)
find_package(hero_interfaces REQUIRED)
find_package(hero_core REQUIRED)

# PCL (for relocalization)
# MPI must be found before PCL/VTK on Ubuntu 24.04 to populate MPI::MPI_C
find_package(MPI REQUIRED)
find_package(PCL REQUIRED COMPONENTS common io filters)
find_package(pcl_conversions REQUIRED)
find_package(pcl_ros REQUIRED)

# --- Library: shared across nodes ---
add_library(${PROJECT_NAME}_lib SHARED
  src/loam_adapter_node.cpp
  src/relocalization_node.cpp
  src/target_computer_node.cpp
)

target_include_directories(${PROJECT_NAME}_lib
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(${PROJECT_NAME}_lib
  PUBLIC
    ${pcl_conversions_INCLUDE_DIRS}
    ${pcl_ros_INCLUDE_DIRS}
    ${nav_msgs_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}_lib
  PUBLIC
    rclcpp::rclcpp
    rclcpp_components::component
    tf2::tf2
    tf2_ros::tf2_ros
    tf2_geometry_msgs::tf2_geometry_msgs
    tf2_eigen::tf2_eigen
    nav_msgs::nav_msgs__rosidl_typesupport_cpp
    hero_interfaces::hero_interfaces__rosidl_typesupport_cpp
    visualization_msgs::visualization_msgs__rosidl_typesupport_cpp
    hero_core::hero_core
    ${PCL_LIBRARIES}
    ${pcl_conversions_LIBRARIES}
    ${pcl_ros_LIBRARIES}
)

# Register as component (enables composition)
rclcpp_components_register_node(${PROJECT_NAME}_lib
  PLUGIN "hero_localization::LoamAdapterNode"
  EXECUTABLE loam_adapter
)

rclcpp_components_register_node(${PROJECT_NAME}_lib
  PLUGIN "hero_localization::RelocalizationNode"
  EXECUTABLE relocalization
)

rclcpp_components_register_node(${PROJECT_NAME}_lib
  PLUGIN "hero_localization::TargetComputerNode"
  EXECUTABLE target_computer
)

# --- Install ---
install(TARGETS ${PROJECT_NAME}_lib
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY config launch
  DESTINATION share/${PROJECT_NAME}
)

ament_export_dependencies(
  rclcpp
  tf2
  tf2_ros
  hero_interfaces
  hero_core
)

ament_package()
