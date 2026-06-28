cmake_minimum_required(VERSION 3.16)
project(hero_core VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# If building via colcon, ament_cmake is available.
# If building standalone (no ROS), fall back to plain CMake.
find_package(ament_cmake QUIET)

# --- Header-only library ---
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME}
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# --- Install ---
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}Targets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(ament_cmake_FOUND)
  install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  )
  ament_export_include_directories(include)
  ament_export_targets(${PROJECT_NAME}Targets)
else()
  install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  )
endif()

# --- Unit tests (standalone gtest, no ROS) ---
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
  enable_testing()
  find_package(GTest REQUIRED)

  add_executable(test_target_computer test/test_target_computer.cpp)
  target_link_libraries(test_target_computer
    PRIVATE ${PROJECT_NAME} GTest::gtest GTest::gtest_main)
  add_test(NAME test_target_computer COMMAND test_target_computer)

  add_executable(test_coordinate_align test/test_coordinate_align.cpp)
  target_link_libraries(test_coordinate_align
    PRIVATE ${PROJECT_NAME} GTest::gtest GTest::gtest_main)
  add_test(NAME test_coordinate_align COMMAND test_coordinate_align)
endif()

if(ament_cmake_FOUND)
  ament_package()
endif()
