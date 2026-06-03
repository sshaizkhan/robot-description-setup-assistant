cmake_minimum_required(VERSION 3.10)

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

find_package(robot_description_setup_framework REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(pluginlib REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rviz_common REQUIRED)
find_package(rviz_rendering REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(YAMLCPP yaml-cpp)

# Instruct CMake to run moc automatically when needed.
# set(CMAKE_AUTOMOC ON)
add_definitions(-DQT_NO_KEYWORDS)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

qt5_wrap_cpp(MOC_FILES
  include/robot_description_core_plugins/start_screen_widget.hpp
  include/robot_description_core_plugins/robot_selection_widget.hpp
  include/robot_description_core_plugins/robot_config_manager.hpp        # NEW
  include/robot_description_core_plugins/robot_specification_widget.hpp # NEW
  include/robot_description_core_plugins/filter_widget.hpp              # NEW
)

add_library(${PROJECT_NAME}
  src/start_screen.cpp
  src/start_screen_widget.cpp
  src/robot_selection.cpp
  src/robot_selection_widget.cpp
  src/robot_config_manager.cpp          # NEW
  src/robot_specification_widget.cpp    # NEW
  src/filter_widget.cpp                 # NEW
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
  yaml-cpp
)

# Optional: Build test executable
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
  
  # Test executable for advanced features
  add_executable(test_advanced_features
    test/test_advanced_features.cpp
    ${MOC_FILES}
  )
  
  target_include_directories(test_advanced_features PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  )
  
  target_link_libraries(test_advanced_features ${PROJECT_NAME})
  
  ament_target_dependencies(test_advanced_features
    rclcpp
    Qt5Core
    Qt5Widgets
  )

  install(TARGETS test_advanced_features
    DESTINATION lib/test_advanced_features
)
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
install(DIRECTORY config DESTINATION share/${PROJECT_NAME})
install(DIRECTORY test DESTINATION share/${PROJECT_NAME})



ament_export_include_directories(include)
ament_export_dependencies(rclcpp)
ament_export_dependencies(Qt5Core)
ament_export_dependencies(Qt5Widgets)
ament_export_dependencies(robot_description_setup_framework)
ament_export_dependencies(rviz_common)
ament_export_dependencies(rviz_rendering)
ament_export_dependencies(rviz_default_plugins)

pluginlib_export_plugin_description_file(robot_description_setup_framework robot_description_setup_framework_plugins.xml)

ament_export_libraries(${PROJECT_NAME})
ament_export_targets(export_${PROJECT_NAME})

ament_package()
