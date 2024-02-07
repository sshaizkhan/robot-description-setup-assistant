cmake_minimum_required(VERSION 3.8)

find_package(robot_description_common REQUIRED)
set(PACKAGE_XML_PATH "${CMAKE_CURRENT_SOURCE_DIR}/package.xml")
set_package_version("${PACKAGE_XML_PATH}")

message(STATUS "Project version: ${PACKAGE_VERSION}")
project(robot_description_setup_framework VERSION ${PACKAGE_VERSION})

robot_description_package()
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_ros REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(moveit_ros_planning REQUIRED)
find_package(moveit_ros_visualization REQUIRED)
find_package(moveit_core REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(pluginlib REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rviz_common REQUIRED)
find_package(rviz_rendering REQUIRED)
find_package(rviz_default_plugins REQUIRED)

# find_package(srdfdom REQUIRED)
find_package(urdf REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

qt5_wrap_cpp(MOC_FILES
  include/robot_description_setup_framework/qt/helper_widgets.hpp
  include/robot_description_setup_framework/qt/rviz_panel.hpp
  include/robot_description_setup_framework/qt/setup_step_widget.hpp
)

add_library(${PROJECT_NAME}
  src/utilities.cpp
  src/helper_widgets.cpp
  src/rviz_panel.cpp
  ${MOC_FILES}
)
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
ament_target_dependencies(${PROJECT_NAME}
  ament_index_cpp
  moveit_core
  moveit_ros_planning
  moveit_ros_visualization
  pluginlib
  Qt5Core
  Qt5Widgets
  rclcpp
  rviz_common
  rviz_rendering
  rviz_default_plugins
  # srdfdom
  urdf
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

install(DIRECTORY include/
  DESTINATION include
)

ament_export_targets(export_robot_description_setup_framework HAS_LIBRARY_TARGET)

install(TARGETS ${PROJECT_NAME}
  EXPORT export_robot_description_setup_framework
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

ament_export_include_directories(include)
ament_export_dependencies(rclcpp)
ament_export_dependencies(Qt5Core)
ament_export_dependencies(moveit_core)
ament_export_dependencies(moveit_ros_planning)
ament_export_dependencies(moveit_ros_visualization)
ament_export_dependencies(Qt5Widgets)
ament_export_dependencies(rviz_common)
ament_export_dependencies(rviz_rendering)
ament_export_dependencies(rviz_default_plugins)

ament_package()
