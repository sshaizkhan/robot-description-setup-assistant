cmake_minimum_required(VERSION 3.8)

# Find the robot_description_common package
find_package(robot_description_common REQUIRED)
set(PACKAGE_XML_PATH "${CMAKE_CURRENT_SOURCE_DIR}/package.xml")
set_package_version("${PACKAGE_XML_PATH}")

message(STATUS "Project version: ${PACKAGE_VERSION}")
project(robot_description_core_plugins VERSION ${PACKAGE_VERSION})
robot_description_package()

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_ros REQUIRED)
find_package(ament_index_cpp REQUIRED)

# find_package(moveit_setup_framework REQUIRED)
find_package(robot_description_setup_framework REQUIRED)
find_package(moveit_ros_visualization REQUIRED)
find_package(moveit_setup_framework REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(pluginlib REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rviz_common REQUIRED)
find_package(rviz_rendering REQUIRED)
find_package(yaml-cpp REQUIRED)

# Instruct CMake to run moc automatically when needed.
# set(CMAKE_AUTOMOC ON)
add_definitions(-DQT_NO_KEYWORDS)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

qt5_wrap_cpp(MOC_FILES
  include/robot_description_core_plugins/start_screen_widget.hpp
  include/robot_description_core_plugins/robot_selection_widget.hpp
)

add_library(${PROJECT_NAME}
  src/start_screen.cpp
  src/start_screen_widget.cpp
  src/robot_selection.cpp
  src/robot_selection_widget.cpp
  src/robot_config_manager.cpp
  ${MOC_FILES}
)

target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

ament_target_dependencies(
  ${PROJECT_NAME}
  ament_index_cpp
  pluginlib
  rclcpp
  Qt5Core
  Qt5Widgets
  rviz_common
  rviz_rendering
  robot_description_setup_framework
  moveit_ros_visualization
  moveit_setup_framework
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

install(FILES robot_description_setup_framework_plugins.xml
        DESTINATION share/${PROJECT_NAME}
)
install(DIRECTORY include/ DESTINATION include)

ament_export_targets(export_robot_description_core_plugins HAS_LIBRARY_TARGET)
install(
  TARGETS ${PROJECT_NAME}
  EXPORT export_robot_description_core_plugins
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES
  DESTINATION include
)

ament_export_include_directories(include)
ament_export_dependencies(rclcpp)
ament_export_dependencies(Qt5Core)
ament_export_dependencies(Qt5Widgets)
ament_export_dependencies(moveit_ros_visualization)
ament_export_dependencies(moveit_setup_framework)
ament_export_dependencies(robot_description_setup_framework)
ament_export_dependencies(rviz_common)
ament_export_dependencies(rviz_rendering)
ament_export_dependencies(rviz_default_plugins)

pluginlib_export_plugin_description_file(robot_description_setup_framework robot_description_setup_framework_plugins.xml)

ament_export_libraries(${PROJECT_NAME})
ament_export_targets(export_${PROJECT_NAME})

ament_package()
