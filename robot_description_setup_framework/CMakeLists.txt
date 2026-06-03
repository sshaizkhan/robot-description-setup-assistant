cmake_minimum_required(VERSION 3.10)

find_package(robot_description_common REQUIRED)
set(PACKAGE_XML_PATH "${CMAKE_CURRENT_SOURCE_DIR}/package.xml")
set_package_version("${PACKAGE_XML_PATH}")

message(STATUS "Project version: ${PACKAGE_VERSION}")
project(robot_description_setup_framework VERSION ${PACKAGE_VERSION})

robot_description_package()
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_ros REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(robot_state_publisher REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(pluginlib REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rviz_common REQUIRED)
find_package(rviz_rendering REQUIRED)
find_package(rviz_default_plugins REQUIRED)

# find_package(srdfdom REQUIRED)
find_package(urdf REQUIRED)
find_package(sensor_msgs REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

qt5_wrap_cpp(MOC_FILES
  include/robot_description_setup_framework/qt/helper_widgets.hpp
  include/robot_description_setup_framework/qt/rviz_panel.hpp
  include/robot_description_setup_framework/qt/setup_step_widget.hpp
)

add_library(${PROJECT_NAME}
  src/utilities.cpp
  src/process_utils.cpp
  src/urdf_loader.cpp
  src/joint_state_zero_publisher.cpp
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
  robot_state_publisher
  tf2_ros
  pluginlib
  Qt5Core
  Qt5Widgets
  rclcpp
  rviz_common
  rviz_rendering
  rviz_default_plugins
  # srdfdom
  urdf
  sensor_msgs
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()

  find_package(ament_cmake_gtest REQUIRED)

  ament_add_gtest(test_app_context test/test_app_context.cpp)
  target_include_directories(test_app_context PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
  ament_target_dependencies(test_app_context rclcpp)

  ament_add_gtest(test_process_utils test/test_process_utils.cpp)
  target_include_directories(test_process_utils PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
  target_link_libraries(test_process_utils ${PROJECT_NAME})

  ament_add_gtest(test_urdf_loader test/test_urdf_loader.cpp)
  target_include_directories(test_urdf_loader PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
  target_compile_definitions(test_urdf_loader PRIVATE
    FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/fixtures")
  target_link_libraries(test_urdf_loader ${PROJECT_NAME})
  ament_target_dependencies(test_urdf_loader urdf)

  ament_add_gtest(test_joint_state_zero_publisher test/test_joint_state_zero_publisher.cpp)
  target_include_directories(test_joint_state_zero_publisher PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
  target_link_libraries(test_joint_state_zero_publisher ${PROJECT_NAME})
  ament_target_dependencies(test_joint_state_zero_publisher rclcpp sensor_msgs)
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
ament_export_dependencies(robot_state_publisher)
ament_export_dependencies(tf2_ros)
ament_export_dependencies(Qt5Widgets)
ament_export_dependencies(rviz_common)
ament_export_dependencies(rviz_rendering)
ament_export_dependencies(rviz_default_plugins)
ament_export_dependencies(sensor_msgs)

ament_package()
